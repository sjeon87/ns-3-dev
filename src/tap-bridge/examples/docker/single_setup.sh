#!/bin/bash
#
# Create the host-side plumbing for one container: a Linux bridge and a TAP
# device that ns-3 will attach to.
#
# Usage: sudo bash single_setup.sh <name>

set -euo pipefail

if [ -z "${1:-}" ]; then
    echo "Usage: $0 <name>" >&2
    exit 1
fi

NAME=$1
BRIDGE=br-$NAME
TAP=tap-$NAME

ip link add name "$BRIDGE" type bridge
ip tuntap add mode tap "$TAP"
ip addr add 0.0.0.0/24 dev "$TAP"
ip link set "$TAP" promisc on up
ip link set dev "$TAP" master "$BRIDGE"
ip link set dev "$BRIDGE" up

# Stop the bridge from handing frames to iptables, which would otherwise filter
# the emulated traffic. The knobs only exist once br_netfilter is loaded.
if modprobe br_netfilter 2>/dev/null && [ -d /proc/sys/net/bridge ]; then
    for f in /proc/sys/net/bridge/bridge-nf-*; do
        [ -e "$f" ] || continue
        echo 0 >"$f"
    done
else
    echo "warning: br_netfilter is not available, skipping bridge-nf-* tuning" >&2
fi
