# Secrets - Password Manager for GNOME

A modern, secure password manager designed for the GNOME desktop environment, built with GTK4 and Libadwaita.

## Features

### 🔐 Secure Password Management
- **Industry-standard encryption** using GPG (GNU Privacy Guard)
- **Password store compatibility** - Built on the proven `pass` standard
- **Local storage** - Your passwords stay on your device
- **Strong password generation** with customizable options

### 🎨 Modern GNOME Integration
- **Native GTK4/Libadwaita design** that follows GNOME HIG
- **Adaptive interface** that works on desktop and mobile
- **Dark mode support** with automatic theme switching
- **Keyboard shortcuts** for power users

### 🔑 Advanced Features
- **TOTP support** - Generate time-based one-time passwords
- **Recovery codes** - Store backup codes securely
- **Folder organization** - Organize passwords in hierarchical folders
- **Search functionality** - Quickly find passwords
- **Git synchronization** - Sync passwords across devices
- **Import/Export** - Migrate from other password managers

### 🛡️ Security First
- **GPG encryption** - Military-grade encryption for all data
- **No cloud dependency** - Works completely offline
- **Secure clipboard** - Automatic clipboard clearing
- **Password visibility controls** - Show/hide passwords as needed

## Installation

### From Flathub (Recommended)

```bash
flatpak install flathub io.github.tobagin.secrets
```

### System Requirements

- **Operating System**: Linux with Flatpak support
- **Desktop Environment**: GNOME 40+ (recommended), other GTK-based DEs supported
- **Architecture**: x86_64 or aarch64
- **Runtime**: GNOME Platform 48

## First-Time Setup

When you first launch Secrets, you'll be guided through a simple setup process:

1. **GPG Key Creation** - Create an encryption key for your passwords
2. **Password Store Initialization** - Set up your local password store

The setup wizard will guide you through each step with clear instructions.

## Usage

### Adding Passwords
- Click the "+" button to add a new password
- Fill in the details: name, username, password, URL, notes
- Optionally add TOTP secrets for two-factor authentication
- Organize passwords in folders for better management

### Managing Passwords
- **View**: Click on any password to see its details
- **Edit**: Use the edit button to modify password information
- **Copy**: Quickly copy passwords or usernames to clipboard
- **Generate**: Use the built-in password generator for strong passwords

### TOTP (Two-Factor Authentication)
- Add TOTP secrets when creating or editing passwords
- View live 6-digit codes with countdown timers
- Store recovery codes for backup access

### Git Synchronization
- Set up Git repositories to sync passwords across devices
- Automatic commit and push/pull operations
- View sync history and status

## Privacy & Security

### Data Storage
- All passwords are encrypted using GPG before storage
- Data is stored locally in `~/.password-store`
- No data is sent to external servers without your explicit action

### Encryption
- Uses GPG (GNU Privacy Guard) for encryption
- Industry-standard AES encryption
- Your GPG key is the only way to decrypt your passwords

### Network Access
- Network access is only used for:
  - Git synchronization (when configured)
  - Checking for dependency updates
- No telemetry or analytics are collected

## Development

### Building from Source

```bash
# Clone the repository
git clone https://github.com/tobagin/Secrets.git
cd Secrets

# Build with Meson
meson setup builddir
meson compile -C builddir

# Run tests
meson test -C builddir

# Install locally
meson install -C builddir
```

### Contributing

Contributions are welcome! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Translations

Help translate Secrets into your language! We use standard gettext for internationalization.

Currently supported languages:
- English (US/UK)
- Portuguese (Brazil/Portugal)
- Spanish
- Irish (Gaeilge)

## Support

### Getting Help
- **Issues**: Report bugs on [GitHub Issues](https://github.com/tobagin/Secrets/issues)
- **Discussions**: Join conversations on [GitHub Discussions](https://github.com/tobagin/Secrets/discussions)
- **Documentation**: Check the [project wiki](https://github.com/tobagin/Secrets/wiki)

### Compatibility
- **Password Store**: Compatible with the standard Unix password store
- **GPG**: Works with existing GPG keys and configurations
- **Git**: Integrates with any Git hosting service (GitHub, GitLab, etc.)

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Built on the excellent [pass](https://www.passwordstore.org/) password manager
- Uses [GPG](https://gnupg.org/) for encryption
- Designed for the [GNOME](https://www.gnome.org/) desktop environment
- Icons and design follow [GNOME Human Interface Guidelines](https://developer.gnome.org/hig/)

---

**Secrets** - Secure, Simple, and Beautiful Password Management for GNOME
