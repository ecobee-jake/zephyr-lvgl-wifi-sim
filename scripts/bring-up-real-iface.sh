#!/bin/sh
# Netdev on the Intel card that NetworkManager does not own.
# Default: second managed vif "zreal" on phy0. --direct: take over wlp0s20f3.
# Run: sudo ./scripts/bring-up-real-iface.sh [--direct]
set -u
PHY=phy0; REAL=wlp0s20f3; VIF=zreal
[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

# --direct after a half-succeeded zreal attempt: drop the stray vif so NM can't re-grab it.
ip link show "$VIF" >/dev/null 2>&1 && [ "${1:-}" = "--direct" ] && iw dev "$VIF" del

if [ "${1:-}" = "--direct" ]; then
    IFACE=$REAL
else
    IFACE=$VIF
    if ! ip link show "$VIF" >/dev/null 2>&1; then
        iw phy "$PHY" interface add "$VIF" type managed || {
            echo "FAIL: $PHY refused a second managed vif; rerun with --direct"; exit 1; }
    fi
fi

# NM may grab a new vif before nmcli sees it; retry until it sticks.
for i in 1 2 3; do
    nmcli device set "$IFACE" managed no >/dev/null 2>&1 && break
    sleep 1
done
ip link set "$IFACE" up
echo "READY: $IFACE  -> host-interface = \"$IFACE\" in boards/native_sim.overlay"
