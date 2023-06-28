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

#ifndef __OU_ATOMIC_H_INCLUDED
#define __OU_ATOMIC_H_INCLUDED


/**
 *	\file
 *	\brief Definitions of atomic (interlocked) API.
 *	
 *	Atomic (interlocked) functions are supposed to provide atomic operations on 
 *	variables in multi-threaded environment without bringing synchronization objects in.
 *	Atomic functions can be used for implementing reliable reference counting, advanced
 *	synchronization objects, complex techniques of relaxed synchronization with minimum
 *	or no synchronization obejcts' usage.
 *	
 *	All atomic functions are implemented as memory barriers.
 *
 *	On Windows, QNX, MacOS, AIX, SunOS atomic API is implemented via native OS calls.
 *	If atomic operation function are not provided (or not fully provided) by target OS,
 *	the missing operations are implemented with aid of other atomic functions or
 *	via mutex locks if that is not possible. The array of \c _OU_ATOMIC_MUTEX_COUNT 
 *	(8 in current version) mutexes is used to decrease probability of several threads 
 *	being competing for the same mutex lock and resulting necessity to block some 
 *	of them during operation.
 *
 *	All atomic API implementations are inlined, Exceptions are implementations via
 *	mutex locks for which it is not reasonable to generate inlined code.
 *
 *	Atomic functions' prototypes were selected to to provide maximal possible 
 *	functionality available on all the platforms mentioned above in common. Function
 *	names were chosen as a mix of Windows and UNIX naming traditions (more closely 
 *	to Windows though).
 *
 *	There are the following groups of API available:
 *	\li Arithmetic (\c AtomicIncrement, \c AtomicDecrement)
 *	\li Integer Exchange (\c AtomicExchange, \c AtomicExchangeAdd, \c AtomicCompareExchange)
 *	\li Bitwise (\c AtomicAnd, \c AtomicOr, \c AtomicXor)
 *	\li Pointer Exchange (\c AtomicExchangePointer, \c AtomicCompareExchangePointer)
 *	
 *	For Arithmetic and Bitwise groups along with \c AtomicExchangeAdd function there
 *	are "no result" variants available. These are written with \c NoResult suffix 
 *	after function name and may operate faster on some platforms. However they do not
 *	provide operation results.
 *
 *	Atomic functions of Arithmetic, Integer Exchange and Bitwise groups operate with
 *	32-bit values regardless if build target address space is 32 or 64 bits wide.
 *	Pointer Exchange functions operate with pointer type and their argument can be
 *	both 32 or 64 bit value depending on build target.
 *
 *	Generic x86 assembler implementation is provided for i486 and later processors. 
 *	However it is never automatically selected. You must explicitly define
 *	\c _OU_ATOMIC_USE_X86_ASSEMBLER symbol in compiler options to select that 
 *	implementation. The option can only be used if \c _OU_TARGET_OS is equal to 
 *	\c _OU_TARGET_OS_GENUNIX. If the symbol is defined for any other target, 
 *	it is silently ignored.
 *	\warning
 *	Never use assembler implementation on systems that provide native atomic API!
 *
 *	Atomic API may require initialization before first use and finalization on program
 *	exit. The initialization may be necessary when operating system does not provide
 *	sufficient functionality to implement all the operations via native calls. However,
 *	to maintain code portability it is recommended that initialization/finalization
 *	functions are always called.
 *
 *	API initialization and finalization calls use reference counting mechanism and
 *	thus may be invoked several times from different subsystems. Initialization and
 *	finalization is \e not \e thread \e safe and should be performed from main thread
 *	only.
 */

#include <ou/features.h>


#if _OU_FEATURE_SET >= _OU_FEATURE_SET_ATOMICS

#include <ou/inttypes.h>
#include <ou/namespace.h>
#include <ou/platform.h>


/**
 *	\typedef atomicord32
 *	\brief An uniform type for 32-bit values to be used as atomic operations' arguments.
 *
 *	This type is supposed to be used for all the variables that store atomic values.
 *	The word "int" was by intent avoided in its name to emphasize that the type
 *	needs not necessary to be a signed integer. It might be either signed or unsigned 
 *	depending on target platform. The only information which could be relied on is
 *	that the type will always be 32 bit wide, regardless if target platform is a
 *	32 or 64-bit one.
 *
 *	Any arithmetic operations should be avoided with type \c atomicord32. 
 *	Instead, the wrapper functions should be used to access the value. The function should 
 *	cast value type to \c atomicord32 in parameters and cast result back to value type.
 *	\code
 *		int ExchangeValue(volatile atomicord32 *paoDestination, int iExchange)
 *		{
 *			return (int)AtomicExchange(paoDestination, (atomicord32)iExchange);
 *		}
 *	\endcode
 *	\see atomicptr
 */

