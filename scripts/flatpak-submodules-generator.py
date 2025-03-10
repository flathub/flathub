# While flatpak module sources supports git submodules, it does not support git-lfs. This generator sets up each submodule as an archive source, because gitlab zips seem to include files stored in LFS.

import os
import json
import subprocess
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

def get_current_remote_url():
	"""Retrieve the remote URL associated with the current branch."""
	result = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], capture_output=True, text=True)
	branch = result.stdout.strip()
	
	remote_result = subprocess.run(["git", "config", f"branch.{branch}.remote"], capture_output=True, text=True)
	remote = remote_result.stdout.strip()
	
	if not remote:
		return None
	
	url_result = subprocess.run(["git", "remote", "get-url", remote], capture_output=True, text=True)
	return url_result.stdout.strip() if url_result.returncode == 0 else None


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


def main():
	repo_path = Path().resolve()
	output_file = repo_path / "submodule-sources.json"

	if not (repo_path / ".gitmodules").exists():
		print("No .gitmodules file found.")
		generate_flatpak_sources([], output_file)
		return
	
	submodules = get_git_submodules(repo_path)
	generate_flatpak_sources(submodules, output_file)

if __name__ == "__main__":
	main()
