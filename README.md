# Build flatpak locally

```
flatpak-node-generator npm ../package-lock.json -o generated-sources.json
flatpak-builder build zone.dos.Browser.yaml --install-deps-from=flathub --force-clean --user --install
```

To enter build shell:
```
flatpak-builder build --install-deps-from=flathub --force-clean --user --install zone.dos.Browser.yaml --build-shell=dos-browser
```

To validate:
```
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest zone.dos.Browser.yaml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream zone.dos.Browser.metainfo.xml
```

# Run flatpak

```
flatpak run zone.dos.Browser
```
