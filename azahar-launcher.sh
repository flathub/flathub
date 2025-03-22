#!/bin/bash -e

report_error() {
    read -r -d '|' MESSAGE <<EOF
Unfortunately, Azahar seems to have crashed.
We kindly ask you to submit a bug report to <a href="https://github.com/flathub/org.azahar_emu.Azahar/issues">https://github.com/flathub/org.azahar_emu.Azahar/issues</a>.

When submitting a bug report, please attach your <b>system information</b> and the <b>Azahar log file</b>.
You seem to be using ${XDG_SESSION_DESKTOP} ${DESKTOP_SESSION} (${XDG_SESSION_TYPE}):

To obtain Azahar log files, please see <a href="https://web.archive.org/web/20240229211203/https://community.citra-emu.org/t/how-to-upload-the-log-file/296/">this guide</a>.

To obtain your system information, please install <tt>inxi</tt> and run <tt>inxi -v3</tt>.
Please also provide the value shown after running <tt>ulimit -nH</tt>. |
EOF
    zenity --warning --no-wrap --title "That's awkward ..." --text "$MESSAGE"
}

unset VK_ICD_FILENAMES VK_DRIVER_FILES
# Discord RPC
for i in {0..9}; do
    test -S "$XDG_RUNTIME_DIR"/"discord-ipc-$i" || ln -sf {app/com.discordapp.Discord,"$XDG_RUNTIME_DIR"}/"discord-ipc-$i";
done


if ! prlimit --nofile=8192 azahar "$@"; then
    report_error
fi
