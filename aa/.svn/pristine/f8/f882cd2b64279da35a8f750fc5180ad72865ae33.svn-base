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

/*


THE ALGORITHM
-------------

solve A*x = b+w, with x and w subject to certain LCP conditions.
each x(i),w(i) must lie on one of the three line segments in the following
diagram. each line segment corresponds to one index set :

     w(i)
     /|\      |           :
      |       |           :
      |       |i in N     :
  w>0 |       |state[i]=0 :
      |       |           :
      |       |           :  i in C
  w=0 +       +-----------------------+
      |                   :           |
      |                   :           |
  w<0 |                   :           |i in N
      |                   :           |state[i]=1
      |                   :           |
      |                   :           |
      +-------|-----------|-----------|----------> x(i)
             lo           0           hi

the Dantzig algorithm proceeds as follows:
  for i=1:n
    * if (x(i),w(i)) is not on the line, push x(i) and w(i) positive or
      negative towards the line. as this is done, the other (x(j),w(j))
      for j<i are constrained to be on the line. if any (x,w) reaches the
      end of a line segment then it is switched between index sets.
    * i is added to the appropriate index set depending on what line segment
      it hits.

we restrict lo(i) <= 0 and hi(i) >= 0. this makes the algorithm a bit
simpler, because the starting point for x(i),w(i) is always on the dotted
line x=0 and x will only ever increase in one direction, so it can only hit
two out of the three line segments.


NOTES
-----

this is an implementation of "lcp_dantzig2_ldlt.m" and "lcp_dantzig_lohi.m".
the implementation is split into an LCP problem object (dLCP) and an LCP
driver function. most optimization occurs in the dLCP object.

a naive implementation of the algorithm requires either a lot of data motion
or a lot of permutation-array lookup, because we are constantly re-ordering
rows and columns. to avoid this and make a more optimized algorithm, a
non-trivial data structure is used to represent the matrix A (this is
implemented in the fast version of the dLCP object).

during execution of this algorithm, some indexes in A are clamped (set C),
some are non-clamped (set N), and some are "don't care" (where x=0).
A,x,b,w (and other problem vectors) are permuted such that the clamped
indexes are first, the unclamped indexes are next, and the don't-care
indexes are last. this permutation is recorded in the array `p'.
initially p = 0..n-1, and as the rows and columns of A,x,b,w are swapped,
the corresponding elements of p are swapped.

because the C and N elements are grouped together in the rows of A, we can do
lots of work with a fast dot product function. if A,x,etc were not permuted
and we only had a permutation array, then those dot products would be much
slower as we would have a permutation array lookup in some inner loops.

A is accessed through an array of row pointers, so that element (i,j) of the
permuted matrix is A[i][j]. this makes row swapping fast. for column swapping
we still have to actually move the data.

during execution of this algorithm we maintain an L*D*L' factorization of
the clamped submatrix of A (call it `AC') which is the top left nC*nC
submatrix of A. there are two ways we could arrange the rows/columns in AC.

(1) AC is always permuted such that L*D*L' = AC. this causes a problem
when a row/column is removed from C, because then all the rows/columns of A
between the deleted index and the end of C need to be rotated downward.
this results in a lot of data motion and slows things down.
(2) L*D*L' is actually a factorization of a *permutation* of AC (which is
itself a permutation of the underlying A). this is what we do - the
permutation is recorded in the vector C. call this permutation A[C,C].
when a row/column is removed from C, all we have to do is swap two
rows/columns and manipulate C.

*/

#include <ode/common.h>
#include <ode/misc.h>
#include <ode/timer.h>		// for testing
#include "config.h"
#include "matrix.h"
#include "lcp.h"
#include "mat.h"		// for testing
#include "util.h"

#include "fastdot_impl.h"
#include "matrix_impl.h"
#include "fastldlt_impl.h"


//***************************************************************************
// code generation parameters

// LCP debugging (mostly for fast dLCP) - this slows things down a lot
//#define DEBUG_LCP

#define dLCP_FAST		// use fast dLCP object

#define NUB_OPTIMIZATIONS // use NUB optimizations


// option 1 : matrix row pointers (less data copying)
#define ROWPTRS
#define ATYPE dReal **
#define AROW(i) (m_A[i])

// option 2 : no matrix row pointers (slightly faster inner loops)
//#define NOROWPTRS
//#define ATYPE dReal *
//#define AROW(i) (m_A+(i)*m_nskip)


//***************************************************************************

#define dMIN(A,B)  ((A)>(B) ? (B) : (A))
#define dMAX(A,B)  ((B)>(A) ? (B) : (A))


#define LMATRIX_ALIGNMENT       dMAX(64, EFFICIENT_ALIGNMENT)

//***************************************************************************


// transfer b-values to x-values
template<bool zero_b>
inline 
void transfer_b_to_x(dReal pairsbx[PBX__MAX], unsigned n)
{
    dReal *const endbx = pairsbx + (size_t)n * PBX__MAX;
    for (dReal *currbx = pairsbx; currbx != endbx; currbx += PBX__MAX) {
        currbx[PBX_X] = currbx[PBX_B];
        if (zero_b) {
            currbx[PBX_B] = REAL(0.0);
        }
    }
}

// swap row/column i1 with i2 in the n*n matrix A. the leading dimension of
// A is nskip. this only references and swaps the lower triangle.
// if `do_fast_row_swaps' is nonzero and row pointers are being used, then
// rows will be swapped by exchanging row pointers. otherwise the data will
// be copied.

static 
void swapRowsAndCols (ATYPE A, unsigned n, unsigned i1, unsigned i2, unsigned nskip, 
                             int do_fast_row_swaps)
{
    dAASSERT (A && n > 0 && i1 >= 0 && i2 >= 0 && i1 < n && i2 < n &&
        nskip >= n && i1 < i2);

# ifdef ROWPTRS
    dReal *A_i1 = A[i1];
    dReal *A_i2 = A[i2];
    for (unsigned i=i1+1; i<i2; ++i) {
        dReal *A_i_i1 = A[i] + i1;
        A_i1[i] = *A_i_i1;
        *A_i_i1 = A_i2[i];
    }
    A_i1[i2] = A_i1[i1];
    A_i1[i1] = A_i2[i1];
    A_i2[i1] = A_i2[i2];
    // swap rows, by swapping row pointers
    if (do_fast_row_swaps) {
        A[i1] = A_i2;
        A[i2] = A_i1;
    }
    else {
        // Only swap till i2 column to match A plain storage variant.
        for (unsigned k = 0; k <= i2; ++k) {
            dxSwap(A_i1[k], A_i2[k]);
        }
    }
    // swap columns the hard way
    for (unsigned j = i2 + 1; j < n; ++j) {
        dReal *A_j = A[j];
        dxSwap(A_j[i1], A_j[i2]);
    }
# else
    dReal *A_i1 = A + (size_t)nskip * i1;
    dReal *A_i2 = A + (size_t)nskip * i2;

    for (unsigned k = 0; k < i1; ++k) {
        dxSwap(A_i1[k], A_i2[k]);
    }

    dReal *A_i = A_i1 + nskip;
    for (unsigned i= i1 + 1; i < i2; A_i += nskip, ++i) {
        dxSwap(A_i2[i], A_i[i1]);
    }

    dxSwap(A_i1[i1], A_i2[i2]);

    dReal *A_j = A_i2 + nskip;
    for (unsigned j = i2 + 1; j < n; A_j += nskip, ++j) {
        dxSwap(A_j[i1], A_j[i2]);
    }
# endif
}


