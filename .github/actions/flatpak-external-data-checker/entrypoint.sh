#!/bin/bash

if [[ -z "$GITHUB_WORKSPACE" || -z "$GITHUB_REPOSITORY" ]]; then
    echo "Script is not running in GitHub Actions CI"
    exit 1
fi

report_dir="$GITHUB_WORKSPACE/checker-health"
mkdir -p "$report_dir" || exit 1
inventory_dir="$GITHUB_WORKSPACE/checker-selection"
mkdir -p "$inventory_dir/repositories" || exit 1

write_inventory() {
    local path=$1
    shift
    jq "$@" > "$path.tmp" && mv -- "$path.tmp" "$path" || exit 1
}

inventory="$inventory_dir/inventory.json"
write_inventory "$inventory" -n 'env | (.APP_ID // "") as $target |
    {schema_version: 1, selection_rule_version: 1,
      run_id: .GITHUB_RUN_ID, run_attempt: .GITHUB_RUN_ATTEMPT,
      workflow_ref: .GITHUB_WORKFLOW_REF, workflow_sha: .GITHUB_WORKFLOW_SHA,
      selection_repository: .GITHUB_REPOSITORY, selection_sha: .GITHUB_SHA,
      scope: (if $target == "" then "public_organization" else "target_request" end),
      target: (if $target == "" then null else $target end),
      shard_index: (if $target == "" then (.SHARD | tonumber) else 0 end),
      shard_count: (if $target == "" then 6 else 1 end),
      listing_complete: false, processing_complete: false, exit_code: null}'
finish_inventory() {
    local status=$?
    trap - EXIT
    if ! (write_inventory "$inventory" --argjson status "$status" \
        '.exit_code = $status' "$inventory"); then
        echo "Failed to write selection inventory" >&2
        ((status != 0)) || status=1
    fi
    exit "$status"
}
trap finish_inventory EXIT
trap 'exit 143' TERM
trap 'exit 130' INT

decision() {
    write_inventory "$inventory_dir/repositories/repo-$repo.json" "$@" \
        "$inventory_dir/repositories/repo-$repo.json"
}

git config --global user.name "flathubbot" && \
git config --global user.email "sysadmin@flathub.org"

mkdir flathub || exit 1
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

cp -- "$inactive_repos_file" "$inventory_dir/inactive-repos.txt" || exit 1
rm -f -- "$inactive_repos_file"

valid_repo_name() {
    [[ "$1" =~ ^[A-Za-z0-9._-]+$ && "$1" != "." && "$1" != ".." ]]
}

repos=()
# Compare listing hashes before combining shards. Each shard fetches its own list.
listing_started=$(date -u +%FT%TZ)
listing_file=$(mktemp) || exit 1
listing_source=github_api
if [[ -n "${APP_ID:-}" ]]; then
    if ! valid_repo_name "$APP_ID"; then
        echo "Invalid application ID" >&2
        exit 1
    fi
    if [[ -n "${inactive_repos[$APP_ID]:-}" ]]; then
        # Inactive targets need no API lookup. Their archived status stays unknown.
        listing_source=inactive_target_input
        jq -n --arg name "$APP_ID" '[{name: $name, archived: null}]' > "$listing_file" || exit 1
    else
        gh api "repos/flathub/$APP_ID" \
            --jq '[select(.private == false) | {name, archived}]' > "$listing_file" || exit 1
    fi
else
    shard=${SHARD:?SHARD is not set}
    [[ "$shard" =~ ^[0-5]$ ]] || { echo "Invalid shard: $shard" >&2; exit 1; }

    echo "==> Listing repos for shard $shard/6"
    (set -o pipefail
        gh api --paginate "orgs/flathub/repos?type=public&per_page=100" \
            --jq '[.[] | select(.private == false) | {name, archived}]' | \
            jq -s 'add // []' > "$listing_file"
    ) || exit 1
fi

# Reject invalid names and duplicates before using the listing.
jq -e --arg source "$listing_source" 'type == "array" and all(.[];
    (.name | type == "string" and test("\\A[A-Za-z0-9._-]+\\z") and . != "." and . != "..")
    and (.archived | type == "boolean" or (. == null and $source == "inactive_target_input")))
    and ((map(.name | ascii_downcase) | unique | length) == length)' "$listing_file" > /dev/null || exit 1
if [[ -n "${APP_ID:-}" ]]; then
    jq -e --arg name "$APP_ID" 'length == 1 and (.[0].name | ascii_downcase) == ($name | ascii_downcase)' \
        "$listing_file" > /dev/null || exit 1
fi
write_inventory "$inventory_dir/listing.json" -S . "$listing_file"
rm -f -- "$listing_file"
listing_hash=$(sha256sum "$inventory_dir/listing.json")
write_inventory "$inventory" --arg hash "${listing_hash%% *}" \
    --arg started "$listing_started" --arg source "$listing_source" \
    --arg finished "$(date -u +%FT%TZ)" \
    '.listing_complete = true | .listing = {file: "listing.json", sha256: $hash,
      started_at: $started, finished_at: $finished, shared_between_shards: false,
      source: $source}' "$inventory"

