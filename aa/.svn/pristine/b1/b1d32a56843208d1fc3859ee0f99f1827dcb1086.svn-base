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

#ifndef __OU_SIMPLEFLAGS_H_INCLUDED
#define __OU_SIMPLEFLAGS_H_INCLUDED


#include <ou/flagsdefines.h>
#include <ou/assert.h>
#include <ou/inttypes.h>
#include <ou/namespace.h>

#include <stddef.h>


BEGIN_NAMESPACE_OU();


template<typename ContainerType>
class CSimpleFlagsTemplate
{
public:
	_OU_INLINE _OU_CONVENTION_METHOD CSimpleFlagsTemplate():
		m_ctFlagsValue(0)
	{
	}

	_OU_INLINE _OU_CONVENTION_METHOD CSimpleFlagsTemplate(ContainerType ctFlagsValue):
		m_ctFlagsValue(ctFlagsValue)
	{
	}

	typedef ContainerType value_type;

public:
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */AssignFlagsAllValues(ContainerType ctFlagsValue)
	{
		m_ctFlagsValue = ctFlagsValue;
	}

	_OU_ALWAYSINLINE ContainerType _OU_CONVENTION_METHOD 
	/*ContainerType */QueryFlagsAllValues() const
	{
		return m_ctFlagsValue;
	}


	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */SetFlagsMaskValue(ContainerType ctFlagsMask, bool bFlagValue)
	{
		m_ctFlagsValue = bFlagValue 
			? (m_ctFlagsValue | ctFlagsMask) 
			: (m_ctFlagsValue & ~ctFlagsMask);
	}

	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */SignalFlagsMaskValue(ContainerType ctFlagsMask)
	{
		m_ctFlagsValue |= ctFlagsMask;
	}

	// Can operate both on single flag and flag set
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */DropFlagsMaskValue(ContainerType ctFlagsMask)
	{
		m_ctFlagsValue &= ~ctFlagsMask;
	}


	// Can operate on single flag only
	// Returns previous flag value
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */ToggleSingleFlagValue(ContainerType ctSingleFlag)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(ContainerType, ctSingleFlag));

		return ((m_ctFlagsValue ^= ctSingleFlag) & ctSingleFlag) == (ContainerType)0;
	}

	// Can operate on single flag only
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */ModifySingleFlagValue(ContainerType ctSingleFlag, bool bFlagValue)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(ContainerType, ctSingleFlag));

		return ((m_ctFlagsValue & ctSingleFlag) != (ContainerType)0) != bFlagValue 
			? ((m_ctFlagsValue ^= ctSingleFlag), true) 
			: (false);
	}


	// Modifies subset of flags
	// Returns previous flags
	_OU_ALWAYSINLINE ContainerType _OU_CONVENTION_METHOD 
	/*ContainerType */AssignFlagsByMask(ContainerType ctFlagsMask, ContainerType ctFlagsValue)
	{
		ContainerType ctFlagsOldValue = m_ctFlagsValue;
	
		m_ctFlagsValue = (ctFlagsOldValue & ~ctFlagsMask) | (ctFlagsValue & ctFlagsMask);
		
		return ctFlagsOldValue;
	}

	// Modifies subset of flags
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */AlterFlagsByMask(ContainerType ctFlagsMask, ContainerType ctFlagsValue)
	{
		ContainerType ctFlagsOldValue = m_ctFlagsValue;
		
		m_ctFlagsValue = (ctFlagsOldValue & ~ctFlagsMask) | (ctFlagsValue & ctFlagsMask);
		
		return ((ctFlagsOldValue ^ ctFlagsValue) & ctFlagsMask) != (ContainerType)0;
	}


	// Returns value of flag or tests for any bit in a mask
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */GetFlagsMaskValue(ContainerType ctFlagsMask) const
	{
		return (m_ctFlagsValue & ctFlagsMask) != (ContainerType)0;
	}
	
	// Returns subset of flags
	_OU_ALWAYSINLINE ContainerType _OU_CONVENTION_METHOD 
	/*ContainerType */QueryFlagsByMask(ContainerType ctFlagsMask) const
	{
		return (m_ctFlagsValue & ctFlagsMask);
	}
	
public:
	// Signal only flag out of mask
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */OnlySignalSingleFlagOutOfMask(ContainerType ctFlagsMask, ContainerType ctSingleFlag)
	{
		OU_ASSERT(OU_FLAGS_FLAG_IS_SINGLE(ContainerType, ctSingleFlag));

		return !(m_ctFlagsValue & ctFlagsMask) 
			? (m_ctFlagsValue |= ctSingleFlag, true) 
			: (false);
	}
	
