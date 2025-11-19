idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES core esp_wifi nvs_flash esp_netif)

