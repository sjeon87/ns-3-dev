#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Test vcpkg bootstrap and early CMake integration without network access."""

import importlib.util
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

SOURCE = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "setup_vcpkg", SOURCE / "utils/setup-vcpkg.py"
)
BOOTSTRAP = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BOOTSTRAP)


class VcpkgIntegrationTest(unittest.TestCase):
    """Exercise failures, pinning and toolchain propagation to child projects."""

    def test_clone_pins_baseline_and_initializes_recursively(self):
        """A new SDK checks out the manifest baseline after recursive initialization."""
        with tempfile.TemporaryDirectory() as directory:
            sdk = Path(directory) / "sdk"
            calls = []

            def run(command, **kwargs):
                calls.append(command)
                if "clone" in command:
                    toolchain = sdk / "scripts/buildsystems/vcpkg.cmake"
                    toolchain.parent.mkdir(parents=True)
                    toolchain.touch()
                return subprocess.CompletedProcess(command, 0)

            with (
                patch.object(BOOTSTRAP.shutil, "which", return_value="tool"),
                patch.object(BOOTSTRAP.subprocess, "run", side_effect=run),
            ):
                BOOTSTRAP.setup(SOURCE, sdk)
            self.assertIn("--recursive", calls[0])
            baseline = json.loads((SOURCE / "vcpkg.json").read_text())[
                "builtin-baseline"
            ]
            self.assertEqual(calls[2][-2:], ["checkout", baseline])
            self.assertEqual(calls[3][-1], "-disableMetrics")

    def test_existing_sdk_is_preserved(self):
        """Reusing a bootstrapped SDK neither checks it out nor bootstraps again."""
        with tempfile.TemporaryDirectory() as directory:
            sdk = Path(directory)
            toolchain = sdk / "scripts/buildsystems/vcpkg.cmake"
            toolchain.parent.mkdir(parents=True)
            toolchain.touch()
            (sdk / ("vcpkg.exe" if os.name == "nt" else "vcpkg")).touch()
            with (
                patch.object(BOOTSTRAP.shutil, "which", return_value="tool"),
                patch.object(BOOTSTRAP.subprocess, "run") as run,
            ):
                self.assertEqual(BOOTSTRAP.setup(SOURCE, sdk, False), toolchain)
                run.assert_not_called()

    def test_failed_clone_stops_setup(self):
        """A failing subprocess must propagate rather than continuing bootstrap."""
        with (
            tempfile.TemporaryDirectory() as directory,
            patch.object(BOOTSTRAP.shutil, "which", return_value="tool"),
            patch.object(
                BOOTSTRAP.subprocess,
                "run",
                side_effect=subprocess.CalledProcessError(1, "git"),
            ) as run,
        ):
            with self.assertRaises(subprocess.CalledProcessError):
                BOOTSTRAP.setup(SOURCE, Path(directory) / "sdk", False)
            self.assertEqual(run.call_count, 1)

    def test_toolchain_runs_before_project_and_reaches_subdirectories(self):
        """A caller-selected triplet and shared manifest survive nested discovery."""
        with tempfile.TemporaryDirectory() as directory:
            temporary = Path(directory)
            toolchain = temporary / "vcpkg.cmake"
            toolchain.write_text(
                'set(NS3_TEST_TOOLCHAIN_ACTIVE TRUE CACHE INTERNAL "")\n'
            )
            child = temporary / "child"
            child.mkdir()
            child.joinpath("CMakeLists.txt").write_text(
                "if(NOT NS3_TEST_TOOLCHAIN_ACTIVE OR NOT NS3_VCPKG)\n"
                '  message(FATAL_ERROR "Toolchain did not reach child")\n'
                "endif()\n"
                'if(NOT VCPKG_TARGET_TRIPLET STREQUAL "custom-triplet")\n'
                '  message(FATAL_ERROR "Caller triplet was overwritten")\n'
                "endif()\n"
                f'if(NOT VCPKG_MANIFEST_DIR STREQUAL "{SOURCE.as_posix()}")\n'
                '  message(FATAL_ERROR "Shared manifest was not selected")\n'
                "endif()\n"
            )
            temporary.joinpath("CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.25)\n"
                f'include("{SOURCE.as_posix()}/build-support/custom-modules/ns3-vcpkg.cmake")\n'
                "project(vcpkg_test LANGUAGES NONE)\n"
                "add_subdirectory(child)\n"
            )
            result = subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(temporary),
                    "-B",
                    str(temporary / "build"),
                    f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
                    "-DVCPKG_TARGET_TRIPLET=custom-triplet",
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
