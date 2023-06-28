/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
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

#ifndef _ODE_THREADINGUTILS_H_
#define _ODE_THREADINGUTILS_H_


#include "odeou.h"


#if !dTHREADING_INTF_DISABLED

static inline 
bool ThrsafeCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
    return AtomicCompareExchange(paoDestination, aoComparand, aoExchange);
}

static inline 
atomicord32 ThrsafeExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
    return AtomicExchange(paoDestination, aoExchange);
}

static inline 
void ThrsafeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
    AtomicExchangeAddNoResult(paoDestination, aoAddend);
}

static inline 
atomicord32 ThrsafeExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
    return AtomicExchangeAdd(paoDestination, aoAddend);
}

static inline 
bool ThrsafeCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
    return AtomicCompareExchangePointer(papDestination, apComparand, apExchange);
}

static inline 
atomicptr ThrsafeExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
    return AtomicExchangePointer(papDestination, apExchange);
}


#else // #if dTHREADING_INTF_DISABLED

static inline 
bool ThrsafeCompareExchange(volatile atomicord32 *paoDestination, atomicord32 aoComparand, atomicord32 aoExchange)
{
    return (*paoDestination == aoComparand) ? ((*paoDestination = aoExchange), true) : false;
}

static inline 
atomicord32 ThrsafeExchange(volatile atomicord32 *paoDestination, atomicord32 aoExchange)
{
    atomicord32 aoDestinationValue = *paoDestination;
    *paoDestination = aoExchange;
    return aoDestinationValue;
}

static inline 
void ThrsafeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
    *paoDestination += aoAddend;
}

static inline 
atomicord32 ThrsafeExchangeAdd(volatile atomicord32 *paoDestination, atomicord32 aoAddend)
{
    atomicord32 aoDestinationValue = *paoDestination;
    *paoDestination += aoAddend;
    return aoDestinationValue;
}

static inline 
bool ThrsafeCompareExchangePointer(volatile atomicptr *papDestination, atomicptr apComparand, atomicptr apExchange)
{
    return (*papDestination == apComparand) ? ((*papDestination = apExchange), true) : false;
}

static inline 
atomicptr ThrsafeExchangePointer(volatile atomicptr *papDestination, atomicptr apExchange)
{
    atomicptr apDestinationValue = *papDestination;
    *papDestination = apExchange;
    return apDestinationValue;
}


#endif // #if dTHREADING_INTF_DISABLED


static inline 
unsigned int ThrsafeIncrementIntUpToLimit(volatile atomicord32 *storagePointer, unsigned int limitValue)
{
    unsigned int resultValue;
    while (true) {
        resultValue = *storagePointer;
        if (resultValue == limitValue) {
            break;
        }
        if (ThrsafeCompareExchange(storagePointer, (atomicord32)resultValue, (atomicord32)(resultValue + 1))) {
            break;
        }
    }
    return resultValue;
}

static inline 
size_t ThrsafeIncrementSizeUpToLimit(volatile size_t *storagePointer, size_t limitValue)
{
    size_t resultValue;
    while (true) {
        resultValue = *storagePointer;
        if (resultValue == limitValue) {
            break;
        }
        if (ThrsafeCompareExchangePointer((volatile atomicptr *)storagePointer, (atomicptr)resultValue, (atomicptr)(resultValue + 1))) {
            break;
        }
    }
    return resultValue;
}



#endif // _ODE_THREADINGUTILS_H_
