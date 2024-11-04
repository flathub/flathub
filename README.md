# MPRIS Timer

![image](https://github.com/user-attachments/assets/d4d9445d-0783-4c84-aa9f-eea20ec5e690)
![image](https://github.com/user-attachments/assets/80c40dee-1a2f-4729-8f9b-89e5eeb934b9)

>MPRIS Timer is very keyboard friendly! It should be very intuitive. \
>As soon as you see the the UI, try to do some navigation or even just input some numbers.

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
flatpak-builder --user --force-clean .build io.github.efogdev.mpris-timer.yml
```

## ToDo

1) Customizable presets
2) Preferences dialog