// swap two indexes in the n*n LCP problem. i1 must be <= i2.

static 
void swapProblem (ATYPE A, dReal pairsbx[PBX__MAX], dReal *w, dReal pairslh[PLH__MAX],
                         unsigned *p, bool *state, int *findex,
                         unsigned n, unsigned i1, unsigned i2, unsigned nskip,
                         int do_fast_row_swaps)
{
    dIASSERT (n>0 && i1 < n && i2 < n && nskip >= n && i1 <= i2);
    
    if (i1 != i2) {
        swapRowsAndCols (A, n, i1, i2, nskip, do_fast_row_swaps);

        dxSwap((pairsbx + (size_t)i1 * PBX__MAX)[PBX_B], (pairsbx + (size_t)i2 * PBX__MAX)[PBX_B]);
        dxSwap((pairsbx + (size_t)i1 * PBX__MAX)[PBX_X], (pairsbx + (size_t)i2 * PBX__MAX)[PBX_X]);
        dSASSERT(PBX__MAX == 2);

        dxSwap(w[i1], w[i2]);

        dxSwap((pairslh + (size_t)i1 * PLH__MAX)[PLH_LO], (pairslh + (size_t)i2 * PLH__MAX)[PLH_LO]);
        dxSwap((pairslh + (size_t)i1 * PLH__MAX)[PLH_HI], (pairslh + (size_t)i2 * PLH__MAX)[PLH_HI]);
        dSASSERT(PLH__MAX == 2);

        dxSwap(p[i1], p[i2]);
        dxSwap(state[i1], state[i2]);

        if (findex != NULL) {
            dxSwap(findex[i1], findex[i2]);
        }
    }
}


// for debugging - check that L,d is the factorization of A[C,C].
// A[C,C] has size nC*nC and leading dimension nskip.
// L has size nC*nC and leading dimension nskip.
// d has size nC.

#ifdef DEBUG_LCP

static 
void checkFactorization (ATYPE A, dReal *_L, dReal *_d,
                                unsigned nC, unsigned *C, unsigned nskip)
{
    unsigned i, j;
    if (nC == 0) return;

    // get A1=A, copy the lower triangle to the upper triangle, get A2=A[C,C]
    dMatrix A1 (nC, nC);
    for (i=0; i < nC; i++) {
        for (j = 0; j <= i; j++) A1(i, j) = A1(j, i) = AROW(i)[j];
    }
    dMatrix A2 = A1.select (nC, C, nC, C);

    // printf ("A1=\n"); A1.print(); printf ("\n");
    // printf ("A2=\n"); A2.print(); printf ("\n");

    // compute A3 = L*D*L'
    dMatrix L (nC, nC, _L, nskip, 1);
    dMatrix D (nC, nC);
    for (i = 0; i < nC; i++) D(i, i) = 1.0 / _d[i];
    L.clearUpperTriangle();
    for (i = 0; i < nC; i++) L(i, i) = 1;
    dMatrix A3 = L * D * L.transpose();

    // printf ("L=\n"); L.print(); printf ("\n");
    // printf ("D=\n"); D.print(); printf ("\n");
    // printf ("A3=\n"); A2.print(); printf ("\n");

    // compare A2 and A3
    dReal diff = A2.maxDifference (A3);
    if (diff > 1e-8)
        dDebug (0, "L*D*L' check, maximum difference = %.6e\n", diff);
}

#endif


// for debugging

#ifdef DEBUG_LCP

static 
void checkPermutations (unsigned i, unsigned n, unsigned nC, unsigned nN, unsigned *p, unsigned *C)
{
    unsigned j,k;
    dIASSERT (/*nC >= 0 && nN >= 0 && */(nC + nN) == i && i < n);
    for (k=0; k<i; k++) dIASSERT (p[k] >= 0 && p[k] < i);
    for (k=i; k<n; k++) dIASSERT (p[k] == k);
    for (j=0; j<nC; j++) {
        int C_is_bad = 1;
        for (k=0; k<nC; k++) if (C[k]==j) C_is_bad = 0;
        dIASSERT (C_is_bad==0);
    }
}

#endif

//***************************************************************************
// dLCP manipulator object. this represents an n*n LCP problem.
//
// two index sets C and N are kept. each set holds a subset of
// the variable indexes 0..n-1. an index can only be in one set.
// initially both sets are empty.
//
// the index set C is special: solutions to A(C,C)\A(C,i) can be generated.

//***************************************************************************
// fast implementation of dLCP. see the above definition of dLCP for
// interface comments.
//
// `p' records the permutation of A,x,b,w,etc. p is initially 1:n and is
// permuted as the other vectors/matrices are permuted.
//
// A,x,b,w,lo,hi,state,findex,p,c are permuted such that sets C,N have
// contiguous indexes. the don't-care indexes follow N.
//
// an L*D*L' factorization is maintained of A(C,C), and whenever indexes are
// added or removed from the set C the factorization is updated.
// thus L*D*L'=A[C,C], i.e. a permuted top left nC*nC submatrix of A.
// the leading dimension of the matrix L is always `nskip'.
//
// at the start there may be other indexes that are unbounded but are not
// included in `nub'. dLCP will permute the matrix so that absolutely all
// unbounded vectors are at the start. thus there may be some initial
// permutation.
//
// the algorithms here assume certain patterns, particularly with respect to
// index transfer.

#ifdef dLCP_FAST

struct dLCP {
    const unsigned m_n;
    const unsigned m_nskip;
    unsigned m_nub;
    unsigned m_nC, m_nN;				// size of each index set
    ATYPE const m_A;				// A rows
    dReal *const m_pairsbx, *const m_w, *const m_pairslh;	// permuted LCP problem data
    dReal *const m_L, *const m_d;				// L*D*L' factorization of set C
    dReal *const m_Dell, *const m_ell, *const m_tmp;
    bool *const m_state;
    int *const m_findex;
    unsigned *const m_p, *const m_C;

