from pathlib import Path
import subprocess
import sys


SCRIPT_DIR = Path(__file__).resolve().parent
ENGINE_DIR = SCRIPT_DIR.parent
PROJECT_DIR = ENGINE_DIR.parent

GENERATOR = SCRIPT_DIR / "gen_profiles_solution.py"

REGISTRY = PROJECT_DIR / "ThirdParty" / "Vulkan" / "registry" / "vk.xml"
PROFILES = ENGINE_DIR / "Profiles"
OUTPUT = ENGINE_DIR / "Generated" / "VulkanProfiles"


def main() -> int:
    OUTPUT.mkdir(parents=True, exist_ok=True)

    paths = {
        "Generator": GENERATOR,
        "Registry": REGISTRY,
        "Profiles": PROFILES,
        "Output": OUTPUT,
    }

    print("Generating Vulkan Profiles...")
    for name, path in paths.items():
        print(f"  {name:<9}: {path}")

    print()

    # Fail early with useful errors instead of letting the generator
    # produce a confusing FileNotFoundError.
    for name, path in paths.items():
        if not path.exists():
            print(f"ERROR: {name} does not exist:")
            print(f"       {path}")
            return 1

    command = [
        sys.executable,
        str(GENERATOR),
        "--registry",
        str(REGISTRY),
        "--input",
        str(PROFILES),
        "--output-library-inc",
        str(OUTPUT),
        "--output-library-src",
        str(OUTPUT),
    ]

    result = subprocess.run(command)

    if result.returncode != 0:
        print(
            f"\nVulkan profile generation failed "
            f"(exit code {result.returncode}).",
            file=sys.stderr,
        )
        return result.returncode

    print("\nVulkan profile generation complete.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())