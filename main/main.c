#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "nvs_config.h"
#include "wifi_manager.h"
#include "adc_driver.h"
#include "zone_fsm.h"
#include "spicy_mqtt.h"
#include "ens210_driver.h"

static const char *TAG = "MAIN";

#define FW_VERSION "0.1.0"

/* ── Pre-built topic strings ─────────────────────────────── */

static char topic_soil[64];     /* garden/{id}/telemetry/soil */
static char topic_env[64];      /* garden/{id}/telemetry/env  */
static char topic_status[64];   /* garden/{id}/status         */
static char topic_alert[64];    /* garden/{id}/alert          */

static system_config_t s_sys_cfg;

/* ── Helpers ──────────────────────────────────────────────── */

static int64_t uptime_s(void)
{
    return esp_timer_get_time() / 1000000LL;
}

static void build_topics(const char *device_id)
{
    snprintf(topic_soil,   sizeof(topic_soil),
             "garden/%s/telemetry/soil", device_id);
    snprintf(topic_env,    sizeof(topic_env),
             "garden/%s/telemetry/env", device_id);
    snprintf(topic_status, sizeof(topic_status),
             "garden/%s/status", device_id);
    snprintf(topic_alert,  sizeof(topic_alert),
             "garden/%s/alert", device_id);
}

/* ── MQTT command dispatch ────────────────────────────────── */

static void mqtt_cmd_handler(const char *topic, int topic_len,
                             const char *data,  int data_len)
{
    /* Extract sub-topic after "cmd/" */
    const char *cmd_prefix = "/cmd/";
    const char *cmd = NULL;
    for (int i = 0; i < topic_len - 4; i++) {
        if (memcmp(topic + i, cmd_prefix, 5) == 0) {
            cmd = topic + i + 5;
            break;
        }
    }
    if (!cmd) {
        ESP_LOGW(TAG, "Unknown cmd topic: %.*s", topic_len, topic);
        return;
    }

    int cmd_len = topic_len - (int)(cmd - topic);

    /* Parse JSON payload */
    char *json_buf = malloc(data_len + 1);
    if (!json_buf) return;
    memcpy(json_buf, data, data_len);
    json_buf[data_len] = '\0';
    cJSON *root = cJSON_Parse(json_buf);
    free(json_buf);

    if (cmd_len >= 5 && memcmp(cmd, "water", 5) == 0) {
        if (root) {
            int zone_ch   = cJSON_GetObjectItem(root, "zone_ch")
                            ? cJSON_GetObjectItem(root, "zone_ch")->valueint : 0;
            int duration  = cJSON_GetObjectItem(root, "duration_s")
                            ? cJSON_GetObjectItem(root, "duration_s")->valueint : 30;
            ESP_LOGI(TAG, "CMD water: zone=%d duration=%ds", zone_ch, duration);
            zone_fsm_cmd_water((uint8_t)zone_ch, (uint16_t)duration);
        }
    } else if (cmd_len >= 4 && memcmp(cmd, "stop", 4) == 0) {
        if (root) {
            int zone_ch = cJSON_GetObjectItem(root, "zone_ch")
                          ? cJSON_GetObjectItem(root, "zone_ch")->valueint : 0;
            ESP_LOGI(TAG, "CMD stop: zone=%d", zone_ch);
            zone_fsm_cmd_stop((uint8_t)zone_ch);
        }
    } else if (cmd_len >= 6 && memcmp(cmd, "unlock", 6) == 0) {
        if (root) {
            int zone_ch = cJSON_GetObjectItem(root, "zone_ch")
                          ? cJSON_GetObjectItem(root, "zone_ch")->valueint : 0;
            ESP_LOGI(TAG, "CMD unlock: zone=%d", zone_ch);
            zone_fsm_cmd_unlock((uint8_t)zone_ch);
        }
    } else if (cmd_len >= 6 && memcmp(cmd, "config", 6) == 0) {
        ESP_LOGI(TAG, "CMD config received (placeholder – Phase 2)");
    } else if (cmd_len >= 8 && memcmp(cmd, "schedule", 8) == 0) {
        ESP_LOGI(TAG, "CMD schedule received (placeholder – Phase 2)");
    } else {
        ESP_LOGW(TAG, "Unknown command: %.*s", cmd_len, cmd);
    }

    if (root) cJSON_Delete(root);
}

