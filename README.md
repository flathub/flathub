# Flatpak para KMeteo

Esta carpeta contiene el manifiesto de Flatpak para publicar la aplicación desde un repositorio remoto, como se hace en Flathub.

## Requisitos

- flatpak
- flatpak-builder
- sistema de runtimes de Flatpak disponible

```bash
flatpak remote-add --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
```

## Compilar desde el repositorio remoto

Desde la raíz del proyecto:

```bash
flatpak-builder --user --install --force-clean build-dir com.gitlab.bitseater.kmeteo.yml
```

## Ejecutar

```bash
flatpak run com.gitlab.bitseater.kmeteo
```

## Rebuild

```bash
flatpak-builder --user --install --force-clean build-dir com.gitlab.bitseater.kmeteo.yml
```

> El módulo de la app usa `type: git` para que el manifiesto pueda construirse desde el repositorio remoto y prepararse para su envío a Flathub.
