#!/usr/bin/env python3

import glob
import json
import os
import re
import subprocess
import sys
import tempfile
import time

import gi
import github
import pygit2
import yaml
from gql import Client, gql
from gql.transport.requests import RequestsHTTPTransport

gi.require_version("Json", "1.0")
from gi.repository import Json  # noqa: E402


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
    appid = None

    for ext in ("yml", "yaml", "json"):
        files.extend(glob.glob(f"{dirname}/*.{ext}"))

    for filename in files:
        print(f"Parsing {filename}")
        if os.path.isfile(filename):
            ext = filename.split(".")[-1]

            with open(filename) as f:
                if ext in ("yml", "yaml"):
                    manifest = yaml.safe_load(f)
                    if "app-id" in manifest:
                        appid = manifest["app-id"]
                    elif "id" in manifest:
                        appid = manifest["id"]
                else:
                    parser = Json.Parser()
                    if parser.load_from_file(filename):
                        root_node = parser.get_root()
                        if root_node.get_node_type() == Json.NodeType.OBJECT:
                            json_object = root_node.get_object()
                            if json_object.has_member("id"):
                                appid = json_object.get_string_member("id")
                            elif json_object.has_member("app-id"):
                                appid = json_object.get_string_member("app-id")

            if not appid:
                continue

            if appid:
                manifest_file = os.path.basename(filename)
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

    github_comment = github_event["comment"]["body"]

    if not github_comment.startswith("/merge"):
        print("The comment does not start with '/merge'")
        sys.exit(0)

    command_pattern = re.compile(r"^/merge(?::([\w.-]+))? head=([a-fA-F0-9]{40})(.*)$")
    matched = command_pattern.search(github_comment)
    if not matched:
        print(
            "The comment is not a valid '/merge' command.\n"
            "Format: '/merge:<optional target repo default branch, default: master> "
            "head=<pr head commit sha 40 chars> "
            "<optional extra collaborators @foo @baz, default: pr author>'"
        )
        sys.exit(1)

    branch_match = matched.group(1) or "master"
    if branch_match in ("master", "beta"):
        target_repo_default_branch = branch_match
    else:
        target_repo_default_branch = f"branch/{branch_match}"

    print(f"Got target branch {target_repo_default_branch} from comment")

    pr_head_sha = str(matched.group(2))
    print(f"Got PR HEAD SHA from comment: {pr_head_sha}")

    rest_comment = matched.group(3)

    # https://docs.github.com/en/enterprise-cloud@latest/admin/managing-iam/iam-configuration-reference/username-considerations-for-external-authentication#about-username-normalization
    # > Usernames for user accounts on GitHub can only contain alphanumeric characters and dashes
    # > If the username is longer than 39 characters (including underscore and short code),
    # > the provisioning attempt will fail with a 400 error.
    additional_colbs = [m[1:] for m in re.findall(r"@[a-zA-Z0-9-]{1,39}", rest_comment)]

    print(f"Got additional collaborators {additional_colbs} from comment")

    gh = github.Github(github_token)
    org = gh.get_organization("flathub")

    admins = org.get_team_by_slug("admins")
    reviewers = org.get_team_by_slug("reviewers")
    comment_author = gh.get_user(github_event["comment"]["user"]["login"])

    flathub = org.get_repo("flathub")

    pr_id = int(github_event["issue"]["number"])
    pr = flathub.get_pull(pr_id)
    pr_branch = pr.head.label.split(":")[1]
    fork_url = pr.head.repo.clone_url
    pr_author = pr.user.login

    if not admins.has_in_members(comment_author) and not reviewers.has_in_members(
        comment_author
    ):
        print(f"{comment_author} is not a reviewer")
        sys.exit(1)

    tmpdir = tempfile.TemporaryDirectory()
    print(f"Cloning {fork_url} (branch: {pr_branch})")
    clone = pygit2.clone_repository(fork_url, tmpdir.name, checkout_branch=pr_branch)
    clone_head_sha = str(clone.head.target)
    print(f"Clone HEAD SHA: {clone_head_sha}")
    clone.submodules.update(init=True)

    assert clone_head_sha == pr_head_sha

    manifest_file, appid = detect_appid(tmpdir.name)
    if manifest_file is None or appid is None:
        print("Failed to detect appid")
        sys.exit(1)

    print(f"Detected {appid} as appid from {manifest_file}")

    try:
        org.get_repo(appid)
        print(
            f"Repository {appid} already exists in the flathub organisation, exiting."
        )
        sys.exit(1)
    except github.GithubException as err:
        if err.status == 404:
            print(
                f"Repository {appid} does not exist in the flathub organisation, continuing."
            )
            pass
        else:
            print(f"Unexpected error while checking for repository {appid}: {err}")
            raise

    print("Creating new repo on Flathub")
    repo = org.create_repo(appid)
    repo_name = repo.name
    time.sleep(5)
    repo.edit(
        homepage=f"https://flathub.org/apps/details/{appid}",
        delete_branch_on_merge=True,
    )

    print("Adding flathub remote")
    clone.remotes.create(
        "flathub", f"https://x-access-token:{github_token}@github.com/flathub/{appid}"
    )

    print("Pushing changes to the new Flathub repo")
    git_push = (
        f"cd {tmpdir.name} && git push flathub {pr_branch}:{target_repo_default_branch}"
    )
    ret = subprocess.run(
        git_push,
        shell=True,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if ret.stdout:
        print(f"Git push stdout:\n{ret.stdout.decode().strip()}")
    if ret.stderr:
        print(f"Git push stderr:\n{ret.stderr.decode().strip()}")

    repo.remove_from_collaborators("flathubbot")

    print("Setting protected branches")
    for branch in ("master", "main", "stable", "branch/*", "beta", "beta/*"):
        set_protected_branch(github_token, appid, branch)

    remote_branch_obj = repo.get_branch(target_repo_default_branch)
    remote_head_sha = str(remote_branch_obj.commit.sha)
    print(f"Remote HEAD SHA: {remote_head_sha}")
    assert pr_head_sha == remote_head_sha

    assert remote_branch_obj.protected is True, (
        f"Branch '{target_repo_default_branch}' is not protected"
    )

    print(f"Adding {pr_author} to collaborators")
    repo.add_to_collaborators(pr_author, permission="push")

    print("Adding 'trusted-maintainers' to collaborators")
    trusted_maintainers = org.get_team_by_slug("trusted-maintainers")
    trusted_maintainers.update_team_repository(repo, "push")

    if repo_name.startswith("org.kde."):
        print("Adding KDE maintainers to collaborators")
        kde_maintainers = org.get_team_by_slug("KDE")
        kde_maintainers.update_team_repository(repo, "push")

    if repo_name.startswith("org.gnome.") and repo_name.count(".") == 2:
        print("Adding GNOME maintainers to collaborators")
        gnome_maintainers = org.get_team_by_slug("GNOME")
        gnome_maintainers.update_team_repository(repo, "push")

    for user in additional_colbs:
        try:
            print(f"Adding mentioned {user} to collaborators")
            repo.add_to_collaborators(user, permission="push")
        except github.GithubException as err:
            print(f"Adding mentioned {user} failed")
            print(err)
            pass

    final_colbs = [user.login for user in repo.get_collaborators(affiliation="outside")]
    print(f"External colloborators added: {final_colbs}")

    close_comment = (
        f"A repository for this submission has been created: {repo.html_url} and it will be published to Flathub in 4-5 hours.",
        "\n",
        f"You will receive an [invite]({repo.html_url}/invitations) to be a collaborator to the above repository. Please make sure to enable 2FA on GitHub and accept the invite within one week.",
        "\n",
        "Please go through the [App maintenance guide](https://docs.flathub.org/docs/for-app-authors/maintenance/) if you have never maintained an app on Flathub before.",
        "\n",
        "If you're the original developer (or an authorized party), please [verify your app](https://docs.flathub.org/docs/for-app-authors/verification) to let users know it's coming from you.",
        "\n",
        "Please follow the [Flathub blog](https://docs.flathub.org/blog) for the latest announcements.",
        "\n",
        "Thanks!",
    )

    print("Closing the pull request")
    pr.create_issue_comment("\n".join(close_comment))
    pr.edit(state="closed")


if __name__ == "__main__":
    main()
