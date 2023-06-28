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

#ifndef __OU_CUSTOMIZATION_H_INCLUDED
#define __OU_CUSTOMIZATION_H_INCLUDED


#include <ou/namespace.h>
#include <ou/inttypes.h>
#include <ou/platform.h>

#include <stddef.h>


BEGIN_NAMESPACE_OU();


//////////////////////////////////////////////////////////////////////////
// Some helper definitions for assert macros

#if !defined(__FILE__)

// Definition of __FILE__ constant for the case if compiler does not support the macro
extern const char *const __FILE__;


#endif // #if !defined(__FILE__)


#if !defined(__LINE__)

// Definition of __LINE__ constant for the case if compiler does not support the macro
extern const unsigned int __LINE__;


#endif // #if !defined(__LINE__)


//////////////////////////////////////////////////////////////////////////
// Assertion checks customization

enum EASSERTIONFAILURESEVERITY
{
	AFS__MIN,

	AFS_ASSERT = AFS__MIN,
	AFS_CHECK,

	AFS__MAX,
};


typedef void (_OU_CONVENTION_CALLBACK *CAssertionFailedProcedure)(EASSERTIONFAILURESEVERITY fsFailureSeverity, 
	const char *szAssertionExpression, const char *szAssertionFileName, unsigned int uiAssertionSourceLine);


class CAssertionCheckCustomization
{
public:
	static _OU_ALWAYSINLINE CAssertionFailedProcedure _OU_CONVENTION_API
	/*CAssertionFailedProcedure */GetAssertFailureCustomHandler()
	{
		return g_fnAssertFailureHandler;
	}

	static _OU_INLINE void _OU_CONVENTION_API CustomizeAssertionChecks(CAssertionFailedProcedure fnAssertionFailureProcedure)
	{
		g_fnAssertFailureHandler = fnAssertionFailureProcedure;
	}

private:
	static CAssertionFailedProcedure g_fnAssertFailureHandler;
};	


//////////////////////////////////////////////////////////////////////////
// Memory manager customization

#define _OU_MEMORY_REQUIRED_ALIGNMENT		sizeof(_OU_NAMESPACE::uint64ou)


typedef void *(_OU_CONVENTION_CALLBACK *CMemoryAllocationProcedure)(size_t nBlockSize);
typedef void *(_OU_CONVENTION_CALLBACK *CMemoryReallocationProcedure)(void *pv_ExistingBlock, size_t nBlockNewSize);
typedef void (_OU_CONVENTION_CALLBACK *CMemoryDeallocationProcedure)(void *pv_ExistingBlock);


class CMemoryManagerCustomization
{
public:
	static _OU_ALWAYSINLINE CMemoryAllocationProcedure _OU_CONVENTION_API
	/*CMemoryAllocationProcedure */GetMemoryAllocationCustomProcedure()
	{
		return g_fnMemoryAllocationProcedure;
	}

	static _OU_ALWAYSINLINE CMemoryReallocationProcedure _OU_CONVENTION_API
	/*CMemoryReallocationProcedure */GetMemoryReallocationCustomProcedure()
	{
		return g_fnMemoryReallocationProcedure;
	}

	static _OU_ALWAYSINLINE CMemoryDeallocationProcedure _OU_CONVENTION_API
	/*CMemoryDeallocationProcedure */GetMemoryDeallocationCustomProcedure()
	{
		return g_fnMemoryDeallocationProcedure;
	}

	static _OU_INLINE void _OU_CONVENTION_API CustomizeMemoryManager(CMemoryAllocationProcedure fnAllocationProcedure,
		CMemoryReallocationProcedure fnReallocationProcedure, CMemoryDeallocationProcedure fnDeallocationProcedure)
	{
		g_fnMemoryAllocationProcedure = fnAllocationProcedure;
		g_fnMemoryReallocationProcedure = fnReallocationProcedure;
		g_fnMemoryDeallocationProcedure = fnDeallocationProcedure;
	}

private:
	static CMemoryAllocationProcedure g_fnMemoryAllocationProcedure;
	static CMemoryReallocationProcedure g_fnMemoryReallocationProcedure;
	static CMemoryDeallocationProcedure g_fnMemoryDeallocationProcedure;
};


END_NAMESPACE_OU();


#endif // #ifndef __OU_CUSTOMIZATION_H_INCLUDED
