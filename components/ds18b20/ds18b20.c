#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rom/ets_sys.h" // Para ets_delay_us

// Tiempos del protocolo OneWire (en microsegundos)
#define TIME_RESET 480
#define TIME_WAIT  60
#define TIME_SLOT  60

static void _write_bit(gpio_num_t pin, int bit) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(bit ? 1 : TIME_SLOT); // 1us para escribir 1, 60us para escribir 0
    gpio_set_level(pin, 1); // Soltar bus (pull-up externo trabaja)
    ets_delay_us(bit ? TIME_SLOT : 1);
}

static int _read_bit(gpio_num_t pin) {
    int bit = 0;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(1); // Hold min 1us
    gpio_set_direction(pin, GPIO_MODE_INPUT); // Soltar bus
    ets_delay_us(10); // Esperar validez
    bit = gpio_get_level(pin);
    ets_delay_us(50); // Completar slot
    return bit;
}

static void _write_byte(gpio_num_t pin, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        _write_bit(pin, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t _read_byte(gpio_num_t pin) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (_read_bit(pin)) byte |= (1 << i);
    }
    return byte;
}

static bool _reset(gpio_num_t pin) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(TIME_RESET);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    ets_delay_us(TIME_WAIT);
    int presence = gpio_get_level(pin);
    ets_delay_us(TIME_RESET);
    return (presence == 0);
}

ds18b20_handle_t ds18b20_init(gpio_num_t gpio_num) {
    ds18b20_handle_t handle = { .gpio_num = gpio_num };
    gpio_reset_pin(gpio_num);
    // IMPORTANTE: El Pull-up interno del ESP32 es débil (45k), poné resistencia externa de 4.7k
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY); 
    return handle;
}

float ds18b20_read_temp(ds18b20_handle_t handle) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
    // Sección crítica para tiempos precisos (deshabilita interrupciones brevemente)
    portENTER_CRITICAL(&mux);
    bool present = _reset(handle.gpio_num);
    portEXIT_CRITICAL(&mux);

    if (!present) return -999.0;

    portENTER_CRITICAL(&mux);
    _write_byte(handle.gpio_num, 0xCC); // Skip ROM (hablamos a todos en el bus, que es solo 1)
    _write_byte(handle.gpio_num, 0x44); // Convert T
    portEXIT_CRITICAL(&mux);

    // Esperar conversión (750ms max para 12 bits). Usamos vTaskDelay para no bloquear CPU.
    vTaskDelay(pdMS_TO_TICKS(750));

    portENTER_CRITICAL(&mux);
    _reset(handle.gpio_num);
    _write_byte(handle.gpio_num, 0xCC); // Skip ROM
    _write_byte(handle.gpio_num, 0xBE); // Read Scratchpad
    
    uint8_t low = _read_byte(handle.gpio_num);
    uint8_t high = _read_byte(handle.gpio_num);
    portEXIT_CRITICAL(&mux);

    int16_t raw = (high << 8) | low;
    return (float)raw / 16.0;
}
