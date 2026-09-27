# Flathub Packaging for Linux Desktop Entry Creator

This directory contains all the required metadata and manifest files to package and publish **Linux Desktop Entry Creator** on [Flathub](https://flathub.org).

## 📁 Files Overview

- **`com.binhmmo.app_launcher_creator.yml`**: The main Flathub Flatpak manifest. Configures runtime (`org.freedesktop.Platform 24.08`), permissions (filesystem access for applications, icons, host binaries), build commands, and release sources.
- **`com.binhmmo.app_launcher_creator.metainfo.xml`**: AppStream 1.0 metadata containing the app description, categories, developer info, release notes, and screenshot links for the Flathub store page.
- **`com.binhmmo.app_launcher_creator.desktop`**: FreeDesktop `.desktop` entry conforming to Flatpak specifications.
- **`flathub.json`**: Flathub build configuration specifying `only-arches: ["x86_64"]` for pre-built binaries.
- **`com.binhmmo.app_launcher_creator.svg`**: Scalable vector application icon.
- **`icon_*.png`**: High-resolution rendered icons (512x512, 128x128, 64x64).

---

## 🚀 How to Publish to Flathub (Step-by-Step)

### Step 1: Create a GitHub Release
1. Push your code to your GitHub repository:
   ```bash
   git remote add origin https://github.com/yaibait/Linux-Desktop-Entry-Creator.git
   git push -u origin main
   ```
2. On GitHub, navigate to **Releases** -> **Draft a new release**.
3. Set tag to `v1.0.0` and title to `v1.0.0`.
4. Upload `LDEC_x86_64.tar.gz` to the Release assets.
   - The SHA256 checksum is already configured in `com.binhmmo.app_launcher_creator.yml`:
     `f17bd12acbd368b3ae50d8ae8d07c213c271555ce40e8f035ebd20e566ecfe30`

---

### Step 2: Validate Metadata Locally

Verify that your AppStream metainfo and desktop entry pass all checks:

```bash
# Validate metainfo (AppStream)
appstreamcli validate --no-net packaging/flatpak/com.binhmmo.app_launcher_creator.metainfo.xml

# Validate desktop file
desktop-file-validate packaging/flatpak/com.binhmmo.app_launcher_creator.desktop
```

---

### Step 3: Test Flatpak Build Locally

To build and test the Flatpak on your local machine:

```bash
# Install Flatpak Builder and Freedesktop SDK if not already present:
flatpak install flathub org.freedesktop.Platform//24.08 org.freedesktop.Sdk//24.08

# Build and install locally
flatpak-builder --user --install --force-clean build-dir packaging/flatpak/com.binhmmo.app_launcher_creator.yml

# Run your Flatpak app
flatpak run com.binhmmo.app_launcher_creator
```

---

### Step 4: Submit to Flathub

1. Fork the official Flathub submission repository:
   [https://github.com/flathub/flathub](https://github.com/flathub/flathub)
2. Clone your fork:
   ```bash
   git clone https://github.com/<your-username>/flathub.git
   cd flathub
   git checkout -b new-app-com.binhmmo.app_launcher_creator
   ```
3. Copy all files from `packaging/flatpak/*` into the root of your branch:
   - `com.binhmmo.app_launcher_creator.yml`
   - `com.binhmmo.app_launcher_creator.metainfo.xml`
   - `com.binhmmo.app_launcher_creator.desktop`
   - `flathub.json`
   - `com.binhmmo.app_launcher_creator.svg`
   - `icon_*.png`
4. Commit and push:
   ```bash
   git add .
   git commit -m "Add com.binhmmo.app_launcher_creator"
   git push origin new-app-com.binhmmo.app_launcher_creator
   ```
5. Open a **Pull Request** on [flathub/flathub](https://github.com/flathub/flathub).
6. The Flathub automated CI bot will test-build your app and provide feedback. Once reviewed by the Flathub team, your app will be published to the Flathub store!
