[<img src="https://teapot.informationsanarchistik.de/Megamichi/BerryPaper/raw/branch/main/berrypaper/assets/logo.svg">](Logo)
# BerryPaper
![Static Badge](https://img.shields.io/badge/version-0.1.1-blue)
![Static Badge](https://img.shields.io/badge/status-beta-orange)

A unofficial Client to download Raspberry Pi magazines

BerryPaper lets you download and read official Raspberry Pi books and magazines. It provides a simple and lightweight interface to browse available content and manage your downloads efficiently.

All content is available in PDF format, allowing you to read your magazines offline at any time. BerryPaper also keeps track of your downloaded files.

## Features:
- offline caching
- wonderfull GUI
- CLI interface (coming soon)

## Dependencys:
```
sudo apt install python3-gi gir1.2-gtk-3.0 python3-setuptool
pip3 install -r requirements.txt
```
## Installation:
```
git clone https://teapot.informationsanarchistik.de/Megamichi/BerryPaper.git
cd BerryPaper
```
> ### Running without installation:
> ```
> python3 -m berrypaper 
> ```
But if you want you can install BerryPaper using pip or better pipx:
```
pipx install --system-site-packages .
# or 
pip install .
```
## For uninstalling
```
pipx uninstall berrypaper
# or:
pip uninstall berrypaper
# and after removing the package:
python3 cleanup.py
```
> cleanup.py deletes everything from BerryPaper, so backup your downloaded Magazins from `~/.local/share/berrypaper/`