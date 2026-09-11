#!/bin/sh
# Undo bring-up-real-iface.sh. Run: sudo ./scripts/clean-up-real-iface.sh
set -u
REAL=wlp0s20f3
[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
pgrep -x zephyr.exe >/dev/null && echo "note: zephyr.exe still running"

echo "handing $REAL back to NetworkManager..."
nmcli device set "$REAL" managed yes ||
    echo "warn: nmcli refused $REAL; kill zephyr.exe and rerun"

# NM has to scan and autoconnect once it owns the device again, which takes a few
# seconds. Waiting here is the whole point: without it the script returned before
# NM was done and looked like it had failed, so it got run a second time.
echo "waiting for NM to scan and autoconnect (up to 20s)..."
n=0
while [ "$n" -lt 20 ]; do
    state=$(nmcli -t -f DEVICE,STATE device status | grep "^$REAL:" | cut -d: -f2)
    echo "  ${n}s: $REAL is ${state:-missing}"
    [ "$state" = "connected" ] && break
    n=$((n + 1))
    sleep 1
done
[ "${state:-}" = "connected" ] && echo "OK: $REAL reconnected" ||
    echo "WARN: $REAL still not connected after ${n}s; check nmcli device status"
nmcli -f DEVICE,STATE,CONNECTION device status | grep -E "^DEVICE|^$REAL"
