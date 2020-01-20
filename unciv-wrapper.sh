#!/bin/sh

UNCIVDIR=${XDG_DATA_HOME}/unciv

mkdir -p $UNCIVDIR
cd $UNCIVDIR
exec java -jar /app/data/Unciv.jar
