#!/usr/bin/env bash
# Run the already-built native_sim binary against the host hwsim radios.
#
# Prereqs (not done here - one-time host setup, does not survive reboot):
#   - mac80211_hwsim loaded with 2 radios, renamed zwifi (STA) + zap (AP), both up
#   - hostapd broadcasting SSID "zephyr-open" on zap (run it in a plain terminal,
#     NOT a VSCode integrated one, or a VSCode restart kills it)
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

EXE=build/zephyr/zephyr.exe
if [[ ! -x "$EXE" ]]; then
	echo "error: $EXE not found - run scripts/build.sh first" >&2
	exit 1
fi

# nl80211 + AF_PACKET need these caps. A relink wipes them, so re-apply every run.
sudo setcap cap_net_raw,cap_net_admin+ep "$EXE"

# Keep NetworkManager's hands off the hwsim interfaces so it doesn't fight the
# Zephyr driver / hostapd for control of zwifi and zap.
sudo nmcli device set zwifi managed no
sudo nmcli device set zap managed no

# Run directly (not `west -t run`, which skips the caps above), tee to a log.
"./$EXE" 2>&1 | tee /tmp/zephyr.log
