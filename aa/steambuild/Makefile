# Makefile for Alien Arena Warriors of Mars on Steam
#
# Builds client/listen server executable under Ubuntu 16.04 LTS
#  with chroot to Steam runtime (which is 12.04 LTS)
#
# 64bit command for this Makefile. default is release build.
# $ schroot --chroot steamrt_scout_amd64 -- make [debug_build=no]
#
# Reference:
# https://partner.steamgames.com/doc/home
# https://partner.steamgames.com/doc/store/application/platforms/linux
# https://github.com/ValveSoftware/steam-runtime
# https://developer.valvesoftware.com/wiki/Steam_under_Linux
# 
# Notes:
#  See $man schroot
#  For now, do dedicated server in other separate makefile.
#  Will probably never make a Linux 32-bit version.
#  Need to determine exact compiler options. (esp. security opts).
#  Use modified config.h from non-chroot autotools build with STEAM_VARIANT 1 defined.
#  The ODE library is not in the steam runtime, link to static libode.a.
#  Install ODE thusly:
#   $sudo schroot --chroot steamrt_scout_amd64 -- apt-get install libode-dev


#------------------------------------------------------------------------------
# TOOLS
#------------------------------------------------------------------------------
CC  := gcc
CXX := g++
AR  := ar
RANLIB := ranlib
MKDIR_P := mkdir -p

#------------------------------------------------------------------------------
# BUILD OPTIONS
#------------------------------------------------------------------------------
debug_build := no

#------------------------------------------------------------------------------
# PATHS
#------------------------------------------------------------------------------

#
# Use VPATH to find sources for dependency rules
# Using the lower case, file-type specific, vpath's is probably better
# than using this: VPATH = ../src
# (':' or SPACE are path separators)
#
# These assume the build is done in the steambuild subdirectory,
# and that there is a symbolic link (src) to the program's SVN directory (source)
# and that a config.h from a normal build, but with STEAM_VARIANT 1 defined, is in the build directory
# and that you have ODE installed
# and that the make command is executed from inside the build directory

# vpath doesn't work for header files,
# https://stackoverflow.com/questions/9446634/makefile-vpath-not-working-for-header-files
# see cppflags_src_inc.
# I couldn't get ode header files to be included with -I/usr/include,
# hence copied the ode folder to steambuild/ode and the minizip folder to steambuild/unix/minizip for the header files.

vpath %.c src

#------------------------------------------------------------------------------
# CPPFLAGS - Defines and Include Paths
#------------------------------------------------------------------------------
cppflags_common  := -DHAVE_CONFIG_H
cppflags_debug   := -DDEBUG_GDB_NOGRAB
cppflags_nodebug := -DNDEBUG
# cppflags_datadir := -DDATADIR='"/usr/local/share/alienarena"'
#  see autotools  -DCOR_DATADIR  location of data1, for steam should be  './'
cppflags_src_inc := -Isrc -Isrc/game -Isrc/game/acesrc -I.
cppflags_ft2_inc := -I/usr/include/freetype2
cppflags_odemath := -DdDOUBLE

#------------------------------------------------------------------------------
# CFLAGS, CXXFLAGS
#------------------------------------------------------------------------------
cflags_common := $(CFLAGS) -pipe -ffast-math -fno-strict-aliasing
cflags_debug   := -O0 -ggdb $(cflags_common)
cflags_nodebug := -O2 $(cflags_common) -fstack-protector
cxxflags_debug   := $(cflags_debug)
cxxflags_nodebug := $(cflags_nodebug)

#------------------------------------------------------------------------------
# LDADD, LDFLAGS
#------------------------------------------------------------------------------
#

ldadd_pthread   := -pthread
#  ldadd_xf86dga   := -lXxf86dga
ldadd_xf86vm    := -lXxf86vm
ldadd_xll       := -lX11
ldadd_curl      := -lcurl
ldadd_freetype  := -lfreetype
ldadd_jpeg      := -ljpeg
ldadd_oggvorbis := -lvorbisfile -lvorbis -logg
ldadd_ode	:= src/libode.a
ldadd_zlib	:= -lz
ldadd_m  := -lm
ldadd_dl := -ldl
ldadd_rt := -lrt
ldadd_stdcxx := -lstdc++

