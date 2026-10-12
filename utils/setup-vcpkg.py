#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Bootstrap the pinned vcpkg SDK and initialize recursive ns-3 submodules."""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


def setup(source, sdk, submodules=True):
    """Initialize submodules and bootstrap an SDK without modifying existing SDKs."""
    required = ["git", "cmake"]
    if os.name != "nt":
        required.extend(["curl", "zip", "unzip", "tar"])
    missing = [program for program in required if shutil.which(program) is None]
    if missing:
        raise RuntimeError("Install host prerequisites first: " + ", ".join(missing))
    if submodules:
        subprocess.run(
            ["git", "-C", str(source), "submodule", "update", "--init", "--recursive"],
            check=True,
        )
    if not sdk.exists():
        baseline = json.loads((source / "vcpkg.json").read_text())["builtin-baseline"]
        sdk.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                "git",
                "clone",
                "--filter=blob:none",
                "--no-checkout",
                "https://github.com/microsoft/vcpkg.git",
                str(sdk),
            ],
            check=True,
        )
        subprocess.run(["git", "-C", str(sdk), "checkout", baseline], check=True)
    toolchain = sdk / "scripts" / "buildsystems" / "vcpkg.cmake"
    if not toolchain.is_file():
        raise RuntimeError(f"Not a vcpkg SDK: {sdk}")
    executable = sdk / ("vcpkg.exe" if os.name == "nt" else "vcpkg")
    if not executable.is_file():
        bootstrap = sdk / (
            "bootstrap-vcpkg.bat" if os.name == "nt" else "bootstrap-vcpkg.sh"
        )
        subprocess.run([str(bootstrap), "-disableMetrics"], cwd=sdk, check=True)
    return toolchain


def main():
    """Parse setup options and report failures with a nonzero exit status."""
    source = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(os.environ.get("VCPKG_ROOT", source / "vcpkg")),
        help="Existing or new SDK directory (default: VCPKG_ROOT or ./vcpkg)",
    )
    parser.add_argument(
        "--no-submodules",
        action="store_true",
        help="Skip recursive submodule initialization",
    )
    args = parser.parse_args()
    try:
        toolchain = setup(
            source, args.root.expanduser().resolve(), not args.no_submodules
        )
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"vcpkg setup failed: {error}", file=sys.stderr)
        return 1
    print(f"vcpkg toolchain: {toolchain}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
