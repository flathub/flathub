# io.github.chcandido.brModelo

Flatpak for brModelo (https://github.com/chcandido/brModelo).

## How to build brModelo

```sh
flatpak-builder --force-clean flatpakbuildir io.github.chcandido.brModelo.yaml
```

## How to install brModelo from flatpak

```sh
flatpak-builder --force-clean flatpakbuildir io.github.chcandido.brModelo.yaml --user --install
```

## How to run brModelo

```sh
flatpak run --user io.github.chcandido.brModelo
```

## How to uninstall brModelo

```sh
flatpak uninstall --user io.github.chcandido.brModelo
```

## Patches

- [`0001-fix-dirs.patch`](./patches/0001-fix-dirs.patch)

## Thanks

This flatpak manifest would not be possible without:

- https://github.com/flathub/org.apache.netbeans/tree/d4cad2d4ce497a3fbc02efe11faec24c043c0dee

- https://github.com/flathub/com.github.mgropp.PdfJumbler/tree/e1dcc40c446f7d6f8ad538d88391c53ec361bde8

- https://github.com/NixOS/nixpkgs/pull/161706