    dLCP (unsigned _n, unsigned _nskip, unsigned _nub, dReal *_Adata, dReal *_pairsbx, dReal *_w,
        dReal *_pairslh, dReal *_L, dReal *_d,
        dReal *_Dell, dReal *_ell, dReal *_tmp,
        bool *_state, int *_findex, unsigned *_p, unsigned *_C, dReal **Arows);
    unsigned getNub() const { return m_nub; }
    void transfer_i_to_C (unsigned i);
    void transfer_i_to_N (unsigned /*i*/) { m_nN++; }			// because we can assume C and N span 1:i-1
    void transfer_i_from_N_to_C (unsigned i);
    void transfer_i_from_C_to_N (unsigned i, void *tmpbuf);
    static size_t estimate_transfer_i_from_C_to_N_mem_req(unsigned nC, unsigned nskip) { return dEstimateLDLTRemoveTmpbufSize(nC, nskip); }
    unsigned numC() const { return m_nC; }
    unsigned numN() const { return m_nN; }
    unsigned indexC (unsigned i) const { return i; }
    unsigned indexN (unsigned i) const { return i+m_nC; }
    dReal Aii (unsigned i) const  { return AROW(i)[i]; }
    template<unsigned q_stride>
    dReal AiC_times_qC (unsigned i, dReal *q) const { return dxtDot<q_stride> (AROW(i), q, m_nC); }
    template<unsigned q_stride>
    dReal AiN_times_qN (unsigned i, dReal *q) const { return dxtDot<q_stride> (AROW(i) + m_nC, q + (size_t)m_nC * q_stride, m_nN); }
    void pN_equals_ANC_times_qC (dReal *p, dReal *q);
    void pN_plusequals_ANi (dReal *p, unsigned i, bool dir_positive);
    template<unsigned p_stride>
    void pC_plusequals_s_times_qC (dReal *p, dReal s, dReal *q);
    void pN_plusequals_s_times_qN (dReal *p, dReal s, dReal *q);
    void solve1 (dReal *a, unsigned i, bool dir_positive, int only_transfer=0);
    void unpermute_X();
    void unpermute_W();
};


dLCP::dLCP (unsigned _n, unsigned _nskip, unsigned _nub, dReal *_Adata, dReal *_pairsbx, dReal *_w,
            dReal *_pairslh, dReal *_L, dReal *_d,
            dReal *_Dell, dReal *_ell, dReal *_tmp,
            bool *_state, int *_findex, unsigned *_p, unsigned *_C, dReal **Arows):
    m_n(_n), m_nskip(_nskip), m_nub(_nub), m_nC(0), m_nN(0),
# ifdef ROWPTRS
    m_A(Arows),
#else
    m_A(_Adata),
#endif
    m_pairsbx(_pairsbx), m_w(_w), m_pairslh(_pairslh), 
    m_L(_L), m_d(_d), m_Dell(_Dell), m_ell(_ell), m_tmp(_tmp),
    m_state(_state), m_findex(_findex), m_p(_p), m_C(_C)
{
    dxtSetZero<PBX__MAX>(m_pairsbx + PBX_X, m_n);

    {
# ifdef ROWPTRS
        // make matrix row pointers
        dReal *aptr = _Adata;
        ATYPE A = m_A;
        const unsigned n = m_n, nskip = m_nskip;
        for (unsigned k=0; k<n; aptr+=nskip, ++k) A[k] = aptr;
# endif
    }

    {
        unsigned *p = m_p;
        const unsigned n = m_n;
        for (unsigned k=0; k != n; ++k) p[k] = k;		// initially unpermutted
    }

    /*
    // for testing, we can do some random swaps in the area i > nub
    {
    const unsigned n = m_n;
    const unsigned nub = m_nub;
    if (nub < n) {
    for (unsigned k=0; k<100; k++) {
    unsigned i1,i2;
    do {
    i1 = dRandInt(n-nub)+nub;
    i2 = dRandInt(n-nub)+nub;
    }
    while (i1 > i2); 
    //printf ("--> %d %d\n",i1,i2);
    swapProblem (m_A, m_pairsbx, m_w, m_pairslh, m_p, m_state, m_findex, n, i1, i2, m_nskip, 0);
    }
    }
    */

    // permute the problem so that *all* the unbounded variables are at the
    // start, i.e. look for unbounded variables not included in `nub'. we can
    // potentially push up `nub' this way and get a bigger initial factorization.
    // note that when we swap rows/cols here we must not just swap row pointers,
    // as the initial factorization relies on the data being all in one chunk.
    // variables that have findex >= 0 are *not* considered to be unbounded even
    // if lo=-inf and hi=inf - this is because these limits may change during the
    // solution process.

    {
        int *findex = m_findex;
        dReal *pairslh = m_pairslh;
        const unsigned n = m_n;
        for (unsigned k = m_nub; k < n; ++k) {
            if (findex && findex[k] >= 0) continue;
            if ((pairslh + (size_t)k * PLH__MAX)[PLH_LO] == -dInfinity && (pairslh + (size_t)k * PLH__MAX)[PLH_HI] == dInfinity) {
                swapProblem (m_A, m_pairsbx, m_w, pairslh, m_p, m_state, findex, n, m_nub, k, m_nskip, 0);
                m_nub++;
            }
        }
    }

    // if there are unbounded variables at the start, factorize A up to that
    // point and solve for x. this puts all indexes 0..nub-1 into C.
    if (m_nub > 0) {
        const unsigned nub = m_nub;
        {
            dReal *Lrow = m_L;
            const unsigned nskip = m_nskip;
            for (unsigned j = 0; j < nub; Lrow += nskip, ++j) memcpy(Lrow, AROW(j), (j + 1) * sizeof(dReal));
        }
        transfer_b_to_x<false> (m_pairsbx, nub);
        dxtFactorLDLT<1> (m_L, m_d, nub, m_nskip);
        dxtSolveLDLT<1, PBX__MAX> (m_L, m_d, m_pairsbx + PBX_X, nub, m_nskip);
        dSetZero (m_w, nub);
        {
            unsigned *C = m_C;
            for (unsigned k = 0; k < nub; ++k) C[k] = k;
        }
        m_nC = nub;
    }

    // permute the indexes > nub such that all findex variables are at the end
    if (m_findex) {
        const unsigned nub = m_nub;
        int *findex = m_findex;
        unsigned num_at_end = 0;
        for (unsigned k = m_n; k > nub; ) {
            --k;
            if (findex[k] >= 0) {
                swapProblem (m_A, m_pairsbx, m_w, m_pairslh, m_p, m_state, findex, m_n, k, m_n - 1 - num_at_end, m_nskip, 1);
                num_at_end++;
            }
        }
    }

    // print info about indexes
    /*
    {
    const unsigned n = m_n;
    const unsigned nub = m_nub;
    for (unsigned k=0; k<n; k++) {
    if (k<nub) printf ("C");
    else if ((m_pairslh + (size_t)k * PLH__MAX)[PLH_LO] == -dInfinity && (m_pairslh + (size_t)k * PLH__MAX)[PLH_HI] == dInfinity) printf ("c");
    else printf (".");
    }
    printf ("\n");
    }
    */
}


