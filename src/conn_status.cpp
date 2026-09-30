#include "conn_status.h"

#include <Arduino.h>
#include <WiFi.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "../include/secrets.h"
#include "config.h"
#include "mqtt_client.h"
#include "palette.h"
#include "timing.h"

LV_FONT_DECLARE(font_guest_22);
LV_FONT_DECLARE(font_text_22);

static constexpr int MAX_INSTANCES = 2; // lights + air
static constexpr int MAX_CONTROLS = 5;

struct conn_mark
{
    lv_obj_t *tile;
    lv_obj_t *mark;
    lv_obj_t *mark_label;
    lv_obj_t *card;
    lv_obj_t *card_label;
    lv_obj_t *controls[MAX_CONTROLS];
    int n_controls;
    int shown_offline; // -1 = unset, 0 = ring, 1 = pill
};

static conn_mark marks[MAX_INSTANCES];
static int n_marks;

static void set_text_if_changed(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) != 0)
        lv_label_set_text(label, text);
}

// compact age: 42s / 12m / 3h / 2d
static void fmt_age(char *buf, size_t len, int32_t s)
{
    if (s < 0)
        snprintf(buf, len, "--");
    else if (s < 60)
        snprintf(buf, len, "%lds", (long)s);
    else if (s < 3600)
        snprintf(buf, len, "%ldm", (long)(s / 60));
    else if (s < 48 * 3600)
        snprintf(buf, len, "%ldh", (long)(s / 3600));
    else
        snprintf(buf, len, "%ldd", (long)(s / 86400));
}

// PubSubClient state codes, German short form
static const char *rc_text(int rc)
{
    switch (rc)
    {
    case -4: return "Timeout";
    case -3: return "Verbindung verloren";
    case -2: return "nicht erreichbar";
    case -1: return "getrennt";
    case 1: return "Protokoll abgelehnt";
    case 2: return "Client-ID abgelehnt";
    case 3: return "Broker nicht bereit";
    case 4: return "Login falsch";
    case 5: return "nicht autorisiert";
    default: return "unbekannt";
    }
}

// "HH:MM" wall time of an event ago_s seconds back; false while clock invalid
static bool clock_ago(char *buf, size_t len, uint32_t ago_s)
{
    time_t t = time(nullptr);
    if (t < 1700000000) // SNTP not synced yet
        return false;
    t -= ago_s;
    struct tm lt;
    localtime_r(&t, &lt);
    snprintf(buf, len, "%02d:%02d", lt.tm_hour, lt.tm_min);
    return true;
}

// append to buf at *n, clamped so a full buffer just truncates
static void put(char *buf, size_t len, size_t &n, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
static void put(char *buf, size_t len, size_t &n, const char *fmt, ...)
{
    if (n >= len)
        return;
    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buf + n, len - n, fmt, ap);
    va_end(ap);
}

