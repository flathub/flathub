/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
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

#include <ode/common.h>
#include <ode/rotation.h>
#include <ode/timer.h>
#include <ode/error.h>
#include <ode/misc.h>
#include "config.h"
#include "matrix.h"
#include "odemath.h"
#include "objects.h"
#include "joints/joint.h"
#include "lcp.h"
#include "util.h"
#include "threadingutils.h"

#include <new>


//***************************************************************************
// configuration

// for the SOR and CG methods:
// uncomment the following line to use warm starting. this definitely
// help for motor-driven joints. unfortunately it appears to hurt
// with high-friction contacts using the SOR method. use with care

// #define WARM_STARTING 1


#define REORDERING_METHOD__DONT_REORDER 0
#define REORDERING_METHOD__BY_ERROR     1
#define REORDERING_METHOD__RANDOMLY     2

// for the SOR method:
// uncomment the following line to determine a new constraint-solving
// order for each iteration. however, the qsort per iteration is expensive,
// and the optimal order is somewhat problem dependent.
// @@@ try the leaf->root ordering.

// #define CONSTRAINTS_REORDERING_METHOD REORDERING_METHOD__BY_ERROR


// for the SOR method:
// uncomment the following line to randomly reorder constraint rows
// during the solution. depending on the situation, this can help a lot
// or hardly at all, but it doesn't seem to hurt.

#define CONSTRAINTS_REORDERING_METHOD REORDERING_METHOD__RANDOMLY


#if !defined(CONSTRAINTS_REORDERING_METHOD)
#define CONSTRAINTS_REORDERING_METHOD REORDERING_METHOD__DONT_REORDER
#endif


//***************************************************************************
// macros, typedefs, forwards and inlines

struct IndexError;


#define dMIN(A,B)  ((A)>(B) ? (B) : (A))
#define dMAX(A,B)  ((B)>(A) ? (B) : (A))


#define dxQUICKSTEPISLAND_STAGE2B_STEP  16U
#define dxQUICKSTEPISLAND_STAGE2C_STEP  32U

#ifdef WARM_STARTING
#define dxQUICKSTEPISLAND_STAGE4A_STEP  256U
#else
#define dxQUICKSTEPISLAND_STAGE4A_STEP  512U
#endif

#define dxQUICKSTEPISLAND_STAGE4LCP_IMJ_STEP 8U
#define dxQUICKSTEPISLAND_STAGE4LCP_AD_STEP  8U

#ifdef WARM_STARTING
#define dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP  128U
#define dxQUICKSTEPISLAND_STAGE4LCP_FC_COMPLETE_TO_PREPARE_COMPLEXITY_DIVISOR  4
#define dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP_PREPARE  (dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP * dxQUICKSTEPISLAND_STAGE4LCP_FC_COMPLETE_TO_PREPARE_COMPLEXITY_DIVISOR)
#define dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP_COMPLETE (dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP)
#else
#define dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP  (dxQUICKSTEPISLAND_STAGE4A_STEP / 2) // Average info.m is 3 for stage4a, while there are 6 reals per index in fc
#endif

#define dxQUICKSTEPISLAND_STAGE4B_STEP  256U

#define dxQUICKSTEPISLAND_STAGE6A_STEP  16U
#define dxQUICKSTEPISLAND_STAGE6B_STEP  1U

template<unsigned int step_size>
inline unsigned int CalculateOptimalThreadsCount(unsigned int complexity, unsigned int max_threads)
{
    unsigned int raw_threads = dMAX(complexity, step_size) / step_size; // Round down on division 
    unsigned int optimum = dMIN(raw_threads, max_threads);
    return optimum;
}

#define dxENCODE_INDEX(index)   ((unsigned int)((index) + 1))
#define dxDECODE_INDEX(code)    ((unsigned int)((code) - 1))
#define dxHEAD_INDEX            0

//****************************************************************************
// special matrix multipliers

// multiply block of B matrix (q x 6) with 12 dReal per row with C vector (q)
static inline void Multiply1_12q1 (dReal *A, const dReal *B, const dReal *C, unsigned int q)
{
    dIASSERT (q>0 && A && B && C);

    dReal a = 0;
    dReal b = 0;
    dReal c = 0;
    dReal d = 0;
    dReal e = 0;
    dReal f = 0;
    dReal s;

    for(unsigned int i=0, k = 0; i<q; k += 12, i++)
    {
        s = C[i]; //C[i] and B[n+k] cannot overlap because its value has been read into a temporary.

        //For the rest of the loop, the only memory dependency (array) is from B[]
        a += B[  k] * s;
        b += B[1+k] * s;
        c += B[2+k] * s;
        d += B[3+k] * s;
        e += B[4+k] * s;
        f += B[5+k] * s;
    }

    A[0] = a;
    A[1] = b;
    A[2] = c;
    A[3] = d;
    A[4] = e;
    A[5] = f;
}

//***************************************************************************
// testing stuff

#ifdef TIMING
#define IFTIMING(x) x
#else
#define IFTIMING(x) ((void)0)
#endif


struct dJointWithInfo1
{
    dxJoint *joint;
    dxJoint::Info1 info;
};


struct dxMIndexItem
{
    unsigned        mIndex;
    unsigned        fbIndex;
};

struct dxJBodiesItem
{
    unsigned        first;
    int             second; // The index is optional and can equal to -1
};

enum dxInvIRowElement
{
    IIE__MIN,

    IIE__MATRIX_MIN = IIE__MIN,
    IIE__MATRIX_MAX = IIE__MATRIX_MIN + dM3E__MAX,

    IIE__MAX = IIE__MATRIX_MAX,
};

enum dxRHSCFMElement
{
    RCE_RHS = dxJoint::GI2_RHS,
    RCE_CFM = dxJoint::GI2_CFM,

    RCE__RHS_CFM_MAX = dxJoint::GI2__RHS_CFM_MAX,
};

enum dxLoHiElement
{
    LHE_LO = dxJoint::GI2_LO,
    LHE_HI = dxJoint::GI2_HI,

    LHE__LO_HI_MAX = dxJoint::GI2__LO_HI_MAX,
};

enum dxJacobiVectorElement
{
    JVE__MIN,
    
    JVE__L_MIN = JVE__MIN + dDA__L_MIN,

    JVE_LX = JVE__MIN + dDA_LX,
    JVE_LY = JVE__MIN + dDA_LY,
    JVE_LZ = JVE__MIN + dDA_LZ,

    JVE__L_MAX = JVE__MIN + dDA__L_MAX,

    JVE__A_MIN = JVE__MIN + dDA__A_MIN,

    JVE_AX = JVE__MIN + dDA_AX,
    JVE_AY = JVE__MIN + dDA_AY,
    JVE_AZ = JVE__MIN + dDA_AZ,

    JVE__A_MAX = JVE__MIN + dDA__A_MAX,
    
    JVE__MAX = JVE__MIN + dDA__MAX,

    JVE__L_COUNT = JVE__L_MAX - JVE__L_MIN,
    JVE__A_COUNT = JVE__A_MAX - JVE__A_MIN,
};

enum dxJacobiMatrixElement
{
    JME__MIN,

    JME__J1_MIN = JME__MIN,
    JME__J1L_MIN = JME__J1_MIN + JVE__L_MIN,

    JME_J1LX = JME__J1_MIN + JVE_LX,
    JME_J1LY = JME__J1_MIN + JVE_LY,
    JME_J1LZ = JME__J1_MIN + JVE_LZ,

    JME__J1L_MAX = JME__J1_MIN + JVE__L_MAX,

    JME__J1A_MIN = JME__J1_MIN + JVE__A_MIN,

    JME_J1AX = JME__J1_MIN + JVE_AX,
    JME_J1AY = JME__J1_MIN + JVE_AY,
    JME_J1AZ = JME__J1_MIN + JVE_AZ,

    JME__J1A_MAX = JME__J1_MIN + JVE__A_MAX,
    JME__J1_MAX = JME__J1_MIN + JVE__MAX,

    JME__RHS_CFM_MIN = JME__J1_MAX,
    JME_RHS = JME__RHS_CFM_MIN + RCE_RHS,
    JME_CFM = JME__RHS_CFM_MIN + RCE_CFM,
    JME__RHS_CFM_MAX = JME__RHS_CFM_MIN + RCE__RHS_CFM_MAX,

    JME__J2_MIN = JME__RHS_CFM_MAX,
    JME__J2L_MIN = JME__J2_MIN + JVE__L_MIN,

    JME_J2LX = JME__J2_MIN + JVE_LX,
    JME_J2LY = JME__J2_MIN + JVE_LY,
    JME_J2LZ = JME__J2_MIN + JVE_LZ,

    JME__J2L_MAX = JME__J2_MIN + JVE__L_MAX,

    JME__J2A_MIN = JME__J2_MIN + JVE__A_MIN,

    JME_J2AX = JME__J2_MIN + JVE_AX,
    JME_J2AY = JME__J2_MIN + JVE_AY,
    JME_J2AZ = JME__J2_MIN + JVE_AZ,

    JME__J2A_MAX = JME__J2_MIN + JVE__A_MAX,
    JME__J2_MAX = JME__J2_MIN + JVE__MAX,

    JME__LO_HI_MIN = JME__J2_MAX,
    JME_LO = JME__LO_HI_MIN + LHE_LO,
    JME_HI = JME__LO_HI_MIN + LHE_HI,
    JME__LO_HI_MAX = JME__LO_HI_MIN + LHE__LO_HI_MAX,

    JME__MAX = JME__LO_HI_MAX, // Is not that a luck to have 16 elements here? ;-)

    JME__J1_COUNT = JME__J1_MAX - JME__J1_MIN,
    JME__J2_COUNT = JME__J2_MAX - JME__J2_MIN,
    JME__J_COUNT = JVE__MAX,
};

dSASSERT(JME__J_COUNT == JME__J1_COUNT);
dSASSERT(JME__J_COUNT == JME__J2_COUNT);

enum dxJacobiCopyElement
{
    JCE__MIN,

    JCE__J1_MIN = JCE__MIN,
    JCE__J1L_MIN = JCE__J1_MIN,

    JCE_J1LX = JCE__J1L_MIN,
    JCE_J1LY,
    JCE_J1LZ,

    JCE__J1L_MAX,

    JCE__J1A_MIN = JCE__J1L_MAX,

    JCE_J1AX = JCE__J1A_MIN,
    JCE_J1AY,
    JCE_J1AZ,

    JCE__J1A_MAX,
    JCE__J1_MAX = JCE__J1A_MAX,

    JCE__J2_MIN = JCE__J1_MAX,
    JCE__J2L_MIN = JCE__J2_MIN,

    JCE_J2LX = JCE__J2L_MIN,
    JCE_J2LY,
    JCE_J2LZ,

    JCE__J2L_MAX,

    JCE__J2A_MIN = JCE__J2L_MAX,

    JCE_J2AX = JCE__J2A_MIN,
    JCE_J2AY,
    JCE_J2AZ,

    JCE__J2A_MAX,
    JCE__J2_MAX = JCE__J2A_MAX,

    JCE__MAX = JCE__J2_MAX,

    JCE__J1_COUNT = JCE__J1_MAX - JCE__J1_MIN,
    JCE__J2_COUNT = JCE__J2_MAX - JCE__J2_MIN,
    JCE__JMAX_COUNT = dMAX(JCE__J1_COUNT, JCE__J2_COUNT),
};

enum dxInvMJTElement
{
    IMJ__MIN,

    IMJ__1_MIN = IMJ__MIN,
    
    IMJ__1L_MIN = IMJ__1_MIN + JVE__L_MIN,

    IMJ_1LX = IMJ__1_MIN + JVE_LX,
    IMJ_1LY = IMJ__1_MIN + JVE_LY,
    IMJ_1LZ = IMJ__1_MIN + JVE_LZ,

    IMJ__1L_MAX = IMJ__1_MIN + JVE__L_MAX,

    IMJ__1A_MIN = IMJ__1_MIN + JVE__A_MIN,

    IMJ_1AX = IMJ__1_MIN + JVE_AX,
    IMJ_1AY = IMJ__1_MIN + JVE_AY,
    IMJ_1AZ = IMJ__1_MIN + JVE_AZ,

    IMJ__1A_MAX = IMJ__1_MIN + JVE__A_MAX,
    
    IMJ__1_MAX = IMJ__1_MIN + JVE__MAX,

    IMJ__2_MIN = IMJ__1_MAX,

    IMJ__2L_MIN = IMJ__2_MIN + JVE__L_MIN,

    IMJ_2LX = IMJ__2_MIN + JVE_LX,
    IMJ_2LY = IMJ__2_MIN + JVE_LY,
    IMJ_2LZ = IMJ__2_MIN + JVE_LZ,

    IMJ__2L_MAX = IMJ__2_MIN + JVE__L_MAX,

    IMJ__2A_MIN = IMJ__2_MIN + JVE__A_MIN,

    IMJ_2AX = IMJ__2_MIN + JVE_AX,
    IMJ_2AY = IMJ__2_MIN + JVE_AY,
    IMJ_2AZ = IMJ__2_MIN + JVE_AZ,

    IMJ__2A_MAX = IMJ__2_MIN + JVE__A_MAX,

    IMJ__2_MAX = IMJ__2_MIN + JVE__MAX,

    IMJ__MAX = IMJ__2_MAX,
};

enum dxContactForceElement
{
    CFE__MIN,

    CFE__DYNAMICS_MIN = CFE__MIN,

    CFE__L_MIN = CFE__DYNAMICS_MIN + dDA__L_MIN,

    CFE_LX = CFE__DYNAMICS_MIN + dDA_LX,
    CFE_LY = CFE__DYNAMICS_MIN + dDA_LY,
    CFE_LZ = CFE__DYNAMICS_MIN + dDA_LZ,

    CFE__L_MAX = CFE__DYNAMICS_MIN + dDA__L_MAX,

    CFE__A_MIN = CFE__DYNAMICS_MIN + dDA__A_MIN,

    CFE_AX = CFE__DYNAMICS_MIN + dDA_AX,
    CFE_AY = CFE__DYNAMICS_MIN + dDA_AY,
    CFE_AZ = CFE__DYNAMICS_MIN + dDA_AZ,

    CFE__A_MAX = CFE__DYNAMICS_MIN + dDA__A_MAX,

    CFE__DYNAMICS_MAX = CFE__DYNAMICS_MIN + dDA__MAX,

    CFE__MAX = CFE__DYNAMICS_MAX,
};

enum dxRHSElement
{
    RHS__MIN,

    RHS__DYNAMICS_MIN = RHS__MIN,

    RHS__L_MIN = RHS__DYNAMICS_MIN + dDA__L_MIN,

    RHS_LX = RHS__DYNAMICS_MIN + dDA_LX,
    RHS_LY = RHS__DYNAMICS_MIN + dDA_LY,
    RHS_LZ = RHS__DYNAMICS_MIN + dDA_LZ,

    RHS__L_MAX = RHS__DYNAMICS_MIN + dDA__L_MAX,

    RHS__A_MIN = RHS__DYNAMICS_MIN + dDA__A_MIN,

    RHS_AX = RHS__DYNAMICS_MIN + dDA_AX,
    RHS_AY = RHS__DYNAMICS_MIN + dDA_AY,
    RHS_AZ = RHS__DYNAMICS_MIN + dDA_AZ,

    RHS__A_MAX = RHS__DYNAMICS_MIN + dDA__A_MAX,

    RHS__DYNAMICS_MAX = RHS__DYNAMICS_MIN + dDA__MAX,

    RHS__MAX = RHS__DYNAMICS_MAX,
};


#define JACOBIAN_ALIGNMENT  dMAX(JME__MAX * sizeof(dReal), EFFICIENT_ALIGNMENT)
dSASSERT(((JME__MAX - 1) & JME__MAX) == 0); // Otherwise there is no reason to over-align the Jacobian

#define JCOPY_ALIGNMENT    dMAX(32, EFFICIENT_ALIGNMENT)
#define INVI_ALIGNMENT     dMAX(32, EFFICIENT_ALIGNMENT)
#define INVMJ_ALIGNMENT    dMAX(32, EFFICIENT_ALIGNMENT)


struct dxQuickStepperStage0Outputs
{
    unsigned int                    nj;
    unsigned int                    m;
    unsigned int                    mfb;
};

