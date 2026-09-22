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

declare -A candidates=()
if [[ -n "${APP_ID:-}" ]]; then
    if [[ ! "$APP_ID" =~ ^[A-Za-z0-9._-]+$ || "$APP_ID" == "." || "$APP_ID" == ".." ]]; then
        echo "Invalid application ID" >&2
        exit 1
    fi
    candidates["$APP_ID"]=1
else
    shard=$(((GITHUB_RUN_NUMBER - 1) % 6))
    echo "==> Discovering apps for shard $shard/6"
    for marker in extra-data x-checker-data .AppImage; do
        for extension in json yaml yml; do
            query="\"$marker\" org:flathub in:file extension:$extension"
            page=1
            while :; do
                sleep 6.1
                result=$(gh api --method GET search/code \
                    -f q="$query" -F per_page=100 -F page="$page") || exit 1
                if ! jq -e '.incomplete_results == false and .total_count < 1000' \
                    <<< "$result" > /dev/null; then
                    echo "Incomplete search or 1000-result ceiling reached: $query (page $page)" >&2
                    exit 1
                fi
                repos=$(jq -r '.items[].repository | select(.private == false) | .name' \
                    <<< "$result") || exit 1
                while IFS= read -r repo; do
                    [[ -n "$repo" ]] || continue
                    if [[ "$marker" == extra-data ]]; then
                        candidates["$repo"]=1
                    elif [[ -z "${candidates[$repo]:-}" ]]; then
                        candidates["$repo"]=0
                    fi
                done <<< "$repos"
                total=$(jq -r '.total_count' <<< "$result") || exit 1
                ((page * 100 < total)) || break
                ((page++))
            done
        done
    done
fi

checker_apps=()
for repo in "${!candidates[@]}"; do
    [[ -z "${inactive_repos[$repo]:-}" ]] || continue
    if [[ "${candidates[$repo]}" == 0 ]]; then
        checksum=$(printf '%s' "$repo" | cksum)
        checksum=${checksum%% *}
        ((checksum % 6 == shard)) || continue
    fi
    active=$(gh api "repos/flathub/$repo" --jq '.archived == false and .private == false') || exit 1
    [[ "$active" == true ]] || continue
    git clone --depth 1 "https://github.com/flathub/$repo" || exit 1
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
