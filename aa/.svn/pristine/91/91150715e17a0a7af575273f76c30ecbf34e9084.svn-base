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

/* generated code, do not edit. */

#include <ode/common.h>
#include "config.h"
#include "matrix.h"
#include "error.h"

#include "fastltsolve_impl.h"


void dxSolveL1T(const dReal *L, dReal *b, unsigned n, unsigned lskip1)
{
    dxtSolveL1T<1>(L, b, n, lskip1);
}


#undef dSolveL1T

void dSolveL1T (const dReal *L, dReal *B, int n, int lskip1)
{
    dAASSERT(n != 0);

    if (n != 0)
    {
        dAASSERT(L != NULL);
        dAASSERT(B != NULL);

        dxSolveL1T(L, B, n, lskip1);
    }
}

