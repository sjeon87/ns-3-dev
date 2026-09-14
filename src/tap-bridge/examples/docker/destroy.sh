#!/bin/bash
#
# Tear down everything created by setup.sh.
#
# Usage: sudo bash destroy.sh

set -uo pipefail

HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

export CONTAINER_RUNTIME=${CONTAINER_RUNTIME:-docker}

bash "$HERE/single_destroy.sh" left
bash "$HERE/single_destroy.sh" right

xhost -local: >/dev/null 2>&1 || true
