

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

// Code style improvements and optimizations by Oleh Derevenko ????-2017


#ifndef _ODE_FASTLDLT_IMPL_H_
#define _ODE_FASTLDLT_IMPL_H_


#include "error.h"


static void dxSolveL1_2 (const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip);
template<unsigned int d_stride>
void dxScaleAndFactorizeL1_2(dReal *ARow, dReal *d, unsigned rowIndex, unsigned rowSkip);
template<unsigned int d_stride>
inline void dxScaleAndFactorizeFirstL1Row_2(dReal *ARow, dReal *d, unsigned rowSkip);

static void dxSolveL1_1 (const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip);
template<unsigned int d_stride>
void dxScaleAndFactorizeL1_1(dReal *ARow, dReal *d, unsigned rowIndex);
template<unsigned int d_stride>
inline void dxScaleAndFactorizeFirstL1Row_1(dReal *ARow, dReal *d);


template<unsigned int d_stride>
void dxtFactorLDLT(dReal *A, dReal *d, unsigned rowCount, unsigned rowSkip)
{
    if (rowCount < 1) return;

    const unsigned lastRowIndex = rowCount - 1;

    dReal *ARow = A;
    unsigned blockStartRow = 0;
    /* compute blocks of 2 rows */
    bool subsequentPass = false;
    for (; blockStartRow < lastRowIndex; subsequentPass = true, ARow += 2 * rowSkip, blockStartRow += 2) 
    {
        if (subsequentPass)
        {
            /* solve L*(D*l)=a, l is scaled elements in 2 x i block at A(i,0) */
            dxSolveL1_2(A, ARow, blockStartRow, rowSkip);
            dxScaleAndFactorizeL1_2<d_stride>(ARow, d, blockStartRow, rowSkip);
        }
        else
        {
            dxScaleAndFactorizeFirstL1Row_2<d_stride>(ARow, d, rowSkip);
        }
        /* done factorizing 2 x 2 block */
    }

    /* compute the (less than 2) rows at the bottom */
    if (!subsequentPass || blockStartRow == lastRowIndex)
    {
        if (subsequentPass)
        {
            dxSolveL1_1(A, ARow, blockStartRow, rowSkip);
            dxScaleAndFactorizeL1_1<d_stride>(ARow, d, blockStartRow);
        }
        else
        {
            dxScaleAndFactorizeFirstL1Row_1<d_stride>(ARow, d);
        }
        /* done factorizing 1 x 1 block */
    }
}

/* solve L*X=B, with B containing 2 right hand sides.
 * L is an n*n lower triangular matrix with ones on the diagonal.
 * L is stored by rows and its leading dimension is rowSkip.
 * B is an n*2 matrix that contains the right hand sides.
 * B is stored by columns and its leading dimension is also rowSkip.
 * B is overwritten with X.
 * this processes blocks of 2*2.
 * if this is in the factorizer source file, n must be a multiple of 2.
 */
