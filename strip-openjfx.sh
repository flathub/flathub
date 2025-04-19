#!/bin/bash

# Safety check: Make sure we're in the right directory
if [ ! -f "javafx.base.jar" ]; then
  echo "❌ Not in the correct openjfx directory. Please cd into it first."
  exit 1
fi

echo "🧹 Stripping unused JavaFX components..."

# Remove WebView components
rm -v javafx.web.jar libjfxwebkit.so 2>/dev/null

# Remove Swing interop
rm -v javafx.swing.jar 2>/dev/null

# Remove SWT interop
rm -v javafx-swt.jar 2>/dev/null

# Remove Media-related files
rm -v javafx.media.jar libjfxmedia.so libgstreamer-lite.so libfxplugins.so 2>/dev/null
rm -v libavplugin-*.so 2>/dev/null

echo "✅ Done! Unused JavaFX components have been removed."
