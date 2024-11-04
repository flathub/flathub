# MPRIS Timer

![image](https://github.com/user-attachments/assets/d4d9445d-0783-4c84-aa9f-eea20ec5e690)

Run:

```shell
go run cmd/main.go -help
```

Build:
```shell
go build -ldflags="-s -w" -o ./.bin/app ./cmd/main.go
```

Flatpak:
```shell
go run github.com/dennwc/flatpak-go-mod@latest .
flatpak-builder --user --force-clean .build tech.efog.mpris-timer.yml
```

## ToDo

1) Customizable presets
2) Preferences dialog
