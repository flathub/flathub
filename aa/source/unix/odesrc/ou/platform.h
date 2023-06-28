/*************************************************************************
 *                                                                       *
 * ODER's Utilities Library. Copyright (C) 2008 Oleh Derevenko.          *
 * All rights reserved.  e-mail: odar@eleks.com (change all "a" to "e")  *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 3 of the License, or (at    *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE-LESSER.TXT. Since LGPL is the extension of GPL     *
 *       the text of GNU General Public License is also provided for     *
 *       your information in file LICENSE.TXT.                           *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *   (3) The zlib/libpng license that is included with this library in   *
 *       the file LICENSE-ZLIB.TXT                                       *
 *                                                                       *
 * This library is distributed WITHOUT ANY WARRANTY, including implied   *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.      *
 * See the files LICENSE.TXT and LICENSE-LESSER.TXT or LICENSE-BSD.TXT   *
 * or LICENSE-ZLIB.TXT for more details.                                 *
 *                                                                       *
 *************************************************************************/

#ifndef __OU_PLATFORM_H_INCLUDED
#define __OU_PLATFORM_H_INCLUDED


//////////////////////////////////////////////////////////////////////////
// Target definitions

#define _OU_TARGET_OS_GENUNIX   1
#define _OU_TARGET_OS_WINDOWS   2
#define _OU_TARGET_OS_QNX       3
#define _OU_TARGET_OS_MAC       4
#define _OU_TARGET_OS_AIX       5
#define _OU_TARGET_OS_SUNOS     6
#define _OU_TARGET_OS_IOS       7

#define _OU_TARGET_OS__MAX      8


#define _OU_TARGET_BITS_32		1
#define _OU_TARGET_BITS_64		2

#define _OU_TARGET_BITS__MAX	3


#define _OU_TARGET_ARCH_OTHER   1
#define _OU_TARGET_ARCH_X86     2
#define _OU_TARGET_ARCH_IA64    3
#define _OU_TARGET_ARCH_X64     4
#define _OU_TARGET_ARCH_POWERPC 5
#define _OU_TARGET_ARCH_SPARC   6
#define _OU_TARGET_ARCH_ARM     7

#define _OU_TARGET_ARCH__MAX    8


//////////////////////////////////////////////////////////////////////////

#if !defined(_OU_TARGET_OS)


#if defined(_WINDOWS) || defined(_WIN32)

#define _OU_TARGET_OS			_OU_TARGET_OS_WINDOWS


#elif defined(__QNX__)

#define _OU_TARGET_OS			_OU_TARGET_OS_QNX


#elif defined(TARGET_OS_IPHONE)
 
#define _OU_TARGET_OS			_OU_TARGET_OS_IOS

#if !defined(MAC_OS_X_VERSION)
#define MAC_OS_X_VERSION	1050
#endif


#elif defined(__APPLE__)

#define _OU_TARGET_OS			_OU_TARGET_OS_MAC


#elif defined(__aix__)

#define _OU_TARGET_OS			_OU_TARGET_OS_AIX


#elif defined(__sun__)

#define _OU_TARGET_OS			_OU_TARGET_OS_SUNOS


#elif defined(__unix__)

#define _OU_TARGET_OS			_OU_TARGET_OS_GENUNIX


#else // if no known define found

#error Build target is not supported


#endif // Target OS definitions


#else // #if defined(_OU_TARGET_OS)

#if _OU_TARGET_OS <= 0 || _OU_TARGET_OS >= _OU_TARGET_OS__MAX

#error Please define a valid value for _OU_TARGET_OS


#endif // #if _OU_TARGET_OS <= 0 || _OU_TARGET_OS >= _OU_TARGET_OS__MAX


#endif // #if defined(_OU_TARGET_OS)


#if _OU_TARGET_OS == _OU_TARGET_OS_MAC

#if !defined(MAC_OS_X_VERSION)

#error Please defile preprocessor symbol MAC_OS_X_VERSION in command line (e.g. "-DMAC_OS_X_VERSION=1050" for MacOS 10.5)


#endif // #if !defined(MAC_OS_X_VERSION)


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_MAC


//////////////////////////////////////////////////////////////////////////

#if !defined(_OU_TARGET_BITS)


#if defined(_LP64) || defined(_WIN64)

#define _OU_TARGET_BITS			_OU_TARGET_BITS_64


#else // #if !defined(_LP64)

#define _OU_TARGET_BITS			_OU_TARGET_BITS_32


#endif // #if !defined(_LP64)


#else // #if defined(_OU_TARGET_BITS)

#if _OU_TARGET_BITS <= 0 || _OU_TARGET_BITS >= _OU_TARGET_BITS__MAX

#error Please define a valid value for _OU_TARGET_BITS


#endif // #if _OU_TARGET_BITS <= 0 || _OU_TARGET_BITS >= _OU_TARGET_BITS__MAX


#endif // #if defined(_OU_TARGET_BITS)


//////////////////////////////////////////////////////////////////////////

#if !defined(_OU_TARGET_ARCH)


