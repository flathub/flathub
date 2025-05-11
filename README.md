# Flatpak packaging for Delta Chat Desktop Tauri Edition

to learn more about dc tauri see <https://github.com/deltachat/deltachat-desktop/blob/main/packages/target-tauri/README.md>.

## Trigger a new release using Codespaces

These are the steps to trigger a new release using github Codespaces:

- create a new PR release-x.x.x
- edit `generate.sh` in codespace and change the tags for
  
  ```
    DESKTOP_CHECKOUT=vx.x.x
  ```
 - add the new release in `<releases>` in chat.delta.desktop.appdata.xml
   - add a link to the Release Changelog
   - add some more info about the release
   - see [Release info](https://docs.flathub.org/docs/for-app-authors/metainfo-guidelines/#release)
 - start the setup script in console `./setup.sh`
 - start the generate script in console `./generate.sh`
 - wait for the build of the preview
 - install the preview locally and check if it works
 - after merging the PR the new version will be released


## Building locally

If you'd like to locally build this flapak, you'll need both `flatpak`
and `flatpak-builder` installed.  E.g. on Debian you can run `apt
install flatpak flatpak-builder` to install these tools.  See
https://flatpak.org/setup/ for more information on this for your
platform.

### flatpak dependencies

If you haven't done so yet, you need to have
[flathub](https://flathub.org) set up as a remote repository:

```
flatpak remote-add --if-not-exists \
    flathub https://flathub.org/repo/flathub.flatpakrepo
```

### Building the application

To simply build the application in a build-directory invoke
`flatpak-builder` pointing to the manifest:
```
flatpak-builder --install-deps-from=flathub build-dir chat.delta.desktop.tauri.yml
```

To install the local build you can add the `--install` flag.  To
upload the built application to a repository, which can just be a
local directory, add the `--repo=repo` flag.


### Uploading to flathub

Each commit to the https://github.com/flathub/chat.delta.desktop.tauri
master branch will result in a new release being published to
flathub.  So once a pull request is merged no more work needs to be
done to publish the release.


### Upgrade to new Release: Re-generating sources

Run `./setup.sh`. (you also need nodejs min version 20 and python3)

> to reset you can run `rm -rf working_dir`


Then edit (put in the tags/branches you want to update to) and run the `generate.sh` script:
```sh
DESKTOP_CHECKOUT=v1.45.4
```

After that, build it locally (if your computer is likely faster than CI, so debugging locally is quicker).
```
rm -r build-dir/ || true && flatpak-builder --install-deps-from=flathub build-dir chat.delta.desktop.tauri.yml --ccache
```

> `--ccache` enables sccache, which speeds up subsequent builds.

