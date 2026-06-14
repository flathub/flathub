#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST="$ROOT/com.interactivebrokers.ibkrdesktop.yml"

CHANNEL="${CHANNEL:-latest-standalone}"
BASE_URL="https://download2.interactivebrokers.com/installers/ntws/$CHANNEL"
VERSION_URL="$BASE_URL/version.json"
CHANNEL_NAME="${CHANNEL%-standalone}"
INSTALLER_URL="$BASE_URL/ntws-$CHANNEL_NAME-standalone-linux-x64.sh"
JRE_URL="https://download2.interactivebrokers.com/installers/jres/linux-amd64-17.0.10.0.101-zulu-nojavafx.tar.gz"

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

need() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "missing command: $1" >&2
        exit 1
    }
}

need curl
need sha256sum
need sed
need awk
need find
need date
need python3

bytes() {
    wc -c < "$1" | tr -d ' '
}

sum256() {
    sha256sum "$1" | awk '{print $1}'
}

download() {
    local url="$1"
    local out="$2"
    curl -fsSL -o "$out" "$url"
}

download_with_headers() {
    local url="$1"
    local out="$2"
    local headers="$3"
    curl -fsSL -D "$headers" -o "$out" "$url"
}

headers_mtime_ms() {
    local headers="$1"
    local last_modified
    local epoch

    last_modified="$(awk '
        tolower($0) ~ /^last-modified:/ {
            sub(/^[^:]*:[[:space:]]*/, "")
            sub(/\r$/, "")
            value = $0
        }
        END { print value }
    ' "$headers")"

    if [ -z "$last_modified" ]; then
        echo "missing Last-Modified header in $headers" >&2
        exit 1
    fi

    epoch="$(date -u -d "$last_modified" +%s)"
    printf '%s000\n' "$epoch"
}

timestamped_jar_name() {
    local jar="$1"
    local mtime_ms="$2"

    if [[ "$jar" != *.jar ]]; then
        echo "internal jar does not end with .jar: $jar" >&2
        exit 1
    fi

    printf '%s_%s.jar\n' "${jar%.jar}" "$mtime_ms"
}

meta="$(curl -fsSL "$VERSION_URL")"
version="$(printf '%s' "$meta" | sed -nE 's/.*"buildVersion":"([^"]+)".*/\1/p')"
build_datetime="$(printf '%s' "$meta" | sed -nE 's/.*"buildDateTime":"([^"]+)".*/\1/p')"

if [ -z "$version" ] || [ -z "$build_datetime" ]; then
    echo "failed to parse $VERSION_URL: $meta" >&2
    exit 1
fi

build_tag="$(printf '%s' "$version-$build_datetime" | tr -d ' :-')"
release_date="$(printf '%s' "$build_datetime" | awk '{print substr($1,1,4) "-" substr($1,5,2) "-" substr($1,7,2)}')"

installer_url_with_tag="$INSTALLER_URL?build=$build_tag"
download "$installer_url_with_tag" "$tmp/ntws-installer.sh"

download "$JRE_URL" "$tmp/zulu-jre.tar.gz"
mkdir -p "$tmp/jre"
tar -xzf "$tmp/zulu-jre.tar.gz" -C "$tmp/jre"
jre_java="$(find "$tmp/jre" -path '*/bin/java' -type f | head -n 1)"
if [ -z "$jre_java" ]; then
    echo "failed to find java in downloaded JRE" >&2
    exit 1
fi
jre_home="$(dirname "$(dirname "$jre_java")")"

mkdir -p "$tmp/home" "$tmp/data" "$tmp/config" "$tmp/cache"
env \
    HOME="$tmp/home" \
    XDG_DATA_HOME="$tmp/data" \
    XDG_CONFIG_HOME="$tmp/config" \
    XDG_CACHE_HOME="$tmp/cache" \
    INSTALL4J_NO_DB=true \
    INSTALL4J_JAVA_HOME_OVERRIDE="$jre_home" \
    sh "$tmp/ntws-installer.sh" -q -dir "$tmp/app" -overwrite >/dev/null

launcher="$tmp/app/ntws"
internal_url="$(grep -aoE -- '-DinternalJarsUrl=[^"]+' "$launcher" | head -n 1 | sed 's/^-DinternalJarsUrl=//')"
mapfile -t internal_jars < <(grep -aoE -- '-DinternalJars[0-9]*=[^"]+' "$launcher" | sed 's/^-DinternalJars[0-9]*=//' | sort -u)

if [ -z "$internal_url" ] || [ "${#internal_jars[@]}" -eq 0 ]; then
    echo "failed to parse internal jar metadata from generated launcher" >&2
    exit 1
fi

declare -a internal_jar_files=()
declare -a internal_jar_urls=()

for jar in "${internal_jars[@]}"; do
    jar_url="$internal_url/$jar?build=$build_tag"
    headers="$tmp/$jar.headers"
    download_with_headers "$jar_url" "$tmp/$jar" "$headers"
    jar_file="$(timestamped_jar_name "$jar" "$(headers_mtime_ms "$headers")")"
    mv "$tmp/$jar" "$tmp/$jar_file"
    internal_jar_files+=("$jar_file")
    internal_jar_urls+=("$jar_url")
done

cat > "$tmp/managed-block.yml" <<EOF
      # BEGIN MANAGED EXTRA-DATA
      - type: extra-data
        filename: ntws-installer.sh
        only-arches:
          - x86_64
        url: $installer_url_with_tag
        sha256: $(sum256 "$tmp/ntws-installer.sh")
        size: $(bytes "$tmp/ntws-installer.sh")
      - type: extra-data
        filename: zulu-jre.tar.gz
        only-arches:
          - x86_64
        url: $JRE_URL
        sha256: $(sum256 "$tmp/zulu-jre.tar.gz")
        size: $(bytes "$tmp/zulu-jre.tar.gz")
EOF

for i in "${!internal_jars[@]}"; do
    jar="${internal_jar_files[$i]}"
    jar_url="${internal_jar_urls[$i]}"
    cat >> "$tmp/managed-block.yml" <<EOF
      - type: extra-data
        filename: $jar
        only-arches:
          - x86_64
        url: $jar_url
        sha256: $(sum256 "$tmp/$jar")
        size: $(bytes "$tmp/$jar")
EOF
done

cat >> "$tmp/managed-block.yml" <<'EOF'
      # END MANAGED EXTRA-DATA
EOF

python3 - "$MANIFEST" "$tmp/managed-block.yml" "$version" "$release_date" <<'PY'
from pathlib import Path
import re
import sys

manifest = Path(sys.argv[1])
block = Path(sys.argv[2]).read_text()
version = sys.argv[3]
release_date = sys.argv[4]

text = manifest.read_text()
text = re.sub(
    r"      # BEGIN MANAGED EXTRA-DATA\n.*?      # END MANAGED EXTRA-DATA",
    block.rstrip(),
    text,
    flags=re.S,
)
manifest.write_text(text)

meta = manifest.with_name("com.interactivebrokers.ibkrdesktop.metainfo.xml")
meta_text = meta.read_text()
meta_text = re.sub(
    r'<release version="[^"]+" date="[^"]+" />',
    f'<release version="{version}" date="{release_date}" />',
    meta_text,
    count=1,
)
meta.write_text(meta_text)
PY

echo "Updated $MANIFEST to IBKR Desktop $version ($build_datetime)"
printf 'Internal jars:\n'
printf '  %s\n' "${internal_jar_files[@]}"
