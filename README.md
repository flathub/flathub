# SDK Extension for Flutter stable

This extension contains a community maintained, read-only release of the [Flutter](https://flutter.dev/) software development kit. Any issues with this SDK should be reported here.

## What do you mean read-only?

Flatpak SDK extensions are immutable; they cannot be changed once built. The Flutter SDK CLI tools have the ability to update the Flutter SDK in-place when the SDK is installed in a user-owned directory. This SDK has those functions disabled and all of Linux related SDK artifacts are pre-downloaded (no Windows, Mac, iOS, or Android builds from this SDK).

## How to build a Flutter App with this SDK

In order to use this extension in a Flatpak SDK environment, the following additions to the manifest are needed :

```yaml
sdk-extensions:
  # Flutter needs clang to compile Linux desktop apps
  - org.freedesktop.Sdk.Extension.llvm15
  - org.freedesktop.Sdk.Extension.flutter

modules:
  - name: your-flutter-app-name
    # Allow flutter to find clang by adding it to the path
    append-path: /usr/lib/sdk/llvm15/bin:/usr/lib/sdk/flutter/bin
    prepend-ld-library-path: /usr/lib/sdk/llvm15/lib
    env:
      # Ask Flutter to ignore its file locking mechanism
      - FLUTTER_ALREADY_LOCKED=true
      # Bring your own pre-cached pub-cache because `pub get` will not work without internet access
      - PUB_CACHE=some-file-in-your-source-archive
```


## Updating this manifest

Get the hashes from all the artifacts
```bash
find . -type f -exec sha256sum {} \; > ~/Documents/flathub/flutter-artifact-hashes
```
