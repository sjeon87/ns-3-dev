#!/usr/bin/env bash
# Copyright (c) 2026 Tom Henderson
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Authored by Claude Code (opus-4.7); reviewed by Tom Henderson
#
# Capture compiler/distro/kernel info from the runner into a per-job JSON file.
# Aggregated later by ci-coverage-report.py to produce a coverage table.
# Sourced from .base-build (gitlab-ci.yml) and .base-test (gitlab-ci-scheduled.yml).

set -u
mkdir -p build

job="${CI_JOB_NAME:-local}"
mode="${MODE:-unknown}"
compiler="${COMPILER:-c++}"
image="${CI_JOB_IMAGE:-host}"

cxx_ver=$("$compiler" --version 2>&1 | head -n1 || echo unknown)
cmake_ver=$(cmake --version 2>&1 | head -n1 || echo unknown)
kernel=$(uname -srvm 2>&1 || echo unknown)

if [[ -r /etc/os-release ]]; then
    distro=$(. /etc/os-release; echo "${PRETTY_NAME:-${NAME:-unknown}}")
elif command -v sw_vers >/dev/null 2>&1; then
    distro="macOS $(sw_vers -productVersion 2>/dev/null)"
else
    distro=unknown
fi

# Minimal JSON escape: backslash then double-quote. Sufficient for the version
# banners and distro strings we capture; they don't contain control chars.
esc() { printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'; }

out="build/ci-env-${job}.json"
cat > "$out" <<EOF
{
  "job":              "$(esc "$job")",
  "mode":             "$(esc "$mode")",
  "image":            "$(esc "$image")",
  "compiler_var":     "$(esc "$compiler")",
  "compiler_version": "$(esc "$cxx_ver")",
  "cmake":            "$(esc "$cmake_ver")",
  "distro":           "$(esc "$distro")",
  "kernel":           "$(esc "$kernel")"
}
EOF
cat "$out"
