#!/usr/bin/env python3
"""Stage source-built engines/resources in the existing portable directory layout."""
from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import shutil


def main() -> None:
    source = Path.cwd()
    project = source / 'application'
    bundle = Path('/app/LightTable')
    spec = importlib.util.spec_from_file_location('linux_package', project / 'scripts/linux/build-release.py')
    package = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(package)
    package.stage_resources(project, source / 'python-engine', source / 'rust-cli', bundle)
    for target, destination in (
        ('resident/release/lighttable-engine', 'Resources/LightTable/engine/lighttable-engine'),
        ('desktop/release/lighttable-desktop-shell', 'bin/lighttable-desktop-shell'),
        ('cli/release/spektrafilm', 'Resources/LightTable/engine/spektrafilm-rs'),
    ):
        shutil.copy2(source / 'target' / target, bundle / destination)
    colors = bundle / 'Resources/LightTable/color-profiles'
    colors.mkdir()
    for path in (source / 'icc').iterdir():
        shutil.copy2(path, colors / path.name)
    (bundle / 'installation-owner.json').write_text(json.dumps({'owner': 'flatpak'}) + '\n')
    manifest = json.loads((source / 'source-provenance.json').read_text())
    (bundle / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    app_id = 'app.lighttable.LightTable'
    for name, relative in (
        ('lighttable-desktop', 'bin/lighttable-desktop'),
        ('lighttable-cli', 'bin/lighttable-cli'),
        (app_id + '.desktop', f'share/applications/{app_id}.desktop'),
        (app_id + '.metainfo.xml', f'share/metainfo/{app_id}.metainfo.xml'),
    ):
        target = Path('/app') / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / name, target)
        if relative.startswith('bin/'):
            target.chmod(0o755)
    for size, relative in (
        (64, 'LightTable.iconset/icon_32x32@2x.png'),
        (128, 'LightTable.iconset/icon_128x128.png'),
        (256, 'LightTable.iconset/icon_256x256.png'),
    ):
        icon = Path(f'/app/share/icons/hicolor/{size}x{size}/apps/{app_id}.png')
        icon.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(project / 'build' / relative, icon)
    licenses = Path(f'/app/share/licenses/{app_id}/lighttable')
    licenses.mkdir(parents=True)
    for name in ('LICENSE', 'THIRD_PARTY_NOTICES.md'):
        shutil.copy2(bundle / name, licenses / name)
    shutil.copytree(bundle / 'Resources/LightTable/licenses', licenses / 'dependencies')
    for name in ('install.sh', 'uninstall.sh', 'desktop-integration.py'):
        (bundle / name).unlink(missing_ok=True)


if __name__ == '__main__':
    main()
