#include "time_sync.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>

static const char *TAG = "TIME_SYNC";

bool sync_time_with_ntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    // Primary NTP server
 sntp_setservername(0, "pool.ntp.org");
sntp_setservername(1, "129.6.15.28");
sntp_setservername(2, "time.google.com");

    sntp_init();

  

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 30;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        // Set timezone
        setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
        tzset();
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        ESP_LOGI(TAG, "Time synced: %s", asctime(&timeinfo));
        return true;
    } else {
        ESP_LOGW(TAG, "Failed to sync time.");
        return false;
    }
}