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

#include <ou/features.h>


#if _OU_FEATURE_SET >= _OU_FEATURE_SET_ATOMICS

#include <ou/atomic.h>
#include <ou/assert.h>
#include <ou/namespace.h>


BEGIN_NAMESPACE_OU();


#if !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)

//////////////////////////////////////////////////////////////////////////
// Implementation via mutex locks

#if !defined(__OU_ATOMIC_INITIALIZATION_FUNCTIONS_REQUIRED)

#error Internal error (Atomic-via-mutex implementations can not appear without initialization)


#endif // #if !defined(__OU_ATOMIC_INITIALIZATION_FUNCTIONS_DEFINED)


END_NAMESPACE_OU();


#include <pthread.h>
#include <errno.h>

#if !defined(EOK)

#define EOK		0


#endif


BEGIN_NAMESPACE_OU();


static unsigned int g_uiAtomicAPIInitializationCount = 0;


#define _OU_ATOMIC_MUTEX_COUNT			8
#define _OU_ATOMIC_MUTES_INDEX_SHIFT	3 // Shift by 3 bits as 8 bytes is a common memory alignment
#define _OU_ATOMIC_MUTEX_INDEX_MASK		(_OU_ATOMIC_MUTEX_COUNT - 1)


// Mutexes array is used to distribute load over multiple mutexes
static pthread_mutex_t g_apmAtomicMutexes[_OU_ATOMIC_MUTEX_COUNT];


static inline unsigned int DeriveAtomicMutexIndex(void *pv_Destination)
{
	return ((unsigned int)(size_t)pv_Destination >> _OU_ATOMIC_MUTES_INDEX_SHIFT) & _OU_ATOMIC_MUTEX_INDEX_MASK;
}


//////////////////////////////////////////////////////////////////////////
// Atomic ord32 functions implementation

#if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)

/*extern*/ atomicord32 AtomicIncrement(volatile atomicord32 *paoDestination)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;

	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);

	const atomicord32 aoNewValue = ++(*paoDestination);

	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);

	atomicord32 aoResult = aoNewValue;
	return aoResult;
}

/*extern*/ atomicord32 AtomicDecrement(volatile atomicord32 *paoDestination)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoNewValue = --(*paoDestination);
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoNewValue;
	return aoResult;
}


/*extern*/ atomicord32 AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;

	*paoDestination = aoExchange;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoOldValue;
	return aoResult;
}

/*extern*/ atomicord32 AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;

	*paoDestination += aoAddend;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoOldValue;
	return aoResult;
}

/*extern*/ bool AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	bool bResult = false;

	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;

	if (aoOldValue == aoComparand)
	{
		*paoDestination = aoExchange;

		bResult = true;
	}
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	return bResult;
}


/*extern*/ atomicord32 AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;

	*paoDestination &= aoBitMask;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoOldValue;
	return aoResult;
}

/*extern*/ atomicord32 AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;
	
	*paoDestination |= aoBitMask;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoOldValue;
	return aoResult;
}

/*extern*/ atomicord32 AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)paoDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicord32 aoOldValue = *paoDestination;
	
	*paoDestination ^= aoBitMask;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicord32 aoResult = aoOldValue;
	return aoResult;
}


#endif // #if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)


//////////////////////////////////////////////////////////////////////////
// Atomic pointer functions implementation

/*extern*/ atomicptr AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)papDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicptr apOldValue = *papDestination;
	
	*papDestination = apExchange;
	
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	atomicptr apResult = apOldValue;
	return apResult;
}

/*extern*/ bool AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
	bool bResult = false;

	const unsigned int uiMutexIndex = DeriveAtomicMutexIndex((void *)papDestination);
	pthread_mutex_t *ptmMutexToBeUsed = g_apmAtomicMutexes + uiMutexIndex;
	
	int iLockResult = pthread_mutex_lock(ptmMutexToBeUsed);
	OU_CHECK(iLockResult == EOK);
	
	const atomicptr apOldValue = *papDestination;
	
	if (apOldValue == apComparand)
	{
		*papDestination = apExchange;

		bResult = true;
	}
		
	int iUnlockResult = pthread_mutex_unlock(ptmMutexToBeUsed);
	OU_CHECK(iUnlockResult == EOK);
	
	return bResult;
}


