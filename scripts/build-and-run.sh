#!/usr/bin/env bash
# Build, then run. Extra args are passed through to `west build`.
set -euo pipefail

HERE="$(dirname "${BASH_SOURCE[0]}")"
"$HERE/build.sh" "$@"
"$HERE/run.sh"
