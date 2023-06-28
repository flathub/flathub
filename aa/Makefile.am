# Process this file with automake to produce Makefile.in
#
# Copyright (C) 2010,2014 COR Entertainment, LLC.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# A copy of the  GNU General Public License is included;
# see the file, GPLv2.
#

DIST_SUBDIRS = source
SUBDIRS = source
if WITH_DATA1PKG
DIST_SUBDIRS += data1
SUBDIRS += data1
endif

ACLOCAL_AMFLAGS = -I m4

#
# Alien Arena documents to be installed in $docdir
#
if DOC_INSTALL
unixdocdir=$(docdir)
dist_unixdoc_DATA = \
	unix_dist/README \
	unix_dist/COPYING \
	unix_dist/INSTALL \
	unix_dist/GPLv2 \
	unix_dist/NEWS \
	unix_dist/alien-arena.desktop
endif

#
# Alien Arena icons to be installed in icon directory
#
if ICONS_INSTALL
unixiconsdir=$(datadir)/icons/alienarena
dist_unixicons_DATA = \
	unix_dist/alien-arena.png
endif

#
# ARENA Default configuration for client and server
#
arenadir=$(pkgdatadir)/arena
dist_arena_DATA = \
	arena/motd.txt \
	arena/server.cfg \
	arena/default.cfg \
	arena/maps.lst

# FUSE game server browser and Server management scripts
# todo: verify/update scripts. deprecate fuse.
#EXTRA_DIST = \
#	Tools/fuse.tar.gz \
#	Tools/LinuxScripts

# remove svn directories from distribution staging area after creating
#  distribution in staging area and before running archiver
# but, it is probably a better idea to export the repository to the
#  staging area since export does not include the .svn parts
#
dist-hook:
	rm -rf `find $(distdir) -name .svn`
