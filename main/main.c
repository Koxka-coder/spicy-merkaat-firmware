/**
 * @file main.c
 * @brief Main application entry point for Spicy Merkaat Firmware
 *
 * This is the main entry point for the ESP32 firmware that coordinates
 * sensor drivers, zone FSM, MQTT client, ESP-WIFI-MESH integration and OTA updates.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "MAIN";

/**
 * @brief Main application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Spicy Merkaat Firmware starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "System initialized successfully");
    
    // TODO: Initialize components
    // - Zone FSM
    // - MQTT Client
    // - Mesh Bridge
    // - ADC Driver
    // - NVS Config
    // - OTA Manager
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
