// Kachel M1 — shell: boot, display, touch, carousel.
#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <esp_lcd_touch.h>

#include "carousel.h"

// The GT911 on this panel self-reports a bogus 1085x600 touch matrix while
// actually delivering native 480x480 coordinates. esp32-smartdisplay trusts
// the self-report and compresses x to <=212, y to <=384. Detach its scaling
// hook; raw coordinates are already display-correct. (AGENTS.md rule 4)
static void fix_gt911_scaling()
{
    auto touch_indev = lv_indev_get_next(nullptr);
    if (touch_indev == nullptr)
        return;
    auto th = (esp_lcd_touch_handle_t)lv_indev_get_user_data(touch_indev);
    if (th != nullptr)
        th->config.process_coordinates = nullptr;
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    log_i("Kachel boot. Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d @ %lu MHz", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz());
    log_i("Free heap: %u bytes, PSRAM: %u bytes", ESP.getFreeHeap(), ESP.getPsramSize());

    smartdisplay_init();
    fix_gt911_scaling();
    carousel_create();
    log_i("Carousel up, resting on ambient face");
}

void loop()
{
    static uint32_t lv_last_tick = millis();
    auto const now = millis();
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();
}
