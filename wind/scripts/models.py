import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def get_cooker() -> Path:
    system = platform.system()
    machine = platform.machine().lower()

    if system == "Linux" and machine in {"x86_64", "amd64"}:
        return ROOT / "tools" / "linux-x64" / "wind-cooker"

    if system == "Windows" and machine in {"x86_64", "amd64"}:
        return ROOT / "tools" / "windows-x64" / "wind-cooker.exe"

    raise RuntimeError(f"Unsupported platform: {system} {machine}")


def main() -> int:
    if len(sys.argv) != 2:
        print(
            "usage: models.py <model.glb|model.gltf>",
            file=sys.stderr,
        )
        return 1

    source = Path(sys.argv[1]).resolve()

    if not source.is_file():
        print(
            f"[ERROR] Model does not exist: {source}",
            file=sys.stderr,
        )
        return 1

    if source.suffix.lower() not in {".glb", ".gltf"}:
        print(
            f"[ERROR] Unsupported model format: {source.suffix}",
            file=sys.stderr,
        )
        return 1

    try:
        cooker = get_cooker()
    except RuntimeError as error:
        print(f"[ERROR] {error}", file=sys.stderr)
        return 1

    if not cooker.is_file():
        print(
            f"[ERROR] Wind cooker not found:\n        {cooker}",
            file=sys.stderr,
        )
        return 1

    output_dir = ROOT / "wind" / "assets" / "models"

    output_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    output = output_dir / f"{source.stem}.wind"

    print(f"[MODEL] {source.name} -> {output.name}")

    subprocess.run(
        [
            str(cooker),
            "--file",
            str(source),
            "--output",
            str(output),
        ],
        check=True,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
