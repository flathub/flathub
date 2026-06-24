#!/usr/bin/env python3
# Copyright 2018 Çağatay Yiğit Şahin
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

from pathlib import Path
from typing import List, Dict
import subprocess
import argparse
import json

def is_git_repository(p):
    is_git_repo = p.is_dir() and (p / ".git").is_dir()
    return is_git_repo

def repo_paths(build_dir: Path) -> List[Path]:
    src_dir = build_dir / 'src'
    repo_paths: List[Path] = []

    domains = src_dir.iterdir()
    for domain in domains:
        domain_users = domain.iterdir()
        for user in domain_users:
            if is_git_repository(user):
                repo_paths.append(user)
            else:
                user_repos = user.iterdir()
                for ur in user_repos:
                    if is_git_repository(ur):
                        repo_paths.append(ur)
    return repo_paths

def repo_source(repo_path: Path) -> Dict[str, str]:
    def current_commit(repo_path: Path) -> str:
        output = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
            cwd=repo_path).decode('ascii').strip()
        return output

    def remote_url(repo_path: Path) -> str:
        output = subprocess.check_output(
            ['git', 'remote', 'get-url', 'origin'],
            cwd=repo_path).decode('ascii').strip()
        return output
    
    repo_path_str = str(repo_path)
    dest_path = repo_path_str[repo_path_str.rfind('src/'):]
    source_object = {'type': 'git', 'url': remote_url(repo_path), 'commit': current_commit(repo_path), 'dest': dest_path}
    return source_object

def sources(build_dir: Path) -> List[Dict[str, str]]:
    return list(map(repo_source, repo_paths(build_dir)))

def main():
    def directory(string: str) -> Path:
        path = Path(string)
        if not path.is_dir():
            msg = 'build-dir should be a directory.'
            raise argparse.ArgumentTypeError(msg)
        return path

    parser = argparse.ArgumentParser(description='For a Go module’s dependencies, output array of sources in flatpak-manifest format.')
    parser.add_argument('build_dir', help='Build directory of the module in .flatpak-builder/build', type=directory)
    parser.add_argument('-o', '--output', dest='output_file', help='The file to write the source list to. Default is <module-name>-sources.json', type=str)
    args = parser.parse_args()
    source_list = sources(args.build_dir)

    output_file = args.output_file
    if output_file is None:
        output_file = args.build_dir.absolute().name + '-sources.json'
        
    with open(output_file, 'w') as out:
        json.dump(source_list, out, indent=2)

if __name__ == '__main__':
    main()
