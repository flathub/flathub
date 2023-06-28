# Process this file with autoconf to produce a configure script.
#
# Alien Arena auto configuration source
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
# A copy of the GNU General Public License is included;
# see the file, GPLv2
#

AC_PREREQ([2.69])
AC_INIT([alienarena],[7.71.4],[alienrace@comcast.net])

AC_CONFIG_SRCDIR([source/ref_gl/r_image.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config/config.h])

dnl foreign = do not apply GNU doc requirements
dnl subdir-objects = compile objects into their sub directories
dnl silent-rules = less output, default off
dnl one might add dist-zip, dist-bzip2 for other distributions besides tar.gz
AM_INIT_AUTOMAKE([1.15 foreign subdir-objects silent-rules])
AM_SILENT_RULES([no])

dnl use --enable-maintainer-mode to get full set of make targets.
dnl autotools generated files will not be updated otherwise.
dnl without --enable-maintainer-mode, autotools programs are
dnl not needed to build the program, which is better for
dnl non-developers who build from repository sources.
AM_MAINTAINER_MODE

dnl
dnl Options for specifying whether to include "nested" data1 package
dnl  for make install and make distcheck. The default is to include
dnl  data1.
dnl
AC_MSG_CHECKING([whether to include game resources package])
AC_ARG_WITH([data1pkg],
	[AS_HELP_STRING([--with(out)-data1pkg],
			[include game resources from data1 directory (default:with)])
		],[
		with_data1pkg=${withval}],[
		with_data1pkg=yes
])
AS_IF([ test "x${with_data1pkg}" != xno ],[
	AC_CONFIG_SUBDIRS([data1])],[
	])
AC_MSG_RESULT([${with_data1pkg}])
AM_CONDITIONAL([WITH_DATA1PKG],[ test "x${with_data1pkg}" = xyes ])

#
# Checks for programs.
#
AC_PROG_AWK
AC_PROG_SED
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

dnl to avoid misleading error messages later
AS_IF([ test "x${PKG_CONFIG}" = x ],
	[AC_MSG_ERROR([required pkg-config utility not found])],
	[])

#
# Check host environment
#
AC_CANONICAL_HOST

unix_host=generic
AS_CASE([ ${host} ],
	[ *-*-darwin*],
		[
		unix_host=darwin
		],
	[ *-*-linux* ],
		[
		unix_host=linux
		])


dnl
dnl host-dependent conditional compile and version string settings for config.h
dnl
AC_DEFINE([UNIX_VARIANT],1,[unix-specific conditional compile])

AS_CASE([${unix_host}],
	[ darwin ],
		[
		AC_DEFINE([DARWIN_SPECIAL_CASE],1,[darwin-specific conditional compile])
		AC_DEFINE([_DARWIN_UNLIMITED_SELECT],1,
				[see select (2) man page for Mac OS X])
		],
	[ linux ],
		[AC_DEFINE([LINUX_SPECIAL_CASE],1,
				[linux-specific conditional compile])],
	[ generic ]
		[AC_DEFINE([GENERIC_UNIX],1,[non-specific unix conditional compile])
])

AC_DEFINE_UNQUOTED([CPUSTRING], "${host_cpu}", [Canonical CPU identification])
AC_DEFINE_UNQUOTED([BUILDSTRING], "${host_os}", [Canonical OS identification])

dnl ---- Darwin search for OpenGL library
dnl  (ref: dlopen(3) for Mac OS X.)
dnl  See Darwin notes above.
gl_dlopen_path=default
AS_IF([ test "x${unix_host}" = xdarwin ],[
	AC_MSG_CHECKING([for GL_LIBDIR (location of libGL.dylib)])
	AC_ARG_VAR( GL_LIBDIR, [directory containing libGL.dylib])
	AC_MSG_RESULT([${GL_LIBDIR}])
	AS_IF([ test "x${GL_LIBDIR}" = x ],[
		gl_dlopen_path=${x_libraries}],[
		gl_dlopen_path=${GL_LIBDIR}])
])

