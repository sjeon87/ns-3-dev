#!/bin/bash
#
# Attach a running container to the bridge created by single_setup.sh.
#
# A veth pair is created on the host: one end joins the bridge, the other is
# moved into the container's network namespace and renamed to eth0. The address
# is a placeholder on 10.12.0.0/16, derived from the index so that each
# container gets a distinct one; the README replaces it with an address on the
# network under test.
#
# Usage: sudo bash container.sh <name> <index>

set -euo pipefail

if [ -z "${1:-}" ] || [ -z "${2:-}" ]; then
    echo "Usage: $0 <name> <index>" >&2
    exit 1
fi

RUNTIME=${CONTAINER_RUNTIME:-docker}
NAME=$1
INDEX=$2
BRIDGE=br-$NAME
HOST_END=side-ext-$NAME
CONT_END=side-int-$NAME

# Interface names are limited to 15 characters by IFNAMSIZ.
if [ ${#HOST_END} -gt 15 ] || [ ${#CONT_END} -gt 15 ]; then
    echo "error: container name '$NAME' is too long for a veth interface name" >&2
    exit 1
fi

PID=$("$RUNTIME" inspect --format '{{ .State.Pid }}' "$NAME")
if [ -z "$PID" ] || [ "$PID" -eq 0 ]; then
    echo "error: container '$NAME' is not running" >&2
    exit 1
fi

SEGMENT3=$((INDEX / 250))
SEGMENT4=$((INDEX % 250 + 1))

# A random locally administered unicast MAC address.
MAC_ADDR=$(printf '12:34:56:%02x:%02x:%02x' \
    $((RANDOM % 256)) $((RANDOM % 256)) $((RANDOM % 256)))

# Create the veth pair, put one end on the bridge and bring it up.
ip link add "$HOST_END" type veth peer name "$CONT_END"
ip link set dev "$HOST_END" master "$BRIDGE"
ip link set "$HOST_END" up

# Move the other end into the container, rename it to eth0 and address it.
# nsenter enters the container's network namespace directly, so no entry has to
# be created under /var/run/netns and none is left behind afterwards.
ip link set "$CONT_END" netns "$PID"
nsenter --target "$PID" --net ip link set dev "$CONT_END" name eth0
nsenter --target "$PID" --net ip link set eth0 address "$MAC_ADDR"
nsenter --target "$PID" --net ip link set eth0 up
nsenter --target "$PID" --net ip addr add "10.12.$SEGMENT3.$SEGMENT4/16" dev eth0
