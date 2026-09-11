#!/bin/sh
# Undo bring-up-real-iface.sh. Run: sudo ./scripts/clean-up-real-iface.sh
set -u
REAL=wlp0s20f3; VIF=zreal
[ "$(id -u)" -eq 0 ] || { echo "run with sudo"; exit 1; }
pgrep -x zephyr.exe >/dev/null && echo "note: zephyr.exe still running"
ip link show "$VIF" >/dev/null 2>&1 && iw dev "$VIF" del
nmcli device set "$REAL" managed yes >/dev/null 2>&1 || true
iw dev | grep Interface || true
