ls
install -D dev.mrquantumoff.mcmodpackmanager.metainfo.xml /app/share/metainfo/dev.mrquantumoff.mcmodpackmanager.metainfo.xml
install -D logohalf.png /app/share/icons/hicolor/256x256/apps/dev.mrquantumoff.mcmodpackmanager.png
mkdir -p /app/opt/dev.mrquantumoff.mcmodpackmanager
install -D dev.mrquantumoff.mcmodpackmanager.desktop  /app/share/applications/dev.mrquantumoff.mcmodpackmanager.desktop
cp GNULinuxBuild.tar.gz /app/opt/dev.mrquantumoff.mcmodpackmanager
cd /app/opt/dev.mrquantumoff.mcmodpackmanager
ls
tar -xpf GNULinuxBuild.tar.gz
rm GNULinuxBuild.tar.gz
# mv build/*/*/*/*/*/ .
mv build/*/*/*/*/* .
rm -rf build
ls /app/opt/dev.mrquantumoff.mcmodpackmanager
chmod +x /app/opt/dev.mrquantumoff.mcmodpackmanager/mcmodpackmanager_reborn
mkdir -p /app/bin
ln -sf /app/opt/dev.mrquantumoff.mcmodpackmanager/mcmodpackmanager_reborn /app/bin/mcmodpackmanager_reborn
