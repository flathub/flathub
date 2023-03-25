# ProtonVPN

High-speed Swiss VPN that safeguards your privacy.

## How do I automatically connect to the VPN server I connected to last time?

1. Create `~/.config/systemd/user/protonvpn_reconnect.service` with the following content:

```
[Unit]
Wants=network-online.target
After=network-online.target

[Install]
WantedBy=default.target

[Service]
Restart=always
RestartSec=5
ExecStart=flatpak run --command=/usr/bin/python3 com.protonvpn.www /app/lib/python3.10/site-packages/protonvpn_nm_lib/daemon/dbus_daemon_reconnector.py
```

2. Run `systemctl --user daemon-reload`
3. Run `systemctl --user enable --now protonvpn_reconnect`