static 
void dxSolveL1_2(const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip)
{
    dIASSERT(rowCount != 0);
    dIASSERT(rowCount % 2 == 0);

    /* compute all 2 x 2 blocks of X */
    unsigned blockStartRow = 0;
    for (bool exitLoop = false, subsequentPass = false; !exitLoop; subsequentPass = true, exitLoop = (blockStartRow += 2) == rowCount) 
    {
        const dReal *ptrLElement;
        dReal *ptrBElement;

        /* declare variables - Z matrix */
        dReal Z11, Z12, Z21, Z22;

        /* compute all 2 x 2 block of X, from rows i..i+2-1 */
        if (subsequentPass)
        {
            ptrLElement = L + blockStartRow * rowSkip;
            ptrBElement = B;

            /* set Z matrix to 0 */
            Z11 = 0; Z12 = 0; Z21 = 0; Z22 = 0;

            /* the inner loop that computes outer products and adds them to Z */
            // The iteration starts with even number and decreases it by 2. So, it must end in zero
            for (unsigned columnCounter = blockStartRow; ;) 
            {
                /* declare p and q vectors, etc */
                dReal p1, q1, p2, q2;

                /* compute outer product and add it to the Z matrix */
                p1 = ptrLElement[0];
                q1 = ptrBElement[0];
                Z11 += p1 * q1;
                q2 = ptrBElement[rowSkip];
                Z12 += p1 * q2;
                p2 = ptrLElement[rowSkip];
                Z21 += p2 * q1;
                Z22 += p2 * q2;

                /* compute outer product and add it to the Z matrix */
                p1 = ptrLElement[1];
                q1 = ptrBElement[1];
                Z11 += p1 * q1;
                q2 = ptrBElement[1 + rowSkip];
                Z12 += p1 * q2;
                p2 = ptrLElement[1 + rowSkip];
                Z21 += p2 * q1;
                Z22 += p2 * q2;

                if (columnCounter > 6)
                {
                    columnCounter -= 6;

                    /* advance pointers */
                    ptrLElement += 6;
                    ptrBElement += 6;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-4];
                    q1 = ptrBElement[-4];
                    Z11 += p1 * q1;
                    q2 = ptrBElement[-4 + rowSkip];
                    Z12 += p1 * q2;
                    p2 = ptrLElement[-4 + rowSkip];
                    Z21 += p2 * q1;
                    Z22 += p2 * q2;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-3];
                    q1 = ptrBElement[-3];
                    Z11 += p1 * q1;
                    q2 = ptrBElement[-3 + rowSkip];
                    Z12 += p1 * q2;
                    p2 = ptrLElement[-3 + rowSkip];
                    Z21 += p2 * q1;
                    Z22 += p2 * q2;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-2];
                    q1 = ptrBElement[-2];
                    Z11 += p1 * q1;
                    q2 = ptrBElement[-2 + rowSkip];
                    Z12 += p1 * q2;
                    p2 = ptrLElement[-2 + rowSkip];
                    Z21 += p2 * q1;
                    Z22 += p2 * q2;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-1];
                    q1 = ptrBElement[-1];
                    Z11 += p1 * q1;
                    q2 = ptrBElement[-1 + rowSkip];
                    Z12 += p1 * q2;
                    p2 = ptrLElement[-1 + rowSkip];
                    Z21 += p2 * q1;
                    Z22 += p2 * q2;
                }
                else
                {
                    /* advance pointers */
                    ptrLElement += 2;
                    ptrBElement += 2;

                    if ((columnCounter -= 2) == 0)
                    {
                        break;
                    }
                }
                /* end of inner loop */
            }
        }
        else
        {
            ptrLElement = L/* + blockStartRow * rowSkip*/; dIASSERT(blockStartRow == 0);
            ptrBElement = B;

            /* set Z matrix to 0 */
            Z11 = 0; Z12 = 0; Z21 = 0; Z22 = 0;
        }

        /* finish computing the X(i) block */
        
        dReal Y11 = ptrBElement[0] - Z11;
        dReal Y12 = ptrBElement[rowSkip] - Z12;

        dReal p2 = ptrLElement[rowSkip];

        ptrBElement[0] = Y11;
        ptrBElement[rowSkip] = Y12;

        dReal Y21 = ptrBElement[1] - Z21 - p2 * Y11;
        dReal Y22 = ptrBElement[1 + rowSkip] - Z22 - p2 * Y12;

        ptrBElement[1] = Y21;
        ptrBElement[1 + rowSkip] = Y22;
        /* end of outer loop */
    }
}

