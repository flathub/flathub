# While flatpak module sources supports git submodules, it does not support git-lfs. This generator sets up each submodule as an archive source, because gitlab zips seem to include files stored in LFS.

import os
import json
import subprocess
import requests
from pathlib import Path
import argparse
from urllib.parse import urlparse
import hashlib

def get_git_submodules(repo_path, upstream_url=None):
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
		if url.startswith("..") and upstream_url is not None:
			urlparsed = urlparse(upstream_url)
			# this is unconventional, but it works
			modified_url_path = Path(urlparsed.path).joinpath(url).resolve()

			url = urlparsed._replace(path=str(modified_url_path)).geturl()

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
			"sha256": submodule["filesha"],
			"commit": submodule["commit"],
			"dest": submodule["name"]
		})
	
	with open(output_file, "w") as f:
		json.dump(sources, f, indent=4)
	print(f"Flatpak sources JSON saved to {output_file}")

def get_sha_for_submodule(submodule):
	zip_url = transform_url_for_zip_download(submodule["url"], submodule["commit"], submodule["name"])
	print(f"Downloading and hashing {zip_url}...")
	response = requests.get(zip_url, stream=True)
	sha256 = hashlib.sha256()
	if response.status_code == 200:
		# with open(zip_path, "wb") as f:
		for chunk in response.iter_content(chunk_size=8192):
			sha256.update(chunk)
		
	return sha256.hexdigest()

def transform_url_for_zip_download(url, commit, name):
	if url.endswith("/"):
		url = url[:-1]
	if url.endswith(".git"):
		url = url[:-4]
	return f"{url}/-/archive/{commit}/{name}.zip"

def main():

	parser = argparse.ArgumentParser(fromfile_prefix_chars='@')

	parser.add_argument('--upstream-url', help="the url to the upstream repository")
	args = parser.parse_args()

	repo_path = Path().resolve()
	output_file = repo_path / "submodule-sources.json"

	if not (repo_path / ".gitmodules").exists():
		print("No .gitmodules file found.")
		generate_flatpak_sources([], output_file)
		return

	submodules = get_git_submodules(repo_path, args.upstream_url)
	for submodule in submodules:
		sha = get_sha_for_submodule(submodule)
		submodule["filesha"] = sha

	generate_flatpak_sources(submodules, output_file)

if __name__ == "__main__":
	main()
