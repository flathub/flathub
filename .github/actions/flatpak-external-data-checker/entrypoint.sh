#!/bin/bash

if [[ -z "$GITHUB_WORKSPACE" || -z "$GITHUB_REPOSITORY" ]]; then
    echo "Script is not running in GitHub Actions CI"
    exit 1
fi

git config --global user.name "flathubbot" && \
git config --global user.email "sysadmin@flathub.org"

mkdir flathub
cd flathub || exit

gh repo list flathub --visibility public -L 8000 --json url --json isArchived --jq '.[] | select(.isArchived == false)|.url' | parallel "git clone --depth 1 {}"

echo "==> Deleting inactive repos"
base_url="https://raw.githubusercontent.com/flathub-infra/flathub-inactive-repo-list/refs/heads/main/"
for file in inactive.txt manual_inactive.txt; do
  curl -s "${base_url}${file}" | while read folder; do
    test -d "$folder" && echo "==> Deleting $folder" && rm -rf "$folder" || true
  done
done

mapfile -t checker_apps < <( grep -rl -E 'extra-data|x-checker-data|\.AppImage' | cut -d/ -f1 | sort -u | shuf )

for repo in "${checker_apps[@]}"; do
    FEDC_OPTS=()

    if [[ -f $repo/flathub.json ]]; then
        # check if repo opted out
        if ! jq -e '."disable-external-data-checker" | not' < "$repo"/flathub.json > /dev/null; then
            continue
        fi
        # check if the app is EOL
        if ! jq -e '."end-of-life" or ."end-of-life-rebase" | not' < "$repo"/flathub.json > /dev/null; then
            continue
        fi
        # add repo-specified f-e-d-c args
        if jq -e '."require-important-update"' < "$repo"/flathub.json > /dev/null; then
            FEDC_OPTS+=("--require-important-update")
        fi
        # disable sending PRs and only commit
        if jq -e '."fedc-commit-only" == true' < "$repo"/flathub.json > /dev/null; then
            FEDC_OPTS+=("--commit-only")
        fi
    fi

    if [[ -f $repo/${repo}.yml ]]; then
        manifest=${repo}.yml
    elif [[ -f $repo/${repo}.yaml ]]; then
        manifest=${repo}.yaml
    elif [[ -f $repo/${repo}.json ]]; then
        manifest=${repo}.json
    else
        continue
    fi

    echo "==> checking ${repo}"
    /app/flatpak-external-data-checker --verbose --update "${FEDC_OPTS[@]}" "$repo/$manifest" || true
done
