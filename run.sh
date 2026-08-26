#!/usr/bin/env bash
set -euo pipefail

DATA_DIR="${XDG_DATA_HOME:-$HOME/.var/app/io.qzz.OninesixY.HMCL/data}/hmcl-bin"
mkdir -p "$DATA_DIR"

VERSION_FILE="$DATA_DIR/version.txt"
EXEC_PATH="$DATA_DIR/HMCL.jar"
TMP_PATH="$DATA_DIR/HMCL.jar.tmp"

echo "[INFO] Checking for the latest HMCL version..."

CURRENT_TAG=""
if [ -f "$VERSION_FILE" ]; then
    CURRENT_TAG=$(cat "$VERSION_FILE" || true)
fi

RELEASE_JSON=$(curl -sL --connect-timeout 8 -H "User-Agent: HMCL-Flatpak-Launcher" https://api.github.com/repos/HMCL-dev/HMCL/releases/latest || true)

UPDATED=false

if [ -n "$RELEASE_JSON" ] && echo "$RELEASE_JSON" | jq -e . >/dev/null 2>&1; then
    LATEST_TAG=$(echo "$RELEASE_JSON" | jq -r '.tag_name // empty')
    
    DOWNLOAD_URL=$(echo "$RELEASE_JSON" | jq -r '.assets[]? | select(.name | test("^HMCL-.*\\.jar$")) | .browser_download_url // empty' | head -n 1)
    if [ -z "$DOWNLOAD_URL" ]; then
        DOWNLOAD_URL=$(echo "$RELEASE_JSON" | jq -r '.assets[]? | select(.name | endswith(".jar")) | .browser_download_url // empty' | head -n 1)
    fi

    if [ -n "$LATEST_TAG" ] && [ -n "$DOWNLOAD_URL" ] && [ "$LATEST_TAG" != "$CURRENT_TAG" ]; then
        echo "[INFO] New version '$LATEST_TAG' detected (current: '${CURRENT_TAG:-none}'). Starting update..."
        
        if curl -sL "$DOWNLOAD_URL" -o "$TMP_PATH" && [ -s "$TMP_PATH" ] && [ $(stat -c%s "$TMP_PATH" 2>/dev/null || stat -f%z "$TMP_PATH") -gt 2097152 ]; then
            mv "$TMP_PATH" "$EXEC_PATH"
            echo "$LATEST_TAG" > "$VERSION_FILE"
            echo "[INFO] Update completed successfully to $LATEST_TAG."
            UPDATED=true
        else
            echo "[WARN] Download failed or file corrupted."
            rm -f "$TMP_PATH"
        fi
    else
        [ -n "$CURRENT_TAG" ] && echo "[INFO] HMCL is up to date (version: $CURRENT_TAG)."
    fi
else
    echo "[WARN] Could not reach GitHub API or response was invalid."
fi

if [ "$UPDATED" = false ] && [ -f "$EXEC_PATH" ]; then
    echo "[INFO] Launching existing local binary: $EXEC_PATH (version: ${CURRENT_TAG:-unknown})"
fi

if [ ! -f "$EXEC_PATH" ]; then
    echo "[ERROR] No HMCL executable found at $EXEC_PATH and online download failed. Exiting." >&2
    exit 1
fi

exec java -jar "$EXEC_PATH" "$@"