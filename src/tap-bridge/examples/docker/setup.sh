#!/bin/bash
#
# Bring up the whole two-container setup: start the containers with no network
# of their own, create a bridge and TAP device for each, and wire the
# containers to the bridges. ns-3 then joins the two TAP devices.
#
# Usage: sudo bash setup.sh

set -euo pipefail

HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
IMAGE=${NS3_DOCKER_IMAGE:-ns3exp:latest}

# Podman is a drop-in for every command used here. Export it so that the
# scripts called below use the same runtime.
RUNTIME=${CONTAINER_RUNTIME:-docker}
export CONTAINER_RUNTIME=$RUNTIME

if ! command -v "$RUNTIME" >/dev/null 2>&1; then
    echo "error: '$RUNTIME' not found, set CONTAINER_RUNTIME to the one you use" >&2
    exit 1
fi

if ! "$RUNTIME" image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "error: image '$IMAGE' not found, build it first with:" >&2
    echo "    $RUNTIME build -t $IMAGE $HERE" >&2
    exit 1
fi

# Let containers on this host reach the X display. This is narrower than a bare
# "xhost +", which would accept connections from anywhere on the network.
xhost +local: >/dev/null

for NAME in left right; do
    "$RUNTIME" run --detach --interactive --tty \
        --name "$NAME" \
        --network none \
        --cap-add NET_ADMIN \
        --volume /tmp/.X11-unix:/tmp/.X11-unix:ro \
        --env DISPLAY="$DISPLAY" \
        "$IMAGE"
done

bash "$HERE/single_setup.sh" left
bash "$HERE/single_setup.sh" right

bash "$HERE/container.sh" left 0
bash "$HERE/container.sh" right 1
