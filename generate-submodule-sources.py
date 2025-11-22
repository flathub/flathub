#!/usr/bin/env python

import os
import sys
import subprocess
import yaml
from pathlib import Path

def parse_gitmodules(root, root_commit, relroot = None, out = None, target_hash_lookup={}):
    if not out:
        out = []
    
    try:
        f = open(os.path.join(root, gitmodules_filename), "r")
    except FileNotFoundError:
        return out

    module = None
    while True:
        l = f.readline()
        if len(l) == 0:
            break
        line = l.strip()
        if line.startswith("[submodule "):
            module = {}
        elif line.startswith("path = "):
            dest = line[7:]
            nextroot = os.path.join(root, dest)
            result = subprocess.run(["git", "-C", nextroot, "rev-parse", "HEAD"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            # nonzero exit code probably means the directory doesn't exist at all
            if result.returncode == 0:
                commit = target_hash_lookup.get(dest)
                # if the directory exists, but rev-parse returned the same hash as it did on the level above,
                # that means the directory isn't a git repo in itself, so we should ignore it
                if commit:
                    if relroot:
                        module["dest"] = os.path.join(relroot, dest)
                    else:
                        module["dest"] = dest
                    module["commit"] = commit
                    out.append(module)
                    parse_gitmodules(nextroot, commit, dest, out)
        elif line.startswith("url = "):
            module["url"] = line[6:]
        elif line.startswith("gclient-condition"):
            module["gclient-condition"] = line[20:] 

    f.close()
    return out


def update_gitmodules(previous_modules, fresh_modules, target_hash_lookup):
    previous_by_url = {item['url']:item for item in previous_modules}
    fresh_by_url = {item['url']:item for item in fresh_modules}
    all_urls = set(previous_by_url.keys()).union(set(fresh_by_url.keys()))
    all_urls = sorted(list(all_urls))

    module_list = []

    for url in all_urls:
        fresh_module = fresh_by_url.get(url)
        previous_module = previous_by_url.get(url)

        if previous_module is None and fresh_module is None:
            continue
        elif previous_module is not None and fresh_module is not None:
            #reuse values that already are present 
            combined_module = dict(previous_module) 
            # set the commit based on the lookup table that was pulled from the git commits
            combined_module["commit"] = target_hash_lookup[fresh_module["dest"]]
            module_list.append(combined_module)
        else: # one or the other (XOR) is none
            module_list.append(fresh_module or previous_module)

    return module_list

def get_existing_modules():
    sm_file = Path(submodule_yaml_filename)
    if not sm_file.exists():
        return None
    with open(submodule_yaml_filename, 'r') as file:
        return yaml.safe_load(file)

def parse_module_target_hashes(root):
    # https://stackoverflow.com/questions/20655073/how-to-see-which-commit-a-git-submodule-points-at?rq=3
    result = subprocess.run(["git", "-C", root, "ls-tree", "-r", "HEAD"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    tree = result.stdout.decode("utf-8").strip()
    tree = tree.split("\n")

    target_hashes = {}
    for item in tree:
        info, path = item.split("\t")
        mode, obj_type, obj_hash = info.split(" ")
        if obj_type == "commit":
            target_hashes[path] = obj_hash
    
    return target_hashes

argc = len(sys.argv)
gitmodules_filename = ".gitmodules"
submodule_yaml_filename = "chromium-submodules.yaml"

if __name__ == "__main__" and argc > 0:
    if argc != 2:
        print("Usage: '{} <path>', where path is the root of your chromium build directory containing .gitmodules".format(sys.argv[0]))
        exit(1)

    root = sys.argv[1]
    target_hashes = parse_module_target_hashes(root)
    result = subprocess.run(["git", "-C", root, "rev-parse", "HEAD"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    roothash = result.stdout.decode("utf-8").strip()
    modules = []

    # calculate (but dont store) a new set of modules based on the .gitmodules file regardless of if one exists
    # this is helpful for getting the raw paths from the repo for each submodule
    fresh_modules = parse_gitmodules(root, roothash, target_hash_lookup=target_hashes)

    # filter out any with gclient-condition, which often designates them as internal.
    fresh_modules = list(filter(lambda m: "internal" not in m.get("gclient-condition", "") and "chrome-internal" not in m.get("url"), fresh_modules))
    
    existing_modules = get_existing_modules()

    if existing_modules is not None:
        modules = update_gitmodules(existing_modules, fresh_modules, target_hashes)
    else:
        modules = fresh_modules
    
    f = open(submodule_yaml_filename, "w")
    for module in modules:
        f.writelines([
            "- type: git\n",
            "  url: {}\n".format(module["url"]),
            "  commit: {}\n".format(module["commit"]),
            "  dest: {}\n".format(module["dest"]),
            "  disable-submodules: true\n"
        ])

    f.close()
