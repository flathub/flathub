#!/bin/sh
export LD_LIBRARY_PATH=/app/lib/ksef:$LD_LIBRARY_PATH

# Próba integracji motywu
export QT_QPA_PLATFORMTHEME=gtk3

# Katalog na bazę danych i konfigurację użytkownika - zgodnie z XDG
DATA_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/KsefInvoice"

if [ ! -d "$DATA_DIR" ]; then
    mkdir -p "$DATA_DIR"
fi

# Przejście do katalogu roboczego, w którym aplikacja może zapisywać pliki
cd "$DATA_DIR" || exit

# Uruchomienie aplikacji
exec /app/lib/ksef/KsefInvoice "$@"
