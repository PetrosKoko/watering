#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "storage.h"
#include "esp_system.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

extern const char *TAG;

static const char *HTML_TEMPLATE =
"<!DOCTYPE html><html><head>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<meta http-equiv='Cache-Control' content='no-store' />"
"<meta http-equiv='Pragma' content='no-cache' />"
"<meta http-equiv='Expires' content='0' />"
"<script defer>function sendSchedule(){"
"var t=document.getElementById('start_time').value;"
"var d=document.getElementById('duration').value;"
"if(!t||!d){alert('Fill in both fields.');return;}"
"var x=new XMLHttpRequest();"
"x.open('POST','/submit',true);"
"x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"
"x.onload=function(){alert(x.status==200?'Schedule saved!':'Error: '+x.status);};"
"x.send('start_time='+encodeURIComponent(t)+'&duration='+encodeURIComponent(d));"
"}</script>"
"<style>body{font-family:sans-serif;background:#eef;padding:0;margin:0}"
".box{max-width:360px;margin:40px auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 0 8px #aaa}"
"h2{text-align:center}label{display:block;margin-top:12px;font-weight:600}"
"input,button{width:100%%;padding:10px;margin-top:6px;border-radius:5px;border:1px solid #ccc;font-size:16px}"
"button{background:#28a745;color:#fff;border:none;font-weight:bold;cursor:pointer}"
"</style></head><body><div class='box'>"
"<h2>Irrigation Scheduler</h2>"
"<p style='text-align:center'>Schedule: %s</p>"
"<p style='text-align:center'>Server Time: %s</p>"
"<label for='start_time'>Start Time:</label><input type='time' id='start_time'>"
"<label for='duration'>Duration (min):</label><input type='number' id='duration' min='1' max='240'>"
"<button onclick='sendSchedule()'>Save</button>"
"</div></body></html>";



static esp_err_t root_get_handler(httpd_req_t *req) {
    schedule_config_t cfg;
    load_schedule_config(&cfg);

    char schedule[32];
    get_schedule_string(&cfg, schedule, sizeof(schedule));

    // 🕒 Format server time
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char current_time[64];
    strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // 📄 Fill in HTML template
    char response[2048];
    int len = snprintf(response, sizeof(response), HTML_TEMPLATE, schedule, current_time);
    if (len >= sizeof(response)) {
        ESP_LOGW("WEB", "HTML page truncated — consider enlarging response buffer.");
    }

    // 🛑 Disable caching
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static esp_err_t submit_post_handler(httpd_req_t *req) {
    char buf[200];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf) - 1));
    if (ret <= 0) {
        httpd_resp_send_408(req);
        return ESP_FAIL;
    }

    buf[ret] = '\0';  // null-terminate safely

    int hour = 0, minute = 0, duration = 0;
    char *start_time = strstr(buf, "start_time=");
    char *duration_str = strstr(buf, "duration=");

    if (start_time && duration_str) {
        sscanf(start_time + 11, "%d%%3A%d", &hour, &minute);  // Handle URL-encoded ":"
        sscanf(duration_str + 9, "%d", &duration);

        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && duration > 0) {
            schedule_config_t cfg = {
                .watering_hour = hour,
                .watering_minute = minute,
                .duration_minutes = duration
            };
            save_schedule_config(&cfg);

            httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);  // ✅ Send response first
            vTaskDelay(pdMS_TO_TICKS(500));  // 💤 allow browser to process response
            esp_restart();  // 💡 THEN reboot

            return ESP_OK;
        }
    }

    httpd_resp_send(req, "Invalid input", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_web_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_start(&server, &config);
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root);

    httpd_uri_t submit = {
        .uri = "/submit",
        .method = HTTP_POST,
        .handler = submit_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &submit);
}