struct dxQuickStepperStage1CallContext
{
    void Initialize(const dxStepperProcessingCallContext *stepperCallContext, void *stageMemArenaState, dReal *invI, dJointWithInfo1 *jointinfos)
    {
        m_stepperCallContext = stepperCallContext;
        m_stageMemArenaState = stageMemArenaState; 
        m_invI = invI;
        m_jointinfos = jointinfos;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    void                            *m_stageMemArenaState;
    dReal                           *m_invI;
    dJointWithInfo1                 *m_jointinfos;
    dxQuickStepperStage0Outputs     m_stage0Outputs;
};

struct dxQuickStepperStage0BodiesCallContext
{
    void Initialize(const dxStepperProcessingCallContext *stepperCallContext, dReal *invI)
    {
        m_stepperCallContext = stepperCallContext;
        m_invI = invI;
        m_tagsTaken = 0;
        m_gravityTaken = 0;
        m_inertiaBodyIndex = 0;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    dReal                           *m_invI;
    atomicord32                     m_tagsTaken;
    atomicord32                     m_gravityTaken;
    volatile atomicord32            m_inertiaBodyIndex;
};

struct dxQuickStepperStage0JointsCallContext
{
    void Initialize(const dxStepperProcessingCallContext *stepperCallContext, dJointWithInfo1 *jointinfos, dxQuickStepperStage0Outputs *stage0Outputs)
    {
        m_stepperCallContext = stepperCallContext;
        m_jointinfos = jointinfos;
        m_stage0Outputs = stage0Outputs;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    dJointWithInfo1                 *m_jointinfos;
    dxQuickStepperStage0Outputs     *m_stage0Outputs;
};

static int dxQuickStepIsland_Stage0_Bodies_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage0_Joints_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage1_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);

static void dxQuickStepIsland_Stage0_Bodies(dxQuickStepperStage0BodiesCallContext *callContext);
static void dxQuickStepIsland_Stage0_Joints(dxQuickStepperStage0JointsCallContext *callContext);
static void dxQuickStepIsland_Stage1(dxQuickStepperStage1CallContext *callContext);


struct dxQuickStepperLocalContext
{
    void Initialize(dReal *invI, dJointWithInfo1 *jointinfos, unsigned int nj, 
        unsigned int m, unsigned int mfb, const dxMIndexItem *mindex, dxJBodiesItem *jb, int *findex, 
        dReal *J, dReal *Jcopy)
    {
        m_invI = invI;
        m_jointinfos = jointinfos;
        m_nj = nj;
        m_m = m;
        m_mfb = mfb;
        m_valid_findices = 0;
        m_mindex = mindex;
        m_jb = jb;
        m_findex = findex; 
        m_J = J;
        m_Jcopy = Jcopy;
    }

    dReal                           *m_invI;
    dJointWithInfo1                 *m_jointinfos;
    unsigned int                    m_nj;
    unsigned int                    m_m;
    unsigned int                    m_mfb;
    volatile atomicord32            m_valid_findices;
    const dxMIndexItem              *m_mindex;
    dxJBodiesItem                   *m_jb;
    int                             *m_findex;
    dReal                           *m_J;
    dReal                           *m_Jcopy;
};

struct dxQuickStepperStage3CallContext
{
    void Initialize(const dxStepperProcessingCallContext *callContext, const dxQuickStepperLocalContext *localContext, 
        void *stage1MemArenaState)
    {
        m_stepperCallContext = callContext;
        m_localContext = localContext;
        m_stage1MemArenaState = stage1MemArenaState;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    const dxQuickStepperLocalContext   *m_localContext;
    void                            *m_stage1MemArenaState;
};

struct dxQuickStepperStage2CallContext
{
    void Initialize(const dxStepperProcessingCallContext *callContext, dxQuickStepperLocalContext *localContext, 
        dReal *rhs_tmp)
    {
        m_stepperCallContext = callContext;
        m_localContext = localContext;
        m_rhs_tmp = rhs_tmp;
        m_ji_J = 0;
        m_ji_jb = 0;
        m_bi = 0;
        m_Jrhsi = 0;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    dxQuickStepperLocalContext      *m_localContext;
    dReal                           *m_rhs_tmp;
    volatile atomicord32            m_ji_J;
    volatile atomicord32            m_ji_jb;
    volatile atomicord32            m_bi;
    volatile atomicord32            m_Jrhsi;
};

static int dxQuickStepIsland_Stage2a_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage2aSync_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage2b_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage2bSync_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage2c_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage3_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);

static void dxQuickStepIsland_Stage2a(dxQuickStepperStage2CallContext *stage2CallContext);
static void dxQuickStepIsland_Stage2b(dxQuickStepperStage2CallContext *stage2CallContext);
static void dxQuickStepIsland_Stage2c(dxQuickStepperStage2CallContext *stage2CallContext);
static void dxQuickStepIsland_Stage3(dxQuickStepperStage3CallContext *stage3CallContext);


struct dxQuickStepperStage5CallContext
{
    void Initialize(const dxStepperProcessingCallContext *callContext, const dxQuickStepperLocalContext *localContext, 
        void *stage3MemArenaState)
    {
        m_stepperCallContext = callContext;
        m_localContext = localContext;
        m_stage3MemArenaState = stage3MemArenaState;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    const dxQuickStepperLocalContext   *m_localContext;
    void                            *m_stage3MemArenaState;
};

struct dxQuickStepperStage4CallContext
{
    void Initialize(const dxStepperProcessingCallContext *callContext, const dxQuickStepperLocalContext *localContext, 
        dReal *lambda, dReal *cforce, dReal *iMJ, IndexError *order, dReal *last_lambda, atomicord32 *bi_links_or_mi_levels, atomicord32 *mi_links)
    {
        m_stepperCallContext = callContext;
        m_localContext = localContext;
        m_lambda = lambda;
        m_cforce = cforce;
        m_iMJ = iMJ;
        m_order = order;
        m_last_lambda = last_lambda;
        m_bi_links_or_mi_levels = bi_links_or_mi_levels;
        m_mi_links = mi_links;
        m_LCP_IterationSyncReleasee = NULL;
        m_LCP_IterationAllowedThreads = 0;
        m_LCP_fcStartReleasee = NULL;
        m_ji_4a = 0;
        m_mi_iMJ = 0;
        m_mi_fc = 0;
        m_mi_Ad = 0;
        m_LCP_iteration = 0;
        m_cf_4b = 0;
        m_ji_4b = 0;
    }

    void AssignLCP_IterationData(dCallReleaseeID releaseeInstance, unsigned int iterationAllowedThreads)
    {
        m_LCP_IterationSyncReleasee = releaseeInstance;
        m_LCP_IterationAllowedThreads = iterationAllowedThreads;
    }

    void AssignLCP_fcStartReleasee(dCallReleaseeID releaseeInstance)
    {
        m_LCP_fcStartReleasee = releaseeInstance;
    }

    void AssignLCP_fcAllowedThreads(unsigned int prepareThreads, unsigned int completeThreads)
    {
        m_LCP_fcPrepareThreadsRemaining = prepareThreads;
        m_LCP_fcCompleteThreadsTotal = completeThreads;
    }

    void ResetLCP_fcComputationIndex()
    {
        m_mi_fc = 0;
    }

    void ResetSOR_ConstraintsReorderVariables(unsigned reorderThreads)
    {
        m_SOR_reorderHeadTaken = 0;
        m_SOR_reorderTailTaken = 0;
        m_SOR_bi_zeroHeadTaken = 0;
        m_SOR_bi_zeroTailTaken = 0;
        m_SOR_mi_zeroHeadTaken = 0;
        m_SOR_mi_zeroTailTaken = 0;
        m_SOR_reorderThreadsRemaining = reorderThreads;
    }

    void RecordLCP_IterationStart(unsigned int totalThreads, dCallReleaseeID nextReleasee)
    {
        m_LCP_iterationThreadsTotal = totalThreads;
        m_LCP_iterationThreadsRemaining = totalThreads;
        m_LCP_iterationNextReleasee = nextReleasee;
    }


