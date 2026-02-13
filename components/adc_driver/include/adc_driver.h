#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOIL_SENSOR_COUNT  3

/** Per-zone sensor reading */
typedef struct {
    uint16_t raw;            /* 0-4095 (12-bit) */
    float    moisture_pct;   /* 0.0-100.0       */
} soil_reading_t;

/**
 * Initialize sensor driver.
 * @param synthetic  true = generate fake data (Phase 1)
 */
esp_err_t adc_driver_init(bool synthetic);

/** Read all SOIL_SENSOR_COUNT channels at once. */
esp_err_t adc_driver_read_all(soil_reading_t readings[SOIL_SENSOR_COUNT]);

/** Read a single channel (0-2). */
esp_err_t adc_driver_read_channel(uint8_t channel, soil_reading_t *reading);

esp_err_t adc_driver_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DRIVER_H */
