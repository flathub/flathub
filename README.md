# `io.github.tfuxu.floodit` Package Repository

## How to test changes

Use `flatpak-builder` to build and install the package from manifest:
```sh
flatpak-builder --install --user --force-clean repo/ io.github.tfuxu.floodit.json
```

## How to update dependencies

1. Use `flatpak-go-mod` in the root of the Dissent repository:
```sh
go run github.com/dennwc/flatpak-go-mod@latest .
```

2. Copy `modules.txt` and `go.mod.yml` to the root of this package.
