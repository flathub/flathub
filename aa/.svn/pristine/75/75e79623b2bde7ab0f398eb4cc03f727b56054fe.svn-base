# Process this file with automake to produce Makefile.in

# Copyright (C) 2013 COR Entertainment, LLC.

# --- ALIEN ARENA COMMON DATA ---


SUBDIRS = botinfo env gfx levelshots maps models particles pics players scripts sound textures vehicles

#
# TODO: add licensing and other docs for the data package
#


fontsdir=$(pkgdatadir)/data1

nobase_dist_fonts_DATA = \
 fonts/agentred.ttf \
 fonts/bluehigd.ttf \
 fonts/chicomono.ttf \
 fonts/creativeblock.ttf \
 fonts/freemono.ttf \
 fonts/freesans.ttf \
 fonts/orbitron.ttf \
 fonts/crackdown.ttf \
 fonts/bendable.ttf

graphical_presetsdir=$(pkgdatadir)/data1

nobase_dist_graphical_presets_DATA = \
 graphical_presets/compatibility.cfg \
 graphical_presets/maxperformance.cfg \
 graphical_presets/performance.cfg \
 graphical_presets/quality.cfg \
 graphical_presets/maxquality.cfg

dist-hook:
	rm -rf `find $(distdir) -name .svn`
