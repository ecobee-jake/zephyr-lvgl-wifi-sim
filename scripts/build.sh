#!/usr/bin/env bash
# Build the lvgl-demo for native_sim (32-bit).
#
# The native_sim target is built with -m32, so the SDL2 / libnl-3 / libnl-genl-3
# deps must be resolved via the i386 pkg-config path, not the host default.
set -euo pipefail

export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

cd "$(dirname "${BASH_SOURCE[0]}")/.."
west build "$@"
