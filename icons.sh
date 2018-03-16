#!/bin/bash
install -dm755 /app/share/icons/hicolor/32x32/status
install -dm755 /app/share/icons/hicolor/scalable/status

# Indicator 32px #

cp /app/share/icons/hicolor/32x32/status/laptopconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.laptopconnected.png
cp /app/share/icons/hicolor/32x32/status/laptopdisconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.laptopdisconnected.png
cp /app/share/icons/hicolor/32x32/status/laptoptrusted.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.laptoptrusted.png
cp /app/share/icons/hicolor/32x32/status/smartphoneconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.smartphoneconnected.png
cp /app/share/icons/hicolor/32x32/status/smartphonedisconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.smartphonedisconnected.png
cp /app/share/icons/hicolor/32x32/status/smartphonetrusted.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.smartphonetrusted.png
cp /app/share/icons/hicolor/32x32/status/tabletconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.tabletconnected.png
cp /app/share/icons/hicolor/32x32/status/tabletdisconnected.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.tablettrusted.png
cp /app/share/icons/hicolor/32x32/status/tablettrusted.png /app/share/icons/hicolor/32x32/status/org.kde.kdeconnect.tablettrusted.png

rm /app/share/icons/hicolor/32x32/status/laptopconnected.png
rm /app/share/icons/hicolor/32x32/status/laptopdisconnected.png
rm /app/share/icons/hicolor/32x32/status/laptoptrusted.png
rm /app/share/icons/hicolor/32x32/status/smartphoneconnected.png
rm /app/share/icons/hicolor/32x32/status/smartphonedisconnected.png
rm /app/share/icons/hicolor/32x32/status/smartphonetrusted.png
rm /app/share/icons/hicolor/32x32/status/tabletconnected.png
rm /app/share/icons/hicolor/32x32/status/tabletdisconnected.png
rm /app/share/icons/hicolor/32x32/status/tablettrusted.png

# Indicator scalable #

cp /app/share/icons/hicolor/scalable/status/laptopconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.laptopconnected.svg
cp /app/share/icons/hicolor/scalable/status/laptopdisconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.laptopdisconnected.svg
cp /app/share/icons/hicolor/scalable/status/laptoptrusted.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.laptoptrusted.svg
cp /app/share/icons/hicolor/scalable/status/smartphoneconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.smartphoneconnected.svg
cp /app/share/icons/hicolor/scalable/status/smartphonedisconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.smartphonedisconnected.svg
cp /app/share/icons/hicolor/scalable/status/smartphonetrusted.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.smartphonetrusted.svg
cp /app/share/icons/hicolor/scalable/status/tabletconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.tabletconnected.svg
cp /app/share/icons/hicolor/scalable/status/tabletdisconnected.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.tablettrusted.svg
cp /app/share/icons/hicolor/scalable/status/tablettrusted.svg /app/share/icons/hicolor/scalable/status/org.kde.kdeconnect.tablettrusted.svg

rm /app/share/icons/hicolor/scalable/status/laptopconnected.svg
rm /app/share/icons/hicolor/scalable/status/laptopdisconnected.svg
rm /app/share/icons/hicolor/scalable/status/laptoptrusted.svg
rm /app/share/icons/hicolor/scalable/status/smartphoneconnected.svg
rm /app/share/icons/hicolor/scalable/status/smartphonedisconnected.svg
rm /app/share/icons/hicolor/scalable/status/smartphonetrusted.svg
rm /app/share/icons/hicolor/scalable/status/tabletconnected.svg
rm /app/share/icons/hicolor/scalable/status/tabletdisconnected.svg
rm /app/share/icons/hicolor/scalable/status/tablettrusted.svg
