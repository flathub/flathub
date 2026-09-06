#!/usr/bin/env python3

import os
import json
import copy
import subprocess

# We *have* to use 24.08, since Kiri pinned SnekStudio to python 3.12.
# SnekStudio also requires wheels, so we use artifact-policy universal
# However, several packages do not have universal wheels
# To fix, we use --prefer-wheels.
# There is no way currently to "force" snekstudio to use the system python.
builder_tools = subprocess.run(
    [
        "./flatpak-builder-tools/pip/.venv/bin/python",
        "./flatpak-builder-tools/pip/flatpak-pip-generator.py",
        "--runtime=org.freedesktop.Sdk//24.08",
        "--requirements-file=SnekStudio/Mods/MediaPipe/_tracker/Project/requirements.txt",
        "--artifact-policy package=universal",
        "--prefer-wheels=jaxlib,mediapipe,numpy,kiwisolver,matplotlib,ml_dtypes,opencv-contrib-python,pillow,scipy,psutil,cffi,contourpy",
    ],
    env=os.environ,
)

try:
    builder_tools.check_returncode()
except Exception:
    print(builder_tools.stderr)
    exit()

sources = []

with open("python3-requirements.json", "r") as file:
    python3_reqs = json.load(file)
    for module in python3_reqs["modules"]:
        for source in module["sources"]:
            if (only_arches := source.get("only-arches")) is not None:
                if "x86_64" in only_arches:
                    source["dest"] = "addons/KiriPythonRPCWrapper/Wheels/Linux-x86_64"
                else:
                    source["dest"] = "addons/KiriPythonRPCWrapper/Wheels/Linux-arm64"
            else:
                aarch64_source = copy.deepcopy(source)
                source["only-arches"] = ["x86_64"]
                source["dest"] = "addons/KiriPythonRPCWrapper/Wheels/Linux-x86_64"
                aarch64_source["only-arches"] = ["aarch64"]
                aarch64_source["dest"] = (
                    "addons/KiriPythonRPCWrapper/Wheels/Linux-arm64"
                )
                sources.append(aarch64_source)
            sources.append(source)
    file.close()

with open("python3-requirements.json", "w") as file:
    json.dump(sources, file, indent=4)