template<unsigned int d_stride>
void dxScaleAndFactorizeL1_2(dReal *ARow, dReal *d, unsigned factorizationRow, unsigned rowSkip)
{
    dIASSERT(factorizationRow != 0);
    dIASSERT(factorizationRow % 2 == 0);

    dReal *ptrAElement = ARow;
    dReal *ptrDElement = d;

    /* scale the elements in a 2 x i block at A(i,0), and also */
    /* compute Z = the outer product matrix that we'll need. */
    dReal Z11 = 0, Z21 = 0, Z22 = 0;

    for (unsigned columnCounter = factorizationRow; ; ) 
    {
        dReal p1, q1, p2, q2, dd;

        p1 = ptrAElement[0];
        p2 = ptrAElement[rowSkip];
        dd = ptrDElement[0 * d_stride];
        q1 = p1 * dd;
        q2 = p2 * dd;
        ptrAElement[0] = q1;
        ptrAElement[rowSkip] = q2;
        Z11 += p1 * q1;
        Z21 += p2 * q1;
        Z22 += p2 * q2;

        p1 = ptrAElement[1];
        p2 = ptrAElement[1 + rowSkip];
        dd = ptrDElement[1 * d_stride];
        q1 = p1 * dd;
        q2 = p2 * dd;
        ptrAElement[1] = q1;
        ptrAElement[1 + rowSkip] = q2;
        Z11 += p1 * q1;
        Z21 += p2 * q1;
        Z22 += p2 * q2;

        if (columnCounter > 6)
        {
            columnCounter -= 6;

            ptrAElement += 6;
            ptrDElement += 6 * d_stride;

            p1 = ptrAElement[-4];
            p2 = ptrAElement[-4 + rowSkip];
            dd = ptrDElement[-4 * (int)d_stride];
            q1 = p1 * dd;
            q2 = p2 * dd;
            ptrAElement[-4] = q1;
            ptrAElement[-4 + rowSkip] = q2;
            Z11 += p1 * q1;
            Z21 += p2 * q1;
            Z22 += p2 * q2;

            p1 = ptrAElement[-3];
            p2 = ptrAElement[-3 + rowSkip];
            dd = ptrDElement[-3 * (int)d_stride];
            q1 = p1 * dd;
            q2 = p2 * dd;
            ptrAElement[-3] = q1;
            ptrAElement[-3 + rowSkip] = q2;
            Z11 += p1 * q1;
            Z21 += p2 * q1;
            Z22 += p2 * q2;

            p1 = ptrAElement[-2];
            p2 = ptrAElement[-2 + rowSkip];
            dd = ptrDElement[-2 * (int)d_stride];
            q1 = p1 * dd;
            q2 = p2 * dd;
            ptrAElement[-2] = q1;
            ptrAElement[-2 + rowSkip] = q2;
            Z11 += p1 * q1;
            Z21 += p2 * q1;
            Z22 += p2 * q2;

            p1 = ptrAElement[-1];
            p2 = ptrAElement[-1 + rowSkip];
            dd = ptrDElement[-1 * (int)d_stride];
            q1 = p1 * dd;
            q2 = p2 * dd;
            ptrAElement[-1] = q1;
            ptrAElement[-1 + rowSkip] = q2;
            Z11 += p1 * q1;
            Z21 += p2 * q1;
            Z22 += p2 * q2;
        }
        else
        {
            ptrAElement += 2;
            ptrDElement += 2 * d_stride;

            if ((columnCounter -= 2) == 0)
            {
                break;
            }
        }
    }

    /* solve for diagonal 2 x 2 block at A(i,i) */
    dReal Y11 = ptrAElement[0] - Z11;
    dReal Y21 = ptrAElement[rowSkip] - Z21;
    dReal Y22 = ptrAElement[1 + rowSkip] - Z22;

    /* factorize 2 x 2 block Y, ptrDElement */
    /* factorize row 1 */
    dReal dd = dRecip(Y11);

    ptrDElement[0 * d_stride] = dd;
    dIASSERT(ptrDElement == d + (size_t)factorizationRow * d_stride);

    /* factorize row 2 */
    dReal q2 = Y21 * dd;
    ptrAElement[rowSkip] = q2;

    dReal sum = Y21 * q2;
    ptrDElement[1 * d_stride] = dRecip(Y22 - sum);
}

