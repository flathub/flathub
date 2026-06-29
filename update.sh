#!/usr/bin/env bash
set -euo pipefail

REPO_OWNER="Dygmalab"
REPO_NAME="Bazecor"
MANIFEST_FILE="com.dygma.bazecor.yml"
METAINFO_FILE="share/metainfo/com.dygma.bazecor.metainfo.xml"
SHARED_MODULES_DIR="shared-modules"

# Create a temporary file for the modified XML
tmpfile=$(mktemp)
trap 'rm -f "$tmpfile"' EXIT

# Helper functions to print messages, handle errors, and check for required commands
die() {
    echo "Error: $*" >&2
    exit 1
}

# Helper function to print usage information
usage() {
    cat <<'EOF'
Usage:
  ./update.sh [--version <latest|VERSION-TAG>]

Examples:
  ./update.sh
  ./update.sh --version latest
  ./update.sh --version v1.9.0
EOF
}

# Helper function to check for required commands, and exit with an error if not found
require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

# Helper function to escape special characters in a string for XML/HTML
html_escape() {
    local s="$1"
    s=${s//&/&amp;/g}
    s=${s//</&lt;/g}
    s=${s//>/&gt;/g}
    s=${s//\"/&quot;/g}
    s=${s//\'/&#39;/g}
    printf '%s\n' "$s"
}

# Helper function to extract version from the AppImage filename or fallback to the tag name
extract_version_from_filename() {
    local filename="$1"
    local fallback_tag="$2"
    local version

    if [[ "$filename" =~ ^Bazecor-([0-9]+(\.[0-9]+)+)([-_].*)?\.AppImage$ ]]; then
        version="${BASH_REMATCH[1]}"
    else
        version="${fallback_tag#v}"
    fi

    [ -n "$version" ] || die "Could not extract version from filename '$filename' or tag '$fallback_tag'."
    printf '%s' "$version"
}

# This function is used to update com.dygma.bazecor.yml with the new AppImage URL, SHA256, and file size.
update_manifest_file() {
    local new_url="$1"
    local new_sha256="$2"
    local new_size="$3"

    NEW_URL="$new_url" NEW_SHA256="$new_sha256" NEW_SIZE="$new_size" \
        yq eval -i '
      (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "extra-data") | .url) = strenv(NEW_URL)
      | (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "extra-data") | .sha256) = strenv(NEW_SHA256)
      | (.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "extra-data") | .size) = (strenv(NEW_SIZE) | tonumber)
    ' "$MANIFEST_FILE" || die "Failed to update $MANIFEST_FILE using yq v4."

    yq eval '.modules[] | select(type == "!!map" and .name == "com.dygma.bazecor") | .sources[] | select(type == "!!map" and .type == "extra-data") | .url' "$MANIFEST_FILE" | grep -q "releases/download/" ||
        die "yq update produced unexpected YAML output; refusing to proceed"
}

# This function is used to update the <releases> section in share/metainfo/com.dygma.bazecor.metainfo.xml
# with the new release information. If the release is missing, it will add a new <release> entry for the
# specified version, date, and details URL, and include the release description as HTML-like content. It is
# also responsible for sorting the releases in descending order by version number, so that the latest release
# appears first. Unsure if they need to be sorted, the guideline only says "it should look like this":
#
#   https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines#release
#
# Which is why we sort them in descending order by version number, so that the latest release appears first.
update_metainfo_file() {
    local version="$1"
    local date="$2"
    local details_url="$3"
    local release_body="$4"

    # Fallback if GitHub release body is empty
    if [ -z "$(printf '%s' "$release_body" | tr -d '[:space:]')" ]; then
        release_body="No release notes provided."
    fi

    local description_xml=""
    local in_list=0
    local list_content=""

    flush_list() {
        if [ $in_list -eq 1 ] && [ -n "$list_content" ]; then
            description_xml+="<ul>${list_content}</ul>"
            list_content=""
            in_list=0
        fi
    }

    while IFS= read -r line; do
        local trimmed_line
        trimmed_line=$(printf '%s' "$(html_escape "$line")" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        
        if [ -z "$trimmed_line" ]; then
            flush_list
            # skip this line entirely to avoid "tag-empty p" warnings in appstreamcli validate
            continue
        fi

        if [[ "$trimmed_line" =~ ^[-*][[:space:]]+(.*) ]]; then
            if [ $in_list -eq 0 ]; then
                in_list=1
                list_content=""
            fi
            list_content+="<li>${BASH_REMATCH[1]}</li>"
        else
            flush_list
            description_xml+="<p>${trimmed_line}</p>"
        fi
    done <<<"$release_body"

    flush_list

    if [ -z "$description_xml" ]; then
        description_xml="<p>No release notes provided.</p>"
    fi

    # Create the new release XML block
    local new_release_xml
    new_release_xml=$(printf '    <release version="%s" date="%s">\n      <description>%s</description>\n      <url type="details">%s</url>\n    </release>' \
        "$version" "$date" "$description_xml" "$details_url")

    # Write the new release block to the global tmpfile
    printf '%s\n' "$new_release_xml" > "$tmpfile"

    # Escape the version for sed regex
    local escaped_version
    escaped_version=$(printf '%s' "$version" | sed 's/[.[\*^$()+?{|\\]/\\&/g')
    
    # Step 1: Remove any existing release with the same version
    sed -i -e "/<release version=\"${escaped_version}\"/,/<\/release>/d" "$METAINFO_FILE"

    # Step 2: Insert the new release block
    if grep -q "<release " "$METAINFO_FILE"; then
        # We want to insert the content of tmpfile before the first <release
        # We use sed to read the file and insert it before the first <release
        # Since sed's 'r' inserts after, we use a trick:
        # Replace the first <release with the content of tmpfile followed by <release
        
        # We need to escape the content of tmpfile for sed replacement
        # This is hard because tmpfile contains newlines and special chars.
        
        # Instead, let's use awk to do the insertion. It's more robust.
        awk -v file="$tmpfile" '
        BEGIN { inserted=0 }
        {
            if (!inserted && /<release /) {
                # Insert the content of the file before this line
                while ((getline line < file) > 0) {
                    print line
                }
                close(file)
                inserted=1
            }
            print
        }' "$METAINFO_FILE" > "${METAINFO_FILE}.tmp"
        mv "${METAINFO_FILE}.tmp" "$METAINFO_FILE"
    else
        # Insert before </releases>
        awk -v file="$tmpfile" '
        {
            if (/<\/releases>/) {
                # Insert the content of the file before this line
                while ((getline line < file) > 0) {
                    print line
                }
                close(file)
            }
            print
        }' "$METAINFO_FILE" > "${METAINFO_FILE}.tmp"
        mv "${METAINFO_FILE}.tmp" "$METAINFO_FILE"
    fi

    echo "Updated $METAINFO_FILE with version $version"
}

main() {
    local version_ref="latest"
    local api_url release_json asset_name asset_url asset_size asset_sha256
    local tag_name release_date release_body extracted_version prompt

    while [ "$#" -gt 0 ]; do
        case "$1" in
        --version)
            shift
            [ "$#" -gt 0 ] || die "Missing value after --version"
            version_ref="$1"
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            die "Unknown argument: $1"
            ;;
        esac
        shift
    done

    require_cmd curl
    require_cmd jq
    require_cmd yq
    require_cmd sha256sum
    require_cmd sed
    require_cmd awk
    require_cmd git
    require_cmd mktemp

    yq --version | grep -q 'mikefarah/yq' || die "This script requires Mike Farah yq v4 (https://github.com/mikefarah/yq)."

    [ -f "$MANIFEST_FILE" ] || die "Could not find $MANIFEST_FILE"
    [ -f "$METAINFO_FILE" ] || die "Could not find $METAINFO_FILE"
    [ -d "$SHARED_MODULES_DIR" ] || die "Could not find submodule directory: $SHARED_MODULES_DIR"

    if [ "$version_ref" = "latest" ]; then
        api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/latest"
    else
        api_url="https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/tags/$version_ref"
    fi

    echo "Fetching release metadata from GitHub API: $api_url"
    release_json="$(curl -fsSL "$api_url")" || die "Failed to fetch release data from GitHub API."

    asset_name="$(printf '%s' "$release_json" | jq -r 'first(.assets[] | select(.name | endswith(".AppImage"))) | .name // empty')"
    asset_url="$(printf '%s' "$release_json" | jq -r 'first(.assets[] | select(.name | endswith(".AppImage"))) | .browser_download_url // empty')"
    asset_size="$(printf '%s' "$release_json" | jq -r 'first(.assets[] | select(.name | endswith(".AppImage"))) | .size // empty')"
    asset_sha256="$(printf '%s' "$release_json" | jq -r 'first(.assets[] | select(.name | endswith(".AppImage"))) | .digest // empty')"

    [ -n "$asset_name" ] || die "No .AppImage file found in the selected release."
    [ -n "$asset_url" ] || die "Missing browser_download_url for asset '$asset_name'."
    [ -n "$asset_size" ] || die "Missing asset size for '$asset_name'."

    if [[ "$asset_sha256" == sha256:* ]]; then
        asset_sha256="${asset_sha256#sha256:}"
    else
        echo "GitHub API response does not include a sha256 digest for this asset. Computing it from the download stream instead..."
        asset_sha256="$(curl -fsSL "$asset_url" | sha256sum | awk '{print $1}')"
        [ -n "$asset_sha256" ] || die "Failed to compute sha256 for $asset_url"
    fi

    tag_name="$(printf '%s' "$release_json" | jq -r '.tag_name // empty')"
    [ -n "$tag_name" ] || die "Release tag_name is missing in API response."

    release_date="$(printf '%s' "$release_json" | jq -r '(.published_at // .created_at // "") | split("T")[0]')"
    [ -n "$release_date" ] || die "Release date is missing in API response."

    release_body="$(printf '%s' "$release_json" | jq -r '.body // ""')"

    # Strip <img> tags to comply with AppStream description rules
    release_body=$(printf '%s' "$release_body" | sed -E 's/<img[^>]*>//g')

    # Strip <a href="..."> tags to comply with AppStream description rules
    release_body=$(printf '%s' "$release_body" | sed -E 's/<a href="[^"]*"[^>]*>//g; s#</a>##g')

    extracted_version="$(extract_version_from_filename "$asset_name" "$tag_name")"

    echo
    echo "AppImage asset: $asset_name"
    echo "Version:        $extracted_version"
    echo "Date:           $release_date"
    echo "SHA256:         $asset_sha256"
    echo "Size:           $asset_size"
    echo

    read -r -p "Proceed with updating files and submodule? [Y/n] " prompt
    if [[ "${prompt:-Y}" =~ ^[Nn]$ ]]; then
        echo "Aborted by user."
        exit 0
    fi

    update_manifest_file "$asset_url" "$asset_sha256" "$asset_size"
    update_metainfo_file "$extracted_version" "$release_date" "https://github.com/$REPO_OWNER/$REPO_NAME/releases/tag/$tag_name" "$release_body"

    echo "Updating shared-modules submodule to latest remote commit..."
    git submodule update --init --remote "$SHARED_MODULES_DIR"

    echo "Done. Updated $MANIFEST_FILE, $METAINFO_FILE, and $SHARED_MODULES_DIR."
}

main "$@"
