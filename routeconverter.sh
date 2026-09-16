#!/bin/sh
exec /app/jre/bin/java -Xmx1024m -jar /app/share/routeconverter/RouteConverterLinux.jar "$@"
