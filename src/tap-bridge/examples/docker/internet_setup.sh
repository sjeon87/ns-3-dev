#!/bin/bash
#
# Enslave a physical interface to one of the bridges, so that the container on
# that bridge can reach the outside world through it.
#
# The host loses its own connectivity on that interface for the duration;
# internet_reset.sh puts it back.
#
# Usage: sudo bash internet_setup.sh <name> <internet device>

set -euo pipefail

if [ -z "${1:-}" ] || [ -z "${2:-}" ]; then
    echo "Usage: $0 <name> <internet device>" >&2
    exit 1
fi

NAME=$1
INTERNET=$2
BRIDGE=br-$NAME

if ! ip link show "$INTERNET" >/dev/null 2>&1; then
    echo "error: no such interface: $INTERNET" >&2
    exit 1
fi

if ! ip link show "$BRIDGE" >/dev/null 2>&1; then
    echo "error: no such bridge: $BRIDGE, run setup.sh first" >&2
    exit 1
fi

ip link set "$INTERNET" up
ip link set dev "$INTERNET" master "$BRIDGE"
