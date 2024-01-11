# Regarding flatpak-pip-generator

I failed to get it working with the default script, something always went wrong,
so I tweaked it to prefer the platform-independent .whl files over other archives,
and that finally allowed me to build a flatpak :) 


## Notes:

See the Makefile for local test builds.