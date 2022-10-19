Epsonscan2
==========

Epson semi-libre scanning software for use with Epson scanners with
proprietary drivers, and others.

This flatpak is NOT official.

See:

http://download.ebz.epson.net/dsc/du/02/DriverDownloadInfo.do?LG2=EN&CN2=&DSCMI=139759&DSCCHK=f0627fdbbe9bd5f9b293c509c82d389fcd5f846c

Source download
http://support.epson.net/linux/src/scanner/epsonscan2/

Content
-------

epsonscan2-non-free: the non-free plugins, drivers, firmware. x86_64
only (there is an armv7 too, but this arch is non longer supported by
the SDK). Required for many scanners, including Epson V600 This is
extracted from the debian packages provided by Epson.

epsonscan2: the Qt based front-end, build from source. LGPL-2.1

Build
-----

### Non x86_64

Given the nature of the package and the binaries not being available,
this is restricted to `x86_64`.

### Finish args

Wayland cause the app to bail out.

Network isn't enabled. Might be needed for networks scanning.

Filesystem locations are the default offered in the app: xdg-pictures,
xdg-documents, xdg-desktop.

Persist `.epsonscan2` because that's where non XDG complient is stores
the settings.

### Build options

`-DCMAKE_INSTALL_LIBDIR=lib/x86_64-linux-gnu` is used because of where
the non-free stuff is (extracted from the Debian package) and
hardcoded in the source code. Note: this is arch specific. But it doesn't
matter here.

If using the RPM, it would be `-DCMAKE_INSTALL_LIBDIR=lib64` (64-bits
archs only).

### Binary patching

`epson_patch.py` is meant to patch a binary that expect to load DSO
from `/usr`. This cause the scanner to not be detected.

### Patches

`epsonscan2-build.patch`: some build fixes where `/usr` is hardcoded.

`epsonscan2-crash.patch`: fix a crash during device detection.

`epsonscan2-xdg-open.patch`: use xdg-open to open directories instead
of file managers (not in the sandbox)

`escan2_app-48.png` and `escan2_app-256.png`: 48 and 256 pixel icons
extracted from the vendor .ico found in the source..

screenshots: unfortunately the vendor doesn't provide screen, so
they'll be hosted here.

### Known issues

epsonscan2 expect `killall`. It's not in the runtime.
It also expects `avahi-browse`. It's not built. (TODO)

Scanner Supported
-----------------