/**
 *	\typedef atomicptr
 *	\brief An uniform type for pointer values to be used as atomic operations' arguments
 *
 *	The type is to be used for those function which operate with pointers rather
 *	than integers. The size of \c atomicptr is platform dependent, just like the
 *	size of the pointer and equals 4 bytes on 32-bit platforms and 8 bytes on 64-bit ones.
 *	\see atomicord32
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicIncrement(volatile atomicord32 *paoDestination)
 *	\brief Increments the destination and returns its new value.
 *	\param paoDestination A pointer to a variable to be incremented.
 *	\return A value of variable pointer to by \a paoDestination after the increment.
 *
 *	The function implements functionality of \c InterlockedIncrement from Win32 API.
 *	It is most commonly used for reference counting.
 *	\see AtomicDecrement
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicDecrement(volatile atomicord32 *paoDestination)
 *	\brief Decrements the destination and returns its new value.
 *	\param paoDestination A pointer to a variable to be decremented.
 *	\return A value of variable pointer to by \a paoDestination after the decrement.
 *
 *	The function implements functionality of \c InterlockedDecrement from Win32 API.
 *	It is most commonly used for reference counting.
 *	\see AtomicIncrement
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
 *	\brief Stores new value in destination and returns old value of destination.
 *	\param paoDestination A pointer to a variable the data is to be exchanged with.
 *	\param aoExchange A value to be used for exchange. 
 *	\return Previous value of variable pointer to by \a paoDestination.
 *
 *	The function performs atomic exchange of \a aoExchange value with memory location 
 *	pointer to by \a paoDestination. Most common uses are relaxed synchronization
 *	and shared value extraction.
 *	\see AtomicCompareExchange
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
 *	\brief Assigns sum of addend and existing destination value to the destination 
 *	and returns original value of destination.
 *	\param paoDestination A pointer to a variable the value is to be added to.
 *	\param aoAddend An addend to be used in operation.
 *	\return Original value of variable pointer to by \a paoDestination.
 *
 *	The function computes sum of \a aoAddend and location pointer to by \a paoDestination
 *	stores it in the location and returns original value instead. The function is 
 *	close to both exchange and increment groups by semantics but it has been
 *	put into exchange group because it returns original value of the destination.
 *	Also, the function was not named \c AtomicAdd to avoid similarity with \c AtomicAnd.
 *	One of supposed applications is the resource counting.
 *	\see AtomicIncrement
 *	\see AtomicDecrement
 */

/**
 *	\fn bool _OU_CONVENTION_API AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
 *	\brief Compares comparand with destination and stores a new value if comparand 
 *	and destination match; returns if the assignment was performed or not.
 *	\param paoDestination A pointer to a variable the data is to be compared and assigned to.
 *	\param aoComparand A value to be used for comparison. 
 *	\param aoExchange A new value to be used for assignment. 
 *	\return \c true if exchange was performed and \c false otherwise.
 *
 *	The function performs comparison of \a aoComparand value with the value pointed
 *	by \a paoDestination. If the values match an \a aoExchange is assigned to 
 *	destination location. If values do not match, the restination remains unchanged.
 *	Function returns boolean status whether the match and assignment occurred or not.
 *	The most common uses are relaxed synchronization and construction of LIFO lists.
 *	\see AtomicExchange
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief Applies a mask with bitwise AND to the destination and returns original 
 *	value of destination.
 *	\param paoDestination A pointer to a variable the bitmask is to be applied to.
 *	\param aoBitMask A bitmask to be used in operation.
 *	\return Original value of variable pointed to by \a paoDestination.
 *
 *	\c AtomicAnd updates variable pointed to by \a paoDestination with result of
 *	 bitwise AND of \a aoBitMask and existing \a paoDestination target. The result 
 *	is original value that was pointer to by \a paoDestination before the operation
 *	was performed. Common applications are object state manipulations.
 *	\see AtomicOr
 *	\see AtomicXor
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief Applies a mask with bitwise OR to the destination and returns original 
 *	value of destination.
 *	\param paoDestination A pointer to a variable the bitmask is to be applied to.
 *	\param aoBitMask A bitmask to be used in operation.
 *	\return Original value of variable pointed to by \a paoDestination.
 *
 *	\c AtomicOr updates variable pointed to by \a paoDestination with result of
 *	 bitwise OR of \a aoBitMask and existing \a paoDestination target. The result 
 *	is original value that was pointer to by \a paoDestination before the operation 
 *	was performed. Common applications are object state manipulations.
 *	\see AtomicAnd
 *	\see AtomicXor
 */

/**
 *	\fn atomicord32 _OU_CONVENTION_API AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief Applies a mask with bitwise XOR to the destination and returns original 
 *	value of destination.
 *	\param paoDestination A pointer to a variable the bitmask is to be applied to.
 *	\param aoBitMask A bitmask to be used in operation.
 *	\return Original value of variable pointed to by \a paoDestination.
 *
 *	\c AtomicXor updates variable pointed to by \a paoDestination with result of
 *	 bitwise XOR of \a aoBitMask and existing \a paoDestination target. The result 
 *	is original value that was pointer to by \a paoDestination before the operation 
 *	was performed. Common applications are object state manipulations.
 *	\see AtomicAnd
 *	\see AtomicOr
 */

