set -eu

APP="/app/opt/darkpurpleofswebsite/bin/DarkPurpleOFsWebsite"

args=()
for a in "$@"; do
    case "$a" in
        file://*)
            a="${a#file://}"
            a="$(printf '%b' "${a//%/\\x}")"   # URL-decode
            ;;
    esac
    args+=("$a")
done

if [ "${#args[@]}" -lt 1 ]; then
    exec "$APP"
fi

first="${args[0]}"
case "${first,,}" in
    *.dpplugin)
        exec "$APP" -plugin -install -path "$first"
        ;;
    *.dpfstickers)
        exec "$APP" --import-stickers "$first"
        ;;
    darkpurpleof://*)
        exec "$APP" "$first"
        ;;
    *)
        exec "$APP" "${args[@]}"
        ;;
esac
