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

#ifndef __OU_ATOMICFLAGS_H_INCLUDED
#define __OU_ATOMICFLAGS_H_INCLUDED


#include <ou/flagsdefines.h>
#include <ou/atomic.h>
#include <ou/assert.h>
#include <ou/namespace.h>


BEGIN_NAMESPACE_OU();


/*
 *	Implementation Note:
 *	Modification functions are implemented as memory barriers. 
 *	Retrieval functions are implemented as ordinary memory accesses.
 *	Practice proves that such approach is quite sufficient to provide
 *	reliable synchronization mechanisms (provided a developer has solid
 *	knowledge in field, of course).
 */

class CAtomicFlags
{
public:
	_OU_INLINE _OU_CONVENTION_METHOD CAtomicFlags():
		m_aoFlagsValue(0)
	{
	}

	_OU_INLINE _OU_CONVENTION_METHOD CAtomicFlags(atomicord32 aoFlagsValue):
		m_aoFlagsValue(aoFlagsValue)
	{
	}

	typedef atomicord32 value_type;

public:
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */AssignFlagsAllValues(atomicord32 aoFlagsValue)
	{
		AtomicExchange(&m_aoFlagsValue, aoFlagsValue);
	}

	_OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_METHOD 
	/*atomicord32 */QueryFlagsAllValues() const
	{
		return m_aoFlagsValue;
	}


	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */SetFlagsMaskValue(atomicord32 aoFlagsMask, bool bFlagValue)
	{
		if (bFlagValue)
		{
			AtomicOrNoResult(&m_aoFlagsValue, aoFlagsMask);
		}
		else
		{
			AtomicAndNoResult(&m_aoFlagsValue, ~aoFlagsMask);
		}
	}

	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */SignalFlagsMaskValue(atomicord32 aoFlagsMask)
	{
		AtomicOrNoResult(&m_aoFlagsValue, aoFlagsMask);
	}

	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */DropFlagsMaskValue(atomicord32 aoFlagsMask)
	{
		AtomicAndNoResult(&m_aoFlagsValue, ~aoFlagsMask);
	}


	// Can operate on single flag only
	// Returns previous flag value
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */ToggleSingleFlagValue(atomicord32 aoSingleFlag)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(atomicord32, aoSingleFlag));

		return (AtomicXor(&m_aoFlagsValue, aoSingleFlag) & aoSingleFlag) != (atomicord32)0;
	}

	// Can operate on single flag only
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */ModifySingleFlagValue(atomicord32 aoSingleFlag, bool bFlagValue)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(atomicord32, aoSingleFlag));

		return bFlagValue 
			? ((AtomicOr(&m_aoFlagsValue, aoSingleFlag) & aoSingleFlag) == (atomicord32)0) 
			: ((AtomicAnd(&m_aoFlagsValue, ~aoSingleFlag) & aoSingleFlag) != (atomicord32)0);
	}


	// Modifies subset of flags
	// Returns previous flags
	_OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_METHOD 
	/*atomicord32 */AssignFlagsByMask(atomicord32 aoFlagsMask, atomicord32 aoFlagsValue)
	{
		atomicord32 aoFlagsOldValue;
		
		do
		{
			aoFlagsOldValue = m_aoFlagsValue;
		}
		while (!AtomicCompareExchange(&m_aoFlagsValue, aoFlagsOldValue, (aoFlagsOldValue & ~aoFlagsMask) | (aoFlagsValue & aoFlagsMask)));
		
		return aoFlagsOldValue;
	}

	// Modifies subset of flags
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */AlterFlagsByMask(atomicord32 aoFlagsMask, atomicord32 aoFlagsValue)
	{
		atomicord32 aoFlagsOldValue;
		
		do
		{
			aoFlagsOldValue = m_aoFlagsValue;
		}
		while (!AtomicCompareExchange(&m_aoFlagsValue, aoFlagsOldValue, (aoFlagsOldValue & ~aoFlagsMask) | (aoFlagsValue & aoFlagsMask)));

		return ((aoFlagsOldValue ^ aoFlagsValue) & aoFlagsMask) != (atomicord32)0;
	}


	// Returns value of flag or tests for any bit in a mask
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */GetFlagsMaskValue(atomicord32 aoFlagsMask) const
	{
		return (m_aoFlagsValue & aoFlagsMask) != (atomicord32)0;
	}
	
	// Returns subset of flags
	_OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_METHOD 
	/*atomicord32 */QueryFlagsByMask(atomicord32 aoFlagsMask) const
	{
		return (m_aoFlagsValue & aoFlagsMask);
	}
	
