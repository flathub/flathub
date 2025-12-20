*** Due to a busy schedule for the holidays this project will be on a temporary hold until January. I will try to work on it when I have some free time.


# Linux Armoury (Rust Edition)

> **⚠️ MAJOR UPDATE**: Linux Armoury is currently being rewritten from Python to **Rust** for improved performance, stability, and resource efficiency. The legacy Python implementation has been removed.

Linux Armoury is a powerful monitoring and control utility for ASUS ROG laptops on Linux. It provides a native, high-performance interface to control power profiles, fan curves, keyboard lighting, and monitoring hardware stats.

## 🚀 Features (Rust Backend)

The new Rust daemon (`linux-armoury-daemon`) exposes a comprehensive DBus API providing the following controls:

### ⚡ Power & Performance
*   **Power Profiles**: Switch between `Balanced`, `Performance`, and `Quiet` modes.
*   **CPU Turbo**: Toggle CPU Turbo Boost on/off.
*   **CPU Governor**: Set CPU scaling governor (e.g., `performance`, `powersave`, `schedutil`).
*   **EPP (Energy Performance Preference)**: Fine-tune energy vs. performance bias.
*   **GPU Performance**: Control GPU power levels (e.g., `auto`, `low`, `high`).
*   **TDP Control**: Manual control over STAPM, FAST, and SLOW power limits (requires `ryzenadj`).

### 🌡️ Thermal Control
*   **Custom Fan Curves**: Enable or disable custom software-controlled fan curves.
*   **Fan Curve Editor**: Write specific temperature/speed points for CPU and GPU fans.

### 🔋 Battery Health
*   **Charge Limiting**: Set a maximum battery charge percentage (0-100%) to prolong battery life.

### 💡 Lighting & Input
*   **Keyboard Brightness**: Adjust keyboard backlight intensity (0-3).
*   **RGB Control**: Set static RGB colors.
*   **Lighting Effects**: Apply built-in ASUS Aura effects:
    *   Static, Breathe, Color Cycle, Rainbow
    *   Star, Rain, Highlight, Laser, Ripple
    *   Strobe, Comet, Flash, MultiStatic

### 📊 Monitoring
*   **Real-time Stats**: CPU temperature and AC power connection status.

## 🛠️ Installation & Building

### Prerequisites
*   **Rust (Cargo)**: Required to build the daemon and GUI.
*   **DBus development headers**: `libdbus-1-dev` (Ubuntu/Debian) or `dbus-devel` (Fedora).
*   **GTK4 / Iced dependencies**: Libraries required for the GUI.

### Building the Daemon
```bash
cd daemon
cargo build --release
```

### Building the GUI
```bash
cd gui
cargo build --release
```

### Running (Development)
1.  **Start the Daemon**:
    ```bash
    sudo ./daemon/target/release/linux-armoury-daemon
    ```
2.  **Start the GUI**:
    ```bash
    ./gui/target/release/linux-armoury-gui
    ```

## 🤝 Contributing
Contributions are welcome! Please check the `docs/` folder for architectural details and the `daemon/` directory for the backend implementation.

## 📜 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.