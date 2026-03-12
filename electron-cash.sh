#!/bin/bash
export LD_LIBRARY_PATH=/app/lib:/app/lib/python3.13/site-packages/electroncash/tor/bin:${LD_LIBRARY_PATH}
export QT_QPA_PLATFORMTHEME=kde
exec /app/bin/electron-cash.real "$@"
