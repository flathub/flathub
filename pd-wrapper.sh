#!/bin/sh

exec pd-l2ork -alsa -alsaadd pulse "$@"
