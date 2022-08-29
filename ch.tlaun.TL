#!/usr/bin/env sh
exec java \
	-Dtlauncher.bootstrap.ignoreSelfUpdate \
	-Dtlauncher.bootstrap.brand=legacy_beta \
	-Dtlauncher.bootstrap.requireMinecraftAccount \
	-jar /app/lib/tl/bootstrap.jar "$@"
