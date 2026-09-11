#!/bin/sh
# Tear down the hwsim dev radios: stop hostapd + dnsmasq, drop the NAT rules,
# unload mac80211_hwsim (which deletes zwifi, zap, hwsim0). Real Wi-Fi is untouched.
# Run: sudo ./scripts/clean-up-dev-iface.sh
set -u

PID=/tmp/hostapd-zap.pid
CONF=/tmp/ap.conf
DNSMASQ_PID=/tmp/dnsmasq-zap.pid
SUBNET=192.168.66
RT_TABLE=66

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

[ -f "$PID" ] && { kill "$(cat "$PID")" 2>/dev/null || true; rm -f "$PID"; }
pkill -f "hostapd .*$CONF" 2>/dev/null || true
rm -f "$CONF"

[ -f "$DNSMASQ_PID" ] && { kill "$(cat "$DNSMASQ_PID")" 2>/dev/null || true; rm -f "$DNSMASQ_PID"; }

# Drop only what bring-up added. Uplink is re-derived the same way, so a changed
# default route is tolerated. bring-up guards with -C, so there is never a duplicate.
UPLINK=$(ip route show default | awk '{print $5; exit}')
[ -n "$UPLINK" ] &&
    iptables -t nat -D POSTROUTING -s "$SUBNET.0/24" -o "$UPLINK" -j MASQUERADE 2>/dev/null

# The VPN-bypass policy rule and its table (bring-up section 5).
ip rule del from "$SUBNET.0/24" lookup "$RT_TABLE" 2>/dev/null || true
ip route flush table "$RT_TABLE" 2>/dev/null || true

# ip_forward is left as-is: it is a global host setting that may predate this script.

pgrep -x zephyr.exe >/dev/null && echo "note: zephyr.exe still running; kill it first if unload fails"

modprobe -r mac80211_hwsim 2>/dev/null || echo "mac80211_hwsim busy or not loaded"

echo "remaining wireless interfaces:"
iw dev | grep Interface || true
