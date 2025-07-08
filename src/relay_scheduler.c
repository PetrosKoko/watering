#include "relay_scheduler.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include "esp_log.h"
#include <stdlib.h>  // 👈 Needed for malloc()

#define RELAY_GPIO GPIO_NUM_25
static const char *TAG = "RELAY";

static void watering_task(void *param) {
    schedule_config_t *cfg = (schedule_config_t *)param;

    ESP_LOGI(TAG, "Scheduler task running: %02d:%02d for %d min",
             cfg->watering_hour, cfg->watering_minute, cfg->duration_minutes);

    static int last_triggered_day = -1;

    while (true) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        // ESP_LOGI(TAG, "Now: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        if (timeinfo.tm_hour == cfg->watering_hour &&
            timeinfo.tm_min == cfg->watering_minute &&
            timeinfo.tm_mday != last_triggered_day) {

            last_triggered_day = timeinfo.tm_mday;

            ESP_LOGI(TAG, "Activating relay");
            gpio_set_level(RELAY_GPIO, 0);  // ON (active-low)
            vTaskDelay(pdMS_TO_TICKS(cfg->duration_minutes * 60000));
            gpio_set_level(RELAY_GPIO, 1);  // OFF
            ESP_LOGI(TAG, "Relay deactivated");
            last_triggered_day = -1;
        }

        // ESP_LOGI(TAG, "Schedule: %02d:%02d for %d min",
        //          cfg->watering_hour, cfg->watering_minute, cfg->duration_minutes);
        // ESP_LOGI(TAG, "Tick: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

        vTaskDelay(pdMS_TO_TICKS(1000));  // check every second
    }

    // If the task ever exits (unlikely), free the heap copy
    free(cfg);
}

void start_scheduler(const schedule_config_t *cfg) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(RELAY_GPIO, 1);  // OFF initially (active-low)

    // ✳️ Allocate a persistent copy of the config
    schedule_config_t *cfg_copy = malloc(sizeof(schedule_config_t));
    if (!cfg_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for schedule config.");
        return;
    }
    *cfg_copy = *cfg;  // Copy the contents

    ESP_LOGI(TAG, "Scheduling task activated.");

    xTaskCreatePinnedToCore(
        watering_task,
        "watering_task",
        4096,
        (void *)cfg_copy,
        5,
        NULL,
        0
    );
}