// card width 448 - 2x18 pad at 14 px mono pitch = 29 columns per line
static void render_card(conn_mark &m, const kachel_link &link, uint32_t down_s)
{
    char text[400];
    size_t n = 0;

    if (WiFi.status() == WL_CONNECTED)
    {
        put(text, sizeof(text), n, "WLAN   %.12s %d dBm\n", WiFi.SSID().c_str(), WiFi.RSSI());
        put(text, sizeof(text), n, "IP     %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        put(text, sizeof(text), n, "WLAN   getrennt\n");
    }

    put(text, sizeof(text), n, "Server %s\n", MQTT_HOST);
    char when[8];
    bool have_when = clock_ago(when, sizeof(when), down_s);
    char dur[8];
    fmt_age(dur, sizeof(dur), down_s);
    if (link.broker_up)
    {
        if (have_when)
            put(text, sizeof(text), n, "       verbunden seit %s\n", when);
        else
            put(text, sizeof(text), n, "       verbunden\n");
    }
    else
    {
        if (!link.ever_connected)
            put(text, sizeof(text), n, "       nie verbunden\n");
        else if (have_when)
            put(text, sizeof(text), n, "       weg seit %s (%s)\n", when, dur);
        else
            put(text, sizeof(text), n, "       weg seit %s\n", dur);
        put(text, sizeof(text), n, "       %s · %lux\n", rc_text(link.last_rc), (unsigned long)link.failed_tries);
    }

    char a[8], w[8], c[8], b[8];
    fmt_age(a, sizeof(a), mqtt_state_age_s(KACHEL_TOPIC_AIR));
    fmt_age(w, sizeof(w), mqtt_state_age_s(KACHEL_TOPIC_WEATHER));
    fmt_age(c, sizeof(c), mqtt_state_age_s(KACHEL_TOPIC_CALENDAR));
    fmt_age(b, sizeof(b), mqtt_state_age_s(KACHEL_TOPIC_BRING));
    put(text, sizeof(text), n, "Daten  Luft %s · Wetter %s\n", a, w);
    put(text, sizeof(text), n, "       Kal. %s · Bring %s\n", c, b);

    char up[8];
    fmt_age(up, sizeof(up), (int32_t)(millis() / 1000));
    put(text, sizeof(text), n, "fw %s · up %s", KACHEL_FW_VERSION, up);

    set_text_if_changed(m.card_label, text);
}

static void set_offline_look(conn_mark &m, bool offline)
{
    if (m.shown_offline == (int)offline)
        return;
    m.shown_offline = offline;
    for (int i = 0; i < m.n_controls; i++)
    {
        if (offline)
            lv_obj_add_state(m.controls[i], LV_STATE_DISABLED);
        else
            lv_obj_remove_state(m.controls[i], LV_STATE_DISABLED);
    }
    if (offline)
    {
        // pill: opaque so it reads cleanly over the dimmed button corners
        lv_obj_set_size(m.mark, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(m.mark, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(m.mark, 1, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(m.mark, 16, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(m.mark, 6, LV_PART_MAIN);
        lv_obj_remove_flag(m.mark_label, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        // ring: hollow, so it never reads as a §5.6 staleness dot
        lv_obj_set_size(m.mark, 12, 12);
        lv_obj_set_style_bg_opa(m.mark, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(m.mark, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_all(m.mark, 0, LV_PART_MAIN);
        lv_obj_add_flag(m.mark_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void refresh(lv_timer_t *)
{
    auto link = mqtt_link();
    uint32_t since_ms = millis() - link.changed_ms;
    uint32_t since_s = since_ms / 1000;
    bool offline = !link.broker_up && since_ms >= KACHEL_T_OFFLINE_GRACE_MS;

    char pill[32];
    if (since_s < 60)
        snprintf(pill, sizeof(pill), "Offline");
    else
    {
        char dur[8];
        fmt_age(dur, sizeof(dur), since_s);
        // "12m" -> "12 min" reads calmer on the pill; h/d stay compact
        if (since_s < 3600)
            snprintf(pill, sizeof(pill), "Offline · %lu min", (unsigned long)(since_s / 60));
        else
            snprintf(pill, sizeof(pill), "Offline · %s", dur);
    }

    for (int i = 0; i < n_marks; i++)
    {
        auto &m = marks[i];
        set_offline_look(m, offline);
        if (offline)
            set_text_if_changed(m.mark_label, pill);

        if (lv_obj_has_flag(m.card, LV_OBJ_FLAG_HIDDEN))
            continue;
        // leaving the layer (swipe or idle return) closes the card
        if (lv_tileview_get_tile_active(lv_obj_get_parent(m.tile)) != m.tile)
        {
            lv_obj_add_flag(m.card, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        render_card(m, link, since_s);
    }
}

static void mark_clicked(lv_event_t *e)
{
    auto &m = *(conn_mark *)lv_event_get_user_data(e);
    auto link = mqtt_link();
    render_card(m, link, (millis() - link.changed_ms) / 1000);
    lv_obj_remove_flag(m.card, LV_OBJ_FLAG_HIDDEN);
}

static void card_clicked(lv_event_t *e)
{
    auto &m = *(conn_mark *)lv_event_get_user_data(e);
    lv_obj_add_flag(m.card, LV_OBJ_FLAG_HIDDEN);
}

void conn_status_attach(lv_obj_t *tile, lv_align_t align, int32_t x, int32_t y,
                        lv_obj_t *const *controls, int n_controls)
{
    if (n_marks >= MAX_INSTANCES)
        return;
    auto &m = marks[n_marks++];
    m.tile = tile;
    m.shown_offline = -1;
    m.n_controls = min(n_controls, MAX_CONTROLS);
    for (int i = 0; i < m.n_controls; i++)
    {
        m.controls[i] = controls[i];
        // static dim, no motion: offline is an info-class change (§5.5)
        lv_obj_set_style_opa(controls[i], LV_OPA_40, LV_PART_MAIN | LV_STATE_DISABLED);
    }

    m.mark = lv_obj_create(tile);
    lv_obj_remove_flag(m.mark, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(m.mark, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(m.mark, 24); // 12 px ring -> 60 px target (§4)
    lv_obj_set_style_radius(m.mark, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(m.mark, KACHEL_BG_REST, LV_PART_MAIN);
    lv_obj_set_style_border_color(m.mark, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_opa(m.mark, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(m.mark, 0, LV_PART_MAIN);
    lv_obj_align(m.mark, align, x, y);
    lv_obj_add_event_cb(m.mark, mark_clicked, LV_EVENT_CLICKED, &m);

    m.mark_label = lv_label_create(m.mark);
    lv_obj_set_style_text_font(m.mark_label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(m.mark_label, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
    lv_label_set_text(m.mark_label, "Offline");
    lv_obj_center(m.mark_label);

    m.card = lv_obj_create(tile);
    lv_obj_remove_flag(m.card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(m.card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(m.card, 448, LV_SIZE_CONTENT);
    lv_obj_center(m.card);
    lv_obj_set_style_radius(m.card, 24, LV_PART_MAIN);
    lv_obj_set_style_bg_color(m.card, KACHEL_BG_REST, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(m.card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(m.card, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(m.card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(m.card, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(m.card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m.card, 18, LV_PART_MAIN);
    lv_obj_add_flag(m.card, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(m.card, card_clicked, LV_EVENT_CLICKED, &m);

    m.card_label = lv_label_create(m.card);
    lv_obj_set_width(m.card_label, lv_pct(100)); // wrap, never clip
    lv_obj_set_style_text_font(m.card_label, &font_text_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(m.card_label, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(m.card_label, 4, LV_PART_MAIN);
    lv_label_set_text(m.card_label, "");

    set_offline_look(m, false);
    if (n_marks == 1)
        lv_timer_create(refresh, KACHEL_T_LAYER_POLL_MS, nullptr);
}
