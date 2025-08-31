#!/bin/bash
# T-Crisis 4 Flatpak Wrapper Script
# Changes to the correct directory before running the game

# Debug information
echo "Wrapper script started"
echo "Current directory: $(pwd)"
echo "App directory contents:"
ls -la /app/bin/ || echo "Cannot list /app/bin/"

# Change to the correct directory where the game and data are located
cd /app/bin || {
    echo "ERROR: Cannot change to /app/bin directory"
    exit 1
}

echo "Changed to: $(pwd)"
echo "Contents of current directory:"
ls -la

# Check if data directory exists
if [ -d "data" ]; then
    echo "✅ data directory found"
    echo "Contents of data directory:"
    ls -la data/
else
    echo "❌ ERROR: data directory not found in $(pwd)"
    echo "Available directories:"
    find . -type d -maxdepth 2
    exit 1
fi

# Check if executable exists
if [ -f "./tc4-tribute3" ]; then
    echo "✅ tc4-tribute3 executable found"
    ls -la ./tc4-tribute3
else
    echo "❌ ERROR: tc4-tribute3 executable not found"
    exit 1
fi

echo "Starting game..."
exec ./tc4-tribute3 "$@"
