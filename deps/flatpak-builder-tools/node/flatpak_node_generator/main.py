from pathlib import Path
from typing import Iterator, List, Set

import argparse
import asyncio
import json
import os
import sys

from .cache import Cache, FilesystemBasedCache
from .manifest import ManifestGenerator
from .node_headers import NodeHeaders
from .package import Package
from .progress import GeneratorProgress
from .providers import ProviderFactory
from .providers.npm import NpmLockfileProvider, NpmModuleProvider, NpmProviderFactory
from .providers.special import SpecialSourceProvider
from .providers.yarn import YarnProviderFactory
from .requests import Requests, StubRequests


def _scan_for_lockfiles(base: Path, patterns: List[str]) -> Iterator[Path]:
    for root, _, files in os.walk(base.parent):
        if base.name in files:
            lockfile = Path(root) / base.name
            if not patterns or any(map(lockfile.match, patterns)):
                yield lockfile


async def _async_main() -> None:
    parser = argparse.ArgumentParser(description='Flatpak Node generator')
    parser.add_argument('type', choices=['npm', 'yarn'])
    parser.add_argument(
        'lockfile', help='The lockfile path (package-lock.json or yarn.lock)'
    )
    parser.add_argument(
        '-o',
        '--output',
        help='The output sources file',
        default='generated-sources.json',
    )
    parser.add_argument(
        '-r',
        '--recursive',
        action='store_true',
        help='Recursively process all files under the lockfile directory with '
        'the lockfile basename',
    )
    parser.add_argument(
        '-R',
        '--recursive-pattern',
        action='append',
        help='Given -r, restrict files to those matching the given pattern.',
    )
    parser.add_argument(
        '--registry',
        help='The registry to use (npm only)',
        default='https://registry.npmjs.org',
    )
    parser.add_argument(
        '--no-trim-index',
        action='store_true',
        help="Don't trim npm package metadata (npm only)",
    )
    parser.add_argument(
        '--no-devel',
        action='store_true',
        help="Don't include devel dependencies (npm only)",
    )
    parser.add_argument(
        '--no-requests-cache',
        action='store_true',
        help='Disable the requests cache',
    )
    parser.add_argument(
        '--max-parallel',
        help='Maximium number of packages to process in parallel',
        type=int,
        default=64,
    )
    parser.add_argument(
        '--retries',
        type=int,
        help='Number of retries of failed requests',
        default=Requests.DEFAULT_RETRIES,
    )
    parser.add_argument(
        '-P',
        '--no-autopatch',
        action='store_true',
        help="Don't automatically patch Git sources from package*.json",
    )
    parser.add_argument(
        '-s',
        '--split',
        action='store_true',
        help='Split the sources file to fit onto GitHub.',
    )
    parser.add_argument(
        '--node-chromedriver-from-electron',
        help='Use the ChromeDriver version associated with the given '
        'Electron version for node-chromedriver',
    )
    # Deprecated alternative to --node-chromedriver-from-electron
    parser.add_argument('--electron-chromedriver', help=argparse.SUPPRESS)
    parser.add_argument(
        '--electron-ffmpeg',
        choices=['archive', 'lib'],
        help='Download prebuilt ffmpeg for matching electron version',
    )
    parser.add_argument(
        '--electron-node-headers',
        action='store_true',
        help='Download the electron node headers',
    )
    parser.add_argument(
        '--nwjs-version',
        help='Specify NW.js version (will use latest otherwise)',
    )
    parser.add_argument(
        '--nwjs-node-headers',
        action='store_true',
        help='Download the NW.js node headers',
    )
    parser.add_argument(
        '--nwjs-ffmpeg',
        action='store_true',
        help='Download prebuilt ffmpeg for current NW.js version',
    )
    # Deprecated, because this is now enabled by default.
    parser.add_argument('--xdg-layout', default=True, help=argparse.SUPPRESS)
    parser.add_argument(
        '--no-xdg-layout',
        action='store_false',
        dest='xdg_layout',
        help="Don't use the XDG layout for caches",
    )
    # Internal option, useful for testing.
    parser.add_argument('--stub-requests', action='store_true', help=argparse.SUPPRESS)

    args = parser.parse_args()

    Requests.retries = args.retries

    if args.type == 'yarn' and (args.no_devel or args.no_autopatch):
        sys.exit('--no-devel and --no-autopatch do not apply to Yarn.')

    if args.electron_chromedriver:
        print('WARNING: --electron-chromedriver is deprecated', file=sys.stderr)
        print(
            '  (Use --node-chromedriver-from-electron instead.)',
            file=sys.stderr,
        )

    if args.stub_requests:
        Requests.instance = StubRequests()

    if not args.no_requests_cache:
        Cache.instance = FilesystemBasedCache()

    lockfiles: List[Path]
    if args.recursive or args.recursive_pattern:
        lockfiles = list(
            _scan_for_lockfiles(Path(args.lockfile), args.recursive_pattern)
        )
        if not lockfiles:
            sys.exit('No lockfiles found.')
        print(f'Found {len(lockfiles)} lockfiles.')
    else:
        lockfiles = [Path(args.lockfile)]

    lockfile_root = Path(args.lockfile).parent

    provider_factory: ProviderFactory
    if args.type == 'npm':
        npm_options = NpmProviderFactory.Options(
            NpmLockfileProvider.Options(no_devel=args.no_devel),
            NpmModuleProvider.Options(
                registry=args.registry,
                no_autopatch=args.no_autopatch,
                no_trim_index=args.no_trim_index,
            ),
        )
        provider_factory = NpmProviderFactory(lockfile_root, npm_options)
    elif args.type == 'yarn':
        provider_factory = YarnProviderFactory()
    else:
        assert False, args.type

    print('Reading packages from lockfiles...')
    packages: Set[Package] = set()
    rcfile_node_headers: Set[NodeHeaders] = set()

    for lockfile in lockfiles:
        lockfile_provider = provider_factory.create_lockfile_provider()
        rcfile_providers = provider_factory.create_rcfile_providers()

        packages.update(lockfile_provider.process_lockfile(lockfile))

        for rcfile_provider in rcfile_providers:
            rcfile = lockfile.parent / rcfile_provider.RCFILE_NAME
            if rcfile.is_file():
                nh = rcfile_provider.get_node_headers(rcfile)
                if nh is not None:
                    rcfile_node_headers.add(nh)

    print(f'{len(packages)} packages read.')

    gen = ManifestGenerator()
    with gen:
        options = SpecialSourceProvider.Options(
            node_chromedriver_from_electron=args.node_chromedriver_from_electron
            or args.electron_chromedriver,
            nwjs_version=args.nwjs_version,
            nwjs_node_headers=args.nwjs_node_headers,
            nwjs_ffmpeg=args.nwjs_ffmpeg,
            xdg_layout=args.xdg_layout,
            electron_ffmpeg=args.electron_ffmpeg,
            electron_node_headers=args.electron_node_headers,
        )
        special = SpecialSourceProvider(gen, options)

        with provider_factory.create_module_provider(gen, special) as module_provider:
            with GeneratorProgress(
                packages,
                module_provider,
                args.max_parallel,
            ) as progress:
                await progress.run()
        for headers in rcfile_node_headers:
            print(f'Generating headers {headers.runtime} @ {headers.target}')
            await special.generate_node_headers(headers)

        if args.xdg_layout:
            script_name = 'setup_sdk_node_headers.sh'
            node_gyp_dir = gen.data_root / 'cache' / 'node-gyp'
            gen.add_script_source(
                [
                    'version=$(node --version | sed "s/^v//")',
                    'nodedir=$(dirname "$(dirname "$(which node)")")',
                    f'mkdir -p "{node_gyp_dir}/$version"',
                    f'ln -s "$nodedir/include" "{node_gyp_dir}/$version/include"',
                    f'echo 9 > "{node_gyp_dir}/$version/installVersion"',
                ],
                destination=gen.data_root / script_name,
            )
            gen.add_command(f'bash {gen.data_root / script_name}')

    if args.split:
        i = 0
        for i, part in enumerate(gen.split_sources()):
            output = Path(args.output)
            output = output.with_suffix(f'.{i}{output.suffix}')
            with open(output, 'w') as fp:
                json.dump(part, fp, indent=ManifestGenerator.JSON_INDENT)

        print(f'Wrote {gen.source_count} to {i + 1} file(s).')
    else:
        with open(args.output, 'w') as fp:
            json.dump(
                list(gen.ordered_sources()),
                fp,
                indent=ManifestGenerator.JSON_INDENT,
            )

            if fp.tell() >= ManifestGenerator.MAX_GITHUB_SIZE:
                print(
                    'WARNING: generated-sources.json is too large for GitHub.',
                    file=sys.stderr,
                )
                print('  (Pass -s to enable splitting.)')

        print(f'Wrote {gen.source_count} source(s).')


def main() -> None:
    asyncio.run(_async_main())
