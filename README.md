# SDK Extension for Flutter stable

This extension contains the [Flutter](https://flutter.dev/) software development kit.

## Debugging/Development

In order to use this extension in a flatpak SDK environment, the following additions to the manifest are needed :

```yaml
sdk-extensions:
  # Flutter needs clang to compile Linux desktop apps
  - org.freedesktop.Sdk.Extension.llvm15

modules:
  - name: your-flutter-app-name
    # Allow flutter to find clang by adding it to the path
    append-path: /usr/lib/sdk/llvm15/bin:/usr/lib/sdk/flutter/bin
    prepend-ld-library-path: /usr/lib/sdk/llvm15/lib
    build-args:
      # Flutter/Dart need network access to get your dependencies
      - --share=network
```
