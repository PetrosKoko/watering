#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <stdio.h>

#define TAG "STORAGE"

bool load_schedule_config(schedule_config_t *cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("sched", NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    int32_t hour, minute, duration;
    bool need_defaults = false;

    if (nvs_get_i32(handle, "hour", &hour) != ESP_OK) {
        hour = 6; need_defaults = true;
    }
    if (nvs_get_i32(handle, "minute", &minute) != ESP_OK) {
        minute = 30; need_defaults = true;
    }
    if (nvs_get_i32(handle, "duration", &duration) != ESP_OK) {
        duration = 15; need_defaults = true;
    }

    if (need_defaults) {
       // ESP_LOG_W(TAG, "Writing default config: 06:30 for 15 min");
        nvs_set_i32(handle, "hour", hour);
        nvs_set_i32(handle, "minute", minute);
        nvs_set_i32(handle, "duration", duration);
        nvs_commit(handle);
    }

    cfg->watering_hour = hour;
    cfg->watering_minute = minute;
    cfg->duration_minutes = duration;
    nvs_close(handle);
    return true;
}

void save_schedule_config(const schedule_config_t *cfg) {
    nvs_handle_t handle;
    nvs_open("sched", NVS_READWRITE, &handle);
    nvs_set_i32(handle, "hour", cfg->watering_hour);
    nvs_set_i32(handle, "minute", cfg->watering_minute);
    nvs_set_i32(handle, "duration", cfg->duration_minutes);
    nvs_commit(handle);
    nvs_close(handle);
}

void get_schedule_string(const schedule_config_t *cfg, char *out, size_t len) {
    snprintf(out, len, "%02d:%02d for %d min", 
             cfg->watering_hour, cfg->watering_minute, cfg->duration_minutes);
}
