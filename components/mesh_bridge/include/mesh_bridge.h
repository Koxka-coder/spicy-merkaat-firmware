/**
 * @file mesh_bridge.h
 * @brief ESP-WIFI-MESH bridge functionality
 *
 * This module implements the mesh bridge functionality for ESP-WIFI-MESH
 * network integration.
 */

#ifndef MESH_BRIDGE_H
#define MESH_BRIDGE_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mesh bridge configuration
 */
typedef struct {
    uint8_t mesh_id[6];         /**< Mesh network ID */
    const char *mesh_password;  /**< Mesh network password */
    uint8_t channel;            /**< WiFi channel */
    uint8_t max_layer;          /**< Maximum mesh layers */
} mesh_bridge_config_t;

/**
 * @brief Initialize mesh bridge
 * 
 * @param config Mesh bridge configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_bridge_init(mesh_bridge_config_t *config);

/**
 * @brief Start mesh bridge
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_bridge_start(void);

/**
 * @brief Stop mesh bridge
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_bridge_stop(void);

/**
 * @brief Send data through mesh network
 * 
 * @param dest Destination MAC address
 * @param data Data to send
 * @param len Data length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mesh_bridge_send(const uint8_t *dest, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* MESH_BRIDGE_H */