dnl
dnl Set names of dlopened libraries for config.h
dnl The Darwin configuration attempts to conform to MacPorts.
dnl   The maintainer at MacPorts is ryandesign@macports
dnl   The dependency checks are below, under the BUILD_CLIENT conditional
dnl
AS_CASE([ ${unix_host} ],
	[ darwin ],
	  [AS_IF([ test "x${gl_dlopen_path}" = xdefault ],
			[AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
				["libGL.dylib"],
				[OpenGL dynamic lib name])
			],
			[AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
				["${gl_dlopen_path}/libGL.dylib"],
				[OpenGL dynamic lib path])
		])
		AC_DEFINE_UNQUOTED([OPENAL_DRIVER],
			["/System/Library/Frameworks/OpenAL.framework/OpenAL"],
			[OpenAL dynamic lib path])
		],[
		AC_DEFINE_UNQUOTED([OPENGL_DRIVER],
			["libGL.so.1"],[OpenGL shared lib name])
		AC_DEFINE_UNQUOTED([OPENAL_DRIVER],
			["libopenal.so.1"],[OpenAL shared lib name])
	])

#
# Check for MAKE options
#

dnl
dnl Dedicated server only build option.
dnl So dedicated server may be built without video/audio dependencies.
dnl
AC_MSG_CHECKING([whether to build the client])
AC_ARG_ENABLE([client],
	[AS_HELP_STRING([--disable-client],
		[build dedicated server only, not the client (default:no)])],
	[build_client=${enableval}],
	[build_client=yes])
AC_MSG_RESULT([${build_client}])
AM_CONDITIONAL([BUILD_CLIENT],[ test "x${build_client}" = xyes ])

dnl
dnl Option for documentation installation
dnl Note: distros may have custom document install methods
dnl
AC_MSG_CHECKING([whether to install documents])
AC_ARG_ENABLE([doc_install],
	[AS_HELP_STRING([--enable-doc-install],
		[Enable document installation (default: yes)])],
	[doc_install=${enableval}],
	[doc_install=yes])
AC_MSG_RESULT([${doc_install}])
AM_CONDITIONAL([DOC_INSTALL],[ test "x${doc_install}" = xyes ])

dnl
dnl Option for icon installation
dnl
AC_MSG_CHECKING([whether to install icons])
AC_ARG_ENABLE([icons_install],
	[AS_HELP_STRING([--enable-icons-install],
		[Enable icons installation (default: yes)])],
	[icons_install=${enableval}],
	[icons_install=yes])
AC_MSG_RESULT([${icons_install}])
AM_CONDITIONAL([ICONS_INSTALL],[ test "x${icons_install}" = xyes ])

AC_ARG_WITH(iconsdir,
	[AS_HELP_STRING([--with-iconsdir=DIR],
		[icons installation directory (default:DATADIR/icons)])],
	[with_iconsdir=${withval}],
	[with_iconsdir=${datadir}/icons])
AC_SUBST([iconsdir], [${with_icondir}])

dnl
dnl ODE (Open Dynamic Engine) Library
dnl
dnl if no option or "--without-system-libode" is specified, use the recommended
dnl  integrated ODE.
dnl if "--with-system-libode" is specified, check. look under BUILD_CLIENT
dnl  for details.
dnl
AC_ARG_WITH([system-libode],
	[AS_HELP_STRING([--with(out)-system-libode],
		[use system-supplied libode (default:without)])],
	[system_libode=${withval}],
	[system_libode=no])

#
# Checks for libraries.
#

dnl ---- math
AC_SEARCH_LIBS([acos],[m])

dnl ---- dynamic
AC_SEARCH_LIBS([dlopen],[dl])

dnl ---- real time
AC_SEARCH_LIBS([clock_gettime],[rt])

