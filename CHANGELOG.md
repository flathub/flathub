# 📝 Kerveros Changelog

**Version 1.0.0 - Released November 15, 2025**

---

## 🎉 Initial Release
First stable release of Kerveros - A secure 2FA code generator with master password protection.

---

## ✨ New Features
- **TOTP Code Generation:** Complete implementation of time-based one-time password algorithm with 30-second intervals.
- **Multi-Account Support:** Manage unlimited 2FA accounts for services like GitHub, Google, PayPal, etc.
- **Master Password Protection:** Optional password protection with SHA-256 hashing and salting.
- **Encrypted Storage:** Automatic encryption of 2FA secrets using XOR encryption with initialization vectors.
- **QR Code Import:** Drag & drop or file menu import of QR code images using zbarimg.
- **System Tray Integration:** Minimize to tray with single-click restore and context menu.
- **Export/Import Functionality:** Full backup and restore of accounts and settings.
- **Single Instance Enforcement:** Prevents multiple app instances using QLocalServer.
- **Cross-Platform Support:** Fully functional on Linux, Windows, and macOS.

---

## 🎨 User Interface
- **Clean Splitter Layout:** Resizable interface separating accounts list from code display.
- **Visual Countdown Timer:** Color-coded progress bar (green/yellow/red) showing time remaining.
- **One-Click Copy:** Instant code copying with visual feedback and auto-revert.
- **Comprehensive Menu System:** File, Security, Settings, and Tools menus for all functionality.
- **Account Management:** Add, edit, and delete accounts with validation and duplicate prevention.

---

## 🔒 Security Enhancements
- **Password Attempt Limiting:** 5 attempts before factory reset option.
- **Automatic Encryption Migration:** Seamless transition when enabling password protection.
- **Factory Reset:** Complete data sanitization by deleting configuration file.
- **Secure Secret Validation:** Base32 character validation for all secret keys.
- **Memory Management:** Proper cleanup and secure data handling.

---

## 🛠️ Technical Foundation
- **Qt Framework:** Built with Qt 6 for robust cross-platform performance.
- **Modular Architecture:** Separated `SecurityManager` and `TwoFAManager` classes.
- **Signal/Slot Communication:** Clean component interaction patterns.
- **QSettings Integration:** Platform-appropriate configuration storage.
- **Resource Management:** Proper RAII principles and memory management.

---

## 📋 System Requirements
- **Operating Systems:** Linux, Windows 10+, macOS 10.14+
- **Dependencies:** zbar-tools (for QR code scanning on Linux)
- **Storage:** Minimal disk space required for configuration files.
- **Memory:** Lightweight application with minimal RAM usage.

---

**Kerveros 1.0.0 - Your Secure 2FA Companion**  
A production-ready application combining security, convenience, and cross-platform reliability.