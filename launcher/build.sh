#!/bin/sh
# Builds 9mm_nx.nro (devkitA64 container). Run ../build.sh first.
HERE="$(cd "$(dirname "$0")" && pwd)"
LAUNCHER_DIR="$HERE" PAYLOAD=9mm_nx exec "$HERE/../runtime/launcher/build.sh" "$@"