ldflags_common := $(LDFLAGS) -Wl,--as-needed

#------------------------------------------------------------------------------
# TARGET COMPONENT NAMES, FLAGS AND LIBS
#------------------------------------------------------------------------------

#
# --- Client Executable ---
#

crx_exe := alienarena.x86_64

ifeq ($(debug_build),$(strip yes))

 crx_cppflags := $(cppflags_common)  $(cppflags_src_inc) $(cppflags_ft2_inc) \
  $(cppflags_odemath) $(cppflags_debug)

 crx_cflags   := $(cflags_debug)

else

 crx_cppflags := $(cppflags_common)  $(cppflags_src_inc) $(cppflags_ft2_inc) \
  $(cppflags_odemath) $(cppflags_nodebug)

 crx_cflags   := $(cflags_nodebug)

endif

crx_libs := $(libs_game)

crx_ldadd := $(ldadd_pthread) $(ldadd_xf86vm) $(ldadd_xll) \
 $(ldadd_freetype) $(ldadd_curl) $(ldadd_jpeg) $(ldadd_oggvorbis) \
 $(ldadd_zlib) $(ldadd_m) $(ldadd_dl) $(ldadd_rt) \
 $(ldadd_ode) $(ldadd_stdcxx)


crx_ldflags := $(ldflags_common)

#
# --- libgame.a ---
#
libs_game := libgame.a

ifeq ($(debug_build),$(strip yes))
 libgame_cppflags := $(cppflags_common) $(cppflags_src_inc) $(cppflags_debug)
 libgame_cflags   := -fpic $(cflags_debug)
else
 libgame_cppflags := $(cppflags_common) $(cppflags_src_inc) $(cppflags_nodebug)
 libgame_cflags   := -fpic $(cflags_nodebug)
endif

libgame_arflags := cru

#------------------------------------------------------------------------------
# SOURCES
#------------------------------------------------------------------------------

# Alien Arena Client
crx_sources := \
	client/cl_ents.c \
	client/cl_fx.c \
	client/cl_http.c \
	client/cl_input.c \
	client/cl_inv.c \
	client/cl_irc.c \
	client/cl_main.c \
	client/cl_parse.c \
	client/cl_pred.c \
	client/cl_scrn.c \
	client/cl_stats.c \
	client/cl_tent.c \
	client/cl_updates.c \
	client/cl_view.c \
	client/console.c \
	client/keys.c \
	client/menu.c \
	client/qal.c \
	client/qmenu.c \
	client/snd_file.c \
	client/snd_openal.c \
	game/q_shared.c \
	qcommon/binheap.c \
	qcommon/cmd.c \
	qcommon/cmodel.c \
	qcommon/common.c \
	qcommon/crc.c \
	qcommon/cvar.c \
	qcommon/files.c \
	qcommon/htable.c \
	qcommon/image.c \
	qcommon/libgarland.c \
	qcommon/md5.c \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/terrain.c \
	ref_gl/r_decals.c \
	ref_gl/r_bloom.c \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_iqm.c \
	ref_gl/r_light.c \
	ref_gl/r_main.c \
	ref_gl/r_math.c \
	ref_gl/r_md2.c \
	ref_gl/r_mesh.c \
	ref_gl/r_misc.c \
	ref_gl/r_model.c \
	ref_gl/r_particle.c \
	ref_gl/r_postprocess.c \
	ref_gl/r_program.c \
	ref_gl/r_ragdoll.c \
	ref_gl/r_script.c \
	ref_gl/r_shadowmaps.c \
	ref_gl/r_surf.c \
	ref_gl/r_terrain.c \
	ref_gl/r_text.c \
	ref_gl/r_ttf.c \
	ref_gl/r_varray.c \
	ref_gl/r_vbo.c \
	ref_gl/r_warp.c \
	server/sv_ccmds.c \
	server/sv_ents.c \
	server/sv_game.c \
	server/sv_init.c \
	server/sv_main.c \
	server/sv_send.c \
	server/sv_user.c \
	server/sv_world.c \
	unix/gl_glx.c \
	unix/glob.c \
	unix/net_udp.c \
	unix/q_shunix.c \
	unix/qal_unix.c \
	unix/qgl_unix.c \
	unix/rw_unix.c \
	unix/sys_unix.c \
	unix/vid_so.c

