#!/bin/bash
#
# Remove the container, bridge and TAP device created by single_setup.sh.
# Every step is best-effort so that a partial setup can still be cleaned up.
#
# Usage: sudo bash single_destroy.sh <name>

set -uo pipefail

if [ -z "${1:-}" ]; then
    echo "Usage: $0 <name>" >&2
    exit 1
fi

RUNTIME=${CONTAINER_RUNTIME:-docker}
NAME=$1
BRIDGE=br-$NAME
TAP=tap-$NAME

"$RUNTIME" stop "$NAME" || true
"$RUNTIME" rm "$NAME" || true

ip link set dev "$TAP" nomaster || true
ip link set dev "$TAP" down || true
ip tuntap del mode tap "$TAP" || true

ip link set dev "$BRIDGE" down || true
ip link del "$BRIDGE" type bridge || true
