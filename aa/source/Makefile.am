# Process this file with automake to produce Makefile.in
#
# Copyright (C) 2010,2014 COR Entertainment, LLC
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

noinst_LIBRARIES = libgame.a
if !USE_SYSTEM_LIBODE
noinst_LIBRARIES += libode.a
endif

#
# set location of `data1', etc
# set local include path
#
AM_CPPFLAGS = \
	-DCOR_DATADIR='"$(pkgdatadir)"' \
	-I$(top_srcdir)/source

#
# Setup the integrated ODE library.
# note: the -isystem option is for making #include <ode/ode.h> look in
#  the build directory before the system directory.
#
if !USE_SYSTEM_LIBODE
AM_CPPFLAGS += -isystem $(top_srcdir)/source/unix
endif

#
# On Unix, always build the server, and the client if BUILD_CLIENT is enabled
#
bin_PROGRAMS = alienarena-ded
if BUILD_CLIENT
bin_PROGRAMS += alienarena
endif


# Dedicated Server libraries and flags
alienarena_ded_CFLAGS =  -DDEDICATED_ONLY $(AM_CPPFLAGS)
alienarena_ded_LDADD = libgame.a

# Client libraries and flags
alienarena_CFLAGS = \
	$(AM_CPPFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(X11_CFLAGS) \
	$(XXF86VM_CFLAGS) \
	$(XXF86DGA_CFLAGS) \
	$(LIBCURL_CFLAGS) \
	$(OGG_CFLAGS) \
	$(VORBIS_CFLAGS) \
	$(FREETYPE2_CFLAGS) \
	$(ZLIB_CFLAGS)


if USE_SYSTEM_LIBODE
alienarena_CFLAGS += $(ODE_CFLAGS)
alienarena_LDADD = libgame.a
else
alienarena_LDADD = libode.a libgame.a
endif

alienarena_LDADD += \
	$(PTHREAD_LIBS) \
	$(X11_LIBS) \
	$(XXF86VM_LIBS) \
	$(XXF86DGA_LIBS) \
	$(LIBCURL_LIBS) \
	$(OGG_LIBS) \
	$(VORBIS_LIBS) \
	$(FREETYPE2_LIBS) \
	$(ZLIB_LIBS)

libgame_a_CFLAGS = -fpic

if USE_SYSTEM_LIBODE
alienarena_LDADD += $(ODE_LIBS)
alienarena_CFLAGS += -DdDOUBLE
else
alienarena_LDADD += -lstdc++
libode_a_CPPFLAGS = \
	-I$(top_srcdir)/source/unix/odesrc/OPCODE \
	-isystem $(top_srcdir)/source/unix \
	-isystem $(top_srcdir)/source/unix/odesrc \
	-DdDOUBLE \
	-DdTRIMESH_ENABLED \
	-DdTRIMESH_OPCODE \
	-DdTHREADING_INTF_DISABLED \
	-DdOU_ENABLED \
	-D_OU_FEATURE_SET=_OU_FEATURE_SET_BASICS \
	-D__ODE__

libode_a_CPPFLAGS += -DNDEBUG -DdNODEBUG

libode_a_CFLAGS = -fpic
libode_a_CXXFLAGS = -fpic
endif


# Alien Arena Client
alienarena_SOURCES = \
	client/anorms.h \
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
	client/client.h \
	client/console.c \
	client/console.h \
	client/input.h \
	client/keys.c \
	client/keys.h \
	client/menu.c \
	client/qal.c \
	client/qal.h \
	client/qmenu.c \
	client/qmenu.h \
	client/ref.h \
	client/screen.h \
	client/snd_file.c \
	client/snd_openal.c \
	client/sound.h \
	client/vid.h \
	game/game.h \
	game/q_shared.c \
	game/q_shared.h \
	qcommon/binheap.c \
	qcommon/binheap.h \
	qcommon/cmd.c \
	qcommon/cmodel.c \
	qcommon/common.c \
	qcommon/crc.c \
	qcommon/crc.h \
	qcommon/cvar.c \
	qcommon/files.c \
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/image.c \
	qcommon/libgarland.c \
	qcommon/libgarland.h \
	qcommon/md5.c \
	qcommon/md5.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	qcommon/terrain.c \
	ref_gl/r_decals.c \
	ref_gl/glext.h \
	ref_gl/glfuncs.h \
	ref_gl/qgl.h \
	ref_gl/r_bloom.c \
	ref_gl/r_draw.c \
	ref_gl/r_image.c \
	ref_gl/r_image.h \
	ref_gl/r_iqm.c \
	ref_gl/r_iqm.h \
	ref_gl/r_light.c \
	ref_gl/r_local.h \
	ref_gl/r_lodcalc.h \
	ref_gl/r_main.c \
	ref_gl/r_math.c \
	ref_gl/r_math.h \
	ref_gl/r_md2.c \
	ref_gl/r_mesh.c \
	ref_gl/r_misc.c \
	ref_gl/r_model.c \
	ref_gl/r_model.h \
	ref_gl/r_particle.c \
	ref_gl/r_postprocess.c \
	ref_gl/r_program.c \
	ref_gl/r_ragdoll.c \
	ref_gl/r_ragdoll.h \
	ref_gl/r_script.c \
	ref_gl/r_script.h \
	ref_gl/r_shadowmaps.c \
	ref_gl/r_surf.c \
	ref_gl/r_terrain.c \
	ref_gl/r_text.c \
	ref_gl/r_text.h \
	ref_gl/r_ttf.c \
	ref_gl/r_ttf.h \
	ref_gl/r_varray.c \
	ref_gl/r_vbo.c \
	ref_gl/r_warp.c \
	server/server.h \
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
	unix/glob.h \
	unix/glw_unix.h \
	unix/net_udp.c \
	unix/q_shunix.c \
	unix/qal_unix.c \
	unix/qgl_unix.c \
	unix/rw_unix.c \
	unix/rw_unix.h \
	unix/sys_unix.c \
	unix/vid_so.c


# Dedicated Server
alienarena_ded_SOURCES = \
	client/anorms.h \
	game/game.h \
	game/q_shared.c \
	game/q_shared.h \
	null/cl_null.c \
	qcommon/binheap.c \
	qcommon/binheap.h \
	qcommon/cmd.c \
	qcommon/cmodel.c \
	qcommon/common.c \
	qcommon/crc.c \
	qcommon/cvar.c \
	qcommon/files.c \
	qcommon/htable.c \
	qcommon/htable.h \
	qcommon/image.c \
	qcommon/libgarland.c \
	qcommon/libgarland.h \
	qcommon/mdfour.c \
	qcommon/net_chan.c \
	qcommon/pmove.c \
	qcommon/qcommon.h \
	qcommon/qfiles.h \
	qcommon/terrain.c \
	server/server.h \
	server/sv_ccmds.c \
	server/sv_ents.c \
	server/sv_game.c \
	server/sv_init.c \
	server/sv_main.c \
	server/sv_send.c \
	server/sv_user.c \
	server/sv_world.c \
	unix/glob.c \
	unix/glob.h \
	unix/net_udp.c \
	unix/q_shunix.c \
	unix/rw_unix.h \
	unix/sys_unix.c


# Game Module
libgame_a_SOURCES =  \
	client/anorms.h \
	game/acesrc/acebot.h \
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
	game/g_deathray.h \
	game/g_func.c \
	game/g_items.c \
	game/g_local.h \
	game/g_main.c \
	game/g_misc.c \
	game/g_monster.c \
	game/g_phys.c \
	game/g_save.c \
	game/g_spawn.c \
	game/g_spider.c \
	game/g_spider.h \
	game/g_svcmds.c \
	game/g_target.c \
	game/g_trigger.c \
	game/g_unlagged.c \
	game/g_utils.c \
	game/g_vehicles.c \
	game/g_weapon.c \
	game/game.h \
	game/m_move.c \
	game/m_player.h \
	game/p_client.c \
	game/p_hud.c \
	game/p_trail.c \
	game/p_view.c \
	game/p_weapon.c \
	game/q_shared.c \
	game/q_shared.h \
	qcommon/qfiles.h \
	unix/minizip/crypt.h \
	unix/minizip/ioapi.c \
	unix/minizip/ioapi.o \
	unix/minizip/miniunz.c \
	unix/minizip/minizip.c \
	unix/minizip/mztools.c \
	unix/minizip/mztools.h \
	unix/minizip/unzip.c \
	unix/minizip/unzip.h \
	unix/minizip/zip.c \
	unix/minizip/zip.h

if !USE_SYSTEM_LIBODE
#
# --- Custom Open Dynamics Engine Library ---
#
libode_a_SOURCES = \
	unix/odesrc/nextafterf.c \
	unix/odesrc/array.cpp \
	unix/odesrc/array.h \
	unix/odesrc/box.cpp \
	unix/odesrc/capsule.cpp \
	unix/odesrc/collision_cylinder_box.cpp \
	unix/odesrc/collision_cylinder_plane.cpp \
	unix/odesrc/collision_cylinder_sphere.cpp \
	unix/odesrc/collision_kernel.cpp \
	unix/odesrc/collision_kernel.h \
	unix/odesrc/collision_quadtreespace.cpp \
	unix/odesrc/collision_sapspace.cpp \
	unix/odesrc/collision_space.cpp \
	unix/odesrc/collision_space_internal.h \
	unix/odesrc/collision_std.h \
	unix/odesrc/collision_transform.cpp \
	unix/odesrc/collision_transform.h \
	unix/odesrc/collision_trimesh_colliders.h \
	unix/odesrc/collision_trimesh_disabled.cpp \
	unix/odesrc/collision_trimesh_internal.h \
	unix/odesrc/collision_trimesh_opcode.h \
	unix/odesrc/collision_trimesh_gimpact.h \
	unix/odesrc/collision_util.cpp \
	unix/odesrc/collision_util.h \
	unix/odesrc/common.h \
	unix/odesrc/convex.cpp \
	unix/odesrc/cylinder.cpp \
	unix/odesrc/error.cpp \
	unix/odesrc/error.h \
	unix/odesrc/export-dif.cpp \
	unix/odesrc/heightfield.cpp \
	unix/odesrc/heightfield.h \
	unix/odesrc/lcp.cpp \
	unix/odesrc/lcp.h \
	unix/odesrc/mass.cpp \
	unix/odesrc/mat.cpp \
	unix/odesrc/mat.h \
	unix/odesrc/matrix.cpp \
	unix/odesrc/matrix_impl.h \
	unix/odesrc/matrix.h \
	unix/odesrc/fastldlt.cpp \
	unix/odesrc/fastldlt_impl.h \
	unix/odesrc/fastlsolve.cpp \
	unix/odesrc/fastsolve_impl.h \
	unix/odesrc/fastltsolve.cpp \
	unix/odesrc/fastltsolve_impl.h \
	unix/odesrc/fastdot.cpp \
	unix/odesrc/fastdot_impl.h \
	unix/odesrc/memory.cpp \
	unix/odesrc/misc.cpp \
	unix/odesrc/objects.cpp \
	unix/odesrc/objects.h \
	unix/odesrc/obstack.cpp \
	unix/odesrc/obstack.h \
	unix/odesrc/ode.cpp \
	unix/odesrc/odeinit.cpp \
	unix/odesrc/odemath.cpp \
	unix/odesrc/odemath.h \
	unix/odesrc/odeou.cpp \
	unix/odesrc/odeou.h \
	unix/odesrc/odetls.h \
	unix/odesrc/plane.cpp \
	unix/odesrc/quickstep.cpp \
	unix/odesrc/quickstep.h \
	unix/odesrc/ray.cpp \
	unix/odesrc/rotation.cpp \
	unix/odesrc/sphere.cpp \
	unix/odesrc/step.cpp \
	unix/odesrc/step.h \
	unix/odesrc/timer.cpp \
	unix/odesrc/threading_atomics_provs.h \
	unix/odesrc/threading_base.cpp \
	unix/odesrc/threading_base.h \
	unix/odesrc/threading_fake_sync.h \
	unix/odesrc/threading_impl.cpp  \
	unix/odesrc/threading_impl.h \
	unix/odesrc/threading_impl_posix.h \
	unix/odesrc/threading_impl_templates.h \
	unix/odesrc/threading_impl_win.h \
	unix/odesrc/threading_pool_posix.cpp \
	unix/odesrc/threading_pool_win.cpp \
	unix/odesrc/threadingutils.h \
	unix/odesrc/typedefs.h \
	unix/odesrc/util.cpp
	unix/odesrc/util.h

#
# OPCODE related sources
#
libode_a_SOURCES += \
	unix/odesrc/collision_trimesh_opcode.cpp \
	unix/odesrc/collision_trimesh_internal.cpp \
	unix/odesrc/collision_trimesh_internal_impl.h \
	unix/odesrc/collision_trimesh_trimesh.cpp \
	unix/odesrc/collision_trimesh_trimesh_old.cpp \
	unix/odesrc/collision_trimesh_sphere.cpp \
	unix/odesrc/collision_trimesh_ray.cpp \
	unix/odesrc/collision_trimesh_box.cpp \
	unix/odesrc/collision_trimesh_ccylinder.cpp \
	unix/odesrc/collision_trimesh_internal.h \
	unix/odesrc/collision_cylinder_trimesh.cpp \
	unix/odesrc/collision_trimesh_plane.cpp \
	unix/odesrc/collision_convex_trimesh.cpp

#
# ode/joints sources
#
libode_a_SOURCES += \
	unix/odesrc/joints/amotor.cpp \
	unix/odesrc/joints/amotor.h \
	unix/odesrc/joints/ball.cpp \
	unix/odesrc/joints/ball.h \
	unix/odesrc/joints/contact.cpp \
	unix/odesrc/joints/contact.h \
	unix/odesrc/joints/dball.cpp \
	unix/odesrc/joints/dball.h \
	unix/odesrc/joints/dhinge.cpp \
	unix/odesrc/joints/dhinge.h \
	unix/odesrc/joints/fixed.cpp \
	unix/odesrc/joints/fixed.h \
	unix/odesrc/joints/hinge.cpp \
	unix/odesrc/joints/hinge.h \
	unix/odesrc/joints/hinge2.cpp \
	unix/odesrc/joints/hinge2.h \
	unix/odesrc/joints/joint.cpp \
	unix/odesrc/joints/joint.h \
	unix/odesrc/joints/joint_internal.h \
	unix/odesrc/joints/joints.h \
	unix/odesrc/joints/lmotor.cpp \
	unix/odesrc/joints/lmotor.h \
	unix/odesrc/joints/null.cpp \
	unix/odesrc/joints/null.h \
	unix/odesrc/joints/piston.cpp \
	unix/odesrc/joints/piston.h \
	unix/odesrc/joints/plane2d.cpp \
	unix/odesrc/joints/plane2d.h \
	unix/odesrc/joints/pr.cpp \
	unix/odesrc/joints/pr.h \
	unix/odesrc/joints/pu.cpp \
	unix/odesrc/joints/pu.h \
	unix/odesrc/joints/slider.cpp \
	unix/odesrc/joints/slider.h \
	unix/odesrc/joints/transmission.cpp \
	unix/odesrc/joints/transmission.h \
	unix/odesrc/joints/universal.cpp \
	unix/odesrc/joints/universal.h

#
# OPCODE/ sources
#
libode_a_SOURCES += \
	unix/odesrc/OPCODE/OPC_AABBCollider.cpp \
	unix/odesrc/OPCODE/OPC_AABBCollider.h \
	unix/odesrc/OPCODE/OPC_AABBTree.cpp \
	unix/odesrc/OPCODE/OPC_AABBTree.h \
	unix/odesrc/OPCODE/OPC_BaseModel.cpp \
	unix/odesrc/OPCODE/OPC_BaseModel.h \
	unix/odesrc/OPCODE/OPC_BoxBoxOverlap.h \
	unix/odesrc/OPCODE/OPC_Collider.cpp \
	unix/odesrc/OPCODE/OPC_Collider.h \
	unix/odesrc/OPCODE/OPC_Common.cpp \
	unix/odesrc/OPCODE/OPC_Common.h \
	unix/odesrc/OPCODE/OPC_HybridModel.cpp \
	unix/odesrc/OPCODE/OPC_HybridModel.h \
	unix/odesrc/OPCODE/OPC_IceHook.h \
	unix/odesrc/OPCODE/OPC_LSSAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_LSSCollider.cpp \
	unix/odesrc/OPCODE/OPC_LSSCollider.h \
	unix/odesrc/OPCODE/OPC_LSSTriOverlap.h \
	unix/odesrc/OPCODE/OPC_MeshInterface.cpp \
	unix/odesrc/OPCODE/OPC_MeshInterface.h \
	unix/odesrc/OPCODE/OPC_Model.cpp \
	unix/odesrc/OPCODE/OPC_Model.h \
	unix/odesrc/OPCODE/OPC_OBBCollider.cpp \
	unix/odesrc/OPCODE/OPC_OBBCollider.h \
	unix/odesrc/OPCODE/OPC_OptimizedTree.cpp \
	unix/odesrc/OPCODE/OPC_OptimizedTree.h \
	unix/odesrc/OPCODE/OPC_Picking.cpp \
	unix/odesrc/OPCODE/OPC_Picking.h \
	unix/odesrc/OPCODE/OPC_PlanesAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_PlanesCollider.cpp \
	unix/odesrc/OPCODE/OPC_PlanesCollider.h \
	unix/odesrc/OPCODE/OPC_PlanesTriOverlap.h \
	unix/odesrc/OPCODE/OPC_RayAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_RayCollider.cpp \
	unix/odesrc/OPCODE/OPC_RayCollider.h \
	unix/odesrc/OPCODE/OPC_RayTriOverlap.h \
	unix/odesrc/OPCODE/OPC_Settings.h \
	unix/odesrc/OPCODE/OPC_SphereAABBOverlap.h \
	unix/odesrc/OPCODE/OPC_SphereCollider.cpp \
	unix/odesrc/OPCODE/OPC_SphereCollider.h \
	unix/odesrc/OPCODE/OPC_SphereTriOverlap.h \
	unix/odesrc/OPCODE/OPC_TreeBuilders.cpp \
	unix/odesrc/OPCODE/OPC_TreeBuilders.h \
	unix/odesrc/OPCODE/OPC_TreeCollider.cpp \
	unix/odesrc/OPCODE/OPC_TreeCollider.h \
	unix/odesrc/OPCODE/OPC_TriBoxOverlap.h \
	unix/odesrc/OPCODE/OPC_TriTriOverlap.h \
	unix/odesrc/OPCODE/OPC_VolumeCollider.cpp \
	unix/odesrc/OPCODE/OPC_VolumeCollider.h \
	unix/odesrc/OPCODE/Opcode.cpp \
	unix/odesrc/OPCODE/Opcode.h \
	unix/odesrc/OPCODE/Stdafx.h

#
# OPCODE/Ice sources
#
libode_a_SOURCES += \
	unix/odesrc/OPCODE/Ice/IceAABB.cpp \
	unix/odesrc/OPCODE/Ice/IceAABB.h \
	unix/odesrc/OPCODE/Ice/IceAxes.h \
	unix/odesrc/OPCODE/Ice/IceBoundingSphere.h \
	unix/odesrc/OPCODE/Ice/IceContainer.cpp \
	unix/odesrc/OPCODE/Ice/IceContainer.h \
	unix/odesrc/OPCODE/Ice/IceFPU.h \
	unix/odesrc/OPCODE/Ice/IceHPoint.cpp \
	unix/odesrc/OPCODE/Ice/IceHPoint.h \
	unix/odesrc/OPCODE/Ice/IceIndexedTriangle.cpp \
	unix/odesrc/OPCODE/Ice/IceIndexedTriangle.h \
	unix/odesrc/OPCODE/Ice/IceLSS.h \
	unix/odesrc/OPCODE/Ice/IceMatrix3x3.cpp \
	unix/odesrc/OPCODE/Ice/IceMatrix3x3.h \
	unix/odesrc/OPCODE/Ice/IceMatrix4x4.cpp \
	unix/odesrc/OPCODE/Ice/IceMatrix4x4.h \
	unix/odesrc/OPCODE/Ice/IceMemoryMacros.h \
	unix/odesrc/OPCODE/Ice/IceOBB.cpp \
	unix/odesrc/OPCODE/Ice/IceOBB.h \
	unix/odesrc/OPCODE/Ice/IcePairs.h \
	unix/odesrc/OPCODE/Ice/IcePlane.cpp \
	unix/odesrc/OPCODE/Ice/IcePlane.h \
	unix/odesrc/OPCODE/Ice/IcePoint.cpp \
	unix/odesrc/OPCODE/Ice/IcePoint.h \
	unix/odesrc/OPCODE/Ice/IcePreprocessor.h \
	unix/odesrc/OPCODE/Ice/IceRandom.cpp \
	unix/odesrc/OPCODE/Ice/IceRandom.h \
	unix/odesrc/OPCODE/Ice/IceRay.cpp \
	unix/odesrc/OPCODE/Ice/IceRay.h \
	unix/odesrc/OPCODE/Ice/IceRevisitedRadix.cpp \
	unix/odesrc/OPCODE/Ice/IceRevisitedRadix.h \
	unix/odesrc/OPCODE/Ice/IceSegment.cpp \
	unix/odesrc/OPCODE/Ice/IceSegment.h \
	unix/odesrc/OPCODE/Ice/IceTriangle.cpp
	unix/odesrc/OPCODE/Ice/IceTriangle.h \
	unix/odesrc/OPCODE/Ice/IceTriList.h \
	unix/odesrc/OPCODE/Ice/IceTypes.h \
	unix/odesrc/OPCODE/Ice/IceUtils.cpp \
	unix/odesrc/OPCODE/Ice/IceUtils.h

#
# ODE OU utilities
#
libode_a_SOURCES += \
	unix/odesrc/ousrc/atomic.cpp \
	unix/odesrc/ousrc/customization.cpp \
	unix/odesrc/ousrc/malloc.cpp \
	unix/odesrc/ousrc/threadlocalstorage.cpp

libode_a_SOURCES += \
	unix/odesrc/ou/assert.h \
	unix/odesrc/ou/atomicflags.h \
	unix/odesrc/ou/atomic.h \
	unix/odesrc/ou/customization.h \
	unix/odesrc/ou/enumarrays.h \
	unix/odesrc/ou/features.h \
	unix/odesrc/ou/flagsdefines.h \
	unix/odesrc/ou/flags.h \
	unix/odesrc/ou/inttypes.h \
	unix/odesrc/ou/macros.h \
	unix/odesrc/ou/malloc.h \
	unix/odesrc/ou/namespace.h \
	unix/odesrc/ou/platform.h \
	unix/odesrc/ou/simpleflags.h \
	unix/odesrc/ou/templates.h \
	unix/odesrc/ou/threadlocalstorage.h \
	unix/odesrc/ou/typewrapper.h

endif