dnl
dnl ---- start of client-only library requirements
dnl
AM_COND_IF([BUILD_CLIENT],[

dnl ---- pthread (client)
AX_PTHREAD([],
	[AC_MSG_ERROR(
		[required pthreads support not found (or invalid CFLAGS)])])
dnl This might be a workaround for DSO missing pthread library error:
AS_IF([ test "x${PTHREAD_LIBS}" = x ],[PTHREAD_LIBS="-pthread"])


dnl ----  X11 (client)
PKG_CHECK_MODULES([X11],[x11],,[
	AC_MSG_ERROR([required X11 library not found])
])

dnl ---- xf86vm (client)
dnl XF86 VidMode extension
dnl  Note: supports full-screen display. if it is not installed, the
dnl   build will succeed but full-screen will not work.
dnl
dnl If no option is specified, then check for the library and act accordingly.
dnl If "--without-xf86vm" is specified, do not check, and disable the library.
dnl If "--with-xf86vm" is specified, check and bail out if it cannot be found.
dnl
AC_ARG_WITH([xf86vm],
	[AS_HELP_STRING([--with(out)-xf86vm],
			[include XF86 VidMode support (default: check)])],
	[],
	[with_xf86vm=check])
have_Xxf86vm=no
AS_IF([ test "x${with_xf86vm}" != xno ],[
	PKG_CHECK_MODULES([XXF86VM],[xxf86vm],[
		have_Xxf86vm=yes
		AC_DEFINE([HAVE_XXF86VM],1,[Enable X86 VidMode support])
	],[
		have_Xxf86vm=no
		AS_IF([ test "x${with_xf86vm}" = xyes ],[
			AC_MSG_FAILURE(
				[--with-xf86vm specified, library or headers found])
		])
	])
])

dnl ---- xf86dga (client)
dnl XF86 DGA extension
dnl  Note: this is changed to default to "without",
dnl    so "--with-xf86dga" must be specified to use this library
dnl
dnl If no option is specified, disable the library.
dnl If "--without-xf86dga" is specified, do not check, and disable the library.
dnl If "--with-xf86dga" is specified, check and bail out if it cannot be found.
dnl
	AC_ARG_WITH([xf86dga],
		[AS_HELP_STRING([--with(out)-xf86dga],
			[include XF86 DGA support (default: without)])],
		[],
		[with_xf86dga=no])
	have_Xxf86dga=no
	AS_IF([ test "x$with_xf86dga" != xno ],[
		PKG_CHECK_MODULES([XXF86DGA],[xxf86dga],[
			have_Xxf86dga=yes
			AC_DEFINE([HAVE_XXF86DGA],1,[Enable X86 DGA support])
			AC_CHECK_HEADERS([X11/extensions/Xxf86dga.h],[],[],[
#include <X11/Xlib.h>
#include <X11/Xatom.h>
				])
			])
		],[
		have_Xxf86dga=no
		AS_IF([ test "x${with_xf86dga}" = xyes ],[
			AC_MSG_FAILURE(
				[--with-xf86dga specified, library or headers not found])
		])
	])

dnl ---- zlib (client)
dnl If no option is specified, then check for the library and act accordingly.
dnl If "--without-zlib" is specified, do not check, and disable the library.
dnl If "--with-zlib" is specified, check and fail if it cannot be found.
dnl
AC_ARG_WITH([zlib],
	[AS_HELP_STRING([--with(out)-zlib],
			[include Zlib compression support (default: check)])],
	[],
	[with_zlib=check])
have_zlib=no
AS_IF([ test "x${with_zlib}" != xno ],[
	PKG_CHECK_MODULES([ZLIB],[zlib],[
		have_zlib=yes
		AC_DEFINE([HAVE_ZLIB],1,[Enable ZLib support])
	],[
		have_zlib=no
		AS_IF([ test "x${with_zlib}" = xyes ],[
			AC_MSG_FAILURE(
				[--with-zlib specified, library or headers not found])
		])
	])
])

dnl ---- libjpeg (client)
AC_SEARCH_LIBS([jpeg_read_header],[jpeg],,[
	AC_MSG_ERROR([required jpeg library not found])
])

dnl ---- libcurl (client)
PKG_CHECK_MODULES([LIBCURL],[libcurl],,[
	AC_MSG_ERROR([required cURL library not found])
])

dnl ---- ogg, vorbis, vorbisfile (client)
PKG_CHECK_MODULES([OGG],[ogg],,[
	AC_MSG_ERROR([required Ogg libraries not found])
])

PKG_CHECK_MODULES([VORBIS],[vorbis vorbisfile],,[
	AC_MSG_ERROR([required Vorbis libraries not found])
])

dnl ---- freetype2 (client)
PKG_CHECK_MODULES([FREETYPE2],[freetype2],,[
	AC_MSG_ERROR([required FreeType2 library not found])
])

dnl
dnl if option to use system libode, rather than integrated ODE, is selected
dnl  use pkg-config to get cflags and libs. do not default to 
dnl  integrated ODE on failure, as that is probably not what is wanted.
dnl
dnl for recommended integrated ODE build, setup client dependencies,
dnl  and configuration. Double precision math is "hardwired" here and
dnl  is not considered to be a user-specifiable option.
dnl
AS_IF([ test "x$system_libode" = "xyes" ],[
	PKG_CHECK_MODULES([ODE],[ode],,[
	AC_MSG_FAILURE(
	[--with-system-libode specified, but the Open Dynamics Engine library could not be found])
	])
],[
AC_SEARCH_LIBS([new_handler],[stdc++])
dnl
dnl config.h hacks from ODE configuration
dnl intP is an integer type that can type convert to/from a pointer
dnl TODO: verify what type to use
dnl
	AH_BOTTOM([
typedef int intP;
#define dEpsilon DBL_EPSILON
	])
])

])
dnl
dnl ---- end of client-only library requirements
dnl

