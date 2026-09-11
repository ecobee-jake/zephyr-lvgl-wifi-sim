#!/bin/sh
# Host-side proof the interface scans real APs. No Zephyr involved.
# Run: sudo ./scripts/test-real-scan.sh [iface]   (default zreal)
set -u
IFACE=${1:-zreal}; OUT=/tmp/real-scan.out
[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
ip link show "$IFACE" >/dev/null 2>&1 || { echo "FAIL: $IFACE missing"; exit 1; }
ip link set "$IFACE" up

# One scan, captured once. Retry: EBUSY is common while NM's supplicant scans the other vif.
for i in 1 2 3 4; do
    iw dev "$IFACE" scan >"$OUT" 2>&1 && break
    sleep 2
done
n=$(grep -c "SSID:" "$OUT")
if [ "$n" -gt 0 ]; then
    echo "OK: $IFACE saw $n SSIDs"; grep "SSID:" "$OUT" | sort -u | head -20
else
    echo "FAIL: no SSIDs on $IFACE"; cat "$OUT"; exit 1
fi
