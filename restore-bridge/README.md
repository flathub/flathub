# Restore bridge source

This directory contains the source for `restore-bridge.jar`. The Flatpak
manifest ships the compiled JAR; the source is included here so reviewers can
audit the added behavior.

Build with JDK 17 or newer:

```sh
javac --release 17 -d /tmp/restore-bridge-classes \
  com/sparrowwallet/sparrow/runtime/RestoreBridge.java \
  com/sparrowwallet/sparrow/runtime/RestorePayload.java
```

The pinned JAR was built from these sources with:

```text
64841f40f81c75188a0382810ab605ad7afe13b8a3584dc638c378c318665ac7  restore-bridge.jar
48362 bytes
```
