#
# Codered Makefile for Alien Arena Map Building Tools for Linux 
#
#  qbsp3
#  qvis3
#  qrad3
#
#  2010-09 bspinfo3 build disabled
#

# BUILD = RELEASE or DEBUG
BUILD ?= RELEASE

# Where programs will be installed on 'make install'
INSTALL_DIR ?= /usr/local/bin

# Compiler
CC ?= gcc

# Compile Options
WITH_PTHREAD ?= -pthread -DUSE_PTHREADS
WITHOUT_PTHREAD ?= -DUSE_SETRLIMIT
# THREADING_OPTION = WITH_PTHREAD or WITHOUT_PTHREAD
THREADING_OPTION ?= $(WITH_PTHREAD)
BASE_CFLAGS ?= -Wall $(THREADING_OPTION) -DUSE_ZLIB
RELEASE_CFLAGS ?= $(BASE_CFLAGS) -O3 
DEBUG_CFLAGS ?= $(BASE_CFLAGS) -O0 -g -ggdb

# Link Options
LDFLAGS ?= -lm -lz

#
srcdir = .
srcbsp = bsp/
includedirs = -Icommon
depdir = .deps

# source locations
vpath %.h bsp/bspinfo3
vpath %.c bsp/bspinfo3
vpath %.h bsp/qbsp3
vpath %.c bsp/qbsp3
vpath %.h bsp/qvis3
vpath %.c bsp/qvis3
vpath %.h bsp/qrad3
vpath %.c bsp/qrad3
vpath %.h common
vpath %.c common

ifeq ($(BUILD),DEBUG)
CFLAGS ?= $(DEBUG_CFLAGS)
builddir = debug
vpath %.o debug
else
CFLAGS ?= $(RELEASE_CFLAGS)
builddir = release
vpath %.o release
endif

# common files 
cmn_srcs = \
	bspfile.c	\
	cmdlib.c	\
	lbmlib.c	\
	mathlib.c	\
	parsecfg.c	\
	polylib.c	\
	scriplib.c	\
	threads.c

cmn_objs = $(cmn_srcs:.c=.o)

cmn_dep = $(addprefix $(depdir)/, $(cmn_srcs:.c=.d) )

#
# files for bspinfo3
#
bspinfo3_srcs = bspinfo3.c

bspinfo3_cmnobjs = \
	bspfile.o	\
	cmdlib.o	\
	mathlib.o	\
	scriplib.o

bspinfo3_objs =	$(bspinfo3_srcs:.c=.o)
bspinfo3_objs_all = $(bspinfo3_objs) $(bspinfo3_cmnobjs)
bspinfo3_dep = $(addprefix $(depdir)/, $(bspinfo3_srcs:.c=.d) )

#
# files for qbsp3
#
qbsp3_srcs =	\
    qbsp3.c     \
	brushbsp.c	\
	faces.c		\
	nodraw.c	\
	glfile.c	\
	leakfile.c	\
	map.c		\
	portals.c	\
	prtfile.c	\
	textures.c	\
	tree.c		\
	writebsp.c	\
	csg.c

qbsp3_cmnobjs = \
	bspfile.o	\
	cmdlib.o	\
	lbmlib.o	\
	mathlib.o	\
	parsecfg.o	\
	polylib.o	\
	scriplib.o	\
	threads.o

qbsp3_objs = $(qbsp3_srcs:.c=.o)
qbsp3_objs_all = $(qbsp3_objs) $(qbsp3_cmnobjs)
qbsp3_dep = $(addprefix $(depdir)/, $(qbsp3_srcs:.c=.d) )

#
# files for qvis3
#
qvis3_srcs =	\
	qvis3.c		\
	flow.c

qvis3_cmnobjs =	\
	bspfile.o	\
	cmdlib.o	\
	mathlib.o	\
	parsecfg.o	\
	scriplib.o	\
	threads.o

qvis3_objs = $(qvis3_srcs:.c=.o)
qvis3_objs_all = $(qvis3_objs) $(qvis3_cmnobjs)
qvis3_dep = $(addprefix $(depdir)/, $(qvis3_srcs:.c=.d) )

