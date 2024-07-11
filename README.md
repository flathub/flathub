# `io.github.getnf.embellish` Package Repository

<p>
  <a href="https://github.com/getnf/embellish">
  embellish
  </a>
</p>

## How to test changes

Use `flatpak-builder` to build and install the package from manifest:
```sh
flatpak-builder --install --user --force-clean repo/ io.github.getnf.embellish.yml
```

## How to update dependencies

1. Use `flatpak-go-mod` in the root of the Embellish repository:
```sh
go run github.com/dennwc/flatpak-go-mod@latest .
```

2. Copy `modules.txt` and `go.mod.yml` to the root of this package.