template<unsigned int d_stride>
void dxScaleAndFactorizeFirstL1Row_2(dReal *ARow, dReal *d, unsigned rowSkip)
{
    dReal *ptrAElement = ARow;
    dReal *ptrDElement = d;

    /* solve for diagonal 2 x 2 block at A(0,0) */
    dReal Y11 = ptrAElement[0]/* - Z11*/;
    dReal Y21 = ptrAElement[rowSkip]/* - Z21*/;
    dReal Y22 = ptrAElement[1 + rowSkip]/* - Z22*/;

    /* factorize 2 x 2 block Y, ptrDElement */
    /* factorize row 1 */
    dReal dd = dRecip(Y11);

    ptrDElement[0 * d_stride] = dd;
    dIASSERT(ptrDElement == d/* + (size_t)factorizationRow * d_stride*/);

    /* factorize row 2 */
    dReal q2 = Y21 * dd;
    ptrAElement[rowSkip] = q2;

    dReal sum = Y21 * q2;
    ptrDElement[1 * d_stride] = dRecip(Y22 - sum);
}


/* solve L*X=B, with B containing 1 right hand sides.
 * L is an n*n lower triangular matrix with ones on the diagonal.
 * L is stored by rows and its leading dimension is lskip.
 * B is an n*1 matrix that contains the right hand sides.
 * B is stored by columns and its leading dimension is also lskip.
 * B is overwritten with X.
 * this processes blocks of 2*2.
 * if this is in the factorizer source file, n must be a multiple of 2.
 */
static 
void dxSolveL1_1(const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip)
{
    dIASSERT(rowCount != 0);
    dIASSERT(rowCount % 2 == 0);

    /* compute all 2 x 1 blocks of X */
    unsigned blockStartRow = 0;
    for (bool exitLoop = false, subsequentPass = false; !exitLoop; subsequentPass = true, exitLoop = (blockStartRow += 2) == rowCount) 
    {
        const dReal *ptrLElement;
        dReal *ptrBElement;

        /* declare variables - Z matrix */
        dReal Z11, Z21;

        if (subsequentPass)
        {
            ptrLElement = L + (size_t)blockStartRow * rowSkip;
            ptrBElement = B;

            /* set the Z matrix to 0 */
            Z11 = 0; Z21 = 0;

            /* compute all 2 x 1 block of X, from rows i..i+2-1 */
            
            /* the inner loop that computes outer products and adds them to Z */
            // The iteration starts with even number and decreases it by 2. So, it must end in zero
            for (unsigned columnCounter = blockStartRow; ; ) 
            {
                /* declare p and q vectors, etc */
                dReal p1, q1, p2;

                /* compute outer product and add it to the Z matrix */
                p1 = ptrLElement[0];
                q1 = ptrBElement[0];
                Z11 += p1 * q1;
                p2 = ptrLElement[rowSkip];
                Z21 += p2 * q1;
                
                /* compute outer product and add it to the Z matrix */
                p1 = ptrLElement[1];
                q1 = ptrBElement[1];
                Z11 += p1 * q1;
                p2 = ptrLElement[1 + rowSkip];
                Z21 += p2 * q1;

                if (columnCounter > 6)
                {
                    columnCounter -= 6;

                    /* advance pointers */
                    ptrLElement += 6;
                    ptrBElement += 6;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-4];
                    q1 = ptrBElement[-4];
                    Z11 += p1 * q1;
                    p2 = ptrLElement[-4 + rowSkip];
                    Z21 += p2 * q1;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-3];
                    q1 = ptrBElement[-3];
                    Z11 += p1 * q1;
                    p2 = ptrLElement[-3 + rowSkip];
                    Z21 += p2 * q1;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-2];
                    q1 = ptrBElement[-2];
                    Z11 += p1 * q1;
                    p2 = ptrLElement[-2 + rowSkip];
                    Z21 += p2 * q1;

                    /* compute outer product and add it to the Z matrix */
                    p1 = ptrLElement[-1];
                    q1 = ptrBElement[-1];
                    Z11 += p1 * q1;
                    p2 = ptrLElement[-1 + rowSkip];
                    Z21 += p2 * q1;
                }
                else
                {
                    /* advance pointers */
                    ptrLElement += 2;
                    ptrBElement += 2;

                    if ((columnCounter -= 2) == 0)
                    {
                        break;
                    }
                }
                /* end of inner loop */
            }
        }
        else
        {
            ptrLElement = L/* + (size_t)blockStartRow * rowSkip*/; dIASSERT(blockStartRow == 0);
            ptrBElement = B;

            /* set the Z matrix to 0 */
            Z11 = 0; Z21 = 0;
        }
        
        /* finish computing the X(i) block */
        dReal p2 = ptrLElement[rowSkip];

        dReal Y11 = ptrBElement[0] - Z11;
        dReal Y21 = ptrBElement[1] - Z21 - p2 * Y11;

        ptrBElement[0] = Y11;
        ptrBElement[1] = Y21;
        /* end of outer loop */
    }
}

