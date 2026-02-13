#include "zone_fsm.h"
#include "nvs_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ZONE_FSM";

static zone_ctx_t      s_zones[ZONE_COUNT];
static zone_event_cb_t s_event_cb;

static const int ZONE_GPIOS[ZONE_COUNT] = {ZONE0_GPIO, ZONE1_GPIO, ZONE2_GPIO};

/* ── helpers ──────────────────────────────────────────────── */

static void valve_set(uint8_t ch, bool on)
{
    gpio_set_level(s_zones[ch].gpio_pin, on ? 1 : 0);
    ESP_LOGI(TAG, "Zone %d valve %s (GPIO %d)", ch, on ? "OPEN" : "CLOSED",
             s_zones[ch].gpio_pin);
}

static void notify(uint8_t ch, zone_evt_type_t evt)
{
    if (s_event_cb) {
        s_event_cb(ch, evt);
    }
}

/* ── init ─────────────────────────────────────────────────── */

esp_err_t zone_fsm_init(zone_event_cb_t cb)
{
    s_event_cb = cb;

    for (uint8_t ch = 0; ch < ZONE_COUNT; ch++) {
        zone_ctx_t *z = &s_zones[ch];
        memset(z, 0, sizeof(*z));

        z->channel  = ch;
        z->gpio_pin = ZONE_GPIOS[ch];
        z->state    = ZONE_STATE_IDLE;
        z->mutex    = xSemaphoreCreateMutex();
        if (!z->mutex) {
            ESP_LOGE(TAG, "Failed to create mutex for zone %d", ch);
            return ESP_ERR_NO_MEM;
        }

        /* Load per-zone config from NVS */
        zone_config_t cfg;
        nvs_config_load_zone(ch, &cfg);
        z->max_duration_s = cfg.max_duration_s;
        z->cooldown_s     = cfg.cooldown_s;
        z->enabled        = cfg.enabled;

        /* Configure GPIO as output, initially LOW (valve closed) */
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << z->gpio_pin),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level(z->gpio_pin, 0);

        ESP_LOGI(TAG, "Zone %d: GPIO %d, max %ds, cooldown %ds, %s",
                 ch, z->gpio_pin, z->max_duration_s, z->cooldown_s,
                 z->enabled ? "enabled" : "disabled");
    }

    ESP_LOGI(TAG, "Zone FSM initialized (%d zones)", ZONE_COUNT);
    return ESP_OK;
}

/* ── tick (call every 1 s) ────────────────────────────────── */

void zone_fsm_tick(void)
{
    for (uint8_t ch = 0; ch < ZONE_COUNT; ch++) {
        zone_ctx_t *z = &s_zones[ch];
        xSemaphoreTake(z->mutex, portMAX_DELAY);

        switch (z->state) {
        case ZONE_STATE_WATERING:
            z->elapsed_s++;
            if (z->elapsed_s >= z->duration_s) {
                /* Normal end → COOLDOWN */
                valve_set(ch, false);
                z->state     = ZONE_STATE_COOLDOWN;
                z->elapsed_s = 0;
                ESP_LOGI(TAG, "Zone %d: watering done → COOLDOWN %ds",
                         ch, z->cooldown_s);
                notify(ch, ZONE_EVT_STOP);
            } else if (z->elapsed_s >= z->max_duration_s) {
                /* Safety timeout → LOCKED */
                valve_set(ch, false);
                z->state          = ZONE_STATE_LOCKED;
                z->safety_tripped = true;
                z->elapsed_s      = 0;
                ESP_LOGW(TAG, "Zone %d: SAFETY TIMEOUT → LOCKED", ch);
                notify(ch, ZONE_EVT_SAFETY_LOCK);
            }
            break;

        case ZONE_STATE_COOLDOWN:
            z->elapsed_s++;
            if (z->elapsed_s >= z->cooldown_s) {
                z->state     = ZONE_STATE_IDLE;
                z->elapsed_s = 0;
                ESP_LOGI(TAG, "Zone %d: cooldown done → IDLE", ch);
                notify(ch, ZONE_EVT_COOLDOWN_END);
            }
            break;

        case ZONE_STATE_IDLE:
        case ZONE_STATE_LOCKED:
            /* nothing to advance */
            break;
        }

        xSemaphoreGive(z->mutex);
    }
}

/* ── commands ─────────────────────────────────────────────── */