# The first skip wins: archived, inactive, marker, opt-out, EOL, manifest.
while IFS=$'\t' read -r repo archived; do
    if [[ -z "${APP_ID:-}" ]]; then
        checksum=$(printf '%s' "$repo" | cksum)
        checksum=${checksum%% *}
        ((checksum % 6 == shard)) || continue
    fi
    reason=""
    if [[ "$archived" == true ]]; then
        reason=archived
    elif [[ -n "${inactive_repos[$repo]:-}" ]]; then
        reason=inactive
    else
        repos+=("$repo")
    fi
    write_inventory "$inventory_dir/repositories/repo-$repo.json" -n \
        --arg repo "$repo" --arg reason "$reason" \
        '{repository: $repo,
          selection: (if $reason == "" then "pending" else "excluded" end),
          reason: (if $reason == "" then null else $reason end),
          error: null, branch: null, commit: null, checkout_unknown_reason: "not_cloned",
          invocation: {started: false, exit_code: null, report: "not_expected"}}'
done < <(jq -r '.[] | [.name, .archived] | @tsv' "$inventory_dir/listing.json")

echo "==> Cloning ${#repos[@]} repos"
if ((${#repos[@]})); then
    printf '%s\n' "${repos[@]}" | \
        parallel -j8 "git clone --quiet --depth 1 https://github.com/flathub/{} ./{} && touch ./{}/.git/checker-cloned"
fi

checker_apps=()
for repo in "${repos[@]}"; do
    # Failed clones can leave directories. Check the success marker instead.
    if [[ ! -f "$repo/.git/checker-cloned" ]]; then
        echo "Failed to clone $repo" >&2
        decision '.selection = "error" | .error = "clone_failed" | .checkout_unknown_reason = "clone_failed"'
        continue
    fi
    branch=$(git -C "$repo" symbolic-ref --short HEAD) || branch=""
    commit=$(git -C "$repo" rev-parse HEAD) || commit=""
    decision --arg branch "$branch" --arg commit "$commit" \
        '.branch = (if $branch == "" then null else $branch end) |
         .commit = (if $commit == "" then null else $commit end) |
         .checkout_unknown_reason = (if $branch == "" or $commit == "" then "git_metadata_unavailable" else null end)'
    grep -rqE --exclude-dir=.git 'extra-data|x-checker-data|\.AppImage' -- "$repo"
    case "$?" in
        0) checker_apps+=("$repo") ;;
        1) decision '.selection = "excluded" | .reason = "no_checker_marker"' ;;
        *) decision '.selection = "error" | .error = "marker_scan_failed"' ;;
    esac
done

checked_apps=0
outdated_apps=0
failed_apps=()

for repo in "${checker_apps[@]}"; do
    FEDC_OPTS=()

    if [[ -f $repo/flathub.json ]]; then
        if ! jq -e -s 'length == 1 and (.[0] | type == "object" or . == null)' \
            < "$repo/flathub.json" > /dev/null; then
            decision '.selection = "error" | .error = "invalid_flathub_json"'
            continue
        fi
        # check if repo opted out
        if ! jq -e '."disable-external-data-checker" | not' < "$repo"/flathub.json > /dev/null; then
            decision '.selection = "excluded" | .reason = "checker_disabled"'
            continue
        fi
        # check if the app is EOL
        if ! jq -e '."end-of-life" or ."end-of-life-rebase" | not' < "$repo"/flathub.json > /dev/null; then
            decision '.selection = "excluded" | .reason = "end_of_life"'
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
        decision '.selection = "excluded" | .reason = "manifest_not_found"'
        continue
    fi

    echo "==> checking ${repo}"
    ((checked_apps += 1))
    decision --arg manifest "$manifest" \
        '.selection = "selected" | .reason = "selected" | .manifest = $manifest |
         .invocation.started = true | .invocation.report = "pending"'
    if /app/flatpak-external-data-checker --verbose --update --report-json "$report_dir/$repo.json" "${FEDC_OPTS[@]}" "$repo/$manifest"; then
        status=0
    else
        status=$?
    fi
    # A valid JSON object does not mean the checker succeeded.
    if [[ ! -f "$report_dir/$repo.json" ]]; then
        report_state=missing
    elif jq -e -s 'length == 1 and (.[0] | type == "object")' "$report_dir/$repo.json" > /dev/null 2>&1; then
        report_state=valid_json_object
    else
        report_state=invalid_json_object
    fi
    decision --argjson status "$status" --arg report "$report_state" \
        '.invocation.exit_code = $status | .invocation.report = $report'
    # FEDC uses a bitmask: 1 = errors, 2 = outdated with --check-outdated.
    # Outdated-only is not a failure; unknown codes and signals are failures.
    case "$status" in
        0) ;;
        2) ((outdated_apps += 1)) ;;
        *) failed_apps+=("| $repo | $status |") ;;
    esac
done

print_check_summary() {
    echo "## External data checker summary"
    echo
    echo "Checked: $checked_apps; failed: ${#failed_apps[@]}; outdated-only: $outdated_apps."
    if ((${#failed_apps[@]})); then
        echo
        echo "| Application | Checker exit code |"
        echo "| --- | --- |"
        printf '%s\n' "${failed_apps[@]}"
        echo
        echo "Exit codes alone cannot distinguish handled checker errors from crashes."
    fi
}

print_check_summary
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    if ! print_check_summary >> "$GITHUB_STEP_SUMMARY"; then
        echo "Failed to write GitHub step summary" >&2
        exit 1
    fi
fi

write_inventory "$inventory" '.processing_complete = true' "$inventory"
exit 0
