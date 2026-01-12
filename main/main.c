#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/adc.h"

// Componentes Propios
#include "wifi_portal.h"    // (Del repo original)
#include "mqtt_connector.h" // (Del repo original)
#include "ds18b20.h"
#include "zmct103c.h"

// --- CONFIGURACIÓN DE PINES Y PARÁMETROS ---
// Diego: Verificá estos pines en tu placa antes de soldar
#define PIN_DS18B20_AIRE       GPIO_NUM_21
#define PIN_DS18B20_SERP       GPIO_NUM_22
#define PIN_DS18B20_ESTATICO   GPIO_NUM_23
#define PIN_AMPERIMETRO        ADC_CHANNEL_6 // GPIO 34 (Input Only)

// Umbrales para lógica de Hielo
#define ALERTA_DELTA_T         15.0  // Si (Aire - Serp) > 15°C, algo anda mal
#define TEMP_AIRE_ALTA         8.0   // Aire caliente
#define TEMP_SERP_CONGELADA    -18.0 // Serpentina muy fría

static const char *TAG = "HELADERA_MAIN";

// Variables globales (thread-safe por atomicidad de float en 32bit o usar mutex)
float g_temp_aire = -999.0;
float g_temp_serp = -999.0;
float g_temp_est  = -999.0;
float g_corriente = 0.0;

// --- FUNCIÓN DE DIAGNÓSTICO ---
void diagnostico_inteligente() {
    // 1. Detección de Bloqueo por Hielo (Evaporador Forzado)
    // Síntoma: La serpentina enfría mucho (-20), pero el aire no baja (se queda en 10).
    // El hielo actúa de aislante térmico.
    if (g_temp_serp < TEMP_SERP_CONGELADA && g_temp_aire > TEMP_AIRE_ALTA) {
        ESP_LOGE(TAG, "!!! ALERTA CRÍTICA: BLOQUEO POR HIELO DETECTADO !!!");
           if (mqtt_app_is_connected()) {
               mqtt_app_publish("gaddbar/heladera/alerta", "{\"msg\": \"BLOQUEO_HIELO\", \"prioridad\": \"ALTA\"}");
        }
    }

    // 2. Detección de Ciclo Continuo (Relé pegado o falta de gas)
    // Si la corriente indica encendido por más de 4 horas (esto requeriría un contador de tiempo real)
    // Por ahora, solo logueamos estado.
    if (g_corriente > 0.5) {
        ESP_LOGI(TAG, "Motor en marcha: %.2f A", g_corriente);
    } else {
        ESP_LOGI(TAG, "Motor detenido (Standby)");
    }
}

// --- TAREA PRINCIPAL DE MONITOREO ---
void task_sensores(void *pvParameters) {
    // 1. Setup Sensores de Temperatura
    ds18b20_handle_t s_aire = ds18b20_init(PIN_DS18B20_AIRE);
    ds18b20_handle_t s_serp = ds18b20_init(PIN_DS18B20_SERP);
    ds18b20_handle_t s_est  = ds18b20_init(PIN_DS18B20_ESTATICO);

    // 2. Setup Amperímetro
    // Configura la sensibilidad de tu módulo ZMCT (mV/A). 
    // Muchos módulos chinos sacan voltaje analógico directo. Ajustar 'sensitivity' probando con una lamparita de 100W.
    // Ejemplo: Si con 1A lees 0.5V extra -> sensitivity = 0.5
    zmct_config_t z_cfg = {
        .adc_channel = PIN_AMPERIMETRO,
        .sensitivity = 0.185, // Ejemplo para módulos ACS712 o similares, ajustar para ZMCT transimpedancia
        .offset_volts = 1.65  // Punto medio para 3.3V
    };
    if (zmct103c_init(&z_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Fallo inicialización ADC ZMCT. Abortando tarea.");
        vTaskDelete(NULL);
    }

    // Bucle Infinito
    while (1) {
        // Lecturas (DS18B20 tarda ~750ms cada una si es secuencial, o paralela si optimizas)
        g_temp_aire = ds18b20_read_temp(s_aire);
        g_temp_serp = ds18b20_read_temp(s_serp);
        g_temp_est  = ds18b20_read_temp(s_est);
        
        // Lectura de corriente (toma ~100ms)
        g_corriente = zmct103c_read_rms();

        ESP_LOGI(TAG, "Sensores -> Aire: %.1f C | Serp: %.1f C | Est: %.1f C | I: %.2f A", 
                 g_temp_aire, g_temp_serp, g_temp_est, g_corriente);

        // Correr lógica
        diagnostico_inteligente();

        // Enviar MQTT
        if (mqtt_app_is_connected()) {
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "t_aire", g_temp_aire);
            cJSON_AddNumberToObject(json, "t_serp", g_temp_serp);
            cJSON_AddNumberToObject(json, "t_est", g_temp_est);
            cJSON_AddNumberToObject(json, "amp", g_corriente);
            
            char *payload = cJSON_PrintUnformatted(json);
            mqtt_app_publish("gaddbar/heladera/telemetria", payload);
            
            free(payload);
            cJSON_Delete(json);
        }

        // Esperar 10 segundos antes de la próxima medición
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    // Inicialización del Sistema
    ESP_LOGI(TAG, "Arrancando Monitor Heladera - Arquitectura Gadd");
    
    // NVS Flash (Necesario para WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Iniciar WiFi y MQTT (Usando tus módulos existentes)
    wifi_portal_init(); 
    mqtt_app_start();

    // Crear Tarea de Sensores en el Core 1 (Application Core)
    xTaskCreatePinnedToCore(task_sensores, "Task_Sensores", 4096, NULL, 5, NULL, 1);
}
