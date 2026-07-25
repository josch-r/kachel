#include "mqtt_client.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "../include/secrets.h"
#include "config.h"
#include "state_model.h"

static WiFiClient wifi_client;
static PubSubClient mqtt(wifi_client);

static uint32_t next_connect_ms;
static uint32_t connect_backoff_ms = 5000;
static uint32_t next_status_ms;

// topic suffix + contract cadence (0 = on-change, never stale-marked)
struct topic_spec
{
    const char *suffix;
    uint32_t cadence_s;
};
static const topic_spec topic_specs[KACHEL_TOPIC_COUNT] = {
    {"air", 0}, {"weather", 900}, {"calendar", 300}, {"bring", 300}, {"timer", 0}};

static kachel_state states[KACHEL_TOPIC_COUNT];
static SemaphoreHandle_t states_lock;

// commands cross from the UI thread to the MQTT task via queue —
// PubSubClient is not thread-safe
struct cmd_msg
{
    char topic[24];
    char payload[24];
};
static QueueHandle_t cmd_queue;

static void on_message(char *topic, uint8_t *payload, unsigned int length)
{
    const char *suffix = strrchr(topic, '/');
    if (suffix == nullptr)
        return;
    suffix++;
    for (int i = 0; i < KACHEL_TOPIC_COUNT; i++)
    {
        if (strcmp(suffix, topic_specs[i].suffix) == 0)
        {
            xSemaphoreTake(states_lock, portMAX_DELAY);
            auto &s = states[i];
            unsigned int n = min(length, (unsigned int)sizeof(s.payload) - 1);
            memcpy(s.payload, payload, n);
            s.payload[n] = '\0';
            s.received_ms = millis();
            s.ever_received = true;
            xSemaphoreGive(states_lock);
            state_model_ingest(i, states[i].payload);
            log_i("state/%s %u bytes", suffix, length);
            return;
        }
    }
}

static void publish_status()
{
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"fw\":\"%s\",\"rssi\":%d,\"uptime\":%lu,\"heap\":%lu,\"psram_free\":%lu}",
             KACHEL_FW_VERSION, WiFi.RSSI(), millis() / 1000,
             (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
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
        mqtt.subscribe("kachel/state/+");
        publish_status();
    }
    else
    {
        // fail calm: back off, retry silently; staleness marks carry the news
        connect_backoff_ms = min(connect_backoff_ms * 2, (uint32_t)60000);
        log_w("MQTT connect failed rc=%d, retry in %lu s", mqtt.state(), connect_backoff_ms / 1000);
    }
}

void mqtt_tick()
{
    auto const now = millis();
    cmd_msg cmd;
    while (mqtt.connected() && xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE)
    {
        mqtt.publish(cmd.topic, cmd.payload);
        log_i("%s %s", cmd.topic, cmd.payload);
    }
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

// own task on core 0: a blocked broker connect stalls only this task,
// never the LVGL/render loop (reviewer debt from M2, calm rule §5.6)
static void mqtt_task(void *)
{
    for (;;)
    {
        mqtt_tick();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void mqtt_begin()
{
    states_lock = xSemaphoreCreateMutex();
    cmd_queue = xQueueCreate(8, sizeof(cmd_msg));
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(1024); // contract payloads stay well below this
    mqtt.setCallback(on_message);
    xTaskCreatePinnedToCore(mqtt_task, "kachel_mqtt", 8192, nullptr, 1, nullptr, 0);
}

bool mqtt_connected()
{
    return mqtt.connected();
}

void mqtt_cmd_scene(uint8_t id)
{
    cmd_msg m;
    strlcpy(m.topic, "kachel/cmd/scene", sizeof(m.topic));
    snprintf(m.payload, sizeof(m.payload), "{\"id\":%u}", id);
    xQueueSend(cmd_queue, &m, 0);
}

void mqtt_cmd_air(uint8_t fan)
{
    cmd_msg m;
    strlcpy(m.topic, "kachel/cmd/air", sizeof(m.topic));
    snprintf(m.payload, sizeof(m.payload), "{\"fan\":%u}", fan);
    xQueueSend(cmd_queue, &m, 0);
}

void mqtt_cmd_air_auto()
{
    cmd_msg m;
    strlcpy(m.topic, "kachel/cmd/air", sizeof(m.topic));
    strlcpy(m.payload, "{\"mode\":\"auto\"}", sizeof(m.payload));
    xQueueSend(cmd_queue, &m, 0);
}

const kachel_state *mqtt_state(kachel_topic topic)
{
    // raw pointer kept for the debug view; writes are short memcpys under
    // lock, torn reads render at worst one garbled debug frame
    return &states[topic];
}

int32_t mqtt_state_age_s(kachel_topic topic)
{
    auto &s = states[topic];
    if (!s.ever_received)
        return -1;
    return (int32_t)((millis() - s.received_ms) / 1000);
}

bool mqtt_state_stale(kachel_topic topic)
{
    auto cadence = topic_specs[topic].cadence_s;
    if (cadence == 0)
        return false;
    auto age = mqtt_state_age_s(topic);
    return age < 0 || (uint32_t)age > 3 * cadence;
}
