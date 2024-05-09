#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.62])
AC_INIT(tinyxml, 2.6.1, www.sourceforge.net/projects/tinyxml)
AC_CONFIG_SRCDIR([tinystr.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
LT_INIT([disable-static pic-only])

AC_LANG(C++)

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([memmove strchr])

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Turn on debugging]),
[case "${enableval}" in
    yes) debug=true ;;
    no)  debug=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],[debug=false])

AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

AC_ARG_ENABLE([profile],
	      AS_HELP_STRING([--enable-profiling], [Turn on profiling]),
[case "${enableval}" in
    yes) profile=true ;;
    no)  profile=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --enable-profiling]) ;;
    esac],[profile=false])

AM_CONDITIONAL([PROFILE], [test x$profile = xtrue])

AC_ARG_ENABLE([stl],
	       AS_HELP_STRING([--disable-stl], [Don't use STL [default is use]]),
[case "${enableval}" in
    yes) tyx_use_stl=true ;;
    no)  tyx_use_stl=false ;;
    *) AC_MSG_ERROR([bad value ${enableval} for --disable-stl]) ;;
    esac],[tyx_use_stl=true])

AM_CONDITIONAL([TINYXML_USE_STL], [test x$tyx_use_stl = xtrue])

case $tyx_use_stl in
    true) TIXML_USE_STL=use_stl_def ;;
    *) TIXML_USE_STL=/dev/null ;;
esac

AC_SUBST_FILE(TIXML_USE_STL)

AC_CONFIG_FILES([Makefile tinyxml.h])
AC_OUTPUT
