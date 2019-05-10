## File locations
The files created by mprime are saved under
`/home/<user>/.var/app/org.mersenne.mprime/data/`, where `<user>` is your
username.

# Running as a daemon
## with systemd:
__WARNING: Currently with this daemon implementation, there's no way to gracefully
stop mprime. Stopping this daemon will result in some work
lost (before the last checkpoint).__

Create a file at `/etc/systemd/system/mprime.service` with the following content:
```
[Unit]
Description=Mersenne Primality Tester

[Service]
Type=simple
User=<user>
ExecStart=sh -c 'flatpak run --command=mprime-daemon org.mersenne.mprime 2>&1'
ExecStop=flatpak kill org.mersenne.mprime
KillMode=process
KillSignal=SIGTERM

[Install]
WantedBy=multi-user.target
```
Replace `<user>` with your username.  
Before activating mprime.service, you should run mprime at least once
to set your preferences!


##### Start the service automatically at boot:
```
systemctl enable mprime.service
```

##### Activate the service with:
```
systemctl start mprime.service
```

##### Stop it with:
```
systemctl stop mprime.service
```

##### Look at daemon logs:
```
journalctl -fu mprime
```
