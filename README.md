Build:
```
flatpak-builder build org.flathub.electron-sample-app.yml --install-deps-from=flathub --force-clean --user --install
```

Run:
```
flatpak run org.flathub.electron-sample-app
```

To build `generated-sources.json` use `flatpak-node-generator`.

```
flatpak-node-generator npm package-lock.json --no-requests-cache
```