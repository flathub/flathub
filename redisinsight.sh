#!/bin/sh

set -oue pipefail

export FLATPAK_ID="${FLATPAK_ID:-com.redis.RedisInsight}"
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/redisinsight/AppRun $@
