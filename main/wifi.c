#include "wifi.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define WIFI_SCAN_INTERVAL_MS 30000
#define MAX_AP_RECORDS 20

static const char *TAG = "wifi";
static wifi_ap_record_t ap_records[MAX_AP_RECORDS];

static void wifi_scan_task(void *arg)
{
    (void)arg;

    while (1) {
        ESP_LOGI(TAG, "Scanning for Wi-Fi networks...");
        ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

        uint16_t total_aps = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&total_aps));

        uint16_t record_count = MAX_AP_RECORDS;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_count, ap_records));

        ESP_LOGI(TAG, "Found %u network(s); showing %u",
                 (unsigned)total_aps, (unsigned)record_count);
        for (uint16_t i = 0; i < record_count; i++) {
            const wifi_ap_record_t *record = &ap_records[i];
            const char *security =
                record->authmode == WIFI_AUTH_OPEN ? "open" : "secured";

            ESP_LOGI(TAG, "%u. SSID: %.*s | RSSI: %d dBm | channel: %u | %s (auth=%d)",
                     (unsigned)(i + 1), (int)(sizeof(record->ssid) - 1U),
                     (const char *)record->ssid, record->rssi,
                     (unsigned)record->primary, security, (int)record->authmode);
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_SCAN_INTERVAL_MS));
    }
}

esp_err_t wifi_start(void)
{
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        return err;
    }

    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGE(TAG, "Failed to create the default Wi-Fi station interface");
        return ESP_FAIL;
    }

    const wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_config);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    if (xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create the Wi-Fi scan task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