void dLCP::transfer_i_to_C (unsigned i)
{
    {
        const unsigned nC = m_nC;

        if (nC > 0) {
            // ell,Dell were computed by solve1(). note, ell = D \ L1solve (L,A(i,C))
            dReal *const Ltgt = m_L + (size_t)m_nskip * nC, *ell = m_ell;
            memcpy(Ltgt, ell, nC * sizeof(dReal));

            dReal ell_Dell_dot = dxDot(m_ell, m_Dell, nC);
            dReal AROW_i_i = AROW(i)[i] != ell_Dell_dot ? AROW(i)[i] : dNextAfter(AROW(i)[i], dInfinity); // A hack to avoid getting a zero in the denominator
            m_d[nC] = dRecip (AROW_i_i - ell_Dell_dot);
        }
        else {
            m_d[0] = dRecip (AROW(i)[i]);
        }

        swapProblem (m_A, m_pairsbx, m_w, m_pairslh, m_p, m_state, m_findex, m_n, nC, i, m_nskip, 1);

        m_C[nC] = nC;
        m_nC = nC + 1; // nC value is outdated after this line
    }

# ifdef DEBUG_LCP
    checkFactorization (m_A, m_L, m_d, m_nC, m_C, m_nskip);
    if (i < (m_n-1)) checkPermutations (i+1, m_n, m_nC, m_nN, m_p, m_C);
# endif
}


void dLCP::transfer_i_from_N_to_C (unsigned i)
{
    {
        const unsigned nC = m_nC;
        if (nC > 0) {
            {
                dReal *const aptr = AROW(i);
                dReal *Dell = m_Dell;
                const unsigned *C = m_C;
#   ifdef NUB_OPTIMIZATIONS
                // if nub>0, initial part of aptr unpermuted
                const unsigned nub = m_nub;
                unsigned j=0;
                for ( ; j<nub; ++j) Dell[j] = aptr[j];
                for ( ; j<nC; ++j) Dell[j] = aptr[C[j]];
#   else
                for (unsigned j=0; j<nC; ++j) Dell[j] = aptr[C[j]];
#   endif
            }
            dxSolveL1 (m_L, m_Dell, nC, m_nskip);

            dReal ell_Dell_dot = REAL(0.0);
            dReal *const Ltgt = m_L + (size_t)m_nskip * nC;
            dReal *ell = m_ell, *Dell = m_Dell, *d = m_d;
            for (unsigned j = 0; j < nC; ++j) {
                dReal ell_j, Dell_j = Dell[j];
                Ltgt[j] = ell[j] = ell_j = Dell_j * d[j];
                ell_Dell_dot += ell_j * Dell_j;
            }
            
            dReal AROW_i_i = AROW(i)[i] != ell_Dell_dot ? AROW(i)[i] : dNextAfter(AROW(i)[i], dInfinity); // A hack to avoid getting a zero in the denominator
            m_d[nC] = dRecip (AROW_i_i - ell_Dell_dot);
        }
        else {
            m_d[0] = dRecip (AROW(i)[i]);
        }

        swapProblem (m_A, m_pairsbx, m_w, m_pairslh, m_p, m_state, m_findex, m_n, nC, i, m_nskip, 1);

        m_C[nC] = nC;
        m_nN--;
        m_nC = nC + 1; // nC value is outdated after this line
    }

    // @@@ TO DO LATER
    // if we just finish here then we'll go back and re-solve for
    // delta_x. but actually we can be more efficient and incrementally
    // update delta_x here. but if we do this, we wont have ell and Dell
    // to use in updating the factorization later.

# ifdef DEBUG_LCP
    checkFactorization (m_A,m_L,m_d,m_nC,m_C,m_nskip);
# endif
}


void dLCP::transfer_i_from_C_to_N (unsigned i, void *tmpbuf)
{
    {
        unsigned *C = m_C;
        // remove a row/column from the factorization, and adjust the
        // indexes (black magic!)
        int last_idx = -1;
        const unsigned nC = m_nC;
        unsigned j = 0;
        for ( ; j < nC; ++j) {
            if (C[j] == nC - 1) {
                last_idx = j;
            }
            if (C[j] == i) {
                dxLDLTRemove (m_A, C, m_L, m_d, m_n, nC, j, m_nskip, tmpbuf);
                unsigned k;
                if (last_idx == -1) {
                    for (k = j + 1 ; k < nC; ++k) {
                        if (C[k] == nC - 1) {
                            break;
                        }
                    }
                    dIASSERT (k < nC);
                }
                else {
                    k = last_idx;
                }
                C[k] = C[j];
                if (j != (nC - 1)) memmove (C + j, C + j + 1, (nC - j - 1) * sizeof(C[0]));
                break;
            }
        }
        dIASSERT (j < nC);

        swapProblem (m_A, m_pairsbx, m_w, m_pairslh, m_p, m_state, m_findex, m_n, i, nC - 1, m_nskip, 1);

        m_nN++;
        m_nC = nC - 1; // nC value is outdated after this line
    }

# ifdef DEBUG_LCP
    checkFactorization (m_A, m_L, m_d, m_nC, m_C, m_nskip);
# endif
}


void dLCP::pN_equals_ANC_times_qC (dReal *p, dReal *q)
{
    // we could try to make this matrix-vector multiplication faster using
    // outer product matrix tricks, e.g. with the dMultidotX() functions.
    // but i tried it and it actually made things slower on random 100x100
    // problems because of the overhead involved. so we'll stick with the
    // simple method for now.
    const unsigned nC = m_nC;
    dReal *ptgt = p + nC;
    const unsigned nN = m_nN;
    for (unsigned i = 0; i < nN; ++i) {
        ptgt[i] = dxDot (AROW(i + nC), q, nC);
    }
}


