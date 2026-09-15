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
inactive_repos_url="https://builds.flathub.org/api/inactive-repos.txt"
inactive_repos_file=$(mktemp) || exit 1

if ! curl --fail --silent --show-error --location \
    --connect-timeout 10 --max-time 60 \
    --output "$inactive_repos_file" "$inactive_repos_url"; then
    rm -f -- "$inactive_repos_file"
    exit 1
fi

while IFS= read -r folder || [[ -n "$folder" ]]; do
    if [[ -z "$folder" || ! "$folder" =~ ^[A-Za-z0-9._-]+$ || "$folder" == "." || "$folder" == ".." ]]; then
        echo "Invalid inactive repository name" >&2
        rm -f -- "$inactive_repos_file"
        exit 1
    fi
done < "$inactive_repos_file"

while IFS= read -r folder; do
    if [[ -d "./$folder" ]]; then
        echo "==> Deleting $folder"
        rm -rf -- "./$folder"
    fi
done < "$inactive_repos_file"

rm -f -- "$inactive_repos_file"

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
