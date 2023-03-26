#!/usr/bin/python

import subprocess
import tempfile
import yaml

from pathlib import Path

GIT_OPTIONS = ["--depth=1", "--recurse-submodules"]

DOTNET_VERSION = "7"
TOOLS_DIR = Path(__file__).parent

PROJECT_DIR = TOOLS_DIR.parent
assert Path(PROJECT_DIR, "com.spacestation14.Launcher.yaml").is_file()

SOURCES_DIR = Path(PROJECT_DIR, "sources")
SOURCES_DIR.mkdir(parents=True, exist_ok=True)

FLATPAK_DOTNET_GENERATOR = Path(PROJECT_DIR, "flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py")
assert FLATPAK_DOTNET_GENERATOR.is_file()

def main():
    with Path(PROJECT_DIR, "com.spacestation14.Launcher.yaml").open("r") as file:
        manifest_data = yaml.safe_load(file)
        assert manifest_data

        assert manifest_data.get("runtime") == "org.freedesktop.Platform"
        freedesktop_version = manifest_data.get("runtime-version")
        assert freedesktop_version

        modules_list = manifest_data.get("modules", [])

        launcher_module = next(
            (
                module for module in modules_list
                if isinstance(module, dict) and module.get("name") == "space-station-14-launcher"
            ),
            None
        )
        assert launcher_module

        launcher_sources_list = launcher_module.get("sources", [])

        # We will assume the first source entry is the git repo
        launcher_source = next(
            (
                source for source in launcher_sources_list
                if isinstance(source, dict)
            ),
            None
        )
        assert launcher_source

        launcher_source_type = launcher_source.get("type")
        assert launcher_source_type == "git"

        launcher_source_url = launcher_source.get("url")
        assert launcher_source_url

        launcher_source_tag = (
            launcher_source.get("branch")
            or launcher_source.get("tag")
        )
        assert launcher_source_tag

    with tempfile.TemporaryDirectory(prefix=".update-nuget-sources-", dir=PROJECT_DIR) as work_dir:
        checkout_dir = Path(work_dir, "SS14.Launcher")

        try:
            subprocess.run([
                "git",
                "clone",
                launcher_source_url,
                checkout_dir.as_posix(),
                "--branch={}".format(launcher_source_tag),
                *GIT_OPTIONS
            ], check=True)
        except subprocess.CalledProcessError:
            return

        subprocess.run([
            FLATPAK_DOTNET_GENERATOR,
            "--dotnet",
            DOTNET_VERSION,
            "--freedesktop",
            freedesktop_version,
            Path(SOURCES_DIR, "space-station-14-launcher-nuget-sources.json"),
            Path(checkout_dir, "SS14.Launcher/SS14.Launcher.csproj")
        ], check=True)

        subprocess.run([
            FLATPAK_DOTNET_GENERATOR,
            "--dotnet",
            DOTNET_VERSION,
            "--freedesktop",
            freedesktop_version,
            Path(SOURCES_DIR, "space-station-14-loader-nuget-sources.json"),
            Path(checkout_dir, "SS14.Loader/SS14.Loader.csproj")
        ], check=True)

    print("\nUpdated nuget-sources files to {new}".format(
        file=file.name,
        new=launcher_source_tag
    ))

if __name__ == '__main__':
    main()
