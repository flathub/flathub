#!/usr/bin/env python3

# originates from https://github.com/flathub/net.veloren.veloren/pull/4

import argparse
import json
import logging
import os
import subprocess
import sys
import urllib.request

GENERATOR_SCRIPT_URL = "https://github.com/flatpak/flatpak-builder-tools/raw/master/cargo/flatpak-cargo-generator.py"


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


def get_latest_commit(url, branch):
    ref = f"refs/heads/{branch}"
    commit, got_ref = run(["git", "ls-remote", url, ref]).split()
    assert got_ref == ref
    return commit


def get_latest_tag(url):
    commit, tag = (
        run(["git", "ls-remote", "--tags", "--sort=-v:refname", url])
        .partition("\n")[0]
        .split()
    )
    tag = tag.removeprefix("refs/tags/").removesuffix("^{}")
    return tag, commit


def update_app_source(app_source, target):
    if target == "commit":
        latest_commit = get_latest_commit(
            app_source["url"], app_source.get("branch", "master")
        )
        if "commit" in app_source.keys() and latest_commit == app_source["commit"]:
            logging.info(f'Commit {app_source["commit"][:7]} is the latest')
            sys.exit(0)
        app_source.update({"commit": latest_commit})
        if "tag" in app_source:
            del app_source["tag"]
        if "x-checker-data" in app_source:
            del app_source["x-checker-data"]
    elif target == "tag":
        latest_tag = get_latest_tag(app_source["url"])
        if latest_tag[1] == app_source["commit"]:
            logging.info(
                f'Tag {app_source["tag"]}, commit {app_source["commit"][:7]} is the latest'
            )
            sys.exit(0)
        app_source.update({"tag": latest_tag[0], "commit": latest_tag[1]})
    return app_source


def generate_sources(
    app_source, clone_dir=None, generator_script=None, generator_args=None
):
    cache_dir = os.environ.get("XDG_CACHE_HOME", os.path.expanduser("~/.cache"))

    assert "commit" in app_source
    if clone_dir is None:
        repo_dir = app_source["url"].replace("://", "_").replace("/", "_")
        clone_dir = os.path.join(cache_dir, "flatpak-cargo-updater", repo_dir)
    if not os.path.isdir(os.path.join(clone_dir, ".git")):
        run(["git", "clone", "--recursive", app_source["url"], clone_dir])

    cur_commit = run(["git", "rev-parse", "HEAD"], cwd=clone_dir)
    if cur_commit[:7] != app_source["commit"][:7]:
        run(["git", "fetch", "origin", app_source["commit"]], cwd=clone_dir)
        run(["git", "checkout", app_source["commit"]], cwd=clone_dir)

    if generator_script is None:
        generator_script = os.path.join(
            cache_dir, "flatpak-cargo-updater", "generator.py"
        )
        logging.info(f"Fetching {GENERATOR_SCRIPT_URL}")
        urllib.request.urlretrieve(GENERATOR_SCRIPT_URL, generator_script)
        os.chmod(generator_script, 0o775)

    if generator_args is None:
        generator_args = []

    generator_cmdline = (
        [generator_script, "-o", "/dev/stdout"]
        + generator_args
        + [os.path.join(clone_dir, "Cargo.lock")]
    )
    generated_sources = json.loads(run(generator_cmdline))
    logging.info("Generation completed")

    return generated_sources


def commit_changes(app_source, files, on_new_branch=True):
    repo_dir = os.getcwd()
    if "tag" in app_source:
        title = f'build: update to version {app_source["tag"]}'
    else:
        title = f'build: update to commit {app_source["commit"][:7]}'
    run(["git", "add", "-v", "--"] + files, cwd=repo_dir)
    if on_new_branch:
        target_branch = f'update-{app_source["commit"][:7]}'
        run(["git", "checkout", "-b", target_branch], cwd=repo_dir)
    else:
        target_branch = run(["git", "branch", "--show-current"], cwd=repo_dir)

    run(["git", "commit", "-m", title], cwd=repo_dir)
    new_commit = run(["git", "rev-parse", "HEAD"], cwd=repo_dir)

    return target_branch, new_commit


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-g", "--generator", required=False)
    parser.add_argument("-a", "--generator-arg", action="append", required=False)
    parser.add_argument("-d", "--clone-dir", required=False)
    parser.add_argument("-o", "--gen-output", default="generated-sources.json")
    parser.add_argument("-n", "--new-branch", action="store_true")
    parser.add_argument("-t", "--target", default="commit")
    parser.add_argument("-k", "--keep-version", action="store_true")
    parser.add_argument("app_source_json")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)

    with open(args.app_source_json, "r") as f:
        app_source = json.load(f)
    if not args.keep_version:
        app_source = update_app_source(app_source, args.target)
    generated_sources = generate_sources(
        app_source,
        clone_dir=args.clone_dir,
        generator_script=args.generator,
        generator_args=args.generator_arg,
    )
    with open(args.app_source_json, "w") as o:
        json.dump(app_source, o, indent=4)
    with open(args.gen_output, "w") as g:
        json.dump(generated_sources, g, indent=4)
    git_status = run(["git", "status", "-s", args.app_source_json, args.gen_output])
    logging.info(f"Current git status: {git_status if git_status else 'clean'}")
    if git_status:
        branch, new_commit = commit_changes(
            app_source,
            files=[args.app_source_json, args.gen_output],
            on_new_branch=args.new_branch,
        )
        logging.info(f"Created commit {new_commit[:7]} on branch {branch}")
    else:
        logging.info("Nothing to commit")


if __name__ == "__main__":
    main()
