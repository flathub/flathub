# Flatpak build notes

Instructions for building Buzz Flatpak. 

General Flatpak notes https://docs.flathub.org/docs/for-app-authors/submission/

## Build

Use code from https://github.com/chidiwilliams/buzz

Install `req2flatpak` by running `pip install req2flatpak`

1. Prepare `pyptoject.toml` file
- Remove extra source used for Windows
  - Search for `"torch"` remove `tool.poetry.source` and dependency for `"2.2.1+cu121"`
  - Run `poetrty lock`

2. Export `requirements.txt`
- `poetry export --without-hashes --format=requirements.txt > requirements.txt`

3. Add desired Buzz version to `requirements.txt`
- `buzz-captions==1.2.0` 

4. Generate dependency `.json`
- `req2flatpak --requirements-file requirements.txt --target-platforms 312-x86_64 > buzz-pip-dependencies.json`

5. Move `buzz-captions` to the main manifest for cleaner and more readable setup

6. Build
