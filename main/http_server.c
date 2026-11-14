#include "http_server.h"

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "led.h"

static const char *TAG = "http_server";
static httpd_handle_t server;

static const char index_html[] =
    "<!doctype html>"
    "<html lang=\"sk\"><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>ESP32 LED</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:32rem;margin:3rem auto;padding:0 1rem}"
    "button{font-size:1.2rem;padding:.7rem 1.2rem;margin:.5rem}"
    "</style></head><body><h1>Ovládanie LED</h1>"
    "<button id=\"led-on\" type=\"button\">Zapnúť</button>"
    "<button id=\"led-off\" type=\"button\">Vypnúť</button>"
    "<p id=\"status\" aria-live=\"polite\">Pripravené</p>"
    "<script>"
    "async function setLed(state){"
    "const status=document.getElementById('status');"
    "status.textContent='Odosielam...';"
    "try{"
    "const response=await fetch('/led/'+state,{method:'POST'});"
    "if(!response.ok)throw new Error('HTTP '+response.status);"
    "status.textContent=state==='on'?'LED je zapnutá.':'LED je vypnutá.';"
    "}catch(error){"
    "status.textContent='Chyba pri ovládaní LED: '+error.message;"
    "}"
    "}"
    "document.getElementById('led-on').addEventListener('click',()=>setLed('on'));"
    "document.getElementById('led-off').addEventListener('click',()=>setLed('off'));"
    "</script>"
    "</body></html>";

static esp_err_t index_handler(httpd_req_t *request)
{
    esp_err_t err = httpd_resp_set_type(request, "text/html; charset=utf-8");
    if (err != ESP_OK) {
        return err;
    }
    return httpd_resp_send(request, index_html, HTTPD_RESP_USE_STRLEN);
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
