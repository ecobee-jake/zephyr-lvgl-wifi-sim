#!/bin/sh
# Hand the real Intel card to Zephyr: take wlp0s20f3 off NetworkManager and bring
# it up. Your host Wi-Fi drops until scripts/clean-up-real-iface.sh runs.
#
# Taking over the primary interface is the only option on this card: phy0
# advertises "#{ managed } <= 1", so a second managed vif can be created but never
# brought up while wlp0s20f3 exists. That path used to live here behind a default
# --direct flag; it could not work, so it is gone.
# Run: sudo ./scripts/bring-up-real-iface.sh
set -u
IFACE=wlp0s20f3
[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

# NM does not always act on the first ask; retry until it sticks.
for i in 1 2 3; do
    nmcli device set "$IFACE" managed no >/dev/null 2>&1 && break
    sleep 1
done
ip link set "$IFACE" up || { echo "FAIL: cannot bring $IFACE up"; exit 1; }
echo "READY: $IFACE taken from NetworkManager"
echo "       build with -DEXTRA_DTC_OVERLAY_FILE=overlays/real-card.overlay"
