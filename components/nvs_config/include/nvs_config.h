/**
 * @file nvs_config.h
 * @brief Non-Volatile Storage configuration management
 *
 * This module provides configuration management using NVS
 * for persistent storage of system settings.
 */

#ifndef NVS_CONFIG_H
#define NVS_CONFIG_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize NVS configuration
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_init(void);

/**
 * @brief Save a string value to NVS
 * 
 * @param key Configuration key
 * @param value String value to save
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_set_str(const char *key, const char *value);

/**
 * @brief Read a string value from NVS
 * 
 * @param key Configuration key
 * @param value Buffer to store the value
 * @param len Buffer length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_get_str(const char *key, char *value, size_t len);

/**
 * @brief Save an integer value to NVS
 * 
 * @param key Configuration key
 * @param value Integer value to save
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_set_int(const char *key, int32_t value);

/**
 * @brief Read an integer value from NVS
 * 
 * @param key Configuration key
 * @param value Pointer to store the value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_get_int(const char *key, int32_t *value);

/**
 * @brief Erase a key from NVS
 * 
 * @param key Configuration key
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t nvs_config_erase(const char *key);

#ifdef __cplusplus
}
#endif

#endif /* NVS_CONFIG_H */