/**
 *	\fn atomicptr _OU_CONVENTION_API AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
 *	\brief The function is identical to \c AtomicExchange except that it operates
 *	with pointers rather than 32-bit integers.
 *	\see AtomicExchange
 */

/**
 *	\fn bool _OU_CONVENTION_API AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
 *	\brief The function is identical to \c AtomicCompareExchange except that it operates
 *	with pointers rather than 32-bit integers.
 *	\see AtomicCompareExchange
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
 *	\brief The function is identical to \c AtomicIncrement but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicIncrement
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
 *	\brief The function is identical to \c AtomicDecrement but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicDecrement
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
 *	\brief The function is identical to \c AtomicExchangeAdd but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicExchangeAdd
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief The function is identical to \c AtomicAnd but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicAnd
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief The function is identical to \c AtomicOr but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicOr
 */

/**
 *	\fn void _OU_CONVENTION_API AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
 *	\brief The function is identical to \c AtomicXor but does not return a result.
 *
 *	The function implementation can be faster on some platforms and it is recommended
 *	to use "NoResult" variants in cases when the result of operation or previous value 
 *	of destination is not used.
 *	\see AtomicXor
 */

/**
 *	\fn bool _OU_CONVENTION_API InitializeAtomicAPI()
 *	\brief Performs initialization tasks to allow using atomic functions.
 *	\return Boolean initialization status.
 *
 *	The function is required to be called before first use of atomic functions.
 *	The initialization uses reference counting, so multiple calls to \c InitializeAtomicAPI
 *	are allowed. However the counter is not thread safe. Therefore it is recommended
 *	that the function is always called from main thread on program startup or
 *	library initialization.
 *
 *	The function returns initialization status. If initialization succeeds, 
 *	\c FinalizeAtomicAPI is to be called for each call to \c InitializeAtomicAPI after
 *	atomic functions are not needed any more. If \c InitializeAtomicAPI returns 
 *	\c false, the atomic functions may not be used and \c FinalizeAtomicAPI must not be called.
 *	\see FinalizeAtomicAPI
 */

/**
 *	\fn void _OU_CONVENTION_API FinalizeAtomicAPI()
 *	\brief Finalizes objects and frees the memory that might be used to provide 
 *	functionality of atomic functions.
 *
 *	The function is to be called on program exit or library client detach to 
 *	release resources that might be allocated to support functionality of Atomic... 
 *	functions. The function must be called once for every successful call to 
 *	\c InitializeAtomicAPI. \c FinalizeAtomicAPI can not fail.
 *	\see InitializeAtomicAPI
 */


BEGIN_NAMESPACE_OU();


//////////////////////////////////////////////////////////////////////////
// Windows implementation 

#if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS


END_NAMESPACE_OU();


#include <windows.h>
#include <stddef.h>


BEGIN_NAMESPACE_OU();


typedef ULONG atomicord32;
typedef PVOID atomicptr;


#if _OU_COMPILER == _OU_COMPILER_MSVC && _OU_COMPILER_VERSION == _OU_COMPILER_VERSION_MSVC1998

#define __ou_intlck_value_t LONG
#define __ou_intlck_target_t LPLONG
#define __ou_xchgadd_target_t LPLONG
#define __ou_cmpxchg_value_t PVOID
#define __ou_cmpxchg_target_t PVOID *


#elif _OU_COMPILER == _OU_COMPILER_GCC

#define __ou_intlck_value_t LONG
#define __ou_intlck_target_t LPLONG
#define __ou_xchgadd_target_t LPLONG
#define __ou_cmpxchg_value_t LONG
#define __ou_cmpxchg_target_t LPLONG


#else // other compilers

#define __ou_intlck_value_t LONG
#define __ou_intlck_target_t volatile LONG *
#define __ou_xchgadd_target_t LPLONG
#define __ou_cmpxchg_value_t LONG
#define __ou_cmpxchg_target_t volatile LONG *


#endif // #if _OU_COMPILER == _OU_COMPILER_GCC


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	return ::InterlockedIncrement((__ou_intlck_target_t)paoDestination);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	return ::InterlockedDecrement((__ou_intlck_target_t)paoDestination);
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	return ::InterlockedExchange((__ou_intlck_target_t)paoDestination, aoExchange);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	return ::InterlockedExchangeAdd((__ou_xchgadd_target_t)paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	return (aoComparand == (atomicord32)::InterlockedCompareExchange((__ou_cmpxchg_target_t)paoDestination, (__ou_cmpxchg_value_t)aoExchange, (__ou_cmpxchg_value_t)aoComparand));
}


