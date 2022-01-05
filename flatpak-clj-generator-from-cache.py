#!/bin/env python3

import hashlib
import json
from os import environ, path, walk


def sha256sum(path):
    hasher = hashlib.sha256()
    with open(path, 'rb') as f:
        for bytes in iter(lambda: f.read(4096), b''):
            hasher.update(bytes)
    return hasher.hexdigest()


def get_remote_url(prefix, fullpath):
    remote_repositories = open(path.dirname(
        fullpath) + '/_remote.repositories').read()
    if 'clojars' in remote_repositories:
        return 'https://clojars.org/repo/{}'.format(fullpath.removeprefix(prefix))
    return 'https://repo1.maven.org/maven2/{}'.format(fullpath.removeprefix(prefix))


mvn_sources = []

prefix = "{}/.m2/repository/".format(environ['HOME'])
for root, dirs, files in walk(prefix):
    for name in files:
        if name.endswith('SNAPSHOT.jar') or name.endswith('SNAPSHOT.pom'):
            continue
        if name.endswith('pom') or name.endswith('jar'):
            url = get_remote_url(prefix, path.join(root, name))
            mvn_sources.append(
                {
                    'type': 'file',
                    'url': url,
                    'sha256': sha256sum(path.join(root, name)),
                    'dest': 'flatpak-openjdk/cache/{}'.format(root.removeprefix(prefix))
                }
            )

print(json.dumps(mvn_sources, indent=2))
