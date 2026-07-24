// Kachel M1 task 1 — display bring-up.
// Test pattern: four palette-token quadrants + wordmark, verifies panel init,
// color path, and orientation. Serial trace doubles as boot evidence.
#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <esp_lcd_touch.h>

#include "palette.h"

// The GT911 on this panel self-reports a bogus 1085x600 touch matrix while
// actually delivering native 480x480 coordinates. esp32-smartdisplay trusts
// the self-report and compresses x to <=212, y to <=384. Detach its scaling
// hook; raw coordinates are already display-correct.
static void fix_gt911_scaling()
{
    auto touch_indev = lv_indev_get_next(nullptr);
    if (touch_indev == nullptr)
        return;
    auto th = (esp_lcd_touch_handle_t)lv_indev_get_user_data(touch_indev);
    if (th != nullptr)
        th->config.process_coordinates = nullptr;
}

static void create_test_pattern()
{
    auto screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, KACHEL_BG_REST, LV_PART_MAIN);

    static const lv_color_t quad_colors[4] = {
        KACHEL_SURFACE_LIGHTS, KACHEL_SURFACE_AIR,
        KACHEL_SURFACE_HOUSE, KACHEL_NIGHT_AMBER};
    static const char *quad_names[4] = {"lights", "air", "house", "amber"};

    for (int i = 0; i < 4; i++)
    {
        auto quad = lv_obj_create(screen);
        lv_obj_remove_style_all(quad);
        lv_obj_set_size(quad, 240, 240);
        lv_obj_set_pos(quad, (i % 2) * 240, (i / 2) * 240);
        lv_obj_set_style_bg_color(quad, quad_colors[i], LV_PART_MAIN);
        lv_obj_set_style_bg_opa(quad, LV_OPA_COVER, LV_PART_MAIN);
        // let touches fall through to the screen for coordinate logging
        lv_obj_remove_flag(quad, LV_OBJ_FLAG_CLICKABLE);

        auto tag = lv_label_create(quad);
        lv_label_set_text(tag, quad_names[i]);
        lv_obj_set_style_text_color(tag, KACHEL_TEXT_DIM, LV_PART_MAIN);
        // corner tags identify quadrant order -> orientation check
        lv_obj_align(tag, LV_ALIGN_TOP_LEFT, 8, 8);
    }

    // M1 task 2: temporary touch-coordinate logging, removed after verification
    lv_obj_add_event_cb(screen, [](lv_event_t *e)
    {
        auto code = lv_event_get_code(e);
        if (code != LV_EVENT_PRESSED && code != LV_EVENT_RELEASED)
            return;
        lv_point_t p;
        lv_indev_get_point(lv_indev_active(), &p);
        log_i("touch %s x=%d y=%d", code == LV_EVENT_PRESSED ? "down" : "up", p.x, p.y);
    }, LV_EVENT_ALL, nullptr);

    auto wordmark = lv_label_create(screen);
    lv_label_set_text(wordmark, "KACHEL");
    lv_obj_set_style_text_color(wordmark, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(wordmark, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_center(wordmark);
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
    create_test_pattern();
    log_i("Test pattern up");
}

void loop()
{
    static uint32_t lv_last_tick = millis();
    auto const now = millis();
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    lv_timer_handler();
}
