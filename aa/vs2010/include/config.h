/*
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

/*

config.h for Alien Arena Microsoft Visual C/C++ Version

Manual edit based on GNU Autotools autoheader generated
config.h.in and config.h from GNU/Linux and MinGW/MSys

IMPORTANT: When the version changes, the version string needs to be updated
in this file.

CODING CONVENTION: The idea is to avoid using the compiler pre-defined macros
for system-specific variations. Macros for supported features are used.
Where this is does not work or is inconvenient, using the WIN32_VARIANT,
LINUX_VARIANT macros is preferred.

Autoconf and autoheader generate a lot of HAVE_THIS and HAVE_THAT. Here, it is
only necessary to define HAVE_SUCH where it makes a difference. For instance,
where the location of header files differs between systems.

*/

/* this is not Canonical OS identification */
#define BUILDSTRING "Win32"

/* this is not Canonical CPU identification, probably always 'x86' */
#if defined _M_X64
#define CPUSTRING "x86-64"
#elif defined _M_IA64
#define CPUSTRING "IA-64"
#elif defined _M_IX86
#define CPUSTRING "x86"
#endif

/* Steam version */
// Enable for the steam build
// #define STEAM_VARIANT 1

/* Version number of package. UPDATE ON VERSION CHANGE */
#define VERSION "7.71.4"

/* OpenAL dll name */
#define OPENAL_DRIVER "OpenAL32.dll"

/* OpenGL DLL name */
#define OPENGL_DRIVER "opengl32.dll"

/* win32 specific conditional compile */
#define WIN32_VARIANT 1

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <direct.h> header file */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <GL/glu.h> header file. */
#define HAVE_GL_GLU_H 1

/* Define to 1 if you have the <GL/gl.h> header file. */
#define HAVE_GL_GL_H 1

/* for OpenAL */
#define AL_NO_PROTOTYPES 1

/* Define to 1 if you have the <alc.h> header file. */
#define HAVE_ALC_H 1

/* Define to 1 if you have the <al.h> header file. */
#define HAVE_AL_H 1

/* Define to 1 if you have the <curl/curl.h> header file. */
#define HAVE_CURL_CURL_H 1

/* Define to 1 if you have the <vorbis/vorbisfile.h> header file. */
#define HAVE_VORBIS_VORBISFILE_H 1

/* Enable ZLib support (for CRX Engine, not cURL) */
/* #define HAVE_ZLIB 1  UNCOMMENT THIS WHEN READY */

/* Define to 1 if you have the <jpeg/jpeglib.h> header file. */
#define HAVE_JPEG_JPEGLIB_H 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the `closesocket' function. */
#define HAVE_CLOSESOCKET 1

/* Define to 1 if you have the `stricmp' function. */
#define HAVE_STRICMP 1

/* Define to 1 if you have the `strnicmp' function. */
#define HAVE_STRNICMP 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/*
  Handling deprecated function names for Posix functions:

  For a function that should have the alternate name (because it is
  Posix OS-dependent, and not ISO C)

  #if defined HAVE_SOMEPOSIXFUNCTION && !defined HAVE__SOMEPOSIXFUNCTION
  #define _someposixfunction someposixfunction
  #endif

  If both are available then not a problem. Except configure.ac needs to
  check for both on systems that have Autotools configuration.

  For MSVC, #define HAVE__FUNC 1 is sufficient.
*/

/* Define to 1 if you have the `_putenv' function. */
#define HAVE__PUTENV 1

/* Define to 1 if you have the `_strdup' function. */
#define HAVE__STRDUP 1

/* Define to 1 if you have the `_stricmp' function. */
#define HAVE__STRICMP 1

/* Define to 1 if you have the `_unlink' function. */
#define HAVE__UNLINK 1

/* Define to 1 if you have the `_getcwd` function */
#define HAVE__GETCWD 1

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline
#endif


/* ====== TBD =======*/

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to `int' if <sys/types.h> doesn't define. */
#define gid_t int

/* Define to `int' if <sys/types.h> doesn't define. */
#define uid_t int




/* ======== probable extraneous stuff  ==========*/

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `gethostbyname' function. */
/* #undef HAVE_GETHOSTBYNAME */

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1


/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

/* Define to 1 if you have a working `mmap' system call. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the `mremap' function. */
/* #undef HAVE_MREMAP */

/* Define to 1 if you have the `munmap' function. */
/* #undef HAVE_MUNMAP */

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if you have the `putenv' function. */
#define HAVE_PUTENV 1

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC 1

/* Define to 1 if you have the `select' function. */
/* #undef HAVE_SELECT */

/* Define to 1 if you have the `socket' function. */
/* #undef HAVE_SOCKET */

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `stricmp' function. */
#define HAVE_STRICMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
/* #undef HAVE_STRNCASECMP 1 */

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vt.h> header file. */
/* #undef HAVE_SYS_VT_H */

/* Define to 1 if you have the <termios.h> header file. */
/* #undef HAVE_TERMIOS_H */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H 1 */

/* Define to 1 if you have the <X11/cursorfont.h> header file. */
/* #undef HAVE_X11_CURSORFONT_H */

/* Define to 1 if you have the <X11/extensions/xf86dga.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86DGA_H */

/* Define to 1 if you have the <X11/extensions/xf86vmode.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86VMODE_H */

/* Define to 1 if you have the <X11/extensions/Xxf86dga.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XXF86DGA_H */

/* Define to 1 if you have the <X11/keysym.h> header file. */
/* #undef HAVE_X11_KEYSYM_H */

/* Define to 1 if you have the <X11/Xatom.h> header file. */
/* #undef HAVE_X11_XATOM_H */

/* Define to 1 if you have the <X11/Xlib.h> header file. */
/* #undef HAVE_X11_XLIB_H */

/* Define to 1 if the system has the type `_Bool'. */
//#define HAVE__BOOL 1


/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
//#define PACKAGE "alienarena"

/* Define to the address where bug reports for this package should be sent. */
//#define PACKAGE_BUGREPORT "alienrace@comcast.net"

/* Define to the full name of this package. */
//#define PACKAGE_NAME "alienarena"

/* Define to the full name and version of this package. */
//#define PACKAGE_STRING "alienarena 7.41.x1"

/* Define to the one symbol short name of this package. */
//#define PACKAGE_TARNAME "alienarena"

/* Define to the home page for this package. */
//#define PACKAGE_URL ""

/* Define to the version of this package. */
//#define PACKAGE_VERSION "7.45.vs2010"


/* unix specific conditional compile */
/* #undef UNIX_VARIANT */



/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */


