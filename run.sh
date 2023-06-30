#!/bin/bash -e

if [ -z "$1" ]; then
	echo "No argument supplied. Possible programs:"
	ls /app/bin/ | sed "s/run\.sh//"
else
	exec /app/bin/$@ -kotlin-home /app/lib/
fi
