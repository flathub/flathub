# Regarding flatpak-pip-generator

I failed to get it working with the default script, something always went wrong,
so I tweaked it to prefer the platform-independent .whl files over other archives,
and that finally allowed me to build a flatpak :) 


## Notes:

See the Makefile for local test builds.


# Collecting manual dependencies.

For torch version 2.1.0 I had no issues running the flatpak, but users reported a strange error that indicated a version 
incompatibility between torch and torchvision. The versions matched, but I did notice that upon startup, torch output
an error message about missing some libjpg library which apparently would've been caused by a bad compilation.

To figure out why my version worked, I created a fresh venv and installed torch using the pip command from
https://pytorch.org/get-started/locally/: pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cpu
Rather than using the index site myself. Pip grabbed a different file than the one on the index site, torchvision remained the
same. The new file worked in the venv, so I incorporated that into the flatpak.

# About DBus-python
this package would be required to provide support for the --notify command line option, but in the gui we don't need to support that,
and since it only causes strange problems, it shall be forcefully ignored.

# About nvidia
Simple lama inpainting introduces many optional nvidia dependencies. Be sure to nuke them from auto-generated pypi dependencies.
