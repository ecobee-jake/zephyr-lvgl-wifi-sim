#!/bin/sh
# Host-side radio for Zephyr native_sim Wi-Fi testing (PR #110681).
#   zwifi = mac80211_hwsim STA the Zephyr driver binds to (DT host-interface)
#   zap   = mac80211_hwsim radio running a WPA2 hostapd AP "zephyr-wpa2"
# Also gives zap a subnet + DHCP (dnsmasq) and NATs it out the host's uplink,
# so an associated Zephyr gets a real lease and real upstream.
# Run: sudo ./scripts/bring-up-dev-iface.sh
set -eu

STA=zwifi
AP=zap
SSID=zephyr-wpa2
PSK=password
CONF=/tmp/ap.conf
PID=/tmp/hostapd-zap.pid

# Same SSID/passphrase as zephyr's tests/net/wifi/interop, so known-good creds.
SUBNET=192.168.66
GW=$SUBNET.1
DNSMASQ_PID=/tmp/dnsmasq-zap.pid
# Routing table + rule priority used to steer this subnet past any VPN split
# routes; must sort ahead of main (32766). See section 5.
RT_TABLE=66
RT_PRIO=900

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }

# Uplink = whatever holds the default route (the real card, normally), LAN_GW its
# gateway. Both come from the one route line: "default via <LAN_GW> dev <UPLINK>".
set -- $(ip route show default)
LAN_GW=${3:-}
UPLINK=${5:-}
[ -n "$UPLINK" ] || { echo "FAIL: no default route; no uplink to NAT to"; exit 1; }

# 1. radios (no-op if module already loaded)
lsmod | grep -q '^mac80211_hwsim' || modprobe mac80211_hwsim radios=2

# 2. name the two radios $STA and $AP. sysfs is authoritative here: it lists only
# the hwsim netdevs, so the hwsim0 monitor and the real card are never candidates.
# Each unnamed netdev takes whichever of the two names is still free, so a re-run
# with the radios already named does nothing.
for r in /sys/devices/virtual/mac80211_hwsim/hwsim*/net/*; do
    [ -e "$r" ] || continue
    n=$(basename "$r")
    case " $STA $AP " in *" $n "*) continue;; esac
    for want in "$STA" "$AP"; do
        [ -e "/sys/class/net/$want" ] && continue
        ip link set "$n" down
        ip link set "$n" name "$want"
        break
    done
done
[ -e "/sys/class/net/$STA" ] && [ -e "/sys/class/net/$AP" ] ||
    { echo "FAIL: need 2 hwsim netdevs to name $STA + $AP"; exit 1; }

# 3. keep NetworkManager's supplicant off both (avoids "Match already configured").
# The p2p-dev-* siblings are separate NM devices with their own managed flag:
# leaving them on lets the supplicant run P2P scans on the same phy, which makes
# every scan here (and in Zephyr) fail with EBUSY. NM only creates them a moment
# after the radios appear, and takes seconds to act, so this is re-asserted in
# the verify loop below rather than being a one-shot.
nm_unmanage() {
    command -v nmcli >/dev/null 2>&1 || return 0
    for d in "$STA" "$AP" "p2p-dev-$STA" "p2p-dev-$AP"; do
        nmcli device set "$d" managed no >/dev/null 2>&1 || true
    done
}
nm_unmanage
# By pidfile only. A blanket "pkill hostapd" would also take down a real AP the
# user happens to be running, and this script has no business doing that.
[ -f "$PID" ] && { kill "$(cat "$PID")" 2>/dev/null || true; rm -f "$PID"; }
sleep 1
ip link set "$STA" up
ip link set "$AP" down          # hostapd brings it up

# 4. AP (restart cleanly if a previous one is still running)
cat > "$CONF" <<EOF
interface=$AP
ssid=$SSID
hw_mode=g
channel=6
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_passphrase=$PSK
rsn_pairwise=CCMP
EOF
hostapd -B -P "$PID" "$CONF"

# 5. L3 on the AP side: address, DHCP server, NAT to the uplink.
ip addr flush dev "$AP"
ip addr add "$GW/24" dev "$AP"
ip link set "$AP" up

[ -f "$DNSMASQ_PID" ] && { kill "$(cat "$DNSMASQ_PID")" 2>/dev/null || true; rm -f "$DNSMASQ_PID"; }
dnsmasq --interface="$AP" --bind-interfaces --except-interface=lo \
        --dhcp-range="$SUBNET.10,$SUBNET.100,12h" \
        --dhcp-option=3,"$GW" --dhcp-option=6,8.8.8.8 \
        --pid-file="$DNSMASQ_PID"

sysctl -qw net.ipv4.ip_forward=1

# VPN/ZTNA clients (zcctun0 here) install a full-internet split - 8.0.0.0/5,
# 16.0.0.0/4, 64.0.0.0/2 ... - that outranks the default route, so forwarded
# traffic would egress the tunnel instead of $UPLINK, miss the MASQUERADE below,
# and be dropped by the tunnel as an unknown source. Give this subnet its own
# table that knows only the LAN default. Scoped to $SUBNET.0/24, so nothing else
# on the host changes.
ip route replace default via "$LAN_GW" dev "$UPLINK" table "$RT_TABLE"
# The on-link route must be in this table too. Without it, replies sourced from
# $GW back to the subnet match the rule, find only the default, and leave via
# $UPLINK instead of $AP - which breaks even a ping to $GW.
ip route replace "$SUBNET.0/24" dev "$AP" src "$GW" table "$RT_TABLE"
ip rule show | grep -q "from $SUBNET.0/24 lookup $RT_TABLE" ||
    ip rule add from "$SUBNET.0/24" lookup "$RT_TABLE" priority "$RT_PRIO"

# NAT is the only rule needed: FORWARD's policy is ACCEPT here, so explicit
# ACCEPTs for this subnet would be no-ops. Head of the chain (-I 1) so docker's
# jump rules can't shadow it; -C first so repeat runs don't stack duplicates.
iptables -t nat -C POSTROUTING -s "$SUBNET.0/24" -o "$UPLINK" -j MASQUERADE 2>/dev/null ||
    iptables -t nat -I POSTROUTING 1 -s "$SUBNET.0/24" -o "$UPLINK" -j MASQUERADE

# 6. verify from the STA side. Advisory only: EBUSY here usually means NM's
# p2p-dev-* sibling holds a scan on the same phy, which Zephyr's own scan
# tolerates - so this must not block the setup. Install
# /etc/NetworkManager/conf.d/99-hwsim-unmanaged.conf to remove the race for good.
for i in 1 2 3; do
    nm_unmanage
    sleep 2
    # "scan dump" reads the cached BSS table: same proof, and it works even if a
    # fresh scan is refused because someone else has one in flight (EBUSY).
    if { iw dev "$STA" scan || iw dev "$STA" scan dump; } 2>/dev/null | grep -q "SSID: $SSID"; then
        echo "OK: $STA sees $SSID on $AP ($GW/24, NAT -> $UPLINK)"
        exit 0
    fi
done
echo "WARN: could not confirm $SSID from $STA (scan busy?); setup is up anyway"
echo "      AP $SSID on $AP, $GW/24, NAT -> $UPLINK"
