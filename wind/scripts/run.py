import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WIND = ROOT / "wind"
BUILD = ROOT / "build"
EXECUTABLE = BUILD / "wind"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Configure, build, and run Wind.")

    build_group = parser.add_mutually_exclusive_group()

    build_group.add_argument(
        "--debug",
        action="store_true",
        help="Build Wind in Debug mode.",
    )

    build_group.add_argument(
        "--release",
        action="store_true",
        help="Build Wind in Release mode.",
    )

    parser.add_argument(
        "--validation",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="Enable or disable Vulkan validation layers.",
    )

    parser.add_argument(
        "--logging",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="Enable or disable Wind logging.",
    )

    parser.add_argument(
        "--warnings-as-errors",
        action=argparse.BooleanOptionalAction,
        default=None,
        help="Treat Wind compiler warnings as errors.",
    )

    parser.add_argument(
        "--clean",
        action="store_true",
        help="Delete the build directory before configuring.",
    )

    parser.add_argument(
        "--no-lfs",
        action="store_true",
        help="Do not run git lfs pull before building.",
    )

    parser.add_argument(
        "program_args",
        nargs=argparse.REMAINDER,
        help="Arguments passed to the Wind executable. Put them after '--'.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if shutil.which("cmake") is None:
        print("[ERROR] CMake was not found in PATH.", file=sys.stderr)
        return 1

    if shutil.which("ninja") is None:
        print("[ERROR] Ninja was not found in PATH.", file=sys.stderr)
        return 1

    # if not args.no_lfs:
    #     if shutil.which("git") is None:
    #         print("[ERROR] Git was not found in PATH.", file=sys.stderr)
    #         return 1

    #     print("==> Updating Git LFS assets")

    #     subprocess.run(
    #         ["git", "lfs", "pull"],
    #         cwd=ROOT,
    #         check=True,
    #     )

    if args.clean:
        print("==> Removing build directory")

        subprocess.run(
            ["cmake", "-E", "remove_directory", str(BUILD)],
            check=True,
        )

    # Default to Debug when no configuration is specified.
    build_type = "Debug"

    if args.release:
        build_type = "Release"

    cmake_args = [
        "cmake",
        "-S",
        str(WIND),
        "-B",
        str(BUILD),
        "-G",
        "Ninja",
        f"-DCMAKE_BUILD_TYPE={build_type}",
    ]

    if args.validation is not None:
        cmake_args.append(f"-DWIND_VALIDATION={'ON' if args.validation else 'OFF'}")

    if args.logging is not None:
        cmake_args.append(f"-DWIND_LOG_ENABLE={'ON' if args.logging else 'OFF'}")

    if args.warnings_as_errors is not None:
        cmake_args.append(
            "-DWIND_RENDERER_WARNINGS_AS_ERRORS="
            f"{'ON' if args.warnings_as_errors else 'OFF'}"
        )

    print("==> Configuring Wind")

    subprocess.run(
        cmake_args,
        check=True,
    )

    print("==> Building Wind")

    subprocess.run(
        [
            "cmake",
            "--build",
            str(BUILD),
            "--parallel",
        ],
        check=True,
    )

    print("==> Running Wind")

    program_args = args.program_args

    if program_args and program_args[0] == "--":
        program_args = program_args[1:]

    subprocess.run(
        [str(EXECUTABLE), *program_args],
        cwd=BUILD,
        check=True,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
