/**
 * @file adc_driver.h
 * @brief ADC driver for sensor readings
 *
 * This module provides ADC driver functionality for reading
 * analog sensor values.
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC channel configuration
 */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_MAX
} adc_channel_t;

/**
 * @brief Initialize ADC driver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t adc_driver_init(void);

/**
 * @brief Read ADC value from specified channel
 * 
 * @param channel ADC channel to read
 * @param value Pointer to store the read value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t adc_driver_read(adc_channel_t channel, uint32_t *value);

/**
 * @brief Deinitialize ADC driver
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t adc_driver_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DRIVER_H */
