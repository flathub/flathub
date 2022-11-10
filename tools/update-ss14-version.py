#!/usr/bin/python

import argparse
import subprocess
import tempfile
import yaml

from pathlib import Path

GIT_OPTIONS = ["--depth=1", "--recurse-submodules"]

TOOLS_DIR = Path(__file__).parent

PROJECT_DIR = TOOLS_DIR.parent
assert Path(PROJECT_DIR, "com.spacestation14.Launcher.yaml").is_file()

SOURCES_DIR = Path(PROJECT_DIR, "modules/sources")
assert SOURCES_DIR.is_dir()

FLATPAK_DOTNET_GENERATOR = Path(PROJECT_DIR, "flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py")
assert FLATPAK_DOTNET_GENERATOR.is_file()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("branch", type=str, help="SS14.Launcher version")
    options = parser.parse_args()
    
    with Path(SOURCES_DIR, "ss14-launcher-git.yaml").open("r") as file:
        old_launcher_source_data = yaml.safe_load(file)
        assert old_launcher_source_data

        launcher_source_type = old_launcher_source_data.get("type")
        assert launcher_source_type == "git"

        launcher_source_url = old_launcher_source_data.get("url")
        assert launcher_source_url

    with tempfile.TemporaryDirectory(prefix=".update-nuget-sources-", dir=PROJECT_DIR) as work_dir:
        checkout_dir = Path(work_dir, "SS14.Launcher")

        try:
            subprocess.run([
                "git",
                "clone",
                launcher_source_url,
                checkout_dir.as_posix(),
                "--branch={}".format(options.branch),
                *GIT_OPTIONS
            ], check=True)
        except subprocess.CalledProcessError:
            return

        subprocess.run([
            FLATPAK_DOTNET_GENERATOR,
            Path(SOURCES_DIR, "space-station-14-launcher-nuget-sources.json"),
            Path(checkout_dir, "SS14.Launcher/SS14.Launcher.csproj")
        ], check=True)

        subprocess.run([
            FLATPAK_DOTNET_GENERATOR,
            Path(SOURCES_DIR, "space-station-14-loader-nuget-sources.json"),
            Path(checkout_dir, "SS14.Loader/SS14.Loader.csproj")
        ], check=True)

    with Path(SOURCES_DIR, "ss14-launcher-git.yaml").open("w") as file:
        new_launcher_source_data = old_launcher_source_data.copy()
        new_launcher_source_data.update({
            "tag": options.branch
        })
        yaml.safe_dump(new_launcher_source_data, file, sort_keys=False)
        print("\nUpdated nuget-sources files and tag in '{file}' from {old} -> {new}".format(
            file=file.name,
            old=old_launcher_source_data.get("tag"),
            new=new_launcher_source_data.get("tag")
        ))

if __name__ == '__main__':
    main()
