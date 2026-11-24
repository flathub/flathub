#!/bin/bash
# MM Generator Flatpak launch wrapper
set -e

# Configure Java runtime from SDK extension
export JAVA_HOME=/app/jre
export PATH=$JAVA_HOME/bin:$PATH

# Ensure JavaFX uses GTK3 backend under Wayland/X11
export JAVAFX_PLATFORM=gtk3

# Launch the application
exec /app/lib/mm-generator/bin/mm-generator "$@"
