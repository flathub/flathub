#!/usr/bin/env python3

import sys
import os
from glob import glob
import json
import subprocess


ADDON_SRC_PREFIX = 'src'


def addon_manifest(def_dir, addon):
    with open(os.path.join(def_dir, addon + '.txt'), 'r') as f:
        a, r, v = f.read().strip().split(' ')
        ls_remote = subprocess.check_output(['git', 'ls-remote', '-q', r, v])
        c = ls_remote.decode().split('\t')[0]
        mf = {
            'name': addon,
            'buildsystem': 'cmake-ninja',
            'sources': [{
                'type': 'git',
                'url': r,
                'commit': c,
            }]
        }
        return mf


def create_manifest(repo_path, target_path):
    for p in glob(os.path.join(repo_path, '*', 'platforms.txt')):

        addon_dir = os.path.abspath(os.path.join(p, os.pardir))
        addon_dict = addon_manifest(addon_dir, os.path.basename(addon_dir))
        # print some progress
        print(os.path.basename(addon_dir))
        with open(os.path.join(target_path, "%s.json" % os.path.basename(addon_dir)), 'w') as t:
            json.dump(addon_dict, t, indent=4)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise ValueError('invalid arguments')
    create_manifest(
        repo_path = sys.argv[1],
        target_path = sys.argv[2]
    )
