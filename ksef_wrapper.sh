#!/bin/sh
# Prostszy wrapper uruchamiający aplikację
# Aplikacja w Pythonie sama dba o ścieżki XDG, więc nie musimy zmieniać katalogu.
# (Pozostawiamy wrapper na wypadek potrzeby ustawienia zmiennych środowiskowych w przyszłości)

exec /app/bin/KsefInvoice "$@"
