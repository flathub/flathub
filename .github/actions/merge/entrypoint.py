#!/usr/bin/env python3

import glob
import json
import os
import re
import subprocess
import sys
import tempfile
import time

import github
import pygit2
import yaml
from gql import Client, gql
from gql.transport.requests import RequestsHTTPTransport


def set_protected_branch(token, repo, branch):
    transport = RequestsHTTPTransport(
        url="https://api.github.com/graphql",
        headers={"Authorization": f"Bearer {token}"},
    )
    client = Client(transport=transport, fetch_schema_from_transport=False)

    gql_get_repo_id = gql(
        """
        query get_repo_id($repo: String!) {
            repository(name: $repo, owner: "flathub") {
                id
            }
        }
        """
    )

    gql_add_branch_protection = gql(
        """
        mutation add_branch_protection($repositoryID: ID!, $pattern: String!) {
            createBranchProtectionRule(
                input: {
                    allowsDeletions: false
                    allowsForcePushes: false
                    dismissesStaleReviews: false
                    isAdminEnforced: false
                    pattern: $pattern
                    repositoryId: $repositoryID
                    requiresApprovingReviews: true
                    requiredApprovingReviewCount: 0
                    requiresCodeOwnerReviews: false
                    requiresStatusChecks: true
                    requiresStrictStatusChecks: true
                    restrictsReviewDismissals: false
                    requiredStatusCheckContexts: ["builds/x86_64"]
                }
            ) {
                branchProtectionRule {
                    id
                }
            }
        }
        """
    )

    repo_id = client.execute(gql_get_repo_id, variable_values={"repo": repo})
    repo_id = repo_id["repository"]["id"]

    result = client.execute(
        gql_add_branch_protection,
        variable_values={"repositoryID": repo_id, "pattern": branch},
    )
    return result


def detect_appid(dirname):
    files = []
    ret = (None, None)

    for ext in ("yml", "yaml", "json"):
        files.extend(glob.glob(f"{dirname}/*.{ext}"))

    for filename in files:
        print(f"Parsing {filename}")
        if os.path.isfile(filename):
            ext = filename.split(".")[-1]

            with open(filename) as f:
                if ext in ("yml", "yaml"):
                    manifest = yaml.safe_load(f)
                else:
                    result = subprocess.run(
                        ["flatpak-builder", "--show-manifest", filename],
                        stdout=subprocess.PIPE,
                    )
                    if result.returncode == 0:
                        manifest = json.loads(result.stdout.decode("utf-8"))
                    else:
                        print(
                            "flatpak-builder failed to print manifest, falling back to json Python module"
                        )
                        try:
                            with open(filename) as f:
                                manifest = json.load(f)
                        except json.decoder.JSONDecodeError:
                            ("Manifest is not a valid JSON")
                            break

            if manifest:
                manifest_file = os.path.basename(filename)
                if "app-id" in manifest:
                    appid = manifest["app-id"]
                elif "id" in manifest:
                    appid = manifest["id"]
                else:
                    continue
                if os.path.splitext(manifest_file)[0] != appid:
                    print(f"Skipping {manifest_file}, does not match appid {appid}")
                    continue
                ret = (manifest_file, appid)

    return ret


def main():
    github_token = os.environ.get("GITHUB_TOKEN")
    if not github_token:
        print("GITHUB_TOKEN environment variable is not set")
        sys.exit(1)

    github_event_path = os.environ.get("GITHUB_EVENT_PATH")
    with open(github_event_path) as f:
        github_event = json.load(f)

    if github_event["action"] != "created":
        print("The event is not a comment")
        sys.exit(0)

    if "pull_request" not in github_event["issue"]:
        print("The issue is not a pull request")
        sys.exit(0)

    command_re = re.search("^/merge.*", github_event["comment"]["body"], re.M)
    if not command_re:
        print("The comment doesn't contain '/merge' command")
        sys.exit(0)
    else:
        command = command_re.group()

    gh = github.Github(github_token)
    org = gh.get_organization("flathub")

    admins = org.get_team_by_slug("admins")
    reviewers = org.get_team_by_slug("reviewers")
    comment_author = gh.get_user(github_event["comment"]["user"]["login"])

    if not admins.has_in_members(comment_author) and not reviewers.has_in_members(
        comment_author
    ):
        print(f"{comment_author} is not a reviewer")
        sys.exit(1)

    flathub = org.get_repo("flathub")
    pr_id = int(github_event["issue"]["number"])
    pr = flathub.get_pull(pr_id)
    pr_author = pr.user.login
    branch = pr.head.label.split(":")[1]
    fork_url = pr.head.repo.clone_url

    tmpdir = tempfile.TemporaryDirectory()
    print(f"Cloning {fork_url} (branch: {branch})")
    clone = pygit2.clone_repository(fork_url, tmpdir.name, checkout_branch=branch)
    clone.update_submodules(init=True)

    manifest_file, appid = detect_appid(tmpdir.name)
    if manifest_file is None or appid is None:
        print("Failed to detect appid")
        sys.exit(1)

    print(f"Detected {appid} as appid from {manifest_file}")

    print("Creating new repo on Flathub")
    repo = org.create_repo(appid)
    time.sleep(5)
    repo.edit(homepage=f"https://flathub.org/apps/details/{appid}")

    print("Adding flathub remote")
    clone.remotes.create(
        "flathub", f"https://x-access-token:{github_token}@github.com/flathub/{appid}"
    )

    try:
        remote_branch = command.split()[0].split(":")[1]
        if remote_branch != "beta":
            remote_branch = f"branch/{remote_branch}"
    except IndexError:
        remote_branch = "master"

    print("Pushing changes to the new Flathub repo")
    git_push = f"cd {tmpdir.name} && git push flathub {branch}:{remote_branch}"
    ret = subprocess.run(
        git_push,
        shell=True,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    print(ret.stdout)
    print(ret.stderr)
    repo.remove_from_collaborators("flathubbot")

    print("Setting protected branches")
    for branch in ("master", "main", "stable", "branch/*", "beta", "beta/*"):
        set_protected_branch(github_token, appid, branch)

    print(f"Adding {pr_author} to collaborators")
    repo.add_to_collaborators(pr_author, permission="push")

    print("Add trusted maintainers to collaborators")
    trusted_maintainers = org.get_team_by_slug("trusted-maintainers")
    trusted_maintainers.update_team_repository(repo, "push")

    if repo.name.startswith("org.kde."):
        print("Add KDE maintainers to collaborators")
        kde_maintainers = org.get_team_by_slug("KDE")
        kde_maintainers.update_team_repository(repo, "push")

    collaborators = {user.replace("@", "") for user in command.split()[1:]}
    for user in collaborators:
        print(f"adding {user} to collaborators")
        repo.add_to_collaborators(user, permission="push")

    close_comment = (
        f"A repository for this submission has been created: {repo.html_url}",
        "\n",
        f"You will receive an invitation to be a collaborator which will grant you write access to the repository above. The invite can be also viewed [here]({repo.html_url}/invitations).",
        "\n",
        "If you have never maintained an application before, common questions are answered in [the app maintenance guide](https://github.com/flathub/flathub/wiki/App-Maintenance).",
        "\n",
        "Thanks!",
    )

    print("Closing the pull request")
    pr.create_issue_comment("\n".join(close_comment))
    pr.edit(state="closed")


if __name__ == "__main__":
    main()
