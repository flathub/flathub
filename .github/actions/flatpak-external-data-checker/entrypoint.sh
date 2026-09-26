#!/bin/bash

if [[ -z "$GITHUB_WORKSPACE" || -z "$GITHUB_REPOSITORY" ]]; then
    echo "Script is not running in GitHub Actions CI"
    exit 1
fi

git config --global user.name "flathubbot" && \
git config --global user.email "sysadmin@flathub.org"

mkdir flathub
cd flathub || exit

echo "==> Fetching inactive repos"
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

declare -A inactive_repos=()
while IFS= read -r folder || [[ -n "$folder" ]]; do
    inactive_repos["$folder"]=1
done < "$inactive_repos_file"

rm -f -- "$inactive_repos_file"

valid_repo_name() {
    [[ "$1" =~ ^[A-Za-z0-9._-]+$ && "$1" != "." && "$1" != ".." ]]
}

repos=()
if [[ -n "${APP_ID:-}" ]]; then
    if ! valid_repo_name "$APP_ID"; then
        echo "Invalid application ID" >&2
        exit 1
    fi
    if [[ -z "${inactive_repos[$APP_ID]:-}" ]]; then
        active=$(gh api "repos/flathub/$APP_ID" --jq '.archived == false and .private == false') || exit 1
        [[ "$active" == true ]] && repos+=("$APP_ID")
    fi
else
    shard=$(((GITHUB_RUN_NUMBER - 1) % 6))

    # Code search is only used to check extra-data apps on every run. Its
    # index is incomplete, so every repo is also covered by its shard below.
    echo "==> Searching for extra-data apps"
    declare -A extra_data_repos=()
    for extension in json yaml yml; do
        query="\"extra-data\" org:flathub in:file extension:$extension"
        page=1
        while :; do
            sleep 6.1
            if ! result=$(gh api --method GET search/code \
                -f q="$query" -F per_page=100 -F page="$page"); then
                echo "Code search failed: $query (page $page)" >&2
                break
            fi
            if ! jq -e '.incomplete_results == false and .total_count < 1000' \
                <<< "$result" > /dev/null; then
                echo "Incomplete search or 1000-result ceiling reached: $query (page $page)" >&2
            fi
            while IFS= read -r repo; do
                [[ -n "$repo" ]] && extra_data_repos["$repo"]=1
            done < <(jq -r '.items[].repository | select(.private == false) | .name' <<< "$result")
            total=$(jq -r '.total_count' <<< "$result") || break
            ((page * 100 < total)) || break
            ((page++))
        done
    done

    echo "==> Listing repos for shard $shard/6"
    org_repos=$(gh api --paginate "orgs/flathub/repos?type=public&per_page=100" \
        --jq '.[] | select(.archived == false and .private == false) | .name') || exit 1
    while IFS= read -r repo; do
        valid_repo_name "$repo" || continue
        [[ -z "${inactive_repos[$repo]:-}" ]] || continue
        if [[ -z "${extra_data_repos[$repo]:-}" ]]; then
            checksum=$(printf '%s' "$repo" | cksum)
            checksum=${checksum%% *}
            ((checksum % 6 == shard)) || continue
        fi
        repos+=("$repo")
    done <<< "$org_repos"
fi

echo "==> Cloning ${#repos[@]} repos"
if ((${#repos[@]})); then
    printf '%s\n' "${repos[@]}" | \
        parallel -j8 "git clone --quiet --depth 1 https://github.com/flathub/{}"
fi

checker_apps=()
for repo in "${repos[@]}"; do
    if [[ ! -d "$repo" ]]; then
        echo "Failed to clone $repo" >&2
        continue
    fi
    grep -rqE --exclude-dir=.git 'extra-data|x-checker-data|\.AppImage' "$repo" || continue
    checker_apps+=("$repo")
done

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
