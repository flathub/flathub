#!/usr/bin/env python3
# Remove Proton-internal registries, proxies and plugins from .yarnrc.yml so
# yarn can install the public packages offline.
import pathlib
import re

path = pathlib.Path(".yarnrc.yml")
text = path.read_text()
for key in ("httpProxy", "httpsProxy", "npmPublishRegistry", "npmMinimalAgeGate"):
    text = re.sub(rf"^{key}:.*\n", "", text, flags=re.M)
for key in ("plugins", "npmScopes"):
    text = re.sub(rf"^{key}:\n([ \t]+.*\n)*", "", text, flags=re.M)
path.write_text(text)
