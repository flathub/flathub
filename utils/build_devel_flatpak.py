#!/usr/bin/env python3
# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Automatically creates a Flatpak bundle of a build."""

from __future__ import print_function

import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
import yaml

try:
    from shlex import quote
except ImportError:
    from pipes import quote

RUNTIME_REPO = 'https://flathub.org/repo/flathub.flatpakrepo'
APP_ID = 'org.chromium.Chromium'


def rewrite_manifest_and_get_env(manifest_file, new_manifest_file, out_dir):
    # Keep in sync with org.chromium.Chromium.yaml
    PATHS = [
        'chrome',
        'icudtl.dat',
        '*.so',
        '*.pak',
        '*.bin',
        '*.png',
        'locales',
        'MEIPreload',
    ]

    with open(manifest_file) as fp:
        manifest = yaml.safe_load(fp)

    module = manifest['modules'][-1]

    module['sources'] = []

    for pattern in PATHS:
        for path in glob.iglob(os.path.join(out_dir, pattern)):
            module['sources'].append({
                'type': 'file',
                'path': path,
            })

    module['build-commands'] = ['cp -rv --reflink=auto . /app/chromium']

    with open(new_manifest_file, 'w') as fp:
        json.dump(manifest, fp)

    return module['build-options']['env']


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('task',
                        help='What to do',
                        choices=['shell', 'bundle', 'install'])
    parser.add_argument('out_dir', help='Directory to build a Flatpak for')
    parser.add_argument(
        '--clean',
        help='Clean the OSTree repository and builder state first',
        action='store_true')
    args = parser.parse_args()

    script_dir = os.path.dirname(os.path.realpath(__file__))
    manifest_dir = os.path.dirname(script_dir)
    manifest_file = os.path.join(manifest_dir, '%s.yaml' % APP_ID)

    out_dir = args.out_dir
    flatpak_dir = os.path.join(out_dir, 'flatpak')
    edited_manifest = os.path.join(flatpak_dir,
                                   os.path.basename(manifest_file))
    build_dir = os.path.join(flatpak_dir, 'build')
    repo_dir = os.path.join(flatpak_dir, 'repo')

    dirs_to_clean = [build_dir]
    if args.clean:
        dirs_to_clean.append(repo_dir)

    for dir_to_clean in filter(os.path.exists, dirs_to_clean):
        print('Cleaning up: %s' % dir_to_clean)
        shutil.rmtree(dir_to_clean)

    for required_dir in flatpak_dir, build_dir, repo_dir:
        if not os.path.exists(required_dir):
            os.mkdir(required_dir)

    print('Setting up Flatpak environment')
    env = rewrite_manifest_and_get_env(manifest_file, edited_manifest, out_dir)
    for source in 'krb5.conf', 'gtk-settings.ini', 'shared-modules':
        source_path = os.path.join(manifest_dir, source)
        out_link = os.path.join(flatpak_dir, os.path.basename(source))
        try:
            os.unlink(out_link)
        except FileNotFoundError:
            pass
        os.symlink(source_path, out_link)

    print('Running flatpak-builder')

    builder_args = [
        'flatpak-builder',
        '--repo=%s' % repo_dir, build_dir, edited_manifest
    ]
    if args.task == 'shell':
        builder_args.append('--stop-at=chromium')
    elif args.task == 'install':
        builder_args.extend(['--install', '--user'])

    subprocess.check_call(builder_args)

    if args.task == 'shell':
        env_args = ['--env=%s=%s' % (var, value) for var, value in env.items()]
        os.execvp('flatpak-builder', [
            'flatpak-builder', '--run', *env_args, build_dir, edited_manifest,
            'bash'
        ])
    elif args.task == 'bundle':
        print('Creating bundle', APP_ID)
        subprocess.check_call([
            'flatpak', 'build-bundle',
            '--runtime-repo=%s' % RUNTIME_REPO, repo_dir,
            os.path.join(flatpak_dir, '%s.flatpak' % APP_ID), APP_ID
        ])


if __name__ == '__main__':
    sys.exit(main())
