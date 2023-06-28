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

#ifndef __OU_TYPEWRAPPER_H_INCLUDED
#define __OU_TYPEWRAPPER_H_INCLUDED


#include <ou/platform.h>
#include <ou/namespace.h>


BEGIN_NAMESPACE_OU();


template<typename ContainedType, const int Instance=0>
struct CTypeSimpleWrapper
{
public:
	_OU_INLINE CTypeSimpleWrapper(): m_ctValue() {}
	_OU_INLINE CTypeSimpleWrapper(const ContainedType &ctValue): m_ctValue(ctValue) {}
	// _OU_INLINE CTypeSimpleWrapper(const CTypeSimpleWrapper &twOtherWrapper): m_ctValue(twOtherWrapper.m_ctValue) {} -- do not uncomment!!! in MSVC 6.0 optimization fails with it. :-/
	
	typedef ContainedType value_type;
	
	_OU_INLINE bool operator ==(const CTypeSimpleWrapper &twOtherWrapper) const { return m_ctValue == twOtherWrapper.m_ctValue; }
	_OU_INLINE bool operator !=(const CTypeSimpleWrapper &twOtherWrapper) const { return !(operator ==(twOtherWrapper)); }
	
	_OU_INLINE bool operator <(const CTypeSimpleWrapper &twOtherWrapper) const { return m_ctValue < twOtherWrapper.m_ctValue; }
	_OU_INLINE bool operator >(const CTypeSimpleWrapper &twOtherWrapper) const { return twOtherWrapper.operator <(*this); }
	_OU_INLINE bool operator <=(const CTypeSimpleWrapper &twOtherWrapper) const { return !(twOtherWrapper.operator <(*this)); }
	_OU_INLINE bool operator >=(const CTypeSimpleWrapper &twOtherWrapper) const { return !(operator <(twOtherWrapper)); }
	
	// _OU_INLINE operator bool() const { return !!m_ctValue; } -- casting to bool is too dangerous - it tends to be used instead of casting to int
	_OU_INLINE bool operator !() const { return !m_ctValue; }
	
	_OU_INLINE CTypeSimpleWrapper &operator =(const ContainedType &ctValue) { m_ctValue = ctValue; return *this; }
	_OU_INLINE CTypeSimpleWrapper &operator =(const CTypeSimpleWrapper &twOtherWrapper) { m_ctValue = twOtherWrapper.m_ctValue; return *this; }

	_OU_INLINE operator const ContainedType &() const { return m_ctValue; }
	_OU_INLINE operator ContainedType &() { return m_ctValue; }

private:
	ContainedType			m_ctValue;
};


template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator ==(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return (const ContainedType &)twLeftWrapper == ctRightValue; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator ==(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return ctLeftValue == (const ContainedType &)twRightWrapper; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator !=(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return !(twLeftWrapper == ctRightValue); }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator !=(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return !(ctLeftValue == twRightWrapper); }


template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator <(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return (const ContainedType &)twLeftWrapper < ctRightValue; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator <(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return ctLeftValue < (const ContainedType &)twRightWrapper; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator >(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return ctRightValue < twLeftWrapper; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator >(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return twRightWrapper < ctLeftValue; }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator <=(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return !(ctRightValue < twLeftWrapper); }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator <=(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return !(twRightWrapper < ctLeftValue); }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator >=(const CTypeSimpleWrapper<ContainedType, Instance> &twLeftWrapper, const ContainedType &ctRightValue) { return !(twLeftWrapper < ctRightValue); }

template<typename ContainedType, const int Instance>
_OU_INLINE bool _OU_CONVENTION_API operator >=(const ContainedType &ctLeftValue, const CTypeSimpleWrapper<ContainedType, Instance> &twRightWrapper) { return !(ctLeftValue < twRightWrapper); }


END_NAMESPACE_OU();


#endif // #ifndef __OU_TYPEWRAPPER_H_INCLUDED
