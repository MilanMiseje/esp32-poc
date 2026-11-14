#include "http_server.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "led.h"

static const char *TAG = "http_server";
static httpd_handle_t server;
extern const char index_html_start[] asm("_binary_index_html_start");

static esp_err_t index_handler(httpd_req_t *request)
{
    esp_err_t err = httpd_resp_set_type(request, "text/html; charset=utf-8");
    if (err != ESP_OK) {
        return err;
    }
    return httpd_resp_send(request, index_html_start, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t set_led_from_request(httpd_req_t *request, bool is_on)
{
    esp_err_t err = led_set_state(is_on);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LED state: %s", esp_err_to_name(err));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to set LED state");
    }

    err = httpd_resp_set_status(request, "204 No Content");
    if (err != ESP_OK) {
        return err;
    }
    return httpd_resp_send(request, NULL, 0);
}

static esp_err_t led_on_handler(httpd_req_t *request)
{
    return set_led_from_request(request, true);
}

static esp_err_t led_off_handler(httpd_req_t *request)
{
    return set_led_from_request(request, false);
}

static esp_err_t brightness_get_handler(httpd_req_t *request)
{
    uint8_t percent;
    esp_err_t err = led_get_brightness(&percent);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LED brightness: %s", esp_err_to_name(err));
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to get LED brightness");
    }

    char response[4];
    const int response_length = snprintf(response, sizeof(response), "%u",
                                         (unsigned)percent);
    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "Failed to format LED brightness");
    }

    err = httpd_resp_set_type(request, "text/plain");
    if (err != ESP_OK) {
        return err;
    }
    return httpd_resp_send(request, response, response_length);
}

static esp_err_t websocket_send_text(httpd_req_t *request, uint8_t *payload,
                                     size_t length)
{
    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = payload,
        .len = length,
    };
    return httpd_ws_send_frame(request, &frame);
}

static esp_err_t websocket_handler(httpd_req_t *request)
{
    httpd_ws_frame_t frame = {0};
    esp_err_t err = httpd_ws_recv_frame(request, &frame, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WebSocket frame length: %s", esp_err_to_name(err));
        return err;
    }

    if (frame.len > 16) {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t payload[17] = {0};
    frame.payload = payload;
    err = httpd_ws_recv_frame(request, &frame, sizeof(payload) - 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WebSocket payload: %s", esp_err_to_name(err));
        return err;
    }

    if (frame.type != HTTPD_WS_TYPE_TEXT || frame.len == 0 || frame.len > 3) {
        static uint8_t error[] = "ERR:range";
        return websocket_send_text(request, error, sizeof(error) - 1);
    }

    unsigned percent = 0;
    for (size_t i = 0; i < frame.len; i++) {
        if (payload[i] < '0' || payload[i] > '9') {
            static uint8_t error[] = "ERR:range";
            return websocket_send_text(request, error, sizeof(error) - 1);
        }
        percent = percent * 10U + (unsigned)(payload[i] - '0');
    }
    if (percent > 100) {
        static uint8_t error[] = "ERR:range";
        return websocket_send_text(request, error, sizeof(error) - 1);
    }

    err = led_set_brightness((uint8_t)percent);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LED brightness: %s", esp_err_to_name(err));
        static uint8_t error[] = "ERR:led";
        return websocket_send_text(request, error, sizeof(error) - 1);
    }

    uint8_t response[7];
    const int response_length = snprintf((char *)response, sizeof(response),
                                         "OK:%u", percent);
    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        return ESP_FAIL;
    }
    return websocket_send_text(request, response, (size_t)response_length);
}

static const httpd_uri_t handlers[] = {
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
    },
    {
        .uri = "/led/on",
        .method = HTTP_POST,
        .handler = led_on_handler,
    },
    {
        .uri = "/led/off",
        .method = HTTP_POST,
        .handler = led_off_handler,
    },
    {
        .uri = "/led/brightness",
        .method = HTTP_GET,
        .handler = brightness_get_handler,
    },
    {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .is_websocket = true,
    },
};

esp_err_t http_server_start(void)
{
    if (server != NULL) {
        return ESP_OK;
    }

    const httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        return err;
    }

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
        err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) {
            const esp_err_t stop_err = httpd_stop(server);
            server = NULL;
            if (stop_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to stop HTTP server after route registration error: %s",
                         esp_err_to_name(stop_err));
            }
            return err;
        }
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
