#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "zmct103c.h"

static const char *TAG = "ZMCT103C";
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static zmct_config_t current_config;

esp_err_t zmct103c_init(const zmct_config_t *config) {
    if (config == NULL) return ESP_ERR_INVALID_ARG;
    
    // Guardamos la config localmente
    memcpy(&current_config, config, sizeof(zmct_config_t));

    // 1. Configurar la Unidad ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1, // Usamos ADC1 siempre (el 2 choca con WiFi)
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. Configurar el Canal Específico
    adc_oneshot_chan_cfg_t config_chan = {
        .bitwidth = ZMCT_ADC_WIDTH,
        .atten = ZMCT_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, (adc_channel_t)config->adc_channel, &config_chan));

    ESP_LOGI(TAG, "ZMCT103C Inicializado en canal %d. Offset esperado: %.2f V", config->adc_channel, config->offset_volts);
    return ESP_OK;
}

float zmct103c_read_rms(void) {
    if (adc1_handle == NULL) {
        ESP_LOGE(TAG, "ADC no inicializado");
        return -1.0;
    }

    int raw_val = 0;
    unsigned long sum_squares = 0;
    int num_samples = 200; // Muestras suficientes para capturar varios ciclos de 50Hz
    
    // Muestreo rápido (Burst)
    for (int i = 0; i < num_samples; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, (adc_channel_t)current_config.adc_channel, &raw_val));
        
        // Convertir raw (0-4095) a voltaje aproximado (0-3.3V)
        // Nota: Para precisión extrema se debería usar "esp_adc_cal", pero para esto basta la lineal
        // ADC 12 bits -> 4096 pasos. 3.3V referencia.
        float voltage = (raw_val * 3.3f) / 4095.0f;
        
        // Restar el offset DC (el punto medio de la onda sinusoidal)
        float current_inst = voltage - current_config.offset_volts;
        
        // Suma de cuadrados (evita negativos)
        sum_squares += (current_inst * current_inst);
        
        // Pequeño delay para no saturar y espaciar muestras (ajustar según frecuencia de muestreo deseada)
        // ets_delay_us(500); 
    }

    // Calculo RMS: Raíz del Promedio de los Cuadrados
    float mean_square = sum_squares / (float)num_samples;
    float voltage_rms = sqrtf(mean_square);

    // Convertir Voltaje RMS a Amperes usando la sensibilidad del sensor
    // Si la sensibilidad es 100mV/A (0.1V/A) -> Amperes = Vrms / 0.1
    float amperes = voltage_rms / current_config.sensitivity;

    // Filtro de ruido (Zero Clamp): Si es muy bajo, es ruido
    if (amperes < 0.05) amperes = 0.0;

    return amperes;
}