#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
        atomicord32 aoNewValue = (atomicord32)::InterlockedCompareExchange((__ou_cmpxchg_target_t)paoDestination, (__ou_cmpxchg_value_t)(aoOldValue & aoBitMask), (__ou_cmpxchg_value_t)aoOldValue);

		if (aoNewValue == aoOldValue)
		{
			break;
		}

		aoOldValue = aoNewValue;
    }
	
    return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
        atomicord32 aoNewValue = (atomicord32)::InterlockedCompareExchange((__ou_cmpxchg_target_t)paoDestination, (__ou_cmpxchg_value_t)(aoOldValue | aoBitMask), (__ou_cmpxchg_value_t)aoOldValue);
		
		if (aoNewValue == aoOldValue)
		{
			break;
		}
		
		aoOldValue = aoNewValue;
    }
	
    return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
        atomicord32 aoNewValue = (atomicord32)::InterlockedCompareExchange((__ou_cmpxchg_target_t)paoDestination, (__ou_cmpxchg_value_t)(aoOldValue ^ aoBitMask), (__ou_cmpxchg_value_t)aoOldValue);
		
		if (aoNewValue == aoOldValue)
		{
			break;
		}
		
		aoOldValue = aoNewValue;
    }
	
    return aoOldValue;
}


#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
#if _OU_TARGET_BITS == _OU_TARGET_BITS_32

	return (atomicptr)(ptrdiff_t)::InterlockedExchange((__ou_intlck_target_t)papDestination, (__ou_intlck_value_t)(ptrdiff_t)apExchange);
	

#else // #if _OU_TARGET_BITS == _OU_TARGET_BITS_64

	return ::InterlockedExchangePointer(papDestination, apExchange);
	

#endif // #if _OU_TARGET_BITS == ...
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
#if _OU_TARGET_BITS == _OU_TARGET_BITS_32
	
	return (apComparand == (atomicptr)(ptrdiff_t)::InterlockedCompareExchange((__ou_cmpxchg_target_t)papDestination, (__ou_cmpxchg_value_t)(ptrdiff_t)apExchange, (__ou_cmpxchg_value_t)(ptrdiff_t)apComparand));

	
#else // #if !defined(__OU_ATOMIC_WINDOWS_OLD_STYLE_PARAMS)
	
	return (apComparand == ::InterlockedCompareExchangePointer(papDestination, apExchange, apComparand));
	
	
#endif // #if !defined(__OU_ATOMIC_WINDOWS_OLD_STYLE_PARAMS)
}


#undef __ou_intlck_value_t
#undef __ou_intlck_target_t
#undef __ou_xchgadd_target_t
#undef __ou_cmpxchg_value_t
#undef __ou_cmpxchg_target_t


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_WINDOWS


//////////////////////////////////////////////////////////////////////////
// QNX implementation 

#if _OU_TARGET_OS == _OU_TARGET_OS_QNX

END_NAMESPACE_OU();


#include <atomic.h>
#include _NTO_CPU_HDR_(smpxchg.h)


BEGIN_NAMESPACE_OU();

typedef unsigned int atomicord32;
typedef void *atomicptr;


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	return (atomic_add_value(paoDestination, 1U) + 1U);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	return (atomic_sub_value(paoDestination, 1U) - 1U);
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	return _smp_xchg(paoDestination, aoExchange);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	return atomic_add_value(paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	return (aoComparand == (atomicord32)_smp_cmpxchg(paoDestination, aoComparand, aoExchange));
}


#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return atomic_clr_value(paoDestination, ~aoBitMask);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return atomic_set_value(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return atomic_toggle_value(paoDestination, aoBitMask);
}


