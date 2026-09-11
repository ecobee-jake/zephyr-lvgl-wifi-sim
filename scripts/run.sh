#!/usr/bin/env bash
# Run the already-built native_sim binary against whatever interface the
# overlay names.
#
# Prereqs (not done here - one-time host setup, does not survive reboot):
#   - hwsim baseline: scripts/bring-up-dev-iface.sh (radios zwifi + zap, hostapd
#     broadcasting "zephyr-wpa2", DHCP + NAT). Run it in a plain terminal, NOT a
#     VSCode integrated one, or a VSCode restart kills the daemons.
#   - real-card path: the overlay's host-interface brought up and unmanaged,
#     see scripts/bring-up-real-iface.sh
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

EXE=build/zephyr/zephyr.exe
if [[ ! -x "$EXE" ]]; then
	echo "error: $EXE not found - run scripts/build.sh first" >&2
	exit 1
fi

# nl80211 + AF_PACKET need these caps. A relink wipes them, so check and only
# re-apply when missing: sudo here waits on fingerprint auth, and that wait looks
# exactly like the simulator hanging with no output.
if ! getcap "$EXE" | grep -q cap_net_admin; then
	echo "granting net caps to $EXE (needs sudo)..."
	sudo setcap cap_net_raw,cap_net_admin+ep "$EXE"
fi

# NetworkManager is deliberately not touched here. Both bring-up scripts already
# unmanage the interface they set up, and doing it again needed another sudo (see
# above) while guessing the interface name from the base overlay - which is wrong
# for the real-card build, since that comes in via EXTRA_DTC_OVERLAY_FILE.

# Run directly (not `west -t run`, which skips the caps above), tee to a log.
# stdbuf: piping to tee switches stdout to 4KB block buffering, so logs would only
# appear once the buffer fills or the process exits.
stdbuf -oL "./$EXE" 2>&1 | tee /tmp/zephyr.log