/* ── Zone FSM event callback → MQTT alerts ────────────────── */

static void zone_event_callback(uint8_t channel, zone_evt_type_t evt)
{
    if (evt != ZONE_EVT_SAFETY_LOCK) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ts", (double)uptime_s());
    cJSON_AddStringToObject(root, "device_id", s_sys_cfg.device_id);
    cJSON_AddStringToObject(root, "severity", "critical");
    cJSON_AddStringToObject(root, "code", "SAFETY_TIMEOUT");
    cJSON_AddNumberToObject(root, "zone_ch", channel);

    char *json = cJSON_PrintUnformatted(root);
    if (json) {
        spicy_mqtt_publish(topic_alert, json, (int)strlen(json), 1, false);
        free(json);
    }
    cJSON_Delete(root);
}

/* ── MQTT connection callback ─────────────────────────────── */

static void mqtt_conn_callback(bool connected)
{
    ESP_LOGI(TAG, "MQTT %s", connected ? "CONNECTED" : "DISCONNECTED");
}

/* ── FreeRTOS Tasks ───────────────────────────────────────── */

static void zone_tick_task(void *arg)
{
    while (1) {
        zone_fsm_tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void telemetry_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(300 * 1000));  /* 5 min */

        if (!spicy_mqtt_is_connected()) continue;

        /* Read all sensors */
        soil_reading_t readings[SOIL_SENSOR_COUNT];
        if (adc_driver_read_all(readings) != ESP_OK) continue;

        ens210_reading_t env;
        bool env_ok = (ens210_read(&env) == ESP_OK);

        /* ── Soil telemetry (includes ambient T/RH per InfluxDB schema) ── */
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "ts", (double)uptime_s());
        cJSON_AddStringToObject(root, "device_id", s_sys_cfg.device_id);
        if (env_ok) {
            cJSON_AddNumberToObject(root, "temp_c", env.temp_c);
            cJSON_AddNumberToObject(root, "humidity_pct", env.humidity_pct);
        }

        cJSON *zones = cJSON_AddArrayToObject(root, "zones");
        for (int i = 0; i < SOIL_SENSOR_COUNT; i++) {
            cJSON *z = cJSON_CreateObject();
            cJSON_AddNumberToObject(z, "ch", i);
            cJSON_AddNumberToObject(z, "raw", readings[i].raw);
            cJSON_AddNumberToObject(z, "pct", readings[i].moisture_pct);
            cJSON_AddStringToObject(z, "state",
                                    zone_state_to_str(zone_fsm_get_state(i)));
            cJSON_AddItemToArray(zones, z);
        }

        char *json = cJSON_PrintUnformatted(root);
        if (json) {
            spicy_mqtt_publish(topic_soil, json, (int)strlen(json), 0, false);
            ESP_LOGI(TAG, "Soil telemetry sent (%d bytes)", (int)strlen(json));
            free(json);
        }
        cJSON_Delete(root);

        /* ── ENS210 environmental telemetry (dedicated topic) ── */
        if (env_ok) {
            cJSON *env_root = cJSON_CreateObject();
            cJSON_AddNumberToObject(env_root, "ts", (double)uptime_s());
            cJSON_AddStringToObject(env_root, "device_id", s_sys_cfg.device_id);
            cJSON_AddNumberToObject(env_root, "temp_c", env.temp_c);
            cJSON_AddNumberToObject(env_root, "humidity_pct", env.humidity_pct);

            char *env_json = cJSON_PrintUnformatted(env_root);
            if (env_json) {
                spicy_mqtt_publish(topic_env, env_json,
                                  (int)strlen(env_json), 0, false);
                ESP_LOGI(TAG, "Env: T=%.1f°C RH=%.1f%%",
                         env.temp_c, env.humidity_pct);
                free(env_json);
            }
            cJSON_Delete(env_root);
        }
    }
}

