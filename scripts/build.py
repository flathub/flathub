#!/usr/bin/env python3
import sys
import os
import logging
import argparse
import subprocess
from pathlib import Path

# --- Configuration ---
BUILDER = "flatpak-builder"
REPO_NAME = "xmcl"
BUILD_DIR = "builddir"
MANIFEST = "io.github.voxelum.xmcl.yaml"

# Get the project root directory (one level up from scripts directory)
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# --- Logging Setup ---
logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s',
    stream=sys.stderr,
)

def error_exit(message: str, exit_code: int = 1):
    """Logs an error message and exits."""
    logging.error(message)
    sys.exit(exit_code)

def check_dependencies():
    """Checks if required dependencies are installed."""
    try:
        subprocess.run([BUILDER, "--version"], capture_output=True, check=True)
    except FileNotFoundError:
        error_exit(f"'{BUILDER}' not found. Please install flatpak-builder.")
    except subprocess.CalledProcessError as e:
        error_exit(f"Error checking flatpak-builder version: {e}")

def check_manifest():
    """Checks if the manifest file exists."""
    manifest_path = PROJECT_ROOT / MANIFEST
    if not manifest_path.is_file():
        error_exit(f"Manifest file not found: {manifest_path}")
    return manifest_path

def build_flatpak(args):
    """Builds the Flatpak package using flatpak-builder."""
    manifest_path = check_manifest()
    os.chdir(PROJECT_ROOT)  # Change to project root directory
    cmd = [BUILDER]

    # Always use these flags
    cmd.extend(["--force-clean", "--install", "--user"])

    # Add required arguments
    cmd.extend([
        "--install-deps-from=flathub",
        f"--repo={REPO_NAME}",
        BUILD_DIR,
        str(manifest_path)
    ])

    logging.info(f"Running build command: {' '.join(cmd)}")

    try:
        subprocess.run(cmd, check=True)
        logging.info("\033[32mBuild completed successfully!\033[0m")
    except subprocess.CalledProcessError as e:
        error_exit(f"Build failed with error code {e.returncode}")

def parse_args():
    """Parses command line arguments."""
    # No arguments needed since flags are always used
    return argparse.Namespace(force_clean=True, install=True, user=True)

def main():
    """Main execution function."""
    args = parse_args()
    check_dependencies()
    build_flatpak(args)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        logging.info("\nBuild interrupted by user")
        sys.exit(130)
    except Exception as e:
        error_exit(f"Unexpected error: {e}")
