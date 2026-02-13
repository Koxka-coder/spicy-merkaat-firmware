#ifndef ENS210_DRIVER_H
#define ENS210_DRIVER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ENS210 I2C slave address (fixed) */
#define ENS210_I2C_ADDR         0x43

/* I2C GPIO pins (from arquitectura.md §5.2) */
#define ENS210_SDA_GPIO         21
#define ENS210_SCL_GPIO         22

/* ENS210 register addresses */
#define ENS210_REG_PART_ID      0x00
#define ENS210_REG_SYS_CTRL     0x10
#define ENS210_REG_SYS_STAT     0x11
#define ENS210_REG_SENS_RUN     0x21
#define ENS210_REG_SENS_START   0x22
#define ENS210_REG_SENS_STOP    0x23
#define ENS210_REG_SENS_STAT    0x24
#define ENS210_REG_T_VAL        0x30
#define ENS210_REG_H_VAL        0x33

typedef struct {
    float temp_c;           /* Temperature in Celsius       */
    float humidity_pct;     /* Relative humidity in %RH     */
    bool  temp_valid;       /* CRC + valid bit OK for temp  */
    bool  hum_valid;        /* CRC + valid bit OK for hum   */
} ens210_reading_t;

/**
 * Initialize I2C master and verify ENS210 is present.
 * @param synthetic  If true, skip I2C init and return synthetic data.
 */
esp_err_t ens210_init(bool synthetic);

/**
 * Perform a single-shot T+RH measurement (~130ms).
 * In synthetic mode, returns simulated values.
 */
esp_err_t ens210_read(ens210_reading_t *reading);

/**
 * De-initialize I2C driver.
 */
esp_err_t ens210_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ENS210_DRIVER_H */
