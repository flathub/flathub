# Chunker

Chunker is an open-source GUI application that allows you to convert your Minecraft worlds between Java and Bedrock editions.

Note that this Flatpak is a community effort and currently isn't officially supported by the developers of Chunker.

This repo hosts the Flatpak version of [Chunker](https://oss.chunker.app), available on [Flathub](https://flathub.org/apps/details/app.chunker.oss.chunker).

```
flatpak install app.chunker.oss.chunker
flatpak run app.chunker.oss.chunker
```

## Running the CLI

Chunker also comes with a CLI that allows you to easily convert your worlds right from your command line.
Unfortunately, there is currently no way to expose a binary from a Flatpak in a nice way.

In the meantime you can run the CLI in the following way:

```
flatpak run --command="/app/chunker/resources/chunker-cli/bin/chunker-cli --help" app.chunker.oss.chunker
```
