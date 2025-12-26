# Launcher

<p align="center">
    <img src="./cloud/ivanbotty/Launcher/resources/cloud.ivanbotty.Launcher.svg" alt="Launcher Icon" width="128">
</p>

<p align="center">
    <strong>A modern desktop application launcher for Linux</strong>
</p>

<p align="center">
    Built with GTK4 and Adwaita | Fast | Extensible | Open Source
</p>

<p align="center">
    <a href="https://github.com/BottyIvan/launcher-app/wiki/Home">📚 Wiki Documentation</a> •
    <a href="https://github.com/BottyIvan/launcher-app/wiki/Installation">💾 Installation</a> •
    <a href="https://github.com/BottyIvan/launcher-app/wiki/Usage-Guide">📖 Usage Guide</a> •
    <a href="https://github.com/BottyIvan/launcher-app/wiki/Contributing">🤝 Contributing</a>
</p>

---

## Overview

**Launcher** provides instant search for installed applications, a built-in calculator, and an extensible architecture for adding custom features. It integrates seamlessly with modern Linux desktop environments.

<p align="center">
    <img src="./assets/Screenshot From 2025-11-09 21-55-46.png" width="600"/>
</p>

> [!IMPORTANT]
> **Launcher** is under active development. Features and interfaces may change. Contributions and feedback are welcome!

## ✨ Key Features

- 🚀 **Instant Search** - Find applications as you type with fuzzy matching
- 🎨 **Modern UI** - Beautiful GTK4/Adwaita interface with smooth animations
- ⌨️ **Keyboard-First** - Complete keyboard navigation with visible shortcuts
- 🧮 **Built-in Calculator** - Evaluate expressions without switching apps
- 🔌 **Extensible** - Plugin system for custom functionality
- 🎯 **Smart Results** - Color-coded category tags for quick identification
- 🔒 **Secure** - Flatpak sandboxing support
- ⚡ **Fast** - Optimized for performance with GPU-accelerated animations
- 🌓 **Adaptive Theming** - Automatic dark/light mode support
- 📏 **Responsive Layout** - Compact and default modes for different screen sizes

## 🚀 Quick Start

### Install with Flatpak (Recommended)

```bash
git clone https://github.com/BottyIvan/launcher-app.git
cd launcher-app
flatpak-builder --user --install --force-clean build-dir manifest.yaml
flatpak run cloud.ivanbotty.Launcher
```

### Run from Source

```bash
git clone https://github.com/BottyIvan/launcher-app.git
cd launcher-app
python3 -m cloud.ivanbotty.Launcher
```

### Keyboard Shortcuts

- **Type** - Start searching for applications and commands
- **↑/↓** - Navigate through results
- **Enter** - Launch the selected item
- **Escape** - Close the launcher
- **Ctrl+,** - Open preferences
- **Ctrl+?** or **F1** - Show all keyboard shortcuts

**📚 For detailed installation instructions, see the [Installation Guide](https://github.com/BottyIvan/launcher-app/wiki/Installation).**

## 📖 Documentation

Complete documentation is available in our **[Wiki](https://github.com/BottyIvan/launcher-app/wiki/Home)**:

- **[Home](https://github.com/BottyIvan/launcher-app/wiki/Home)** - Project overview and quick links
- **[Installation](https://github.com/BottyIvan/launcher-app/wiki/Installation)** - Installation for all platforms
- **[Usage Guide](https://github.com/BottyIvan/launcher-app/wiki/Usage-Guide)** - How to use Launcher effectively
- **[Features](https://github.com/BottyIvan/launcher-app/wiki/Features)** - Complete feature list and descriptions
- **[Configuration](https://github.com/BottyIvan/launcher-app/wiki/Configuration)** - Customize Launcher settings
- **[Architecture](https://github.com/BottyIvan/launcher-app/wiki/Architecture)** - Technical design and structure
- **[API Reference](https://github.com/BottyIvan/launcher-app/wiki/API-Reference)** - API documentation for developers
- **[Contributing](https://github.com/BottyIvan/launcher-app/wiki/Contributing)** - Contribution guidelines and dev setup
- **[FAQ](https://github.com/BottyIvan/launcher-app/wiki/FAQ)** - Frequently asked questions and troubleshooting
- **[Changelog](https://github.com/BottyIvan/launcher-app/wiki/Changelog)** - Version history and release notes
- **[License](https://github.com/BottyIvan/launcher-app/wiki/License)** - Licensing information

### UI Enhancement Documentation

See the [UI Enhancements Guide](docs/UI_ENHANCEMENTS.md) for details on the modern interface improvements, including:
- Custom CSS styling following GNOME HIG
- Smooth animations and transitions
- Keyboard-first navigation
- Color-coded category tags
- Responsive layout modes (Compact/Default)

For visual reference, check the [UI Visual Reference](docs/UI_VISUAL_REFERENCE.md).

## 🤝 Contributing

We welcome contributions! Whether you want to:
- Report bugs
- Suggest features
- Improve documentation
- Write code

**Please read our [Contributing Guide](https://github.com/BottyIvan/launcher-app/wiki/Contributing) to get started.**

## 💬 Community & Support

- **Issues**: [GitHub Issues](https://github.com/BottyIvan/launcher-app/issues)
- **Discussions**: Use GitHub Issues for questions
- **Email**: droidbotty@gmail.com

## 📋 Requirements

- Python 3.11+
- GTK4 and Adwaita
- PyGObject 3.44+

**See [Installation Guide](https://github.com/BottyIvan/launcher-app/wiki/Installation) for complete requirements.**

## 📜 License

**GPL-3.0-or-later** - See [LICENSE](LICENSE) file for details.

Learn more: [License Documentation](https://github.com/BottyIvan/launcher-app/wiki/License)

## 🙏 Acknowledgments

- Built with [GTK4](https://gtk.org/) and [Adwaita](https://gnome.pages.gitlab.gnome.org/libadwaita/)
- Icon from [Icons8](https://icons8.it/icon/qW0hxm9M3J5x/ricerca)
- Thanks to all [contributors](https://github.com/BottyIvan/launcher-app/graphs/contributors)

---

<p align="center">
    Made with ❤️ by <a href="https://github.com/BottyIvan">Ivan Bottigelli</a>
</p>

<p align="center">
    ⭐ Star us on GitHub if you find this project useful!
</p>