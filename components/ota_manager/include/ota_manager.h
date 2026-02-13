/**
 * @file ota_manager.h
 * @brief OTA (Over-The-Air) update manager with rollback support
 *
 * This module handles OTA firmware updates with rollback capability
 * for the Spicy Merkaat system.
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA update configuration
 */
typedef struct {
    const char *url;            /**< OTA firmware URL */
    const char *cert_pem;       /**< Server certificate (optional) */
    int timeout_ms;             /**< Timeout in milliseconds */
} ota_config_t;

/**
 * @brief Initialize OTA manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_init(void);

/**
 * @brief Start OTA update process
 * 
 * @param config OTA configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_start_update(ota_config_t *config);

/**
 * @brief Mark current firmware as valid (prevent rollback)
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_mark_valid(void);

/**
 * @brief Check if rollback is possible
 * 
 * @return true if rollback is possible, false otherwise
 */
bool ota_manager_can_rollback(void);

/**
 * @brief Perform rollback to previous firmware
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_rollback(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MANAGER_H */
