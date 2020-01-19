# xsane-flatpak

**XSane** is an application to scan images using a hardware scanner attached to your computer. It is able to save in a variety of image formats, including TIFF and JPEG and can even save your scan as a PDF. XSane also has support for scanning multiple pages and merging them into a single document.

[Homepage](http://www.sane-project.org/sane-frontends.html)

## FAQ

### Does flatpak-ed XSane run as superuser?

[No](https://github.com/flatpak/flatpak/issues/1557). It is a [MATE](https://github.com/mate-desktop)/[marco](https://github.com/mate-desktop/marco) [issue](https://github.com/mate-desktop/marco/issues/301).

### Why not a RPM package?

There are already classic [packages](https://pkgs.org/download/xsane) for various distributions.
The main purpose of this package is to test and improve scanner support in flatpak packages.

### Why sandboxed XSane doesn't recognize my scanner, while it works flawless on the host?

Because flatpak doesn't have access to `sane` from the host. Currently, we basically need to include each `sane` backend in the flatpak package, which is insane to maintenance. I tried my best, but I am unable to support all [external backends](http://www.sane-project.org/lists/sane-backends-external.html). What's worse, a lot of them require an insane amount of patches. Even if somehow I managed to include them here, I have no way to test them all.

There are some plans to extend [xdg desktop portals](https://github.com/flatpak/xdg-desktop-portal/issues/13), or - to be more specific - add the [scanner portal](https://github.com/flatpak/xdg-desktop-portal/issues/218), so it would provide a sane way to access scanners.
Unfortunately, these are just plans. No one is currently working on it.
By the way, that's one of the main reasons why we don't have the flatpak package for [simple-scan](https://gitlab.gnome.org/GNOME/simple-scan/issues/21) yet.

However, there is a relatively easy way to access scanner via the `net` backend.
First of all, you have to install `saned` (`sane` daemon) on the host.

Instruction for EL7:

```
# yum install sane-backends
```

Instruction for EL8:

```
# yum install sane-backends-daemon
```

Instruction for Fedora:

```
# dnf install sane-backends-daemon
```

Instruction for Debian/Ubuntu:

```
# apt-cache update && apt-get install sane-utils
```

Then, make sure to add `localhost` to the `/etc/sane.d/saned.conf` file:

```
# grep '^[[:blank:]]*localhost[[:blank:]]*$' "/etc/sane.d/saned.conf" || echo 'localhost' >> "/etc/sane.d/saned.conf"
```

Finally, enable and start the `saned` service:

```
# systemctl enable saned.socket
```

```
# systemctl start saned.socket
```

Now you should be able to use your scanner in this flatpak package.

See also:

* [Similar tutorial for Paperwork](https://gitlab.gnome.org/World/OpenPaperwork/paperwork/blob/master/flatpak/README.markdown#quick-start)

### What SANE backends are supported?

Short answer: all default **SANE** backends + `v4l` + `hpaio`.

#### Supported backends

 * [abaton](http://www.sane-project.org/sane-backends.html#S-ABATON)
 * [agfafocus](http://www.sane-project.org/sane-backends.html#S-AGFAFOCUS)
 * [apple](http://www.sane-project.org/sane-backends.html#S-APPLE)
 * [artec](http://www.sane-project.org/sane-backends.html#S-ARTEC)
 * [artec_eplus48u](http://www.sane-project.org/sane-backends.html#S-ARTEC-EPLUS48U)
 * [as6e](http://www.sane-project.org/sane-backends.html#S-AS6E)
 * [avision](http://www.sane-project.org/sane-backends.html#S-AVISION)
 * [bh](http://www.sane-project.org/sane-backends.html#S-BH)
 * [canon](http://www.sane-project.org/sane-backends.html#S-CANON)
 * [canon630u](http://www.sane-project.org/sane-backends.html#S-CANON630U)
 * [canon_dr](http://www.sane-project.org/sane-backends.html#S-CANON-DR)
 * [cardscan](http://www.sane-project.org/sane-backends.html#S-CARDSCAN)
 * [coolscan](http://www.sane-project.org/sane-backends.html#S-COOLSCAN)
 * [coolscan3](http://www.sane-project.org/sane-backends.html#S-COOLSCAN3)
 * [dell1600n_net](http://www.sane-project.org/sane-backends.html#S-DELL1600N-NET)
 * [dmc](http://www.sane-project.org/sane-backends.html#C-DMC)
 * [epjitsu](http://www.sane-project.org/sane-backends.html#S-EPJITSU)
 * [epson2](http://www.sane-project.org/sane-backends.html#S-EPSON2)
 * [epsonds](http://www.sane-project.org/sane-backends.html#S-EPSONDS)
 * [fujitsu](http://www.sane-project.org/sane-backends.html#S-FUJITSU)
 * [genesys](http://www.sane-project.org/sane-backends.html#S-GENESYS)
 * [gt68xx](http://www.sane-project.org/sane-backends.html#S-GT68XX)
 * [hp](http://www.sane-project.org/sane-backends.html#S-HP)
 * [hp3500](http://www.sane-project.org/sane-backends.html#S-HP3500)
 * [hp3900](http://www.sane-project.org/sane-backends.html#S-HP3900)
 * [hp4200](http://www.sane-project.org/sane-backends.html#S-HP4200)
 * [hp5400](http://www.sane-project.org/sane-backends.html#S-HP5400)
 * [hp5590](http://www.sane-project.org/sane-backends.html#S-HP5590)
 * [hpaio](http://www.sane-project.org/lists/sane-backends-external.html#S-HPAIO)
 * [hpljm1005](http://www.sane-project.org/sane-backends.html#S-HPLJM1005)
 * [hpsj5s](http://www.sane-project.org/sane-backends.html#S-HPSJ5S)
 * [hs2p](http://www.sane-project.org/sane-backends.html#S-HS2P)
 * [ibm](http://www.sane-project.org/sane-backends.html#S-IBM)
 * [kodak](http://www.sane-project.org/sane-backends.html#S-KODAK)
 * [kodakaio](http://www.sane-project.org/sane-backends.html#S-KODAKAIO)
 * [kvs1025](http://www.sane-project.org/sane-backends.html#S-KVS1025)
 * [kvs20xx](http://www.sane-project.org/sane-backends.html#S-KVS20XX)
 * [leo](http://www.sane-project.org/sane-backends.html#S-LEO)
 * [lexmark](http://www.sane-project.org/sane-backends.html#S-LEXMARK)
 * [ma1509](http://www.sane-project.org/sane-backends.html#S-MA1509)
 * [magicolor](http://www.sane-project.org/sane-backends.html#S-MAGICOLOR)
 * [matsushita](http://www.sane-project.org/sane-backends.html#S-MATSUSHITA)
 * [microtek](http://www.sane-project.org/sane-backends.html#S-MICROTEK)
 * [microtek2](http://www.sane-project.org/sane-backends.html#S-MICROTEK2)
 * [mustek](http://www.sane-project.org/sane-backends.html#S-MUSTEK)
 * [mustek_usb](http://www.sane-project.org/sane-backends.html#S-MUSTEK-USB)
 * [mustek_usb2](http://www.sane-project.org/sane-backends.html#S-MUSTEK-USB2)
 * [nec](http://www.sane-project.org/sane-backends.html#S-NEC)
 * [niash](http://www.sane-project.org/sane-backends.html#S-NIASH)
 * [pie](http://www.sane-project.org/sane-backends.html#S-PIE)
 * [pint](http://www.sane-project.org/sane-backends.html#A-PINT)
 * [pixma](http://www.sane-project.org/sane-backends.html#S-PIXMA)
 * [plustek](http://www.sane-project.org/sane-backends.html#S-PLUSTEK)
 * [qcam](http://www.sane-project.org/sane-backends.html#V-QCAM)
 * [ricoh](http://www.sane-project.org/sane-backends.html#S-RICOH)
 * [ricoh2](http://www.sane-project.org/sane-backends.html#S-RICOH2)
 * [rts8891](http://www.sane-project.org/sane-backends.html#S-RTS8891)
 * [s9036](http://www.sane-project.org/sane-backends.html#S-S9036)
 * [sceptre](http://www.sane-project.org/sane-backends.html#S-SCEPTRE)
 * [sharp](http://www.sane-project.org/sane-backends.html#S-SHARP)
 * [sm3600](http://www.sane-project.org/sane-backends.html#S-SM3600)
 * [sm3840](http://www.sane-project.org/sane-backends.html#S-SM3840)
 * [snapscan](http://www.sane-project.org/sane-backends.html#S-SNAPSCAN)
 * [sp15c](http://www.sane-project.org/sane-backends.html#S-SP15C)
 * [tamarack](http://www.sane-project.org/sane-backends.html#S-TAMARACK)
 * [teco1](http://www.sane-project.org/sane-backends.html#S-TECO1)
 * [teco2](http://www.sane-project.org/sane-backends.html#S-TECO2)
 * [teco3](http://www.sane-project.org/sane-backends.html#S-TECO3)
 * [u12](http://www.sane-project.org/sane-backends.html#S-U12)
 * [umax](http://www.sane-project.org/sane-backends.html#S-UMAX)
 * [umax1220u](http://www.sane-project.org/sane-backends.html#S-UMAX1220U)
 * [v4l](http://www.sane-project.org/sane-backends.html#A-V4L)
 * [xerox_mfp](http://www.sane-project.org/sane-backends.html#S-XEROX-MFP)

#### Special backends

 * [dll](http://www.sane-project.org/sane-backends.html#M-DLL)
 * [net](http://www.sane-project.org/sane-backends.html#M-NET)

#### Disabled backends

 * [canon_pp](http://www.sane-project.org/sane-backends.html#S-CANON-PP)
 * [coolscan2](http://www.sane-project.org/sane-backends.html#S-COOLSCAN2)
 * [dc25](http://www.sane-project.org/sane-backends.html#C-DC25)
 * [dc210](http://www.sane-project.org/sane-backends.html#C-DC210)
 * [dc240](http://www.sane-project.org/sane-backends.html#C-DC240)
 * [epson](http://www.sane-project.org/sane-backends.html#S-EPSON)
 * [gphoto2](http://www.sane-project.org/sane-backends.html#A-GPHOTO2)
 * [mustek_pp](http://www.sane-project.org/sane-backends.html#S-MUSTEK-PP)
 * [p5](http://www.sane-project.org/sane-backends.html#S-P5)
 * [plustek_pp](http://www.sane-project.org/sane-backends.html#S-PLUSTEK-PP)
 * [pnm](http://www.sane-project.org/sane-backends.html#A-PNM)
 * [st400](http://www.sane-project.org/sane-backends.html#S-ST400)
 * [stv680](http://www.sane-project.org/sane-backends.html#V-STV680)
 * [test](http://www.sane-project.org/sane-backends.html#A-TEST)
 * [umax_pp](http://www.sane-project.org/sane-backends.html#S-UMAX-PP)

#### Unsupported backends

 * [brother](http://www.sane-project.org/lists/sane-backends-external.html#S-BROTHER)
 * [brother-mfc4600](http://www.sane-project.org/lists/sane-backends-external.html#S-BROTHER-MFC4600)
 * [brother2](http://www.sane-project.org/lists/sane-backends-external.html#S-BROTHER2)
 * [canon_pixma](http://www.sane-project.org/lists/sane-backends-external.html#S-CANON-PIXMA)
 * [cs3200f](http://www.sane-project.org/lists/sane-backends-external.html#S-CS3200F)
 * [epkowa](http://www.sane-project.org/lists/sane-backends-external.html#S-EPKOWA)
 * [geniusvp2](http://www.sane-project.org/lists/sane-backends-external.html#S-GENIUSVP2)
 * [hp3770](http://www.sane-project.org/lists/sane-backends-external.html#S-HP3770)
 * [hp8200](http://www.sane-project.org/lists/sane-backends-external.html#S-HP8200)
 * [hpoj](http://www.sane-project.org/lists/sane-backends-external.html#S-HPOJ)
 * [kodak-twain](http://www.sane-project.org/lists/sane-backends-external.html#S-KODAK-TWAIN)
 * [kvs40xx](http://www.sane-project.org/sane-backends.html#S-KVS40XX)
 * [lhii](http://www.sane-project.org/lists/sane-backends-external.html#S-LHII)
 * [mustek_a3p1](http://www.sane-project.org/lists/sane-backends-external.html#S-MUSTEK-A3P1)
 * [panamfs](http://www.sane-project.org/lists/sane-backends-external.html#S-PANAMFS)
 * [primascan](http://www.sane-project.org/lists/sane-backends-external.html#S-PRIMASCAN)
 * [primax](http://www.sane-project.org/lists/sane-backends-external.html#S-PRIMAX)
 * [samsung](http://www.sane-project.org/lists/sane-backends-external.html#S-SAMSUNG)
 * [scanwit](http://www.sane-project.org/lists/sane-backends-external.html#S-SCANWIT)
 * [utsushi](http://www.sane-project.org/lists/sane-backends-external.html#S-UTSUSHI)
 * [v4l2](http://www.sane-project.org/lists/sane-backends-external.html#A-V4L2)
 * [viceo](http://www.sane-project.org/lists/sane-backends-external.html#S-VICEO)

Please keep in mind that you can always try to use the `net` backend to access scanners available on the host.

