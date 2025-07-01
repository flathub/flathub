# NormCap Flatpak

## Update deps

```sh
flatpak-cargo-generator ../sphereview/Cargo.lock -o cargo-sources.json

flatpak-node-generator npm ../sphereview/resources/photosphereviewer/package-lock.json -o node-sources.json
```

## Build & install

```sh
flatpak-builder --install-deps-from=flathub --install --user --force-clean build-dir io.github.dynobo.sphereview.yml
```

## Run

```sh
flatpak run io.github.dynobo.sphereview
```

## Application Source

See [dynobo/sphereview](https://github.com/dynobo/sphereview).
