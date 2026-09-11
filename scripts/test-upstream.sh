#!/bin/sh
# Prove or disprove the *host* side of the hwsim upstream path, with Zephyr out
# of the picture: associate a plain Linux STA with the same AP on the same radio,
# then ping through the NAT.
#
# The radio is moved into a netns so its traffic is genuinely forwarded by the
# host, exactly like Zephyr's. A plain "ping -I zap" would be locally generated
# and would skip the FORWARD chain, so it proves nothing about this path.
#
# Address is static rather than DHCP: DHCP is already known to work (Zephyr gets
# a lease), so leaving it out removes a dependency and a failure mode.
#
# Zephyr cannot use $STA while this runs. The phy is moved back on exit.
# Run: sudo ./scripts/test-upstream.sh [remote-ip]
set -eu

STA=zwifi
SSID=zephyr-wpa2
PSK=password
NS=zsta
SUBNET=192.168.66
GW=$SUBNET.1
ADDR=$SUBNET.50
REMOTE=${1:-8.8.8.8}
WPACONF=/tmp/wpa-$STA.conf

[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
[ -e "/sys/class/net/$STA" ] || { echo "FAIL: no $STA; run bring-up-dev-iface.sh first"; exit 1; }

PHY=$(basename "$(readlink -f "/sys/class/net/$STA/phy80211")")

cleanup() {
    # Hand the radio back to the root netns (pid 1) so bring-up/run still work.
    ip netns exec "$NS" pkill -f "wpa_supplicant.*$STA" 2>/dev/null || true
    ip netns exec "$NS" iw phy "$PHY" set netns 1 2>/dev/null || true
    ip netns del "$NS" 2>/dev/null || true
    rm -f "$WPACONF"
}
trap cleanup EXIT

ip netns del "$NS" 2>/dev/null || true
ip netns add "$NS"
iw phy "$PHY" set netns name "$NS"

wpa_passphrase "$SSID" "$PSK" > "$WPACONF"
ip netns exec "$NS" ip link set "$STA" up
ip netns exec "$NS" wpa_supplicant -B -i "$STA" -c "$WPACONF" >/dev/null

# Association takes a moment; poll rather than guess a sleep.
i=0
while [ "$i" -lt 15 ]; do
    ip netns exec "$NS" iw dev "$STA" link | grep -q "Connected to" && break
    i=$((i + 1))
    sleep 1
done
ip netns exec "$NS" iw dev "$STA" link | grep -q "Connected to" ||
    { echo "FAIL: $STA never associated with $SSID"; exit 1; }
echo "OK: associated with $SSID"

ip netns exec "$NS" ip addr add "$ADDR/24" dev "$STA"
ip netns exec "$NS" ip route add default via "$GW"

# Staged: the gateway proves the link, the remote proves routing + NAT.
if ip netns exec "$NS" ping -c 2 -W 2 "$GW" >/dev/null 2>&1; then
    echo "OK: $ADDR -> $GW (link works)"
else
    echo "FAIL: cannot reach $GW; the link itself is broken, NAT is irrelevant"
    exit 1
fi

echo "--- ping $REMOTE (forwarded + NATed by the host) ---"
if ip netns exec "$NS" ping -c 3 -W 2 "$REMOTE"; then
    echo "OK: host path to $REMOTE works -> if Zephyr still fails, the bug is app-side"
else
    echo "FAIL: host cannot forward/NAT $SUBNET.0/24 to $REMOTE -> host plumbing bug, not Zephyr"
    echo
    echo "=== sysctls (0 for forwarding drops before FORWARD is ever reached) ==="
    for f in ip_forward "conf/zap/forwarding" "conf/all/rp_filter" "conf/zap/rp_filter"; do
        echo "$f = $(cat /proc/sys/net/ipv4/$f 2>/dev/null || echo '?')"
    done
    echo
    echo "=== iptables FORWARD (full, unfiltered) ==="
    iptables -L FORWARD -nv --line-numbers
    echo
    echo "=== iptables nat POSTROUTING (full, unfiltered) ==="
    iptables -t nat -L POSTROUTING -nv --line-numbers
    echo
    # Docker 27+ and firewalld install *native* nft base chains. Those hook
    # forward at their own priority and are invisible to iptables -L, so a drop
    # there looks like the packet vanishing before FORWARD.
    echo "=== nft base chains hooking forward/postrouting ==="
    nft -a list ruleset 2>/dev/null | grep -E 'table |chain |type .* hook (forward|postrouting)|policy drop|drop|counter' |
        grep -v '^\s*$' || echo "(nft unavailable)"
    exit 1
fi
