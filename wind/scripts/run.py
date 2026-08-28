import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

WIND = ROOT / "wind"
BUILD = ROOT / "build"

EXECUTABLE = BUILD / "wind"


def main() -> int:
    if not (BUILD / "CMakeCache.txt").exists():
        subprocess.run(
            [
                "cmake",
                "-S",
                str(WIND),
                "-B",
                str(BUILD),
            ],
            check=True,
        )

    # Build Wind.
    subprocess.run(
        [
            "cmake",
            "--build",
            str(BUILD),
            "--parallel",
        ],
        check=True,
    )

    # Run Wind.
    subprocess.run(
        [str(EXECUTABLE), *sys.argv[1:]],
        cwd=BUILD,
        check=True,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