void dLCP::pN_plusequals_ANi (dReal *p, unsigned i, bool dir_positive)
{
    const unsigned nC = m_nC;
    dReal *aptr = AROW(i) + nC;
    dReal *ptgt = p + nC;
    if (dir_positive) {
        const unsigned nN = m_nN;
        for (unsigned j=0; j < nN; ++j) ptgt[j] += aptr[j];
    }
    else {
        const unsigned nN = m_nN;
        for (unsigned j=0; j < nN; ++j) ptgt[j] -= aptr[j];
    }
}

template<unsigned p_stride>
void dLCP::pC_plusequals_s_times_qC (dReal *p, dReal s, dReal *q)
{
    const unsigned nC = m_nC;
    dReal *q_end = q + nC;
    for (; q != q_end; p += p_stride, ++q) {
        *p += s * (*q);
    }
}

void dLCP::pN_plusequals_s_times_qN (dReal *p, dReal s, dReal *q)
{
    const unsigned nC = m_nC;
    dReal *ptgt = p + nC, *qsrc = q + nC;
    const unsigned nN = m_nN;
    for (unsigned i = 0; i < nN; ++i) {
        ptgt[i] += s * qsrc[i];
    }
}

void dLCP::solve1 (dReal *a, unsigned i, bool dir_positive, int only_transfer)
{
    // the `Dell' and `ell' that are computed here are saved. if index i is
    // later added to the factorization then they can be reused.
    //
    // @@@ question: do we need to solve for entire delta_x??? yes, but
    //     only if an x goes below 0 during the step.

    const unsigned nC = m_nC;
    if (nC > 0) {
        {
            dReal *Dell = m_Dell;
            unsigned *C = m_C;
            dReal *aptr = AROW(i);
#   ifdef NUB_OPTIMIZATIONS
            // if nub>0, initial part of aptr[] is guaranteed unpermuted
            const unsigned nub = m_nub;
            unsigned j = 0;
            for ( ; j < nub; ++j) Dell[j] = aptr[j];
            for ( ; j < nC; ++j) Dell[j] = aptr[C[j]];
#   else
            for (unsigned j = 0; j < nC; ++j) Dell[j] = aptr[C[j]];
#   endif
        }
        dxSolveL1 (m_L, m_Dell, nC, m_nskip);
        {
            dReal *ell = m_ell, *Dell = m_Dell, *d = m_d;
            for (unsigned j = 0; j < nC; ++j) ell[j] = Dell[j] * d[j];
        }

        if (!only_transfer) {
            dReal *tmp = m_tmp, *ell = m_ell;
            {
                for (unsigned j = 0; j < nC; ++j) tmp[j] = ell[j];
            }
            dxSolveL1T (m_L, tmp, nC, m_nskip);
            if (dir_positive) {
                unsigned *C = m_C;
                dReal *tmp = m_tmp;
                for (unsigned j = 0; j < nC; ++j) a[C[j]] = -tmp[j];
            } else {
                unsigned *C = m_C;
                dReal *tmp = m_tmp;
                for (unsigned j = 0; j < nC; ++j) a[C[j]] = tmp[j];
            }
        }
    }
}


void dLCP::unpermute_X()
{
    unsigned *p = m_p;
    dReal *pairsbx = m_pairsbx;
    const unsigned n = m_n;
    for (unsigned j = 0; j < n; ++j) {
        unsigned k = p[j];
        if (k != j) {
            // p[j] = j; -- not going to be checked anymore anyway
            dReal x_j = (pairsbx + (size_t)j * PBX__MAX)[PBX_X];
            for (;;) {
                dxSwap(x_j, (pairsbx + (size_t)k * PBX__MAX)[PBX_X]);

                unsigned orig_k = p[k];
                p[k] = k;
                if (orig_k == j) {
                    break;
                }
                k = orig_k;
            }
            (pairsbx + (size_t)j * PBX__MAX)[PBX_X] = x_j;
        }
    }
}

void dLCP::unpermute_W()
{
    memcpy (m_tmp, m_w, m_n * sizeof(dReal));

    const unsigned *p = m_p;
    dReal *w = m_w, *tmp = m_tmp;
    const unsigned n = m_n;
    for (unsigned j = 0; j < n; ++j) {
        unsigned k = p[j];
        w[k] = tmp[j];
    }
}

#endif // dLCP_FAST


static void dxSolveLCP_AllUnbounded (dxWorldProcessMemArena *memarena, unsigned n, dReal *A, dReal pairsbx[PBX__MAX]);
static void dxSolveLCP_Generic (dxWorldProcessMemArena *memarena, unsigned n, dReal *A, dReal pairsbx[PBX__MAX], 
                                dReal *outer_w/*=NULL*/, unsigned nub, dReal pairslh[PLH__MAX], int *findex);

/*extern */
void dxSolveLCP (dxWorldProcessMemArena *memarena, unsigned n, dReal *A, dReal pairsbx[PBX__MAX],
    dReal *outer_w/*=NULL*/, unsigned nub, dReal pairslh[PLH__MAX], int *findex)
{
    if (nub >= n)
    {
        dxSolveLCP_AllUnbounded (memarena, n, A, pairsbx);
    }
    else
    {
        dxSolveLCP_Generic (memarena, n, A, pairsbx, outer_w, nub, pairslh, findex);
    }
}

//***************************************************************************
// if all the variables are unbounded then we can just factor, solve, and return

static 
void dxSolveLCP_AllUnbounded (dxWorldProcessMemArena *memarena, unsigned n, dReal *A, dReal pairsbx[PBX__MAX])
{
    dAASSERT(A != NULL);
    dAASSERT(pairsbx != NULL);
    dAASSERT(n != 0);

    transfer_b_to_x<true> (pairsbx, n);    

    unsigned nskip = dPAD(n);
    dxtFactorLDLT<PBX__MAX> (A, pairsbx + PBX_B, n, nskip);
    dxtSolveLDLT<PBX__MAX, PBX__MAX> (A, pairsbx + PBX_B, pairsbx + PBX_X, n, nskip);
}

//***************************************************************************
// an optimized Dantzig LCP driver routine for the lo-hi LCP problem.

