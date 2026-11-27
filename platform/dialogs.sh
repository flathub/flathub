# Notices for desktops that lack a tray or the card reader service.
# Sourced by the launcher.

has_tray() {
    gdbus call --session --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus \
        --method org.freedesktop.DBus.NameHasOwner org.kde.StatusNotifierWatcher 2>/dev/null |
        grep -q true
}

has_reader_service() {
    [ -S /run/pcscd/pcscd.comm ]
}

dialog() {
    zenity --title="Adalet E-imza" --width=360 "$@" 2>/dev/null
}

# succeeds when the user chooses to quit
ask_quit() {
    dialog --question --default-cancel --ok-label="Çık" --cancel-label="Tamam" \
        --text="Adalet E-imza arka planda çalışıyor. Bildirim alanı simgesi için AppIndicator eklentisi gerekir."
}

warn_reader() {
    dialog --warning \
        --text="Kart okuyucu servisi bulunamadı. Sisteminize pcscd paketini kurun."
}
