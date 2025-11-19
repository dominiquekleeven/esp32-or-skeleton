#ifndef CORE_H
#define CORE_H

#include <stdint.h>

/**
 * @brief Event source enumeration
 * 
 * @param EVENT_SOURCE_WIFI The WiFi event source
 * @param EVENT_SOURCE_MQTT The MQTT event source
 * @param EVENT_SOURCE_UART The UART event source
 */
typedef enum {
    EVENT_SOURCE_WIFI,
    EVENT_SOURCE_MQTT,
    EVENT_SOURCE_UART
} event_source_t;

/**
 * @brief Event structure
 * 
 * @param source The source of the event
 * @param type The type of the event
 * @param data The data of the event
 */
typedef struct {
    event_source_t source;
    uint32_t type;
    void *data;
} app_event_t;

#endif

