#!/usr/bin/env python3
"""Pin ORT's CPU archives and prepare their offline CMake build.

Regenerate (network needed only here):
  python3 packaging/flatpak/generate-onnx-sources.py

Every dependency archive must match Microsoft's cmake/deps.txt SHA-1 before
its SHA-256 enters onnxruntime-sources.json. The source revision and the
dependency file itself are pinned independently. CMake calls --configure;
that mode and --install-licenses never access the network.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tarfile
import urllib.request


VERSION = "1.30.0"
COMMIT = "f2c39fe2f838cf35ce7da92824f5a5e3ee6e88a7"
DEPS_SHA256 = "880edacbd7954c32b8bff34959e6fc5bbae0dc402f8f99861c1c3f057b976c0d"
SOURCE_URL = f"https://github.com/microsoft/onnxruntime/archive/{COMMIT}.tar.gz"
# cmake/deps.txt name -> FetchContent name (lowercase).
CPU_DEPS = {
    "abseil_cpp": "abseil_cpp",
    "date": "date",
    "eigen": "eigen3",
    "flatbuffers": "flatbuffers",
    "json": "nlohmann_json",
    "microsoft_gsl": "gsl",
    "mp11": "mp11",
    "onnx": "onnx",
    "protobuf": "protobuf",
    "pytorch_cpuinfo": "pytorch_cpuinfo",
    "re2": "re2",
    "safeint": "safeint",
}


def digest(path, algorithm):
    value = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def parse_deps(data):
    if hashlib.sha256(data).hexdigest() != DEPS_SHA256:
        raise ValueError("cmake/deps.txt differs from the pinned ORT source")
    deps = {}
    for line in data.decode("utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        name, url, sha1 = line.split(";")
        if name in deps or len(sha1) != 40:
            raise ValueError(f"invalid dependency pin: {line}")
        deps[name] = (url, sha1)
    missing = CPU_DEPS.keys() - deps.keys()
    if missing:
        raise ValueError(f"missing CPU dependency pins: {sorted(missing)}")
    return deps


def archive_name(name):
    return name + (".tar.gz" if name == "protobuf" else ".zip")


def download(url, destination):
    if destination.is_file():
        return
    temporary = destination.with_name(destination.name + ".part")
    request = urllib.request.Request(url, headers={"User-Agent": "CalibRaw-Flatpak-source-generator"})
    try:
        with urllib.request.urlopen(request, timeout=120) as response, temporary.open("wb") as output:
            shutil.copyfileobj(response, output)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def generate(cache_dir, output):
    cache_dir.mkdir(parents=True, exist_ok=True)
    source_archive = cache_dir / f"onnxruntime-{VERSION}.tar.gz"
    download(SOURCE_URL, source_archive)
    # Read only these two small members; do not extract upstream's test data.
    with tarfile.open(source_archive, "r:gz") as archive:
        root = f"onnxruntime-{COMMIT}"
        version_member = archive.extractfile(f"{root}/VERSION_NUMBER")
        deps_member = archive.extractfile(f"{root}/cmake/deps.txt")
        if version_member is None or deps_member is None:
            raise ValueError("source archive is missing its version or dependency file")
        if version_member.read().decode("utf-8").strip() != VERSION:
            raise ValueError("source archive has the wrong ONNX Runtime version")
        deps = parse_deps(deps_member.read())

    def pin(name):
        url, expected_sha1 = deps[name]
        path = cache_dir / archive_name(name)
        download(url, path)
        actual_sha1 = digest(path, "sha1")
        if actual_sha1 != expected_sha1:
            raise ValueError(f"{name}: expected SHA-1 {expected_sha1}, found {actual_sha1}")
        print(f"verified {name}: {actual_sha1}", file=sys.stderr)
        return {
            "type": "file",
            "url": url,
            "sha256": digest(path, "sha256"),
            "dest": "flatpak-source-archives",
            "dest-filename": archive_name(name),
        }

    sources = [{
        "type": "archive",
        "url": SOURCE_URL,
        "sha256": digest(source_archive, "sha256"),
        "dest-filename": f"onnxruntime-{VERSION}.tar.gz",
    }]
    with ThreadPoolExecutor(max_workers=4) as executor:
        sources.extend(executor.map(pin, CPU_DEPS))
    # No partial manifest is written if any download or pin check fails.
    output.write_text(json.dumps(sources, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {output} with {len(CPU_DEPS)} verified CPU archives", file=sys.stderr)


def configure(root):
    root = root.resolve()
    deps_file = root / "cmake/deps.txt"
    original = deps_file.with_name("deps.txt.flatpak-original")
    data = original.read_bytes() if original.exists() else deps_file.read_bytes()
    deps = parse_deps(data)
    paths = {}
    # Validate the entire selected set before modifying cmake/deps.txt.
    for name in CPU_DEPS:
        path = root / "flatpak-source-archives" / archive_name(name)
        if not path.is_file():
            raise ValueError(f"missing offline dependency archive: {path}")
        actual = digest(path, "sha1")
        if actual != deps[name][1]:
            raise ValueError(f"{name}: SHA-1 mismatch (found {actual})")
        paths[name] = path.as_posix()
    lines = []
    for line in data.decode("utf-8").splitlines():
        if line and not line.startswith("#"):
            name, _url, sha1 = line.split(";")
            # Optional dependencies deliberately have no network fallback.
            url = paths.get(name, (root / "flatpak-disabled-deps" / archive_name(name)).as_posix())
            line = f"{name};{url};{sha1}"
        lines.append(line)
    if not original.exists():
        original.write_bytes(data)
    deps_file.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"prepared {len(paths)} offline CPU dependencies")


def install_licenses(root, destination):
    root = root.resolve()
    packages = [("onnxruntime", root)]
    packages.extend((name, root / "flatpak-deps" / f"{content}-src")
                    for name, content in CPU_DEPS.items())
    packages.append(("utf8_range", root / "flatpak-deps/protobuf-src/third_party/utf8_range"))
    for name, source in packages:
        licenses = sorted(path for path in source.iterdir()
                          if path.is_file() and path.name.upper().startswith(
                              ("LICENSE", "COPYING", "NOTICE", "COPYRIGHT", "THIRDPARTYNOTICES")))
        if name == "mp11":
            # This standalone Boost repository points to the license on
            # boost.org rather than including a copy in its source archive.
            license_path = root / "flatpak-source-archives/mp11-LICENSE_1_0.txt"
            if digest(license_path, "sha256") != "c9bff75738922193e67fa726fa225535870d2aa1059f91452c411736284ad566":
                raise ValueError("mp11 Boost license differs from its pinned source")
            licenses.append(license_path)
        if not licenses:
            raise ValueError(f"no license files found for {name} in {source}")
        package_destination = destination if name == "onnxruntime" else destination / name
        package_destination.mkdir(parents=True, exist_ok=True)
        for path in licenses:
            target = package_destination / path.name
            shutil.copyfile(path, target)
            target.chmod(0o644)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--configure", metavar="SOURCE_ROOT", type=Path)
    modes.add_argument("--install-licenses", nargs=2, metavar=("SOURCE_ROOT", "DESTINATION"), type=Path)
    parser.add_argument("--cache-dir", type=Path, default=Path("/tmp/calibraw-onnx-sources"))
    parser.add_argument("--output", type=Path, default=Path(__file__).with_name("onnxruntime-sources.json"))
    args = parser.parse_args()
    try:
        if args.configure:
            configure(args.configure)
        elif args.install_licenses:
            install_licenses(*args.install_licenses)
        else:
            generate(args.cache_dir, args.output)
    except (OSError, ValueError, tarfile.TarError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
