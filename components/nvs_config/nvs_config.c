#include "nvs_config.h"
#include "nvs.h"
#include "esp_mac.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "NVS_CFG";

static nvs_handle_t s_sys_handle;
static nvs_handle_t s_zone_handle;

/* ── defaults ─────────────────────────────────────────────── */

void nvs_config_defaults_system(system_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    strncpy(cfg->mqtt_uri, "mqtt://mosquitto:1883", MQTT_URI_LEN - 1);
    cfg->schedule_version = 0;

    /* derive device_id from base MAC */
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(cfg->device_id, DEVICE_ID_LEN,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void nvs_config_defaults_zone(zone_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->dry_threshold  = 2800;
    cfg->wet_threshold  = 1200;
    cfg->max_duration_s = 600;
    cfg->cooldown_s     = 120;
    cfg->enabled        = true;
}

/* ── init ─────────────────────────────────────────────────── */

esp_err_t nvs_config_init(void)
{
    esp_err_t err;
    err = nvs_open("sys_cfg", NVS_READWRITE, &s_sys_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open sys_cfg: %s", esp_err_to_name(err));
        return err;
    }
    err = nvs_open("zone_cfg", NVS_READWRITE, &s_zone_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open zone_cfg: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "NVS config initialized");
    return ESP_OK;
}

/* ── system config ────────────────────────────────────────── */

esp_err_t nvs_config_load_system(system_config_t *cfg)
{
    nvs_config_defaults_system(cfg);
    size_t len = sizeof(*cfg);
    esp_err_t err = nvs_get_blob(s_sys_handle, "sys_config", cfg, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "sys_config not in NVS, using defaults");
        return ESP_OK;
    }
    if (err == ESP_OK && len != sizeof(*cfg)) {
        ESP_LOGW(TAG, "sys_config size mismatch (%d vs %d), using defaults",
                 (int)len, (int)sizeof(*cfg));
        nvs_config_defaults_system(cfg);
    }
    return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) ? ESP_OK : err;
}

esp_err_t nvs_config_save_system(const system_config_t *cfg)
{
    esp_err_t err = nvs_set_blob(s_sys_handle, "sys_config", cfg, sizeof(*cfg));
    if (err != ESP_OK) return err;
    return nvs_commit(s_sys_handle);
}

/* ── zone config ──────────────────────────────────────────── */

esp_err_t nvs_config_load_zone(uint8_t channel, zone_config_t *cfg)
{
    if (channel >= MAX_ZONES) return ESP_ERR_INVALID_ARG;
    nvs_config_defaults_zone(cfg);

    char key[16];
    snprintf(key, sizeof(key), "zone_%d", channel);
    size_t len = sizeof(*cfg);
    esp_err_t err = nvs_get_blob(s_zone_handle, key, cfg, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "%s not in NVS, using defaults", key);
        return ESP_OK;
    }
    if (err == ESP_OK && len != sizeof(*cfg)) {
        ESP_LOGW(TAG, "%s size mismatch, using defaults", key);
        nvs_config_defaults_zone(cfg);
    }
    return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) ? ESP_OK : err;
}

esp_err_t nvs_config_save_zone(uint8_t channel, const zone_config_t *cfg)
{
    if (channel >= MAX_ZONES) return ESP_ERR_INVALID_ARG;
    char key[16];
    snprintf(key, sizeof(key), "zone_%d", channel);
    esp_err_t err = nvs_set_blob(s_zone_handle, key, cfg, sizeof(*cfg));
    if (err != ESP_OK) return err;
    return nvs_commit(s_zone_handle);
}
