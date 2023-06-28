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

#ifndef _OU_THREADLOCALSTORAGE_H_INCLUDED
#define _OU_THREADLOCALSTORAGE_H_INCLUDED


#include <ou/features.h>


#if _OU_FEATURE_SET >= _OU_FEATURE_SET_TLS

#include <ou/typewrapper.h>
#include <ou/macros.h>
#include <ou/assert.h>
#include <ou/platform.h>
#include <ou/namespace.h>

#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS

#include <windows.h>


#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS

#include <pthread.h>


#endif // #if _OU_TARGET_OS == ...


BEGIN_NAMESPACE_OU();


//////////////////////////////////////////////////////////////////////////
// API specific types

typedef CTypeSimpleWrapper<void *, 1> HTLSKEYVALUE;
typedef CTypeSimpleWrapper<HTLSKEYVALUE *, 0> HTLSKEYSELECTOR;
typedef HTLSKEYSELECTOR HTLSKEY;

typedef void *tlsvaluetype;
typedef unsigned int tlsindextype;

typedef void (_OU_CONVENTION_CALLBACK *CTLSValueDestructor)(tlsvaluetype vValueData);


#define OU_TLS_VALUE_AS_POINTER(value)	(value)


//////////////////////////////////////////////////////////////////////////
// Internal types required for functions to be made inline

struct CTLSStorageArray;

struct CTLSStorageBlock
{
/*
 *	Implementation Note:
 *	1) Value destructors are stored in separate array since those are 
 *	rarely accessed values and not intermixing them with data
 *	potentially simplifies data access (well, just theoretically, of course :)).
 *	2) Destructors are stored with negative offset to allow accessing them 
 *	without the knowledge of value count.
 *	3) Well, intermixing or not intermixing has really minor impact on 
 *	implementation characteristics, so why not to choose it after the current mood? :)
 */
private:
	enum
	{
		TSB_RESERVEDPOINTER_HOSTARRAY,
			
		TSB_RESERVEDPOINTER__MAX,
	};

public:
	enum
	{
		TSB_LARGEST_ALIGNMENT = sizeof(void *) > sizeof(tlsvaluetype) ? sizeof(void *) : sizeof(tlsvaluetype),
	};
	
public:
	static inline size_t GetRequiredSize(tlsindextype iValueCount)
	{
		return OU_ALIGNED_SIZE(iValueCount * (sizeof(tlsvaluetype) + sizeof(CTLSValueDestructor)) + TSB_RESERVEDPOINTER__MAX * sizeof(void *), TSB_LARGEST_ALIGNMENT);
	}

	static inline size_t GetZeroOffset(tlsindextype iValueCount)
	{
		// Since pointers and values are stored in different directions,
		// alignment correction must fall entirely to either side and 
		// required size will not be exceeded.
		return OU_ALIGNED_SIZE(iValueCount * sizeof(CTLSValueDestructor) + TSB_RESERVEDPOINTER__MAX * sizeof(void *), TSB_LARGEST_ALIGNMENT);
	}

public:
	inline void SetValueData(tlsindextype iValueIndex, tlsvaluetype vValueData)
	{
		un.m_av_ValueDatas[iValueIndex] = vValueData;
	}

	inline tlsvaluetype GetValueData(tlsindextype iValueIndex) const
	{
		return un.m_av_ValueDatas[iValueIndex];
	}

	inline void SetHostArray(CTLSStorageArray *psaInstance)
	{
		un.m_asaHostArrays[(ptrdiff_t)0 - (1 + TSB_RESERVEDPOINTER_HOSTARRAY)] = psaInstance;
	}
	
	inline CTLSStorageArray *GetHostArray() const
	{
		return un.m_asaHostArrays[(ptrdiff_t)0 - (1 + TSB_RESERVEDPOINTER_HOSTARRAY)];
	}

	inline void SetValueDestructor(tlsindextype iValueIndex, CTLSValueDestructor fvValue)
	{
		un.m_afnValueDestructors[-((ptrdiff_t)iValueIndex) - (1 + TSB_RESERVEDPOINTER__MAX)] = fvValue;
	}
	
	inline CTLSValueDestructor GetValueDestructor(tlsindextype iValueIndex) const
	{
		return un.m_afnValueDestructors[-((ptrdiff_t)iValueIndex) - (1 + TSB_RESERVEDPOINTER__MAX)];
	}

private:
	union
	{
		tlsvaluetype		m_av_ValueDatas[1];
		CTLSValueDestructor	m_afnValueDestructors[1];
		CTLSStorageArray	*m_asaHostArrays[1];
	} un;
};


//////////////////////////////////////////////////////////////////////////
// API declaration