#define __OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
{
	atomic_add(paoDestination, 1U);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
{
	atomic_sub(paoDestination, 1U);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	atomic_add(paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomic_clr(paoDestination, ~aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomic_set(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomic_toggle(paoDestination, aoBitMask);
}


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_QNX


//////////////////////////////////////////////////////////////////////////
// Mac implementation

#if _OU_TARGET_OS == _OU_TARGET_OS_MAC || _OU_TARGET_OS == _OU_TARGET_OS_IOS

#if MAC_OS_X_VERSION >= 1040


END_NAMESPACE_OU();


#include <libkern/OSAtomic.h>


BEGIN_NAMESPACE_OU();


typedef uint32_t atomicord32;
typedef void *atomicptr;


#define __ou_intlck_target_t volatile int32_t *
#define __ou_xchgadd_target_t volatile int32_t *
#define __ou_cmpxchg_value_t int32_t
#define __ou_cmpxchg_target_t volatile int32_t *
#define __ou_bitmsk_target_t volatile uint32_t *


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	return OSAtomicIncrement32Barrier((__ou_intlck_target_t)paoDestination);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	return OSAtomicDecrement32Barrier((__ou_intlck_target_t)paoDestination);
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	__ou_cmpxchg_value_t aoOldValue = *paoDestination;

	/*
	 *	Implementation Note:
	 *	It is safe to use compare-and-swap without memory barrier for subsequent attempts
	 *	because current thread had already had a barrier and does not have any additional
	 *	memory access until function exit. On the other hand it is expected that other 
	 *	threads will be using this API set for manipulations with paoDestination as well
	 *	and hence will not issue writes after/without memory barrier.
	 */
	for (bool bSwapExecuted = OSAtomicCompareAndSwap32Barrier(aoOldValue, aoExchange, (__ou_cmpxchg_target_t)paoDestination);
		!bSwapExecuted; bSwapExecuted = OSAtomicCompareAndSwap32(aoOldValue, aoExchange, (__ou_cmpxchg_target_t)paoDestination))
	{
		aoOldValue = *paoDestination;
	}
	
	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	return (OSAtomicAdd32Barrier(aoAddend, (__ou_xchgadd_target_t)paoDestination) - aoAddend);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	return OSAtomicCompareAndSwap32Barrier(aoComparand, aoExchange, (__ou_cmpxchg_target_t)paoDestination);
}


#if MAC_OS_X_VERSION >= 1050

#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return OSAtomicAnd32OrigBarrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return OSAtomicOr32OrigBarrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return OSAtomicXor32OrigBarrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}


#else // #if MAC_OS_X_VERSION < 1050 (...&& MAC_OS_X_VERSION >= 1040)

#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;

	/*
	 *	Implementation Note:
	 *	It is safe to use compare-and-swap without memory barrier for subsequent attempts
	 *	because current thread had already had a barrier and does not have any additional
	 *	memory access until function exit. On the other hand it is expected that other 
	 *	threads will be using this API set for manipulations with paoDestination as well
	 *	and hence will not issue writes after/without memory barrier.
	 */
	for (bool bSwapExecuted = OSAtomicCompareAndSwap32Barrier(aoOldValue, (aoOldValue & aoBitMask), paoDestination);
		!bSwapExecuted; bSwapExecuted = OSAtomicCompareAndSwap32(aoOldValue, (aoOldValue & aoBitMask), paoDestination))
	{
		aoOldValue = *paoDestination;
	}
	
	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;

	/*
	 *	Implementation Note:
	 *	It is safe to use compare-and-swap without memory barrier for subsequent attempts
	 *	because current thread had already had a barrier and does not have any additional
	 *	memory access until function exit. On the other hand it is expected that other 
	 *	threads will be using this API set for manipulations with paoDestination as well
	 *	and hence will not issue writes after/without memory barrier.
	 */
	for (bool bSwapExecuted = OSAtomicCompareAndSwap32Barrier(aoOldValue, (aoOldValue | aoBitMask), paoDestination);
		!bSwapExecuted; bSwapExecuted = OSAtomicCompareAndSwap32(aoOldValue, (aoOldValue | aoBitMask), paoDestination))
	{
		aoOldValue = *paoDestination;
	}
	
	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return (OSAtomicXor32Barrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination) ^ aoBitMask);
}


#endif // #if MAC_OS_X_VERSION < 1050 (...&& MAC_OS_X_VERSION >= 1040)


#if MAC_OS_X_VERSION >= 1050

#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
	atomicptr apOldValue = *papDestination;

	/*
	 *	Implementation Note:
	 *	It is safe to use compare-and-swap without memory barrier for subsequent attempts
	 *	because current thread had already had a barrier and does not have any additional
	 *	memory access until function exit. On the other hand it is expected that other 
	 *	threads will be using this API set for manipulations with papDestination as well
	 *	and hence will not issue writes after/without memory barrier.
	 */
	for (bool bSwapExecuted = OSAtomicCompareAndSwapPtrBarrier(apOldValue, apExchange, papDestination);
		!bSwapExecuted; bSwapExecuted = OSAtomicCompareAndSwapPtr(apOldValue, apExchange, papDestination))
	{
		apOldValue = *papDestination;
	}
	
	return apOldValue;
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
	return OSAtomicCompareAndSwapPtrBarrier(apComparand, apExchange, papDestination);
}


#endif // #if MAC_OS_X_VERSION >= 1050


