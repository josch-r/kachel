#include "mqtt_client.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "../include/secrets.h"
#include "config.h"

static WiFiClient wifi_client;
static PubSubClient mqtt(wifi_client);

static uint32_t next_connect_ms;
static uint32_t connect_backoff_ms = 5000;
static uint32_t next_status_ms;

static void publish_status()
{
    char payload[96];
    snprintf(payload, sizeof(payload), "{\"fw\":\"%s\",\"rssi\":%d,\"uptime\":%lu}",
             KACHEL_FW_VERSION, WiFi.RSSI(), millis() / 1000);
    mqtt.publish("kachel/sys/status", payload);
}

static void try_connect()
{
    if (WiFi.status() != WL_CONNECTED)
        return;
    log_i("MQTT connecting to %s:%d", MQTT_HOST, MQTT_PORT);
    if (mqtt.connect("kachel", MQTT_USER, MQTT_PASSWORD))
    {
        connect_backoff_ms = 5000;
        next_status_ms = millis() + 60000;
        log_i("MQTT connected");
        publish_status();
    }
    else
    {
        // fail calm: back off, retry silently; staleness marks carry the news
        connect_backoff_ms = min(connect_backoff_ms * 2, (uint32_t)60000);
        log_w("MQTT connect failed rc=%d, retry in %lu s", mqtt.state(), connect_backoff_ms / 1000);
    }
}

void mqtt_begin()
{
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(1024); // contract payloads stay well below this
}

void mqtt_tick()
{
    auto const now = millis();
    if (!mqtt.connected())
    {
        if ((int32_t)(now - next_connect_ms) >= 0)
        {
            next_connect_ms = now + connect_backoff_ms;
            try_connect();
        }
        return;
    }
    mqtt.loop();
    if ((int32_t)(now - next_status_ms) >= 0)
    {
        next_status_ms = now + 60000; // contract cadence: 60 s
        publish_status();
    }
}

bool mqtt_connected()
{
    return mqtt.connected();
}