class CThreadLocalStorage
{
public: // Safe methods
	/*
	 *	Implementation Note:
	 *	Since the function is potentially slow and should not be frequently
	 *	called anyway, there is no sense in creating additional overload without
	 *	destructor parameter which would preserve current destructor procedure.
	 */
	static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
	/*bool */SetStorageValue(const HTLSKEY &hskStorageKey, tlsindextype iValueIndex, tlsvaluetype vValueData, CTLSValueDestructor fnValueDestructor=NULL)
	{
		bool bResult;
		
		CTLSStorageBlock *psbStorageBlock = GetKeyStorageBlock(hskStorageKey);
			
		if (psbStorageBlock)
		{
			psbStorageBlock->SetValueData(iValueIndex, vValueData);
			psbStorageBlock->SetValueDestructor(iValueIndex, fnValueDestructor);
			
			bResult = true;
		}
		else
		{
			bResult = AllocateAndSetStorageValue(hskStorageKey, iValueIndex, vValueData, fnValueDestructor);
		}

		return bResult;
	}

	static _OU_ALWAYSINLINE tlsvaluetype _OU_CONVENTION_API 
	/*tlsvaluetype */GetStorageValue(const HTLSKEY &hskStorageKey, tlsindextype iValueIndex)
	{
		tlsvaluetype vValueData = 0;
		
		CTLSStorageBlock *psbStorageBlock = GetKeyStorageBlock(hskStorageKey);

		if (psbStorageBlock)
		{
			vValueData = psbStorageBlock->GetValueData(iValueIndex);
		}
		
		return vValueData;
	}

public: // Unsafe methods
	static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
	/*void */UnsafeSetStorageValue(const HTLSKEY &hskStorageKey, tlsindextype iValueIndex, tlsvaluetype vValueData)
	{
		CTLSStorageBlock *psbStorageBlock = GetKeyStorageBlock(hskStorageKey);
		psbStorageBlock->SetValueData(iValueIndex, vValueData);
	}

	static _OU_ALWAYSINLINE tlsvaluetype _OU_CONVENTION_API 
	/*tlsvaluetype */UnsafeGetStorageValue(const HTLSKEY &hskStorageKey, tlsindextype iValueIndex)
	{
		CTLSStorageBlock *psbStorageBlock = GetKeyStorageBlock(hskStorageKey);
		return psbStorageBlock->GetValueData(iValueIndex);
	}

private:
	static bool _OU_CONVENTION_API AllocateAndSetStorageValue(const HTLSKEYSELECTOR &hksKeySelector,
		tlsindextype iValueIndex, tlsvaluetype vValueData, CTLSValueDestructor fnValueDestructor);
	
private:
	friend class CTLSInitialization;
	
	static inline void _OU_CONVENTION_API SetKeyStorageBlock(const HTLSKEYSELECTOR &hskStorageKey, CTLSStorageBlock *psbInstance)
	{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
		
		::TlsSetValue((DWORD)(size_t)(HTLSKEYVALUE::value_type)(*(HTLSKEYSELECTOR::value_type)hskStorageKey), (LPVOID)psbInstance);
		
		
#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS
		
		pthread_setspecific((pthread_key_t)(size_t)(HTLSKEYVALUE::value_type)(*(HTLSKEYSELECTOR::value_type)hskStorageKey), (void *)psbInstance);
		
		
#endif // #if _OU_TARGET_OS == ...
	}

	static inline CTLSStorageBlock *_OU_CONVENTION_API GetKeyStorageBlock(const HTLSKEYSELECTOR &hskStorageKey)
	{
#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS
		
		CTLSStorageBlock *psbStorageBlock = (CTLSStorageBlock *)::TlsGetValue((DWORD)(size_t)(HTLSKEYVALUE::value_type)(*(HTLSKEYSELECTOR::value_type)hskStorageKey));
		
		
#else // #if _OU_TARGET_OS != _OU_TARGET_OS_WINDOWS
		
		CTLSStorageBlock *psbStorageBlock = (CTLSStorageBlock *)pthread_getspecific((pthread_key_t)(size_t)(HTLSKEYVALUE::value_type)(*(HTLSKEYSELECTOR::value_type)hskStorageKey));
		
		
#endif // #if _OU_TARGET_OS == ...

		return psbStorageBlock;
	}
};


//////////////////////////////////////////////////////////////////////////
// Initialization/finalization

class CTLSInitialization
{
public:
	enum EINITIALIZATIONFLAGS
	{
		SIF_MANUAL_CLEANUP_ON_THREAD_EXIT	= 0x00000001,
	};

public:
	// Initialization must be performed from main thread
	static bool _OU_CONVENTION_API InitializeTLSAPI(HTLSKEY &hskOutStorageKey, tlsindextype iValueCount,
		unsigned int uiInitializationFlags=0);
	static void _OU_CONVENTION_API FinalizeTLSAPI();

	static void _OU_CONVENTION_API CleanupOnThreadExit();

private:
	static bool _OU_CONVENTION_API InitializeTLSAPIValidated(unsigned int uiInstanceKind, 
		tlsindextype iValueCount, unsigned int uiInitializationFlags);
	static void _OU_CONVENTION_API FinalizeTLSAPIValidated(unsigned int uiInstanceKind);
};


END_NAMESPACE_OU();


#endif // #if _OU_FEATURE_SET >= _OU_FEATURE_SET_TLS


#endif // #ifndef _OU_THREADLOCALSTORAGE_H_INCLUDED
