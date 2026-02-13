/**
 * @file zone_fsm.h
 * @brief Finite State Machine for zone management
 *
 * This module implements a finite state machine to manage different zones
 * and their states in the Spicy Merkaat system.
 */

#ifndef ZONE_FSM_H
#define ZONE_FSM_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zone FSM states
 */
typedef enum {
    ZONE_STATE_IDLE = 0,
    ZONE_STATE_ACTIVE,
    ZONE_STATE_ALARM,
    ZONE_STATE_ERROR
} zone_state_t;

/**
 * @brief Zone FSM events
 */
typedef enum {
    ZONE_EVENT_ACTIVATE = 0,
    ZONE_EVENT_DEACTIVATE,
    ZONE_EVENT_TRIGGER,
    ZONE_EVENT_RESET
} zone_event_t;

/**
 * @brief Initialize the zone FSM
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t zone_fsm_init(void);

/**
 * @brief Process an event in the zone FSM
 * 
 * @param zone_id Zone identifier
 * @param event Event to process
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t zone_fsm_process_event(uint8_t zone_id, zone_event_t event);

/**
 * @brief Get current state of a zone
 * 
 * @param zone_id Zone identifier
 * @return Current zone state
 */
zone_state_t zone_fsm_get_state(uint8_t zone_id);

#ifdef __cplusplus
}
#endif

#endif /* ZONE_FSM_H */
