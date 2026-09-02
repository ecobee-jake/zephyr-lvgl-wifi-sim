#!/bin/sh
# Host-side radio for Zephyr native_sim Wi-Fi scan testing (PR #110681).
#   zwifi = mac80211_hwsim STA the Zephyr driver binds to (DT host-interface)
#   zap   = mac80211_hwsim radio running an open hostapd AP "zephyr-open"
# Run: sudo ./scripts/bring-up-dev-iface.sh
set -eu

STA=zwifi
AP=zap
SSID=zephyr-open
CONF=/tmp/ap.conf
PID=/tmp/hostapd-zap.pid

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

# 1. radios (no-op if module already loaded)
lsmod | grep -q '^mac80211_hwsim' || modprobe mac80211_hwsim radios=2

# 2. hwsim netdevs, from sysfs (authoritative; ignores hwsim0 monitor)
names=""
for r in /sys/devices/virtual/mac80211_hwsim/hwsim*/net/*; do
    [ -e "$r" ] && names="$names $(basename "$r")"
done
set -- $names
[ $# -ge 2 ] || { echo "need 2 hwsim netdevs, found:$names"; exit 1; }

# 3. rename to fixed names (skip any already named)
have() { case " $names " in *" $1 "*) return 0;; *) return 1;; esac; }
rename() { ip link set "$1" down; ip link set "$1" name "$2"; }
if ! have "$STA"; then
    for n in "$@"; do [ "$n" != "$AP" ] && { rename "$n" "$STA"; break; }; done
fi
names=$(for r in /sys/devices/virtual/mac80211_hwsim/hwsim*/net/*; do basename "$r"; done)
if ! have "$AP"; then
    for n in $names; do [ "$n" != "$STA" ] && { rename "$n" "$AP"; break; }; done
fi

# 4. keep NetworkManager's supplicant off both (avoids "Match already configured")
if command -v nmcli >/dev/null 2>&1; then
    nmcli device set "$STA" managed no >/dev/null 2>&1 || true
    nmcli device set "$AP"  managed no >/dev/null 2>&1 || true
fi
[ -f "$PID" ] && { kill "$(cat "$PID")" 2>/dev/null || true; rm -f "$PID"; }
pkill -x hostapd 2>/dev/null || true
sleep 1
ip link set "$STA" up
ip link set "$AP" down          # hostapd brings it up

# 5. AP (restart cleanly if a previous one is still running)
rm -f "$CONF"
printf 'interface=%s\nssid=%s\nhw_mode=g\nchannel=1\n' "$AP" "$SSID" > "$CONF"
hostapd -B -P "$PID" "$CONF"

# 6. verify from the STA side
sleep 2
if iw dev "$STA" scan 2>/dev/null | grep -q "SSID: $SSID"; then
    echo "OK: $STA sees $SSID on $AP"
else
    echo "FAIL: $STA did not see $SSID"; iw dev | grep -E "Interface|type"; exit 1
fi
