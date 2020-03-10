#!/usr/bin/env sh

set -e

if [[ -d /usr/lib/sdk/dotnet ]]; then
  . /usr/lib/sdk/dotnet/enable.sh
fi

if [[ -d /usr/lib/sdk/mono5 ]]; then
  . /usr/lib/sdk/mono5/enable.sh
fi

exec env JAVA_TOOL_OPTIONS=-Djava.io.tmpdir=${XDG_CACHE_HOME}/tmp/ \
    TMPDIR=${XDG_CACHE_HOME}/tmp/ \
    /app/extra/rider/bin/rider.sh "$@"