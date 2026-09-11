#!/bin/sh
# Switch which Zephyr this workspace builds against.
#
#   base - upstream v4.4.2, LVGL only, no Wi-Fi        (west-base.yml)
#   wifi - jukkar's fork with the native_sim Wi-Fi driver (west.yml)
#
# Exclusive: only one is checked out at a time, so switching costs a west update.
# Run with no argument to print the current mode.
# Run: ./scripts/switch-zephyr.sh [base|wifi]
set -eu

cd "$(dirname "$0")/.."

case "${1:-}" in
    base) MANIFEST=west-base.yml ;;
    wifi) MANIFEST=west.yml ;;
    "")
        # manifest.file lives in .west/config, which is untracked - nothing in the
        # repo records the mode, so this is the only way to ask.
        current=$(west config manifest.file 2>/dev/null || echo west.yml)
        case "$current" in
            west-base.yml) echo "base (upstream v4.4.2, no Wi-Fi)" ;;
            *)             echo "wifi (fork with native_sim Wi-Fi driver)" ;;
        esac
        exit 0
        ;;
    *) echo "usage: $0 [base|wifi]   (no argument prints current mode)"; exit 1 ;;
esac

west config manifest.file "$MANIFEST"
west update

# The build dir caches the path to the old Zephyr tree and a .config generated
# against it. Keeping it after swapping the whole tree only produces confusing
# failures, so it goes.
rm -rf build

echo
echo "now on: $("$0")"
