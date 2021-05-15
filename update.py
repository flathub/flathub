#!/usr/bin/env python3

# Modified from https://raw.githubusercontent.com/flathub/net.veloren.veloren/master/update.py

import argparse
import json
import logging
import os
import stat
import subprocess
import sys
import shutil
import urllib.request

GENERATOR_SCRIPT_URL = f"https://github.com/flatpak/flatpak-builder-tools/raw/master/node/flatpak-node-generator.py"


def run(cmdline, cwd=None):
    logging.info(f"Running {cmdline}")
    if cwd is None:
        cwd = os.getcwd()
    try:
        process = subprocess.run(
            cmdline, cwd=cwd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as e:
        logging.error(e.stderr.decode())
        raise
    return process.stdout.decode().strip()


def generate_sources(
    app_source,
    clone_dir=None,
    generator_script_url=None,
    generator_script=None,
    generator_args=None,
):
    cache_dir = os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache"))

    if clone_dir is None:
        repo_dir = app_source["url"].replace("://", "_").replace("/", "_")
        clone_dir = os.path.join(cache_dir, "flatpak-updater", repo_dir)
    if not os.path.isdir(os.path.join(clone_dir, ".git")):
        run(["git", "clone", "--recursive", app_source["url"], clone_dir])

    run(["git", "fetch", "origin", app_source["ref"]], cwd=clone_dir)
    run(["git", "checkout", app_source["ref"]], cwd=clone_dir)

    if generator_script == None:
        generator_script = os.path.join(cache_dir, "flatpak-updater", "generator.py")
        urllib.request.urlretrieve(
            generator_script_url or GENERATOR_SCRIPT_URL, generator_script
        )
    else:
        generator_script = os.path.abspath(generator_script)
    os.chmod(generator_script, stat.S_IRWXU)

    if generator_args is None:
        generator_args = []

    generator_cmdline = [generator_script, "-o", "generated-sources.json"]
    generator_cmdline.extend(generator_args)
    run(generator_cmdline, cwd=clone_dir)
    shutil.move(
        os.path.join(clone_dir, "generated-sources.json"), "generated-sources.json"
    )
    generated_sources = None
    with open("generated-sources.json") as generated_sources:
        generated_sources = json.loads(generated_sources.read())
    logging.info(f"Generation completed")

    return generated_sources


def commit_changes(app_source, files, on_new_branch=True):
    repo_dir = os.getcwd()
    title = f'build: update to ref {app_source["ref"]}'
    run(["git", "add", "-v", "--"] + files, cwd=repo_dir)
    if on_new_branch:
        target_branch = f'update-{app_source["ref"]}'
        run(["git", "checkout", "-b", target_branch], cwd=repo_dir)
    else:
        target_branch = run(["git", "branch", "--show-current"], cwd=repo_dir)

    run(["git", "commit", "-m", title], cwd=repo_dir)
    new_commit = run(["git", "rev-parse", "HEAD"], cwd=repo_dir)
    logging.info(f"Committed {new_commit[:7]} on {target_branch}")

    return target_branch, new_commit


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-g", "--generator", required=False)
    parser.add_argument("--generator-script-url", required=False)
    parser.add_argument("-a", "--generator-arg", action="append", required=False)
    parser.add_argument("-d", "--clone-dir", required=False)
    parser.add_argument("-o", "--gen-output", default="generated-sources.json")
    parser.add_argument("-n", "--new-branch", action="store_true")
    parser.add_argument("--ref", default="master")
    parser.add_argument("app_source_json")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    with open(args.app_source_json, "r") as f:
        app_source = json.load(f)

    if args.ref == app_source["ref"]:
        logging.info(f'Ref {app_source["ref"]} is the latest')
        sys.exit(0)

    app_source.update({"ref": args.ref})

    generated_sources = generate_sources(
        app_source,
        clone_dir=args.clone_dir,
        generator_script_url=args.generator_script_url,
        generator_script=args.generator,
        generator_args=args.generator_arg,
    )
    with open(args.app_source_json, "w") as o:
        json.dump(app_source, o, indent=4)
    with open(args.gen_output, "w") as g:
        json.dump(generated_sources, g, indent=4)

    branch, new_commit = commit_changes(
        app_source,
        files=[args.app_source_json, args.gen_output],
        on_new_branch=args.new_branch,
    )
    logging.info(f"Created commit {new_commit[:7]} on branch {branch}")


if __name__ == "__main__":
    main()
