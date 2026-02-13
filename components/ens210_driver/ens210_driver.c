#include "ens210_driver.h"
#include "esp_log.h"
#include "esp_random.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ENS210";

#define I2C_PORT            I2C_NUM_0
#define I2C_FREQ_HZ         400000      /* 400 kHz fast mode */
#define I2C_TIMEOUT_MS      200
#define CONVERSION_TIME_MS  130         /* T+RH single shot */

static bool s_synthetic;
static bool s_initialized;

/* ── CRC-7 (from ENS210 datasheet §Computing CRC-7) ────── */

#define CRC7WIDTH   7
#define CRC7POLY    0x89
#define CRC7IVEC    0x7F
#define DATA7WIDTH  17
#define DATA7MASK   ((1UL << DATA7WIDTH) - 1)
#define DATA7MSB    (1UL << (DATA7WIDTH - 1))

static uint32_t crc7(uint32_t val)
{
    uint32_t pol = CRC7POLY;
    pol = pol << (DATA7WIDTH - CRC7WIDTH - 1);
    uint32_t bit = DATA7MSB;
    val = val << CRC7WIDTH;
    bit = bit << CRC7WIDTH;
    pol = pol << CRC7WIDTH;
    val |= CRC7IVEC;
    while (bit & (DATA7MASK << CRC7WIDTH)) {
        if (bit & val) val ^= pol;
        bit >>= 1;
        pol >>= 1;
    }
    return val;
}

/* ── I2C helpers ──────────────────────────────────────────── */

static esp_err_t ens210_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c_master_write_to_device(I2C_PORT, ENS210_I2C_ADDR,
                                      buf, sizeof(buf),
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

static esp_err_t ens210_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_PORT, ENS210_I2C_ADDR,
                                        &reg, 1, data, len,
                                        pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

/* ── Synthetic data ───────────────────────────────────────── */

static void synth_reading(ens210_reading_t *r)
{
    /* Temp: base 22.0°C ±2.0 */
    uint32_t rnd = esp_random();
    r->temp_c = 22.0f + ((float)(rnd % 401) - 200.0f) / 100.0f;

    /* RH: base 55% ±10 */
    rnd = esp_random();
    r->humidity_pct = 55.0f + ((float)(rnd % 2001) - 1000.0f) / 100.0f;

    r->temp_valid = true;
    r->hum_valid  = true;
}

/* ── Parse 24-bit readout register (datasheet §Processing T_VAL/H_VAL) ── */

static bool parse_val(const uint8_t raw[3], uint16_t *data)
{
    /* Little-endian: raw[0]=LSB, raw[1]=MSB of data, raw[2]=valid+crc */
    uint32_t val = ((uint32_t)raw[2] << 16) |
                   ((uint32_t)raw[1] << 8)  |
                   ((uint32_t)raw[0]);

    uint32_t d     = (val >> 0)  & 0xFFFF;
    uint32_t valid = (val >> 16) & 0x1;
    uint32_t crc_r = (val >> 17) & 0x7F;

    uint32_t payload = val & 0x1FFFF;
    bool crc_ok = (crc7(payload) == crc_r);

    *data = (uint16_t)d;
    return valid && crc_ok;
}

/* ── Public API ───────────────────────────────────────────── */

esp_err_t ens210_init(bool synthetic)
{
    s_synthetic = synthetic;

    if (synthetic) {
        ESP_LOGI(TAG, "ENS210 init (SYNTHETIC mode)");
        s_initialized = true;
        return ESP_OK;
    }

    /* Configure I2C master */
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = ENS210_SDA_GPIO,
        .scl_io_num       = ENS210_SCL_GPIO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_PORT, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) return err;

    /* Verify device presence: read SYS_STAT (should return 1 = standby) */
    vTaskDelay(pdMS_TO_TICKS(2));  /* wait for boot (1.2ms max) */
    uint8_t sys_stat = 0;
    err = ens210_read_reg(ENS210_REG_SYS_STAT, &sys_stat, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ENS210 not found on I2C (addr 0x%02X)", ENS210_I2C_ADDR);
        i2c_driver_delete(I2C_PORT);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "ENS210 detected, SYS_STAT=0x%02X", sys_stat);
    s_initialized = true;
    return ESP_OK;
}

esp_err_t ens210_read(ens210_reading_t *reading)
{
    if (!s_initialized || !reading) return ESP_ERR_INVALID_STATE;

    if (s_synthetic) {
        synth_reading(reading);
        return ESP_OK;
    }

    /* Single shot: set SENS_RUN to 0 (both T and H single shot) */
    esp_err_t err = ens210_write_reg(ENS210_REG_SENS_RUN, 0x00);
    if (err != ESP_OK) return err;

    /* Start T+RH measurement: write 0x03 to SENS_START */
    err = ens210_write_reg(ENS210_REG_SENS_START, 0x03);
    if (err != ESP_OK) return err;

    /* Wait for conversion */
    vTaskDelay(pdMS_TO_TICKS(CONVERSION_TIME_MS));

    /* Read 6 bytes: T_VAL (3 bytes at 0x30) + H_VAL (3 bytes at 0x33) */
    uint8_t buf[6];
    err = ens210_read_reg(ENS210_REG_T_VAL, buf, 6);
    if (err != ESP_OK) return err;

    /* Parse temperature */
    uint16_t t_data = 0;
    reading->temp_valid = parse_val(&buf[0], &t_data);
    float t_kelvin = (float)t_data / 64.0f;
    reading->temp_c = t_kelvin - 273.15f;

    /* Parse humidity */
    uint16_t h_data = 0;
    reading->hum_valid = parse_val(&buf[3], &h_data);
    reading->humidity_pct = (float)h_data / 512.0f;

    /* Clamp RH */
    if (reading->humidity_pct < 0.0f)   reading->humidity_pct = 0.0f;
    if (reading->humidity_pct > 100.0f) reading->humidity_pct = 100.0f;

    ESP_LOGD(TAG, "T=%.1f°C (valid=%d) RH=%.1f%% (valid=%d)",
             reading->temp_c, reading->temp_valid,
             reading->humidity_pct, reading->hum_valid);

    return ESP_OK;
}

esp_err_t ens210_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    s_initialized = false;
    if (!s_synthetic) {
        i2c_driver_delete(I2C_PORT);
    }
    ESP_LOGI(TAG, "ENS210 deinitialized");
    return ESP_OK;
}
