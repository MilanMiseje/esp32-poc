#include "wifi.h"

#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

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

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to configured Wi-Fi network");
        const esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start Wi-Fi connection: %s", esp_err_to_name(err));
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event = event_data;
        ESP_LOGW(TAG, "Wi-Fi disconnected (reason %d); reconnecting", event->reason);
        const esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reconnect to Wi-Fi: %s", esp_err_to_name(err));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = event_data;
        ESP_LOGI(TAG, "Connected; open http://" IPSTR "/", IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_start(void)
{
    const size_t ssid_length = strlen(CONFIG_POC_WIFI_SSID);
    const size_t password_length = strlen(CONFIG_POC_WIFI_PASSWORD);
    wifi_config_t station_config = {0};

    if (ssid_length == 0 ||
        ssid_length > sizeof(station_config.sta.ssid) ||
        password_length > sizeof(station_config.sta.password)) {
        ESP_LOGE(TAG, "Set a valid SSID and password in the ESP32 POC Wi-Fi menuconfig options");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(station_config.sta.ssid, CONFIG_POC_WIFI_SSID, ssid_length);
    memcpy(station_config.sta.password, CONFIG_POC_WIFI_PASSWORD, password_length);

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

    err = esp_wifi_set_config(WIFI_IF_STA, &station_config);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                     wifi_event_handler, NULL);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    return err;
}

esp_err_t wifi_scan_start(void)
{
    if (xTaskCreate(wifi_scan_task, "wifi_scan", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create the Wi-Fi scan task");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
