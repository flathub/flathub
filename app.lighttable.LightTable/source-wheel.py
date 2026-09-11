#!/usr/bin/env python3
"""Build a local sdist offline, optionally using source-built override wheels.

Invoked inside flatpak-builder only. No wheel is downloaded: the wheel directory
contains outputs of earlier source modules and is removed from the final image.
"""
from __future__ import annotations

import argparse
import importlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import tomllib

WHEELS = Path('/app/source-wheels')


def run(*args: str) -> None:
    subprocess.run(args, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bootstrap', action='store_true')
    parser.add_argument('--build-only', action='store_true')
    parser.add_argument('--override', action='append', default=[])
    parser.add_argument('--config', action='append', default=[])
    args = parser.parse_args()
    WHEELS.mkdir(exist_ok=True)
    os.environ.update(PIP_NO_INDEX='1', PIP_DISABLE_PIP_VERSION_CHECK='1',
                      PIP_NO_BUILD_ISOLATION='1', PYTHONNOUSERSITE='1')
    if args.bootstrap:
        data = tomllib.loads(Path('pyproject.toml').read_text())['build-system']
        sys.path[:0] = [str(Path(p).resolve()) for p in data.get('backend-path', [])]
        backend = importlib.import_module(data['build-backend'])
        wheel = WHEELS / backend.build_wheel(str(WHEELS))
        # The installer source tree can install its own newly built wheel.
        if Path('src/installer').is_dir():
            os.environ['PYTHONPATH'] = str(Path('src').resolve())
        run(sys.executable, '-m', 'installer', str(wheel))
        return
    with tempfile.TemporaryDirectory(prefix='lighttable-source-build-') as scratch:
        python = sys.executable
        if args.override:
            run(python, '-m', 'venv', '--without-pip', '--system-site-packages', scratch)
            python = str(Path(scratch) / 'bin/python')
            run(python, '-m', 'pip', 'install', '--no-index', '--no-deps',
                '--ignore-installed', '--find-links', str(WHEELS), *args.override)
        # A per-module output directory avoids selecting a wheel from an earlier
        # build of a different version of the same distribution.
        output = Path(scratch) / 'wheel'
        settings = [part for value in args.config for part in ('--config-settings', value)]
        run(python, '-m', 'pip', 'wheel', '--no-index', '--no-deps',
            '--no-build-isolation', '--no-cache-dir', '--wheel-dir', str(output), *settings, '.')
        wheels = list(output.glob('*.whl'))
        if len(wheels) != 1:
            raise RuntimeError(f'Expected one source-built wheel, found {wheels}')
        wheel = WHEELS / wheels[0].name
        # Flatpak mounts /tmp and /app on different filesystems.
        shutil.copy2(wheels[0], wheel)
        if not args.build_only:
            run(sys.executable, '-m', 'pip', 'install', '--no-index', '--no-deps',
                '--force-reinstall', str(wheel))


if __name__ == '__main__':
    main()