#
# files for qrad3
#
qrad3_srcs =	\
	binheap.c	\
	image.c		\
	libgarland.c\
	lightmap.c	\
	patches.c	\
	qrad3.c		\
	terrain.c	\
	terrain_trace.c\
	trace.c

qrad3_cmnobjs = \
	bspfile.o	\
	cmdlib.o	\
	lbmlib.o	\
	mathlib.o	\
	parsecfg.o	\
	polylib.o	\
	scriplib.o	\
	threads.o

qrad3_objs = $(qrad3_srcs:.c=.o)
qrad3_objs_all = $(qrad3_objs) $(qrad3_cmnobjs)
qrad3_dep = $(addprefix $(depdir)/, $(qrad3_srcs:.c=.d) )

#
# --- targets ---
#
# note: bspinfo3 not included in build

all: subdirectories $(builddir)/qbsp3 $(builddir)/qvis3 $(builddir)/qrad3

# dependency (.d) files
include $(cmn_dep)
## include $(bspinfo3_dep)
include $(qbsp3_dep)
include $(qvis3_dep)
include $(qrad3_dep)

# compile common sources
$(cmn_objs): %o : %c
	$(CC) -c $(includedirs) $(CFLAGS) $< -o $(builddir)/$@

# link bspinfo3
## $(builddir)/bspinfo3: $(bspinfo3_objs_all)
##	$(CC) $(CFLAGS) $(LDFLAGS) $(addprefix $(builddir)/, $(bspinfo3_objs_all)) -o $(builddir)/bspinfo3

# compile bspinfo3 sources
## $(bspinfo3_objs): %o : %c
##	$(CC) -c $(includedirs) $(CFLAGS) $< -o $(builddir)/$@

# link qbsp3
$(builddir)/qbsp3: $(qbsp3_objs_all)
	$(CC) $(CFLAGS) $(addprefix $(builddir)/, $(qbsp3_objs_all)) -o $(builddir)/qbsp3 $(LDFLAGS)

# compile qbsp3 sources
$(qbsp3_objs): %o : %c
	$(CC) -c $(includedirs) $(CFLAGS) $< -o $(builddir)/$@

# link qvis3
$(builddir)/qvis3: $(qvis3_objs_all)
	$(CC) $(CFLAGS) $(addprefix $(builddir)/, $(qvis3_objs_all)) -o $(builddir)/qvis3 $(LDFLAGS)

# compile qvis3 sources
$(qvis3_objs): %o : %c
	$(CC) -c $(includedirs) $(CFLAGS) $< -o $(builddir)/$@

# link qrad3
$(builddir)/qrad3: $(qrad3_objs_all)
	$(CC) $(CFLAGS) $(addprefix $(builddir)/, $(qrad3_objs_all)) -o $(builddir)/qrad3 $(LDFLAGS)

# compile qrad3 sources
$(qrad3_objs): %o : %c
	$(CC) -c $(includedirs) $(CFLAGS) $< -o $(builddir)/$@

subdirectories:
	mkdir -p $(depdir)
	mkdir -p $(builddir)

clean:
	rm -f $(builddir)/*.o
	rm -f $(builddir)/qbsp3
	rm -f $(builddir)/qvis3
	rm -f $(builddir)/qrad3

##	rm -f $(builddir)/bspinfo

install:
	cp $(builddir)/qbsp3   $(INSTALL_DIR)
	cp $(builddir)/qvis3   $(INSTALL_DIR)
	cp $(builddir)/qrad3   $(INSTALL_DIR)

##	cp $(builddir)/bspinfo3 $(INSTALL_DIR)

dependents: $(cmn_dep) $(qbsp3_dep) $(qvis3_dep) $(qrad3_dep)

clean_dependents:
	rm -f $(depdir)/*.d

.PHONY:	clean install dependents subdirectories clean_dependents

# rule to generate dependency include files (.d files)
#  using compiler -MM option
$(depdir)/%.d : %.c
	mkdir -p $(depdir)
	set -e ;	\
	rm -f $@ ;	\
	$(CC) $(includedirs) -MM $(CFLAGS) $<  > $@.$$$$ ;	\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@ ;	\
	rm -f $@.$$$$