static 
void dxSolveLCP_Generic (dxWorldProcessMemArena *memarena, unsigned n, dReal *A, dReal pairsbx[PBX__MAX],
    dReal *outer_w/*=NULL*/, unsigned nub, dReal pairslh[PLH__MAX], int *findex)
{
    dAASSERT (n > 0 && A && pairsbx && pairslh && nub >= 0 && nub < n);
# ifndef dNODEBUG
    {
        // check restrictions on lo and hi
        dReal *endlh = pairslh + (size_t)n * PLH__MAX;
        for (dReal *currlh = pairslh; currlh != endlh; currlh += PLH__MAX) dIASSERT (currlh[PLH_LO] <= 0 && currlh[PLH_HI] >= 0);
    }
# endif

    const unsigned nskip = dPAD(n);
    dReal *L = memarena->AllocateOveralignedArray<dReal> ((size_t)nskip * n, LMATRIX_ALIGNMENT);
    dReal *d = memarena->AllocateArray<dReal> (n);
    dReal *w = outer_w != NULL ? outer_w : memarena->AllocateArray<dReal> (n);
    dReal *delta_w = memarena->AllocateArray<dReal> (n);
    dReal *delta_x = memarena->AllocateArray<dReal> (n);
    dReal *Dell = memarena->AllocateArray<dReal> (n);
    dReal *ell = memarena->AllocateArray<dReal> (n);
#ifdef ROWPTRS
    dReal **Arows = memarena->AllocateArray<dReal *> (n);
#else
    dReal **Arows = NULL;
#endif
    unsigned *p = memarena->AllocateArray<unsigned> (n);
    unsigned *C = memarena->AllocateArray<unsigned> (n);

    // for i in N, state[i] is 0 if x(i)==lo(i) or 1 if x(i)==hi(i)
    bool *state = memarena->AllocateArray<bool> (n);

    // create LCP object. note that tmp is set to delta_w to save space, this
    // optimization relies on knowledge of how tmp is used, so be careful!
    dLCP lcp(n, nskip, nub, A, pairsbx, w, pairslh, L, d, Dell, ell, delta_w, state, findex, p, C, Arows);
    unsigned adj_nub = lcp.getNub();

    // loop over all indexes adj_nub..n-1. for index i, if x(i),w(i) satisfy the
    // LCP conditions then i is added to the appropriate index set. otherwise
    // x(i),w(i) is driven either +ve or -ve to force it to the valid region.
    // as we drive x(i), x(C) is also adjusted to keep w(C) at zero.
    // while driving x(i) we maintain the LCP conditions on the other variables
    // 0..i-1. we do this by watching out for other x(i),w(i) values going
    // outside the valid region, and then switching them between index sets
    // when that happens.

    bool hit_first_friction_index = false;
    for (unsigned i = adj_nub; i < n; ++i) {
        bool s_error = false;
        // the index i is the driving index and indexes i+1..n-1 are "dont care",
        // i.e. when we make changes to the system those x's will be zero and we
        // don't care what happens to those w's. in other words, we only consider
        // an (i+1)*(i+1) sub-problem of A*x=b+w.

        // if we've hit the first friction index, we have to compute the lo and
        // hi values based on the values of x already computed. we have been
        // permuting the indexes, so the values stored in the findex vector are
        // no longer valid. thus we have to temporarily unpermute the x vector. 
        // for the purposes of this computation, 0*infinity = 0 ... so if the
        // contact constraint's normal force is 0, there should be no tangential
        // force applied.

        if (!hit_first_friction_index && findex && findex[i] >= 0) {
            // un-permute x into delta_w, which is not being used at the moment
            for (unsigned j = 0; j < n; ++j) delta_w[p[j]] = (pairsbx + (size_t)j * PBX__MAX)[PBX_X];

            // set lo and hi values
            for (unsigned k = i; k < n; ++k) {
                dReal *currlh = pairslh + (size_t)k * PLH__MAX;
                dReal wfk = delta_w[findex[k]];
                if (wfk == 0) {
                    currlh[PLH_HI] = 0;
                    currlh[PLH_LO] = 0;
                }
                else {
                    currlh[PLH_HI] = dFabs (currlh[PLH_HI] * wfk);
                    currlh[PLH_LO] = -currlh[PLH_HI];
                }
            }
            hit_first_friction_index = true;
        }

        // thus far we have not even been computing the w values for indexes
        // greater than i, so compute w[i] now.
        dReal wPrep = lcp.AiC_times_qC<PBX__MAX> (i, pairsbx + PBX_X) + lcp.AiN_times_qN<PBX__MAX> (i, pairsbx + PBX_X);

        dReal *currbx = pairsbx + (size_t)i * PBX__MAX;

        w[i] = wPrep - currbx[PBX_B];

        // if lo=hi=0 (which can happen for tangential friction when normals are
        // 0) then the index will be assigned to set N with some state. however,
        // set C's line has zero size, so the index will always remain in set N.
        // with the "normal" switching logic, if w changed sign then the index
        // would have to switch to set C and then back to set N with an inverted
        // state. this is pointless, and also computationally expensive. to
        // prevent this from happening, we use the rule that indexes with lo=hi=0
        // will never be checked for set changes. this means that the state for
        // these indexes may be incorrect, but that doesn't matter.

        dReal *currlh = pairslh + (size_t)i * PLH__MAX;

        // see if x(i),w(i) is in a valid region
        if (currlh[PLH_LO] == 0 && w[i] >= 0) {
            lcp.transfer_i_to_N (i);
            state[i] = false;
        }
        else if (currlh[PLH_HI] == 0 && w[i] <= 0) {
            lcp.transfer_i_to_N (i);
            state[i] = true;
        }
        else if (w[i] == 0) {
            // this is a degenerate case. by the time we get to this test we know
            // that lo != 0, which means that lo < 0 as lo is not allowed to be +ve,
            // and similarly that hi > 0. this means that the line segment
            // corresponding to set C is at least finite in extent, and we are on it.
            // NOTE: we must call lcp.solve1() before lcp.transfer_i_to_C()
            lcp.solve1 (delta_x, i, false, 1);

            lcp.transfer_i_to_C (i);
        }
        else {
            // we must push x(i) and w(i)
            for (;;) {
                // find direction to push on x(i)
                bool dir_positive = (w[i] <= 0);

                // compute: delta_x(C) = -dir*A(C,C)\A(C,i)
                lcp.solve1 (delta_x, i, dir_positive);

                // note that delta_x[i] = (dir_positive ? 1 : -1), but we wont bother to set it

                // compute: delta_w = A*delta_x ... note we only care about
                // delta_w(N) and delta_w(i), the rest is ignored
                lcp.pN_equals_ANC_times_qC (delta_w, delta_x);
                lcp.pN_plusequals_ANi (delta_w, i, dir_positive);
                delta_w[i] = dir_positive 
                    ? lcp.AiC_times_qC<1> (i, delta_x) + lcp.Aii(i)
                    : lcp.AiC_times_qC<1> (i, delta_x) - lcp.Aii(i);

                // find largest step we can take (size=s), either to drive x(i),w(i)
                // to the valid LCP region or to drive an already-valid variable
                // outside the valid region.

                int cmd = 1;		// index switching command
                unsigned si = 0;		// si = index to switch if cmd>3

                dReal s = delta_w[i] != REAL(0.0)
                    ? -w[i] / delta_w[i]
                    : (w[i] != REAL(0.0) ? dCopySign(dInfinity, -w[i]) : REAL(0.0));
                    
                if (dir_positive) {
                    if (currlh[PLH_HI] < dInfinity) {
                        dReal s2 = (currlh[PLH_HI] - currbx[PBX_X]);	// was (hi[i]-x[i])/dirf	// step to x(i)=hi(i)
                        if (s2 < s) {
                            s = s2;
                            cmd = 3;
                        }
                    }
                }
                else {
                    if (currlh[PLH_LO] > -dInfinity) {
                        dReal s2 = (currbx[PBX_X] - currlh[PLH_LO]); // was (lo[i]-x[i])/dirf	// step to x(i)=lo(i)
                        if (s2 < s) {
                            s = s2;
                            cmd = 2;
                        }
                    }
                }

                {
                    const unsigned numN = lcp.numN();
                    for (unsigned k = 0; k < numN; ++k) {
                        const unsigned indexN_k = lcp.indexN(k);
                        if (!state[indexN_k] ? delta_w[indexN_k] < 0 : delta_w[indexN_k] > 0) {
                            // don't bother checking if lo=hi=0
                            dReal *indexlh = pairslh + (size_t)indexN_k * PLH__MAX;
                            if (indexlh[PLH_LO] == 0 && indexlh[PLH_HI] == 0) continue;
                            dReal s2 = -w[indexN_k] / delta_w[indexN_k];
                            if (s2 < s) {
                                s = s2;
                                cmd = 4;
                                si = indexN_k;
                            }
                        }
                    }
                }

                {
                    const unsigned numC = lcp.numC();
                    for (unsigned k = adj_nub; k < numC; ++k) {
                        const unsigned indexC_k = lcp.indexC(k);
                        dReal *indexlh = pairslh + (size_t)indexC_k * PLH__MAX;
                        if (delta_x[indexC_k] < 0 && indexlh[PLH_LO] > -dInfinity) {
                            dReal s2 = (indexlh[PLH_LO] - (pairsbx + (size_t)indexC_k * PBX__MAX)[PBX_X]) / delta_x[indexC_k];
                            if (s2 < s) {
                                s = s2;
                                cmd = 5;
                                si = indexC_k;
                            }
                        }
                        if (delta_x[indexC_k] > 0 && indexlh[PLH_HI] < dInfinity) {
                            dReal s2 = (indexlh[PLH_HI] - (pairsbx + (size_t)indexC_k * PBX__MAX)[PBX_X]) / delta_x[indexC_k];
                            if (s2 < s) {
                                s = s2;
                                cmd = 6;
                                si = indexC_k;
                            }
                        }
                    }
                }

                //static char* cmdstring[8] = {0,"->C","->NL","->NH","N->C",
                //			     "C->NL","C->NH"};
                //printf ("cmd=%d (%s), si=%d\n",cmd,cmdstring[cmd],(cmd>3) ? si : i);

                // if s <= 0 then we've got a problem. if we just keep going then
                // we're going to get stuck in an infinite loop. instead, just cross
                // our fingers and exit with the current solution.
                if (s <= REAL(0.0)) {
                    dMessage (d_ERR_LCP, "LCP internal error, s <= 0 (s=%.4e)",(double)s);
                    if (i < n) {
                        dxtSetZero<PBX__MAX>(currbx + PBX_X, n - i);
                        dxSetZero (w + i, n - i);
                    }
                    s_error = true;
                    break;
                }

                // apply x = x + s * delta_x
                lcp.pC_plusequals_s_times_qC<PBX__MAX> (pairsbx + PBX_X, s, delta_x);
                currbx[PBX_X] = dir_positive 
                    ? currbx[PBX_X] + s
                    : currbx[PBX_X] - s;

                // apply w = w + s * delta_w
                lcp.pN_plusequals_s_times_qN (w, s, delta_w);
                w[i] += s * delta_w[i];

                void *tmpbuf;
                // switch indexes between sets if necessary
                switch (cmd) {
                case 1:		// done
                    w[i] = 0;
                    lcp.transfer_i_to_C (i);
                    break;
                case 2:		// done
                    currbx[PBX_X] = currlh[PLH_LO];
                    state[i] = false;
                    lcp.transfer_i_to_N (i);
                    break;
                case 3:		// done
                    currbx[PBX_X] = currlh[PLH_HI];
                    state[i] = true;
                    lcp.transfer_i_to_N (i);
                    break;
                case 4:		// keep going
                    w[si] = 0;
                    lcp.transfer_i_from_N_to_C (si);
                    break;
                case 5:		// keep going
                    (pairsbx + (size_t)si * PBX__MAX)[PBX_X] = (pairslh + (size_t)si * PLH__MAX)[PLH_LO];
                    state[si] = false;
                    tmpbuf = memarena->PeekBufferRemainder();
                    lcp.transfer_i_from_C_to_N (si, tmpbuf);
                    break;
                case 6:		// keep going
                    (pairsbx + (size_t)si * PBX__MAX)[PBX_X] = (pairslh + (size_t)si * PLH__MAX)[PLH_HI];
                    state[si] = true;
                    tmpbuf = memarena->PeekBufferRemainder();
                    lcp.transfer_i_from_C_to_N (si, tmpbuf);
                    break;
                }

                if (cmd <= 3) break;
            } // for (;;)
        } // else

        if (s_error) {
            break;
        }
    } // for (unsigned i = adj_nub; i < n; ++i)

    // now we have to un-permute x and w
    if (outer_w != NULL) {
        lcp.unpermute_W();
    }
    lcp.unpermute_X(); // This destroys p[] and must be done last
}

