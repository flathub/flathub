#!/usr/bin/env bash
# build_appimage_v2.sh — Bouwt de AppImage zonder systeem-pip problemen.

set -euo pipefail

SRC="$(pwd)"
BUILD_DIR="$(mktemp -d)"
DIST_DIR="$SRC/dist"
APP_NAME="Lopus"
VERSION="1.0"
ARCH="$(uname -m)"

# Determine Python version for directory structure
python_ver=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
APP_DIR="$BUILD_DIR/app_structure"
LIB_PATH="$APP_DIR/usr/lib/python$python_ver/site-packages"

echo "=== Starting AppImage Build (V2: Venv-based) ==="
echo "Python version detected: $python_ver"

# 1. Setup structure
mkdir -p "$APP_DIR/usr/bin"
mkdir -p "$LIB_PATH"
mkdir -p "$APP_DIR/usr/share/applications"
mkdir -p "$APP_DIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$DIST_DIR"

# 2. Install dependencies
echo ">>> Creating local venv and installing dependencies into $LIB_PATH..."
python3 -m venv "$BUILD_DIR/venv"
source "$BUILD_DIR/venv/bin/activate"

# We use --target so the libs actually end up in our AppImage structure
python3 -m pip install --target="$LIB_PATH" PyQt6 mutagen

# 3. Copy Python source files
echo ">>> Copying Python source files..."
find "$SRC" -maxdepth 1 -name "*.py" -exec cp {} "$APP_DIR/usr/bin/" \;

# 4. Copy packaging assets
echo ">>> Copying desktop and icons..."
if [ -d "$SRC/packaging" ]; then
    cp "$SRC/packaging/lopus.desktop" "$APP_DIR/lopus.desktop" 2>/dev/null || true
    cp "$SRC/packaging/lopus.desktop" "$APP_DIR/usr/share/applications/lopus.desktop" 2>/dev/null || true

    # Fix the Exec and Categories in the desktop file properties
    sed -i 's|^Exec=.*|Exec=python3 /usr/bin/lopus.py %F|' "$APP_DIR/lopus.desktop"
    sed -i 's|^Exec=.*|Exec=python3 /usr/bin/lopus.py %F|' "$APP_DIR/usr/share/applications/lopus.desktop"
    sed -i 's|^Categories=.*|Categories=System;FileTools;|' "$APP_DIR/lopus.desktop"
    sed -i 's|^Categories=.*|Categories=System;FileTools;|' "$APP_DIR/usr/share/applications/lopus.desktop" 2>/dev/null || true

    cp "$SRC/packaging/lopus.svg" "$APP_DIR/usr/share/icons/hicolor/scalable/apps/lopus.svg" 2>/dev/null || true
    cp "$SRC/packaging/lopus.svg" "$APP_DIR/lopus.svg" 2>/dev/null || true
fi

# 5. Create AppRun (The entry point)
echo ">>> Creating AppRun..."
cat > "$APP_DIR/AppRun" <<EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
export PYTHONPATH="\$HERE/usr/lib/python$python_ver/site-packages"
export XDG_DATA_DIRS="\$HERE/usr/share:\$XDG_DATA_DIRS"
exec python3 "\$HERE/usr/bin/lopus.py" "\$@"
EOF
chmod +x "$APP_DIR/AppRun"

# 6. Packaging with appimagetool
echo ">>> Packaging with appimagetool..."
if ! command -v appimagetool &> /dev/null; then
    echo ">>> appimagetool not found. Downloading..."
    curl -L -o "$BUILD_DIR/appimagetool.AppImage" "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$BUILD_DIR/appimagetool.AppImage"
    TOOL="$BUILD_DIR/appimagetool.AppImage"
else
    TOOL="appimagetool"
fi

"$TOOL" "$APP_DIR" "$DIST_DIR/${APP_NAME}-${VERSION}-${ARCH}.AppImage"

echo "=== Build Complete! ==="
echo "Location: $DIST_DIR/${APP_NAME}-${VERSION}-${ARCH}.AppImage"
