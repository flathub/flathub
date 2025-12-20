#!/usr/bin/env python
"""
BrowserOS Flatpak Update Script

This script fetches the latest BrowserOS release from GitHub,
updates the flatpak YAML manifest with new AppImage information,
and updates the metainfo XML file with release details.

Features:
- Only updates when changes are detected (idempotent)
- Uses SHA256 from GitHub API (no download needed)
- Converts GitHub markdown release notes to XML using pandoc
- Extracts version from AppImage filename (not tag)
"""

import requests
import yaml
import xml.etree.ElementTree as ET
import subprocess
import sys
import re
import argparse
from datetime import datetime
from pathlib import Path
import tempfile
import os


class BrowserOSUpdater:
    def __init__(self, test_mode=False):
        self.github_repo = "browseros-ai/BrowserOS"
        self.yaml_file = Path("com.browseros.BrowserOS.yml")
        self.xml_file = Path("com.browseros.BrowserOS.metainfo.xml")
        self.test_mode = test_mode
        self.github_api_base = "https://api.github.com"

    def fetch_latest_release(self):
        """Fetch the latest release data from GitHub API"""
        print("Fetching latest release from GitHub...")
        url = f"{self.github_api_base}/repos/{self.github_repo}/releases/latest"

        try:
            response = requests.get(url, timeout=30)
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Error fetching release: {e}")
            sys.exit(1)

    def extract_appimage_info(self, release_data):
        """Extract AppImage URL, SHA256, and version from filename"""
        print("Extracting AppImage information...")

        # Find the x64 AppImage asset
        appimage_asset = None
        for asset in release_data["assets"]:
            if asset["name"].endswith("_x64.AppImage"):
                appimage_asset = asset
                break

        if not appimage_asset:
            print("Error: No x64 AppImage found in release assets")
            sys.exit(1)

        # Extract version from filename (e.g., "BrowserOS_v0.33.0.1_x64.AppImage" -> "0.33.0.1")
        filename = appimage_asset["name"]
        version_match = re.search(r"BrowserOS_v([\d.]+)_x64\.AppImage", filename)
        if not version_match:
            print(f"Error: Could not extract version from filename: {filename}")
            sys.exit(1)

        version = version_match.group(1)
        url = appimage_asset["browser_download_url"]
        sha256 = appimage_asset["digest"].replace("sha256:", "")

        print(f"Found AppImage: {filename}")
        print(f"Version: {version}")
        print(f"URL: {url}")
        print(f"SHA256: {sha256}")

        return {"version": version, "filename": filename, "url": url, "sha256": sha256}

    def get_current_versions(self):
        """Get current versions from YAML and XML files"""
        current_versions = {}

        # Get version from YAML (find it in the AppImage references)
        if self.yaml_file.exists():
            with open(self.yaml_file, "r") as f:
                yaml_content = f.read()
                # Look for AppImage filename pattern
                match = re.search(r"BrowserOS_v([\d.]+)_x64\.AppImage", yaml_content)
                if match:
                    current_versions["yaml"] = match.group(1)

        # Get version from XML
        if self.xml_file.exists():
            try:
                tree = ET.parse(self.xml_file)
                root = tree.getroot()
                releases = root.find("releases")
                if releases is not None and len(releases):
                    first_release = releases[0]
                    if "version" in first_release.attrib:
                        current_versions["xml"] = first_release.attrib["version"]
            except ET.ParseError as e:
                print(f"Error parsing XML file: {e}")
                sys.exit(1)

        return current_versions

    def needs_update(self, latest_version, current_versions):
        """Check if updates are needed"""
        print(f"Latest version: {latest_version}")
        print(f"Current YAML version: {current_versions.get('yaml', 'not found')}")
        print(f"Current XML version: {current_versions.get('xml', 'not found')}")

        yaml_needs_update = current_versions.get("yaml") != latest_version
        xml_needs_update = current_versions.get("xml") != latest_version

        return yaml_needs_update, xml_needs_update

    def update_yaml(self, appimage_info):
        """Update YAML manifest with new AppImage information"""
        print(f"Updating {self.yaml_file}...")

        with open(self.yaml_file, "r") as f:
            content = f.read()

        # Replace AppImage filename in specific contexts (build commands only)
        # More specific patterns to avoid corrupting URL lines
        content = re.sub(
            r"(\./)BrowserOS_v[\d.]+_x64\.AppImage(--appimage-extract)",
            r"\1" + appimage_info["filename"] + r"\2",
            content,
        )

        content = re.sub(
            r"(chmod \+x )BrowserOS_v[\d.]+_x64\.AppImage",
            r"\1" + appimage_info["filename"],
            content,
        )

        content = re.sub(
            r"(rm -rf squashfs-root )BrowserOS_v[\d.]+_x64\.AppImage",
            r"\1" + appimage_info["filename"],
            content,
        )

        # Update URL line more specifically
        url_pattern = r"(url: )https://github\.com/browseros-ai/BrowserOS/releases/download/[^/]+/BrowserOS_v[\d.]+_x64\.AppImage"
        content = re.sub(url_pattern, r"\1" + appimage_info["url"], content)

        # Update SHA256 line more specifically
        sha_pattern = r"(sha256: )[a-f0-9]+"
        content = re.sub(sha_pattern, r"\g<1>" + appimage_info["sha256"], content)

        with open(self.yaml_file, "w") as f:
            f.write(content)

        print("YAML file updated successfully")

    def convert_markdown_to_xml(self, markdown_text):
        """Convert markdown release notes to XML-compatible elements using pandoc"""
        print("Converting markdown release notes to XML...")

        try:
            # Create a temporary file for markdown input
            with tempfile.NamedTemporaryFile(
                mode="w", suffix=".md", delete=False
            ) as md_file:
                md_file.write(markdown_text)
                md_file_path = md_file.name

            # Convert using pandoc to HTML fragment
            result = subprocess.run(
                ["pandoc", "-f", "markdown", "-t", "html", "--wrap=none", md_file_path],
                capture_output=True,
                text=True,
                check=True,
            )

            # Clean up temp file
            os.unlink(md_file_path)

            return result.stdout.strip()

        except subprocess.CalledProcessError as e:
            print(f"Error running pandoc: {e}")
            print(f"pandoc stderr: {e.stderr}")
            # Fallback to simple text if pandoc fails
            return markdown_text.strip()
        except FileNotFoundError:
            print("Warning: pandoc not found. Using simple text conversion.")
            return markdown_text.strip()

    def update_xml(self, release_data, appimage_info):
        """Update XML metainfo with new release entry"""
        print(f"Updating {self.xml_file}...")

        try:
            tree = ET.parse(self.xml_file)
            root = tree.getroot()
        except ET.ParseError as e:
            print(f"Error parsing XML file: {e}")
            sys.exit(1)

        # Find or create releases section
        releases = root.find("releases")
        if releases is None:
            releases = ET.SubElement(root, "releases")

        # Check if release with this version already exists
        existing_release = None
        for release in releases.findall("release"):
            if release.get("version") == appimage_info["version"]:
                existing_release = release
                break

        if existing_release:
            # Update existing release
            print(f"Updating existing release {appimage_info['version']}")
            existing_release.set("date", datetime.now().strftime("%Y-%m-%d"))
            # Remove old description
            description = existing_release.find("description")
            if description is not None:
                existing_release.remove(description)
        else:
            # Create new release element
            print(f"Creating new release {appimage_info['version']}")
            new_release = ET.Element("release")
            new_release.set("version", appimage_info["version"])
            new_release.set("date", datetime.now().strftime("%Y-%m-%d"))
            # Insert new release at the beginning (most recent first)
            releases.insert(0, new_release)
            existing_release = new_release

        # Add description with converted release notes
        description = ET.SubElement(existing_release, "description")

        # Convert markdown release notes and parse them into XML elements
        markdown_text = release_data.get("body", "")

        # Parse markdown to extract structured content
        lines = markdown_text.strip().split("\n")

        # Look for main heading
        for line in lines:
            line = line.strip()
            if line.startswith("##"):
                summary = line.replace("##", "").strip()
                p = ET.SubElement(description, "p")
                p.text = summary
                break

        # Extract bullet points from markdown
        bullet_points = []
        for line in lines:
            line = line.strip()
            if line.startswith("- "):
                bullet_points.append(line[2:])
            elif line.startswith("* "):
                bullet_points.append(line[2:])

        if bullet_points:
            ul = ET.SubElement(description, "ul")
            for point in bullet_points:
                li = ET.SubElement(ul, "li")
                li.text = point.strip()

        # Write updated XML
        ET.indent(tree, space="  ", level=0)
        tree.write(self.xml_file, encoding="utf-8", xml_declaration=True)

        print("XML file updated successfully")

    def remove_current_release_for_test(self):
        """Remove current release from XML for testing purposes"""
        print("TEST MODE: Removing current release from XML...")

        if not self.xml_file.exists():
            print("XML file not found")
            return False

        try:
            tree = ET.parse(self.xml_file)
            root = tree.getroot()
            releases = root.find("releases")

            if releases is not None:
                # Get all releases first
                all_releases = releases.findall("release")
                print(f"Found releases section with {len(all_releases)} release(s)")

                # Remove all releases to ensure clean state for testing
                for release in all_releases:
                    releases.remove(release)

                ET.indent(tree, space="  ", level=0)
                tree.write(self.xml_file, encoding="utf-8", xml_declaration=True)
                print("Removed all releases from XML for testing")
                return True
            else:
                print("No releases section found")
                return False

        except ET.ParseError as e:
            print(f"Error parsing XML file: {e}")
            return False

        try:
            tree = ET.parse(self.xml_file)
            root = tree.getroot()
            releases = root.find("releases")

            if releases is not None:
                print(f"Found releases section with {len(releases)} release(s)")
                # Print all release versions before removal
                for i, release in enumerate(releases):
                    print(f"  Release {i}: version={release.get('version')}")

                if len(releases) > 0:
                    # Remove first (latest) release
                    removed_release = releases[0]
                    print(
                        f"Removing release with version: {removed_release.get('version')}"
                    )
                    releases.remove(removed_release)
                    ET.indent(tree, space="  ", level=0)
                    tree.write(self.xml_file, encoding="utf-8", xml_declaration=True)
                    print("Removed current release from XML for testing")
                    return True
                else:
                    print("No releases to remove")
                    return False
            else:
                print("No releases section found")
                return False

        except ET.ParseError as e:
            print(f"Error parsing XML file: {e}")
            return False

    def run(self):
        """Main execution function"""
        print("BrowserOS Flatpak Updater")
        print("=" * 30)

        if self.test_mode:
            print("TEST MODE ENABLED")

        # Fetch latest release
        release_data = self.fetch_latest_release()
        appimage_info = self.extract_appimage_info(release_data)

        # Get current versions
        current_versions = self.get_current_versions()

        # Check if update is needed
        yaml_needs_update, xml_needs_update = self.needs_update(
            appimage_info["version"], current_versions
        )

        if self.test_mode:
            print("\nTest mode: removing current release first...")
            self.remove_current_release_for_test()
            # Force update in test mode
            yaml_needs_update = True
            xml_needs_update = True

        if not yaml_needs_update and not xml_needs_update:
            print("\nNo updates needed - already at latest version")
            return

        print(f"\nUpdates needed:")
        if yaml_needs_update:
            print(
                f"  - YAML: {current_versions.get('yaml')} -> {appimage_info['version']}"
            )
        if xml_needs_update:
            print(
                f"  - XML: {current_versions.get('xml')} -> {appimage_info['version']}"
            )

        # Perform updates
        if yaml_needs_update:
            self.update_yaml(appimage_info)

        if xml_needs_update:
            self.update_xml(release_data, appimage_info)

        print("\nUpdate completed successfully!")


def main():
    parser = argparse.ArgumentParser(
        description="Update BrowserOS flatpak with latest GitHub release"
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Test mode: remove current release and re-add it",
    )
    parser.add_argument("--version", action="version", version="1.0.0")

    args = parser.parse_args()

    updater = BrowserOSUpdater(test_mode=args.test)
    updater.run()


if __name__ == "__main__":
    main()
