#!/bin/sh
# the app Gradle built, with its own Java runtime; libmpv comes from /app/lib
exec /app/tubehaus/bin/TubeHaus "$@"
