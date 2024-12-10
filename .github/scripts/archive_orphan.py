import datetime
import os
import time

import github
from github.GithubException import (
    RateLimitExceededException,
    GithubException,
)


def main() -> None:
    token = os.environ["GITHUB_TOKEN"]
    g = github.Github(auth=github.Auth.Token(token))

    org = g.get_organization("flathub")

    #    excludes = {}

    earliest = datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(
        weeks=60
    )

    for repo in org.get_repos():
        if not repo.archived:
            try:
                default_branch = repo.default_branch
                branch = repo.get_branch(default_branch)
                last_commit = repo.get_commit(branch.commit.sha)
                last_commit_time = last_commit.commit.committer.date.astimezone(
                    datetime.timezone.utc
                )
                collaborators = repo.get_collaborators(affiliation="direct")
                colb_count = collaborators.totalCount
            except GithubException:
                last_commit_time = datetime.datetime.now(
                    datetime.timezone.utc
                ) + datetime.timedelta(seconds=10)
                colb_count = 1
                pass
            except RateLimitExceededException:
                print("Rate limited")
                time.sleep(g.rate_limiting_resettime - time.time() + 10)
                continue

            if colb_count == 0 and last_commit_time < earliest:
                print(
                    "Archiving: {} Repo has no collaborators. Last push: {}, earlier than: {}".format(
                        repo.html_url,
                        last_commit_time.isoformat(),
                        earliest.isoformat(),
                    )
                )
                # desc = "This repo is archived by Flathub as it is orphaned. If this was done in error or you wish to maintain it, please open an issue at https://github.com/flathub/flathub/issues"


#                repo.edit(description=desc)
#                repo.edit(archived=True)


if __name__ == "__main__":
    main()
