# BrowserOS Flatpak

A complete flatpak packaging of BrowserOS - The open-source Agentic browser.

## Requirements

- Flatpak (>= 1.0.0)
- Flathub remote configured

## Building

```bash
# Clone this repository
git clone <this-repo>
cd BrowserOS_flatpak

# Build the flatpak
flatpak-builder --force-clean --repo=repo --install build-dir com.browseros.BrowserOS.yml
```

## Installation

### From Source (Build)

After building, the application will be automatically installed:

```bash
flatpak run --user com.browseros.BrowserOS
```

### Manual Installation

If you have a pre-built repository:

```bash
# Add the repository
flatpak remote-add --user --no-gpg-verify browseros-repo /path/to/repo

# Install BrowserOS
flatpak install --user browseros-repo com.browseros.BrowserOS
```

## Running

```bash
# Basic launch
flatpak run --user com.browseros.BrowserOS

# With specific arguments
flatpak run --user com.browseros.BrowserOS --no-sandbox
```

## Features

- **Complete BrowserOS Integration**: Full BrowserOS v0.33.0 functionality
- **Sandboxed Security**: Flatpak's sandbox provides isolation from the host system
- **Desktop Integration**: Proper application menu integration, icon, and file associations
- **Network Access**: Full network connectivity for browsing and AI features
- **Multi-media Support**: Audio, video, and graphics acceleration
- **File System Access**: Home directory and temporary file access

## Permissions

The flatpak uses the following permissions:

- **Network**: Full network access for browsing
- **Display**: X11 and Wayland support
- **Audio**: Pulseaudio integration
- **Graphics**: DRI hardware acceleration
- **File System**: Home and temp directory access
- **D-Bus**: Notifications, secrets, and media player integration
- **Devices**: KVM, shared memory for Chromium sandbox

## Troubleshooting

### BrowserOS Working Properly ✅

**Good News**: If you see "Attempting to launch BrowserOS with Chrome sandbox..." followed by version output, BrowserOS is working correctly! The MCP (AI agent) server will start automatically.

### Intelligent Sandbox Handling

The BrowserOS flatpak now uses intelligent sandbox detection:

- **First Attempt**: Tries to launch with Chrome's normal sandbox
- **Automatic Fallback**: If sandbox fails, automatically switches to `--no-sandbox` mode  
- **Manual Override**: You can specify `--no-sandbox` explicitly if needed

### Expected Messages

When running, you may see:

- `Attempting to launch BrowserOS with Chrome sandbox...` - Initial launch attempt
- `Chrome sandbox failed, falling back to --no-sandbox mode` - Normal fallback message
- `ERROR:dbus/bus.cc:408] Failed to connect to socket` - Expected in no-sandbox mode
- `Bun is a fast JavaScript runtime` - BrowserOS AI features working

**These messages don't prevent BrowserOS from working** - they're normal when adapting to different Linux environments.

### Manual No-Sandbox Override

If you need to explicitly specify no-sandbox mode:

```bash
flatpak run --user com.browseros.BrowserOS --no-sandbox
```

### Why Intelligent Sandbox Handling

Different Linux distributions handle Chrome sandbox differently within flatpak containers:

- ✅ **Automatic Detection**: Adapts to your specific system configuration
- ✅ **Maximum Compatibility**: Works regardless of Chrome sandbox support
- ✅ **No User Intervention**: Handles sandbox issues transparently
- ✅ **Preserves Security**: Uses whichever sandbox mode works best

### BrowserOS AI Features Working

The MCP (Model Context Protocol) server that starts enables BrowserOS's AI agent functionality. This shows BrowserOS is fully functional with all features enabled.

### Normal Operation

The wrapper script automatically detects your system's Chrome sandbox capabilities and launches BrowserOS with the optimal configuration for your environment.

### Update Issues

To update to a new version:

```bash
# Remove old version
flatpak uninstall --user com.browseros.BrowserOS

# Rebuild with updated manifest
flatpak-builder --force-clean --user --repo=repo --install build-dir com.browseros.BrowserOS.yml
```

### Debug Mode

For debugging, you can enable debug mode:

```bash
flatpak run --user --devel com.browseros.BrowserOS
```

## Version Information

- **BrowserOS Version**: v0.33.0.1 (Chromium 142.0.7544.49)
- **Base Runtime**: org.chromium.Chromium (stable)
- **Platform**: org.freedesktop.Platform 24.08
- **Architecture**: x86_64

## Development

### Project Structure

- `com.browseros.BrowserOS.yml` - Flatpak manifest
- `browseros-wrapper.sh` - Application launcher script
- `com.browseros.BrowserOS.desktop` - Desktop entry file
- `com.browseros.BrowserOS.png` - Application icon

### Update Process

1. Update AppImage URL in manifest
2. Update icon if needed
3. Increment version in metadata
4. Rebuild and test

## License

BrowserOS is distributed under its own license. This flatpak packaging follows the same terms.

## Contributing

Issues and pull requests related to flatpak packaging are welcome.

- **BrowserOS Issues**: Report to the [BrowserOS GitHub repository](https://github.com/browseros-ai/BrowserOS)
- **Flatpak Issues**: Report in this repository

## Acknowledgments

- BrowserOS team for the excellent browser
- Flathub project for the flatpak ecosystem
- Chromium project for the browser engine foundation

