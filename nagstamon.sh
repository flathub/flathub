#!/bin/bash

set -e

if [ -f ~/.nagstamon/nagstamon.pid ]
then
    rm -f ~/.nagstamon/nagstamon.pid
fi

exec python3 /app/bin/nagstamon.py