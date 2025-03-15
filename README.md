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

Manifest validation:
```
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream io.github.piotrek_k._2dTaskBoard.metainfo.xml
```