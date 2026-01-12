#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

// Estructura para manejar instancias independientes
typedef struct {
    gpio_num_t gpio_num;
} ds18b20_handle_t;

/**
 * @brief Inicializa un pin para sensor DS18B20
 */
ds18b20_handle_t ds18b20_init(gpio_num_t gpio_num);

/**
 * @brief Lee la temperatura de UN sensor conectado a ese pin.
 * Asume topología de 1 sensor por pin (Skip ROM).
 * @return Temperatura en Celsius o -999.0 si hay error.
 */
float ds18b20_read_temp(ds18b20_handle_t handle);
