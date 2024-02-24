export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper.sh /app/extra/feishu/vulcan/vulcan-original "$@"
