import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(
            "usage: shaders.py <input> <output>",
            file=sys.stderr,
        )
        return 1

    source = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()

    if not source.is_file():
        print(
            f"[ERROR] Shader source does not exist: {source}",
            file=sys.stderr,
        )
        return 1

    output.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    print(f"[SHADER] {source.name} -> {output}")

    subprocess.run(
        [
            "glslc",
            str(source),
            "-o",
            str(output),
        ],
        check=True,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
