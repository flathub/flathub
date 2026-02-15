# AutoFirma Flatpak (`es.gob.afirma`)

[Leer en español](README.md)

---

Unofficial Flatpak for AutoFirma, the digital signature application provided by the Government of Spain.

> [!WARNING]
> **Unofficial community package** - This is not supported by the Spanish Government.

## Current status

✅ **Compatible with:**
- Local document signing
- Firefox, Chromium, Google Chrome, etc. (may require [manual action][issue-ca-install])

❌ **Unverified:**
- DNIe (Spanish Electronic ID) — likely non-functional

### Actions required for specific features

Visit the [troubleshooting section][troubleshooting] to find solutions for common issues you might encounter using this community distribution of AutoFirma.

## Installation

**Prerequisites:**
```sh
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08 org.freedesktop.Sdk.Extension.openjdk17//25.08

```

**Build and install:**

```sh
git clone https://github.com/flathub/es.gob.afirma.git
cd es.gob.afirma
flatpak-builder --force-clean --user --install build-dir es.gob.afirma.yaml

```

You can then launch it from the terminal using:

```sh
flatpak run es.gob.afirma

```

## Contributing

Help us improve!

* [**Report issues**][issues]
* [**Matrix chat**][matrix-chat]

## Acknowledgements

- To [Alberto Ruiz](https://github.com/aruiz) for paving the way.
- To [Ismael Asensio](https://gitlab.com/ismailof) for finally getting source-code compilation over the line.
- To [David Marzal](https://gitlab.com/Marzal) for his research and assistance with logs, certificates, and screenshots.
- To the [entire Mastodon community](https://mastodon.social/tags/AutofirmaFlatpak) for their ongoing support of this project.

## References

* [Official AutoFirma website][official-website]
* [Official repository][official-repo]
* [Flatpak documentation][flatpak-docs]
* [**Unofficial build attempt by aruiz**][aruiz-repo]


[aruiz-repo]: https://github.com/aruiz/autofirma-flatpak
[firefox-flathub]: https://flathub.org/apps/org.mozilla.firefox
[flatpak-docs]: https://docs.flatpak.org/
[issues]: https://github.com/flathub/es.gob.afirma/issues
[issue-ca-install]: docs/users/troubleshooting/en.md#failure-to-call-autofirma-from-the-browser
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org
[official-repo]: https://github.com/ctt-gob-es/clienteafirma
[official-website]: https://firmaelectronica.gob.es/  
[troubleshooting]: docs/users/troubleshooting/en.md
