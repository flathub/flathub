# PopSearch - A Minimal and Fast Web Search Bar

PopSearch is a lightweight and efficient search bar for Linux, built with GTK. It provides instant search suggestions from Google and allows quick web searches with customizable shortcuts.

## 🚀 Features
- 🔍 **Google Search Suggestions**: Get real-time autocomplete suggestions while typing.
- 🌐 **Quick Search Shortcuts**: Type `youtube/keyword` to search directly on YouTube, `github/keyword` for GitHub, and more.
- 🎨 **Minimal UI**: A distraction-free and stylish interface.
- ⌨️ **Custom Keyboard Shortcut**: Set your preferred shortcut on first launch.
- ❌ **Auto Close**: Closes automatically when it loses focus.
- 🔗 **Lightweight & Fast**: Optimized for speed and performance.

## 📦 Installation (Flatpak)

### 1️⃣ **Enable Flatpak (if not installed)**
If Flatpak is not already set up on your system, follow the official guide:
[Flatpak Setup](https://flatpak.org/setup/)

### 2️⃣ **Install PopSearch**
Once available on Flathub, install using:
```sh
flatpak install flathub com.github.rajnisht7.PopSearch
```

### 3️⃣ **Run the App**
```sh
flatpak run com.github.rajnisht7.PopSearch
```

## 🔧 Manual Shortcut Setup
If the automatic shortcut setup does not work, you can manually assign a shortcut:

1. Open **Settings** > **Keyboard** > **Custom Shortcuts**
2. Click **Add Shortcut**
3. Set Name: `PopSearch`
4. Command: 
   ```sh
   flatpak run com.github.rajnisht7.PopSearch
   ```
5. Set your preferred shortcut (e.g., `Ctrl + Space`)
6. Click **Add** and you're done!

## 🛠 Development & Contribution

The source code is available on GitHub:  
👉 **[PopSearch Repository](https://github.com/rajnisht7/PopSearch)**

Feel free to fork, open issues, or contribute improvements! 🎉

## 📜 License
PopSearch is licensed under the **MIT License**, allowing free use, modification, and distribution.

---
Enjoy fast and efficient searching with PopSearch! 🚀
