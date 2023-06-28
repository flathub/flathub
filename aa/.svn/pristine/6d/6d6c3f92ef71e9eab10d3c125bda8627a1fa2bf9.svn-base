

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


#ifndef _ODE_FASTLTSOLVE_IMPL_H_
#define _ODE_FASTLTSOLVE_IMPL_H_



/* solve L^T * x=b, with b containing 1 right hand side.
 * L is an n*n lower triangular matrix with ones on the diagonal.
 * L is stored by rows and its leading dimension is rowSkip.
 * b is an n*1 matrix that contains the right hand side.
 * b is overwritten with x.
 * this processes blocks of 4.
 */

template<unsigned int b_stride>
void dxtSolveL1T(const dReal *L, dReal *B, unsigned rowCount, unsigned rowSkip)
{
    dIASSERT(rowCount != 0);

    /* special handling for L and B because we're solving L1 *transpose* */
    const dReal *lastLElement = L + (size_t)(rowCount - 1) * (rowSkip + 1);
    dReal *lastBElement = B + (size_t)(rowCount - 1) * b_stride;

    /* compute rows at end that are not a multiple of block size */
    const unsigned loopX1RowCount = rowCount % 4;
    
    unsigned blockStartRow = loopX1RowCount;
    bool subsequentPass  = false;

    /* compute rightmost bottom X(i) block */
    if (loopX1RowCount != 0)
    {
        subsequentPass = true;

        const dReal *ptrLElement = lastLElement;
        dReal *ptrBElement = lastBElement;

        dReal Y11 = ptrBElement[0 * b_stride]/* - Z11*/;
        // ptrBElement[0 * b_stride] = Y11; -- unchanged

        if (loopX1RowCount >= 2)
        {
            dReal p2 = ptrLElement[-1];
            dReal Y21 = ptrBElement[-1 * (int)b_stride]/* - Z21 */- p2 * Y11;
            ptrBElement[-1 * (int)b_stride] = Y21;
            
            if (loopX1RowCount > 2)
            {
                dReal p3 = ptrLElement[-2];
                dReal p3_1 = (ptrLElement - rowSkip)[-2];
                dReal Y31 = ptrBElement[-2 * (int)b_stride]/* - Z31 */- p3 * Y11 - p3_1 * Y21;
                ptrBElement[-2 * (int)b_stride] = Y31;
            }
        }
    }
    
    /* compute all 4 x 1 blocks of X */
    for (; !subsequentPass || blockStartRow < rowCount; subsequentPass = true, blockStartRow += 4)
    {
        /* compute all 4 x 1 block of X, from rows i..i+4-1 */

        /* declare variables - Z matrix, p and q vectors, etc */
        const dReal *ptrLElement;
        dReal *ptrBElement;

        dReal Z41, Z31, Z21, Z11;

        if (subsequentPass)
        {
            ptrLElement = lastLElement - blockStartRow;
            ptrBElement = lastBElement;

            /* set the Z matrix to 0 */
            Z41 = 0; Z31 = 0; Z21 = 0; Z11 = 0;

            unsigned rowCounter = blockStartRow;

            if (rowCounter % 2 != 0)
            {
                dReal q1, p4, p3, p2, p1;

                /* load p and q values */
                q1 = ptrBElement[0 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                ptrBElement -= 1 * b_stride;
                rowCounter -= 1;
            }

            if (rowCounter % 4 != 0)
            {
                dReal q1, p4, p3, p2, p1;

                /* load p and q values */
                q1 = ptrBElement[0 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                /* load p and q values */
                q1 = ptrBElement[-1 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                ptrBElement -= 2 * b_stride;
                rowCounter -= 2;
            }

            /* the inner loop that computes outer products and adds them to Z */
            for (bool exitLoop = rowCounter == 0; !exitLoop; exitLoop = false)
            {
                dReal q1, p4, p3, p2, p1;

                /* load p and q values */
                q1 = ptrBElement[0 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                /* load p and q values */
                q1 = ptrBElement[-1 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                /* load p and q values */
                q1 = ptrBElement[-2 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                /* load p and q values */
                q1 = ptrBElement[-3 * (int)b_stride];
                p4 = ptrLElement[-3];
                p3 = ptrLElement[-2];
                p2 = ptrLElement[-1];
                p1 = ptrLElement[0];
                ptrLElement -= rowSkip;

                /* compute outer product and add it to the Z matrix */
                Z41 += p4 * q1;
                Z31 += p3 * q1;
                Z21 += p2 * q1;
                Z11 += p1 * q1;

                if (rowCounter > 12)
                {
                    rowCounter -= 12;

                    ptrBElement -= 12 * b_stride;

                    /* load p and q values */
                    q1 = ptrBElement[8 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[7 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[6 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[5 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[4 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[3 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[2 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;

                    /* load p and q values */
                    q1 = ptrBElement[1 * b_stride];
                    p4 = ptrLElement[-3];
                    p3 = ptrLElement[-2];
                    p2 = ptrLElement[-1];
                    p1 = ptrLElement[0];
                    ptrLElement -= rowSkip;

                    /* compute outer product and add it to the Z matrix */
                    Z41 += p4 * q1;
                    Z31 += p3 * q1;
                    Z21 += p2 * q1;
                    Z11 += p1 * q1;
                }
                else
                {
                    ptrBElement -= 4 * b_stride;

                    if ((rowCounter -= 4) == 0)
                    {
                        break;
                    }
                }
                /* end of inner loop */
            }
        }
        else
        {
            ptrLElement = lastLElement/* - blockStartRow*/; dIASSERT(blockStartRow == 0);
            ptrBElement = lastBElement;

            /* set the Z matrix to 0 */
            Z41 = 0; Z31 = 0; Z21 = 0; Z11 = 0;
        }

        /* finish computing the X(i) block */
        dReal Y11, Y21, Y31, Y41;
        {
            Y11 = ptrBElement[0 * b_stride] - Z11;
            ptrBElement[0 * b_stride] = Y11;
        }
        {
            dReal p2 = ptrLElement[-1];
            Y21 = ptrBElement[-1 * (int)b_stride] - Z21 - p2 * Y11;
            ptrBElement[-1 * (int)b_stride] = Y21;
        }
        {
            dReal p3 = ptrLElement[-2];
            dReal p3_1 = (ptrLElement - rowSkip)[-2];
            Y31 = ptrBElement[-2 * (int)b_stride] - Z31 - p3 * Y11 - p3_1 * Y21;
            ptrBElement[-2 * (int)b_stride] = Y31;
        }
        {
            dReal p4 = ptrLElement[-3];
            dReal p4_1 = (ptrLElement - rowSkip)[-3];
            dReal p4_2 = (ptrLElement - rowSkip * 2)[-3];
            Y41 = ptrBElement[-3 * (int)b_stride] - Z41 - p4 * Y11 - p4_1 * Y21 - p4_2 * Y31;
            ptrBElement[-3 * (int)b_stride] = Y41;
        }
        /* end of outer loop */
    }
}


#endif
