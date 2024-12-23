import re
import subprocess
from pathlib import Path
import requests

flatpak_cargo_generator_path = Path("./flatpak-cargo-generator.py")

def ensure_flatpak_cargo_generator_exists():
    if flatpak_cargo_generator_path.exists():
        return
    url = "https://raw.githubusercontent.com/flatpak/flatpak-builder-tools/refs/heads/master/cargo/flatpak-cargo-generator.py"
    res = requests.get(url)
    with flatpak_cargo_generator_path.open("w") as f:
        f.write(res.text)


def cleanup_flatpak_cargo_generator():
    flatpak_cargo_generator_path.unlink()


def update_library(library: str, tag: str, out_file: str | Path) -> None:
    url = f"https://raw.githubusercontent.com/{library}/refs/tags/{tag}/Cargo.lock"
    res = requests.get(url)
    cargo_lock_path = Path("./cargo.lock")
    with cargo_lock_path.open("w") as f:
        f.write(res.text)
    subprocess.run(["python",
                    flatpak_cargo_generator_path,
                    cargo_lock_path,
                    "-o",
                    out_file])
    cargo_lock_path.unlink()


def get_tag(yaml_file: str, library: str) -> str:
    return re.search(rf"{library}.git.*\n.*tag: (.*)\n", yaml_file).group(1)


def get_yaml_file():
    yml_files = list(Path(".").glob("*.yml"))
    assert len(yml_files) == 1
    yml_file = yml_files[0]
    with yml_file.open("r") as f:
        return f.read()


def main():
    ensure_flatpak_cargo_generator_exists()
    yaml_file = get_yaml_file()
    for library in ("sxyazi/yazi", "ajeetdsouza/zoxide", "BurntSushi/ripgrep", "sharkdp/fd"):
        tag = get_tag(yaml_file, library)
        target = f"cargo-sources-{library.split('/')[-1]}.json"
        update_library(library, tag, target)
    cleanup_flatpak_cargo_generator()


if __name__ == "__main__":
    main()