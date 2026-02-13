#include "spicy_mqtt.h"
#include "mqtt_client.h"   /* ESP-IDF's esp-mqtt header */
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "SPICY_MQTT";

static esp_mqtt_client_handle_t s_client;
static bool                     s_connected;
static spicy_mqtt_cmd_cb_t      s_cmd_cb;
static spicy_mqtt_conn_cb_t     s_conn_cb;
static char                     s_cmd_topic[64]; /* "garden/{id}/cmd/#" */

/* ── event handler ────────────────────────────────────────── */

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t evt = (esp_mqtt_event_handle_t)event_data;

    switch (evt->event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "Connected to broker");
        /* Subscribe to command topics */
        int mid = esp_mqtt_client_subscribe(s_client, s_cmd_topic, 1);
        ESP_LOGI(TAG, "Subscribed to %s (msg_id=%d)", s_cmd_topic, mid);
        if (s_conn_cb) s_conn_cb(true);
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "Disconnected from broker");
        if (s_conn_cb) s_conn_cb(false);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Incoming: topic=%.*s", evt->topic_len, evt->topic);
        if (s_cmd_cb) {
            s_cmd_cb(evt->topic, evt->topic_len,
                     evt->data,  evt->data_len);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error type=%d", evt->error_handle->error_type);
        break;

    default:
        break;
    }
}

/* ── public API ───────────────────────────────────────────── */

esp_err_t spicy_mqtt_init(const spicy_mqtt_config_t *config)
{
    if (!config || !config->broker_uri || !config->device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    s_cmd_cb  = config->cmd_cb;
    s_conn_cb = config->conn_cb;

    /* Build subscription topic: garden/{device_id}/cmd/# */
    snprintf(s_cmd_topic, sizeof(s_cmd_topic),
             "garden/%s/cmd/#", config->device_id);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config->broker_uri,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "MQTT client started → %s", config->broker_uri);
    return ESP_OK;
}

int spicy_mqtt_publish(const char *topic, const char *data, int len,
                       int qos, bool retain)
{
    if (!s_client) return -1;
    return esp_mqtt_client_publish(s_client, topic, data, len, qos,
                                  retain ? 1 : 0);
}

bool spicy_mqtt_is_connected(void)
{
    return s_connected;
}