#define __OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
{
	OSAtomicIncrement32Barrier((__ou_intlck_target_t)paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
{
	OSAtomicDecrement32Barrier((__ou_intlck_target_t)paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	OSAtomicAdd32Barrier(aoAddend, (__ou_xchgadd_target_t)paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	OSAtomicAnd32Barrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	OSAtomicOr32Barrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	OSAtomicXor32Barrier(aoBitMask, (__ou_bitmsk_target_t)paoDestination);
}


#endif // #if MAC_OS_X_VERSION >= 1040


#undef __ou_bitmsk_target_t
#undef __ou_cmpxchg_target_t
#undef __ou_cmpxchg_value_t
#undef __ou_xchgadd_target_t
#undef __ou_intlck_target_t

#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_MAC


//////////////////////////////////////////////////////////////////////////
// AIX implementation

#if _OU_TARGET_OS == _OU_TARGET_OS_AIX


END_NAMESPACE_OU();


#include <sys/atomic_op.h>


BEGIN_NAMESPACE_OU();


typedef unsigned int atomicord32;
typedef void *atomicptr;

#define __ou_cas_value_t int
#define __ou_caslp_value_t long


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	return (fetch_and_add((atomic_p)paoDestination, 1) + 1);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	return (fetch_and_add((atomic_p)paoDestination, -1) - 1);
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	__ou_cas_value_t aoOldValue = *paoDestination;

	while (!compare_and_swap((atomic_p)paoDestination, &aoOldValue, aoExchange))
	{
		// Do nothing
	}

	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	return fetch_and_add((atomic_p)paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	__ou_cas_value_t aoOldValue = aoComparand;

	return compare_and_swap((atomic_p)paoDestination, &aoOldValue, aoExchange);
}


#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return fetch_and_and((atomic_p)paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	return fetch_and_or((atomic_p)paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	volatile __ou_cas_value_t aoOldValue = *paoDestination;
	
	while (!compare_and_swap((atomic_p)paoDestination, &aoOldValue, aoOldValue ^ aoBitMask))
	{
		// Do nothing
	}
	
	return aoOldValue;
}


#if _OU_TARGET_BITS == _OU_TARGET_BITS_64 // Otherwise functions will be forwarded to ord32 further in this file

#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
	__ou_caslp_value_t liOldValue = (__ou_caslp_value_t)*papDestination;
	
	while (!compare_and_swaplp((atomic_l)papDestination, &liOldValue, (__ou_caslp_value_t)apExchange))
	{
		// Do nothing
	}
	
	return liOldValue;
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
	__ou_caslp_value_t liOldValue = (__ou_caslp_value_t)apComparand;
	
	return compare_and_swaplp((atomic_l)papDestination, &liOldValue, (__ou_caslp_value_t)apExchange);
}


#endif // #if _OU_TARGET_BITS == _OU_TARGET_BITS_64


#undef __ou_caslp_value_t
#undef __ou_cas_value_t


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_AIX


//////////////////////////////////////////////////////////////////////////
// SunOS implementation

#if _OU_TARGET_OS == _OU_TARGET_OS_SUNOS


END_NAMESPACE_OU();


#include <atomic.h>


BEGIN_NAMESPACE_OU();


typedef uint32_t atomicord32;
typedef void *atomicptr;


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	return atomic_inc_32_nv(paoDestination);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	return atomic_dec_32_nv(paoDestination);
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	return atomic_swap_32(paoDestination, aoExchange);
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	return (atomic_add_32_nv(paoDestination, aoAddend) - aoAddend);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	return (aoComparand == atomic_cas_32(paoDestination, aoComparand, aoExchange));
}


#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;

	while (true)
	{
		atomicord32 aoNewValue = atomic_cas_32(paoDestination, aoOldValue, aoOldValue & aoBitMask);

		if (aoNewValue == aoOldValue)
		{
			break;
		}

		aoOldValue = aoNewValue;
	}

	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
	while (true)
	{
		atomicord32 aoNewValue = atomic_cas_32(paoDestination, aoOldValue, aoOldValue | aoBitMask);
		
		if (aoNewValue == aoOldValue)
		{
			break;
		}
		
		aoOldValue = aoNewValue;
	}
	
	return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
	while (true)
	{
		atomicord32 aoNewValue = atomic_cas_32(paoDestination, aoOldValue, aoOldValue ^ aoBitMask);
		
		if (aoNewValue == aoOldValue)
		{
			break;
		}
		
		aoOldValue = aoNewValue;
	}
	
	return aoOldValue;
}


#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
	return atomic_swap_ptr(papDestination, apExchange);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
	return (apComparand == atomic_cas_ptr(papDestination, apComparand, apExchange));
}


