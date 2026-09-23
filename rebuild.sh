#!/usr/bin/env bash
if ! command -v ansifilter >/dev/null 2>&1; then
  echo "Warning: ansifilter is not found. ANSI escape codes will be written to logs." >&2
  ansifilter() { cat; }
fi

./ns3 clean && NO_COLOR=1 ./ns3 configure --enable-tests --enable-examples 2>&1 | ansifilter > configure.log && NO_COLOR=1 ./ns3 build 2>&1 | ansifilter > build.log