size_t dxEstimateSolveLCPMemoryReq(unsigned n, bool outer_w_avail)
{
    const unsigned nskip = dPAD(n);

    size_t res = 0;

    res += dOVERALIGNED_SIZE(sizeof(dReal) * ((size_t)n * nskip), LMATRIX_ALIGNMENT); // for L
    res += 5 * dEFFICIENT_SIZE(sizeof(dReal) * n); // for d, delta_w, delta_x, Dell, ell
    if (!outer_w_avail) {
        res += dEFFICIENT_SIZE(sizeof(dReal) * n); // for w
    }
#ifdef ROWPTRS
    res += dEFFICIENT_SIZE(sizeof(dReal *) * n); // for Arows
#endif
    res += 2 * dEFFICIENT_SIZE(sizeof(unsigned) * n); // for p, C
    res += dEFFICIENT_SIZE(sizeof(bool) * n); // for state

    // Use n instead of nC as nC varies at runtime while n is greater or equal to nC
    size_t lcp_transfer_req = dLCP::estimate_transfer_i_from_C_to_N_mem_req(n, nskip);
    res += dEFFICIENT_SIZE(lcp_transfer_req); // for dLCP::transfer_i_from_C_to_N

    return res;
}


//***************************************************************************
// accuracy and timing test

