# Smart-Home-Plan

Stand: 2026-09-30. Ergebnis der Grilling-Session zu KAJPLATS, dem Tapo-Küchenstecker und dem Aufräumen des Setups.
Die Begriffe stehen in [CONTEXT.md](../CONTEXT.md), die Thread-Entscheidung in [ADR 0001](adr/0001-ha-owns-thread-network.md).

**Legende:** **[Du]** = physisch, Logins, Apps, Käufe. **[Claude]** = per HA-API, du gibst frei.
Menünamen in der Fritz!Box und in den Apps können je nach Version leicht abweichen.

## Übergabe für den nächsten Agenten

**Stand 2026-09-30:** Alle Entscheidungen sind getroffen und unten dokumentiert. In Home Assistant wurde noch **nichts geändert**; die Session hat nur gelesen. Es geht ans Umsetzen.

**Es wartet auf Josch:**
- ESP-Teile bestellen (Einkaufsliste).
- Flurbirne per Touchlink an die Hue Bridge koppeln (1.1). Der erste Versuch am 2026-09-29 hat nicht geklappt, die App fand die Birne nicht.
- Phase 0: Schritte 0.1, 0.2, 0.3, 0.5, 0.6 und der Hue-Teil von 0.4.
- Wohnort in HA prüfen (Einstellungen → System → Allgemein), weil `sun.sun` daraus rechnet. Das konnte nicht per API geprüft werden.

**Es wartet auf den Agenten** (jeder Schreibzugriff auf HA erst nach Joschs Go):
1. 0.4: Bereich „Flur“ anlegen, Schreibi nach „Büro“, Bereich „Josch's Schreibtisch“ löschen.
2. 1.2: Sobald die Flurbirne in HA auftaucht, die Entity in `light.flur` umbenennen, Bereich „Flur“. Die Szenen `kachel_s1` und `kachel_s2` um `light.flur` und `light.buro_frau_marschik` erweitern.
3. 2.2: ESPHome-Konfiguration für den Flur-Sensor schreiben, sobald die Teile da sind.
4. 2.4: Die Automation anlegen. Der Entity-Name `binary_sensor.flur_praesenz` ist angenommen und muss zur ESPHome-Konfiguration passen.

**Zugang zu HA:**
- `http://192.168.178.77:8123`, nur aus dem Heimnetz erreichbar. Token in `.kachel/ha_token` (gitignored).
- Zum Lesen hat sich `curl` mit `POST /api/template` bewährt. Python-`urllib` ist einmal mit „No route to host“ gescheitert, während `curl` lief.
- MQTT-Broker auf demselben Host, Zugangsdaten in `include/secrets.h`. Die Kachel meldet sich auf `kachel/sys/status`.

**Fallen:**
- **`ha/setup_m2.py` nicht einfach neu ausführen.** Die Entity-IDs im Skript sind Platzhalter. Ein Lauf überschreibt die echten Szenen und Automationen in HA. Live enthalten `kachel_s1` und `kachel_s2`: `switch.oma`, `switch.kuche`, `switch.schreibi`, `switch.doni`, `light.limette`.
- Szenen-Entity-IDs weichen von den Konfig-IDs ab: `kachel_s1` heißt in HA `scene.kachel_1_alles_an`, `kachel_s2` heißt `scene.kachel_2_alles_aus`.
- Tapo-P100-Firmware nicht aktualisieren (siehe Ist-Stand).
- Frau Marschik erscheint in HA ohne Hersteller und mit Firmware „0.0.0“. Das ist bei einer per Touchlink gekoppelten KAJPLATS normal.
- Das Repo ist öffentlich. Die zweite Bewohnerin wird in der Doku nicht namentlich genannt.
- Der Matter-Server in HA läuft, hat aber noch keine Geräte.

## Zielbild

