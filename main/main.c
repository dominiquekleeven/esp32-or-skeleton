#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core.h"
#include "wifi.h"

static const char *TAG = "main";

typedef enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_ERROR,
    STATE_READY
} app_state_t;


/**
 * @brief Reducer function for the application state
 * 
 * @param state The current state of the application
 * @param event The event to process
 * @return The new state of the application
 */
static app_state_t reducer(app_state_t state, const app_event_t *event)
{
    if (event->source == EVENT_SOURCE_WIFI) {
        wifi_app_event_t wifi_event = (wifi_app_event_t)event->type;
        
        switch (state) {
            case STATE_INIT:
            case STATE_CONNECTING:
                if (wifi_event == EVENT_WIFI_CONNECTED) {
                    ESP_LOGI(TAG, "STATE: WiFi connected");
                    return STATE_READY;
                } else if (wifi_event == EVENT_WIFI_FAILED) {
                    ESP_LOGE(TAG, "STATE: WiFi connection failed");
                    return STATE_ERROR;
                }
                break;  
            case STATE_READY:
                break;        
            case STATE_ERROR:
                break;
        }
    }
    
    return state;
}

/**
 * @brief Main function for the application
 * 
 * @return void
 */
void app_main(void)
{
    app_state_t state = STATE_INIT;
    QueueHandle_t event_queue = xQueueCreate(10, sizeof(app_event_t));
    
    ESP_LOGI(TAG, "Application started");
    
    // Set initial state and initialize WiFi station
    state = STATE_CONNECTING;
    esp_err_t ret = wifi_init_sta(event_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        state = STATE_ERROR;
    }
    
    // Main loop
    while (1) {
        app_event_t event;
        
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE) {
            state = reducer(state, &event);
        }
        
        switch (state) {
            case STATE_READY:
                // Do normal operation tasks here
                // E.g. listen to mqtt, uart frames, etc.
                break;
                
            case STATE_ERROR:
                ESP_LOGE(TAG, "System in error state");
                vTaskDelay(pdMS_TO_TICKS(5000));
                break;
                
            default:
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}
