#!/usr/bin/env bash
set -euo pipefail

exec /usr/lib/sdk/openjdk21/bin/java -Dfile.encoding=UTF-8 -jar /app/lib/CircuitSim.jar "$@"
