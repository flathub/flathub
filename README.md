# Talanoa

**Talanoa** is a modern email client designed to be fast, secure, and focused — built for professionals who want clarity in their inbox.

This repository contains the **Flatpak wrapper** for [Talanoa](https://talanoa.email), available on [Flathub](https://flathub.org/apps/details/com.talanoa.Talanoa).

## 📦 Installation

To install Talanoa via Flathub:

```bash
flatpak install flathub com.talanoa.Talanoa
flatpak run com.talanoa.Talanoa
```

## 🖼️ Wayland Support

Wayland support is available but opt-in. To enable it:

```bash
flatpak override --user --socket=wayland com.talanoa.Talanoa
```

To disable it:

```bash
flatpak override --user --nosocket=wayland com.talanoa.Talanoa
```