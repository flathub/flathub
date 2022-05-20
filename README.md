# Arduino IDE 2.x Flatpak

Flatpak build of [Arduino IDE 2.x](https://github.com/arduino/arduino-ide). To run the
app you need USB permissions, preferably, the user has to be part of the
`dialout` group. Alternatively, add 
``` sh
KERNEL=="ttyUSB[0-9]*",MODE="0666"
KERNEL=="ttyACM[0-9]*",MODE="0666"
```
to `/etc/udev/rules.d/50-arduino.rules`.