#if defined(__i386__) || defined(_M_IX86)

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_X86


#elif defined(__ia64__) || defined(_M_IA64)

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_IA64


#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_X64


#elif defined(__ppc__)

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_POWERPC


#elif defined(__sparc__)

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_SPARC


#elif defined(TARGET_OS_IPHONE)
 
#define _OU_TARGET_ARCH         _OU_TARGET_ARCH_ARM


#else // Unknown architecture

#define _OU_TARGET_ARCH			_OU_TARGET_ARCH_OTHER


#endif // Architecture definitions


#else // #if defined(_OU_TARGET_ARCH)

#if _OU_TARGET_ARCH <= 0 || _OU_TARGET_ARCH >= _OU_TARGET_ARCH__MAX

#error Please define a valid value for _OU_TARGET_ARCH


#endif // #if _OU_TARGET_ARCH <= 0 || _OU_TARGET_ARCH >= _OU_TARGET_ARCH__MAX


#endif // #if defined(_OU_TARGET_ARCH)


//////////////////////////////////////////////////////////////////////////
// Compiler definition

#define _OU_COMPILER__OTHER	1
#define _OU_COMPILER_GCC	2
#define _OU_COMPILER_MSVC	3

#define _OU_COMPILER__MAX	4


#define _OU_COMPILER_VERSION__OTHER		1
#define _OU_COMPILER_VERSION_MSVC1998	2
#define _OU_COMPILER_VERSION_GCCLT4		3

#define _OU_COMPILER_VERSION__MAX		4


//////////////////////////////////////////////////////////////////////////

#if !defined(_OU_COMPILER)

#if defined(__GNUC__)

#define _OU_COMPILER		_OU_COMPILER_GCC

#if __GNUC__ < 4

#define _OU_COMPILER_VERSION	_OU_COMPILER_VERSION_GCCLT4


#endif // compiler version


#elif defined(_MSC_VER)

#define _OU_COMPILER		_OU_COMPILER_MSVC

#if _MSC_VER <= 1200

#define _OU_COMPILER_VERSION	_OU_COMPILER_VERSION_MSVC1998


#endif // compiler version


#else // if no known define found

#define _OU_COMPILER		_OU_COMPILER__OTHER


#endif // Compiler specific definitions


#else // #if defined(_OU_COMPILER)

#if _OU_COMPILER <= 0 || _OU_COMPILER >= _OU_COMPILER__MAX

#error Please define a valid value for _OU_COMPILER


#endif // #if _OU_COMPILER <= 0 || _OU_COMPILER >= _OU_COMPILER__MAX


#endif // #if defined(_OU_COMPILER)


#if !defined(_OU_COMPILER_VERSION)

#define _OU_COMPILER_VERSION _OU_COMPILER_VERSION__OTHER


#endif // #if !defined(_OU_COMPILER_VERSION)


#if _OU_COMPILER_VERSION <= 0 || _OU_COMPILER_VERSION >= _OU_COMPILER_VERSION__MAX

#error Please define a valid value for _OU_COMPILER_VERSION


#endif // #if _OU_COMPILER_VERSION <= 0 || _OU_COMPILER_VERSION >= _OU_COMPILER_VERSION__MAX


//////////////////////////////////////////////////////////////////////////
// Calling convention definition

#if !defined(__OU_CONVENTIONS_DEFINED)

#define __OU_CONVENTIONS_DEFINED


#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

#define _OU_CONVENTION_METHOD
#define _OU_CONVENTION_API __stdcall
#define _OU_CONVENTION_CALLBACK __stdcall


#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS

#define _OU_CONVENTION_METHOD
#define _OU_CONVENTION_API
#define _OU_CONVENTION_CALLBACK


#endif // #if _OU_TARGET_OS == ...


#endif // #if !defined(__OU_CONVENTIONS_DEFINED)


//////////////////////////////////////////////////////////////////////////
// _OU_ALWAYSINLINE/_OU_INLINE definition

#if !defined(__OU_INLINES_DEFINED)

#define __OU_INLINES_DEFINED


#if _OU_COMPILER == _OU_COMPILER_GCC

#define _OU_ALWAYSINLINE__DEFINITION inline __attribute__((always_inline))


#elif _OU_COMPILER == _OU_COMPILER_MSVC

#define _OU_ALWAYSINLINE__DEFINITION inline __forceinline


#else // if _OU_COMPILER == _OU_COMPILER_OTHER

#define _OU_ALWAYSINLINE__DEFINITION inline


#endif // #if _OU_COMPILER == ...


#if defined(_DEBUG)

#define _OU_ALWAYSINLINE inline

#define _OU_INLINE inline


#else // #if !defined(_DEBUG)

#define _OU_ALWAYSINLINE _OU_ALWAYSINLINE__DEFINITION

#define _OU_INLINE inline


#endif // #if !defined(_DEBUG)


#endif // #if !defined(__OU_INLINES_DEFINED)


#endif // #ifndef __OU_PLATFORM_H_INCLUDED
