#!/bin/sh
/app/bin/mono /app/lib/mono/4.5/cert-sync.exe --user /etc/ssl/cert.pem
exec /app/bin/mono /app/Kochbuch.exe "$@"
