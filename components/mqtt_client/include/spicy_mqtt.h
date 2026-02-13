#ifndef SPICY_MQTT_H
#define SPICY_MQTT_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback for incoming MQTT command messages.
 * topic/data are NOT null-terminated; use the length parameters.
 */
typedef void (*spicy_mqtt_cmd_cb_t)(const char *topic, int topic_len,
                                    const char *data,  int data_len);

/** Callback for connection state changes. */
typedef void (*spicy_mqtt_conn_cb_t)(bool connected);

typedef struct {
    const char          *broker_uri;   /* e.g. "mqtt://mosquitto:1883" */
    const char          *device_id;    /* e.g. "AA:BB:CC:DD:EE:FF"    */
    spicy_mqtt_cmd_cb_t  cmd_cb;       /* incoming command handler     */
    spicy_mqtt_conn_cb_t conn_cb;      /* connection state handler     */
} spicy_mqtt_config_t;

/**
 * Initialize and start the MQTT client.
 * Subscribes to garden/{device_id}/cmd/# on connect.
 */
esp_err_t spicy_mqtt_init(const spicy_mqtt_config_t *config);

/**
 * Publish a message.
 * @return msg_id (>0) on success, -1 on error.
 */
int spicy_mqtt_publish(const char *topic, const char *data, int len,
                       int qos, bool retain);

/** Check if connected to broker. */
bool spicy_mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* SPICY_MQTT_H */
