#!/bin/sh
export LD_LIBRARY_PATH=/app/lib/ksef:$LD_LIBRARY_PATH

# Launch the application
exec /app/lib/ksef/KsefInvoice "$@"