//////////////////////////////////////////////////////////////////////////
// Atomic initialization functions implementation

static void FreeAtomicMutexes(unsigned int nLastMutexIndex=0)
{
	const unsigned int nMutexCount = nLastMutexIndex == 0 ? _OU_ATOMIC_MUTEX_COUNT : nLastMutexIndex;

	for (unsigned int nMutexIndex = 0; nMutexIndex != nMutexCount; ++nMutexIndex)
	{
		int iMutexDestroyResult = pthread_mutex_destroy(g_apmAtomicMutexes + nMutexIndex);
		OU_VERIFY(iMutexDestroyResult == EOK); // Ignore the error
	}
}

static bool CreateAtomicMutexesWithAttributes(pthread_mutexattr_t *pmaMutexAttributes)
{
	const unsigned int nMutexCount = _OU_ATOMIC_MUTEX_COUNT;

	unsigned int nMutexIndex = 0;

	for (; nMutexIndex != nMutexCount; ++nMutexIndex)
	{
		int iMutexInitResult = pthread_mutex_init(g_apmAtomicMutexes + nMutexIndex, pmaMutexAttributes);
		
		if (iMutexInitResult != EOK)
		{
			if (nMutexIndex != 0)
			{
				FreeAtomicMutexes(nMutexIndex);
			}

			break;
		}
	}

	bool bResult = nMutexIndex == nMutexCount;
	return bResult;
}

static bool CreateAtomicMutexes()
{
	bool bResult = false;

	pthread_mutexattr_t maMutexAttributes;
	
	int iAttrInitResult = pthread_mutexattr_init(&maMutexAttributes);
	
	if (iAttrInitResult == EOK)
	{
		bResult = CreateAtomicMutexesWithAttributes(&maMutexAttributes);

		int iAttrDestroyResult = pthread_mutexattr_destroy(&maMutexAttributes);
		OU_VERIFY(iAttrDestroyResult == EOK); // Ignore error
	}
	
	return bResult;
}


static bool InitializeAtomicAPIValidated()
{
	bool bResult = false;
	
	do
	{
		if (!CreateAtomicMutexes())
		{
			break;
		}

		bResult = true;
	}
	while (false);
	
	return bResult;
}

static void FinalizeAtomicAPIValidated()
{
	FreeAtomicMutexes();
}


/*extern*/ bool InitializeAtomicAPI()
{
	OU_ASSERT(g_uiAtomicAPIInitializationCount != 0U - 1U);

	bool bResult = false;
	
	do
	{
		if (g_uiAtomicAPIInitializationCount == 0) // Initialization/finalization must be called from main thread
		{
			if (!InitializeAtomicAPIValidated())
			{
				break;
			}
		}
	
		++g_uiAtomicAPIInitializationCount;

		bResult = true;
	}
	while (false);

	return bResult;
}

/*extern*/ void FinalizeAtomicAPI()
{
	OU_ASSERT(g_uiAtomicAPIInitializationCount != 0U);

	if (--g_uiAtomicAPIInitializationCount == 0) // Initialization/finalization must be called from main thread
	{
		FinalizeAtomicAPIValidated();
	}
}


#else // #if defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)

#if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)

#error Internal error (Atomic ord32 functions can not be undefined while pointer functions are defined)


#endif // #if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)


#if defined(__OU_ATOMIC_INITIALIZATION_FUNCTIONS_REQUIRED)

#error Internal error (Atomic initialization can not be required while atomic functions are defined)


#endif // #if defined(__OU_ATOMIC_INITIALIZATION_FUNCTIONS_REQUIRED)


#endif // #if !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)


END_NAMESPACE_OU();


#endif // #if _OU_FEATURE_SET >= _OU_FEATURE_SET_ATOMICS

