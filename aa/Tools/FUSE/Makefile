# FUSE makefile
CC?=gcc

# Compiler options
CFLAGS=-DLINUX -c -Wall -g

# Linker options
LDFLAGS=

# Paths for dependent include files
CFLAGS+=`pkg-config --cflags gtk+-2.0`
CFLAGS+=`pkg-config --cflags libglade-2.0`

# Paths for dependent libraries
LDFLAGS+=`pkg-config --libs gtk+-2.0`
LDFLAGS+=`pkg-config --libs libglade-2.0`

# The build sources themselves
SRCS = main.c slink.c models.c config.c
HDRS = slink.h models.h global.h config.h
OBJS = $(subst .c,.o,$(SRCS))

# The files put into releases
RELEASE_FILES_LINUX = Makefile $(SRCS) $(HDRS) fuse.glade fuse.ico COPYING.TXT BUILDING.TXT
RELEASE_FILES_WINDOWS = fuse.exe fuse.glade iconv.dll libglade-2.0-0.dll COPYING.TXT


all: FUSE
	@echo "*** Don't forget to edit config/games.xml to set your paths ***"

FUSE: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o FUSE

.c.o:
	$(CC) $(CFLAGS) $*.c

depend:
	makedepend -- $(CFLAGS) -- $(SRCS)

clean:
	rm -f $(OBJS) FUSE


release-windows: fuse.exe
	@mkdir FUSE
	cp $(RELEASE_FILES_WINDOWS) FUSE
	zip -r FUSE.zip FUSE
	@rm -rf FUSE
	@echo ***Windows release complete***
	
release-linux: FUSE
	@mkdir fuse
	@cp $(RELEASE_FILES_LINUX) fuse
	tar czf fuse.tar.gz fuse
	@rm -rf fuse
	@echo ***Linux release complete***

release-all: release-linux release-windows

release-clean:
	rm -f fuse.zip fuse.tar.gz

clean-all: clean release-clean

# DO NOT DELETE
