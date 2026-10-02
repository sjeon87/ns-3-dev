#!/usr/bin/env bash
# Load (init + update) all git submodules, recursively.
# Covers nested submodules (e.g. contrib/satellite/data).
# Usage: ./post-clone.sh [--depth 1] [extra git submodule update args]
set -euo pipefail

cd "$(dirname "$0")"

git submodule sync --recursive
git submodule update --init --recursive --jobs 8 "$@"

# Add canonical upstream remote to each submodule, if missing.
# Idempotent: keeps existing `upstream` untouched (warns on URL mismatch).
add_upstream() {
    local path="$1"
    local url="$2"
    if [ ! -e "$path/.git" ]; then
        echo "skip $path (not present)"
        return 0
    fi
    local cur
    if cur=$(git -C "$path" remote get-url upstream 2>/dev/null); then
        if [ "$cur" != "$url" ]; then
            echo "keep $path upstream (already $cur, want $url)"
        else
            echo "exists $path upstream $cur"
        fi
    else
        git -C "$path" remote add upstream "$url"
        echo "added $path upstream $url"
    fi
}

add_upstream "contrib/nr" "https://gitlab.com/cttc-lena/nr"
add_upstream "contrib/ns3-ai" "https://github.com/Muhammaduazir69/ns3-ai"
add_upstream "contrib/ntn-cho" "https://github.com/Muhammaduazir69/ntn-cho-framework"
add_upstream "contrib/ntn-constellation" "https://github.com/Muhammaduazir69/ntn-constellation"
add_upstream "contrib/ntn-observability" "https://github.com/Muhammaduazir69/ntn-observability"
add_upstream "contrib/ntn-rrc" "https://github.com/Muhammaduazir69/ntn-rrc"
add_upstream "contrib/ntn-sagin" "https://github.com/Muhammaduazir69/ntn-sagin"
add_upstream "contrib/ntn-slice" "https://github.com/Muhammaduazir69/ntn-slice"
add_upstream "contrib/ntn-v2x" "https://github.com/Muhammaduazir69/ntn-v2x"
add_upstream "contrib/oran-ntn" "https://github.com/Muhammaduazir69/oran-ntn"
add_upstream "contrib/satellite" "https://github.com/sns3/sns3-satellite"
add_upstream "contrib/traffic" "https://github.com/sns3/traffic.git"
add_upstream "external/ns3-mmwave" "https://github.com/nyuwireless-unipd/ns3-mmwave"
add_upstream "src/quic" "https://github.com/signetlabdei/quic"
# No separate upstream: origin is canonical.
# - contrib/magister-stats (origin https://github.com/sns3/stats.git)
# - contrib/satellite/data (origin https://github.com/sns3/sns3-data.git)

echo "All submodules loaded:"
git submodule status --recursive