DS-1610, DS-1630, DS-1660W, DS-30000, DS-310, DS-31100, DS-31200,
DS-320, DS-32000, DS-360W, DS-40, DS-410, DS-50000, DS-510, DS-520,
DS-530, DS-530II, DS-531, DS-535, DS-535H, DS-535II, DS-5500, DS-560,
DS-570W, DS-570WII, DS-571W, DS-575W, DS-575WII, DS-60000, DS-6500,
DS-70, DS-70000, DS-730N, DS-7500, DS-760, DS-770, DS-770II, DS-775,
DS-775II, DS-780N, DS-790WN, DS-80W, DS-860, DS-870, DS-875, DS-970,
DS-975, DS-G20000, EC-4020 Series, EC-4030 Series, EC-4040 Series,
EC-C7000 Series, EP-10VA Series, EP-30VA Series, EP-708A Series,
EP-709A Series, EP-710A Series, EP-711A Series, EP-712A Series,
EP-713A Series, EP-714A Series, EP-715A Series, EP-808A Series,
EP-810A Series, EP-811A Series, EP-812A Series, EP-813A Series,
EP-814A Series, EP-815A Series, EP-879A Series, EP-880A Series,
EP-881A Series, EP-882A Series, EP-883A Series, EP-884A Series,
EP-885A Series, EP-978A3 Series, EP-979A3 Series, EP-982A3 Series,
EP-M552T Series, EP-M553T Series, EP-M570T Series, ES-200, ES-300W,
ES-300WR, ES-400, ES-400II, ES-50, ES-500W, ES-500WII, ES-500WR,
ES-55R, ES-580W, ES-60W, ES-60WB, ES-60WW, ES-65WR, ES-865, ET-15000
Series, ET-16500 Series, ET-16600 Series, ET-16650 Series, ET-16680
Series, ET-2400 Series, ET-2500 Series, ET-2550 Series, ET-2600
Series, ET-2610 Series, ET-2650 Series, ET-2700 Series, ET-2710
Series, ET-2720 Series, ET-2750 Series, ET-2760 Series, ET-2800
Series, ET-2810 Series, ET-2820 Series, ET-2850 Series, ET-3600
Series, ET-3700 Series, ET-3710 Series, ET-3750 Series, ET-3760
Series, ET-3800 Series, ET-3830 Series, ET-3840 Series, ET-3850
Series, ET-4500 Series, ET-4550 Series, ET-4700 Series, ET-4750
Series, ET-4760 Series, ET-4800 Series, ET-4850 Series, ET-5150
Series, ET-5170 Series, ET-5180 Series, ET-5800 Series, ET-5850
Series, ET-5880 Series, ET-7700 Series, ET-7750 Series, ET-8500
Series, ET-8550 Series, ET-8700 Series, ET-M16600 Series, ET-M16680
Series, ET-M2120 Series, ET-M2140 Series, ET-M2170 Series, ET-M3140
Series, ET-M3170 Series, ET-M3180 Series, EW-052A Series, EW-452A
Series, EW-M5071FT Series, EW-M5610FT Series, EW-M571T Series,
EW-M630T Series, EW-M634T Series, EW-M660FT Series, EW-M670FT Series,
EW-M674FT Series, EW-M752T Series, EW-M754T Series, EW-M770T Series,
EW-M873T Series, EW-M970A3T Series, EW-M973A3T Series, Expression
12000XL, FF-640, FF-680W, GT-F730, GT-F740, GT-S630, GT-S640, GT-S650,
GT-X820, GT-X980, L14150 Series, L1455 Series, L15150 Series, L15160
Series, L15180 Series, L3050 Series, L3060 Series, L3070 Series, L3100
Series, L3110 Series, L3150 Series, L3160 Series, L3200 Series, L3210
Series, L3250 Series, L3260 Series, L375 Series, L380 Series, L382
Series, L385 Series, L386 Series, L395 Series, L396 Series, L405
Series, L4150 Series, L4160 Series, L4260 Series, L455 Series, L456
Series, L475 Series, L485 Series, L486 Series, L495 Series, L5190
Series, L5290 Series, L565 Series, L566 Series, L575 Series, L605
Series, L6160 Series, L6170 Series, L6190 Series, L6260 Series, L6270
Series, L6290 Series, L6460 Series, L6490 Series, L655 Series, L6550
Series, L6570 Series, L6580 Series, L7160 Series, L7180 Series, L8160
Series, L8180 Series, L850 Series, LP-M8180A, LP-M8180F, LX-10000F,
LX-10000FK, LX-10010MF, LX-10020M, LX-10050KF, LX-10050MF, LX-6050MF,
LX-7000F, LX-7550MF, M15140 Series, M15180 Series, M200 Series, M205
Series, M2110 Series, M2120 Series, M2140 Series, M2170 Series, M3140
Series, M3170 Series, M3180 Series, PX-048A Series, PX-049A Series,
PX-M160T Series, PX-M270FT Series, PX-M270T Series, PX-M350F,
PX-M380F, PX-M381FL, PX-M5040F, PX-M5041F, PX-M5080F Series, PX-M5081F
Series, PX-M6712FT Series, PX-M680F Series, PX-M7050 Series,
PX-M7050FP, PX-M7050FX, PX-M7070FX, PX-M7080FX, PX-M7090FX, PX-M7110F,
PX-M7110FP, PX-M740F, PX-M741F, PX-M780F Series, PX-M781F Series,
PX-M791FT Series, PX-M840F, PX-M840FX, PX-M860F, PX-M880FX, PX-M884F,
PX-M885F, PX-M886FL, Perfection V19, Perfection V33, Perfection V330
Photo, Perfection V37, Perfection V370, Perfection V39, Perfection
V550, Perfection V600 photo, Perfection V800 Photo, Perfection V850
Pro, RR-600W, ST-2000 Series, ST-3000 Series, ST-4000 Series, ST-C2100
Series, ST-C4100 Series, ST-C8000 Series, ST-C8090 Series, ST-M3000
Series, WF-2750 Series, WF-2760 Series, WF-2810 Series, WF-2820
Series, WF-2830 Series, WF-2840 Series, WF-2850 Series, WF-2860
Series, WF-2870 Series, WF-2880 Series, WF-2910 Series, WF-2930
Series, WF-2950 Series, WF-2960 Series, WF-3620 Series, WF-3640
Series, WF-3720 Series, WF-3730 Series, WF-4720 Series, WF-4730
Series, WF-4740 Series, WF-5620 Series, WF-5690 Series, WF-6530
Series, WF-6590 Series, WF-7610 Series, WF-7620 Series, WF-7710
Series, WF-7720 Series, WF-8510 Series, WF-8590 Series, WF-C17590
Series, WF-C20590 Series, WF-C20600 Series, WF-C20750 Series,
WF-C21000 Series, WF-C4810 Series, WF-C5710 Series, WF-C5790 Series,
WF-C5790BA, WF-C5790BAM, WF-C579R Series, WF-C579RB, WF-C579RBAM,
WF-C5890 Series, WF-C5890BAM, WF-C8610 Series, WF-C8690 Series,
WF-C8690B, WF-C869R Series, WF-C878R Series, WF-C878RB, WF-C879R
Series, WF-C879RB, WF-M20590 Series, WF-M21000 Series, WF-M5690
Series, WF-M5799 Series, WF-R8590 Series, XP-2100 Series, XP-2150
Series, XP-220 Series, XP-2200 Series, XP-230 Series, XP-235 Series,
XP-240 Series, XP-243 245 247 Series, XP-255 257 Series, XP-3100
Series, XP-3150 Series, XP-3200 Series, XP-332 335 Series, XP-340
Series, XP-342 343 345 Series, XP-352 355 Series, XP-4100 Series,
XP-4150 Series, XP-4200 Series, XP-430 Series, XP-432 435 Series,
XP-440 Series, XP-442 445 Series, XP-452 455 Series, XP-5100 Series,
XP-5150 Series, XP-5200 Series, XP-530 Series, XP-540 Series, XP-6000
Series, XP-6100 Series, XP-630 Series, XP-640 Series, XP-7100 Series,
XP-830 Series, XP-8500 Series, XP-8600 Series, XP-8700 Series, XP-900
Series, XP-960 Series, XP-970 Series
