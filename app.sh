#!/bin/sh
export PATH=/app/jre/bin:/app/bin
export _JAVA_OPTIONS=-Djava.util.prefs.userRoot=$HOME/.var/app/com.empesol.timetracker/config
java -jar /app/bin/app.jar
