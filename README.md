# Anura — Flathub Submission

This repository contains the Flatpak manifest for [Anura](https://github.com/D3M-Sudo/Anura),
an intuitive OCR text extraction tool for the GNOME desktop.

## About Anura

Anura lets you grab text from any source on your screen — videos, PDFs, images, or
protected web pages — using high-accuracy OCR powered by Tesseract.

**Key features:**
- High-accuracy OCR via Tesseract
- Built-in QR code decoding and link following
- Support for multiple languages with on-demand model download
- Privacy-focused: no telemetry, no anonymous data collection
- Modern GTK4 / Libadwaita interface following GNOME HIG

**Homepage:** https://github.com/D3M-Sudo/Anura  
**App ID:** `com.github.d3msudo.anura`  
**License:** MIT

## Installing from Flathub

```bash
flatpak install flathub com.github.d3msudo.anura
flatpak run com.github.d3msudo.anura
```

## Building locally

```bash
flatpak-builder --install --user builddir com.github.d3msudo.anura.json
```

## Reporting issues

- **App bugs:** https://github.com/D3M-Sudo/Anura/issues
- **Packaging issues:** open an issue in this repository