esp_err_t zone_fsm_cmd_water(uint8_t channel, uint16_t duration_s)
{
    if (channel >= ZONE_COUNT) return ESP_ERR_INVALID_ARG;
    zone_ctx_t *z = &s_zones[channel];

    xSemaphoreTake(z->mutex, portMAX_DELAY);

    if (!z->enabled) {
        ESP_LOGW(TAG, "Zone %d: disabled, ignoring water cmd", channel);
        xSemaphoreGive(z->mutex);
        return ESP_ERR_INVALID_STATE;
    }
    if (z->state != ZONE_STATE_IDLE) {
        ESP_LOGW(TAG, "Zone %d: not IDLE (state=%s), ignoring water cmd",
                 channel, zone_state_to_str(z->state));
        xSemaphoreGive(z->mutex);
        return ESP_ERR_INVALID_STATE;
    }

    /* Clamp to max duration */
    if (duration_s > z->max_duration_s) {
        ESP_LOGW(TAG, "Zone %d: duration %d clamped to max %d",
                 channel, duration_s, z->max_duration_s);
        duration_s = z->max_duration_s;
    }

    z->duration_s     = duration_s;
    z->elapsed_s      = 0;
    z->safety_tripped = false;
    z->state          = ZONE_STATE_WATERING;
    valve_set(channel, true);

    ESP_LOGI(TAG, "Zone %d: START watering %ds", channel, duration_s);
    notify(channel, ZONE_EVT_START);

    xSemaphoreGive(z->mutex);
    return ESP_OK;
}

esp_err_t zone_fsm_cmd_stop(uint8_t channel)
{
    if (channel >= ZONE_COUNT) return ESP_ERR_INVALID_ARG;
    zone_ctx_t *z = &s_zones[channel];

    xSemaphoreTake(z->mutex, portMAX_DELAY);

    if (z->state != ZONE_STATE_WATERING) {
        ESP_LOGW(TAG, "Zone %d: not WATERING (state=%s), ignoring stop cmd",
                 channel, zone_state_to_str(z->state));
        xSemaphoreGive(z->mutex);
        return ESP_ERR_INVALID_STATE;
    }

    valve_set(channel, false);
    z->state     = ZONE_STATE_COOLDOWN;
    z->elapsed_s = 0;
    ESP_LOGI(TAG, "Zone %d: STOP watering → COOLDOWN %ds",
             channel, z->cooldown_s);
    notify(channel, ZONE_EVT_STOP);

    xSemaphoreGive(z->mutex);
    return ESP_OK;
}

esp_err_t zone_fsm_cmd_unlock(uint8_t channel)
{
    if (channel >= ZONE_COUNT) return ESP_ERR_INVALID_ARG;
    zone_ctx_t *z = &s_zones[channel];

    xSemaphoreTake(z->mutex, portMAX_DELAY);

    if (z->state != ZONE_STATE_LOCKED) {
        ESP_LOGW(TAG, "Zone %d: not LOCKED (state=%s), ignoring unlock cmd",
                 channel, zone_state_to_str(z->state));
        xSemaphoreGive(z->mutex);
        return ESP_ERR_INVALID_STATE;
    }

    z->state          = ZONE_STATE_IDLE;
    z->safety_tripped = false;
    z->elapsed_s      = 0;
    ESP_LOGI(TAG, "Zone %d: UNLOCKED → IDLE", channel);
    notify(channel, ZONE_EVT_UNLOCK);

    xSemaphoreGive(z->mutex);
    return ESP_OK;
}

/* ── queries ──────────────────────────────────────────────── */

zone_state_t zone_fsm_get_state(uint8_t channel)
{
    if (channel >= ZONE_COUNT) return ZONE_STATE_IDLE;
    return s_zones[channel].state;
}

esp_err_t zone_fsm_get_ctx(uint8_t channel, zone_ctx_t *out)
{
    if (channel >= ZONE_COUNT || !out) return ESP_ERR_INVALID_ARG;
    zone_ctx_t *z = &s_zones[channel];

    xSemaphoreTake(z->mutex, portMAX_DELAY);
    memcpy(out, z, sizeof(*out));
    out->mutex = NULL; /* don't expose internal mutex handle */
    xSemaphoreGive(z->mutex);
    return ESP_OK;
}

const char *zone_state_to_str(zone_state_t state)
{
    switch (state) {
    case ZONE_STATE_IDLE:     return "IDLE";
    case ZONE_STATE_WATERING: return "WATERING";
    case ZONE_STATE_COOLDOWN: return "COOLDOWN";
    case ZONE_STATE_LOCKED:   return "LOCKED";
    default:                  return "UNKNOWN";
    }
}
