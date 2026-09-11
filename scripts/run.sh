#!/usr/bin/env bash
# Run the already-built native_sim binary against whatever interface the
# overlay names.
#
# Prereqs (not done here - one-time host setup, does not survive reboot):
#   - hwsim baseline: mac80211_hwsim loaded with 2 radios, renamed zwifi (STA) +
#     zap (AP), both up; hostapd broadcasting SSID "zephyr-open" on zap (run it
#     in a plain terminal, NOT a VSCode integrated one, or a VSCode restart kills it)
#   - real-card path: the overlay's host-interface brought up and unmanaged,
#     see scripts/bring-up-real-iface.sh
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

EXE=build/zephyr/zephyr.exe
if [[ ! -x "$EXE" ]]; then
	echo "error: $EXE not found - run scripts/build.sh first" >&2
	exit 1
fi

# nl80211 + AF_PACKET need these caps. A relink wipes them, so re-apply every run.
sudo setcap cap_net_raw,cap_net_admin+ep "$EXE"

# Keep NetworkManager's hands off whatever interface the overlay is driving,
# plus zap (the hwsim AP radio hostapd uses), so it doesn't fight Zephyr for
# control. Either may not exist depending on which setup is active - skip it.
IFACE=$(grep -oP 'host-interface\s*=\s*"\K[^"]+' boards/native_sim.overlay)
for dev in "$IFACE" zap; do
	nmcli device show "$dev" &>/dev/null && sudo nmcli device set "$dev" managed no || true
done

# Run directly (not `west -t run`, which skips the caps above), tee to a log.
"./$EXE" 2>&1 | tee /tmp/zephyr.log