template<unsigned int d_stride>
void dxScaleAndFactorizeL1_1(dReal *ARow, dReal *d, unsigned factorizationRow)
{
    dReal *ptrAElement = ARow;
    dReal *ptrDElement = d;

    /* scale the elements in a 1 x i block at A(i,0), and also */
    /* compute Z = the outer product matrix that we'll need. */
    dReal Z11 = 0, Z22 = 0;

    for (unsigned columnCounter = factorizationRow; ; ) 
    {
        dReal p1, p2, q1, q2, dd1, dd2;

        p1 = ptrAElement[0];
        p2 = ptrAElement[1];
        dd1 = ptrDElement[0 * d_stride];
        dd2 = ptrDElement[1 * d_stride];
        q1 = p1 * dd1;
        q2 = p2 * dd2;
        ptrAElement[0] = q1;
        ptrAElement[1] = q2;
        Z11 += p1 * q1;
        Z22 += p2 * q2;

        if (columnCounter > 6)
        {
            columnCounter -= 6;

            ptrAElement += 6;
            ptrDElement += 6 * d_stride;

            p1 = ptrAElement[-4];
            p2 = ptrAElement[-3];
            dd1 = ptrDElement[-4 * (int)d_stride];
            dd2 = ptrDElement[-3 * (int)d_stride];
            q1 = p1 * dd1;
            q2 = p2 * dd2;
            ptrAElement[-4] = q1;
            ptrAElement[-3] = q2;
            Z11 += p1 * q1;
            Z22 += p2 * q2;

            p1 = ptrAElement[-2];
            p2 = ptrAElement[-1];
            dd1 = ptrDElement[-2 * (int)d_stride];
            dd2 = ptrDElement[-1 * (int)d_stride];
            q1 = p1 * dd1;
            q2 = p2 * dd2;
            ptrAElement[-2] = q1;
            ptrAElement[-1] = q2;
            Z11 += p1 * q1;
            Z22 += p2 * q2;
        }
        else
        {
            ptrAElement += 2;
            ptrDElement += 2 * d_stride;

            if ((columnCounter -= 2) == 0)
            {
                break;
            }
        }
    }

    dReal Y11 = ptrAElement[0] - (Z11 + Z22);

    /* solve for diagonal 1 x 1 block at A(i,i) */
    dIASSERT(ptrDElement == d + (size_t)factorizationRow * d_stride);
    /* factorize 1 x 1 block Y, ptrDElement */
    /* factorize row 1 */
    ptrDElement[0 * d_stride] = dRecip(Y11);
}

template<unsigned int d_stride>
void dxScaleAndFactorizeFirstL1Row_1(dReal *ARow, dReal *d)
{
    dReal *ptrAElement = ARow;
    dReal *ptrDElement = d;

    dReal Y11 = ptrAElement[0];

    /* solve for diagonal 1 x 1 block at A(0,0) */
    /* factorize 1 x 1 block Y, ptrDElement */
    /* factorize row 1 */
    ptrDElement[0 * d_stride] = dRecip(Y11);
}


#endif // #ifndef _ODE_FASTLDLT_IMPL_H_
