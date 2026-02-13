#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ZONES        3
#define DEVICE_ID_LEN    18   /* "AA:BB:CC:DD:EE:FF\0" */
#define WIFI_SSID_LEN    33
#define WIFI_PASS_LEN    65
#define MQTT_URI_LEN     128

/** Per-zone configuration stored as NVS blob (key "zone_N"). */
typedef struct {
    uint16_t dry_threshold;    /* ADC raw: soil dry above this  (default 2800) */
    uint16_t wet_threshold;    /* ADC raw: soil wet below this  (default 1200) */
    uint16_t max_duration_s;   /* safety hard limit seconds     (default 600)  */
    uint16_t cooldown_s;       /* cooldown after watering       (default 120)  */
    bool     enabled;
    uint8_t  _pad[3];
} zone_config_t;

/** System-wide configuration stored as NVS blob (key "sys_config"). */
typedef struct {
    char     mqtt_uri[MQTT_URI_LEN];
    char     device_id[DEVICE_ID_LEN];
    char     wifi_ssid[WIFI_SSID_LEN];
    char     wifi_pass[WIFI_PASS_LEN];
    uint16_t schedule_version;
    uint8_t  _pad[2];
} system_config_t;

esp_err_t nvs_config_init(void);

esp_err_t nvs_config_load_system(system_config_t *cfg);
esp_err_t nvs_config_save_system(const system_config_t *cfg);

esp_err_t nvs_config_load_zone(uint8_t channel, zone_config_t *cfg);
esp_err_t nvs_config_save_zone(uint8_t channel, const zone_config_t *cfg);

void nvs_config_defaults_system(system_config_t *cfg);
void nvs_config_defaults_zone(zone_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* NVS_CONFIG_H */
