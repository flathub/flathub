# SkyTemple Flatpak

- App ID: `org.skytemple.SkyTemple`

## Repository Structure
- `/`: Contains the manifest files, desktop file, appdata and the run.sh start script.
- `flatpak-builder-tools` (submodule): Community builder tools repository
- `_generate.sh`: Script that uses the builder tools to generate required JSON files.
- `patches/`: Contains patch files used during the build.
