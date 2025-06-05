# Qobuz for Linux

This is an UNOFFICIAL wrapper around the Qobuz Webplayer. Since it uses Electron, playing HiRes through widevine is also possible. I built this app, because my main browser Firefox is only playing MP3-quality streams, even if I enable DRM-features in settings.

## Updating sources

First, make sure the flatpak manifest contains the latest git tag:

```
$ flatpak install --noninteractive org.flathub.flatpak-external-data-checker
$ flatpak run org.flathub.flatpak-external-data-checker --edit-only dev.mukkematti.qobuz-linux.yml
```

Next, retrieve the git tag from the flatpak manifest:

```
$ COMMIT_ID="$(python3 -c 'import json, yaml; print(json.dumps(yaml.safe_load(open("dev.mukkematti.qobuz-linux.yml"))))' | jq -r '.modules[] | select(.name == "qobuz-linux").sources[] | select(type=="object") | select(.url == "https://github.com/mattipunkt/qobuz-linux").commit')"
$ echo "$COMMIT_ID"
```

Now download `package.json` and `package-lock.json`:

```
$ curl -O -O https://raw.githubusercontent.com/mattipunkt/qobuz-linux/${COMMIT_ID}/package{,-lock}.json
$ ls -lh package{,-lock}.json
```

Finally, generate NPM build sources:

```
$ pip install git+https://github.com/flatpak/flatpak-builder-tools.git#subdirectory=node
$ flatpak-node-generator npm package-lock.json --output generated-sources.json
```

## Building

First, make sure flatpak-builder and SDKs are installed:

```
$ flatpak install --noninteractive org.flatpak.Builder org.freedesktop.Sdk//24.08 org.freedesktop.Sdk.Extension.node24//24.08
```

Now build the flatpak:

```
$ flatpak run org.flatpak.Builder --force-clean --repo=_repo _build dev.mukkematti.qobuz-linux.yml
$ flatpak build-bundle _repo qobuz-linux.flatpak dev.mukkematti.qobuz-linux
```


