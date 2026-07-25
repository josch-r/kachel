#include "state_model.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstring>

#include "mqtt_client.h"

#include "config.h"

static kachel_air air;
static kachel_weather weather;
static kachel_event events[KACHEL_EVENTS_MAX];
static int event_count = 0;
static kachel_timer timer_state;
static kachel_bring bring;
static SemaphoreHandle_t lock;

static void ensure_lock()
{
    if (lock == nullptr)
        lock = xSemaphoreCreateMutex();
}

static int hhmm_to_min(const char *s)
{
    if (s == nullptr || strlen(s) < 4)
        return -1;
    return atoi(s) * 60 + atoi(s + 3);
}

// "2026-07-25T19:00:00+02:00" -> time_t, offset ignored (device shares the
// backend's local timezone; contract payloads are Europe/Berlin local)
static time_t iso_local_to_epoch(const char *s)
{
    struct tm t = {};
    if (s == nullptr || sscanf(s, "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon,
                               &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) < 5)
        return 0;
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    t.tm_isdst = -1;
    return mktime(&t);
}

static kachel_condition parse_condition(const char *c)
{
    if (c == nullptr)
        return KACHEL_COND_UNKNOWN;
    if (strstr(c, "partly"))
        return KACHEL_COND_PARTLYCLOUDY;
    if (strstr(c, "cloud"))
        return KACHEL_COND_CLOUDY;
    if (strstr(c, "rain") || strstr(c, "pouring") || strstr(c, "lightning"))
        return KACHEL_COND_RAIN;
    if (strstr(c, "fog"))
        return KACHEL_COND_FOG;
    if (strstr(c, "snow") || strstr(c, "hail"))
        return KACHEL_COND_SNOW;
    if (strstr(c, "sunny") || strstr(c, "clear"))
        return KACHEL_COND_CLEAR;
    return KACHEL_COND_UNKNOWN;
}

void state_model_ingest(int topic, const char *payload)
{
    ensure_lock();
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok)
    {
        log_w("state_model: bad JSON on topic %d", topic);
        return;
    }

    xSemaphoreTake(lock, portMAX_DELAY);
    switch (topic)
    {
    case KACHEL_TOPIC_AIR:
        air.pm25 = doc["pm25"] | -1;
        air.fan = doc["fan"] | 0;
        air.filter_pct = doc["filter_pct"] | -1;
        air.valid = air.pm25 >= 0;
        break;
    case KACHEL_TOPIC_WEATHER:
    {
        weather.temp = doc["temp"] | 0.0f;
        weather.condition = parse_condition(doc["condition"] | (const char *)nullptr);
        weather.precip_12h_mm = doc["precip_12h_mm"] | 0.0f;
        int sr = hhmm_to_min(doc["sunrise"] | (const char *)nullptr);
        int ss = hhmm_to_min(doc["sunset"] | (const char *)nullptr);
        if (sr >= 0)
            weather.sunrise_min = sr;
        if (ss >= 0)
            weather.sunset_min = ss;
        weather.valid = true;
        break;
    }
    case KACHEL_TOPIC_CALENDAR:
    {
        event_count = 0;
        JsonArray next = doc["next"];
        if (!next.isNull())
        {
            for (JsonObject ev : next)
            {
                if (event_count >= KACHEL_EVENTS_MAX)
                    break;
                kachel_event &e = events[event_count];
                strlcpy(e.title, ev["title"] | "", sizeof(e.title));
                e.start = iso_local_to_epoch(ev["start"] | (const char *)nullptr);
                e.valid = e.start > 0;
                if (e.valid)
                    event_count++;
            }
        }
        break;
    }
    case KACHEL_TOPIC_BRING:
    {
        bring.count = doc["count"] | 0;
        bring.item_count = 0;
        JsonArray items = doc["items"];
        if (!items.isNull())
        {
            for (const char *item : items)
            {
                if (bring.item_count >= KACHEL_BRING_ITEMS_MAX || item == nullptr)
                    break;
                strlcpy(bring.items[bring.item_count++], item, sizeof(bring.items[0]));
            }
        }
        bring.valid = true;
        break;
    }
    case KACHEL_TOPIC_TIMER:
    {
        const char *ends = doc["ends_at"] | (const char *)nullptr;
        if (ends != nullptr)
        {
            strlcpy(timer_state.label, doc["label"] | "Timer", sizeof(timer_state.label));
            timer_state.ends_at = iso_local_to_epoch(ends);
            timer_state.active = timer_state.ends_at > 0;
        }
        else
            timer_state.active = false;
        break;
    }
    default:
        break;
    }
    xSemaphoreGive(lock);
}

#define SNAPSHOT(type, var)                    \
    ensure_lock();                             \
    xSemaphoreTake(lock, portMAX_DELAY);       \
    type copy = var;                           \
    xSemaphoreGive(lock);                      \
    return copy;

kachel_air state_air() { SNAPSHOT(kachel_air, air) }
kachel_weather state_weather() { SNAPSHOT(kachel_weather, weather) }
kachel_event state_next_event() { SNAPSHOT(kachel_event, events[0]) }
kachel_timer state_timer() { SNAPSHOT(kachel_timer, timer_state) }
kachel_bring state_bring() { SNAPSHOT(kachel_bring, bring) }

int state_events(kachel_event *out)
{
    ensure_lock();
    xSemaphoreTake(lock, portMAX_DELAY);
    int n = event_count;
    for (int i = 0; i < n; i++)
        out[i] = events[i];
    xSemaphoreGive(lock);
    return n;
}

// --- PM2.5 history ring (UI-thread only: tick + reader both run under LVGL) ---

static int16_t *history_ring;
static int history_start, history_count;
static uint32_t last_sample_ms;
static bool ever_sampled;

void state_history_tick()
{
    kachel_air a = state_air();
    if (!a.valid)
        return;
    uint32_t now = millis();
    if (ever_sampled && now - last_sample_ms < KACHEL_PM25_SAMPLE_S * 1000UL)
        return;
    if (history_ring == nullptr)
    {
        history_ring = (int16_t *)heap_caps_malloc(
            KACHEL_PM25_HISTORY_N * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (history_ring == nullptr) // no PSRAM? tiny buffer, heap is fine
            history_ring = (int16_t *)malloc(KACHEL_PM25_HISTORY_N * sizeof(int16_t));
        if (history_ring == nullptr)
            return;
    }
    last_sample_ms = now;
    ever_sampled = true;
    int slot = (history_start + history_count) % KACHEL_PM25_HISTORY_N;
    history_ring[slot] = (int16_t)a.pm25;
    if (history_count < KACHEL_PM25_HISTORY_N)
        history_count++;
    else
        history_start = (history_start + 1) % KACHEL_PM25_HISTORY_N;
}

int state_history(int16_t *out, int n)
{
    if (history_ring == nullptr || history_count == 0)
        return -1;
    int count = history_count < n ? history_count : n;
    int from = history_count - count; // newest-biased window, oldest-first order
    for (int i = 0; i < count; i++)
        out[i] = history_ring[(history_start + from + i) % KACHEL_PM25_HISTORY_N];
    return count;
}
