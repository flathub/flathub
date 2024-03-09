# `so.libdb.dissent` Package Repository

<p>
  <a href="https://matrix.to/#/#nixhub-home:matrix.org">
    <img alt="nixhub Matrix chat room" src="https://img.shields.io/matrix/nixhub-home:matrix.org?color=%23222&label=nixhub&logo=Matrix&logoColor=white" />
  </a>
  <a href="https://discord.gg/hnzYamS">
    <img alt="nixhub Discord chat room" src="https://img.shields.io/discord/118456055842734083?color=%23738ADB&label=nixhub&logo=Discord&logoColor=white" />
  </a>
  <a href="https://github.com/diamondburned/dissent">
    <img alt="official repo" src="https://img.shields.io/static/v1?message=diamondburned/dissent&color=CDD9E5&label=github&logo=Github&logoColor=white" />
  </a>
</p>

## How to test changes

Use `flatpak-builder` to build and install the package from manifest:
```sh
flatpak-builder --install --user --force-clean repo/ so.libdb.dissent.yml
```

## How to update dependencies

1. Use `flatpak-go-mod` with updated `modfile` dependency in the root of the Dissent repository:
```sh
go run github.com/tfuxu/flatpak-go-mod@update-deps .
```

2. Copy `modules.txt` and `go.mod.yml` to the root of this package.