public:
	// Signal only flag out of mask
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */OnlySignalSingleFlagOutOfMask(atomicord32 aoFlagsMask, atomicord32 aoSingleFlag)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(atomicord32, aoSingleFlag));

		bool bResult;
		
		atomicord32 aoFlagsOldValue;
		
		do
		{
			aoFlagsOldValue = m_aoFlagsValue;

			/*
			 *	Implementation Note:
			 *	1) This function may be not a memory barrier. However that would also mean that
			 *	no modification occurred and result is 'false'. Such behavior should be OK.
			 *	2) Even though second assignment to bResult is unnecessary it might yield
			 *	better code as compiler does not need to save variable's value for the call
			 *	to AtomicCompareExchange in this case.
			 */
		}
		while ((bResult = !(aoFlagsOldValue & aoFlagsMask)) && !(bResult = AtomicCompareExchange(&m_aoFlagsValue, aoFlagsOldValue, aoFlagsOldValue | aoSingleFlag)));
		
		return bResult;
	}
	
public:
	// Set value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumSetEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum, bool bFlagValue) 
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		SetFlagsMaskValue(aoStartingFlag << uiEnumeratedValue, bFlagValue);
	}

	// Signal value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumSignalEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		SignalFlagsMaskValue(aoStartingFlag << uiEnumeratedValue);
	}

	// Drop value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumDropEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum) 
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
		
		DropFlagsMaskValue(aoStartingFlag << uiEnumeratedValue);
	}
	

	// Can operate on single flag only
	// Returns previous flag value
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumToggleEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
		
		return ToggleSingleFlagValue(aoStartingFlag << uiEnumeratedValue); 
	}

	// Can operate on single flag only
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumModifyEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum, bool bFlagValue)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		return ModifySingleFlagValue(aoStartingFlag << uiEnumeratedValue, bFlagValue);
	}
	

	// Returns if this was the first flag signaled
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumSignalFirstEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		return (AssignFlagsByMask(aoStartingFlag << uiEnumeratedValue, aoStartingFlag << uiEnumeratedValue) & OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum)) == (atomicord32)0;
	}
	
	// Returns if this was the last flag signaled
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumSignalLastEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
		
		return (AssignFlagsByMask(aoStartingFlag << uiEnumeratedValue, aoStartingFlag << uiEnumeratedValue) & OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum)) == (OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum) & ~(aoStartingFlag << uiEnumeratedValue));
	}
	
	
	// Retrieve value of flag indexed by enum
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumGetEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		return GetFlagsMaskValue(aoStartingFlag << uiEnumeratedValue);
	}
	
	// Find enum value for first flag signaled
	_OU_INLINE int _OU_CONVENTION_METHOD 
	/*int */EnumFindFirstEnumeratedFlag(atomicord32 aoStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		unsigned int uiResult = 0;

		atomicord32 aoFlagsMask = aoStartingFlag;
		for (; uiResult < uiEnumeratedMaximum; ++uiResult, aoFlagsMask <<= 1)
		{
			if (GetFlagsMaskValue(aoFlagsMask))
			{
				break;
			}
		}

		return uiResult;
	}
	
public:
	// Signal all flags indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumAllSignalEnumeratedFlags(atomicord32 aoStartingFlag, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		SignalFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
	}

	// Drop all flags indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumAllDropEnumeratedFlags(atomicord32 aoStartingFlag, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
		
		DropFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
	}
	

	// Query all flags indexed by enum
	_OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_METHOD 
	/*atomicord32 */EnumAllQueryEnumeratedFlags(atomicord32 aoStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		return QueryFlagsByMask(OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
	}

	// Get if any flag indexed by enum is set
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumAnyGetEnumeratedFlagValue(atomicord32 aoStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(atomicord32, aoStartingFlag, uiEnumeratedMaximum));

		return GetFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(atomicord32, aoStartingFlag, uiEnumeratedMaximum));
	}
	
public:
	// Store enumerated value in flags
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */StoreFlagsEnumeratedValue(atomicord32 aoEnumeratedValueMask, unsigned int uiEnumeratedValueShift, unsigned int uiEnumeratedValue)
	{
		OU_ASSERT(OU_FLAGS_STOREENUM_VALUE_IN_MASK(atomicord32, uiEnumeratedValue, aoEnumeratedValueMask));

		AssignFlagsByMask(aoEnumeratedValueMask << uiEnumeratedValueShift, (atomicord32)uiEnumeratedValue << uiEnumeratedValueShift);
	}

	// Retrieve enumerated value from flags
	_OU_ALWAYSINLINE unsigned int _OU_CONVENTION_METHOD 
	/*unsigned int */RetrieveFlagsEnumeratedValue(atomicord32 aoEnumeratedValueMask, unsigned int uiEnumeratedValueShift) const
	{
		return (unsigned int)((QueryFlagsAllValues() >> uiEnumeratedValueShift) & aoEnumeratedValueMask);
	}
	
private:
	atomicord32		m_aoFlagsValue;
};


END_NAMESPACE_OU();


#endif // #ifndef __OU_ATOMICFLAGS_H_INCLUDED
