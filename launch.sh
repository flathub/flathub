#!/bin/sh

NS_DIR="$HOME/.syno_ns_app"
#NS_DIR_BACKUP="$HOME/.syno_ns_app.bak"
# Changing Path to reduce permissions in Flatpak
NS_DIR_BACKUP="$HOME/.syno_ns_backup/.syno_ns_app.bak"
NS_DATA_DIR=".note_data"
NS_BIN="$NS_DIR/synology-note-station"

if [ ! -x "$NS_BIN" ]; then
	mkdir -p $NS_DIR/
  #	cp -af /opt/synology-note-station/* $NS_DIR/
  # Changing Path to reduce permissions in Flatpak
	cp -af /app/extra/* $NS_DIR/
fi

if [ ! -d "$NS_DIR/$NS_DATA_DIR" -a -d "$NS_DIR_BACKUP" -a -d "$NS_DIR_BACKUP/$NS_DATA_DIR" ]; then
	cp -af "$NS_DIR_BACKUP/$NS_DATA_DIR" "$NS_DIR/$NS_DATA_DIR"
	rm -rf "$NS_DIR_BACKUP"
fi

cd $NS_DIR
exec $NS_BIN