# Game Library
libgame_sources := \
	game/acesrc/acebot_ai.c \
	game/acesrc/acebot_cmds.c \
	game/acesrc/acebot_items.c \
	game/acesrc/acebot_movement.c \
	game/acesrc/acebot_nodes.c \
	game/acesrc/acebot_spawn.c \
	game/c_cam.c \
	game/g_ai.c \
	game/g_chase.c \
	game/g_cmds.c \
	game/g_combat.c \
	game/g_ctf.c \
	game/g_deathray.c \
	game/g_func.c \
	game/g_items.c \
	game/g_main.c \
	game/g_misc.c \
	game/g_monster.c \
	game/g_phys.c \
	game/g_save.c \
	game/g_spawn.c \
	game/g_spider.c \
	game/g_svcmds.c \
	game/g_target.c \
	game/g_trigger.c \
	game/g_unlagged.c \
	game/g_utils.c \
	game/g_vehicles.c \
	game/g_weapon.c \
	game/m_move.c \
	game/p_client.c \
	game/p_hud.c \
	game/p_trail.c \
	game/p_view.c \
	game/p_weapon.c \
	game/q_shared.c \
	unix/minizip/ioapi.c \
	unix/minizip/miniunz.c \
	unix/minizip/minizip.c \
	unix/minizip/mztools.c \
	unix/minizip/unzip.c \
	unix/minizip/zip.c

#------------------------------------------------------------------------------
# TARGETS and RULES
#------------------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES: .c .o .d

all: $(libs_game) $(crx_exe)
	@echo "all done"

#
# Generate object and dependency file names from source names.
#
# Note the added suffixes. These are needed to differentiate
#  objects from shared source files.
#
libgame_objects     := $(libgame_sources:%.c=%_g.o)
crx_objects         := $(crx_sources:.c=.o)

crx_depinc     := $(crx_sources:.c=.d)
libgame_depinc := $(libgame_sources:.c=_g.d)

#
# Include the dependency rules
#
# Note: sort removes duplicates, but may not really be needed.
#
dependency_inc := $(sort \
 $(crx_depinc) \
 $(libgame_depinc) )


$(dependency_inc):

-include  $(dependency_inc)


$(libgame_objects) : %_g.o : %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(libgame_cppflags) $(libgame_cflags) -MMD -c $<  -o $@


$(crx_objects) :  %.o : %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(crx_cppflags) $(crx_cflags) -MMD -c $<  -o $@


$(libs_game): $(libgame_objects)
	@echo "building $@"
	$(MKDIR_P) $(dir $@)
	@rm -f $@
	$(AR) $(libgame_arflags)  $@  $(libgame_objects)
	$(RANLIB) $@
	@echo "$@ complete"


$(crx_exe): $(crx_objects) $(libs_game)
	@echo "building $@"
	$(MKDIR_P) $(dir $@)
	@rm -f  $@
	$(CC) $(crx_cflags) $(crx_ldflags) -o $@ $(crx_objects) \
		$(libs_game) $(crx_ldadd)
	@echo "$@ complete"



clean: clean_cor clean_dep


clean_cor:
	rm -f $(libgame_objects)
	rm -f $(libs_game)
	rm -f $(crx_objects)
	rm -f $(crx_exe)


clean_dep:
	rm -f $(dependency_inc)


.PHONY:	clean clean_cor clean_dep dependency_inc