static size_t EstimateTestSolveLCPMemoryReq(unsigned n)
{
    const unsigned nskip = dPAD(n);

    size_t res = 0;

    res += 2 * dEFFICIENT_SIZE(sizeof(dReal) * ((size_t)n * nskip)); // for A, A2
    res += 7 * dEFFICIENT_SIZE(sizeof(dReal) * n); // for x, b, w, lo, hi, tmp1, tmp2
    res += dEFFICIENT_SIZE(sizeof(dReal) * PBX__MAX * n); // for pairsbx, 
    res += dEFFICIENT_SIZE(sizeof(dReal) * PLH__MAX * n); // for pairslh

    res += dxEstimateSolveLCPMemoryReq(n, true);

    return res;
}

extern "C" ODE_API int dTestSolveLCP()
{
    const unsigned n = 100;

    size_t memreq = EstimateTestSolveLCPMemoryReq(n);
    dxWorldProcessMemArena *arena = dxAllocateTemporaryWorldProcessMemArena(memreq, NULL, NULL);
    if (arena == NULL) {
        return 0;
    }
    arena->ResetState();

    unsigned i,nskip = dPAD(n);
#ifdef dDOUBLE
    const dReal tol = REAL(1e-9);
#endif
#ifdef dSINGLE
    const dReal tol = REAL(1e-4);
#endif
    printf ("dTestSolveLCP()\n");

    dReal *A = arena->AllocateArray<dReal> (n*nskip);
    dReal *x = arena->AllocateArray<dReal> (n);
    dReal *b = arena->AllocateArray<dReal> (n);
    dReal *w = arena->AllocateArray<dReal> (n);
    dReal *lo = arena->AllocateArray<dReal> (n);
    dReal *hi = arena->AllocateArray<dReal> (n);

    dReal *A2 = arena->AllocateArray<dReal> (n*nskip);
    dReal *pairsbx = arena->AllocateArray<dReal> (n * PBX__MAX);
    dReal *pairslh = arena->AllocateArray<dReal> (n * PLH__MAX);

    dReal *tmp1 = arena->AllocateArray<dReal> (n);
    dReal *tmp2 = arena->AllocateArray<dReal> (n);

    double total_time = 0;
    for (unsigned count=0; count < 1000; count++) {
        BEGIN_STATE_SAVE(arena, saveInner) {

            // form (A,b) = a random positive definite LCP problem
            dMakeRandomMatrix (A2,n,n,1.0);
            dMultiply2 (A,A2,A2,n,n,n);
            dMakeRandomMatrix (x,n,1,1.0);
            dMultiply0 (b,A,x,n,n,1);
            for (i=0; i<n; i++) b[i] += (dRandReal()*REAL(0.2))-REAL(0.1);

            // choose `nub' in the range 0..n-1
            unsigned nub = 50; //dRandInt (n);

            // make limits
            for (i=0; i<nub; i++) lo[i] = -dInfinity;
            for (i=0; i<nub; i++) hi[i] = dInfinity;
            //for (i=nub; i<n; i++) lo[i] = 0;
            //for (i=nub; i<n; i++) hi[i] = dInfinity;
            //for (i=nub; i<n; i++) lo[i] = -dInfinity;
            //for (i=nub; i<n; i++) hi[i] = 0;
            for (i=nub; i<n; i++) lo[i] = -(dRandReal()*REAL(1.0))-REAL(0.01);
            for (i=nub; i<n; i++) hi[i] =  (dRandReal()*REAL(1.0))+REAL(0.01);

            // set a few limits to lo=hi=0
            /*
            for (i=0; i<10; i++) {
            unsigned j = dRandInt (n-nub) + nub;
            lo[j] = 0;
            hi[j] = 0;
            }
            */

            // solve the LCP. we must make copy of A,b,lo,hi (A2,b2,lo2,hi2) for
            // SolveLCP() to permute. also, we'll clear the upper triangle of A2 to
            // ensure that it doesn't get referenced (if it does, the answer will be
            // wrong).

            memcpy (A2, A, n * nskip * sizeof(dReal));
            dClearUpperTriangle (A2, n);
            for (i = 0; i != n; ++i) {
                dReal *currbx = pairsbx + i * PBX__MAX;
                currbx[PBX_B] = b[i];
                currbx[PBX_X] = 0;
            }
            for (i = 0; i != n; ++i) {
                dReal *currlh = pairslh + i * PLH__MAX;
                currlh[PLH_LO] = lo[i];
                currlh[PLH_HI] = hi[i];
            }
            dSetZero (w,n);

            dStopwatch sw;
            dStopwatchReset (&sw);
            dStopwatchStart (&sw);

            dxSolveLCP (arena,n,A2,pairsbx,w,nub,pairslh,0);

            dStopwatchStop (&sw);
            double time = dStopwatchTime(&sw);
            total_time += time;
            double average = total_time / double(count+1) * 1000.0;

            for (i = 0; i != n; ++i) {
                const dReal *currbx = pairsbx + i * PBX__MAX;
                x[i] = currbx[PBX_X];
            }

            // check the solution

            dMultiply0 (tmp1,A,x,n,n,1);
            for (i=0; i<n; i++) tmp2[i] = b[i] + w[i];
            dReal diff = dMaxDifference (tmp1,tmp2,n,1);
            // printf ("\tA*x = b+w, maximum difference = %.6e - %s (1)\n",diff,
            //	    diff > tol ? "FAILED" : "passed");
            if (diff > tol) dDebug (0,"A*x = b+w, maximum difference = %.6e",diff);
            unsigned n1=0,n2=0,n3=0;
            for (i=0; i<n; i++) {
                if (x[i]==lo[i] && w[i] >= 0) {
                    n1++;	// ok
                }
                else if (x[i]==hi[i] && w[i] <= 0) {
                    n2++;	// ok
                }
                else if (x[i] >= lo[i] && x[i] <= hi[i] && w[i] == 0) {
                    n3++;	// ok
                }
                else {
                    dDebug (0,"FAILED: i=%d x=%.4e w=%.4e lo=%.4e hi=%.4e",i,
                        x[i],w[i],lo[i],hi[i]);
                }
            }

            // pacifier
            printf ("passed: NL=%3d NH=%3d C=%3d   ",n1,n2,n3);
            printf ("time=%10.3f ms  avg=%10.4f\n",time * 1000.0,average);

        } END_STATE_SAVE(arena, saveInner);
    }

    dxFreeTemporaryWorldProcessMemArena(arena);
    return 1;
}
