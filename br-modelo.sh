#!/bin/sh

if [[ ! -f "$XDG_DATA_HOME/Template.brMt" ]]; then
  cp /app/br-modelo/Template.brMt $XDG_DATA_HOME/Template.brMt
fi

exec java -jar /app/br-modelo/brModelo.jar "${@}"