ls
tar -xpvf flutter_linux_*-stable.tar.xz
export FLUTTER_PATH=$(pwd)/flutter
export PATH="$PATH:$FLUTTER_PATH/bin"
flutter pub get
flutter build linux
install -D dev.mrquantumoff.mcmodpackmanager.metainfo.xml /app/share/metainfo/dev.mrquantumoff.mcmodpackmanager.metainfo.xml
install -D ./assets/icons/logohalf.png /app/share/icons/hicolor/256x256/apps/dev.mrquantumoff.mcmodpackmanager.png
install -D dev.mrquantumoff.mcmodpackmanager.desktop  /app/share/applications/dev.mrquantumoff.mcmodpackmanager.desktop
ls