    const dxStepperProcessingCallContext *m_stepperCallContext;
    const dxQuickStepperLocalContext   *m_localContext;
    dReal                           *m_lambda;
    dReal                           *m_cforce;
    dReal                           *m_iMJ;
    IndexError                      *m_order;
    dReal                           *m_last_lambda;
    atomicord32                     *m_bi_links_or_mi_levels;
    atomicord32                     *m_mi_links;
    dCallReleaseeID                 m_LCP_IterationSyncReleasee;
    unsigned int                    m_LCP_IterationAllowedThreads;
    dCallReleaseeID                 m_LCP_fcStartReleasee;
    volatile atomicord32            m_ji_4a;
    volatile atomicord32            m_mi_iMJ;
    volatile atomicord32            m_mi_fc;
    volatile atomicord32            m_LCP_fcPrepareThreadsRemaining;
    unsigned int                    m_LCP_fcCompleteThreadsTotal;
    volatile atomicord32            m_mi_Ad;
    unsigned int                    m_LCP_iteration;
    unsigned int                    m_LCP_iterationThreadsTotal;
    volatile atomicord32            m_LCP_iterationThreadsRemaining;
    dCallReleaseeID                 m_LCP_iterationNextReleasee;
    volatile atomicord32            m_SOR_reorderHeadTaken;
    volatile atomicord32            m_SOR_reorderTailTaken;
    volatile atomicord32            m_SOR_bi_zeroHeadTaken;
    volatile atomicord32            m_SOR_bi_zeroTailTaken;
    volatile atomicord32            m_SOR_mi_zeroHeadTaken;
    volatile atomicord32            m_SOR_mi_zeroTailTaken;
    volatile atomicord32            m_SOR_reorderThreadsRemaining;
    volatile atomicord32            m_cf_4b;
    volatile atomicord32            m_ji_4b;
};


static int dxQuickStepIsland_Stage4a_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_iMJ_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_iMJSync_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_fcStart_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_fc_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
#ifdef WARM_STARTING
static int dxQuickStepIsland_Stage4LCP_fcWarmComplete_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
#endif
static int dxQuickStepIsland_Stage4LCP_Ad_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_ReorderPrep_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_IterationStart_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_ConstraintsReordering_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_ConstraintsReorderingSync_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_Iteration_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4LCP_IterationSync_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage4b_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage5_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);

static void dxQuickStepIsland_Stage4a(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_iMJComputation(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_MTfcComputation(dxQuickStepperStage4CallContext *stage4CallContext, dCallReleaseeID callThisReleasee);
#ifdef WARM_STARTING
static void dxQuickStepIsland_Stage4LCP_MTfcComputation_warm(dxQuickStepperStage4CallContext *stage4CallContext, dCallReleaseeID callThisReleasee);
static void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmZeroArrays(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmPrepare(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmComplete(dxQuickStepperStage4CallContext *stage4CallContext);
#endif
static void dxQuickStepIsland_Stage4LCP_MTfcComputation_cold(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_STfcComputation(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_AdComputation(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_ReorderPrep(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_ConstraintsReordering(dxQuickStepperStage4CallContext *stage4CallContext);
static bool dxQuickStepIsland_Stage4LCP_ConstraintsShuffling(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int iteration);
static void dxQuickStepIsland_Stage4LCP_LinksArraysZeroing(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_DependencyMapForNewOrderRebuilding(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_DependencyMapFromSavedLevelsReconstruction(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_MTIteration(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int initiallyKnownToBeCompletedLevel);
static void dxQuickStepIsland_Stage4LCP_STIteration(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage4LCP_IterationStep(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int i);
static void dxQuickStepIsland_Stage4b(dxQuickStepperStage4CallContext *stage4CallContext);
static void dxQuickStepIsland_Stage5(dxQuickStepperStage5CallContext *stage5CallContext);


struct dxQuickStepperStage6CallContext
{
    void Initialize(const dxStepperProcessingCallContext *callContext, const dxQuickStepperLocalContext *localContext)
    {
        m_stepperCallContext = callContext;
        m_localContext = localContext;
        m_bi_6a = 0;
        m_bi_6b = 0;
    }

    const dxStepperProcessingCallContext *m_stepperCallContext;
    const dxQuickStepperLocalContext *m_localContext;
    volatile atomicord32            m_bi_6a;
    volatile atomicord32            m_bi_6b;
};

static int dxQuickStepIsland_Stage6a_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage6aSync_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
static int dxQuickStepIsland_Stage6b_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);

static void dxQuickStepIsland_Stage6a(dxQuickStepperStage6CallContext *stage6CallContext);
static void dxQuickStepIsland_Stage6_VelocityCheck(dxQuickStepperStage6CallContext *stage6CallContext);
static void dxQuickStepIsland_Stage6b(dxQuickStepperStage6CallContext *stage6CallContext);

//***************************************************************************
// various common computations involving the matrix J

// compute iMJ = inv(M)*J'

template<unsigned int step_size>
void compute_invM_JT (volatile atomicord32 *mi_storage, dReal *iMJ, 
    unsigned int m, const dReal *J, const dxJBodiesItem *jb,
    dxBody * const *body, const dReal *invI)
{
    unsigned int m_steps = (m + (step_size - 1)) / step_size;

    unsigned mi_step;
    while ((mi_step = ThrsafeIncrementIntUpToLimit(mi_storage, m_steps)) != m_steps) {
        unsigned int mi = mi_step * step_size;
        const unsigned int miend = mi + dMIN(step_size, m - mi);

        dReal *iMJ_ptr = iMJ + (size_t)mi * IMJ__MAX;
        const dReal *J_ptr = J + (size_t)mi * JME__MAX;
        while (true) {
            int b1 = jb[mi].first;
            int b2 = jb[mi].second;

            dReal k1 = body[(unsigned)b1]->invMass;
            for (unsigned int j = 0; j != JVE__L_COUNT; j++) iMJ_ptr[IMJ__1L_MIN + j] = k1 * J_ptr[JME__J1L_MIN + j];
            const dReal *invIrow1 = invI + (size_t)(unsigned)b1 * IIE__MAX + IIE__MATRIX_MIN;
            dMultiply0_331 (iMJ_ptr + IMJ__1A_MIN, invIrow1, J_ptr + JME__J1A_MIN);

            if (b2 != -1) {
                dReal k2 = body[(unsigned)b2]->invMass;
                for (unsigned int j = 0; j != JVE__L_COUNT; ++j) iMJ_ptr[IMJ__2L_MIN + j] = k2 * J_ptr[JME__J2L_MIN + j];
                const dReal *invIrow2 = invI + (size_t)(unsigned)b2 * IIE__MAX + IIE__MATRIX_MIN;
                dMultiply0_331 (iMJ_ptr + IMJ__2A_MIN, invIrow2, J_ptr + JME__J2A_MIN);
            }
        
            if (++mi == miend) {
                break;
            }
            iMJ_ptr += IMJ__MAX;
            J_ptr += JME__MAX;
        }
    }
}

#ifdef WARM_STARTING

static 
void multiply_invM_JT_init_array(unsigned int nb, atomicord32 *bi_links/*=[nb]*/)
{
    // const unsigned businessIndex_none = dxENCODE_INDEX(-1);
    // for (unsigned int bi = 0; bi != nb; ++bi) {
    //     bi_links[bi] = businessIndex_none;
    // }
    memset(bi_links, 0, nb * sizeof(bi_links[0]));
}

// compute out = inv(M)*J'*in.
template<unsigned int step_size>
void multiply_invM_JT_prepare(volatile atomicord32 *mi_storage, 
    unsigned int m, const dxJBodiesItem *jb, atomicord32 *bi_links/*=[nb]*/, atomicord32 *mi_links/*=[2*m]*/)
{
    unsigned int m_steps = (m + (step_size - 1)) / step_size;

    unsigned mi_step;
    while ((mi_step = ThrsafeIncrementIntUpToLimit(mi_storage, m_steps)) != m_steps) {
        unsigned int mi = mi_step * step_size;
        const unsigned int miend = mi + dMIN(step_size, m - mi);

        while (true) {
            int b1 = jb[mi].first;
            int b2 = jb[mi].second;

            const unsigned encoded_mi = dxENCODE_INDEX(mi);
            unsigned oldIndex_b1 = ThrsafeExchange(&bi_links[b1], encoded_mi);
            mi_links[(size_t)mi * 2] = oldIndex_b1;

            if (b2 != -1) {
                unsigned oldIndex_b2 = ThrsafeExchange(&bi_links[b2], encoded_mi);
                mi_links[(size_t)mi * 2 + 1] = oldIndex_b2;
            }

            if (++mi == miend) {
                break;
            }
        }
    }
}

template<unsigned int step_size, unsigned int out_offset, unsigned int out_stride>
void multiply_invM_JT_complete(volatile atomicord32 *bi_storage, dReal *out, 
    unsigned int nb, const dReal *iMJ, const dxJBodiesItem *jb, const dReal *in, 
    atomicord32 *bi_links/*=[nb]*/, atomicord32 *mi_links/*=[2*m]*/)
{
    const unsigned businessIndex_none = dxENCODE_INDEX(-1);

    unsigned int nb_steps = (nb + (step_size - 1)) / step_size;

    unsigned bi_step;
    while ((bi_step = ThrsafeIncrementIntUpToLimit(bi_storage, nb_steps)) != nb_steps) {
        unsigned int bi = bi_step * step_size;
        const unsigned int biend = bi + dMIN(step_size, nb - bi);

        dReal *out_ptr = out + (size_t)bi * out_stride + out_offset;
        while (true) {
            dReal psum0 = REAL(0.0), psum1 = REAL(0.0), psum2 = REAL(0.0), psum3 = REAL(0.0), psum4 = REAL(0.0), psum5 = REAL(0.0);

            unsigned businessIndex = bi_links[bi];
            while (businessIndex != businessIndex_none) {
                unsigned int mi = dxDECODE_INDEX(businessIndex);
                const dReal *iMJ_ptr;
                
                if (bi == jb[mi].first) {
                    iMJ_ptr = iMJ + (size_t)mi * IMJ__MAX + IMJ__1_MIN;
                    businessIndex = mi_links[(size_t)mi * 2];
                }
                else {
                    dIASSERT(bi == jb[mi].second);

                    iMJ_ptr = iMJ + (size_t)mi * IMJ__MAX + IMJ__2_MIN;
                    businessIndex = mi_links[(size_t)mi * 2 + 1];
                }

                const dReal in_i = in[mi];
                psum0 += in_i * iMJ_ptr[JVE_LX]; psum1 += in_i * iMJ_ptr[JVE_LY]; psum2 += in_i * iMJ_ptr[JVE_LZ];
                psum3 += in_i * iMJ_ptr[JVE_AX]; psum4 += in_i * iMJ_ptr[JVE_AY]; psum5 += in_i * iMJ_ptr[JVE_AZ];
            }

            out_ptr[dDA_LX] = psum0; out_ptr[dDA_LY] = psum1; out_ptr[dDA_LZ] = psum2; 
            out_ptr[dDA_AX] = psum3; out_ptr[dDA_AY] = psum4; out_ptr[dDA_AZ] = psum5;
         
            if (++bi == biend) {
                break;
            }
            out_ptr += out_stride;
        }
    }
}

template<unsigned int out_offset, unsigned int out_stride>
void _multiply_invM_JT (dReal *out, 
    unsigned int m, unsigned int nb, dReal *iMJ, const dxJBodiesItem *jb, const dReal *in)
{
    dSetZero (out, (size_t)nb * out_stride);
    const dReal *iMJ_ptr = iMJ;
    for (unsigned int i=0; i<m; i++) {
        int b1 = jb[i].first;
        int b2 = jb[i].second;
        const dReal in_i = in[i];

        dReal *out_ptr = out + (size_t)(unsigned)b1 * out_stride + out_offset;
        for (unsigned int j = JVE__MIN; j != JVE__MAX; j++) out_ptr[j - JVE__MIN] += iMJ_ptr[IMJ__1_MIN + j] * in_i;
        dSASSERT(out_stride - out_offset >= JVE__MAX);
        dSASSERT(JVE__MAX == (int)dDA__MAX);

        if (b2 != -1) {
            out_ptr = out + (size_t)(unsigned)b2 * out_stride + out_offset;
            for (unsigned int j = JVE__MIN; j != JVE__MAX; j++) out_ptr[j - JVE__MIN] += iMJ_ptr[IMJ__2_MIN + j] * in_i;
            dSASSERT(out_stride - out_offset >= JVE__MAX);
            dSASSERT(JVE__MAX == (int)dDA__MAX);
        }

        iMJ_ptr += IMJ__MAX;
    }
}
#endif

// compute out = J*in.
template<unsigned int step_size, unsigned int in_offset, unsigned int in_stride>
void multiplyAdd_J (volatile atomicord32 *mi_storage, 
    unsigned int m, dReal *J, const dxJBodiesItem *jb, const dReal *in)
{
    unsigned int m_steps = (m + (step_size - 1)) / step_size;

    unsigned mi_step;
    while ((mi_step = ThrsafeIncrementIntUpToLimit(mi_storage, m_steps)) != m_steps) {
        unsigned int mi = mi_step * step_size;
        const unsigned int miend = mi + dMIN(step_size, m - mi);

        dReal *J_ptr = J + (size_t)mi * JME__MAX;
        while (true) {
            int b1 = jb[mi].first;
            int b2 = jb[mi].second;
            dReal sum = REAL(0.0);
            const dReal *in_ptr = in + (size_t)(unsigned)b1 * in_stride + in_offset;
            for (unsigned int j = 0; j != JME__J1_COUNT; ++j) sum += J_ptr[j + JME__J1_MIN] * in_ptr[j];
            dSASSERT(in_offset + JME__J1_COUNT <= in_stride);

            if (b2 != -1) {
                in_ptr = in + (size_t)(unsigned)b2 * in_stride + in_offset;
                for (unsigned int j = 0; j != JME__J2_COUNT; ++j) sum += J_ptr[j + JME__J2_MIN] * in_ptr[j];
                dSASSERT(in_offset + JME__J2_COUNT <= in_stride);
            }
            J_ptr[JME_RHS] += sum;

            if (++mi == miend) {
                break;
            }
            J_ptr += JME__MAX;
        }
    }
}


struct IndexError {
#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR
    dReal error;		// error to sort on
#endif
    int index;		// row index
};


#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR

static int compare_index_error (const void *a, const void *b)
{
    const IndexError *i1 = (IndexError*) a;
    const IndexError *i2 = (IndexError*) b;
    if (i1->error < i2->error) return -1;
    if (i1->error > i2->error) return 1;
    return 0;
}

#endif // #if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR

static inline 
bool IsSORConstraintsReorderRequiredForIteration(unsigned iteration)
{
    bool result = false;
#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR
    result = true;
#elif CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__RANDOMLY
    if ((iteration & 7) == 0) {
        result = true;
    }
#else  // #if CONSTRAINTS_REORDERING_METHOD != REORDERING_METHOD__BY_ERROR && CONSTRAINTS_REORDERING_METHOD != REORDERING_METHOD__RANDOMLY
    if (iteration == 0) {
        result = true;
    }
#endif
    return result;
}

/*extern */
void dxQuickStepIsland(const dxStepperProcessingCallContext *callContext)
{
    dxWorldProcessMemArena *memarena = callContext->m_stepperArena;
    unsigned int nb = callContext->m_islandBodiesCount;
    unsigned int _nj = callContext->m_islandJointsCount;

    dReal *invI = memarena->AllocateOveralignedArray<dReal>((size_t)nb * IIE__MAX, INVI_ALIGNMENT);
    dJointWithInfo1 *const jointinfos = memarena->AllocateArray<dJointWithInfo1>(_nj);

    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
    dIASSERT(allowedThreads != 0);

    void *stagesMemArenaState = memarena->SaveState();

    dxQuickStepperStage1CallContext *stage1CallContext = (dxQuickStepperStage1CallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage1CallContext));
    stage1CallContext->Initialize(callContext, stagesMemArenaState, invI, jointinfos);

    dxQuickStepperStage0BodiesCallContext *stage0BodiesCallContext = (dxQuickStepperStage0BodiesCallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage0BodiesCallContext));
    stage0BodiesCallContext->Initialize(callContext, invI);

    dxQuickStepperStage0JointsCallContext *stage0JointsCallContext = (dxQuickStepperStage0JointsCallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage0JointsCallContext));
    stage0JointsCallContext->Initialize(callContext, jointinfos, &stage1CallContext->m_stage0Outputs);

    if (allowedThreads == 1)
    {
        IFTIMING(dTimerStart("preprocessing"));
        dxQuickStepIsland_Stage0_Bodies(stage0BodiesCallContext);
        dxQuickStepIsland_Stage0_Joints(stage0JointsCallContext);
        dxQuickStepIsland_Stage1(stage1CallContext);
    }
    else
    {
        unsigned bodyThreads = CalculateOptimalThreadsCount<1U>(nb, allowedThreads);
        unsigned jointThreads = 1;

        dxWorld *world = callContext->m_world;

        dCallReleaseeID stage1CallReleasee;
        world->PostThreadedCallForUnawareReleasee(NULL, &stage1CallReleasee, bodyThreads + jointThreads, callContext->m_finalReleasee, 
            NULL, &dxQuickStepIsland_Stage1_Callback, stage1CallContext, 0, "QuickStepIsland Stage1");

        // It is preferable to post single threaded task first to be started sooner
        world->PostThreadedCall(NULL, NULL, 0, stage1CallReleasee, NULL, &dxQuickStepIsland_Stage0_Joints_Callback, stage0JointsCallContext, 0, "QuickStepIsland Stage0-Joints");
        dIASSERT(jointThreads == 1);

        if (bodyThreads > 1) {
            world->PostThreadedCallsGroup(NULL, bodyThreads - 1, stage1CallReleasee, &dxQuickStepIsland_Stage0_Bodies_Callback, stage0BodiesCallContext, "QuickStepIsland Stage0-Bodies");
        }
        dxQuickStepIsland_Stage0_Bodies(stage0BodiesCallContext);
        world->AlterThreadedCallDependenciesCount(stage1CallReleasee, -1);
    }
}    

static 
int dxQuickStepIsland_Stage0_Bodies_Callback(void *_callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage0BodiesCallContext *callContext = (dxQuickStepperStage0BodiesCallContext *)_callContext;
    dxQuickStepIsland_Stage0_Bodies(callContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage0_Bodies(dxQuickStepperStage0BodiesCallContext *callContext)
{
    dxBody * const *body = callContext->m_stepperCallContext->m_islandBodiesStart;
    unsigned int nb = callContext->m_stepperCallContext->m_islandBodiesCount;

    if (ThrsafeExchange(&callContext->m_tagsTaken, 1) == 0)
    {
        // number all bodies in the body list - set their tag values
        for (unsigned int i=0; i<nb; i++) body[i]->tag = i;
    }

    if (ThrsafeExchange(&callContext->m_gravityTaken, 1) == 0)
    {
        dxWorld *world = callContext->m_stepperCallContext->m_world;

        // add the gravity force to all bodies
        // since gravity does normally have only one component it's more efficient
        // to run three loops for each individual component
        dxBody *const *const bodyend = body + nb;
        dReal gravity_x = world->gravity[0];
        if (gravity_x) {
            for (dxBody *const *bodycurr = body; bodycurr != bodyend; bodycurr++) {
                dxBody *b = *bodycurr;
                if ((b->flags & dxBodyNoGravity) == 0) {
                    b->facc[0] += b->mass.mass * gravity_x;
                }
            }
        }
        dReal gravity_y = world->gravity[1];
        if (gravity_y) {
            for (dxBody *const *bodycurr = body; bodycurr != bodyend; bodycurr++) {
                dxBody *b = *bodycurr;
                if ((b->flags & dxBodyNoGravity) == 0) {
                    b->facc[1] += b->mass.mass * gravity_y;
                }
            }
        }
        dReal gravity_z = world->gravity[2];
        if (gravity_z) {
            for (dxBody *const *bodycurr = body; bodycurr != bodyend; bodycurr++) {
                dxBody *b = *bodycurr;
                if ((b->flags & dxBodyNoGravity) == 0) {
                    b->facc[2] += b->mass.mass * gravity_z;
                }
            }
        }
    }

    // for all bodies, compute the inertia tensor and its inverse in the global
    // frame, and compute the rotational force and add it to the torque
    // accumulator. I and invI are a vertical stack of 3x4 matrices, one per body.
    {
        dReal *invI = callContext->m_invI;
        unsigned int bodyIndex;
        while ((bodyIndex = ThrsafeIncrementIntUpToLimit(&callContext->m_inertiaBodyIndex, nb)) != nb) {
            dReal *invIrow = invI + (size_t)bodyIndex * IIE__MAX;
            dxBody *b = body[bodyIndex];

            dMatrix3 tmp;
            // compute inverse inertia tensor in global frame
            dMultiply2_333 (tmp, b->invI, b->posr.R);
            dMultiply0_333 (invIrow + IIE__MATRIX_MIN, b->posr.R, tmp);

            // Don't apply gyroscopic torques to bodies
            // if not flagged or the body is kinematic
            if ((b->flags & dxBodyGyroscopic) && (b->invMass > 0)) {
                dMatrix3 I;
                // compute inertia tensor in global frame
                dMultiply2_333 (tmp, b->mass.I, b->posr.R);
                dMultiply0_333 (I, b->posr.R, tmp);
                // compute rotational force
#if 0
                // Explicit computation
                dMultiply0_331 (tmp, I, b->avel);
                dSubtractVectorCross3(b->tacc, b->avel, tmp);
#else
                // Do the implicit computation based on 
                //"Stabilizing Gyroscopic Forces in Rigid Multibody Simulations"
                // (Lacoursière 2006)
                dReal h = callContext->m_stepperCallContext->m_stepSize; // Step size
                dVector3 L; // Compute angular momentum
                dMultiply0_331(L, I, b->avel);
                
                // Compute a new effective 'inertia tensor'
                // for the implicit step: the cross-product 
                // matrix of the angular momentum plus the
                // old tensor scaled by the timestep.  
                // Itild may not be symmetric pos-definite, 
                // but we can still use it to compute implicit
                // gyroscopic torques.
                dMatrix3 Itild = { 0 };  
                dSetCrossMatrixMinus(Itild, L, 4);
                for (int ii = dM3E__MIN; ii < dM3E__MAX; ++ii) {
                    Itild[ii] = Itild[ii] * h + I[ii];
                }

                // Scale momentum by inverse time to get 
                // a sort of "torque"
                dScaleVector3(L, dRecip(h)); 
                // Invert the pseudo-tensor
                dMatrix3 itInv;
                // This is a closed-form inversion.
                // It's probably not numerically stable
                // when dealing with small masses with
                // a large asymmetry.
                // An LU decomposition might be better.
                if (dInvertMatrix3(itInv, Itild) != 0) {
                    // "Divide" the original tensor
                    // by the pseudo-tensor (on the right)
                    dMultiply0_333(Itild, I, itInv);
                    // Subtract an identity matrix
                    Itild[dM3E_XX] -= 1; Itild[dM3E_YY] -= 1; Itild[dM3E_ZZ] -= 1;

                    // This new inertia matrix rotates the 
                    // momentum to get a new set of torques
                    // that will work correctly when applied
                    // to the old inertia matrix as explicit
                    // torques with a semi-implicit update
                    // step.
                    dVector3 tau0;
                    dMultiply0_331(tau0, Itild, L);
                    
                    // Add the gyro torques to the torque 
                    // accumulator
                    dAddVectors3(b->tacc, b->tacc, tau0);
                }
#endif
            }
        }
    }
}

static 
int dxQuickStepIsland_Stage0_Joints_Callback(void *_callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage0JointsCallContext *callContext = (dxQuickStepperStage0JointsCallContext *)_callContext;
    dxQuickStepIsland_Stage0_Joints(callContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage0_Joints(dxQuickStepperStage0JointsCallContext *callContext)
{
    dxJoint * const *_joint = callContext->m_stepperCallContext->m_islandJointsStart;
    unsigned int _nj = callContext->m_stepperCallContext->m_islandJointsCount;

    // get joint information (m = total constraint dimension, nub = number of unbounded variables).
    // joints with m=0 are inactive and are removed from the joints array
    // entirely, so that the code that follows does not consider them.
    {
        unsigned int mcurr = 0, mfbcurr = 0;
        dJointWithInfo1 *jicurr = callContext->m_jointinfos;
        dxJoint *const *const _jend = _joint + _nj;
        for (dxJoint *const *_jcurr = _joint; _jcurr != _jend; _jcurr++) {	// jicurr=dest, _jcurr=src
            dxJoint *j = *_jcurr;
            j->getInfo1 (&jicurr->info);
            dIASSERT (/*jicurr->info.m >= 0 && */jicurr->info.m <= 6 && /*jicurr->info.nub >= 0 && */jicurr->info.nub <= jicurr->info.m);

            unsigned int jm = jicurr->info.m;
            if (jm != 0) {
                mcurr += jm;
                if (j->feedback != NULL) {
                    mfbcurr += jm;
                }
                jicurr->joint = j;
                jicurr++;
            }
        }
        callContext->m_stage0Outputs->m = mcurr;
        callContext->m_stage0Outputs->mfb = mfbcurr;
        callContext->m_stage0Outputs->nj = (unsigned int)(jicurr - callContext->m_jointinfos); 
        dIASSERT((size_t)(jicurr - callContext->m_jointinfos) < UINT_MAX || (size_t)(jicurr - callContext->m_jointinfos) == UINT_MAX); // to avoid "...always evaluates to true" warnings
    }
}

static 
int dxQuickStepIsland_Stage1_Callback(void *_stage1CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage1CallContext *stage1CallContext = (dxQuickStepperStage1CallContext *)_stage1CallContext;
    dxQuickStepIsland_Stage1(stage1CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage1(dxQuickStepperStage1CallContext *stage1CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage1CallContext->m_stepperCallContext;
    dReal *invI = stage1CallContext->m_invI;
    dJointWithInfo1 *jointinfos = stage1CallContext->m_jointinfos;
    unsigned int nj = stage1CallContext->m_stage0Outputs.nj;
    unsigned int m = stage1CallContext->m_stage0Outputs.m;
    unsigned int mfb = stage1CallContext->m_stage0Outputs.mfb;

    dxWorldProcessMemArena *memarena = callContext->m_stepperArena;
    memarena->RestoreState(stage1CallContext->m_stageMemArenaState);
    stage1CallContext = NULL; // WARNING! _stage1CallContext is not valid after this point!
    dIVERIFY(stage1CallContext == NULL); // To suppress unused variable assignment warnings

    {
        unsigned int _nj = callContext->m_islandJointsCount;
        memarena->ShrinkArray<dJointWithInfo1>(jointinfos, _nj, nj);
    }

    dxMIndexItem *mindex = NULL;
    dxJBodiesItem *jb = NULL;
    int *findex = NULL;
    dReal *J = NULL, *Jcopy = NULL;

    // if there are constraints, compute the constraint force
    if (m > 0) {
        mindex = memarena->AllocateArray<dxMIndexItem>(nj + 1);
        {
            dxMIndexItem *mcurr = mindex;
            unsigned int moffs = 0, mfboffs = 0;
            mcurr->mIndex = moffs;
            mcurr->fbIndex = mfboffs;
            ++mcurr;

            const dJointWithInfo1 *const jiend = jointinfos + nj;
            for (const dJointWithInfo1 *jicurr = jointinfos; jicurr != jiend; ++jicurr) {
                dxJoint *joint = jicurr->joint;
                moffs += jicurr->info.m;
                if (joint->feedback) { mfboffs += jicurr->info.m; }
                mcurr->mIndex = moffs;
                mcurr->fbIndex = mfboffs;
                ++mcurr;
            }
        }

        jb = memarena->AllocateArray<dxJBodiesItem>(m);
        findex = memarena->AllocateArray<int>(m);
        J = memarena->AllocateOveralignedArray<dReal>((size_t)m * JME__MAX, JACOBIAN_ALIGNMENT);
        Jcopy = memarena->AllocateOveralignedArray<dReal>((size_t)mfb * JCE__MAX, JCOPY_ALIGNMENT);
    }

    dxQuickStepperLocalContext *localContext = (dxQuickStepperLocalContext *)memarena->AllocateBlock(sizeof(dxQuickStepperLocalContext));
    localContext->Initialize(invI, jointinfos, nj, m, mfb, mindex, jb, findex, J, Jcopy);

    void *stage1MemarenaState = memarena->SaveState();
    dxQuickStepperStage3CallContext *stage3CallContext = (dxQuickStepperStage3CallContext*)memarena->AllocateBlock(sizeof(dxQuickStepperStage3CallContext));
    stage3CallContext->Initialize(callContext, localContext, stage1MemarenaState);

    if (m > 0) {
        unsigned int nb = callContext->m_islandBodiesCount;
        // create a constraint equation right hand side vector `rhs', a constraint
        // force mixing vector `cfm', and LCP low and high bound vectors, and an
        // 'findex' vector.
        dReal *rhs_tmp = memarena->AllocateArray<dReal>((size_t)nb * RHS__MAX);

        dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext*)memarena->AllocateBlock(sizeof(dxQuickStepperStage2CallContext));
        stage2CallContext->Initialize(callContext, localContext, rhs_tmp);

        const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
        dIASSERT(allowedThreads != 0);

        if (allowedThreads == 1)
        {
            IFTIMING (dTimerNow ("create J"));
            dxQuickStepIsland_Stage2a(stage2CallContext);
            IFTIMING (dTimerNow ("compute rhs_tmp"));
            dxQuickStepIsland_Stage2b(stage2CallContext);
            dxQuickStepIsland_Stage2c(stage2CallContext);
            dxQuickStepIsland_Stage3(stage3CallContext);
        }
        else
        {
            dxWorld *world = callContext->m_world;
            
            dCallReleaseeID stage3CallReleasee;
            world->PostThreadedCallForUnawareReleasee(NULL, &stage3CallReleasee, 1, callContext->m_finalReleasee, 
                NULL, &dxQuickStepIsland_Stage3_Callback, stage3CallContext, 0, "QuickStepIsland Stage3");

            dCallReleaseeID stage2bSyncReleasee;
            world->PostThreadedCall(NULL, &stage2bSyncReleasee, 1, stage3CallReleasee, 
                NULL, &dxQuickStepIsland_Stage2bSync_Callback, stage2CallContext, 0, "QuickStepIsland Stage2b Sync");

            unsigned stage2a_allowedThreads = CalculateOptimalThreadsCount<1U>(nj, allowedThreads);

            dCallReleaseeID stage2aSyncReleasee;
            world->PostThreadedCall(NULL, &stage2aSyncReleasee, stage2a_allowedThreads, stage2bSyncReleasee, 
                NULL, &dxQuickStepIsland_Stage2aSync_Callback, stage2CallContext, 0, "QuickStepIsland Stage2a Sync");

            if (stage2a_allowedThreads > 1) {
                world->PostThreadedCallsGroup(NULL, stage2a_allowedThreads - 1, stage2aSyncReleasee, &dxQuickStepIsland_Stage2a_Callback, stage2CallContext, "QuickStepIsland Stage2a");
            }
            dxQuickStepIsland_Stage2a(stage2CallContext);
            world->AlterThreadedCallDependenciesCount(stage2aSyncReleasee, -1);
        }
    }
    else {
        dxQuickStepIsland_Stage3(stage3CallContext);
    }
}


static 
int dxQuickStepIsland_Stage2a_Callback(void *_stage2CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext *)_stage2CallContext;
    dxQuickStepIsland_Stage2a(stage2CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage2a(dxQuickStepperStage2CallContext *stage2CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage2CallContext->m_stepperCallContext;
    dxQuickStepperLocalContext *localContext = stage2CallContext->m_localContext;
    dJointWithInfo1 *jointinfos = localContext->m_jointinfos;
    unsigned int nj = localContext->m_nj;
    const dxMIndexItem *mindex = localContext->m_mindex;

    const dReal stepsizeRecip = dRecip(callContext->m_stepSize);
    {
        int *findex = localContext->m_findex;
        dReal *J = localContext->m_J;
        dReal *JCopy = localContext->m_Jcopy;

        // get jacobian data from constraints. an m*16 matrix will be created
        // to store the two jacobian blocks from each constraint. it has this
        // format:
        //
        //   l1 l1 l1 a1 a1 a1 rhs cfm l2 l2 l2 a2 a2 a2 lo hi \    .
        //   l1 l1 l1 a1 a1 a1 rhs cfm l2 l2 l2 a2 a2 a2 lo hi  }-- jacobian for joint 0, body 1 and body 2 (3 rows)
        //   l1 l1 l1 a1 a1 a1 rhs cfm l2 l2 l2 a2 a2 a2 lo hi /
        //   l1 l1 l1 a1 a1 a1 rhs cfm l2 l2 l2 a2 a2 a2 lo hi }--- jacobian for joint 1, body 1 and body 2 (3 rows)
        //   etc...
        //
        //   (lll) = linear jacobian data
        //   (aaa) = angular jacobian data
        //
        dxWorld *world = callContext->m_world;
        const dReal worldERP = world->global_erp;
        const dReal worldCFM = world->global_cfm;

        unsigned validFIndices = 0;

        unsigned ji;
        while ((ji = ThrsafeIncrementIntUpToLimit(&stage2CallContext->m_ji_J, nj)) != nj) {
            const unsigned ofsi = mindex[ji].mIndex;
            const unsigned int infom = mindex[ji + 1].mIndex - ofsi;

            dReal *const JRow = J + (size_t)ofsi * JME__MAX;
            {
                dReal *const JEnd = JRow + infom * JME__MAX;
                for (dReal *JCurr = JRow; JCurr != JEnd; JCurr += JME__MAX) {
                    dSetZero(JCurr + JME__J1_MIN, JME__J1_COUNT);
                    JCurr[JME_RHS] = REAL(0.0);
                    JCurr[JME_CFM] = worldCFM;
                    dSetZero(JCurr + JME__J2_MIN, JME__J2_COUNT);
                    JCurr[JME_LO] = -dInfinity;
                    JCurr[JME_HI] = dInfinity;
                    dSASSERT(JME__J1_COUNT + 2 + JME__J2_COUNT + 2 == JME__MAX);
                }
            }
            int *findexRow = findex + ofsi;
            dSetValue(findexRow, infom, -1);
            
            dxJoint *joint = jointinfos[ji].joint;
            joint->getInfo2(stepsizeRecip, worldERP, JME__MAX, JRow + JME__J1_MIN, JRow + JME__J2_MIN, JME__MAX, JRow + JME__RHS_CFM_MIN, JRow + JME__LO_HI_MIN, findexRow);

            // findex iteration is compact and is not going to pollute caches - do it first
            {
                // adjust returned findex values for global index numbering
                int *const findicesEnd = findexRow + infom;
                for (int *findexCurr = findexRow; findexCurr != findicesEnd; ++findexCurr) {
                    int fival = *findexCurr;
                    if (fival != -1) {
                        *findexCurr = fival + ofsi;
                        ++validFIndices;
                    }
                }
            }
            {
                dReal *const JEnd = JRow + infom * JME__MAX;
                for (dReal *JCurr = JRow; JCurr != JEnd; JCurr += JME__MAX) {
                    JCurr[JME_RHS] *= stepsizeRecip;
                    JCurr[JME_CFM] *= stepsizeRecip;
                }
            }
            {
                // we need a copy of Jacobian for joint feedbacks
                // because it gets destroyed by SOR solver
                // instead of saving all Jacobian, we can save just rows
                // for joints, that requested feedback (which is normally much less)
                unsigned mfbCount = mindex[ji + 1].fbIndex - mindex[ji].fbIndex;
                if (mfbCount != 0) {
                    dReal *const JEnd = JRow + (size_t)mfbCount * JME__MAX;
                    for (const dReal *JCurr = JRow; ; ) {
                        for (unsigned i = 0; i != JME__J1_COUNT; ++i) { JCopy[i + JCE__J1_MIN] = JCurr[i + JME__J1_MIN]; }
                        for (unsigned j = 0; j != JME__J2_COUNT; ++j) { JCopy[j + JCE__J2_MIN] = JCurr[j + JME__J2_MIN]; }
                        JCopy += JCE__MAX;
                        dSASSERT((unsigned)JCE__J1_COUNT == JME__J1_COUNT);
                        dSASSERT((unsigned)JCE__J2_COUNT == JME__J2_COUNT);
                        dSASSERT(JCE__J1_COUNT + JCE__J2_COUNT == JCE__MAX);
                        
                        if ((JCurr += JME__MAX) == JEnd) {
                            break;
                        }
                    }
                }
            }
        }

        if (validFIndices != 0) {
            ThrsafeAdd(&localContext->m_valid_findices, validFIndices);
        }
    }

    {
        dxJBodiesItem *jb = localContext->m_jb;

        // create an array of body numbers for each joint row
        unsigned ji;
        while ((ji = ThrsafeIncrementIntUpToLimit(&stage2CallContext->m_ji_jb, nj)) != nj) {
            dxJoint *joint = jointinfos[ji].joint;
            int b1 = (joint->node[0].body) ? (joint->node[0].body->tag) : -1;
            int b2 = (joint->node[1].body) ? (joint->node[1].body->tag) : -1;

            dxJBodiesItem *const jb_end = jb + mindex[ji + 1].mIndex;
            dxJBodiesItem *jb_ptr = jb + mindex[ji].mIndex;
            for (; jb_ptr != jb_end; ++jb_ptr) {
                jb_ptr->first = b1;
                jb_ptr->second = b2;
            }
        }
    }
}

static 
int dxQuickStepIsland_Stage2aSync_Callback(void *_stage2CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext *)_stage2CallContext;
    const dxStepperProcessingCallContext *callContext = stage2CallContext->m_stepperCallContext;
    const unsigned int nb = callContext->m_islandBodiesCount;

    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
    unsigned int stage2b_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE2B_STEP>(nb, allowedThreads);

    if (stage2b_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage2b_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage2b_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage2b_Callback, stage2CallContext, "QuickStepIsland Stage2b");
    }
    dxQuickStepIsland_Stage2b(stage2CallContext);

    return 1;
}

static 
int dxQuickStepIsland_Stage2b_Callback(void *_stage2CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext *)_stage2CallContext;
    dxQuickStepIsland_Stage2b(stage2CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage2b(dxQuickStepperStage2CallContext *stage2CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage2CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage2CallContext->m_localContext;

    const dReal stepsizeRecip = dRecip(callContext->m_stepSize);
    {
        // Warning!!!
        // This code reads facc/tacc fields of body objects which (the fields)
        // may be modified by dxJoint::getInfo2(). Therefore the code must be
        // in different sub-stage from Jacobian construction in Stage2a 
        // to ensure proper synchronization and avoid accessing numbers being modified.
        // Warning!!!
        dxBody * const *const body = callContext->m_islandBodiesStart;
        const unsigned int nb = callContext->m_islandBodiesCount;
        const dReal *invI = localContext->m_invI;
        dReal *rhs_tmp = stage2CallContext->m_rhs_tmp;

        // compute the right hand side `rhs'

        const unsigned int step_size = dxQUICKSTEPISLAND_STAGE2B_STEP;
        unsigned int nb_steps = (nb + (step_size - 1)) / step_size;

        // put -(v/h + invM*fe) into rhs_tmp
        unsigned bi_step;
        while ((bi_step = ThrsafeIncrementIntUpToLimit(&stage2CallContext->m_bi, nb_steps)) != nb_steps) {
            unsigned int bi = bi_step * step_size;
            const unsigned int biend = bi + dMIN(step_size, nb - bi);

            dReal *rhscurr = rhs_tmp + (size_t)bi * RHS__MAX;
            const dReal *invIrow = invI + (size_t)bi * IIE__MAX;
            while (true) {
                dxBody *b = body[bi];
                dReal body_invMass = b->invMass;
                for (unsigned int j = dSA__MIN; j != dSA__MAX; ++j) rhscurr[RHS__L_MIN + j] = -(b->facc[dV3E__AXES_MIN + j] * body_invMass + b->lvel[dV3E__AXES_MIN + j] * stepsizeRecip);
                dMultiply0_331 (rhscurr + RHS__A_MIN, invIrow + IIE__MATRIX_MIN, b->tacc);
                for (unsigned int k = dSA__MIN; k != dSA__MAX; ++k) rhscurr[RHS__A_MIN + k] = -(b->avel[dV3E__AXES_MIN + k] * stepsizeRecip) - rhscurr[RHS__A_MIN + k];
                
                if (++bi == biend) {
                    break;
                }
                rhscurr += RHS__MAX;
                invIrow += IIE__MAX;
            }
        }
    }
}

static 
int dxQuickStepIsland_Stage2bSync_Callback(void *_stage2CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext *)_stage2CallContext;
    const dxStepperProcessingCallContext *callContext = stage2CallContext->m_stepperCallContext;
    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;

    const dxQuickStepperLocalContext *localContext = stage2CallContext->m_localContext;
    unsigned int m = localContext->m_m;

    unsigned int stage2c_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE2C_STEP>(m, allowedThreads);

    if (stage2c_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage2c_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage2c_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage2c_Callback, stage2CallContext, "QuickStepIsland Stage2c");
    }
    dxQuickStepIsland_Stage2c(stage2CallContext);

    return 1;
}


static 
int dxQuickStepIsland_Stage2c_Callback(void *_stage2CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage2CallContext *stage2CallContext = (dxQuickStepperStage2CallContext *)_stage2CallContext;
    dxQuickStepIsland_Stage2c(stage2CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage2c(dxQuickStepperStage2CallContext *stage2CallContext)
{
    //const dxStepperProcessingCallContext *callContext = stage2CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage2CallContext->m_localContext;

    //const dReal stepsizeRecip = dRecip(callContext->m_stepSize);
    {
        // Warning!!!
        // This code depends on rhs_tmp and therefore must be in different sub-stage 
        // from rhs_tmp calculation in Stage2b to ensure proper synchronization 
        // and avoid accessing numbers being modified.
        // Warning!!!
        dReal *J = localContext->m_J;
        const dxJBodiesItem *jb = localContext->m_jb;
        const dReal *rhs_tmp = stage2CallContext->m_rhs_tmp;
        const unsigned int m = localContext->m_m;

        // add J*rhs_tmp to rhs
        multiplyAdd_J<dxQUICKSTEPISLAND_STAGE2C_STEP, RHS__DYNAMICS_MIN, RHS__MAX>(&stage2CallContext->m_Jrhsi, m, J, jb, rhs_tmp);
    }
}


static 
int dxQuickStepIsland_Stage3_Callback(void *_stage3CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage3CallContext *stage3CallContext = (dxQuickStepperStage3CallContext *)_stage3CallContext;
    dxQuickStepIsland_Stage3(stage3CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage3(dxQuickStepperStage3CallContext *stage3CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage3CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage3CallContext->m_localContext;

    dxWorldProcessMemArena *memarena = callContext->m_stepperArena;
    memarena->RestoreState(stage3CallContext->m_stage1MemArenaState);
    stage3CallContext = NULL; // WARNING! stage3CallContext is not valid after this point!
    dIVERIFY(stage3CallContext == NULL); // To suppress unused variable assignment warnings

    void *stage3MemarenaState = memarena->SaveState();
    dxQuickStepperStage5CallContext *stage5CallContext = (dxQuickStepperStage5CallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage5CallContext));
    stage5CallContext->Initialize(callContext, localContext, stage3MemarenaState);

    unsigned int m = localContext->m_m;

    if (m > 0) {
        // load lambda from the value saved on the previous iteration
        dReal *lambda = memarena->AllocateArray<dReal>(m);

        unsigned int nb = callContext->m_islandBodiesCount;
        dReal *cforce = memarena->AllocateArray<dReal>((size_t)nb * CFE__MAX);
        dReal *iMJ = memarena->AllocateOveralignedArray<dReal>((size_t)m * IMJ__MAX, INVMJ_ALIGNMENT);
        // order to solve constraint rows in
        IndexError *order = memarena->AllocateArray<IndexError>(m);
        dReal *last_lambda = NULL;
#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR
        // the lambda computed at the previous iteration.
        // this is used to measure error for when we are reordering the indexes.
        last_lambda = memarena->AllocateArray<dReal>(m);
#endif

        const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
        bool singleThreadedExecution = allowedThreads == 1;
        dIASSERT(allowedThreads >= 1);

        atomicord32 *bi_links_or_mi_levels = NULL;
        atomicord32 *mi_links = NULL;
#if !dTHREADING_INTF_DISABLED
        bi_links_or_mi_levels = memarena->AllocateArray<atomicord32>(dMAX(nb, m));
        mi_links = memarena->AllocateArray<atomicord32>(2 * ((size_t)m + 1));
#else
        dIASSERT(singleThreadedExecution);
#endif
        dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage4CallContext));
        stage4CallContext->Initialize(callContext, localContext, lambda, cforce, iMJ, order, last_lambda, bi_links_or_mi_levels, mi_links);

        if (singleThreadedExecution)
        {
            dxQuickStepIsland_Stage4a(stage4CallContext);

            IFTIMING (dTimerNow ("solving LCP problem"));
            dxQuickStepIsland_Stage4LCP_iMJComputation(stage4CallContext);
            dxQuickStepIsland_Stage4LCP_STfcComputation(stage4CallContext);
            dxQuickStepIsland_Stage4LCP_AdComputation(stage4CallContext);
            dxQuickStepIsland_Stage4LCP_ReorderPrep(stage4CallContext);
            
            dxWorld *world = callContext->m_world;
            const unsigned int num_iterations = world->qs.num_iterations;
            for (unsigned int iteration=0; iteration < num_iterations; iteration++) {
                if (IsSORConstraintsReorderRequiredForIteration(iteration)) {
                    stage4CallContext->ResetSOR_ConstraintsReorderVariables(0);
                    dxQuickStepIsland_Stage4LCP_ConstraintsShuffling(stage4CallContext, iteration);
                }
                dxQuickStepIsland_Stage4LCP_STIteration(stage4CallContext);
            }

            dxQuickStepIsland_Stage4b(stage4CallContext);
            dxQuickStepIsland_Stage5(stage5CallContext);
        }
        else
        {
            dxWorld *world = callContext->m_world;

            dCallReleaseeID stage5CallReleasee;
            world->PostThreadedCallForUnawareReleasee(NULL, &stage5CallReleasee, 1, callContext->m_finalReleasee, 
                NULL, &dxQuickStepIsland_Stage5_Callback, stage5CallContext, 0, "QuickStepIsland Stage5");

            dCallReleaseeID stage4LCP_IterationSyncReleasee;
            world->PostThreadedCall(NULL, &stage4LCP_IterationSyncReleasee, 1, stage5CallReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_IterationSync_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_Iteration Sync");

            unsigned int stage4LCP_Iteration_allowedThreads = CalculateOptimalThreadsCount<1U>(m, allowedThreads);
            stage4CallContext->AssignLCP_IterationData(stage4LCP_IterationSyncReleasee, stage4LCP_Iteration_allowedThreads);

            dCallReleaseeID stage4LCP_IterationStartReleasee;
            world->PostThreadedCall(NULL, &stage4LCP_IterationStartReleasee, 3, stage4LCP_IterationSyncReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_IterationStart_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_Iteration Start");

            unsigned int nj = localContext->m_nj;
            unsigned int stage4a_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4A_STEP>(nj, allowedThreads);

            dCallReleaseeID stage4LCP_fcStartReleasee;
            // Note: It is unnecessary to make fc dependent on 4a if there is no WARM_STARTING
            // However I'm doing so to minimize use of preprocessor conditions in sources
            unsigned stage4LCP_fcDependenciesCountToUse = stage4a_allowedThreads;
#ifdef WARM_STARTING
            // Posted with extra dependency to be removed from dxQuickStepIsland_Stage4LCP_iMJSync_Callback
            stage4LCP_fcDependenciesCountToUse += 1;
#endif
            world->PostThreadedCall(NULL, &stage4LCP_fcStartReleasee, stage4LCP_fcDependenciesCountToUse, stage4LCP_IterationStartReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_fcStart_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_fc Start");
#ifdef WARM_STARTING
            stage4CallContext->AssignLCP_fcStartReleasee(stage4LCP_fcStartReleasee);
#endif

            unsigned stage4LCP_iMJ_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4LCP_IMJ_STEP>(m, allowedThreads);

            dCallReleaseeID stage4LCP_iMJSyncReleasee;
            world->PostThreadedCall(NULL, &stage4LCP_iMJSyncReleasee, stage4LCP_iMJ_allowedThreads, stage4LCP_IterationStartReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_iMJSync_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_iMJ Sync");

            world->PostThreadedCall(NULL, NULL, 0, stage4LCP_IterationStartReleasee, NULL, &dxQuickStepIsland_Stage4LCP_ReorderPrep_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_ReorderPrep");
            world->PostThreadedCallsGroup(NULL, stage4a_allowedThreads, stage4LCP_fcStartReleasee, &dxQuickStepIsland_Stage4a_Callback, stage4CallContext, "QuickStepIsland Stage4a");
            
            if (stage4LCP_iMJ_allowedThreads > 1) {
                world->PostThreadedCallsGroup(NULL, stage4LCP_iMJ_allowedThreads - 1, stage4LCP_iMJSyncReleasee, &dxQuickStepIsland_Stage4LCP_iMJ_Callback, stage4CallContext, "QuickStepIsland Stage4LCP_iMJ");
            }
            dxQuickStepIsland_Stage4LCP_iMJComputation(stage4CallContext);
            world->AlterThreadedCallDependenciesCount(stage4LCP_iMJSyncReleasee, -1);
        }
    }
    else {
        dxQuickStepIsland_Stage5(stage5CallContext);
    }
}

static 
int dxQuickStepIsland_Stage4a_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4a(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4a(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    dReal *lambda = stage4CallContext->m_lambda;
    const dxMIndexItem *mindex = localContext->m_mindex;
#ifdef WARM_STARTING
    dJointWithInfo1 *jointinfos = localContext->m_jointinfos;
#endif
    unsigned int nj = localContext->m_nj;
    const unsigned int step_size = dxQUICKSTEPISLAND_STAGE4A_STEP;
    unsigned int nj_steps = (nj + (step_size - 1)) / step_size;
    
    unsigned ji_step;
    while ((ji_step = ThrsafeIncrementIntUpToLimit(&stage4CallContext->m_ji_4a, nj_steps)) != nj_steps) {
        unsigned int ji = ji_step * step_size;
        dReal *lambdacurr = lambda + mindex[ji].mIndex;
#ifdef WARM_STARTING
        const dJointWithInfo1 *jicurr = jointinfos + ji;
        const dJointWithInfo1 *const jiend = jicurr + dMIN(step_size, nj - ji);
        
        do {
            const dReal *joint_lambdas = jicurr->joint->lambda;
            dReal *const lambdsnext = lambdacurr + jicurr->info.m;
            
            while (true) {
                // for warm starting, multiplication by 0.9 seems to be necessary to prevent
                // jerkiness in motor-driven joints. I have no idea why this works.
                *lambdacurr = *joint_lambdas * 0.9;

                if (++lambdacurr == lambdsnext) {
                    break;
                }

                ++joint_lambdas;
            }
        } 
        while (++jicurr != jiend);
#else
        dReal *lambdsnext = lambda + mindex[ji + dMIN(step_size, nj - ji)].mIndex;
        dSetZero(lambdacurr, lambdsnext - lambdacurr);
#endif
    }
}

static 
int dxQuickStepIsland_Stage4LCP_iMJ_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4LCP_iMJComputation(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_iMJComputation(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    dReal *iMJ = stage4CallContext->m_iMJ;
    unsigned int m = localContext->m_m;
    dReal *J = localContext->m_J;
    const dxJBodiesItem *jb = localContext->m_jb;
    dxBody * const *body = callContext->m_islandBodiesStart;
    dReal *invI = localContext->m_invI;

    // precompute iMJ = inv(M)*J'
    compute_invM_JT<dxQUICKSTEPISLAND_STAGE4LCP_IMJ_STEP>(&stage4CallContext->m_mi_iMJ, iMJ, m, J, jb, body, invI);
}

static 
int dxQuickStepIsland_Stage4LCP_iMJSync_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
    
    unsigned int m = localContext->m_m;
    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;

    unsigned int stage4LCP_Ad_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4LCP_AD_STEP>(m, allowedThreads);

#ifdef WARM_STARTING
    {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(stage4CallContext->m_LCP_fcStartReleasee, -1);
    }
#endif
    
    if (stage4LCP_Ad_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage4LCP_Ad_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage4LCP_Ad_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage4LCP_Ad_Callback, stage4CallContext, "QuickStepIsland Stage4LCP_Ad");
    }
    dxQuickStepIsland_Stage4LCP_AdComputation(stage4CallContext);

    return 1;
}

static 
int dxQuickStepIsland_Stage4LCP_fcStart_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    unsigned int fcPrepareComplexity, fcCompleteComplexity;
#ifdef WARM_STARTING
    fcPrepareComplexity = localContext->m_m / dxQUICKSTEPISLAND_STAGE4LCP_FC_COMPLETE_TO_PREPARE_COMPLEXITY_DIVISOR;
    fcCompleteComplexity = callContext->m_islandBodiesCount;
#else
    fcPrepareComplexity = localContext->m_m;
    fcCompleteComplexity = 0;
#endif
    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
    unsigned int stage4LCP_fcPrepare_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP>(fcPrepareComplexity, allowedThreads);
    unsigned int stage4LCP_fcComplete_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP>(fcCompleteComplexity, allowedThreads);
    stage4CallContext->AssignLCP_fcAllowedThreads(stage4LCP_fcPrepare_allowedThreads, stage4LCP_fcComplete_allowedThreads);

#ifdef WARM_STARTING
    dxQuickStepIsland_Stage4LCP_MTfcComputation_warmZeroArrays(stage4CallContext);
#endif

    if (stage4LCP_fcPrepare_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage4LCP_fcPrepare_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage4LCP_fcPrepare_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage4LCP_fc_Callback, stage4CallContext, "QuickStepIsland Stage4LCP_fc");
    }
    dxQuickStepIsland_Stage4LCP_MTfcComputation(stage4CallContext, callThisReleasee);

    return 1;
}

static 
int dxQuickStepIsland_Stage4LCP_fc_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4LCP_MTfcComputation(stage4CallContext, callThisReleasee);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation(dxQuickStepperStage4CallContext *stage4CallContext, dCallReleaseeID callThisReleasee)
{
#ifdef WARM_STARTING
    dxQuickStepIsland_Stage4LCP_MTfcComputation_warm(stage4CallContext, callThisReleasee);
#else
    (void)callThisReleasee; // unused
    dxQuickStepIsland_Stage4LCP_MTfcComputation_cold(stage4CallContext);
#endif
}

#ifdef WARM_STARTING

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation_warm(dxQuickStepperStage4CallContext *stage4CallContext, dCallReleaseeID callThisReleasee)
{
    dxQuickStepIsland_Stage4LCP_MTfcComputation_warmPrepare(stage4CallContext);

    if (ThrsafeExchangeAdd(&stage4CallContext->m_LCP_fcPrepareThreadsRemaining, (atomicord32)(-1)) == 1) {
        stage4CallContext->ResetLCP_fcComputationIndex();

        const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
        unsigned int stage4LCP_fcComplete_allowedThreads = stage4CallContext->m_LCP_fcCompleteThreadsTotal;

        if (stage4LCP_fcComplete_allowedThreads > 1) {
            dxWorld *world = callContext->m_world;
            world->AlterThreadedCallDependenciesCount(callThisReleasee, stage4LCP_fcComplete_allowedThreads - 1);
            world->PostThreadedCallsGroup(NULL, stage4LCP_fcComplete_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage4LCP_fcWarmComplete_Callback, stage4CallContext, "QuickStepIsland Stage4LCP_fcWarmComplete");
        }
        dxQuickStepIsland_Stage4LCP_MTfcComputation_warmComplete(stage4CallContext);
    }
}

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmZeroArrays(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;

    unsigned int nb = callContext->m_islandBodiesCount;
    atomicord32 *bi_links = stage4CallContext->m_bi_links_or_mi_levels;

    multiply_invM_JT_init_array(nb, bi_links);
}

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmPrepare(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    unsigned int m = localContext->m_m;
    const dxJBodiesItem *jb = localContext->m_jb;

    // Prepare to compute fc=(inv(M)*J')*lambda. we will incrementally maintain fc
    // as we change lambda.
    multiply_invM_JT_prepare<dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP_PREPARE>(&stage4CallContext->m_mi_fc, m, jb, stage4CallContext->m_bi_links_or_mi_levels, stage4CallContext->m_mi_links);
}

static 
int dxQuickStepIsland_Stage4LCP_fcWarmComplete_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;

    dxQuickStepIsland_Stage4LCP_MTfcComputation_warmComplete(stage4CallContext);

    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation_warmComplete(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    dReal *fc = stage4CallContext->m_cforce;
    unsigned int nb = callContext->m_islandBodiesCount;
    dReal *iMJ = stage4CallContext->m_iMJ;
    const dxJBodiesItem *jb = localContext->m_jb;
    dReal *lambda = stage4CallContext->m_lambda;

    // Complete computation of fc=(inv(M)*J')*lambda. we will incrementally maintain fc
    // as we change lambda.
    multiply_invM_JT_complete<dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP_COMPLETE, CFE__DYNAMICS_MIN, CFE__MAX>(&stage4CallContext->m_mi_fc, fc, nb, iMJ, jb, lambda, stage4CallContext->m_bi_links_or_mi_levels, stage4CallContext->m_mi_links);
}

#else // #ifndef WARM_STARTING

static 
void dxQuickStepIsland_Stage4LCP_MTfcComputation_cold(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;

    dReal *fc = stage4CallContext->m_cforce;
    unsigned int nb = callContext->m_islandBodiesCount;
    const unsigned int step_size = dxQUICKSTEPISLAND_STAGE4LCP_FC_STEP;
    unsigned int nb_steps = (nb + (step_size - 1)) / step_size;

    unsigned bi_step;
    while ((bi_step = ThrsafeIncrementIntUpToLimit(&stage4CallContext->m_mi_fc, nb_steps)) != nb_steps) {
        unsigned int bi = bi_step * step_size;
        unsigned int bicnt = dMIN(step_size, nb - bi);
        dSetZero(fc + (size_t)bi * CFE__MAX, (size_t)bicnt * CFE__MAX);
    }
}

#endif // #ifndef WARM_STARTING


static 
void dxQuickStepIsland_Stage4LCP_STfcComputation(dxQuickStepperStage4CallContext *stage4CallContext)
{
#ifdef WARM_STARTING
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    dReal *fc = stage4CallContext->m_cforce;
    unsigned int m = localContext->m_m;
    unsigned int nb = callContext->m_islandBodiesCount;
    dReal *iMJ = stage4CallContext->m_iMJ;
    const dxJBodiesItem *jb = localContext->m_jb;
    dReal *lambda = stage4CallContext->m_lambda;

    // compute fc=(inv(M)*J')*lambda. we will incrementally maintain fc
    // as we change lambda.
    _multiply_invM_JT<CFE__DYNAMICS_MIN, CFE__MAX>(fc, m, nb, iMJ, jb, lambda);
#else
	dReal *fc = stage4CallContext->m_cforce;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    unsigned int nb = callContext->m_islandBodiesCount;

    dSetZero(fc, (size_t)nb * CFE__MAX);
#endif

}

static 
int dxQuickStepIsland_Stage4LCP_Ad_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4LCP_AdComputation(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_AdComputation(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    const dxJBodiesItem *jb = localContext->m_jb;
    dReal *J = localContext->m_J;
    unsigned int m = localContext->m_m;

    dxWorld *world = callContext->m_world;
    dxQuickStepParameters *qs = &world->qs;
    const dReal sor_w = qs->w;		// SOR over-relaxation parameter

    dReal *iMJ = stage4CallContext->m_iMJ;

    const unsigned int step_size = dxQUICKSTEPISLAND_STAGE4LCP_AD_STEP;
    unsigned int m_steps = (m + (step_size - 1)) / step_size;

    unsigned mi_step;
    while ((mi_step = ThrsafeIncrementIntUpToLimit(&stage4CallContext->m_mi_Ad, m_steps)) != m_steps) {
        unsigned int mi = mi_step * step_size;
        const unsigned int miend = mi + dMIN(step_size, m - mi);

        const dReal *iMJ_ptr = iMJ + (size_t)mi * IMJ__MAX;
        dReal *J_ptr = J + (size_t)mi * JME__MAX;
        while (true) {
            dReal sum = REAL(0.0);
            {
                for (unsigned int j = JVE__MIN; j != JVE__MAX; ++j) sum += iMJ_ptr[IMJ__1_MIN + j] * J_ptr[JME__J1_MIN + j];
                dSASSERT(JME__J1_COUNT == (int)JVE__MAX);
            }

            int b2 = jb[mi].second;
            if (b2 != -1) {
                for (unsigned int k = JVE__MIN; k != JVE__MAX; ++k) sum += iMJ_ptr[IMJ__2_MIN + k] * J_ptr[JME__J2_MIN + k];
                dSASSERT(JME__J2_COUNT == (int)JVE__MAX);
            }

            dReal cfm_i = J_ptr[JME_CFM];
            dReal Ad_i = sor_w / (sum + cfm_i);

            // NOTE: This may seem unnecessary but it's indeed an optimization 
            // to move multiplication by Ad[i] and cfm[i] out of iteration loop.

            // scale cfm, J and b by Ad
            J_ptr[JME_CFM] = cfm_i * Ad_i;
            J_ptr[JME_RHS] *= Ad_i;

            {
                for (unsigned int j = JVE__MIN; j != JVE__MAX; ++j) J_ptr[JME__J1_MIN + j] *= Ad_i;
                dSASSERT(JME__J1_COUNT == (int)JVE__MAX);
            }

            if (b2 != -1) {
                for (unsigned int k = JVE__MIN; k != JVE__MAX; ++k) J_ptr[JME__J2_MIN + k] *= Ad_i;
                dSASSERT(JME__J2_COUNT == (int)JVE__MAX);
            }

            if (++mi == miend) {
                break;
            }
            iMJ_ptr += IMJ__MAX;
            J_ptr += JME__MAX;
        }
    }
}

static 
int dxQuickStepIsland_Stage4LCP_ReorderPrep_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4LCP_ReorderPrep(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_ReorderPrep(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
    unsigned int m = localContext->m_m;
    unsigned int valid_findices = localContext->m_valid_findices;

    IndexError *order = stage4CallContext->m_order;

    {
        // make sure constraints with findex < 0 come first.
        IndexError *orderhead = order, *ordertail = order + (m - valid_findices);
        const int *findex = localContext->m_findex;

        // Fill the array from both ends
        for (unsigned int i = 0; i != m; ++i) {
            if (findex[i] == -1) {
                orderhead->index = i; // Place them at the front
                ++orderhead;
            } else {
                ordertail->index = i; // Place them at the end
                ++ordertail;
            }
        }
        dIASSERT(orderhead == order + (m - valid_findices));
        dIASSERT(ordertail == order + m);
    }
}

static 
int dxQuickStepIsland_Stage4LCP_IterationStart_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;

    dxWorld *world = callContext->m_world;
    dxQuickStepParameters *qs = &world->qs;

    const unsigned int num_iterations = qs->num_iterations;
    unsigned iteration = stage4CallContext->m_LCP_iteration;
    
    if (iteration < num_iterations)
    {
        dCallReleaseeID nextReleasee;
        dCallReleaseeID stage4LCP_IterationSyncReleasee = stage4CallContext->m_LCP_IterationSyncReleasee;
        unsigned int stage4LCP_Iteration_allowedThreads = stage4CallContext->m_LCP_IterationAllowedThreads;

        bool reorderRequired = false;
        unsigned syncCallDependencies = stage4LCP_Iteration_allowedThreads;

        if (IsSORConstraintsReorderRequiredForIteration(iteration))
        {
            syncCallDependencies = 1;
            reorderRequired = true;
        }

        // Increment iterations counter in advance as anyway it needs to be incremented 
        // before independent tasks (the reordering or the iteration) are posted
        // (otherwise next iteration may complete before the increment 
        // and the same iteration index may be used again).
        stage4CallContext->m_LCP_iteration = iteration + 1;

        if (iteration + 1 != num_iterations) {
            dCallReleaseeID stage4LCP_IterationStartReleasee;
            world->PostThreadedCallForUnawareReleasee(NULL, &stage4LCP_IterationStartReleasee, syncCallDependencies, stage4LCP_IterationSyncReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_IterationStart_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_Iteration Start");
            nextReleasee = stage4LCP_IterationStartReleasee;
        }
        else {
            world->AlterThreadedCallDependenciesCount(stage4LCP_IterationSyncReleasee, syncCallDependencies);
            nextReleasee = stage4LCP_IterationSyncReleasee;
        }

        if (reorderRequired) {
            const unsigned int reorderThreads = 2;
            dIASSERT(callContext->m_stepperAllowedThreads >= 2); // Otherwise the single-threaded execution path would be taken

            stage4CallContext->ResetSOR_ConstraintsReorderVariables(reorderThreads);

            dCallReleaseeID stage4LCP_ConstraintsReorderingSyncReleasee;
            world->PostThreadedCall(NULL, &stage4LCP_ConstraintsReorderingSyncReleasee, reorderThreads, nextReleasee, 
                NULL, &dxQuickStepIsland_Stage4LCP_ConstraintsReorderingSync_Callback, stage4CallContext, 0, "QuickStepIsland Stage4LCP_ConstraintsReordering Sync");

            if (reorderThreads > 1) {
                world->PostThreadedCallsGroup(NULL, reorderThreads - 1, stage4LCP_ConstraintsReorderingSyncReleasee, &dxQuickStepIsland_Stage4LCP_ConstraintsReordering_Callback, stage4CallContext, "QuickStepIsland Stage4LCP_ConstraintsReordering");
            }
            dxQuickStepIsland_Stage4LCP_ConstraintsReordering(stage4CallContext);
            world->AlterThreadedCallDependenciesCount(stage4LCP_ConstraintsReorderingSyncReleasee, -1);
        }
        else {
            dIASSERT(iteration != 0); {
                dxQuickStepIsland_Stage4LCP_DependencyMapFromSavedLevelsReconstruction(stage4CallContext);
            }

            stage4CallContext->RecordLCP_IterationStart(stage4LCP_Iteration_allowedThreads, nextReleasee);

            unsigned knownToBeCompletedLevel = dxHEAD_INDEX;
            if (stage4LCP_Iteration_allowedThreads > 1) {
                world->PostThreadedCallsIndexOverridenGroup(NULL, stage4LCP_Iteration_allowedThreads - 1, nextReleasee, &dxQuickStepIsland_Stage4LCP_Iteration_Callback, stage4CallContext, knownToBeCompletedLevel, "QuickStepIsland Stage4LCP_Iteration");
            }
            dxQuickStepIsland_Stage4LCP_MTIteration(stage4CallContext, knownToBeCompletedLevel);
            world->AlterThreadedCallDependenciesCount(nextReleasee, -1);
        }
    }

    return 1;
}

static 
int dxQuickStepIsland_Stage4LCP_ConstraintsReordering_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4LCP_ConstraintsReordering(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_ConstraintsReordering(dxQuickStepperStage4CallContext *stage4CallContext)
{
    unsigned int iteration = stage4CallContext->m_LCP_iteration - 1; // Iteration is pre-incremented before scheduled tasks are released for execution
    if (dxQuickStepIsland_Stage4LCP_ConstraintsShuffling(stage4CallContext, iteration)) {

        dxQuickStepIsland_Stage4LCP_LinksArraysZeroing(stage4CallContext);
        if (ThrsafeExchangeAdd(&stage4CallContext->m_SOR_reorderThreadsRemaining, (atomicord32)(-1)) == 1) { // If last thread has exited the reordering routine...
            // Rebuild the object dependency map
            dxQuickStepIsland_Stage4LCP_DependencyMapForNewOrderRebuilding(stage4CallContext);
        }
    }
}

static 
bool dxQuickStepIsland_Stage4LCP_ConstraintsShuffling(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int iteration)
{
    bool result = false;

#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR
    struct ConstraintsReorderingHelper
    {
        void operator ()(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int startIndex, unsigned int endIndex)
        {
            const dReal *lambda = stage4CallContext->m_lambda;
            dReal *last_lambda = stage4CallContext->m_last_lambda;
            IndexError *order = stage4CallContext->m_order;

            for (unsigned int index = startIndex; index != endIndex; ++index) {
                unsigned int i = order[index].index;
                dReal lambda_i = lambda[i];
                if (lambda_i != REAL(0.0)) {
                    //@@@ relative error: order[i].error = dFabs(lambda[i]-last_lambda[i])/max;
                    order[index].error = dFabs(lambda_i - last_lambda[i]);
                }
                else if (last_lambda[i] != REAL(0.0)) {
                    //@@@ relative error: order[i].error = dFabs(lambda[i]-last_lambda[i])/max;
                    order[index].error = dFabs(/*lambda_i - */last_lambda[i]); // lambda_i == 0
                }
                else {
                    order[index].error = dInfinity;
                }
                // Finally copy the lambda for the next iteration
                last_lambda[i] = lambda_i;
            }
            qsort (order + startIndex, endIndex - startIndex, sizeof(IndexError), &compare_index_error);
        }
    };

    if (iteration > 1) { // Only reorder starting from iteration #2
        // sort the constraints so that the ones converging slowest
        // get solved last. use the absolute (not relative) error.
        if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderHeadTaken, 1) == 0) {
            // Process the head
            const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
            ConstraintsReorderingHelper()(stage4CallContext, 0, localContext->m_m - localContext->m_valid_findices);
        }
        
        if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderTailTaken, 1) == 0) {
            // Process the tail
            const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
            ConstraintsReorderingHelper()(stage4CallContext, localContext->m_m - localContext->m_valid_findices, localContext->m_m);
        }

        result = true;
    }
    else if (iteration == 1) {
        if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderHeadTaken, 1) == 0) {
            // Process the first half
            const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
            unsigned int startIndex = 0;
            unsigned int indicesCount = localContext->m_m / 2;
            // Just copy the lambdas for the next iteration
            memcpy(stage4CallContext->m_last_lambda + startIndex, stage4CallContext->m_lambda + startIndex, indicesCount * sizeof(dReal));
        }

        if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderTailTaken, 1) == 0) {
            // Process the second half
            const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
            unsigned int startIndex = localContext->m_m / 2;
            unsigned int indicesCount = localContext->m_m - startIndex;
            // Just copy the lambdas for the next iteration
            memcpy(stage4CallContext->m_last_lambda + startIndex, stage4CallContext->m_lambda + startIndex, indicesCount * sizeof(dReal));
        }

        // result = false; -- already 'false'
    } 
    else if (iteration < 1) {
        result = true; // return true on 0th iteration to build dependency map for initial order 
    }
#elif CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__RANDOMLY
    struct ConstraintsReorderingHelper
    {
        void operator ()(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int startIndex, unsigned int indicesCount)
        {
            IndexError *order = stage4CallContext->m_order + startIndex;

            for (unsigned int index = 1; index < indicesCount; ++index) {
                int swapIndex = dRandInt(index + 1);
                IndexError tmp = order[index];
                order[index] = order[swapIndex];
                order[swapIndex] = tmp;
            }
        }
    };

    if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderHeadTaken, 1) == 0) {
        // Process the head
        const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
        ConstraintsReorderingHelper()(stage4CallContext, 0, localContext->m_m - localContext->m_valid_findices);
    }

    if (ThrsafeExchange(&stage4CallContext->m_SOR_reorderTailTaken, 1) == 0) {
        // Process the tail
        const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
        ConstraintsReorderingHelper()(stage4CallContext, localContext->m_m - localContext->m_valid_findices, localContext->m_valid_findices);
    }

    result = true;
#else // #if CONSTRAINTS_REORDERING_METHOD != REORDERING_METHOD__BY_ERROR && CONSTRAINTS_REORDERING_METHOD != REORDERING_METHOD__RANDOMLY
    if (iteration == 0) {
        result = true; // return true on 0th iteration to build dependency map for initial order 
    }
#endif

    return result;
}

static 
void dxQuickStepIsland_Stage4LCP_LinksArraysZeroing(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
    
    if (ThrsafeExchange(&stage4CallContext->m_SOR_bi_zeroHeadTaken, 1) == 0) {
        atomicord32 *bi_links = stage4CallContext->m_bi_links_or_mi_levels;/*=[nb]*/
        unsigned int nb = callContext->m_islandBodiesCount;
        memset(bi_links, 0, sizeof(bi_links[0]) * (nb / 2));
    }
    if (ThrsafeExchange(&stage4CallContext->m_SOR_bi_zeroTailTaken, 1) == 0) {
        atomicord32 *bi_links = stage4CallContext->m_bi_links_or_mi_levels;/*=[nb]*/
        unsigned int nb = callContext->m_islandBodiesCount;
        memset(bi_links + nb / 2, 0, sizeof(bi_links[0]) * (nb - nb / 2));
    }

    if (ThrsafeExchange(&stage4CallContext->m_SOR_mi_zeroHeadTaken, 1) == 0) {
        atomicord32 *mi_links = stage4CallContext->m_mi_links;/*=[2*(m + 1)]*/
        unsigned int m = localContext->m_m;
        memset(mi_links, 0, sizeof(mi_links[0]) * (m + 1));
    }
    if (ThrsafeExchange(&stage4CallContext->m_SOR_mi_zeroTailTaken, 1) == 0) {
        atomicord32 *mi_links = stage4CallContext->m_mi_links;/*=[2*(m + 1)]*/
        unsigned int m = localContext->m_m;
        memset(mi_links + (m + 1), 0, sizeof(mi_links[0]) * (m + 1));
    }
}

static 
void dxQuickStepIsland_Stage4LCP_DependencyMapForNewOrderRebuilding(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
    
    atomicord32 *bi_links = stage4CallContext->m_bi_links_or_mi_levels;/*=[nb]*/
    atomicord32 *mi_links = stage4CallContext->m_mi_links;/*=[2*(m + 1)]*/

    IndexError *order = stage4CallContext->m_order;
    const dxJBodiesItem *jb = localContext->m_jb;

    unsigned int m = localContext->m_m;
    for (unsigned int i = 0; i != m; ++i) {
        unsigned int index = order[i].index;

        int b1 = jb[index].first;
        int b2 = jb[index].second;

        unsigned int encioded_i = dxENCODE_INDEX(i);

        unsigned int encoded_depi = bi_links[(unsigned int)b1];
        bi_links[(unsigned int)b1] = encioded_i;

        if (b2 != -1 && b2 != b1) {
            if (encoded_depi < (unsigned int)bi_links[(unsigned int)b2]) {
                encoded_depi = bi_links[(unsigned int)b2];
            }
            bi_links[(unsigned int)b2] = encioded_i;
        }

        // OD: There is also a dependency on findex[index],
        // however the findex can only refer to the rows of the same joint 
        // and hence that index is going to have the same bodies. Since the 
        // indices are sorted in a way that the meaningful findex values 
        // always come last, the dependency of findex[index] is going to
        // be implicitly satisfied via matching bodies at smaller "i"s.

        // Check that the dependency targets an earlier "i"
        dIASSERT(encoded_depi < encioded_i);

        unsigned encoded_downi = mi_links[(size_t)encoded_depi * 2 + 1];
        mi_links[(size_t)encoded_depi * 2 + 1] = encioded_i; // Link i as down-dependency for depi
        mi_links[(size_t)encioded_i * 2] = encoded_downi; // Link previous down-chain as the level-dependency with i
    }
}

static 
void dxQuickStepIsland_Stage4LCP_DependencyMapFromSavedLevelsReconstruction(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    atomicord32 *mi_levels = stage4CallContext->m_bi_links_or_mi_levels;/*=[m]*/
    atomicord32 *mi_links = stage4CallContext->m_mi_links;/*=[2*(m + 1)]*/

    unsigned int m = localContext->m_m;
    for (unsigned int i = 0; i != m; ++i) {
        unsigned int currentLevelRoot = mi_levels[i];
        unsigned int currentLevelFirstLink = mi_links[2 * (size_t)currentLevelRoot + 1];
        unsigned int encoded_i = dxENCODE_INDEX(i);
        mi_links[2 * (size_t)currentLevelRoot + 1] = encoded_i;
        mi_links[2 * (size_t)encoded_i + 0] = currentLevelFirstLink;
    }

    // Additionally reset available level root's list head
    mi_links[2 * dxHEAD_INDEX + 0] = dxHEAD_INDEX;
}

static 
int dxQuickStepIsland_Stage4LCP_ConstraintsReorderingSync_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    unsigned int stage4LCP_Iteration_allowedThreads = stage4CallContext->m_LCP_IterationAllowedThreads;

    stage4CallContext->RecordLCP_IterationStart(stage4LCP_Iteration_allowedThreads, callThisReleasee);

    unsigned knownToBeCompletedLevel = dxHEAD_INDEX;
    if (stage4LCP_Iteration_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage4LCP_Iteration_allowedThreads - 1);
        world->PostThreadedCallsIndexOverridenGroup(NULL, stage4LCP_Iteration_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage4LCP_Iteration_Callback, stage4CallContext, knownToBeCompletedLevel, "QuickStepIsland Stage4LCP_Iteration");
    }
    dxQuickStepIsland_Stage4LCP_MTIteration(stage4CallContext, knownToBeCompletedLevel);

    return 1;
}

static 
int dxQuickStepIsland_Stage4LCP_Iteration_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    unsigned int initiallyKnownToBeCompletedLevel = (unsigned int)callInstanceIndex;
    dIASSERT(initiallyKnownToBeCompletedLevel == callInstanceIndex); // A truncation check...

    dxQuickStepIsland_Stage4LCP_MTIteration(stage4CallContext, initiallyKnownToBeCompletedLevel);
    return 1;
}

static 
void dxQuickStepIsland_Stage4LCP_MTIteration(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int initiallyKnownToBeCompletedLevel)
{
    atomicord32 *mi_levels = stage4CallContext->m_bi_links_or_mi_levels;
    atomicord32 *mi_links = stage4CallContext->m_mi_links;

    unsigned int knownToBeCompletedLevel = initiallyKnownToBeCompletedLevel;

    while (true) {
        unsigned int initialLevelRoot = mi_links[2 * dxHEAD_INDEX + 0];
        if (initialLevelRoot != dxHEAD_INDEX && initialLevelRoot == knownToBeCompletedLevel) {
            // No work is (currently) available
            break;
        }
        
        for (unsigned int currentLevelRoot = initialLevelRoot; ; currentLevelRoot = mi_links[2 * (size_t)currentLevelRoot + 0]) {
            while (true) {
                const unsigned invalid_link = dxENCODE_INDEX(-1);

                unsigned currentLevelFirstLink = mi_links[2 * (size_t)currentLevelRoot + 1];
                if (currentLevelFirstLink == invalid_link) {
                    break;
                }
                
                // Try to extract first record from linked list
                unsigned currentLevelNextLink = mi_links[2 * (size_t)currentLevelFirstLink + 0];
                if (ThrsafeCompareExchange(&mi_links[2 * (size_t)currentLevelRoot + 1], currentLevelFirstLink, currentLevelNextLink)) {
                    // if succeeded, execute selected iteration step...
                    dxQuickStepIsland_Stage4LCP_IterationStep(stage4CallContext, dxDECODE_INDEX(currentLevelFirstLink));

                    // Check if there are any dependencies
                    unsigned level0DownLink = mi_links[2 * (size_t)currentLevelFirstLink + 1];
                    if (level0DownLink != invalid_link) {
                        // ...and if yes, insert the record into the list of available level roots
                        unsigned int levelRootsFirst;
                        do {
                            levelRootsFirst = mi_links[2 * dxHEAD_INDEX + 0];
                            mi_links[2 * (size_t)currentLevelFirstLink + 0] = levelRootsFirst;
                        }
                        while (!ThrsafeCompareExchange(&mi_links[2 * dxHEAD_INDEX + 0], levelRootsFirst, currentLevelFirstLink));

                        // If another level was added and some threads have already exited...
                        unsigned int threadsTotal = stage4CallContext->m_LCP_iterationThreadsTotal;
                        unsigned int threadsRemaining = ThrsafeIncrementIntUpToLimit(&stage4CallContext->m_LCP_iterationThreadsRemaining, threadsTotal);
                        if (threadsRemaining != threadsTotal) {
                            // ...go on an schedule one more...
                            const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
                            dxWorld *world = callContext->m_world;
                            // ...passing knownToBeCompletedLevel as the initial one for the spawned call
                            world->PostThreadedCallForUnawareReleasee(NULL, NULL, 0, stage4CallContext->m_LCP_iterationNextReleasee, NULL, &dxQuickStepIsland_Stage4LCP_Iteration_Callback, stage4CallContext, knownToBeCompletedLevel, "QuickStepIsland Stage4LCP_Iteration");
                            // NOTE: it's hard to predict whether it is reasonable to re-post a call
                            // each time a new level is added (provided some calls have already exited, of course).
                            // The efficiency very much depends on dependencies patterns between levels 
                            // (i.e. it depends on the amount of available work added with each level).
                            // The strategy of re-posting exited calls as frequently as possible
                            // leads to potential wasting execution cycles in some cores for the aid
                            // of keeping other cores busy as much as possible and not letting all the
                            // work be executed by just a partial cores subset. With emergency of large
                            // available work amounts (the work that is not dependent on anything and 
                            // ready to be executed immediately) this strategy is going to transit into 
                            // full cores set being busy executing useful work. If amounts of work 
                            // emerging from added levels are small, the strategy should lead to 
                            // approximately the same efficiency as if the work was done by only a cores subset 
                            // with the remaining cores wasting (some) cycles for re-scheduling calls 
                            // to those busy cores rather than being idle or handling other islands. 
                        }
                    }

                    // Finally record the root index of current record's level
                    mi_levels[dxDECODE_INDEX(currentLevelFirstLink)] = currentLevelRoot;
                }
            }

            if (currentLevelRoot == knownToBeCompletedLevel) {
                break;
            }
            dIASSERT(currentLevelRoot != dxHEAD_INDEX); // Zero level is expected to be the deepest one in the list and execution must not loop past it.
        }
        // Save the level root we started from as known to be completed
        knownToBeCompletedLevel = initialLevelRoot;
    }

    // Decrement running threads count on exit
    ThrsafeAdd(&stage4CallContext->m_LCP_iterationThreadsRemaining, (atomicord32)(-1));
}

static 
void dxQuickStepIsland_Stage4LCP_STIteration(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    unsigned int m = localContext->m_m;
    for (unsigned int i = 0; i != m; ++i) {
        dxQuickStepIsland_Stage4LCP_IterationStep(stage4CallContext, i);
    }
}

//***************************************************************************
// SOR-LCP method

// nb is the number of bodies in the body array.
// J is an m*16 matrix of constraint rows with rhs, cfm, lo and hi in padding
// jb is an array of first and second body numbers for each constraint row
// invI is the global frame inverse inertia for each body (stacked 3x3 matrices)
//
// this returns lambda and fc (the constraint force).
// note: fc is returned as inv(M)*J'*lambda, the constraint force is actually J'*lambda
//
// b, lo and hi are modified on exit

static 
void dxQuickStepIsland_Stage4LCP_IterationStep(dxQuickStepperStage4CallContext *stage4CallContext, unsigned int i)
{
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    IndexError *order = stage4CallContext->m_order;
    unsigned int index = order[i].index;

    dReal *fc_ptr1;
    dReal *fc_ptr2 = NULL;
    dReal delta;

    dReal *lambda = stage4CallContext->m_lambda;
    dReal old_lambda = lambda[index];

    dReal *J = localContext->m_J;
    const dReal *J_ptr = J + (size_t)index * JME__MAX;

    {
        delta = J_ptr[JME_RHS] - old_lambda * J_ptr[JME_CFM];

        dReal *fc = stage4CallContext->m_cforce;

        const dxJBodiesItem *jb = localContext->m_jb;
        int b2 = jb[index].second;
        int b1 = jb[index].first;

        // @@@ potential optimization: SIMD-ize this and the b2 >= 0 case
        fc_ptr1 = fc + (size_t)(unsigned)b1 * CFE__MAX;
        delta -= fc_ptr1[CFE_LX] * J_ptr[JME_J1LX] + fc_ptr1[CFE_LY] * J_ptr[JME_J1LY] +
            fc_ptr1[CFE_LZ] * J_ptr[JME_J1LZ] + fc_ptr1[CFE_AX] * J_ptr[JME_J1AX] +
            fc_ptr1[CFE_AY] * J_ptr[JME_J1AY] + fc_ptr1[CFE_AZ] * J_ptr[JME_J1AZ];
        // @@@ potential optimization: handle 1-body constraints in a separate
        //     loop to avoid the cost of test & jump?
        if (b2 != -1) {
            fc_ptr2 = fc + (size_t)(unsigned)b2 * CFE__MAX;
            delta -= fc_ptr2[CFE_LX] * J_ptr[JME_J2LX] + fc_ptr2[CFE_LY] * J_ptr[JME_J2LY] +
                fc_ptr2[CFE_LZ] * J_ptr[JME_J2LZ] + fc_ptr2[CFE_AX] * J_ptr[JME_J2AX] +
                fc_ptr2[CFE_AY] * J_ptr[JME_J2AY] + fc_ptr2[CFE_AZ] * J_ptr[JME_J2AZ];
        }
    }

    {
        dReal hi_act, lo_act;

        // set the limits for this constraint. 
        // this is the place where the QuickStep method differs from the
        // direct LCP solving method, since that method only performs this
        // limit adjustment once per time step, whereas this method performs
        // once per iteration per constraint row.
        // the constraints are ordered so that all lambda[] values needed have
        // already been computed.
        const int *findex = localContext->m_findex;
        if (findex[index] != -1) {
            hi_act = dFabs (J_ptr[JME_HI] * lambda[findex[index]]);
            lo_act = -hi_act;
        } else {
            hi_act = J_ptr[JME_HI];
            lo_act = J_ptr[JME_LO];
        }

        // compute lambda and clamp it to [lo,hi].
        // @@@ potential optimization: does SSE have clamping instructions
        //     to save test+jump penalties here?
        dReal new_lambda = old_lambda + delta;
        if (new_lambda < lo_act) {
            delta = lo_act - old_lambda;
            lambda[index] = lo_act;
        }
        else if (new_lambda > hi_act) {
            delta = hi_act - old_lambda;
            lambda[index] = hi_act;
        }
        else {
            lambda[index] = new_lambda;
        }
    }

    //@@@ a trick that may or may not help
    //dReal ramp = (1-((dReal)(iteration+1)/(dReal)num_iterations));
    //delta *= ramp;

    {
        dReal *iMJ = stage4CallContext->m_iMJ;
        const dReal *iMJ_ptr = iMJ + (size_t)index * IMJ__MAX;
        // update fc.
        // @@@ potential optimization: SIMD for this and the b2 >= 0 case
        fc_ptr1[CFE_LX] += delta * iMJ_ptr[IMJ_1LX];
        fc_ptr1[CFE_LY] += delta * iMJ_ptr[IMJ_1LY];
        fc_ptr1[CFE_LZ] += delta * iMJ_ptr[IMJ_1LZ];
        fc_ptr1[CFE_AX] += delta * iMJ_ptr[IMJ_1AX];
        fc_ptr1[CFE_AY] += delta * iMJ_ptr[IMJ_1AY];
        fc_ptr1[CFE_AZ] += delta * iMJ_ptr[IMJ_1AZ];
        // @@@ potential optimization: handle 1-body constraints in a separate
        //     loop to avoid the cost of test & jump?
        if (fc_ptr2) {
            fc_ptr2[CFE_LX] += delta * iMJ_ptr[IMJ_2LX];
            fc_ptr2[CFE_LY] += delta * iMJ_ptr[IMJ_2LY];
            fc_ptr2[CFE_LZ] += delta * iMJ_ptr[IMJ_2LZ];
            fc_ptr2[CFE_AX] += delta * iMJ_ptr[IMJ_2AX];
            fc_ptr2[CFE_AY] += delta * iMJ_ptr[IMJ_2AY];
            fc_ptr2[CFE_AZ] += delta * iMJ_ptr[IMJ_2AZ];
        }
    }
}

static inline 
bool IsStage4bJointInfosIterationRequired(const dxQuickStepperLocalContext *localContext)
{
    return 
#ifdef WARM_STARTING
        true ||      
#endif
        localContext->m_mfb > 0;
}

static 
int dxQuickStepIsland_Stage4LCP_IterationSync_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;
    
    unsigned int stage4b_allowedThreads = 1;
    if (IsStage4bJointInfosIterationRequired(localContext)) {
        unsigned int allowedThreads = callContext->m_stepperAllowedThreads;
        dIASSERT(allowedThreads >= stage4b_allowedThreads);
        stage4b_allowedThreads += CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE4B_STEP>(localContext->m_nj, allowedThreads - stage4b_allowedThreads);
    }

    if (stage4b_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage4b_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage4b_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage4b_Callback, stage4CallContext, "QuickStepIsland Stage4b");
    }
    dxQuickStepIsland_Stage4b(stage4CallContext);
    
    return 1;
}

static 
int dxQuickStepIsland_Stage4b_Callback(void *_stage4CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage4CallContext *stage4CallContext = (dxQuickStepperStage4CallContext *)_stage4CallContext;
    dxQuickStepIsland_Stage4b(stage4CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage4b(dxQuickStepperStage4CallContext *stage4CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage4CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage4CallContext->m_localContext;

    if (ThrsafeExchange(&stage4CallContext->m_cf_4b, 1) == 0) {
        dxBody * const *body = callContext->m_islandBodiesStart;
        unsigned int nb = callContext->m_islandBodiesCount;
        const dReal *cforce = stage4CallContext->m_cforce;
        dReal stepsize = callContext->m_stepSize;
        // add stepsize * cforce to the body velocity
        const dReal *cforcecurr = cforce;
        dxBody *const *const bodyend = body + nb;
        for (dxBody *const *bodycurr = body; bodycurr != bodyend; cforcecurr += CFE__MAX, bodycurr++) {
            dxBody *b = *bodycurr;
            for (unsigned int j = dSA__MIN; j != dSA__MAX; j++) {
                b->lvel[dV3E__AXES_MIN + j] += stepsize * cforcecurr[CFE__L_MIN + j];
                b->avel[dV3E__AXES_MIN + j] += stepsize * cforcecurr[CFE__A_MIN + j];
            }
        }
    }


    // note that the SOR method overwrites rhs and J at this point, so
    // they should not be used again.

    if (IsStage4bJointInfosIterationRequired(localContext)) {
        dReal data[JVE__MAX];
        const dReal *Jcopy = localContext->m_Jcopy;
        const dReal *lambda = stage4CallContext->m_lambda;
        const dxMIndexItem *mindex = localContext->m_mindex;
        dJointWithInfo1 *jointinfos = localContext->m_jointinfos;

        unsigned int nj = localContext->m_nj;
        const unsigned int step_size = dxQUICKSTEPISLAND_STAGE4B_STEP;
        unsigned int nj_steps = (nj + (step_size - 1)) / step_size;

        unsigned ji_step;
        while ((ji_step = ThrsafeIncrementIntUpToLimit(&stage4CallContext->m_ji_4b, nj_steps)) != nj_steps) {
            unsigned int ji = ji_step * step_size;
            const dReal *lambdacurr = lambda + mindex[ji].mIndex;
            const dReal *Jcopycurr = Jcopy + (size_t)mindex[ji].fbIndex * JCE__MAX;
            const dJointWithInfo1 *jicurr = jointinfos + ji;
            const dJointWithInfo1 *const jiend = jicurr + dMIN(step_size, nj - ji);

            while (true) {
                dxJoint *joint = jicurr->joint;
                unsigned int infom = jicurr->info.m;
#ifdef WARM_STARTING
                memcpy(joint->lambda, lambdacurr, infom * sizeof(dReal));
#endif

                // straightforward computation of joint constraint forces:
                // multiply related lambdas with respective J' block for joints
                // where feedback was requested
                dJointFeedback *fb = joint->feedback;
                if (fb != NULL) {
                    Multiply1_12q1 (data, Jcopycurr + JCE__J1_MIN, lambdacurr, infom);
                    dSASSERT(JCE__MAX == 12);

                    fb->f1[dSA_X] = data[JVE_LX];
                    fb->f1[dSA_Y] = data[JVE_LY];
                    fb->f1[dSA_Z] = data[JVE_LZ];
                    fb->t1[dSA_X] = data[JVE_AX];
                    fb->t1[dSA_Y] = data[JVE_AY];
                    fb->t1[dSA_Z] = data[JVE_AZ];

                    if (joint->node[1].body) {
                        Multiply1_12q1 (data, Jcopycurr + JCE__J2_MIN, lambdacurr, infom);
                        dSASSERT(JCE__MAX == 12);

                        fb->f2[dSA_X] = data[JVE_LX];
                        fb->f2[dSA_Y] = data[JVE_LY];
                        fb->f2[dSA_Z] = data[JVE_LZ];
                        fb->t2[dSA_X] = data[JVE_AX];
                        fb->t2[dSA_Y] = data[JVE_AY];
                        fb->t2[dSA_Z] = data[JVE_AZ];
                    }

                    Jcopycurr += infom * JCE__MAX;
                }

                if (++jicurr == jiend) {
                    break;
                }
                lambdacurr += infom;
            }
        }
    }
}

static 
int dxQuickStepIsland_Stage5_Callback(void *_stage5CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage5CallContext *stage5CallContext = (dxQuickStepperStage5CallContext *)_stage5CallContext;
    dxQuickStepIsland_Stage5(stage5CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage5(dxQuickStepperStage5CallContext *stage5CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage5CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage5CallContext->m_localContext;

    dxWorldProcessMemArena *memarena = callContext->m_stepperArena;
    memarena->RestoreState(stage5CallContext->m_stage3MemArenaState);
    stage5CallContext = NULL; // WARNING! stage3CallContext is not valid after this point!
    dIVERIFY(stage5CallContext == NULL); // To suppress unused variable assignment warnings

    dxQuickStepperStage6CallContext *stage6CallContext = (dxQuickStepperStage6CallContext *)memarena->AllocateBlock(sizeof(dxQuickStepperStage6CallContext));
    stage6CallContext->Initialize(callContext, localContext);

    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
    dIASSERT(allowedThreads >= 1);

    if (allowedThreads == 1) {
        IFTIMING (dTimerNow ("compute velocity update"));
        dxQuickStepIsland_Stage6a(stage6CallContext);
        dxQuickStepIsland_Stage6_VelocityCheck(stage6CallContext);
        IFTIMING (dTimerNow ("update position and tidy up"));
        dxQuickStepIsland_Stage6b(stage6CallContext);
        IFTIMING (dTimerEnd());
        IFTIMING (if (m > 0) dTimerReport (stdout,1));
    }
    else {
        unsigned int nb = callContext->m_islandBodiesCount;
        unsigned int stage6a_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE6A_STEP>(nb, allowedThreads);

        dxWorld *world = callContext->m_world;

        dCallReleaseeID stage6aSyncReleasee;
        world->PostThreadedCallForUnawareReleasee(NULL, &stage6aSyncReleasee, stage6a_allowedThreads, callContext->m_finalReleasee, 
            NULL, &dxQuickStepIsland_Stage6aSync_Callback, stage6CallContext, 0, "QuickStepIsland Stage6a Sync");

        if (stage6a_allowedThreads > 1) {
            world->PostThreadedCallsGroup(NULL, stage6a_allowedThreads - 1, stage6aSyncReleasee, &dxQuickStepIsland_Stage6a_Callback, stage6CallContext, "QuickStepIsland Stage6a");
        }
        dxQuickStepIsland_Stage6a(stage6CallContext);
        world->AlterThreadedCallDependenciesCount(stage6aSyncReleasee, -1);
    }
}


static 
int dxQuickStepIsland_Stage6a_Callback(void *_stage6CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage6CallContext *stage6CallContext = (dxQuickStepperStage6CallContext *)_stage6CallContext;
    dxQuickStepIsland_Stage6a(stage6CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage6a(dxQuickStepperStage6CallContext *stage6CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage6CallContext->m_stepperCallContext;
    const dxQuickStepperLocalContext *localContext = stage6CallContext->m_localContext;

    dReal stepsize = callContext->m_stepSize;
    dReal *invI = localContext->m_invI;
    dxBody * const *body = callContext->m_islandBodiesStart;

    unsigned int nb = callContext->m_islandBodiesCount;
    const unsigned int step_size = dxQUICKSTEPISLAND_STAGE6A_STEP;
    unsigned int nb_steps = (nb + (step_size - 1)) / step_size;

    unsigned bi_step;
    while ((bi_step = ThrsafeIncrementIntUpToLimit(&stage6CallContext->m_bi_6a, nb_steps)) != nb_steps) {
        unsigned int bi = bi_step * step_size;
        unsigned int bicnt = dMIN(step_size, nb - bi);

        const dReal *invIrow = invI + (size_t)bi * IIE__MAX;
        dxBody *const *bodycurr = body + bi;
        dxBody *const *bodyend = bodycurr + bicnt;
        while (true) {
            // compute the velocity update:
            // add stepsize * invM * fe to the body velocity
            dxBody *b = *bodycurr;
            dReal body_invMass_mul_stepsize = stepsize * b->invMass;
            for (unsigned int j = dSA__MIN; j != dSA__MAX; ++j) {
                b->lvel[dV3E__AXES_MIN + j] += body_invMass_mul_stepsize * b->facc[dV3E__AXES_MIN + j];
                b->tacc[dV3E__AXES_MIN + j] *= stepsize;
            }
            dMultiplyAdd0_331 (b->avel, invIrow + IIE__MATRIX_MIN, b->tacc);
            
            if (++bodycurr == bodyend) {
                break;
            }
            invIrow += IIE__MAX;
        }
    }
}


static 
int dxQuickStepIsland_Stage6aSync_Callback(void *_stage6CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage6CallContext *stage6CallContext = (dxQuickStepperStage6CallContext *)_stage6CallContext;
    dxQuickStepIsland_Stage6_VelocityCheck(stage6CallContext);

    const dxStepperProcessingCallContext *callContext = stage6CallContext->m_stepperCallContext;

    const unsigned allowedThreads = callContext->m_stepperAllowedThreads;
    unsigned int nb = callContext->m_islandBodiesCount;
    unsigned int stage6b_allowedThreads = CalculateOptimalThreadsCount<dxQUICKSTEPISLAND_STAGE6B_STEP>(nb, allowedThreads);

    if (stage6b_allowedThreads > 1) {
        dxWorld *world = callContext->m_world;
        world->AlterThreadedCallDependenciesCount(callThisReleasee, stage6b_allowedThreads - 1);
        world->PostThreadedCallsGroup(NULL, stage6b_allowedThreads - 1, callThisReleasee, &dxQuickStepIsland_Stage6b_Callback, stage6CallContext, "QuickStepIsland Stage6b");
    }
    dxQuickStepIsland_Stage6b(stage6CallContext);

    return 1;
}

static 
void dxQuickStepIsland_Stage6_VelocityCheck(dxQuickStepperStage6CallContext *stage6CallContext)
{
    (void)stage6CallContext; // can be unused
#ifdef CHECK_VELOCITY_OBEYS_CONSTRAINT
    const dxQuickStepperLocalContext *localContext = stage6CallContext->m_localContext;

    unsigned int m = localContext->m_m;
    if (m > 0) {
        const dxStepperProcessingCallContext *callContext = stage6CallContext->m_stepperCallContext;
        dxBody * const *body = callContext->m_islandBodiesStart;
        dReal *J = localContext->m_J;
        const dxJBodiesItem *jb = localContext->m_jb;

        dReal error = 0;
        const dReal* J_ptr = J;
        for (unsigned int i = 0; i < m; ++i) {
            int b1 = jb[i].first;
            int b2 = jb[i].second;
            dReal sum = 0;
            dxBody *bodycurr = body[(unsigned)b1];
            for (unsigned int j = dSA__MIN; j != dSA__MAX; ++j) sum += J_ptr[JME__J1L_MIN + j] * bodycurr->lvel[dV3E__AXES_MIN + j] + J_ptr[JME__J1A_MIN + j] * bodycurr->avel[dV3E__AXES_MIN + j];
            if (b2 != -1) {
                dxBody *bodycurr = body[(unsigned)b2];
                for (unsigned int k = dSA__MIN; k != dSA__MAX; ++k) sum += J_ptr[JME__J2L_MIN + k] * bodycurr->lvel[dV3E__AXES_MIN + k] + J_ptr[JME__J2A_MIN + k] * bodycurr->avel[dV3E__AXES_MIN + k];
            }
            J_ptr += JME__MAX;
            error += dFabs(sum);
        }
        printf ("velocity error = %10.6e\n", error);
    }
#endif
}

static 
int dxQuickStepIsland_Stage6b_Callback(void *_stage6CallContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxQuickStepperStage6CallContext *stage6CallContext = (dxQuickStepperStage6CallContext *)_stage6CallContext;
    dxQuickStepIsland_Stage6b(stage6CallContext);
    return 1;
}

static 
void dxQuickStepIsland_Stage6b(dxQuickStepperStage6CallContext *stage6CallContext)
{
    const dxStepperProcessingCallContext *callContext = stage6CallContext->m_stepperCallContext;

    dReal stepsize = callContext->m_stepSize;
    dxBody * const *body = callContext->m_islandBodiesStart;

    // update the position and orientation from the new linear/angular velocity
    // (over the given timestep)
    unsigned int nb = callContext->m_islandBodiesCount;
    const unsigned int step_size = dxQUICKSTEPISLAND_STAGE6B_STEP;
    unsigned int nb_steps = (nb + (step_size - 1)) / step_size;

    unsigned bi_step;
    while ((bi_step = ThrsafeIncrementIntUpToLimit(&stage6CallContext->m_bi_6b, nb_steps)) != nb_steps) {
        unsigned int bi = bi_step * step_size;
        unsigned int bicnt = dMIN(step_size, nb - bi);

        dxBody *const *bodycurr = body + bi;
        dxBody *const *bodyend = bodycurr + bicnt;
        while (true) {
            dxBody *b = *bodycurr;
            dxStepBody (b, stepsize);
            dZeroVector3 (b->facc);
            dZeroVector3 (b->tacc);
            if (++bodycurr == bodyend) {
                break;
            }
        }
    }
}



/*extern */
size_t dxEstimateQuickStepMemoryRequirements (dxBody * const *body,
                                              unsigned int nb,
                                              dxJoint * const *_joint,
                                              unsigned int _nj)
{
    (void)body; // unused
    unsigned int nj, m, mfb;

    {
        unsigned int njcurr = 0, mcurr = 0, mfbcurr = 0;
        dxJoint::SureMaxInfo info;
        dxJoint *const *const _jend = _joint + _nj;
        for (dxJoint *const *_jcurr = _joint; _jcurr != _jend; _jcurr++) {	
            dxJoint *j = *_jcurr;
            j->getSureMaxInfo (&info);

            unsigned int jm = info.max_m;
            if (jm > 0) {
                njcurr++;

                mcurr += jm;
                if (j->feedback)
                    mfbcurr += jm;
            }
        }
        nj = njcurr; m = mcurr; mfb = mfbcurr;
    }

    size_t res = 0;

    res += dOVERALIGNED_SIZE(sizeof(dReal) * IIE__MAX * nb, INVI_ALIGNMENT); // for invI

    {
        size_t sub1_res1 = dEFFICIENT_SIZE(sizeof(dJointWithInfo1) * _nj); // for initial jointinfos

        size_t sub1_res2 = dEFFICIENT_SIZE(sizeof(dJointWithInfo1) * nj); // for shrunk jointinfos
        sub1_res2 += dEFFICIENT_SIZE(sizeof(dxQuickStepperLocalContext)); // for dxQuickStepLocalContext
        if (m > 0) {
            sub1_res2 += dEFFICIENT_SIZE(sizeof(dxMIndexItem) * (nj + 1)); // for mindex
            sub1_res2 += dEFFICIENT_SIZE(sizeof(dxJBodiesItem) * m); // for jb
            sub1_res2 += dEFFICIENT_SIZE(sizeof(int) * m); // for findex
            sub1_res2 += dOVERALIGNED_SIZE(sizeof(dReal) * JME__MAX * m, JACOBIAN_ALIGNMENT); // for J
            sub1_res2 += dOVERALIGNED_SIZE(sizeof(dReal) * JCE__MAX * mfb, JCOPY_ALIGNMENT); // for Jcopy
            {
                size_t sub2_res1 = dEFFICIENT_SIZE(sizeof(dxQuickStepperStage3CallContext)); // for dxQuickStepperStage3CallContext
                sub2_res1 += dEFFICIENT_SIZE(sizeof(dReal) * RHS__MAX * nb); // for rhs_tmp
                sub2_res1 += dEFFICIENT_SIZE(sizeof(dxQuickStepperStage2CallContext)); // for dxQuickStepperStage2CallContext

                size_t sub2_res2 = 0;
                {
                    size_t sub3_res1 = dEFFICIENT_SIZE(sizeof(dxQuickStepperStage5CallContext)); // for dxQuickStepperStage5CallContext;
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(dReal) * m); // for lambda
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(dReal) * CFE__MAX * nb); // for cforce
                    sub3_res1 += dOVERALIGNED_SIZE(sizeof(dReal) * IMJ__MAX * m, INVMJ_ALIGNMENT); // for iMJ
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(IndexError) * m); // for order
#if CONSTRAINTS_REORDERING_METHOD == REORDERING_METHOD__BY_ERROR
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(dReal) * m); // for last_lambda
#endif
#if !dTHREADING_INTF_DISABLED
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(atomicord32) * dMAX(nb, m)); // for bi_links_or_mi_levels
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(atomicord32) * 2 * ((size_t)m + 1)); // for mi_links
#endif
                    sub3_res1 += dEFFICIENT_SIZE(sizeof(dxQuickStepperStage4CallContext)); // for dxQuickStepperStage4CallContext;

                    size_t sub3_res2 = dEFFICIENT_SIZE(sizeof(dxQuickStepperStage6CallContext)); // for dxQuickStepperStage6CallContext;
                    
                    sub2_res2 += dMAX(sub3_res1, sub3_res2);
                }

                sub1_res2 += dMAX(sub2_res1, sub2_res2);
            }
        }
        else {
            sub1_res2 += dEFFICIENT_SIZE(sizeof(dxQuickStepperStage3CallContext)); // for dxQuickStepperStage3CallContext
        }

        size_t sub1_res12_max = dMAX(sub1_res1, sub1_res2);
        size_t stage01_contexts = dEFFICIENT_SIZE(sizeof(dxQuickStepperStage0BodiesCallContext))
            + dEFFICIENT_SIZE(sizeof(dxQuickStepperStage0JointsCallContext))
            + dEFFICIENT_SIZE(sizeof(dxQuickStepperStage1CallContext));
        res += dMAX(sub1_res12_max, stage01_contexts);
    }

    return res;
}

/*extern */
unsigned dxEstimateQuickStepMaxCallCount(unsigned activeThreadCount, unsigned allowedThreadCount)
{
    (void)activeThreadCount; // unused
    unsigned result = 1 // dxQuickStepIsland itself
        + 5 + (2 * allowedThreadCount + 1) // for Stage4 related schedules
        + 1 // dxStepIsland_Stage5
        + allowedThreadCount; // Reserve
    return result;
}

