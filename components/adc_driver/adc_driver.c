#include "adc_driver.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char *TAG = "ADC_DRV";

static bool     s_synthetic;
static bool     s_initialized;

/* Base values per zone – realistic spread */
static const uint16_t SYNTH_BASE[SOIL_SENSOR_COUNT] = {2200, 1800, 3000};
#define SYNTH_DELTA  150   /* ±150 random noise */

/* ── helpers ──────────────────────────────────────────────── */

static float raw_to_pct(uint16_t raw)
{
    /* Linear: 0 raw = 100 %, 4095 raw = 0 % */
    float pct = (1.0f - (float)raw / 4095.0f) * 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f)  pct = 100.0f;
    return pct;
}

static uint16_t synth_sample(uint8_t ch)
{
    uint32_t rnd = esp_random();
    int16_t  delta = (int16_t)(rnd % (2 * SYNTH_DELTA + 1)) - SYNTH_DELTA;
    int32_t  val   = (int32_t)SYNTH_BASE[ch] + delta;
    if (val < 0)    val = 0;
    if (val > 4095) val = 4095;
    return (uint16_t)val;
}

/* ── public API ───────────────────────────────────────────── */

esp_err_t adc_driver_init(bool synthetic)
{
    s_synthetic  = synthetic;
    s_initialized = true;

    if (synthetic) {
        ESP_LOGI(TAG, "ADC driver init (SYNTHETIC mode)");
    } else {
        ESP_LOGI(TAG, "ADC driver init (HW mode – not implemented yet)");
        /* TODO Phase 2: configure ADC oneshot on GPIO 34/35/36 */
    }
    return ESP_OK;
}

esp_err_t adc_driver_read_all(soil_reading_t readings[SOIL_SENSOR_COUNT])
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    for (uint8_t ch = 0; ch < SOIL_SENSOR_COUNT; ch++) {
        if (s_synthetic) {
            readings[ch].raw = synth_sample(ch);
        } else {
            /* TODO Phase 2: real ADC read */
            readings[ch].raw = 0;
        }
        readings[ch].moisture_pct = raw_to_pct(readings[ch].raw);
    }
    return ESP_OK;
}

esp_err_t adc_driver_read_channel(uint8_t channel, soil_reading_t *reading)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (channel >= SOIL_SENSOR_COUNT || !reading) return ESP_ERR_INVALID_ARG;

    if (s_synthetic) {
        reading->raw = synth_sample(channel);
    } else {
        reading->raw = 0;
    }
    reading->moisture_pct = raw_to_pct(reading->raw);
    return ESP_OK;
}

esp_err_t adc_driver_deinit(void)
{
    s_initialized = false;
    ESP_LOGI(TAG, "ADC driver deinitialized");
    return ESP_OK;
}
