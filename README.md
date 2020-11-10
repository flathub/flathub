# TL;DR

You need Docker and Flatpak installed in your computer

For only the first time:

```
docker run -d --name=iked --net=host --privileged -v /etc/resolv.conf:/etc/resolv.conf -v /run:/run beardoverflow/ike
```

And later:

```
flatpak run net.shrew.ike.qikea
```

**Warning**: iked must be running before starting. If not, qikea will not work

# ike

## What is ike?

A collection of tools developed by Shrew Soft, Inc. to communicate with Open Source VPN servers (e.g. ipsec-tools) as well as some commercial VPN servers

**Disclaimer**: This image is not verified by, affiliated with, or supported by Shrew Soft, Inc.

| Tool | Description | Download |
| - | - | - |
| iked | Daemon which manages  tun interfaces (*the real vpn client*) | https://hub.docker.com/r/beardoverflow/ike |
| ikec | Command-line client to talk to the daemon | https://hub.docker.com/r/beardoverflow/ike |
| qikea | Graphical interface to talk to the daemon (*me*) | https://flathub.org/apps/details/net.shrew.ike.qikea |

## What can I find here?

qikea, a frontend for iked daemon packaged for Flatpak. You can find more information at https://hub.docker.com/r/beardoverflow/ike