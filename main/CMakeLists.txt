idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver nvs_flash esp_wifi json ds18b20 mqtt_connector wifi_portal zmct103c)
