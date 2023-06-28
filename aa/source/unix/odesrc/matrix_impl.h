

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

#ifndef _ODE_MATRIX_IMPL_H_
#define _ODE_MATRIX_IMPL_H_


#include "fastsolve_impl.h"
#include "fastltsolve_impl.h"


template<unsigned int a_stride, unsigned int d_stride>
void dxtVectorScale (dReal *aStart, const dReal *dStart, unsigned elementCount)
{
    dAASSERT (aStart && dStart && elementCount >= 0);
    
    const unsigned step = 4;

    dReal *ptrA = aStart;
    const dReal *ptrD = dStart;
    const dReal *const dStepsEnd = dStart + (size_t)(elementCount & ~(step - 1)) * d_stride;
    for (; ptrD != dStepsEnd; ptrA += step * a_stride, ptrD += step * d_stride) 
    {
        dReal a0 = ptrA[0], a1 = ptrA[1 * a_stride], a2 = ptrA[2 * a_stride], a3 = ptrA[3 * a_stride];
        dReal d0 = ptrD[0], d1 = ptrD[1 * d_stride], d2 = ptrD[2 * d_stride], d3 = ptrD[3 * d_stride];
        a0 *= d0;
        a1 *= d1;
        a2 *= d2;
        a3 *= d3;
        ptrA[0] = a0; ptrA[1 * a_stride] = a1; ptrA[2 * a_stride] = a2; ptrA[3 * a_stride] = a3;
        dSASSERT(step == 4);
    }

    switch (elementCount & (step - 1))
    {
        case 3:
        {
            dReal a2 = ptrA[2 * a_stride];
            dReal d2 = ptrD[2 * d_stride];
            ptrA[2 * a_stride] = a2 * d2;
            // break; -- proceed to case 2
        }

        case 2:
        {
            dReal a1 = ptrA[1 * a_stride];
            dReal d1 = ptrD[1 * d_stride];
            ptrA[1 * a_stride] = a1 * d1;
            // break; -- proceed to case 1
        }

        case 1:
        {
            dReal a0 = ptrA[0];
            dReal d0 = ptrD[0];
            ptrA[0] = a0 * d0;
            break;
        }
    }
}


template<unsigned int d_stride, unsigned int b_stride>
void dxtSolveLDLT (const dReal *L, const dReal *d, dReal *b, unsigned rowCount, unsigned rowSkip)
{
    dAASSERT(L != NULL);
    dAASSERT(d != NULL);
    dAASSERT(b != NULL);
    dAASSERT(rowCount > 0);
    dAASSERT(rowSkip >= rowCount);

    dxtSolveL1<b_stride> (L, b, rowCount, rowSkip);
    dxtVectorScale<b_stride, d_stride> (b, d, rowCount);
    dxtSolveL1T<b_stride> (L, b, rowCount, rowSkip);
}


#endif
