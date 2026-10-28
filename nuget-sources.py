#!/usr/bin/env python3
"""Pin the release project's offline NuGet feed for each Flatpak architecture.

Adapted from LightPlayer's packaging/flatpak/nuget-sources.py. Package URLs come
from NuGet's recorded source. GitHub release assets are checksum-pinned from
the evaluated MSBuild items.
"""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import subprocess
import tempfile
import urllib.request
from pathlib import Path

FLAT_CONTAINER = "PackageBaseAddress/3.0.0"
TARGETS = {
    "x86_64": ("linux-x64", "x64"),
    "aarch64": ("linux-arm64", "ARM64"),
}


def restore(
    project: Path,
    packages_dir: Path,
    arch: str,
    dotnet: str,
    freedesktop: str,
    dotnet_args: list[str],
) -> tuple[Path, list[dict[str, str]]]:
    runtime, platform = TARGETS[arch]
    shell_command = (
        f'PATH="$PATH:/usr/lib/sdk/dotnet{dotnet}/bin" '
        f'LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/sdk/dotnet{dotnet}/lib" '
        'exec dotnet "$@"'
    )
    argv = [
        "flatpak",
        "run",
        f"--arch={arch}",
        "--env=DOTNET_CLI_TELEMETRY_OPTOUT=true",
        "--env=DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true",
        "--command=sh",
        f"--runtime=org.freedesktop.Sdk//{freedesktop}",
        "--share=network",
        "--filesystem=host",
        f"org.freedesktop.Sdk.Extension.dotnet{dotnet}//{freedesktop}",
        "-c",
        shell_command,
        "--",
    ]
    property_args = [
        "-p:Configuration=Release",
        "-p:Flatpak=true",
        "-p:IsStoreRelease=true",
        f"-p:Platform={platform}",
        "-p:SelfContained=true",
    ]
    subprocess.run(
        argv + [
            "restore",
            "--packages",
            str(packages_dir),
            str(project),
            "-r",
            runtime,
        ] + property_args + dotnet_args,
        check=True,
    )
    property_args += [
        arg for arg in dotnet_args
        if arg.lower().startswith(("-p:", "/p:", "-property:", "/property:"))
    ]
    property_args += [f"-p:RuntimeIdentifier={runtime}"]
    evaluation = json.loads(subprocess.check_output(
        argv + [
            "msbuild", str(project),
            "-getItem:GitHubNuGetAsset", "-getProperty:GitHubNuGetFeed",
        ] + property_args,
        text=True,
    ))
    return (
        Path(evaluation["Properties"]["GitHubNuGetFeed"]).resolve(),
        evaluation["Items"].get("GitHubNuGetAsset", []),
    )


def flat_container(source: str, cache: dict[str, str]) -> str:
    if source not in cache:
        if not source.startswith(("http://", "https://")):
            raise SystemExit(f"error: cannot build a download URL for source {source}")
        with urllib.request.urlopen(source) as response:
            index = json.load(response)
        addresses = [
            resource["@id"]
            for resource in index["resources"]
            if resource["@type"].startswith(FLAT_CONTAINER)
        ]
        if not addresses:
            raise SystemExit(f"error: source {source} exposes no {FLAT_CONTAINER}")
        cache[source] = addresses[0].rstrip("/")
    return cache[source]


def collect_sources(
    packages_dir: Path,
    destdir: str,
    release_feed: Path,
    release_assets: list[dict[str, str]],
    repo_root: Path,
) -> list[dict[str, str]]:
    cache: dict[str, str] = {}
    sources = []
    release_packages = set()
    for asset in release_assets:
        filename = asset["Identity"]
        sha256 = asset["Sha256"]
        marker = release_feed / f"{filename}.{sha256}.complete"
        release_packages.update(name.lower() for name in marker.read_text().splitlines())
        sources.append(
            {
                "type": "file",
                "url": asset["Url"],
                "sha256": sha256,
                "dest": (release_feed.parent / "downloads").relative_to(repo_root).as_posix(),
                "dest-filename": filename,
            }
        )
    for hash_path in sorted(packages_dir.glob("*/*/*.nupkg.sha512")):
        name = hash_path.parent.parent.name
        version = hash_path.parent.name
        filename = f"{name}.{version}.nupkg"
        metadata = json.loads((hash_path.parent / ".nupkg.metadata").read_text())
        if metadata["source"] == str(release_feed):
            if filename not in release_packages:
                raise SystemExit(f"error: no pinned GitHub release asset supplies {filename}")
            continue
        base = flat_container(metadata["source"], cache)
        sha512 = binascii.hexlify(base64.b64decode(hash_path.read_text())).decode("ascii")
        sources.append(
            {
                "type": "file",
                "url": f"{base}/{name}/{version}/{filename}",
                "sha512": sha512,
                "dest": destdir,
                "dest-filename": filename,
            }
        )
    return sources


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="The output JSON sources file")
    parser.add_argument("project", type=Path, help="The released project file to restore")
    parser.add_argument(
        "--arch",
        nargs="+",
        choices=TARGETS,
        default=list(TARGETS),
        help="Flatpak architectures to restore (default: both)",
    )
    parser.add_argument("--freedesktop", default="26.08")
    parser.add_argument("--dotnet", default="10")
    parser.add_argument("--destdir", default="nuget-sources")
    parser.add_argument(
        "--dotnet-args",
        nargs=argparse.REMAINDER,
        default=[],
        help="Additional dotnet restore arguments (must come last)",
    )
    args = parser.parse_args()
    project = args.project.resolve()
    repo_root = project.parents[2]
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    packages: dict[str, tuple[dict[str, str], list[str]]] = {}

    # The SDK sandbox cannot access the host's /tmp.
    with tempfile.TemporaryDirectory(dir=output.parent) as tmp:
        for arch in dict.fromkeys(args.arch):
            packages_dir = Path(tmp) / arch
            release_feed, release_assets = restore(
                project,
                packages_dir,
                arch,
                args.dotnet,
                args.freedesktop,
                args.dotnet_args,
            )
            sources = collect_sources(
                packages_dir, args.destdir, release_feed, release_assets, repo_root
            )
            if not sources:
                raise SystemExit(f"error: the {arch} restore produced no packages")
            for source in sources:
                filename = source["dest-filename"]
                if filename in packages:
                    pinned, arches = packages[filename]
                    if pinned != source:
                        raise SystemExit(
                            f"error: inconsistent package across architectures: {filename}"
                        )
                    arches.append(arch)
                else:
                    packages[filename] = (source, [arch])

    sources_with_arches = [
        {**source, "only-arches": arches}
        for _, (source, arches) in sorted(packages.items())
    ]
    output.write_text(json.dumps(sources_with_arches, indent=4) + "\n")
    print(f"{output}: pinned {len(sources_with_arches)} packages")


if __name__ == "__main__":
    main()