public:
	// Set value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumSetEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum, bool bFlagValue) 
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		SetFlagsMaskValue(ctStartingFlag << uiEnumeratedValue, bFlagValue);
	}

	// Signal value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumSignalEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		SignalFlagsMaskValue(ctStartingFlag << uiEnumeratedValue);
	}

	// Drop value of flag indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumDropEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum) 
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
		
		DropFlagsMaskValue(ctStartingFlag << uiEnumeratedValue);
	}
	

	// Can operate on single flag only
	// Returns previous flag value
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumToggleEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
		
		return ToggleSingleFlagValue(ctStartingFlag << uiEnumeratedValue); 
	}

	// Can operate on single flag only
	// Returns if modification occurred
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumModifyEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum, bool bFlagValue)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		return ModifySingleFlagValue(ctStartingFlag << uiEnumeratedValue, bFlagValue);
	}
	

	// Returns if this was the first flag signaled
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumSignalFirstEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		return (AssignFlagsByMask(ctStartingFlag << uiEnumeratedValue, ctStartingFlag << uiEnumeratedValue) & OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum)) == (ContainerType)0;
	}
	
	// Returns if this was the last flag signaled
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumSignalLastEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
		
		return (AssignFlagsByMask(ctStartingFlag << uiEnumeratedValue, ctStartingFlag << uiEnumeratedValue) & OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum)) == (OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum) & ~(ctStartingFlag << uiEnumeratedValue));
	}
	
	
	// Retrieve value of flag indexed by enum
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumGetEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedValue, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(uiEnumeratedValue < uiEnumeratedMaximum && OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		return GetFlagsMaskValue(ctStartingFlag << uiEnumeratedValue);
	}
	
	// Find enum value for first flag signaled
	_OU_INLINE unsigned int _OU_CONVENTION_METHOD 
	/*unsigned int */EnumFindFirstEnumeratedFlag(ContainerType ctStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		unsigned int uiResult = 0;

		ContainerType ctFlagsMask = ctStartingFlag;
		for (; uiResult < uiEnumeratedMaximum; ++uiResult, ctFlagsMask <<= 1)
		{
			if (GetFlagsMaskValue(ctFlagsMask))
			{
				break;
			}
		}

		return uiResult;
	}
	
public:
	// Signal all flags indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumAllSignalEnumeratedFlags(ContainerType ctStartingFlag, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		SignalFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
	}

	// Drop all flags indexed by enum
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */EnumAllDropEnumeratedFlags(ContainerType ctStartingFlag, unsigned int uiEnumeratedMaximum)
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
		
		DropFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
	}
	

	// Query all flags indexed by enum
	_OU_ALWAYSINLINE ContainerType _OU_CONVENTION_METHOD 
	/*ContainerType */EnumAllQueryEnumeratedFlags(ContainerType ctStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		return QueryFlagsByMask(OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
	}

	// Get if any flag indexed by enum is set
	_OU_ALWAYSINLINE bool _OU_CONVENTION_METHOD 
	/*bool */EnumAnyGetEnumeratedFlagValue(ContainerType ctStartingFlag, unsigned int uiEnumeratedMaximum) const
	{
		OU_ASSERT(OU_FLAGS_ENUMFLAGS_START_VALID(ContainerType, ctStartingFlag, uiEnumeratedMaximum));

		return GetFlagsMaskValue(OU_FLAGS_ENUMFLAGS_MASK(ContainerType, ctStartingFlag, uiEnumeratedMaximum));
	}
	
public:
	// Store enumerated value in flags
	_OU_ALWAYSINLINE void _OU_CONVENTION_METHOD 
	/*void */StoreFlagsEnumeratedValue(ContainerType ctEnumeratedValueMask, unsigned int uiEnumeratedValueShift, unsigned int uiEnumeratedValue)
	{
		OU_ASSERT(OU_FLAGS_STOREENUM_VALUE_IN_MASK(ContainerType, uiEnumeratedValue, ctEnumeratedValueMask));

		AssignFlagsByMask(ctEnumeratedValueMask << uiEnumeratedValueShift, (ContainerType)uiEnumeratedValue << uiEnumeratedValueShift);
	}

	// Retrieve enumerated value from flags
	_OU_ALWAYSINLINE unsigned int _OU_CONVENTION_METHOD 
	/*unsigned int */RetrieveFlagsEnumeratedValue(ContainerType ctEnumeratedValueMask, unsigned int uiEnumeratedValueShift) const
	{
		return (unsigned int)((QueryFlagsAllValues() >> uiEnumeratedValueShift) & ctEnumeratedValueMask);
	}
	
private:
	ContainerType		m_ctFlagsValue;
};


typedef CSimpleFlagsTemplate<uint32ou> CSimpleFlags;


END_NAMESPACE_OU();


#endif // #ifndef __OU_SIMPLEFLAGS_H_INCLUDED