#
# Checks for header files
#
dnl see 5.6.2 Particular Header Checks
dnl
dnl  note: if the source code needs ifdef's for including some .h,
dnl  then a check is required. realize that some source files may
dnl  unconditionally include some .h files.
dnl

AC_HEADER_STDBOOL
AC_HEADER_DIRENT
AC_HEADER_ASSERT

AC_CHECK_HEADERS([unistd.h stdarg.h],[],[],[])
AC_CHECK_HEADERS([dlfcn.h],[],[],[])
AC_CHECK_HEADERS([sys/time.h time.h],[],[],[])
AC_CHECK_HEADERS([string.h strings.h ctype.h],[],[],[])
AC_CHECK_HEADERS([signal.h],[],[],[])
AC_CHECK_HEADERS([setjmp.h],[],[],[])
AC_CHECK_HEADERS([math.h float.h],[],[],[])
AC_CHECK_HEADERS([ \
	sys/socket.h \
	netinet/in.h  \
	netdb.h \
	sys/param.h \
	sys/ioctl.h \
	sys/uio.h \
])

AC_CHECK_HEADERS([ \
	arpa/inet.h \
	fcntl.h \
	inttypes.h \
	limits.h \
	malloc.h \
	memory.h \
	termios.h \
	sys/mman.h
])

dnl
dnl ---- start of client-only header requirements
dnl
AM_COND_IF([BUILD_CLIENT],[

dnl ---- jpeg (client)
jpeglib_h_found=no
AC_CHECK_HEADERS([jpeglib.h jpeg/jpeglib.h],[ jpeglib_h_found=yes ])
AS_IF([ test "x${jpeglib_h_found}" = xno ],
	[AC_MSG_ERROR([required JPEG header file, jpeglib.h, not found])])

dnl ---- OpenAL (client)
openal_h_found=no
AC_CHECK_HEADERS([AL/al.h al.h OpenAL/al.h],[ openal_h_found=yes ])
AS_IF([ test "x${openal_h_found}" = xno ],
	[AC_MSG_ERROR([required OpenAL header file, al.h, not found])])

openalc_h_found=no
AC_CHECK_HEADERS([AL/alc.h alc.h OpenAL/alc.h],[ openalc_h_found=yes ])
AS_IF([ test "x${openalc_h_found}" = xno ],
	[AC_MSG_ERROR([required OpenAL header file, alc.h, not found])])

dnl ---- OpenGL (client)
AC_CHECK_HEADERS([GL/gl.h],,
	[AC_MSG_ERROR([required OpenGL header file, gl.h, not found])])
AC_CHECK_HEADERS([GL/glx.h],,
	[AC_MSG_ERROR([required OpenGL header file, glx.h, not found])])

dnl ---- ODE (client)
AS_IF([ test "x$system_libode" = "xyes"],[ 
	AC_CHECK_HEADERS([ode/ode.h],,
		[AC_MSG_ERROR([with-system-libode: required ODE header file, ode.h, not found])]
		[],
		[
#define dDOUBLE
		])
	])
])

AM_CONDITIONAL([USE_SYSTEM_LIBODE],[ test "x$system_libode" = "xyes" ])

dnl
dnl ------------------------ end of client-only header requirements
dnl

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_TYPES([uintptr_t])

#
# Checks for library functions.
#
AC_FUNC_ERROR_AT_LINE
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_OBSTACK

AC_CHECK_FUNCS([vsnprintf])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([nanosleep usleep])
AC_CHECK_FUNCS([clock_gettime floor getcwd gethostbyname gettimeofday \
  isascii memmove memset mkdir munmap pow putenv select socket sqrt \
  strchr strrchr strstr strtoul])
AC_CHECK_FUNCS([stat fstat filelength strlcat strlcpy])

dnl these are windows specials for manual win32 config.h
AC_CHECK_FUNCS([ _begin_thread _getcwd _mkdir _putenv _strdup _stricmp \
   _strnicmp unlink _unlink ])

dnl winsock
AC_CHECK_FUNCS([closesocket])