#define __OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
{
	atomic_inc_32(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
{
	atomic_dec_32(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	atomic_add_32(paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomic_and_32(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomic_or_32(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	AtomicXor(paoDestination, aoBitMask);
}


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_SUNOS


//////////////////////////////////////////////////////////////////////////
// Generic UNIX implementation

#if _OU_TARGET_OS == _OU_TARGET_OS_GENUNIX

// No atomic functions for generic UNIX

#if _OU_COMPILER == _OU_COMPILER_GCC && (_OU_TARGET_ARCH == _OU_TARGET_ARCH_X86 || _OU_TARGET_ARCH == _OU_TARGET_ARCH_X64)

#define __OU_ATOMIC_X86_ASSEMBLER_USE_AUTOENABLED    1


#endif // Otherwise the x86 assembler implementation must be engaged explicitly


#if _OU_ATOMIC_USE_X86_ASSEMBLER || (__OU_ATOMIC_X86_ASSEMBLER_USE_AUTOENABLED && !defined(_OU_ATOMIC_USE_X86_ASSEMBLER))

typedef uint32ou atomicord32;
typedef void *atomicptr;


struct _ou_atomic_CLargeStruct
{ 
	unsigned int	m_uiData[32];
};


#define __OU_ATOMIC_ORD32_FUNCTIONS_DEFINED


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicIncrement(volatile atomicord32 *paoDestination)
{
	register atomicord32 aoResult = 1;

	asm volatile (
		"lock; xaddl %2, %0;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=r" (aoResult)
		: "1" (aoResult), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult + 1;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicDecrement(volatile atomicord32 *paoDestination)
{
	register atomicord32 aoResult = (atomicord32)(-1);

	asm volatile (
		"lock; xaddl %2, %0;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=r" (aoResult)
		: "1" (aoResult), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult - 1;
}


static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
	register atomicord32 aoResult;

	asm volatile (
		"xchgl %2, %0;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=r" (aoResult)
		: "1" (aoExchange), "m" (*paoDestination)
		: "memory");

	return aoResult;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	register atomicord32 aoResult;

	asm volatile (
		"lock; xaddl %2, %0;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=r" (aoResult)
		: "1" (aoAddend), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult;
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
	register bool bResult;

	asm volatile (
        "lock; cmpxchgl %3, %0;"
        "setzb %1;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=a" (bResult)
		: "a" (aoComparand), "r" (aoExchange), "m" (*paoDestination)
		: "memory", "cc");

	return bResult;
}


#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	register atomicord32 aoResult;
	register atomicord32 aoExchange;

	asm volatile (
	"0:;"
		"movl  %4, %2;"
		"andl  %3, %2;"
		"lock; cmpxchgl %2, %0;"
		"jnz   0b;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=a" (aoResult), "=&r" (aoExchange)
		: "a" (*paoDestination), "g" (aoBitMask), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	register atomicord32 aoResult;
	register atomicord32 aoExchange;

	asm volatile (
	"0:;"
		"movl  %4, %2;"
		"orl   %3, %2;"
		"lock; cmpxchgl %2, %0;"
		"jnz   0b;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=a" (aoResult), "=&r" (aoExchange)
		: "a" (*paoDestination), "g" (aoBitMask), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	register atomicord32 aoResult;
	register atomicord32 aoExchange;

	asm volatile (
	"0:;"
		"movl  %4, %2;"
		"xorl  %3, %2;"
		"lock; cmpxchgl %2, %0;"
		"jnz   0b;"
		: "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination), "=a" (aoResult), "=&r" (aoExchange)
		: "a" (*paoDestination), "g" (aoBitMask), "m" (*paoDestination)
		: "memory", "cc");

	return aoResult;
}


#define __OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
{
    AtomicIncrement(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
{
    AtomicDecrement(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
    AtomicExchangeAdd(paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
    asm volatile (
        "lock; andl %1, %0;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination) 
        : "r" (aoBitMask), "m" (*paoDestination)
        : "memory", "cc");
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
    asm volatile (
        "lock; orl %1, %0;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination) 
        : "r" (aoBitMask), "m" (*paoDestination)
        : "memory", "cc");
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
    asm volatile (
        "lock; xorl %1, %0;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)paoDestination) 
        : "r" (aoBitMask), "m" (*paoDestination)
        : "memory", "cc");
}


#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
    register atomicptr apResult;

    asm volatile (
        "xchg %2, %0;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)papDestination), "=r" (apResult)
        : "1" (apExchange), "m" (*papDestination)
        : "memory");

    return apResult;
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
    register bool bResult;

    asm volatile (
        "lock; cmpxchg %3, %0;"
        "setzb %1;"
        : "=m" (*(volatile _ou_atomic_CLargeStruct *)papDestination), "=a" (bResult)
        : "a" (apComparand), "r" (apExchange), "m" (*papDestination)
        : "memory", "cc");

    return bResult;
}


#endif // #if defined(_OU_ATOMIC_USE_X86_ASSEMBLER)


#endif // #if _OU_TARGET_OS == _OU_TARGET_OS_GENUNIX


//////////////////////////////////////////////////////////////////////////
// BitMask to CompareExchange forwarders

#if defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED) && !defined(__OU_ATOMIC_BIT_FUNCTIONS_DEFINED)

#define __OU_ATOMIC_BIT_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
		if (AtomicCompareExchange(paoDestination, aoOldValue, (aoOldValue & aoBitMask)))
		{
			break;
		}
		
		aoOldValue = *paoDestination;
    }
	
    return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
		if (AtomicCompareExchange(paoDestination, aoOldValue, (aoOldValue | aoBitMask)))
		{
			break;
		}
		
		aoOldValue = *paoDestination;
    }
	
    return aoOldValue;
}

static _OU_ALWAYSINLINE atomicord32 _OU_CONVENTION_API 
/*atomicord32 */AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	atomicord32 aoOldValue = *paoDestination;
	
    while (true)
	{
		if (AtomicCompareExchange(paoDestination, aoOldValue, (aoOldValue ^ aoBitMask)))
		{
			break;
		}
		
		aoOldValue = *paoDestination;
    }
	
    return aoOldValue;
}


#endif // #if defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED) && !defined(__OU_ATOMIC_BIT_FUNCTIONS_DEFINED)


//////////////////////////////////////////////////////////////////////////
// Pointer to ord32 forwarders

#if defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED) && !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED) && _OU_TARGET_BITS == _OU_TARGET_BITS_32

#define __OU_ATOMIC_PTR_FUNCTIONS_DEFINED

static _OU_ALWAYSINLINE atomicptr _OU_CONVENTION_API 
/*atomicptr */AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
	return (atomicptr)AtomicExchange((volatile atomicord32 *)papDestination, (atomicord32)apExchange);
}

static _OU_ALWAYSINLINE bool _OU_CONVENTION_API 
/*bool */AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
	return AtomicCompareExchange((volatile atomicord32 *)papDestination, (atomicord32)apComparand, (atomicord32)apExchange);
}


#endif // #if defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED) && !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED) && _OU_TARGET_BITS == _OU_TARGET_BITS_32


//////////////////////////////////////////////////////////////////////////
// Atomic-via-mutex implementations

#if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)


END_NAMESPACE_OU();


#include <stddef.h>


BEGIN_NAMESPACE_OU();


typedef int32_t atomicord32;
typedef void *atomicptr;


atomicord32 _OU_CONVENTION_API AtomicIncrement(volatile atomicord32 *paoDestination);
atomicord32 _OU_CONVENTION_API AtomicDecrement(volatile atomicord32 *paoDestination);

atomicord32 _OU_CONVENTION_API AtomicExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange);
atomicord32 _OU_CONVENTION_API AtomicExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend);
bool _OU_CONVENTION_API AtomicCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange);

atomicord32 _OU_CONVENTION_API AtomicAnd(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);
atomicord32 _OU_CONVENTION_API AtomicOr(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);
atomicord32 _OU_CONVENTION_API AtomicXor(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);


#if defined(__OU_ATOMIC_BIT_FUNCTIONS_DEFINED)

#error Internal error (__OU_ATOMIC_BIT_FUNCTIONS_DEFINED must not be defined in this case)


#endif // #if defined(__OU_ATOMIC_BIT_FUNCTIONS_DEFINED)

#if defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)

#error Internal error (__OU_ATOMIC_PTR_FUNCTIONS_DEFINED must not be defined in this case)


#endif // #if defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)


#endif // #if !defined(__OU_ATOMIC_ORD32_FUNCTIONS_DEFINED)


#if !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)

atomicptr _OU_CONVENTION_API AtomicExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange);
bool _OU_CONVENTION_API AtomicCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange);


#if defined(__OU_DOXYGEN__) 

// Doxygen fooling declarations (used for documentation generation only)
void _OU_CONVENTION_API AtomicIncrementNoResult(volatile atomicord32 *paoDestination);
void _OU_CONVENTION_API AtomicDecrementNoResult(volatile atomicord32 *paoDestination);
void _OU_CONVENTION_API AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend);
void _OU_CONVENTION_API AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);
void _OU_CONVENTION_API AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);
void _OU_CONVENTION_API AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask);


