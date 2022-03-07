#!/bin/sh
FILEBOT_HOME="/app"


# sanity check
if [ -z "$HOME" ]; then
	echo '$HOME must be set'
	exit 1
fi

if [ "$EUID" = "0" ]; then
	echo "$0 must NOT run as root"
fi


# select application data folder
APP_DATA="$XDG_CONFIG_HOME/filebot"
LIBRARY_PATH="$FILEBOT_HOME/lib/$(uname -m):/lib64"
MODULE_PATH=$(echo /usr/lib/jvm/java-*-openjdk/lib/javafx.* | tr ' ' ':')

java -Dapplication.deployment=aur --module-path "$MODULE_PATH" --add-modules ALL-MODULE-PATH -Dapplication.update=skip -Dnet.filebot.archive.extractor=ShellExecutables --add-opens=java.base/java.lang=ALL-UNNAMED --add-opens=java.base/java.lang.reflect=ALL-UNNAMED --add-opens=java.base/java.lang.invoke=ALL-UNNAMED --add-opens=java.base/java.util=ALL-UNNAMED --add-opens=java.base/java.util.function=ALL-UNNAMED --add-opens=java.base/java.util.regex=ALL-UNNAMED --add-opens=java.base/java.net=ALL-UNNAMED --add-opens=java.base/java.io=ALL-UNNAMED --add-opens=java.base/java.nio=ALL-UNNAMED --add-opens=java.base/java.nio.file=ALL-UNNAMED --add-opens=java.base/java.nio.file.attribute=ALL-UNNAMED --add-opens=java.base/java.nio.channels=ALL-UNNAMED --add-opens=java.base/java.nio.charset=ALL-UNNAMED --add-opens=java.base/java.time=ALL-UNNAMED --add-opens=java.base/java.time.chrono=ALL-UNNAMED --add-opens=java.base/java.util.concurrent=ALL-UNNAMED --add-opens=java.base/java.text=ALL-UNNAMED --add-opens=java.base/sun.nio.fs=ALL-UNNAMED --add-opens=java.logging/java.util.logging=ALL-UNNAMED --add-opens=java.desktop/java.awt=ALL-UNNAMED --add-opens=java.desktop/sun.awt=ALL-UNNAMED --add-opens=java.desktop/sun.swing=ALL-UNNAMED --add-opens=java.desktop/javax.swing.text.html=ALL-UNNAMED --add-opens=java.prefs/java.util.prefs=ALL-UNNAMED -XX:+DisableAttachMechanism -Djna.boot.library.path="$LIBRARY_PATH" -Djna.library.path="$LIBRARY_PATH" -Djava.library.path="$LIBRARY_PATH" -Dapplication.dir="$APP_DATA" -Dapplication.cache="$APP_DATA/cache" -Djava.io.tmpdir="$APP_DATA/tmp" -Dfile.encoding="UTF-8" -Dsun.jnu.encoding="UTF-8" -Dawt.useSystemAAFontSettings=on -Dprism.order=sw -Dnet.filebot.theme=Darcula -Dnet.filebot.UserFiles.fileChooser=Zenity -DuseGVFS=true -Dnet.filebot.gio.GVFS="$XDG_RUNTIME_DIR/gvfs" $JAVA_OPTS $FILEBOT_OPTS -jar "$FILEBOT_HOME/jar/filebot.jar" "$@"
