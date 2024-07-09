# Crow Translate Flatpak

[Crow Translate](https://apps.kde.org/crowtranslate) is an application written in C++ / Qt that allows you to translate and speak text using [Mozhi](https://codeberg.org/aryak/mozhi). This repository allows installing the app through [Flatpak](https://flatpak.org).

## Installation

```bash
flatpak install flathub org.kde.CrowTranslate
```

## Running

You can run Crow Translate from your desktop menu, or via command line:

```bash
flatpak run org.kde.CrowTranslate
```

## Building

To compile the app as a Flatpak, you'll need [Flatpak Builder](http://docs.flatpak.org/en/latest/flatpak-builder.html) installed. Just clone this repository and run the following command:

```bash
flatpak-builder --user --install-deps-from=flathub --repo=repo --install builddir org.kde.CrowTranslate.yaml
```

### OCR

Application can translate text from the screen using [Tesseract](https://github.com/tesseract-ocr/tesseract). To recognize text you need to additionally install trained models. We provide optional `org.kde.CrowTranslate.tessdata` extension with [tessdata_fast](https://github.com/tesseract-ocr/tessdata_fast) models. You can install it with the following command:

```bash
flatpak install flathub org.kde.CrowTranslate.tessdata
```

The extension will be installed automatically if you [build](#building) the flatpak from source.
