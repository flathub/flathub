# Autofirma Flatpak (`es.gob.afirma`)

[Read in English](README.en.md)

---

Flatpak no oficial de Autofirma, la aplicación de firma electrónica del Gobierno de España.

> [!WARNING]
> **Paquete comunitario no oficial** - No está soportado por el Gobierno de España.

## Estado actual

✅ **Compatible con:**
- Firma local de documentos
- Firefox, Chromium, Google Chrome, etc. (puede requerir de una [acción manual][issue-ca-install])

❌ **No verificado:**
- DNIe (probablemente no funcione)

### Acciones necesarias para características determinadas

Acude a [la sección de _troubleshooting_][troubleshooting] para conocer diferentes soluciones a algunos problemas que puedes encontrar al usar esta distribución comunitaria de Autofirma.


## Instalación

**Prerrequisitos:**
```sh
flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08 org.freedesktop.Sdk.Extension.openjdk17//25.08
```

**Construir e instalar:**
```sh
git clone https://github.com/flathub/es.gob.afirma.git
cd es.gob.afirma
flatpak-builder --force-clean --user --install build-dir es.gob.afirma.yaml
```

Y luego la puedes lanzar desde la terminal con
```sh
flatpak run es.gob.afirma
```


## Colaboración

¡Ayúdanos a mejorar!

- [**Reportar incidencias**][issues]
- [**Chat en Matrix**][matrix-chat]


## Agradecimientos

- a [Alberto Ruiz](https://github.com/aruiz) por mostrar un camino a recorrer
- a [Ismael Asensio](https://gitlab.com/ismailof), por desbloquear por fin la compilación desde el código fuente
- a [David Marzal](https://gitlab.com/Marzal), por sus investigaciones y ayuda con logs, certificados y capturas de pantalla
- a [toda la comunidad en Mastodon](https://mastodon.social/tags/AutofirmaFlatpak) que ha estado apoyando esta idea

## Referencias

- [Sitio oficial de AutoFirma][official-website]
- [Repositorio oficial][official-repo]
- [Documentación Flatpak][flatpak-docs]
- [**Intento de construcción no oficial por aruiz**][aruiz-repo]

[aruiz-repo]: https://github.com/aruiz/autofirma-flatpak
[firefox-flathub]: https://flathub.org/apps/org.mozilla.firefox
[flatpak-docs]: https://docs.flatpak.org/
[issues]: https://github.com/flathub/es.gob.afirma/issues
[issue-ca-install]: docs/users/troubleshooting/es.md#fallo-al-llamar-a-autofirma-desde-el-navegador
[matrix-chat]: https://matrix.to/#/#autofirma-flatpak:matrix.org
[official-repo]: https://github.com/ctt-gob-es/clienteafirma
[official-website]: https://firmaelectronica.gob.es/
[troubleshooting]: docs/users/troubleshooting/es.md
