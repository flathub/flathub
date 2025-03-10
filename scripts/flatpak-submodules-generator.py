# While flatpak module sources supports git submodules, it does not support git-lfs. This generator sets up each submodule as an archive source, because gitlab zips seem to include files stored in LFS.

import os
import json
import subprocess
import requests
from pathlib import Path

def get_git_submodules(repo_path):
	"""Retrieve submodule details from a Git repository."""
	os.chdir(repo_path)
	result = subprocess.run(["git", "submodule", "status"], capture_output=True, text=True, check=True)
	submodules = []
	
	for line in result.stdout.strip().split("\n"):
		parts = line.strip().split()
		if len(parts) < 2:
			continue
		commit, path = parts[:2]
		url = subprocess.run(["git", "config", f"--file=.gitmodules", f"submodule.{path}.url"],
							capture_output=True, text=True, check=True).stdout.strip()
		submodules.append({
			"name": path.replace("/", "-"),
			"url": url,
			"commit": commit.lstrip("+-")  # Remove leading `+` or `-`
		})
	return submodules

def generate_flatpak_sources(submodules, output_file):
	"""Generate a Flatpak sources JSON file from the submodule list."""
	sources = []
	
	for submodule in submodules:
		sources.append({
			"type": "git",
			"url": submodule["url"],
			"commit": submodule["commit"],
			"dest": submodule["name"]
		})
	
	with open(output_file, "w") as f:
		json.dump(sources, f, indent=4)
	print(f"Flatpak sources JSON saved to {output_file}")

def download_submodules_as_zip(submodules, output_dir):
	"""Download submodules as zip archives."""
	output_dir.mkdir(parents=True, exist_ok=True)
	for submodule in submodules:
		zip_url = f"{submodule['url']}/archive/{submodule['commit']}.zip"
		zip_path = output_dir / f"{submodule['name']}.zip"
		print(f"Downloading {zip_url}...")
		response = requests.get(zip_url, stream=True)
		if response.status_code == 200:
			with open(zip_path, "wb") as f:
				for chunk in response.iter_content(chunk_size=8192):
					f.write(chunk)
			print(f"Saved to {zip_path}")
		else:
			print(f"Failed to download {zip_url}")

def main():
	repo_path = Path(input("Enter the path to the Git repository: ")).resolve()
	output_file = repo_path / "flatpak-sources.json"
	zip_output_dir = repo_path / "submodules_zips"
	
	if not (repo_path / ".gitmodules").exists():
		print("No .gitmodules file found.")
		generate_flatpak_sources([], output_file)
		return
	
	submodules = get_git_submodules(repo_path)
	generate_flatpak_sources(submodules, output_file)
	
	if input("Download submodules as zip? (y/n): ").strip().lower() == "y":
		download_submodules_as_zip(submodules, zip_output_dir)

if __name__ == "__main__":
	main()
