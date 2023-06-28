

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


#ifndef _ODE_FASTSOLVE_IMPL_H_
#define _ODE_FASTSOLVE_IMPL_H_


/* solve L*X=B, with B containing 1 right hand sides.
 * L is an n*n lower triangular matrix with ones on the diagonal.
 * L is stored by rows and its leading dimension is lskip.
 * B is an n*1 matrix that contains the right hand sides.
 * B is stored by columns and its leading dimension is also lskip.
 * B is overwritten with X.
 * this processes blocks of 4*4.
 * if this is in the factorizer source file, n must be a multiple of 4.
 */

template<unsigned int b_stride>
void dxtSolveL1 (const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip)
{
    dIASSERT(rowCount != 0);

    /* compute all 4 x 1 blocks of X */
    unsigned blockStartRow = 0;
    bool subsequentPass = false;
    bool goForLoopX4 = rowCount >= 4;
    const unsigned loopX4LastRow = goForLoopX4 ? rowCount - 4 : 0;
    for (; goForLoopX4; subsequentPass = true, goForLoopX4 = (blockStartRow += 4) <= loopX4LastRow) 
    {
        /* declare variables - Z matrix, p and q vectors, etc */
        const dReal *ptrLElement;
        dReal *ptrBElement;

        dReal Z11, Z21, Z31, Z41;

        /* compute all 4 x 1 block of X, from rows i..i+4-1 */
        if (subsequentPass)
        {
            ptrLElement = L + rowSkip + blockStartRow * rowSkip;
            ptrBElement = B;
            /* set the Z matrix to 0 */
            Z11 = 0; Z21 = 0; Z31 = 0; Z41 = 0;

            /* the inner loop that computes outer products and adds them to Z */
            for (unsigned columnCounter = blockStartRow; ; )
            {
                dReal q1, p1, p2, p3, p4;

                /* load p and q values */
                q1 = ptrBElement[0 * b_stride];
                p1 = (ptrLElement - rowSkip)[0];
                p2 = ptrLElement[0];
                ptrLElement += rowSkip;
                p3 = ptrLElement[0];
                p4 = ptrLElement[0 + rowSkip];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z21 += p2 * q1;
                Z31 += p3 * q1;
                Z41 += p4 * q1;

                /* load p and q values */
                q1 = ptrBElement[1 * b_stride];
                p3 = ptrLElement[1];
                p4 = ptrLElement[1 + rowSkip];
                ptrLElement -= rowSkip;
                p1 = (ptrLElement - rowSkip)[1];
                p2 = ptrLElement[1];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z21 += p2 * q1;
                Z31 += p3 * q1;
                Z41 += p4 * q1;

                /* load p and q values */
                q1 = ptrBElement[2 * b_stride];
                p1 = (ptrLElement - rowSkip)[2];
                p2 = ptrLElement[2];
                ptrLElement += rowSkip;
                p3 = ptrLElement[2];
                p4 = ptrLElement[2 + rowSkip];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z21 += p2 * q1;
                Z31 += p3 * q1;
                Z41 += p4 * q1;

                /* load p and q values */
                q1 = ptrBElement[3 * b_stride];
                p3 = ptrLElement[3];
                p4 = ptrLElement[3 + rowSkip];
                ptrLElement -= rowSkip;
                p1 = (ptrLElement - rowSkip)[3];
                p2 = ptrLElement[3];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z21 += p2 * q1;
                Z31 += p3 * q1;
                Z41 += p4 * q1;

                if (columnCounter > 12)
                {
                    columnCounter -= 12;

                    /* advance pointers */
                    ptrLElement += 12;
                    ptrBElement += 12 * b_stride;

                    /* load p and q values */
                    q1 = ptrBElement[-8 * (int)b_stride];
                    p1 = (ptrLElement - rowSkip)[-8];
                    p2 = ptrLElement[-8];
                    ptrLElement += rowSkip;
                    p3 = ptrLElement[-8];
                    p4 = ptrLElement[-8 + rowSkip];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-7 * (int)b_stride];
                    p3 = ptrLElement[-7];
                    p4 = ptrLElement[-7 + rowSkip];
                    ptrLElement -= rowSkip;
                    p1 = (ptrLElement - rowSkip)[-7];
                    p2 = ptrLElement[-7];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-6 * (int)b_stride];
                    p1 = (ptrLElement - rowSkip)[-6];
                    p2 = ptrLElement[-6];
                    ptrLElement += rowSkip;
                    p3 = ptrLElement[-6];
                    p4 = ptrLElement[-6 + rowSkip];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-5 * (int)b_stride];
                    p3 = ptrLElement[-5];
                    p4 = ptrLElement[-5 + rowSkip];
                    ptrLElement -= rowSkip;
                    p1 = (ptrLElement - rowSkip)[-5];
                    p2 = ptrLElement[-5];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-4 * (int)b_stride];
                    p1 = (ptrLElement - rowSkip)[-4];
                    p2 = ptrLElement[-4];
                    ptrLElement += rowSkip;
                    p3 = ptrLElement[-4];
                    p4 = ptrLElement[-4 + rowSkip];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-3 * (int)b_stride];
                    p3 = ptrLElement[-3];
                    p4 = ptrLElement[-3 + rowSkip];
                    ptrLElement -= rowSkip;
                    p1 = (ptrLElement - rowSkip)[-3];
                    p2 = ptrLElement[-3];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-2 * (int)b_stride];
                    p1 = (ptrLElement - rowSkip)[-2];
                    p2 = ptrLElement[-2];
                    ptrLElement += rowSkip;
                    p3 = ptrLElement[-2];
                    p4 = ptrLElement[-2 + rowSkip];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[-1 * (int)b_stride];
                    p3 = ptrLElement[-1];
                    p4 = ptrLElement[-1 + rowSkip];
                    ptrLElement -= rowSkip;
                    p1 = (ptrLElement - rowSkip)[-1];
                    p2 = ptrLElement[-1];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z21 += p2 * q1;
                    Z31 += p3 * q1;
                    Z41 += p4 * q1;
                }
                else
                {
                    /* advance pointers */
                    ptrLElement += 4;
                    ptrBElement += 4 * b_stride;

                    if ((columnCounter -= 4) == 0)
                    {
                        break;
                    }
                }
                /* end of inner loop */
            }
        }
        else
        {
            ptrLElement = L + rowSkip/* + blockStartRow * rowSkip*/; dIASSERT(blockStartRow == 0);
            ptrBElement = B;
            /* set the Z matrix to 0 */
            Z11 = 0; Z21 = 0; Z31 = 0; Z41 = 0;
        }

        /* finish computing the X(i) block */
        dReal Y11, Y21, Y31, Y41;
        {
            Y11 = ptrBElement[0 * b_stride] - Z11;
            ptrBElement[0 * b_stride] = Y11;
        }
        {
            dReal p2 = ptrLElement[0];
            Y21 = ptrBElement[1 * b_stride] - Z21 - p2 * Y11;
            ptrBElement[1 * b_stride] = Y21;
        }
        {
            ptrLElement += rowSkip;
            dReal p3 = ptrLElement[0];
            dReal p3_1 = ptrLElement[1];
            Y31 = ptrBElement[2 * b_stride] - Z31 - p3 * Y11 - p3_1 * Y21;
            ptrBElement[2 * b_stride] = Y31;
        }
        {
            dReal p4 = ptrLElement[rowSkip];
            dReal p4_1 = ptrLElement[1 + rowSkip];
            dReal p4_2 = ptrLElement[2 + rowSkip];
            Y41 = ptrBElement[3 * b_stride] - Z41 - p4 * Y11 - p4_1 * Y21 - p4_2 * Y31;
            ptrBElement[3 * b_stride] = Y41;
        }
        /* end of outer loop */
    }

    /* compute rows at end that are not a multiple of block size */
    for (; !subsequentPass || blockStartRow < rowCount; subsequentPass = true, ++blockStartRow) 
    {
        /* compute all 1 x 1 block of X, from rows i..i+1-1 */
        dReal *ptrBElement;

        dReal Z11, Z12;

        if (subsequentPass)
        {
            ptrBElement = B;
            /* set the Z matrix to 0 */
            Z11 = 0; Z12 = 0;

            const dReal *ptrLElement = L + blockStartRow * rowSkip;

            /* the inner loop that computes outer products and adds them to Z */
            unsigned columnCounter = blockStartRow;
            for (bool exitLoop = columnCounter < 4; !exitLoop; exitLoop = false) 
            {
                dReal p1, p2, q1, q2;

                /* load p and q values */
                p1 = ptrLElement[0];
                p2 = ptrLElement[1];
                q1 = ptrBElement[0 * b_stride];
                q2 = ptrBElement[1 * b_stride];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z12 += p2 * q2;

                /* load p and q values */
                p1 = ptrLElement[2];
                p2 = ptrLElement[3];
                q1 = ptrBElement[2 * b_stride];
                q2 = ptrBElement[3 * b_stride];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z12 += p2 * q2;

                if (columnCounter >= (12 + 4))
                {
                    columnCounter -= 12;

                    /* advance pointers */
                    ptrLElement += 12;
                    ptrBElement += 12 * b_stride;

                    /* load p and q values */
                    p1 = ptrLElement[-8];
                    p2 = ptrLElement[-7];
                    q1 = ptrBElement[-8 * (int)b_stride];
                    q2 = ptrBElement[-7 * (int)b_stride];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z12 += p2 * q2;

                    /* load p and q values */
                    p1 = ptrLElement[-6];
                    p2 = ptrLElement[-5];
                    q1 = ptrBElement[-6 * (int)b_stride];
                    q2 = ptrBElement[-5 * (int)b_stride];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z12 += p2 * q2;

                    /* load p and q values */
                    p1 = ptrLElement[-4];
                    p2 = ptrLElement[-3];
                    q1 = ptrBElement[-4 * (int)b_stride];
                    q2 = ptrBElement[-3 * (int)b_stride];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z12 += p2 * q2;

                    /* load p and q values */
                    p1 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    q1 = ptrBElement[-2 * (int)b_stride];
                    q2 = ptrBElement[-1 * (int)b_stride];

                    /* compute outer product and add it to the Z matrix */
                    Z11 += p1 * q1;
                    Z12 += p2 * q2;
                }
                else
                {
                    /* advance pointers */
                    ptrLElement += 4;
                    ptrBElement += 4 * b_stride;

                    if ((columnCounter -= 4) < 4)
                    {
                        break;
                    }
                }
                /* end of inner loop */
            }

            /* compute left-over iterations */
            if ((columnCounter & 2) != 0) 
            {
                dReal p1, p2, q1, q2;

                /* load p and q values */
                p1 = ptrLElement[0];
                p2 = ptrLElement[1];
                q1 = ptrBElement[0 * b_stride];
                q2 = ptrBElement[1 * b_stride];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;
                Z12 += p2 * q2;

                /* advance pointers */
                ptrLElement += 2;
                ptrBElement += 2 * b_stride;
            }

            if ((columnCounter & 1) != 0)
            {
                dReal p1, q1;

                /* load p and q values */
                p1 = ptrLElement[0];
                q1 = ptrBElement[0 * b_stride];

                /* compute outer product and add it to the Z matrix */
                Z11 += p1 * q1;

                /* advance pointers */
                // ptrLElement += 1; -- not needed any more
                ptrBElement += 1 * b_stride;
            }

            /* finish computing the X(i) block */
            dReal Y11 = ptrBElement[0 * b_stride] - (Z11 + Z12);
            ptrBElement[0 * b_stride] = Y11;
        }
    }
}


#endif