static void status_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60 * 1000));  /* 1 min */

        if (!spicy_mqtt_is_connected()) continue;

        int8_t rssi = 0;
        wifi_manager_get_rssi(&rssi);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "ts", (double)uptime_s());
        cJSON_AddStringToObject(root, "device_id", s_sys_cfg.device_id);
        cJSON_AddStringToObject(root, "fw_version", FW_VERSION);
        cJSON_AddNumberToObject(root, "uptime_s", (double)uptime_s());
        cJSON_AddNumberToObject(root, "heap_free",
                                (double)esp_get_free_heap_size());
        cJSON_AddNumberToObject(root, "rssi", rssi);

        /* Zone states summary */
        cJSON *zones = cJSON_AddArrayToObject(root, "zones");
        for (int i = 0; i < ZONE_COUNT; i++) {
            zone_ctx_t ctx;
            zone_fsm_get_ctx(i, &ctx);
            cJSON *z = cJSON_CreateObject();
            cJSON_AddNumberToObject(z, "ch", i);
            cJSON_AddStringToObject(z, "state", zone_state_to_str(ctx.state));
            cJSON_AddBoolToObject(z, "enabled", ctx.enabled);
            cJSON_AddItemToArray(zones, z);
        }

        char *json = cJSON_PrintUnformatted(root);
        if (json) {
            spicy_mqtt_publish(topic_status, json, (int)strlen(json), 1, false);
            ESP_LOGI(TAG, "Status sent (%d bytes)", (int)strlen(json));
            free(json);
        }
        cJSON_Delete(root);
    }
}

/* ── app_main ─────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "Spicy Merkaat Firmware v%s starting...", FW_VERSION);

    /* 1. NVS flash */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. NVS config */
    ESP_ERROR_CHECK(nvs_config_init());
    ESP_ERROR_CHECK(nvs_config_load_system(&s_sys_cfg));

    /* 3. WiFi credentials — dev mode provisioning */
#ifdef CONFIG_SPICY_DEV_MODE
    if (strlen(s_sys_cfg.wifi_ssid) == 0) {
        ESP_LOGI(TAG, "NVS WiFi empty → writing Kconfig defaults");
        strncpy(s_sys_cfg.wifi_ssid, CONFIG_SPICY_WIFI_SSID,
                WIFI_SSID_LEN - 1);
        strncpy(s_sys_cfg.wifi_pass, CONFIG_SPICY_WIFI_PASS,
                WIFI_PASS_LEN - 1);
        nvs_config_save_system(&s_sys_cfg);
    }
#endif

    ESP_LOGI(TAG, "Device ID: %s", s_sys_cfg.device_id);
    ESP_LOGI(TAG, "MQTT URI:  %s", s_sys_cfg.mqtt_uri);

    /* 4. Build MQTT topic strings */
    build_topics(s_sys_cfg.device_id);

    /* 5. ADC driver (synthetic for Phase 1) */
    ESP_ERROR_CHECK(adc_driver_init(true));

    /* 5b. ENS210 T+RH sensor (synthetic for Phase 1) */
    ESP_ERROR_CHECK(ens210_init(true));

    /* 6. Zone FSM */
    ESP_ERROR_CHECK(zone_fsm_init(zone_event_callback));

    /* 7. WiFi */
    ESP_ERROR_CHECK(wifi_manager_init(s_sys_cfg.wifi_ssid,
                                      s_sys_cfg.wifi_pass));
    ret = wifi_manager_wait_connected(30000);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi not connected after 30s — continuing anyway");
    }

    /* 8. MQTT */
    const spicy_mqtt_config_t mqtt_cfg = {
        .broker_uri = s_sys_cfg.mqtt_uri,
        .device_id  = s_sys_cfg.device_id,
        .cmd_cb     = mqtt_cmd_handler,
        .conn_cb    = mqtt_conn_callback,
    };
    ESP_ERROR_CHECK(spicy_mqtt_init(&mqtt_cfg));

    /* 9. Create FreeRTOS tasks */
    xTaskCreate(zone_tick_task,  "zone_tick",  2048, NULL, 10, NULL);
    xTaskCreate(telemetry_task,  "telemetry",  4096, NULL,  5, NULL);
    xTaskCreate(status_task,     "status",     4096, NULL,  5, NULL);

    ESP_LOGI(TAG, "All systems go. Tasks running.");
}
