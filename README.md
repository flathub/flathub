# Audax Data Manager Flatpak

Release and maintenance steps for the Flatpak package `com.commandprompt.audax_data_manager`. Run the commands from this folder.

## Release a new version

1. In the Audax Data Manager repo, before you create the release tag, add a `<release>` entry for the new version (version, date and release-notes URL) to `deploy/linux/flatpak/com.commandprompt.audax_data_manager.metainfo.xml`. Then validate the file:

```sh
appstreamcli validate deploy/linux/flatpak/com.commandprompt.audax_data_manager.metainfo.xml
```

2. In the git source of the `audaxdm` module, set `tag` to the release tag and `commit` to its full commit hash.
3. If `requirements.txt` or `package-lock.json` changed in the release, regenerate the dependency files (see the next sections).
   If the release uses a different NW.js version (`NWJS_URL` in `deploy/linux/tarbuild/entrypoint.sh`), update the `nwjs` source.
4. Set `AUDAXDM_VERSION` in the `audaxdm` module to the new version.
5. Push the changes to a pull request in this repo.

## Regenerate the Python dependencies

Run the script with `requirements.txt` of the release when it changes. When you change `runtime-version` in the manifest, also change `--runtime` in the script and run it again.

```sh
./regen-pip.sh <path to requirements.txt>
```

## Regenerate the npm sources

Run these commands when `package-lock.json` changes:

```sh
flatpak-node-generator npm package-lock.json -o npm-sources.json
```

Every package in `package-lock.json` must have a `resolved` and an `integrity` field. The generator skips entries without them, and the offline `npm ci` then stops with `ENOTCACHED`.
