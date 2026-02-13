#ifndef ZONE_FSM_H
#define ZONE_FSM_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZONE_COUNT  3

/* GPIO pins per zone (from arquitectura.md §5.2) */
#define ZONE0_GPIO  32
#define ZONE1_GPIO  33
#define ZONE2_GPIO  25

/* ── FSM states ───────────────────────────────────────────── */

typedef enum {
    ZONE_STATE_IDLE = 0,
    ZONE_STATE_WATERING,
    ZONE_STATE_COOLDOWN,
    ZONE_STATE_LOCKED       /* safety timeout tripped */
} zone_state_t;

/* ── Event types for callback ─────────────────────────────── */

typedef enum {
    ZONE_EVT_START,         /* watering started        */
    ZONE_EVT_STOP,          /* watering stopped (cmd)  */
    ZONE_EVT_COOLDOWN_END,  /* cooldown finished       */
    ZONE_EVT_SAFETY_LOCK,   /* safety timeout tripped  */
    ZONE_EVT_UNLOCK         /* manual unlock           */
} zone_evt_type_t;

/* ── Per-zone context ─────────────────────────────────────── */

typedef struct {
    zone_state_t     state;
    uint8_t          channel;           /* 0-2                         */
    uint16_t         duration_s;        /* requested watering duration */
    uint16_t         elapsed_s;         /* seconds in current state    */
    uint16_t         max_duration_s;    /* safety hard limit           */
    uint16_t         cooldown_s;        /* cooldown after watering     */
    int              gpio_pin;          /* 32, 33, or 25               */
    SemaphoreHandle_t mutex;
    bool             safety_tripped;
    bool             enabled;
} zone_ctx_t;

/**
 * Callback invoked on zone state transitions.
 * Allows main.c to publish MQTT alerts without coupling zone_fsm to MQTT.
 */
typedef void (*zone_event_cb_t)(uint8_t channel, zone_evt_type_t evt);

/* ── Public API ───────────────────────────────────────────── */

/**
 * Configure GPIOs as output LOW, create mutexes, load config from NVS.
 * @param cb  Optional callback for state-change events (may be NULL).
 */
esp_err_t zone_fsm_init(zone_event_cb_t cb);

/**
 * Must be called every 1 s from a dedicated FreeRTOS task.
 * Advances timers, checks safety limits, transitions states.
 */
void zone_fsm_tick(void);

/** Start watering on channel (only from IDLE). Duration clamped to max. */
esp_err_t zone_fsm_cmd_water(uint8_t channel, uint16_t duration_s);

/** Stop watering → transitions to COOLDOWN. */
esp_err_t zone_fsm_cmd_stop(uint8_t channel);

/** Unlock a LOCKED zone → transitions to IDLE. */
esp_err_t zone_fsm_cmd_unlock(uint8_t channel);

/** Get current state of a zone. */
zone_state_t zone_fsm_get_state(uint8_t channel);

/** Get full context (read-only copy). */
esp_err_t zone_fsm_get_ctx(uint8_t channel, zone_ctx_t *out);

/** Convert state enum to string. */
const char *zone_state_to_str(zone_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* ZONE_FSM_H */
