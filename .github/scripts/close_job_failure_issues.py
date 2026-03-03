#!/usr/bin/env python3

import os
import sys
import time
from github import Github, Auth, GithubException

GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")

if not GITHUB_TOKEN:
    print("GITHUB_TOKEN not set")
    sys.exit(1)

auth = Auth.Token(GITHUB_TOKEN)
g = Github(auth=auth)

query = (
    f'is:issue is:open author:flathubbot created:>=2026-02-17 '
    '("Stable commit job failed" OR '
    '"Beta commit job failed" OR '
    '"Stable publish job failed" OR '
    '"Beta publish job failed")'
)

issues = g.search_issues(query)

checked = 0
closed = 0

for issue in issues:
    checked += 1
    repo_name = issue.repository.full_name

    print(f"[{repo_name}] Closing #{issue.number}: {issue.title}")

    try:
        issue.edit(state="closed")
        closed += 1
        time.sleep(0.5)
    except GithubException as e:
        print(f"Failed to close #{issue.number}: {e}")
        continue

print(f"Issues checked: {checked}")
print(f"Issues closed: {closed}")
