#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "core.h"

typedef enum {
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_WIFI_FAILED
} wifi_app_event_t;

esp_err_t wifi_init_sta(QueueHandle_t event_queue);

#endif

