#include "sdkconfig.h"
#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static QueueHandle_t s_event_queue = NULL;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY          5


/**
 * @brief Event handler for the WiFi station
 * 
 * @param arg The argument to the event handler
 * @param event_base The base event type
 * @param event_id The event ID
 * @param event_data The event data
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    app_event_t app_event;
    app_event.source = EVENT_SOURCE_WIFI;
    app_event.data = NULL;
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connecting to AP");
            if (s_event_queue && s_retry_num > 1) {
                app_event.type = EVENT_WIFI_DISCONNECTED;
                xQueueSend(s_event_queue, &app_event, 0);
            }
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            if (s_event_queue) {
                app_event.type = EVENT_WIFI_FAILED;
                xQueueSend(s_event_queue, &app_event, 0);
            }
        }
        ESP_LOGI(TAG, "Connect to AP failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_event_queue) {
            app_event.type = EVENT_WIFI_CONNECTED;
            xQueueSend(s_event_queue, &app_event, 0);
        }
    }
}

/**
 * @brief Initialize the WiFi station
 * 
 * @param event_queue The event queue to send events to
 * @return ESP_OK if successful, otherwise ESP_FAIL
 */
esp_err_t wifi_init_sta(QueueHandle_t event_queue)
{
    s_event_queue = event_queue;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &event_handler,
                                                         NULL,
                                                         &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                         IP_EVENT_STA_GOT_IP,
                                                         &event_handler,
                                                         NULL,
                                                         &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init finished. Connecting to %s", CONFIG_WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", CONFIG_WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", CONFIG_WIFI_SSID);
        return ESP_FAIL;
    }

    return ESP_ERR_TIMEOUT;
}

