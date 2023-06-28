/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading atomics providers file.                                     *
 * Copyright (C) 2011-2012 Oleh Derevenko. All rights reserved.          *
 * e-mail: odar@eleks.com (change all "a" to "e")                        *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 2.1 of the License, or (at  *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE.TXT.                                               *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *                                                                       *
 * This library is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files    *
 * LICENSE.TXT and LICENSE-BSD.TXT for more details.                     *
 *                                                                       *
 *************************************************************************/

/*
 *  Fake atomics provider for built-in threading support provider.
 *  OU-based atomics provider for built-in threading support provider.
 *
 *  The classes have been moved into a separate header as they are to be used 
 *  in both WIN and POSIX implementations.
 */


#ifndef _ODE_THREADING_ATOMICS_PROVS_H_
#define _ODE_THREADING_ATOMICS_PROVS_H_


#include <ode/odeconfig.h>
#include <ode/error.h>


/************************************************************************/
/* Fake atomics provider class implementation                           */
/************************************************************************/

class dxFakeAtomicsProvider
{
public:
    typedef unsigned long atomicord_t;
    typedef void *atomicptr_t;

public:
    static void IncrementTargetNoRet(volatile atomicord_t *value_accumulator_ptr)
    {
        ++(*value_accumulator_ptr);
    }

    static void DecrementTargetNoRet(volatile atomicord_t *value_accumulator_ptr)
    {
        --(*value_accumulator_ptr);
    }

    static atomicord_t QueryTargetValue(volatile atomicord_t *value_storage_ptr)
    {
        return *value_storage_ptr;
    }

    template<unsigned type_size>
    static size_t AddValueToTarget(volatile void *value_accumulator_ptr, ptrdiff_t value_addend);

    static bool CompareExchangeTargetPtr(volatile atomicptr_t *pointer_storage_ptr, 
        atomicptr_t comparand_value, atomicptr_t new_value)
    {
        bool exchange_result = false;

        atomicptr_t original_value = *pointer_storage_ptr;

        if (original_value == comparand_value)
        {
            *pointer_storage_ptr = new_value;

            exchange_result = true;
        }

        return exchange_result;
    }
};

template<>
inline size_t dxFakeAtomicsProvider::AddValueToTarget<sizeof(dxFakeAtomicsProvider::atomicord_t)>(volatile void *value_accumulator_ptr, ptrdiff_t value_addend)
{
    atomicord_t original_value = *(volatile atomicord_t *)value_accumulator_ptr;

    *(volatile atomicord_t *)value_accumulator_ptr = original_value + (atomicord_t)value_addend;

    return original_value;
}

template<>
inline size_t dxFakeAtomicsProvider::AddValueToTarget<2 * sizeof(dxFakeAtomicsProvider::atomicord_t)>(volatile void *value_accumulator_ptr, ptrdiff_t value_addend)
{
    atomicptr_t original_value = *(volatile atomicptr_t *)value_accumulator_ptr;

    *(volatile atomicptr_t *)value_accumulator_ptr = (atomicptr_t)((size_t)original_value + (size_t)value_addend);

    return (size_t)original_value;
}


#if dBUILTIN_THREADING_IMPL_ENABLED

/************************************************************************/
/* dxOUAtomicsProvider class implementation                             */
/************************************************************************/

#if !dOU_ENABLED
#error OU library must be enabled for this to compile
#elif !dATOMICS_ENABLED
#error OU Atomics must be enabled for this to compile
#endif
#include "odeou.h"

class dxOUAtomicsProvider
{
public:
    typedef _OU_NAMESPACE::atomicord32 atomicord_t;
    typedef _OU_NAMESPACE::atomicptr atomicptr_t;

public:
    static void IncrementTargetNoRet(volatile atomicord_t *value_accumulator_ptr)
    {
        _OU_NAMESPACE::AtomicIncrementNoResult(value_accumulator_ptr);
    }

    static void DecrementTargetNoRet(volatile atomicord_t *value_accumulator_ptr)
    {
        _OU_NAMESPACE::AtomicDecrementNoResult(value_accumulator_ptr);
    }

    static atomicord_t QueryTargetValue(volatile atomicord_t *value_storage_ptr)
    {
        // Query value with memory barrier before
        atomicord_t result_value = *value_storage_ptr;

        if (!_OU_NAMESPACE::AtomicCompareExchange(value_storage_ptr, result_value, result_value))
        {
            result_value = *value_storage_ptr;
        }

        return result_value;
    }

    template<unsigned type_size>
    static size_t AddValueToTarget(volatile void *value_accumulator_ptr, ptrdiff_t value_addend);

    static bool CompareExchangeTargetPtr(volatile atomicptr_t *pointer_storage_ptr, 
        atomicptr_t comparand_value, atomicptr_t new_value)
    {
        return _OU_NAMESPACE::AtomicCompareExchangePointer(pointer_storage_ptr, comparand_value, new_value);
    }
};

template<>
inline size_t dxOUAtomicsProvider::AddValueToTarget<sizeof(dxOUAtomicsProvider::atomicord_t)>(volatile void *value_accumulator_ptr, ptrdiff_t value_addend)
{
    return _OU_NAMESPACE::AtomicExchangeAdd((volatile atomicord_t *)value_accumulator_ptr, (atomicord_t)value_addend);
}

template<>
inline size_t dxOUAtomicsProvider::AddValueToTarget<2 * sizeof(dxOUAtomicsProvider::atomicord_t)>(volatile void *value_accumulator_ptr, ptrdiff_t value_addend)
{
    atomicptr_t original_value;

    while (true)
    {
        original_value = *(volatile atomicptr_t *)value_accumulator_ptr;

        atomicptr_t new_value = (atomicptr_t)((size_t)original_value + (size_t)value_addend);
        if (_OU_NAMESPACE::AtomicCompareExchangePointer((volatile atomicptr_t *)value_accumulator_ptr, original_value, new_value))
        {
            break;
        }
    }

    return (size_t)original_value;
}


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#endif // #ifndef _ODE_THREADING_ATOMICS_PROVS_H_
