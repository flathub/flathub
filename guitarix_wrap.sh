#!/usr/bin/env bash

DEVICE=hw:0
RATE=48000
PERIOD=256
NUMBER_OF_PERIODS=2

# start jack
jackd -RSv -d alsa -r $RATE -p $PERIOD -n $NUMBER_OF_PERIOUDS -m -d $DEVICE &
pid_of_jack=$!

sleep 1

/app/bin/guitarix

# kill jack
kill -9 $pid_of_jack
