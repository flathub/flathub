# org.gnome.Crosswords.PuzzleSets.technopol

## Build + install

```bash
flatpak run --command=flathub-build org.flatpak.Builder --install org.gnome.Crosswords.PuzzleSets.technopol.json
```

## Lint

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest org.gnome.Crosswords.PuzzleSets.technopol.json
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
```

## Export to .flatpak file

```bash
flatpak build-bundle repo technopol.flatpak runtime/org.gnome.Crosswords.PuzzleSets.technopol/x86_64/1
```
