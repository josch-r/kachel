#!/usr/bin/env bash
# Thread radio firmware: Espressif's ot_rcp for the XIAO ESP32-C6, USB transport,
# built-in antenna. Usage: ./build.sh [serial-port-to-flash]
set -euo pipefail
IDF=${IDF_PATH:-$HOME/esp/esp-idf-v6.0-beta1} # v6.0-beta1: release/v6.0 drops the link after 10-20 min
HERE=$(cd "$(dirname "$0")" && pwd)
APP=$IDF/examples/openthread/ot_rcp

if ! git -C "$IDF" apply --reverse --check "$HERE/usb-short-write.patch" 2>/dev/null; then
  git -C "$IDF" apply "$HERE/usb-short-write.patch"
fi
. "$IDF/export.sh" >/dev/null

idf=(idf.py -C "$APP" -B "$HERE/build" -D SDKCONFIG="$HERE/build/sdkconfig"
     -D SDKCONFIG_DEFAULTS="$APP/sdkconfig.defaults;$HERE/sdkconfig.usb")
[ -f "$HERE/build/sdkconfig" ] || "${idf[@]}" set-target esp32c6
"${idf[@]}" build
grep -q '^CONFIG_OPENTHREAD_RCP_USB_SERIAL_JTAG=y' "$HERE/build/sdkconfig" # fail loud if USB got dropped
[ $# -eq 0 ] || "${idf[@]}" -p "$1" flash
