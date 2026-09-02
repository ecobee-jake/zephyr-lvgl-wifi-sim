#!/bin/sh
# Tear down the hwsim dev radios: stop hostapd, unload mac80211_hwsim
# (which deletes zwifi, zap, hwsim0). Real Wi-Fi is untouched.
# Run: sudo ./scripts/clean-up-dev-iface.sh
set -u

PID=/tmp/hostapd-zap.pid
CONF=/tmp/ap.conf

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

[ -f "$PID" ] && { kill "$(cat "$PID")" 2>/dev/null || true; rm -f "$PID"; }
pkill -f "hostapd .*$CONF" 2>/dev/null || true
rm -f "$CONF"

pgrep -x zephyr.exe >/dev/null && echo "note: zephyr.exe still running; kill it first if unload fails"

modprobe -r mac80211_hwsim 2>/dev/null || echo "mac80211_hwsim busy or not loaded"

echo "remaining wireless interfaces:"
iw dev | grep Interface || true
