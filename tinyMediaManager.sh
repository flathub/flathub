#!/bin/bash
cd /app/tmm
exec /app/tmm/tinyMediaManager -Dtmm.noupdate=true "$@"