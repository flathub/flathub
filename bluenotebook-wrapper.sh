#!/bin/bash
set -e

# Ajouter dynamiquement tous les dossiers site-packages trouvés (y compris local et dist-packages)
for dir in /app/lib/python*/site-packages \
           /app/lib64/python*/site-packages \
           /app/local/lib/python*/site-packages \
           /app/lib/python*/dist-packages; do
    if [ -d "$dir" ]; then
        export PYTHONPATH=$PYTHONPATH:$dir
    fi
done
export PYTHONPATH=$PYTHONPATH:/app/share/bluenotebook

# Fix pour QtWebEngineProcess : Le trouver et définir la variable d'environnement
if [ -z "$QTWEBENGINEPROCESS_PATH" ]; then
    PROCESS_PATH=$(find /app -name QtWebEngineProcess -type f 2>/dev/null | head -n 1)
    if [ -n "$PROCESS_PATH" ]; then
        export QTWEBENGINEPROCESS_PATH="$PROCESS_PATH"
    fi
fi

# Définir le répertoire du journal par défaut
if [ -z "$JOURNAL_DIRECTORY" ]; then
    # Fallback si XDG_DOCUMENTS_DIR n'est pas défini (fréquent dans Flatpak)
    if [ -z "$XDG_DOCUMENTS_DIR" ]; then
        export JOURNAL_DIRECTORY="$HOME/Documents/BlueNotebookJournal"
    else
        export JOURNAL_DIRECTORY="$XDG_DOCUMENTS_DIR/BlueNotebookJournal"
    fi
fi

# Lancer l'application
exec python3 /app/share/bluenotebook/bluenotebook/main.py "$@"