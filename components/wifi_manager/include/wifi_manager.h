#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WiFi in STA mode and start connection.
 * Non-blocking — use wifi_manager_wait_connected() to block until IP.
 */
esp_err_t wifi_manager_init(const char *ssid, const char *password);

/**
 * Block until connected and IP obtained, or timeout.
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);

/** @return true if STA has an IP address */
bool wifi_manager_is_connected(void);

/** Get current RSSI. Returns ESP_OK or ESP_ERR_WIFI_NOT_CONNECT. */
esp_err_t wifi_manager_get_rssi(int8_t *rssi);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