dnl termios headers and functions may not be needed.
AC_SYS_POSIX_TERMIOS

dnl
dnl required gcc options.
dnl  fast-math because strict adherence to IEEE standard is not needed.
dnl  no-strict-aliasing because strict aliasing is not adhered to in places.
dnl  -fcommon because -fnocommon became the default and results into duplicate definitions
dnl
AX_APPEND_FLAG([-ffast-math],[CFLAGS])
AX_APPEND_FLAG([-fno-strict-aliasing],[CFLAGS])
AX_APPEND_FLAG([-fcommon],[CFLAGS])

#
# Outputs
#
AC_CONFIG_FILES([
		Makefile
		source/Makefile
])

AC_OUTPUT

#
# Status message
#
dnl Option to disable this message.
AC_ARG_ENABLE([build-status],[AS_HELP_STRING([--disable-build-status],
			[hide the status message at the end of the configuration script (defdault:no)])],
			[hide_status=${enableval}],[hide_status=no])

AS_IF([ test "x${hide_status}" = xno ],[
	AS_ECHO()
	AS_ECHO(["-------------------------------------------"])
	AS_ECHO(["Package: ${PACKAGE_NAME} Version: ${PACKAGE_VERSION}"])
	AS_ECHO([" OS: .................... ${host_os}"])
	AS_ECHO([" CPU: ................... ${host_cpu}"])
	AS_IF([ test "x${build_client}" = xyes ],[
			dedicated_only=no],[
			dedicated_only=yes])
	AS_ECHO([" Dedicated Only: ........ ${dedicated_only}"])

	AM_COND_IF([BUILD_CLIENT],[
		AS_ECHO([" XF86 DGA support: ...... ${have_Xxf86dga}"])
		AS_ECHO([" XF86 VidMode support: .. ${have_Xxf86vm}"])
		AS_ECHO([" Zlib support: .......... ${have_zlib}"])
		AS_IF([ test "x${unix_host}" = xdarwin ],[
			AS_ECHO([" GL_LIBDIR: ............. ${gl_dlopen_path}"])
			])
	])
	AS_ECHO([" prefix ................ ${prefix}"])
	AS_ECHO([" doc-install ........... ${doc_install}"])
	AS_ECHO([" icons-install: ........ ${icons_install}"])
	AS_ECHO([" with data1: ........... ${with_data1pkg}"])
	AS_ECHO([" assertions enabled: ... ${ac_enable_assert}"])
	AS_ECHO()
	AS_ECHO(["Advanced Build Information:"])
	AS_ECHO([" See README for information and recommendations."])
	AS_ECHO([" Run ./configure --help for configure options."])
	AS_ECHO([" Compiler options:"])
	AS_ECHO(["  CFLAGS:.... ${CFLAGS}"])
	AS_ECHO()
	AS_ECHO(["X11:"])
	AS_ECHO(["  cflags: ${X11_CFLAGS} ${XXF86VM_CFLAGS} ${XXF86DGA_CFLAGS}"])
	AS_ECHO(["  libs:   ${X11_LIBS} ${XXF86VM_LIBS} ${XXF86DGA_LIBS}"])
	AS_ECHO(["FREETYPE2:"])
	AS_ECHO(["  cflags: ${FREETYPE2_CFLAGS}"])
	AS_ECHO(["  libs: ${FREETYPE2_LIBS}"])
	AS_ECHO(["LIBCURL:"])
	AS_ECHO(["  cflags: ${LIBCURL_CFLAGS}"])
	AS_ECHO(["  libs: ${LIBCURL_LIBS}"])
	AS_IF([ test "x$system_libode" = "xyes"],[
		AS_ECHO(["ODE:"])
		AS_ECHO(["  cflags: ${ODE_CFLAGS}"])
		AS_ECHO(["  libs: ${ODE_LIBS}"])
	])
	AS_ECHO(["OGG:"])
	AS_ECHO(["  cflags: ${OGG}"])
	AS_ECHO(["  libs: ${OGG_LIBS}"])
	AS_ECHO(["VORBIS:"])
	AS_ECHO(["  cflags: ${VORBIS_CFLAGS}"])
	AS_ECHO(["  libs: ${VORBIS_LIBS}"])
	AS_ECHO(["ZLIB:"])
	AS_ECHO(["  cflags: ${ZLIB_CFLAGS}"])
	AS_ECHO(["  libs: ${ZLIB_LIBS}"])
	AS_ECHO()
])
