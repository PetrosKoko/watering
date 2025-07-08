#include "nvs_flash.h"
#include "wifi_manager.h"
#include "time_sync.h"
#include "relay_scheduler.h"
#include "storage.h"
#include "web_server.h"
#include "esp_log.h"
#include "esp_wifi.h"

#define RELAY_GPIO GPIO_NUM_25

static const char *TAG = "APP_MAIN";

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    schedule_config_t cfg;
    bool has_config = load_schedule_config(&cfg);

    if (!has_config) {
        start_wifi_ap_mode();
        start_web_server();
    } else {
        start_wifi_sta_mode();
        if (sync_time_with_ntp()) {
            esp_wifi_stop();          // Stop STA mode
            start_wifi_ap_mode();     // Switch to AP
            start_web_server();       // Serve dashboard or status page
        } else {
            ESP_LOGE("MAIN", "Failed to sync time, staying in STA mode.");
        }

        
    }
    start_scheduler(&cfg);
}
