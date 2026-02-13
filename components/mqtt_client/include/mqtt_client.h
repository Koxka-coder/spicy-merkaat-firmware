/**
 * @file mqtt_client.h
 * @brief MQTT client wrapper for Spicy Merkaat system
 *
 * This module provides MQTT client functionality for communication
 * with the backend server.
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT client configuration
 */
typedef struct {
    const char *broker_uri;     /**< MQTT broker URI */
    const char *client_id;      /**< MQTT client ID */
    const char *username;       /**< MQTT username */
    const char *password;       /**< MQTT password */
} mqtt_client_config_t;

/**
 * @brief Initialize MQTT client
 * 
 * @param config MQTT client configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_client_init(mqtt_client_config_t *config);

/**
 * @brief Start MQTT client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_client_start(void);

/**
 * @brief Stop MQTT client
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_client_stop(void);

/**
 * @brief Publish a message to MQTT broker
 * 
 * @param topic MQTT topic
 * @param data Message data
 * @param len Message length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_client_publish(const char *topic, const char *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_CLIENT_H */
