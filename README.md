# 移动云电脑

`移动云电脑` is a paid cloud desktop client software provided by China Mobile, supporting remote access to cloud virtual desktops.

`移动云电脑`是中国移动提供的付费云桌面客户端软件，支持远程访问云虚拟桌面。

## About This Repository

This application itself is **proprietary software** (closed source).

This Flatpak package is **not** verified, affiliated with, or supported by China Mobile (Hangzhou) Information Technology Co., Ltd.

## Build Instructions

This Flatpak package is built based on the [official UOS (Tongxin) DEB package](https://soho.komect.com/clientDownload).

此 Flatpak 包基于[官方 UOS（统信）DEB 包](https://soho.komect.com/clientDownload)构建。

### Architecture Support / 架构支持

- x86_64 (AMD64)
- aarch64 (ARM64) - Not verified. If you can successfully install and use it on ARM devices, please submit a PR to remove this note.

### Known Limitations

1. After disconnecting, the main interface remains in full-screen loading mode. Temporary solution: Force restart the app.

2. USB device forwarding and printer forwarding not supported.

### Credits

This repository is modified based on https://github.com/flathub/com.qq.QQ
