# 🔐 Kerveros

**Secure 2FA Code Generator with Master Password Protection**

Kerveros is a robust, cross-platform two-factor authentication (2FA) code generator that combines enterprise-grade security with user-friendly features. Generate time-based one-time passwords (TOTP) for all your online accounts while keeping your secrets encrypted and protected.

![Qt](https://img.shields.io/badge/Qt-6.0%2B-green.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-blue.svg)
![License](https://img.shields.io/badge/License-GPLv3-blue.svg)

## ✨ Features

### 🔒 Core Security
- **TOTP Code Generation** - RFC-compliant time-based one-time passwords
- **Master Password Protection** - SHA-256 hashing with salting
- **Encrypted Secret Storage** - XOR encryption with initialization vectors
- **Password Attempt Limiting** - 5 attempts before factory reset option
- **Single Instance Enforcement** - Prevents multiple app conflicts

### 🎯 User Experience
- **Real-time Code Updates** - Automatic refresh every 30 seconds
- **Visual Countdown Timer** - Color-coded progress bar (green/yellow/red)
- **One-Click Copy** - Instant clipboard copy with visual feedback
- **QR Code Import** - Drag & drop or file menu import of QR code images
- **System Tray Integration** - Minimize to tray with quick restore

### 📱 Account Management
- **Unlimited Accounts** - Support for GitHub, Google, PayPal, Discord, etc.
- **Easy Setup** - Manual entry or QR code scanning
- **Export/Import** - Full backup and restore functionality
- **Duplicate Prevention** - Automatic validation and conflict detection

### 🌐 Cross-Platform
- **Native Performance** - Built with Qt Framework
- **Consistent Experience** - Same features on all platforms
- **Platform Integration** - Uses native settings storage

## 🚀 Quick Start

### Installation

#### Linux
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install zbar-tools

# Build from source
git clone https://github.com/alamahant/kerveros.git
cd kerveros
mkdir build && cd build
cmake ..
make -j$(nproc)
./kerveros
