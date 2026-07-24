#include "time_sync.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>

#include "../include/secrets.h"

// Europe/Berlin with DST rules
static const char *TZ_BERLIN = "CET-1CEST,M3.5.0,M10.5.0/3";

void time_sync_begin()
{
    // AGENTS.md rule 1: WiFi stack must never write NVS at runtime —
    // flash writes stall the RGB refill and flicker the panel.
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    configTzTime(TZ_BERLIN, "pool.ntp.org", "time.cloudflare.com");
    log_i("WiFi connecting to %s, SNTP configured", WIFI_SSID);
}

bool time_sync_valid()
{
    time_t now = time(nullptr);
    bool valid = now > 1700000000; // sane epoch => SNTP has set the clock
    static bool logged = false;
    if (valid && !logged)
    {
        logged = true;
        struct tm local;
        localtime_r(&now, &local);
        log_i("SNTP synced: %02d:%02d:%02d local, WiFi RSSI %d dBm",
              local.tm_hour, local.tm_min, local.tm_sec, WiFi.RSSI());
    }
    return valid;
}

int time_sync_minute_of_day()
{
    if (!time_sync_valid())
        return -1;
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    return local.tm_hour * 60 + local.tm_min;
}

bool time_sync_clock_text(char *buf, unsigned len)
{
    if (!time_sync_valid())
        return false;
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    snprintf(buf, len, "%02d:%02d", local.tm_hour, local.tm_min);
    return true;
}
