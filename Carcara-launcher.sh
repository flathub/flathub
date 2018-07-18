#!/bin/sh

MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  exec "/app/carcara_lin_6_1.5.x86_64"
else
  exec "/app/carcara_lin_6_1.5.x86"
fi