#endif // #if defined(__OU_DOXYGEN__)


#define __OU_ATOMIC_OPERATIONS_VIA_MUTEXES
#define __OU_ATOMIC_INITIALIZATION_FUNCTIONS_REQUIRED

// Initialization must be performed from main thread
bool _OU_CONVENTION_API InitializeAtomicAPI();
void _OU_CONVENTION_API FinalizeAtomicAPI();


#endif // #if !defined(__OU_ATOMIC_PTR_FUNCTIONS_DEFINED)


//////////////////////////////////////////////////////////////////////////
// No-result to result forwarders

#if !defined(__OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED)

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicIncrementNoResult(volatile atomicord32 *paoDestination)
{
	AtomicIncrement(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicDecrementNoResult(volatile atomicord32 *paoDestination)
{
	AtomicDecrement(paoDestination);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicExchangeAddNoResult(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
	AtomicExchangeAdd(paoDestination, aoAddend);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicAndNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	AtomicAnd(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicOrNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	AtomicOr(paoDestination, aoBitMask);
}

static _OU_ALWAYSINLINE void _OU_CONVENTION_API 
/*void */AtomicXorNoResult(volatile atomicord32 *paoDestination, atomicord32 aoBitMask)
{
	AtomicXor(paoDestination, aoBitMask);
}


#endif // #if !defined(__OU_ATOMIC_ORD32_NORESULT_FUNCTIONS_DEFINED)


//////////////////////////////////////////////////////////////////////////
// Atomic initialization function stubs

#if !defined(__OU_ATOMIC_INITIALIZATION_FUNCTIONS_REQUIRED)

// Initialization must be performed from main thread
static _OU_INLINE bool _OU_CONVENTION_API InitializeAtomicAPI()
{
	// Do nothing
	
	return true;
}

static _OU_INLINE void _OU_CONVENTION_API FinalizeAtomicAPI()
{
	// Do nothing
}


#endif // #if !defined(__OU_ATOMIC_INITIALIZE_FUNCTIONS_DEFINED)


END_NAMESPACE_OU();


#endif // #if _OU_FEATURE_SET >= _OU_FEATURE_SET_ATOMICS


#endif // #ifndef __OU_ATOMIC_H_INCLUDED
