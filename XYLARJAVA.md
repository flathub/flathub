# Flatpak / Flathub packaging for XylarJava

App ID: `io.xylarjava.XylarJava`  
Binary / command: `XylarJava`  
Homepage: https://github.com/mrmariixx/XylarJava

## Local build (Linux)

```bash
flatpak install -y flathub org.flatpak.Builder org.kde.Sdk//6.9 org.kde.Platform//6.9 org.freedesktop.Sdk.Extension.openjdk17//24.08
flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo

flatpak run --command=flathub-build org.flatpak.Builder --install flatpak/io.xylarjava.XylarJava.yml
flatpak run io.xylarjava.XylarJava
```

Lint:

```bash
flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest flatpak/io.xylarjava.XylarJava.yml
```

## Flathub submission

Follow https://docs.flathub.org/docs/for-app-authors/submission

1. Fork https://github.com/flathub/flathub (uncheck “Copy the master branch only”)
2. Clone with `new-pr` branch, create branch `io.xylarjava.XylarJava`
3. Copy the contents of this `flatpak/` folder into the PR root
4. Open PR against base branch **`new-pr`** with title: `Add io.xylarjava.XylarJava`
5. After review, comment `bot, build`

## Notes

- Push all launcher changes to `mrmariixx/XylarJava` **before** expecting a green Flathub build (manifest builds from GitHub `main`).
- Screenshots in AppStream (`metainfo`) improve listing quality; add hosted URLs when available.
- Microsoft login still requires Azure App ID approval from Mojang: https://aka.ms/mce-reviewappid