| Rolle | Wer | Aufgabe |
|---|---|---|
| Brain | Home Assistant (Pi 4, LAN, .77) | Hält alle Logik: Szenen, Zeitpläne, Automationen |
| Voice channel | Alexa (Echo Dot 3) | Darf schalten, hält keine Logik |
| Radio bridge | Hue Bridge v2 | Zigbee-Funk, auch für die IKEA-Birnen im Zigbee-Modus |
| Thread radio | ESP32-C6 (Test), sonst HA Connect ZBT-2 | **Backlog.** Kommt erst, wenn ein Gerät Thread wirklich braucht |
| Failsafe | Schalter an der Lampe / Knopf am Stecker | Funktioniert ohne Brain |

**Kaufregel:** Gekaufte Fertiggeräte (Sensoren, Stecker, Lampen) nur noch als **Matter-over-Thread**. Selbstbau läuft über **ESPHome** (WLAN, lokal). Kein eigenes Zigbee-Netz aufbauen. Die Hue Bridge bleibt nur für die zwei KAJPLATS im Zigbee-Modus und fällt weg, sobald sie auf Thread umgezogen sind. Als Sensoren nimmt die Hue Bridge ohnehin praktisch nur Hue-Geräte an.

## Ist-Stand

| Gerät | Typ | Room | Anbindung | Status |
|---|---|---|---|---|
| Schreibi | Tapo P100 (.26) | Büro | WLAN → HA | ok (Bereich heißt noch „Josch's Schreibtisch“) |
| Döni | Tapo P100 (.29) | Wohnzimmer | WLAN → HA | ok |
| Oma | Tapo P100 (.57) | Wohnzimmer | WLAN → HA | ausgesteckt |
| Küche | Tapo P100 (.58) | Küche | WLAN → HA | fällt am Küchenplatz aus (WLAN-Empfang), steht derzeit neben der Fritz!Box |
| Limette | Tapo L610 (.56) | Wohnzimmer | WLAN → HA | ok, **Auto-Update an** |
| Frau Marschik | IKEA KAJPLATS, Zigbee-Modus | Wohnzimmer | Hue Bridge → HA | läuft stabil (2 Wochen Verlauf geprüft), Hue-Raum heißt noch „Büro“ |
| Flur | IKEA KAJPLATS E27 Farbe+Weiß + BILRESA | Flur | vorerst nur Fernbedienung | wird in Phase 1 per Zigbee-Modus an die Hue Bridge gekoppelt |
| Luftreiniger | Levoit Core 300S | Wohnzimmer | VeSync-Cloud → HA | ok |
| Kachel | ESP32-S3, fw 0.7.0-link | Küche | MQTT → .77 | ok |
| Hue Bridge | BSB002 (.38) | – | LAN | ok |
| Router | FRITZ!Box 7682, FRITZ!OS 8.25 | – | – | kein Zigbee/Thread, kein Repeater |

HA: Core 2026.7.4 (2026.9.4 verfügbar), HAOS 18.3, Matter Server 9.2 (läuft, noch ohne Geräte).
Die P100 laufen auf Firmware 1.5.5 von 2023. **Nicht aktualisieren:** Neuere Tapo-Firmware (TPAP) macht die Steuerung durch HA kaputt.

## Einkaufsliste (ESP-Bestellung)

Preise geschätzt.

| Teil | ca. | Wofür | Stand |
|---|---|---|---|
| HLK-LD2410C (Radar-Präsenzsensor, 2,54-mm-Stiftleiste) | 4 € | Flur-Sensor (Phase 2) | entschieden |
| ESP32-C3 | 4 € | Flur-Sensor | entschieden |
| ESP32-C3 (kein ESP8266, wegen Bluetooth) | 4 € | Velux-Fernbedienung, Bluetooth-Proxy | empfohlen |
| BME280 (Temperatur, Feuchte) | 3 € | Schlafzimmer, am Velux-ESP | empfohlen |
| 2× ESP32-C6 mit Antennenanschluss + externe 2,4-GHz-Antenne (U.FL) | 19 € | Thread radio, Ersatz oder Thread-Verstärker Richtung Küche | empfohlen |
| USB-2-Verlängerung | 5 € | Thread radio am Pi | empfohlen |
| USB-Netzteil + Kabel für den Flur | vorhanden? | Flur-Sensor | prüfen |

---

## Phase 0 – jetzt, ohne Kauf

### 0.1 Feste IPs in der Fritz!Box [Du]
HA merkt sich die IP-Adressen der Tapo-Geräte. Wechselt eine, meldet HA das Gerät als „nicht verfügbar“.
1. `http://fritz.box` → Heimnetz → Netzwerk → Netzwerkverbindungen.
2. Beim Gerät auf den Stift → „Diesem Netzwerkgerät immer die gleiche IPv4-Adresse zuweisen“ → Übernehmen.
3. Das für folgende Geräte machen: Pi (LAN, .77), Hue Bridge (.38), Schreibi (.26), Döni (.29), Limette (.56), Oma (.57), Küche (.58).

### 0.2 Tapo-Auto-Updates aus [Du]
1. Tapo-App → Limette → Zahnrad → Firmware-Update → Automatisches Update **aus**.
2. Bei allen vier P100 prüfen, dass Auto-Update aus ist.

### 0.3 HA Core aktualisieren [Du]
Einstellungen → System → Updates → Home Assistant Core 2026.9.4 → Installieren.
Tipp: Im Update-Dialog das Häkchen „Backup erstellen“ setzen. Das ist eine einmalige Sicherung.

### 0.4 Rooms angleichen [Claude + Du]
- [Claude] Bereich **Flur** anlegen, Schreibi in **Büro** verschieben, Bereich „Josch's Schreibtisch“ löschen.
- [Du] Hue-App: Frau Marschik in einen Raum **Wohnzimmer** verschieben, den leeren Raum „Büro“ löschen.

### 0.5 HACS-Reste entfernen [Du]
HACS → „Alexa Media Player“ → ⋮ → Entfernen. Dasselbe für „Velux Active with Netatmo“. Danach HA neu starten.

### 0.6 Alexa-Gruppen pro Room [Du]
Alexa-App → Geräte → + → Gruppe hinzufügen. Gruppen **Wohnzimmer**, **Küche**, **Büro** und **Flur** mit den passenden Geräten anlegen.

---

## Phase 1 – Flur-KAJPLATS an die Hue Bridge (Zigbee-Modus)

Dieser Weg hat bei Frau Marschik funktioniert. Die BILRESA-Fernbedienung funktioniert danach nicht mehr, und die Birne bekommt keine Firmware-Updates mehr.

### 1.1 Koppeln [Du]
1. Die Birne in eine Lampe **direkt neben der Hue Bridge** schrauben, am besten eine mit Kabelschalter oder an einer schaltbaren Steckdosenleiste.
2. **12-mal aus- und einschalten:** jeweils ca. 0,7 s an, 1,5 s aus. Nach dem 12. Einschalten pulsiert die Birne kaltweiß. Das ist der Zigbee-Kopplungsmodus. Wenn nicht: 10 s warten und mit **15** Zyklen wiederholen.
3. Innerhalb weniger Minuten in **Hue Essentials** (Premium): Geräte → + → Lampe → Bridge wählen → **Touchlink** → „Nach Lampen suchen“. Die Birne blinkt und wird hinzugefügt.
   Die offizielle Hue-App findet die Birne nicht, weil sie kein Touchlink kann. iConnectHue kann Touchlink auch; nimm die App, für die du schon bezahlt hast.
4. **Hue-App:** Die neue Lampe taucht als „nicht zugewiesen“ auf → **Flur** nennen → einen Raum **Flur** anlegen und die Lampe zuweisen.
5. Die Birne in die richtige Flurlampe schrauben.

### 1.2 In HA übernehmen [Claude]
- Die Hue-Integration nimmt die Lampe automatisch auf. Die Entity in `light.flur` umbenennen, Bereich **Flur**.
- „Kachel 1 (alles an)“ und „Kachel 2 (alles aus)“ bekommen `light.flur` und `light.buro_frau_marschik` dazu.

### 1.3 Alexa [Du]
„Alexa, suche neue Geräte“. Die Lampe kommt über die Hue-Verbindung von Alexa. Danach den Flur in die Alexa-Gruppe **Flur** aufnehmen.

### 1.4 Abnahme [Du]
- [ ] Der Flur lässt sich über HA, Alexa und die Kachel („Alles an“/„Alles aus“) schalten und dimmen.
- [ ] Lampenschalter aus, 10 s warten, wieder an → das Licht brennt und ist innerhalb von ca. 1 Minute in HA wieder erreichbar.

**Wenn An/Aus nicht funktioniert** (bei einigen Nutzern bekannt): die Birne in der Hue-App löschen, 6-mal aus- und einschalten (Reset, sie blinkt warmweiß), 10 s warten, dann 1.1 mit 12 oder 15 Zyklen wiederholen.

---

## Phase 2 – Flur-Sensor im Selbstbau (ESPHome)

Im Flur ist eine Steckdose vorhanden. Der Sensor läuft im WLAN und braucht kein Thread.

### 2.1 Aufbau [Du]
| LD2410C | ESP32-C3 |
|---|---|
| VCC | 5V |
| GND | GND |
| TX | freier GPIO als RX |
| RX | freier GPIO als TX |

Der Radar braucht 5 V Versorgung, seine Datenpins arbeiten mit 3,3 V. UART-Geschwindigkeit: 256000 Baud.

### 2.2 Firmware [Claude + Du]
- [Du] HA → Apps → „ESPHome Device Builder“ installieren.
- [Claude] ESPHome-Konfiguration schreiben: `ld2410` (Präsenz), feste IP.
- [Du] Einmal per USB flashen, danach Updates über WLAN. Das Gerät in HA dem Bereich **Flur** zuordnen.

### 2.3 Einstellen [Du]
Der Radar sieht durch dünne Türen und Trockenbauwände. In der App „HLKRadarTool“ (Bluetooth) die Reichweite so weit verringern, dass Bewegung im Nachbarraum nicht mehr auslöst.

### 2.4 Automation: Licht bei Präsenz im Dunkeln [Claude]
Der Sensor meldet nur, ob jemand im Flur ist. Ob es Dark ist, entscheidet das Brain aus dem Sonnenstand am Wohnort (`sun.sun`), nicht aus einer Messung. Der Flur hat kein Fenster; einen Lichtsensor gibt es bewusst nicht. Tagsüber bleibt das Licht aus.

Vor dem Bau prüfen: Einstellungen → System → Allgemein. Der Wohnort in HA muss stimmen, sonst rechnet `sun.sun` falsch.

Alle Zahlen sind Startwerte und werden vor Ort nachgestellt: Schwelle 3°, 70 % / 10 %, Nachtfenster 22–6 Uhr, Nachlauf 2 Minuten.
```yaml
alias: "Flur: Licht bei Präsenz im Dunkeln"
mode: restart
triggers:
  - trigger: state
    entity_id: binary_sensor.flur_praesenz
    to: "on"
    id: kommt
  - trigger: state
    entity_id: binary_sensor.flur_praesenz
    to: "off"
    for: "00:02:00"
    id: geht
actions:
  - choose:
      - conditions:
          - condition: trigger
            id: kommt
          - condition: numeric_state      # Dark = Sonne < 3° über dem Horizont
            entity_id: sun.sun
            attribute: elevation
            below: 3
        sequence:
          - choose:
              - conditions:
                  - condition: time       # Nacht, wie der Nachtmodus der Kachel
                    after: "22:00:00"
                    before: "06:00:00"
                sequence:
                  - action: light.turn_on
                    target: { entity_id: light.flur }
                    data: { brightness_pct: 10, color_temp_kelvin: 2200 }
            default:
              - action: light.turn_on
                target: { entity_id: light.flur }
                data: { brightness_pct: 70, color_temp_kelvin: 2700 }
      - conditions:
          - condition: trigger
            id: geht
        sequence:
          - action: light.turn_off
            target: { entity_id: light.flur }
```

### 2.5 Abnahme [Du]
- [ ] Abends in den Flur gehen → Licht geht mit 70 % an, zwei Minuten nach dem Verlassen wieder aus.
- [ ] Nach 22 Uhr → Licht geht mit 10 % an.
- [ ] Tagsüber → Licht bleibt aus.
- [ ] Bewegung im Nachbarraum bei geschlossener Tür löst nicht aus.

---

## Backlog – Thread radio
Kommt, sobald ein Gerät Thread wirklich braucht. Voraussichtlich ist das der Küchenstecker.

**Erst testen: ESP32-C6 (ca. 5–10 €).** Ein Board mit Antennenanschluss nehmen, zum Beispiel XIAO ESP32-C6. Firmware: Espressifs OpenThread RCP aus dem esp-idf. Das busware-Projekt „esp-coordinator“ ist Zigbee-Firmware und passt hier nicht.
**Abbruchkriterium:** Läuft der C6 zwei Wochen stabil und erreicht er die Küche, bleibt er. Sonst wird der **ZBT-2** gekauft (ca. 65 € inkl. USB-Verlängerung), und die Thread-Geräte werden neu eingebunden.

Was dann möglich wird:
- **Küche:** GRILLPLATS (Thread, ca. 9 €) statt des WLAN-P100. Alternative ohne Thread radio: Hue Smart Plug (ca. 40 €, Zigbee).
- **BILRESA-Fernbedienung** wieder nutzbar, über eine HA-Automation.
- **Die beiden KAJPLATS** auf Thread umziehen (Reset mit 6 Zyklen, dann per Matter-QR-Code). Dann gibt es wieder Firmware-Updates.
- **Die Hue Bridge ablösen,** sobald kein Zigbee-Gerät mehr übrig ist.
- **`emulated_hue`** für Geräte, die nur HA kennt, mit fester Geräteliste (siehe ADR 0001).

Einrichtung mit dem ESP32-C6:
1. OpenThread-RCP-Firmware flashen. Der Firmware-Stand muss zur OTBR-App passen, sonst gibt es Timeouts.
2. Den C6 über eine USB-Verlängerung an einen USB-2-Port des Pi stecken.
3. HA → Apps → „OpenThread Border Router“ installieren. Gerät: `/dev/serial/by-id/usb-Espressif_USB_JTAG…`. Hardware-Flusskontrolle **aus**.
4. Einstellungen → System → Netzwerk → IPv6 auf **Automatisch**.
5. Einstellungen → Geräte & Dienste → Thread: das HA-Netz als bevorzugtes Netzwerk setzen. In der Companion-App „Zugangsdaten an Telefon senden“.
6. Geräte per Matter-QR-Code in der Companion-App hinzufügen.

Mit dem ZBT-2 entfallen Schritt 1 und 3: HA erkennt den Stick unter „Entdeckt“, dort **Thread** wählen.

## Später
- **„Niemand zu Hause → Alles aus“:** braucht die Erkennung per Handy (Fritz!Box-Integration plus Companion-App auf beiden iPhones, „Private WLAN-Adresse“ auf „Fest“). Für das Flurlicht ist sie nicht nötig, das macht der Präsenzsensor.
- **Stimmungs-Scenes** für Slot 3 und 4, gemeinsam festlegen.
- **Velux (Schlafzimmer, ein Fenster mit Außenrollladen):** Das Fenster selbst öffnet nicht motorisch. Die Fernbedienung hat drei echte Tasten (Modell noch ablesen, vermutlich KLI 310–313). Budget: höchstens 10 €.
  - Weg: die drei Tasten an einen ESP32-C3 anlöten, der sie per ESPHome „drückt“. Fünf Drähte: 3 V, Masse, Auf, Zu, Stopp.
  - Ergebnis: Rollladen Auf/Zu/Stopp in HA und per Alexa, ohne Positionsrückmeldung. Die Fernbedienung funktioniert von Hand weiter.
  - Am selben ESP: BME280 für Temperatur und Feuchte im Schlafzimmer, dazu Bluetooth-Proxy.
  - Risiko: Es ist die einzige Fernbedienung. Ersatz kostet ca. 40–50 €.
  - Verworfen wegen des Budgets: KLF 200, Velux Active, ESP32 mit 868-MHz-Funkmodul.
- **Backups:** bewusst abgelehnt. Das Risiko bleibt: Geht die SD-Karte kaputt, ist die HA-Konfiguration weg.
