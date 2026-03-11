# com.getgrist.grist

Flatpak for [Grist Desktop](https://www.getgrist.com/), spreadsheet software to end data chaos.

## Updating to a new Grist version

1. Find out the latest grist-desktop release from its [releases](https://github.com/gristlabs/grist-desktop/releases).
   Make note of the _tag_ name (which starts with `v`).

2. Find out the corresponding grist-core release, it will usually be mentioned in the grist-desktop release.
   Otherwise, browser the source of the grist-desktop release tag, find the `core` commit, and find the
   version in the [grist-core repository](https://github.com/gristlabs/grist-core).
   Make note of the _tag_ name (which starts with `v`).

3. Run the update script with grist-desktop and grist-core release.
   For example, if you have grist-desktop `v0.3.10` the grist-core tag is `v1.7.11`, then run

   ```sh
   ./update.sh v0.3.10 v1.7.11
   ```

4. Build the flatpak with `flatpak-builder build com.getgrist.grist.yml --install --user`,
   and run it with `flatpak run com.getgrist.grist`. Make sure it works well.

5. If the update script mentioned anything about updating sqlite3, do this manually by downloading
   the relevant binary modules from [@gristlabs/sqlite3 releases](https://github.com/gristlabs/node-sqlite3/releases),
   and updating them in the manifest. Also update the version in the update script.
   (Note that this step can be removed when the source build works again.)
