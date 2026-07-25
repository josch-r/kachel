#!/usr/bin/env python3
"""Install Kachel M2 automations + stub scenes into HA via config API."""
import json, os, sys, urllib.request

BASE = "http://homeassistant.local:8123"
TOKEN = open(os.path.join(os.path.dirname(__file__), "..", ".kachel", "ha_token")).read().strip()

def api(method, path, body=None):
    req = urllib.request.Request(
        BASE + path, method=method,
        data=json.dumps(body).encode() if body is not None else None,
        headers={"Authorization": f"Bearer {TOKEN}", "Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return r.status, r.read().decode()[:200]
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode()[:300]

FAN = "fan.core_300s_series"

automations = {
    "kachel_pub_air": {
        "alias": "Kachel: publish state/air",
        "triggers": [
            {"trigger": "state", "entity_id": [
                "sensor.core_300s_series_pm25",
                "sensor.core_300s_series_luftqualitat",
                "sensor.core_300s_series_restlebensdauer_des_filters",
                FAN]},
            {"trigger": "homeassistant", "event": "start"},
        ],
        "actions": [{
            "action": "mqtt.publish",
            "data": {
                "topic": "kachel/state/air", "retain": True,
                "payload": (
                    '{"pm25": {{ states("sensor.core_300s_series_pm25") | int(-1) }},'
                    ' "aqi_level": "{{ states("sensor.core_300s_series_luftqualitat") }}",'
                    ' "fan": {% if is_state("' + FAN + '", "off") %}0{% else %}'
                    '{{ (((state_attr("' + FAN + '", "percentage") | int(0)) / 100) * 3) | round(0) | int }}{% endif %},'
                    ' "mode": "{{ state_attr("' + FAN + '", "preset_mode") or "manual" }}",'
                    ' "filter_pct": {{ states("sensor.core_300s_series_restlebensdauer_des_filters") | int(-1) }}}'
                )}}],
        "mode": "single"},

    "kachel_pub_weather": {
        "alias": "Kachel: publish state/weather",
        "triggers": [
            {"trigger": "time_pattern", "minutes": "/15"},
            {"trigger": "homeassistant", "event": "start"},
        ],
        "actions": [
            {"action": "weather.get_forecasts", "target": {"entity_id": "weather.home"},
             "data": {"type": "hourly"}, "response_variable": "fc"},
            {"action": "mqtt.publish", "data": {
                "topic": "kachel/state/weather", "retain": True,
                "payload": (
                    '{% set fc12 = fc["weather.home"]["forecast"][:12] %}'
                    '{{ {"temp": state_attr("weather.home", "temperature"),'
                    ' "condition": states("weather.home"),'
                    ' "precip_12h_mm": (fc12 | map(attribute="precipitation") | sum) | round(1),'
                    ' "sunrise": as_timestamp(state_attr("sun.sun", "next_rising")) | timestamp_custom("%H:%M"),'
                    ' "sunset": as_timestamp(state_attr("sun.sun", "next_setting")) | timestamp_custom("%H:%M")} | tojson }}'
                )}}],
        "mode": "single"},

    "kachel_pub_calendar": {
        "alias": "Kachel: publish state/calendar",
        "triggers": [
            {"trigger": "time_pattern", "minutes": "/5"},
            {"trigger": "homeassistant", "event": "start"},
        ],
        "actions": [
            {"action": "calendar.get_events", "target": {"entity_id": "calendar.shared"},
             "data": {"duration": {"hours": 48}}, "response_variable": "ev"},
            {"action": "mqtt.publish", "data": {
                "topic": "kachel/state/calendar", "retain": True,
                "payload": (
                    '{% set ns = namespace(out=[]) %}'
                    '{% for e in ev["calendar.shared"]["events"][:3] %}'
                    '{% set ns.out = ns.out + [{"title": e.summary, "start": e.start, "cal": "shared"}] %}'
                    '{% endfor %}'
                    '{{ {"next": ns.out} | tojson }}'
                )}}],
        "mode": "single"},

    "kachel_pub_bring": {
        "alias": "Kachel: publish state/bring",
        "triggers": [
            {"trigger": "time_pattern", "minutes": "/5"},
            {"trigger": "state", "entity_id": ["todo.shared_list"]},
            {"trigger": "homeassistant", "event": "start"},
        ],
        "actions": [
            {"action": "todo.get_items", "target": {"entity_id": "todo.shared_list"},
             "data": {"status": "needs_action"}, "response_variable": "items"},
            {"action": "mqtt.publish", "data": {
                "topic": "kachel/state/bring", "retain": True,
                "payload": (
                    '{% set li = items["todo.shared_list"]["items"] %}'
                    '{{ {"count": li | count, "items": li[:5] | map(attribute="summary") | list} | tojson }}'
                )}}],
        "mode": "single"},

    "kachel_pub_timer": {
        "alias": "Kachel: publish state/timer (stub until AMP)",
        "triggers": [{"trigger": "homeassistant", "event": "start"}],
        "actions": [{"action": "mqtt.publish",
                     "data": {"topic": "kachel/state/timer", "retain": True, "payload": "{}"}}],
        "mode": "single"},

    "kachel_cmd_scene": {
        "alias": "Kachel: handle cmd/scene",
        "triggers": [{"trigger": "mqtt", "topic": "kachel/cmd/scene"}],
        "actions": [{
            "choose": [
                {"conditions": [{"condition": "template", "value_template": "{{ trigger.payload_json.id == %d }}" % i}],
                 "sequence": [{"action": "scene.turn_on", "target": {"entity_id": eid}}]}
                for i, eid in {
                    1: "scene.kachel_1_stub_alles_an", 2: "scene.kachel_2_stub_gemuetlich",
                    3: "scene.kachel_3_stub_fokus", 4: "scene.kachel_4_stub_alles_aus"}.items()]}],
        "mode": "single"},

    "kachel_cmd_air": {
        "alias": "Kachel: handle cmd/air",
        "triggers": [{"trigger": "mqtt", "topic": "kachel/cmd/air"}],
        "actions": [{
            "choose": [
                {"conditions": [{"condition": "template", "value_template": "{{ trigger.payload_json.get('mode', '') == 'auto' }}"}],
                 "sequence": [
                     {"action": "fan.turn_on", "target": {"entity_id": FAN}},
                     {"action": "fan.set_preset_mode", "target": {"entity_id": FAN},
                      "data": {"preset_mode": "auto"}}]},
                {"conditions": [{"condition": "template", "value_template": "{{ trigger.payload_json.get('fan', -1) == 0 }}"}],
                 "sequence": [{"action": "fan.turn_off", "target": {"entity_id": FAN}}]}],
            "default": [
                {"action": "fan.turn_on", "target": {"entity_id": FAN}},
                {"action": "fan.set_percentage", "target": {"entity_id": FAN},
                 "data": {"percentage": "{{ [33, 66, 100][(trigger.payload_json.get('fan', 1) | int) - 1] }}"}}]}],
        "mode": "single"},
}

scenes = {
    "kachel_1": {"name": "Kachel 1 (stub: alles an)", "entities": {
        "switch.lamp_a": "on", "switch.lamp_kitchen": "on", "switch.lamp_desk": "on", "switch.lamp_c": "on",
        "light.lamp_main": {"state": "on", "brightness": 255}}},
    "kachel_2": {"name": "Kachel 2 (stub: gemuetlich)", "entities": {
        "switch.lamp_a": "on", "switch.lamp_kitchen": "off", "switch.lamp_desk": "off", "switch.lamp_c": "on",
        "light.lamp_main": {"state": "on", "brightness": 100}}},
    "kachel_3": {"name": "Kachel 3 (stub: fokus)", "entities": {
        "switch.lamp_a": "off", "switch.lamp_kitchen": "on", "switch.lamp_desk": "on", "switch.lamp_c": "off",
        "light.lamp_main": {"state": "on", "brightness": 255}}},
    "kachel_4": {"name": "Kachel 4 (stub: alles aus)", "entities": {
        "switch.lamp_a": "off", "switch.lamp_kitchen": "off", "switch.lamp_desk": "off", "switch.lamp_c": "off",
        "light.lamp_main": "off"}},
}

for sid, scene in scenes.items():
    body = dict(scene, id=sid)
    print(f"scene {sid}:", api("POST", f"/api/config/scene/config/{sid}", body)[0])
for aid, auto in automations.items():
    body = dict(auto, id=aid)
    print(f"automation {aid}:", api("POST", f"/api/config/automation/config/{aid}", body)[0])
print("reload scene:", api("POST", "/api/services/scene/reload", {})[0])
print("reload automation:", api("POST", "/api/services/automation/reload", {})[0])
