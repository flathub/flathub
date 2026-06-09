# 💱 Currency Converter

[![Flatpak](https://img.shields.io/badge/Platform-Flatpak-universal?style=for-the-badge&logo=flatpak&logoColor=white&color=3b6cb3)](https://flatpak.org/)
[![GTK4](https://img.shields.io/badge/UI-GTK4-universal?style=for-the-badge&logo=gnome&logoColor=white&color=4a4a4a)](https://www.gtk.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

A sleek, lightweight, and blazing-fast desktop currency converter for Linux built natively with **Python 3** and **GTK4**. 

Powered by the robust [xe.otek.today](https://xe.otek.today) API, this utility features instant lookups, smart text-based filtering, and local caching designed to fit flawlessly into modern Linux environments.

---

## ✨ Features

* 🔍 **Type-to-Search Dropdowns:** No more endless scrolling. Instantly filter through global currency codes by typing.
* 🔤 **Alphabetically Sorted:** All major global fiat currencies and top cryptocurrencies (BTC, ETH) are clean and sorted by currency code.
* ⚡ **Smart Local Caching:** Conversion rates are securely cached locally for **6 hours**, protecting your API usage limit and ensuring instant responsiveness.
* ⇄ **Instant Inversion:** Swap your "From" and "To" currencies with a single click.
* 📦 **Universal Sandbox:** Distributed as a safe, isolated Flatpak bundle that runs natively on Wayland and fallback X11.

---

## 📸 Screenshot

![Currency Converter Application Preview](https://otek.today/files/otek_currency_converter_screen.png)

---

## 🚀 Installation & Distribution

This application is bundled universally via **Flatpak**. First-time users do not need to hunt down system dependencies or install manual Python GTK bindings.

### Build from Source

If you want to compile and build the package locally on your system, open your terminal inside the project directory and run:

### 1. Add the Flathub remote repository (if not already added)
```bash
flatpak remote-add --if-not-exists flathub [https://dl.flathub.org/repo/flathub.flatpakrepo](https://dl.flathub.org/repo/flathub.flatpakrepo)
```

### 2. Install the necessary GNOME SDK runtime dependencies
```bash
flatpak install flathub org.gnome.Platform//47 org.gnome.Sdk//47
```

### 3. Build and install the app locally
```bash
flatpak-builder build-dir today.otek.currency_converter.yml --force-clean --user --install
```

### Create a Standalone `.flatpak` Bundle

To generate a single, highly distributive `.flatpak` binary file that you can share with anyone:

```bash
flatpak build-bundle ~/.local/share/flatpak/repo otek_currency_converter.flatpak today.otek.currency_converter
```

### Install from a Bundle File

Any Linux user can easily install your generated `otek_currency_converter.flatpak` file via their Software Center GUI or by typing:

```bash
flatpak install ./otek_currency_converter.flatpak -y
```

---

## 🎮 Usage

Once installed, you can launch the app directly via your desktop environment's application launcher menu (search for **"Currency Converter"**), or execute it cleanly from the terminal:

```bash
flatpak run today.otek.currency_converter
```

---

## 🛠️ Tech Stack & Permissions

* **Language:** Python 3
* **UI Framework:** PyGObject (GTK 4.0)
* **Sandbox Security Sandbox Settings:**
* `--socket=wayland` (Clean, secure UI drawing)
* `--socket=fallback-x11` (Legacy display fallback compatibility)
* `--share=network` (Minimal required internet communication access to query API exchange updates)



---

## 📄 License

This project is licensed under the **MIT License**. See the `LICENSE` file for details.

Developed with ❤️ by **[ÖTEK TODAY](https://omertek.com/)**.
