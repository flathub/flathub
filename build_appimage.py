#!/usr/bin/python3
import os

def get_version():
    with open("Cargo.toml") as fh:
        for line in fh:
            if line.startswith("version"):
                return line.replace("version", "").replace("=", "").replace('"', '').strip()
    return ''

if __name__ == '__main__':
    # check version
    version = get_version()
    os.chdir("appimage")
    os.system("sed -i 's/^Version=.*/Version=%s/g' hoptodesk.desktop" % version)
    os.system("sed -i 's/^    version: .*/    version: %s/g' AppImageBuilder.yml" % version)
    # build appimage
    ret = os.system("appimage-builder --recipe AppImageBuilder.yml --skip-test")
    if ret == 0:
        print("HopToDesk AppImage build success :)")
        print("Check AppImage in '/path/to/hoptodesk/appimage/HopToDesk-VERSION-TARGET_PLATFORM.AppImage'")
    else:
        print("HopToDesk AppImage build failed")
