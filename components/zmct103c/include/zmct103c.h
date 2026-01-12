#pragma once
#include "esp_err.h"
#include "hal/adc_types.h"

// Configuración del ADC (Atenuación de 11dB para leer hasta ~3.1V)
#define ZMCT_ADC_ATTEN ADC_ATTEN_DB_12 
#define ZMCT_ADC_WIDTH ADC_BITWIDTH_DEFAULT

typedef struct {
    int adc_channel;       // Canal ADC1 (ej: ADC_CHANNEL_6 para GPIO34)
    float sensitivity;     // Sensibilidad (ajustar según calibración)
    float offset_volts;    // Punto cero (usualmente VCC/2 = 1.65V o 2.5V según módulo)
} zmct_config_t;

/**
 * @brief Inicializa el ADC Oneshot unit
 * @return ESP_OK si sale todo bien
 */
esp_err_t zmct103c_init(const zmct_config_t *config);

/**
 * @brief Lee la corriente RMS (Root Mean Square) muestreando varios ciclos de red.
 * BLOQUEANTE: Tarda aprox 100ms (5 ciclos de 50Hz)
 * @return Corriente en Amperes (float)
 */
float zmct103c_read_rms(void);
