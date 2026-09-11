#!/usr/bin/env python3
"""Fail before expensive source builds while recorded dependency gaps remain."""
import argparse
import json
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('status', type=Path)
    parser.add_argument('--check-tools', action='store_true')
    args = parser.parse_args()
    status = json.loads(args.status.read_text())
    if args.check_tools:
        missing = [name for name in ('gcc', 'g++', 'gfortran', 'cmake', 'ninja', 'pkg-config', 'cargo', 'nasm')
                   if shutil.which(name) is None]
        if missing:
            parser.exit(2, 'Source SDK is missing: ' + ', '.join(missing) + '\n')
    # Validation remains unfinished until a build has actually run. It cannot
    # itself block that first build; only missing build prerequisites do so.
    blockers = [item for item in status.get('unresolved', []) if item.get('blocks_build', True)]
    if blockers:
        for item in blockers:
            print(item['id'] + ': ' + item['detail'])
        parser.exit(2, 'Source-only candidate blocked before compilation; see source-status.json.\n')
    print('No recorded build dependency blockers; native verification is still required.')


if __name__ == '__main__':
    main()
