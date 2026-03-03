# wrapper kept for future use, currently just a simple pass-through to the main binary. This allows us to set environment variables or do other setup in the future if needed without changing the Flatpak manifest.
exec /app/bin/KsefInvoice "$@"
