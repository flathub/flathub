#!/bin/sh

export FAKE_PID="${FAKE_PID:-424242}"

export LD_PRELOAD="/app/lib/fakepid.so${LD_PRELOAD:+:$LD_PRELOAD}"

exec /app/ab-download-manager/bin/ABDownloadManager "$@"

