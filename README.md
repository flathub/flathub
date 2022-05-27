# Arduino IDE 2.x Flatpak

Flatpak build of [Arduino IDE 2.x](https://github.com/arduino/arduino-ide). To run the
app you need USB permissions, preferably, the user has to be part of the
`dialout` group. Alternatively, add 
``` sh
KERNEL=="ttyUSB[0-9]*",MODE="0666"
KERNEL=="ttyACM[0-9]*",MODE="0666"
```
to `/etc/udev/rules.d/50-arduino.rules`.

## Running from console
If you are running the application from the console (you might need to do this to view the application's log while it is being used), run it using the `flatpak run` command:
``` sh
flatpak run cc.arduino.IDE2
```
Any arguments passed to the flatpak run command will be passed to the IDE i.e.
``` sh
flatpak run cc.arduino.IDE2 --log-level=warn
```
which only displays warning or above in the log output to the console.
