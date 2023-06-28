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

#include <ode/ode.h>
#include "config.h"
#include "objects.h"
#include "joints/joint.h"
#include "util.h"
#include "threadingutils.h"

#include <new>


#define dMIN(A,B)  ((A)>(B) ? (B) : (A))
#define dMAX(A,B)  ((B)>(A) ? (B) : (A))


//****************************************************************************
// Malloc based world stepping memory manager

/*extern */dxWorldProcessMemoryManager g_WorldProcessMallocMemoryManager(dAlloc, dRealloc, dFree);
/*extern */dxWorldProcessMemoryReserveInfo g_WorldProcessDefaultReserveInfo(dWORLDSTEP_RESERVEFACTOR_DEFAULT, dWORLDSTEP_RESERVESIZE_DEFAULT);


//****************************************************************************
// dxWorldProcessContext

const char *const dxWorldProcessContext::m_aszContextMutexNames[dxPCM__MAX] = 
{
    "Stepper Arena Obtain Lock" , // dxPCM_STEPPER_ARENA_OBTAIN,
    "Joint addLimot Serialize Lock" , // dxPCM_STEPPER_ADDLIMOT_SERIALIZE
    "Stepper StepBody Serialize Lock" , // dxPCM_STEPPER_STEPBODY_SERIALIZE,
};

dxWorldProcessContext::dxWorldProcessContext():
    m_pmaIslandsArena(NULL),
    m_pmaStepperArenas(NULL),
    m_pswObjectsAllocWorld(NULL),
    m_pmgStepperMutexGroup(NULL),
    m_pcwIslandsSteppingWait(NULL)
{
    // Do nothing
}

dxWorldProcessContext::~dxWorldProcessContext()
{
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pmgStepperMutexGroup != NULL));
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pcwIslandsSteppingWait != NULL));

    if (m_pswObjectsAllocWorld != NULL)
    {
        m_pswObjectsAllocWorld->FreeMutexGroup(m_pmgStepperMutexGroup);
        m_pswObjectsAllocWorld->FreeThreadedCallWait(m_pcwIslandsSteppingWait);
    }

    dxWorldProcessMemArena *pmaStepperArenas = m_pmaStepperArenas;
    if (pmaStepperArenas != NULL)
    {
        FreeArenasList(pmaStepperArenas);
    }

    if (m_pmaIslandsArena != NULL)
    {
        dxWorldProcessMemArena::FreeMemArena(m_pmaIslandsArena);
    }
}

void dxWorldProcessContext::CleanupWorldReferences(dxWorld *pswWorldInstance)
{
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pmgStepperMutexGroup != NULL));
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pcwIslandsSteppingWait != NULL));

    if (m_pswObjectsAllocWorld == pswWorldInstance)
    {
        m_pswObjectsAllocWorld->FreeMutexGroup(m_pmgStepperMutexGroup);
        m_pswObjectsAllocWorld->FreeThreadedCallWait(m_pcwIslandsSteppingWait);

        m_pswObjectsAllocWorld = NULL;
        m_pmgStepperMutexGroup = NULL;
        m_pcwIslandsSteppingWait = NULL;
    }
}

bool dxWorldProcessContext::EnsureStepperSyncObjectsAreAllocated(dxWorld *pswWorldInstance)
{
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pmgStepperMutexGroup != NULL));
    dIASSERT((m_pswObjectsAllocWorld != NULL) == (m_pcwIslandsSteppingWait != NULL));

    bool bResult = false;

    dMutexGroupID pmbStepperMutexGroup = NULL;
    bool bStepperMutexGroupAllocated = false;

    do
    {
        if (m_pswObjectsAllocWorld == NULL)
        {
            pmbStepperMutexGroup = pswWorldInstance->AllocMutexGroup(dxPCM__MAX, m_aszContextMutexNames);
            if (!pmbStepperMutexGroup)
            {
                break;
            }

            bStepperMutexGroupAllocated = true;

            dCallWaitID pcwIslandsSteppingWait = pswWorldInstance->AllocThreadedCallWait();
            if (!pcwIslandsSteppingWait)
            {
                break;
            }

            m_pswObjectsAllocWorld = pswWorldInstance;
            m_pmgStepperMutexGroup = pmbStepperMutexGroup;
            m_pcwIslandsSteppingWait = pcwIslandsSteppingWait;
        }

        bResult = true;
    }
    while (false);

    if (!bResult)
    {
        if (bStepperMutexGroupAllocated)
        {
            pswWorldInstance->FreeMutexGroup(pmbStepperMutexGroup);
        }
    }

    return bResult;
}


dxWorldProcessMemArena *dxWorldProcessContext::ObtainStepperMemArena()
{
    dxWorldProcessMemArena *pmaArenaInstance = NULL;

    while (true)
    {
        dxWorldProcessMemArena *pmaRawArenasHead = GetStepperArenasHead();
        if (pmaRawArenasHead == NULL)
        {
            break;
        }

        // Extraction must be locked so that other thread does not "steal" head arena,
        // use it and then reinsert back with a different "next"
        dxMutexGroupLockHelper lhLockHelper(m_pswObjectsAllocWorld, m_pmgStepperMutexGroup, dxPCM_STEPPER_ARENA_OBTAIN);

        dxWorldProcessMemArena *pmaArenasHead = GetStepperArenasHead(); // Arenas head must be re-extracted after mutex has been locked
        bool bExchangeResult = pmaArenasHead != NULL && TryExtractingStepperArenasHead(pmaArenasHead);

        lhLockHelper.UnlockMutex();

        if (bExchangeResult)
        {
            pmaArenasHead->ResetState();
            pmaArenaInstance = pmaArenasHead;
            break;
        }
    }

    return pmaArenaInstance;
}

void dxWorldProcessContext::ReturnStepperMemArena(dxWorldProcessMemArena *pmaArenaInstance)
{
    while (true)
    {
        dxWorldProcessMemArena *pmaArenasHead = GetStepperArenasHead();
        pmaArenaInstance->SetNextMemArena(pmaArenasHead);

        if (TryInsertingStepperArenasHead(pmaArenaInstance, pmaArenasHead))
        {
            break;
        }
    }
}


dxWorldProcessMemArena *dxWorldProcessContext::ReallocateIslandsMemArena(size_t nMemoryRequirement, 
    const dxWorldProcessMemoryManager *pmmMemortManager, float fReserveFactor, unsigned uiReserveMinimum)
{
    dxWorldProcessMemArena *pmaExistingArena = GetIslandsMemArena();
    dxWorldProcessMemArena *pmaNewMemArena = dxWorldProcessMemArena::ReallocateMemArena(pmaExistingArena, nMemoryRequirement, pmmMemortManager, fReserveFactor, uiReserveMinimum);
    SetIslandsMemArena(pmaNewMemArena);

    pmaNewMemArena->ResetState();

    return pmaNewMemArena;
}

bool dxWorldProcessContext::ReallocateStepperMemArenas(
    dxWorld *world, unsigned nIslandThreadsCount, size_t nMemoryRequirement, 
    const dxWorldProcessMemoryManager *pmmMemortManager, float fReserveFactor, unsigned uiReserveMinimum)
{
    dxWorldProcessMemArena *pmaRebuiltArenasHead = NULL, *pmaRebuiltArenasTail = NULL;
    dxWorldProcessMemArena *pmaExistingArenas = GetStepperArenasList();
    unsigned nArenasToProcess = nIslandThreadsCount;

    (void)world; // unused

    // NOTE!
    // The list is reallocated in a way to assure the largest arenas are at end 
    // and if number of threads decreases they will be freed first of all.

    while (true)
    {
        if (nArenasToProcess == 0)
        {
            FreeArenasList(pmaExistingArenas);
            break;
        }

        dxWorldProcessMemArena *pmaOldMemArena = pmaExistingArenas;

        if (pmaExistingArenas != NULL)
        {
            pmaExistingArenas = pmaExistingArenas->GetNextMemArena();
        }
        else
        {
            // If existing arenas ended, terminate and erase tail so that new arenas 
            // would be appended to list head.
            if (pmaRebuiltArenasTail != NULL)
            {
                pmaRebuiltArenasTail->SetNextMemArena(NULL);
                pmaRebuiltArenasTail = NULL;
            }
        }

        dxWorldProcessMemArena *pmaNewMemArena = dxWorldProcessMemArena::ReallocateMemArena(pmaOldMemArena, nMemoryRequirement, pmmMemortManager, fReserveFactor, uiReserveMinimum);

        if (pmaNewMemArena != NULL)
        {
            // Append reallocated arenas to list tail while old arenas still exist...
            if (pmaRebuiltArenasTail != NULL)
            {
                pmaRebuiltArenasTail->SetNextMemArena(pmaNewMemArena);
                pmaRebuiltArenasTail = pmaNewMemArena;
            }
            else if (pmaRebuiltArenasHead == NULL)
            {
                pmaRebuiltArenasHead = pmaNewMemArena;
                pmaRebuiltArenasTail = pmaNewMemArena;
            }
            // ...and append them to list head if those are additional arenas created
            else
            {
                pmaNewMemArena->SetNextMemArena(pmaRebuiltArenasHead);
                pmaRebuiltArenasHead = pmaNewMemArena;
            }

            --nArenasToProcess;
        }
        else if (pmaOldMemArena == NULL)
        {
            break;
        }
    }

    if (pmaRebuiltArenasTail != NULL)
    {
        pmaRebuiltArenasTail->SetNextMemArena(NULL);
    }

    SetStepperArenasList(pmaRebuiltArenasHead);

    bool bResult = nArenasToProcess == 0;
    return bResult;
}

void dxWorldProcessContext::FreeArenasList(dxWorldProcessMemArena *pmaExistingArenas)
{
    while (pmaExistingArenas != NULL)
    {
        dxWorldProcessMemArena *pmaCurrentMemArena = pmaExistingArenas;
        pmaExistingArenas = pmaExistingArenas->GetNextMemArena();

        dxWorldProcessMemArena::FreeMemArena(pmaCurrentMemArena);
    }
}

dxWorldProcessMemArena *dxWorldProcessContext::GetStepperArenasHead() const
{
    return m_pmaStepperArenas;
}

bool dxWorldProcessContext::TryExtractingStepperArenasHead(dxWorldProcessMemArena *pmaHeadInstance)
{
    dxWorldProcessMemArena *pmaNextInstance = pmaHeadInstance->GetNextMemArena();
    return ThrsafeCompareExchangePointer((volatile atomicptr *)&m_pmaStepperArenas, (atomicptr)pmaHeadInstance, (atomicptr)pmaNextInstance);
}

bool dxWorldProcessContext::TryInsertingStepperArenasHead(dxWorldProcessMemArena *pmaArenaInstance, dxWorldProcessMemArena *pmaExistingHead)
{
    return ThrsafeCompareExchangePointer((volatile atomicptr *)&m_pmaStepperArenas, (atomicptr)pmaExistingHead, (atomicptr)pmaArenaInstance);
}


void dxWorldProcessContext::LockForAddLimotSerialization()
{
    m_pswObjectsAllocWorld->LockMutexGroupMutex(m_pmgStepperMutexGroup, dxPCM_STEPPER_ADDLIMOT_SERIALIZE);
}

void dxWorldProcessContext::UnlockForAddLimotSerialization()
{
    m_pswObjectsAllocWorld->UnlockMutexGroupMutex(m_pmgStepperMutexGroup, dxPCM_STEPPER_ADDLIMOT_SERIALIZE);
}


void dxWorldProcessContext::LockForStepbodySerialization()
{
    m_pswObjectsAllocWorld->LockMutexGroupMutex(m_pmgStepperMutexGroup, dxPCM_STEPPER_STEPBODY_SERIALIZE);
}

void dxWorldProcessContext::UnlockForStepbodySerialization()
{
    m_pswObjectsAllocWorld->UnlockMutexGroupMutex(m_pmgStepperMutexGroup, dxPCM_STEPPER_STEPBODY_SERIALIZE);
}


//****************************************************************************
// Threading call contexts

struct dxSingleIslandCallContext;

struct dxIslandsProcessingCallContext
{
    dxIslandsProcessingCallContext(dxWorld *world, const dxWorldProcessIslandsInfo &islandsInfo, dReal stepSize, dstepper_fn_t stepper):
        m_world(world), m_islandsInfo(islandsInfo), m_stepSize(stepSize), m_stepper(stepper),
        m_groupReleasee(NULL), m_islandToProcessStorage(0), m_stepperAllowedThreads(0)
    {
    }

    void AssignGroupReleasee(dCallReleaseeID groupReleasee) { m_groupReleasee = groupReleasee; }
    void SetStepperAllowedThreads(unsigned allowedThreadsLimit) { m_stepperAllowedThreads = allowedThreadsLimit; }

    static int ThreadedProcessGroup_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
    bool ThreadedProcessGroup();

    static int ThreadedProcessJobStart_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
    void ThreadedProcessJobStart();

    static int ThreadedProcessIslandSearch_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
    void ThreadedProcessIslandSearch(dxSingleIslandCallContext *stepperCallContext);

    static int ThreadedProcessIslandStepper_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee);
    void ThreadedProcessIslandStepper(dxSingleIslandCallContext *stepperCallContext);

    size_t ObtainNextIslandToBeProcessed(size_t islandsCount);

    dxWorld                         *const m_world;
    dxWorldProcessIslandsInfo const &m_islandsInfo;
    dReal                           const m_stepSize;
    dstepper_fn_t                   const m_stepper;
    dCallReleaseeID                 m_groupReleasee;
    size_t                          volatile m_islandToProcessStorage;
    unsigned                        m_stepperAllowedThreads;
};


struct dxSingleIslandCallContext
{
    dxSingleIslandCallContext(dxIslandsProcessingCallContext *islandsProcessingContext, 
        dxWorldProcessMemArena *stepperArena, void *arenaInitialState, 
        dxBody *const *islandBodiesStart, dxJoint *const *islandJointsStart):
        m_islandsProcessingContext(islandsProcessingContext), m_islandIndex(0), 
        m_stepperArena(stepperArena), m_arenaInitialState(arenaInitialState), 
        m_stepperCallContext(islandsProcessingContext->m_world, islandsProcessingContext->m_stepSize, islandsProcessingContext->m_stepperAllowedThreads, stepperArena, islandBodiesStart, islandJointsStart)
    {
    }

    void AssignIslandSearchProgress(size_t islandIndex)
    {
        m_islandIndex = islandIndex; 
    }

    void AssignIslandSelection(dxBody *const *islandBodiesStart, dxJoint *const *islandJointsStart, 
        unsigned islandBodiesCount, unsigned islandJointsCount)
    {
        m_stepperCallContext.AssignIslandSelection(islandBodiesStart, islandJointsStart, islandBodiesCount, islandJointsCount);
    }

    dxBody *const *GetSelectedIslandBodiesEnd() const { return m_stepperCallContext.GetSelectedIslandBodiesEnd(); }
    dxJoint *const *GetSelectedIslandJointsEnd() const { return m_stepperCallContext.GetSelectedIslandJointsEnd(); }
    
    void RestoreSavedMemArenaStateForStepper()
    {
        m_stepperArena->RestoreState(m_arenaInitialState);
    }

    void AssignStepperCallFinalReleasee(dCallReleaseeID finalReleasee)
    {
        m_stepperCallContext.AssignStepperCallFinalReleasee(finalReleasee);
    }

    dxIslandsProcessingCallContext  *m_islandsProcessingContext;
    size_t                          m_islandIndex;
    dxWorldProcessMemArena          *m_stepperArena;
    void                            *m_arenaInitialState;
    dxStepperProcessingCallContext  m_stepperCallContext;
};


//****************************************************************************
// Auto disabling

void dInternalHandleAutoDisabling (dxWorld *world, dReal stepsize)
{
    dxBody *bb;
    for ( bb=world->firstbody; bb; bb=(dxBody*)bb->next )
    {
        // don't freeze objects mid-air (patch 1586738)
        if ( bb->firstjoint == NULL ) continue;

        // nothing to do unless this body is currently enabled and has
        // the auto-disable flag set
        if ( (bb->flags & (dxBodyAutoDisable|dxBodyDisabled)) != dxBodyAutoDisable ) continue;

        // if sampling / threshold testing is disabled, we can never sleep.
        if ( bb->adis.average_samples == 0 ) continue;

        //
        // see if the body is idle
        //

#ifndef dNODEBUG
        // sanity check
        if ( bb->average_counter >= bb->adis.average_samples )
        {
            dUASSERT( bb->average_counter < bb->adis.average_samples, "buffer overflow" );

            // something is going wrong, reset the average-calculations
            bb->average_ready = 0; // not ready for average calculation
            bb->average_counter = 0; // reset the buffer index
        }
#endif // dNODEBUG

        // sample the linear and angular velocity
        bb->average_lvel_buffer[bb->average_counter][0] = bb->lvel[0];
        bb->average_lvel_buffer[bb->average_counter][1] = bb->lvel[1];
        bb->average_lvel_buffer[bb->average_counter][2] = bb->lvel[2];
        bb->average_avel_buffer[bb->average_counter][0] = bb->avel[0];
        bb->average_avel_buffer[bb->average_counter][1] = bb->avel[1];
        bb->average_avel_buffer[bb->average_counter][2] = bb->avel[2];
        bb->average_counter++;

        // buffer ready test
        if ( bb->average_counter >= bb->adis.average_samples )
        {
            bb->average_counter = 0; // fill the buffer from the beginning
            bb->average_ready = 1; // this body is ready now for average calculation
        }

        int idle = 0; // Assume it's in motion unless we have samples to disprove it.

        // enough samples?
        if ( bb->average_ready )
        {
            idle = 1; // Initial assumption: IDLE

            // the sample buffers are filled and ready for calculation
            dVector3 average_lvel, average_avel;

            // Store first velocity samples
            average_lvel[0] = bb->average_lvel_buffer[0][0];
            average_avel[0] = bb->average_avel_buffer[0][0];
            average_lvel[1] = bb->average_lvel_buffer[0][1];
            average_avel[1] = bb->average_avel_buffer[0][1];
            average_lvel[2] = bb->average_lvel_buffer[0][2];
            average_avel[2] = bb->average_avel_buffer[0][2];

            // If we're not in "instantaneous mode"
            if ( bb->adis.average_samples > 1 )
            {
                // add remaining velocities together
                for ( unsigned int i = 1; i < bb->adis.average_samples; ++i )
                {
                    average_lvel[0] += bb->average_lvel_buffer[i][0];
                    average_avel[0] += bb->average_avel_buffer[i][0];
                    average_lvel[1] += bb->average_lvel_buffer[i][1];
                    average_avel[1] += bb->average_avel_buffer[i][1];
                    average_lvel[2] += bb->average_lvel_buffer[i][2];
                    average_avel[2] += bb->average_avel_buffer[i][2];
                }

                // make average
                dReal r1 = dReal( 1.0 ) / dReal( bb->adis.average_samples );

                average_lvel[0] *= r1;
                average_avel[0] *= r1;
                average_lvel[1] *= r1;
                average_avel[1] *= r1;
                average_lvel[2] *= r1;
                average_avel[2] *= r1;
            }

            // threshold test
            dReal av_lspeed, av_aspeed;
            av_lspeed = dCalcVectorDot3( average_lvel, average_lvel );
            if ( av_lspeed > bb->adis.linear_average_threshold )
            {
                idle = 0; // average linear velocity is too high for idle
            }
            else
            {
                av_aspeed = dCalcVectorDot3( average_avel, average_avel );
                if ( av_aspeed > bb->adis.angular_average_threshold )
                {
                    idle = 0; // average angular velocity is too high for idle
                }
            }
        }

        // if it's idle, accumulate steps and time.
        // these counters won't overflow because this code doesn't run for disabled bodies.
        if (idle) {
            bb->adis_stepsleft--;
            bb->adis_timeleft -= stepsize;
        }
        else {
            // Reset countdowns
            bb->adis_stepsleft = bb->adis.idle_steps;
            bb->adis_timeleft = bb->adis.idle_time;
        }

        // disable the body if it's idle for a long enough time
        if ( bb->adis_stepsleft <= 0 && bb->adis_timeleft <= 0 )
        {
            bb->flags |= dxBodyDisabled; // set the disable flag

            // disabling bodies should also include resetting the velocity
            // should prevent jittering in big "islands"
            bb->lvel[0] = 0;
            bb->lvel[1] = 0;
            bb->lvel[2] = 0;
            bb->avel[0] = 0;
            bb->avel[1] = 0;
            bb->avel[2] = 0;
        }
    }
}


//****************************************************************************
// body rotation

// return sin(x)/x. this has a singularity at 0 so special handling is needed
// for small arguments.

static inline dReal sinc (dReal x)
{
    // if |x| < 1e-4 then use a taylor series expansion. this two term expansion
    // is actually accurate to one LS bit within this range if double precision
    // is being used - so don't worry!
    if (dFabs(x) < 1.0e-4) return REAL(1.0) - x*x*REAL(0.166666666666666666667);
    else return dSin(x)/x;
}


// given a body b, apply its linear and angular rotation over the time
// interval h, thereby adjusting its position and orientation.

void dxStepBody (dxBody *b, dReal h)
{
    // cap the angular velocity
    if (b->flags & dxBodyMaxAngularSpeed) {
        const dReal max_ang_speed = b->max_angular_speed;
        const dReal aspeed = dCalcVectorDot3( b->avel, b->avel );
        if (aspeed > max_ang_speed*max_ang_speed) {
            const dReal coef = max_ang_speed/dSqrt(aspeed);
            dScaleVector3(b->avel, coef);
        }
    }
    // end of angular velocity cap


    // handle linear velocity
    for (unsigned int j=0; j<3; j++) b->posr.pos[j] += h * b->lvel[j];

    if (b->flags & dxBodyFlagFiniteRotation) {
        dVector3 irv;	// infitesimal rotation vector
        dQuaternion q;	// quaternion for finite rotation

        if (b->flags & dxBodyFlagFiniteRotationAxis) {
            // split the angular velocity vector into a component along the finite
            // rotation axis, and a component orthogonal to it.
            dVector3 frv;		// finite rotation vector
            dReal k = dCalcVectorDot3 (b->finite_rot_axis,b->avel);
            frv[0] = b->finite_rot_axis[0] * k;
            frv[1] = b->finite_rot_axis[1] * k;
            frv[2] = b->finite_rot_axis[2] * k;
            irv[0] = b->avel[0] - frv[0];
            irv[1] = b->avel[1] - frv[1];
            irv[2] = b->avel[2] - frv[2];

            // make a rotation quaternion q that corresponds to frv * h.
            // compare this with the full-finite-rotation case below.
            h *= REAL(0.5);
            dReal theta = k * h;
            q[0] = dCos(theta);
            dReal s = sinc(theta) * h;
            q[1] = frv[0] * s;
            q[2] = frv[1] * s;
            q[3] = frv[2] * s;
        }
        else {
            // make a rotation quaternion q that corresponds to w * h
            dReal wlen = dSqrt (b->avel[0]*b->avel[0] + b->avel[1]*b->avel[1] +
                b->avel[2]*b->avel[2]);
            h *= REAL(0.5);
            dReal theta = wlen * h;
            q[0] = dCos(theta);
            dReal s = sinc(theta) * h;
            q[1] = b->avel[0] * s;
            q[2] = b->avel[1] * s;
            q[3] = b->avel[2] * s;
        }

        // do the finite rotation
        dQuaternion q2;
        dQMultiply0 (q2,q,b->q);
        for (unsigned int j=0; j<4; j++) b->q[j] = q2[j];

        // do the infitesimal rotation if required
        if (b->flags & dxBodyFlagFiniteRotationAxis) {
            dReal dq[4];
            dWtoDQ (irv,b->q,dq);
            for (unsigned int j=0; j<4; j++) b->q[j] += h * dq[j];
        }
    }
    else {
        // the normal way - do an infitesimal rotation
        dReal dq[4];
        dWtoDQ (b->avel,b->q,dq);
        for (unsigned int j=0; j<4; j++) b->q[j] += h * dq[j];
    }

    // normalize the quaternion and convert it to a rotation matrix
    dNormalize4 (b->q);
    dQtoR (b->q,b->posr.R);

    // notify all attached geoms that this body has moved
    dxWorldProcessContext *world_process_context = b->world->UnsafeGetWorldProcessingContext(); 
    for (dxGeom *geom = b->geom; geom; geom = dGeomGetBodyNext (geom)) {
        world_process_context->LockForStepbodySerialization();
        dGeomMoved (geom);
        world_process_context->UnlockForStepbodySerialization();
    }

    // notify the user
    if (b->moved_callback != NULL) {
        b->moved_callback(b);
    }

    // damping
    if (b->flags & dxBodyLinearDamping) {
        const dReal lin_threshold = b->dampingp.linear_threshold;
        const dReal lin_speed = dCalcVectorDot3( b->lvel, b->lvel );
        if ( lin_speed > lin_threshold) {
            const dReal k = 1 - b->dampingp.linear_scale;
            dScaleVector3(b->lvel, k);
        }
    }
    if (b->flags & dxBodyAngularDamping) {
        const dReal ang_threshold = b->dampingp.angular_threshold;
        const dReal ang_speed = dCalcVectorDot3( b->avel, b->avel );
        if ( ang_speed > ang_threshold) {
            const dReal k = 1 - b->dampingp.angular_scale;
            dScaleVector3(b->avel, k);
        }
    }
}


//****************************************************************************
// island processing

enum dxISLANDSIZESELEMENT
{
    dxISE_BODIES_COUNT,
    dxISE_JOINTS_COUNT,

    dxISE__MAX
};

// This estimates dynamic memory requirements for dxProcessIslands
static size_t EstimateIslandProcessingMemoryRequirements(dxWorld *world)
{
    size_t res = 0;

    size_t islandcounts = dEFFICIENT_SIZE((size_t)(unsigned)world->nb * 2 * sizeof(int));
    res += islandcounts;

    size_t bodiessize = dEFFICIENT_SIZE((size_t)(unsigned)world->nb * sizeof(dxBody*));
    size_t jointssize = dEFFICIENT_SIZE((size_t)(unsigned)world->nj * sizeof(dxJoint*));
    res += bodiessize + jointssize;

    size_t sesize = (bodiessize < jointssize) ? bodiessize : jointssize;
    res += sesize;

    return res;
}

static size_t BuildIslandsAndEstimateStepperMemoryRequirements(
    dxWorldProcessIslandsInfo &islandsinfo, dxWorldProcessMemArena *memarena, 
    dxWorld *world, dReal stepsize, dmemestimate_fn_t stepperestimate)
{
    size_t maxreq = 0;

    // handle auto-disabling of bodies
    dInternalHandleAutoDisabling (world,stepsize);

    unsigned int nb = world->nb, nj = world->nj;
    // Make array for island body/joint counts
    unsigned int *islandsizes = memarena->AllocateArray<unsigned int>(2 * (size_t)nb);
    unsigned int *sizescurr;

    // make arrays for body and joint lists (for a single island) to go into
    dxBody **body = memarena->AllocateArray<dxBody *>(nb);
    dxJoint **joint = memarena->AllocateArray<dxJoint *>(nj);

    BEGIN_STATE_SAVE(memarena, stackstate) {
        // allocate a stack of unvisited bodies in the island. the maximum size of
        // the stack can be the lesser of the number of bodies or joints, because
        // new bodies are only ever added to the stack by going through untagged
        // joints. all the bodies in the stack must be tagged!
        unsigned int stackalloc = (nj < nb) ? nj : nb;
        dxBody **stack = memarena->AllocateArray<dxBody *>(stackalloc);

        {
            // set all body/joint tags to 0
            for (dxBody *b=world->firstbody; b; b=(dxBody*)b->next) b->tag = 0;
            for (dxJoint *j=world->firstjoint; j; j=(dxJoint*)j->next) j->tag = 0;
        }

        sizescurr = islandsizes;
        dxBody **bodystart = body;
        dxJoint **jointstart = joint;
        for (dxBody *bb=world->firstbody; bb; bb=(dxBody*)bb->next) {
            // get bb = the next enabled, untagged body, and tag it
            if (!bb->tag) {
                if (!(bb->flags & dxBodyDisabled)) {
                    bb->tag = 1;

                    dxBody **bodycurr = bodystart;
                    dxJoint **jointcurr = jointstart;

                    // tag all bodies and joints starting from bb.
                    *bodycurr++ = bb;

                    unsigned int stacksize = 0;
                    dxBody *b = bb;

                    while (true) {
                        // traverse and tag all body's joints, add untagged connected bodies
                        // to stack
                        for (dxJointNode *n=b->firstjoint; n; n=n->next) {
                            dxJoint *njoint = n->joint;
                            if (!njoint->tag) {
                                if (njoint->isEnabled()) {
                                    njoint->tag = 1;
                                    *jointcurr++ = njoint;

                                    dxBody *nbody = n->body;
                                    // Body disabled flag is not checked here. This is how auto-enable works.
                                    if (nbody && nbody->tag <= 0) {
                                        nbody->tag = 1;
                                        // Make sure all bodies are in the enabled state.
                                        nbody->flags &= ~dxBodyDisabled;
                                        stack[stacksize++] = nbody;
                                    }
                                } else {
                                    njoint->tag = -1; // Used in Step to prevent search over disabled joints (not needed for QuickStep so far)
                                }
                            }
                        }
                        dIASSERT(stacksize <= (unsigned int)world->nb);
                        dIASSERT(stacksize <= (unsigned int)world->nj);

                        if (stacksize == 0) {
                            break;
                        }

                        b = stack[--stacksize];	// pop body off stack
                        *bodycurr++ = b;	// put body on body list
                    }

                    unsigned int bcount = (unsigned int)(bodycurr - bodystart);
                    unsigned int jcount = (unsigned int)(jointcurr - jointstart);
                    dIASSERT((size_t)(bodycurr - bodystart) <= (size_t)UINT_MAX);
                    dIASSERT((size_t)(jointcurr - jointstart) <= (size_t)UINT_MAX);

                    sizescurr[dxISE_BODIES_COUNT] = bcount;
                    sizescurr[dxISE_JOINTS_COUNT] = jcount;
                    sizescurr += dxISE__MAX;

                    size_t islandreq = stepperestimate(bodystart, bcount, jointstart, jcount);
                    maxreq = (maxreq > islandreq) ? maxreq : islandreq;

                    bodystart = bodycurr;
                    jointstart = jointcurr;
                } else {
                    bb->tag = -1; // Not used so far (assigned to retain consistency with joints)
                }
            }
        }
    } END_STATE_SAVE(memarena, stackstate);

# ifndef dNODEBUG
    // if debugging, check that all objects (except for disabled bodies,
    // unconnected joints, and joints that are connected to disabled bodies)
    // were tagged.
    {
        for (dxBody *b=world->firstbody; b; b=(dxBody*)b->next) {
            if (b->flags & dxBodyDisabled) {
                if (b->tag > 0) dDebug (0,"disabled body tagged");
            }
            else {
                if (b->tag <= 0) dDebug (0,"enabled body not tagged");
            }
        }
        for (dxJoint *j=world->firstjoint; j; j=(dxJoint*)j->next) {
            if ( (( j->node[0].body && (j->node[0].body->flags & dxBodyDisabled)==0 ) ||
                (j->node[1].body && (j->node[1].body->flags & dxBodyDisabled)==0) )
                && 
                j->isEnabled() ) {
                    if (j->tag <= 0) dDebug (0,"attached enabled joint not tagged");
            }
            else {
                if (j->tag > 0) dDebug (0,"unattached or disabled joint tagged");
            }
        }
    }
# endif

    size_t islandcount = ((size_t)(sizescurr - islandsizes) / dxISE__MAX);
    islandsinfo.AssignInfo(islandcount, islandsizes, body, joint);

    return maxreq;
}

static unsigned EstimateIslandProcessingSimultaneousCallsMaximumCount(unsigned activeThreadCount, unsigned islandsAllowedThreadCount, 
    unsigned stepperAllowedThreadCount, dmaxcallcountestimate_fn_t maxCallCountEstimator)
{
    unsigned stepperCallsMaximum = maxCallCountEstimator(activeThreadCount, stepperAllowedThreadCount);
    unsigned islandsIntermediateCallsMaximum = (1 + 2); // ThreadedProcessIslandSearch_Callback + (ThreadedProcessIslandStepper_Callback && ThreadedProcessIslandSearch_Callback)

    unsigned result = 
        1 // ThreadedProcessGroup_Callback
        + islandsAllowedThreadCount * dMAX(stepperCallsMaximum, islandsIntermediateCallsMaximum)
        + dMIN(islandsAllowedThreadCount, (unsigned)(activeThreadCount - islandsAllowedThreadCount)) // ThreadedProcessJobStart_Callback
        /*...the end*/;
    return result;
}

// this groups all joints and bodies in a world into islands. all objects
// in an island are reachable by going through connected bodies and joints.
// each island can be simulated separately.
// note that joints that are not attached to anything will not be included
// in any island, an so they do not affect the simulation.
//
// this function starts new island from unvisited bodies. however, it will
// never start a new islands from a disabled body. thus islands of disabled
// bodies will not be included in the simulation. disabled bodies are
// re-enabled if they are found to be part of an active island.
bool dxProcessIslands (dxWorld *world, const dxWorldProcessIslandsInfo &islandsInfo, 
    dReal stepSize, dstepper_fn_t stepper, dmaxcallcountestimate_fn_t maxCallCountEstimator)
{
    bool result = false;

    dxIslandsProcessingCallContext callContext(world, islandsInfo, stepSize, stepper);

    do {
        dxStepWorkingMemory *wmem = world->wmem;
        dIASSERT(wmem != NULL);
        dxWorldProcessContext *context = wmem->GetWorldProcessingContext(); 
        dIASSERT(context != NULL);
        dCallWaitID pcwGroupCallWait = context->GetIslandsSteppingWait();

        int summaryFault = 0;

        unsigned activeThreadCount;
        const unsigned islandsAllowedThreadCount = world->GetThreadingIslandsMaxThreadsCount(&activeThreadCount);
        dIASSERT(islandsAllowedThreadCount != 0);
        dIASSERT(activeThreadCount >= islandsAllowedThreadCount);

        unsigned stepperAllowedThreadCount = islandsAllowedThreadCount; // For now, set stepper allowed threads equal to island stepping threads

        unsigned simultaneousCallsCount = EstimateIslandProcessingSimultaneousCallsMaximumCount(activeThreadCount, islandsAllowedThreadCount, stepperAllowedThreadCount, maxCallCountEstimator);
        if (!world->PreallocateResourcesForThreadedCalls(simultaneousCallsCount)) {
            break;
        }

        dCallReleaseeID groupReleasee;
        // First post a group call with dependency count set to number of expected threads
        world->PostThreadedCall(&summaryFault, &groupReleasee, islandsAllowedThreadCount, NULL, pcwGroupCallWait, 
            &dxIslandsProcessingCallContext::ThreadedProcessGroup_Callback, (void *)&callContext, 0, "World Islands Stepping Group");

        callContext.AssignGroupReleasee(groupReleasee);
        callContext.SetStepperAllowedThreads(stepperAllowedThreadCount);

        // Summary fault flag may be omitted as any failures will automatically propagate to dependent releasee (i.e. to groupReleasee)
        world->PostThreadedCallsGroup(NULL, islandsAllowedThreadCount, groupReleasee, 
            &dxIslandsProcessingCallContext::ThreadedProcessJobStart_Callback, (void *)&callContext, "World Islands Stepping Start");

        // Wait until group completes (since jobs were the dependencies of the group the group is going to complete only after all the jobs end)
        world->WaitThreadedCallExclusively(NULL, pcwGroupCallWait, NULL, "World Islands Stepping Wait");

        if (summaryFault != 0) {
            break;
        }

        result = true;
    }
    while (false);

    return result;
}


int dxIslandsProcessingCallContext::ThreadedProcessGroup_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    return static_cast<dxIslandsProcessingCallContext *>(callContext)->ThreadedProcessGroup();
}

bool dxIslandsProcessingCallContext::ThreadedProcessGroup()
{
    // Do nothing - it's just a wrapper call
    return true;
}

int dxIslandsProcessingCallContext::ThreadedProcessJobStart_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    static_cast<dxIslandsProcessingCallContext *>(callContext)->ThreadedProcessJobStart();
    return true;
}

void dxIslandsProcessingCallContext::ThreadedProcessJobStart()
{
    dxWorldProcessContext *context = m_world->UnsafeGetWorldProcessingContext(); 

    dxWorldProcessMemArena *stepperArena = context->ObtainStepperMemArena();
    dIASSERT(stepperArena != NULL && stepperArena->IsStructureValid());

    const dxWorldProcessIslandsInfo &islandsInfo = m_islandsInfo;
    dxBody *const *islandBodiesStart = islandsInfo.GetBodiesArray();
    dxJoint *const *islandJointsStart = islandsInfo.GetJointsArray();

    dxSingleIslandCallContext *stepperCallContext = (dxSingleIslandCallContext *)stepperArena->AllocateBlock(sizeof(dxSingleIslandCallContext));
    // Save area state after context allocation to be restored for the stepper
    void *arenaState = stepperArena->SaveState();
    new(stepperCallContext) dxSingleIslandCallContext(this, stepperArena, arenaState, islandBodiesStart, islandJointsStart);

    // Summary fault flag may be omitted as any failures will automatically propagate to dependent releasee (i.e. to m_groupReleasee)
    m_world->PostThreadedCallForUnawareReleasee(NULL, NULL, 0, m_groupReleasee, NULL, 
        &dxIslandsProcessingCallContext::ThreadedProcessIslandSearch_Callback, (void *)stepperCallContext, 0, "World Islands Stepping Selection");
}

int dxIslandsProcessingCallContext::ThreadedProcessIslandSearch_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxSingleIslandCallContext *stepperCallContext = static_cast<dxSingleIslandCallContext *>(callContext);
    stepperCallContext->m_islandsProcessingContext->ThreadedProcessIslandSearch(stepperCallContext);
    return true;
}

void dxIslandsProcessingCallContext::ThreadedProcessIslandSearch(dxSingleIslandCallContext *stepperCallContext)
{
    bool finalizeJob = false;

    const dxWorldProcessIslandsInfo &islandsInfo = m_islandsInfo;
    unsigned int const *islandSizes = islandsInfo.GetIslandSizes();

    const size_t islandsCount = islandsInfo.GetIslandsCount();
    size_t islandToProcess = ObtainNextIslandToBeProcessed(islandsCount);

    if (islandToProcess != islandsCount) {
        // First time, the counts are zeros and on next passes, adding counts will skip island that has just been processed by stepper
        dxBody *const *islandBodiesStart = stepperCallContext->GetSelectedIslandBodiesEnd();
        dxJoint *const *islandJointsStart = stepperCallContext->GetSelectedIslandJointsEnd();
        size_t islandIndex = stepperCallContext->m_islandIndex;

        for (; ; ++islandIndex) {
            unsigned int bcount = islandSizes[islandIndex * dxISE__MAX + dxISE_BODIES_COUNT];
            unsigned int jcount = islandSizes[islandIndex * dxISE__MAX + dxISE_JOINTS_COUNT];

            if (islandIndex == islandToProcess) {
                // Store selected island details
                stepperCallContext->AssignIslandSelection(islandBodiesStart, islandJointsStart, bcount, jcount);

                // Store next island index to continue search from
                ++islandIndex;
                stepperCallContext->AssignIslandSearchProgress(islandIndex);

                // Restore saved stepper memory arena position
                stepperCallContext->RestoreSavedMemArenaStateForStepper();

                dCallReleaseeID nextSearchReleasee;

                // Summary fault flag may be omitted as any failures will automatically propagate to dependent releasee (i.e. to m_groupReleasee)
                m_world->PostThreadedCallForUnawareReleasee(NULL, &nextSearchReleasee, 1, m_groupReleasee, NULL, 
                    &dxIslandsProcessingCallContext::ThreadedProcessIslandSearch_Callback, (void *)stepperCallContext, 0, "World Islands Stepping Selection");

                stepperCallContext->AssignStepperCallFinalReleasee(nextSearchReleasee);

                m_world->PostThreadedCall(NULL, NULL, 0, nextSearchReleasee, NULL, 
                    &dxIslandsProcessingCallContext::ThreadedProcessIslandStepper_Callback, (void *)stepperCallContext, 0, "Island Stepping Job Start");

                break;
            }

            islandBodiesStart += bcount;
            islandJointsStart += jcount;
        }
    }
    else {
        finalizeJob = true;
    }

    if (finalizeJob) {
        dxWorldProcessMemArena *stepperArena = stepperCallContext->m_stepperArena;
        stepperCallContext->dxSingleIslandCallContext::~dxSingleIslandCallContext();

        dxWorldProcessContext *context = m_world->UnsafeGetWorldProcessingContext(); 
        context->ReturnStepperMemArena(stepperArena);
    }
}

int dxIslandsProcessingCallContext::ThreadedProcessIslandStepper_Callback(void *callContext, dcallindex_t callInstanceIndex, dCallReleaseeID callThisReleasee)
{
    (void)callInstanceIndex; // unused
    (void)callThisReleasee; // unused
    dxSingleIslandCallContext *stepperCallContext = static_cast<dxSingleIslandCallContext *>(callContext);
    stepperCallContext->m_islandsProcessingContext->ThreadedProcessIslandStepper(stepperCallContext);
    return true;
}

void dxIslandsProcessingCallContext::ThreadedProcessIslandStepper(dxSingleIslandCallContext *stepperCallContext)
{
    m_stepper(&stepperCallContext->m_stepperCallContext);
}

size_t dxIslandsProcessingCallContext::ObtainNextIslandToBeProcessed(size_t islandsCount)
{
    return ThrsafeIncrementSizeUpToLimit(&m_islandToProcessStorage, islandsCount);
}


//****************************************************************************
// World processing context management

dxWorldProcessMemArena *dxWorldProcessMemArena::ReallocateMemArena (
    dxWorldProcessMemArena *oldarena, size_t memreq, 
    const dxWorldProcessMemoryManager *memmgr, float rsrvfactor, unsigned rsrvminimum)
{
    dxWorldProcessMemArena *arena = oldarena;
    bool allocsuccess = false;

    size_t nOldArenaSize; 
    void *pOldArenaBuffer;

    do {
        size_t oldmemsize = oldarena ? oldarena->GetMemorySize() : 0;
        if (oldarena == NULL || oldmemsize < memreq) {
            nOldArenaSize = oldarena ? dxWorldProcessMemArena::MakeArenaSize(oldmemsize) : 0;
            pOldArenaBuffer = oldarena ? oldarena->m_pArenaBegin : NULL;

            if (!dxWorldProcessMemArena::IsArenaPossible(memreq)) {
                break;
            }

            size_t arenareq = dxWorldProcessMemArena::MakeArenaSize(memreq);
            size_t arenareq_with_reserve = AdjustArenaSizeForReserveRequirements(arenareq, rsrvfactor, rsrvminimum);
            size_t memreq_with_reserve = memreq + (arenareq_with_reserve - arenareq);

            if (oldarena != NULL) {
                oldarena->m_pArenaMemMgr->m_fnFree(pOldArenaBuffer, nOldArenaSize);
                oldarena = NULL;

                // Zero variables to avoid another freeing on exit
                pOldArenaBuffer = NULL;
                nOldArenaSize = 0;
            }

            // Allocate new arena
            void *pNewArenaBuffer = memmgr->m_fnAlloc(arenareq_with_reserve);
            if (pNewArenaBuffer == NULL) {
                break;
            }

            arena = (dxWorldProcessMemArena *)dEFFICIENT_PTR(pNewArenaBuffer);

            void *blockbegin = dEFFICIENT_PTR(arena + 1);
            void *blockend = dOFFSET_EFFICIENTLY(blockbegin, memreq_with_reserve);

            arena->m_pAllocBegin = blockbegin;
            arena->m_pAllocEnd = blockend;
            arena->m_pArenaBegin = pNewArenaBuffer;
            arena->m_pAllocCurrentOrNextArena = NULL;
            arena->m_pArenaMemMgr = memmgr;
        }

        allocsuccess = true;
    }
    while (false);

    if (!allocsuccess) {
        if (pOldArenaBuffer != NULL) {
            dIASSERT(oldarena != NULL);
            oldarena->m_pArenaMemMgr->m_fnFree(pOldArenaBuffer, nOldArenaSize);
        }
        arena = NULL;
    }

    return arena;
}

void dxWorldProcessMemArena::FreeMemArena (dxWorldProcessMemArena *arena)
{
    size_t memsize = arena->GetMemorySize();
    size_t arenasize = dxWorldProcessMemArena::MakeArenaSize(memsize);

    void *pArenaBegin = arena->m_pArenaBegin;
    arena->m_pArenaMemMgr->m_fnFree(pArenaBegin, arenasize);
}


size_t dxWorldProcessMemArena::AdjustArenaSizeForReserveRequirements(size_t arenareq, float rsrvfactor, unsigned rsrvminimum)
{
    float scaledarena = arenareq * rsrvfactor;
    size_t adjustedarena = (scaledarena < SIZE_MAX) ? (size_t)scaledarena : SIZE_MAX;
    size_t boundedarena = (adjustedarena > rsrvminimum) ? adjustedarena : (size_t)rsrvminimum;
    return dEFFICIENT_SIZE(boundedarena);
}


bool dxReallocateWorldProcessContext (dxWorld *world, dxWorldProcessIslandsInfo &islandsInfo, 
    dReal stepSize, dmemestimate_fn_t stepperEstimate)
{
    bool result = false;

    do
    {
        dxStepWorkingMemory *wmem = AllocateOnDemand(world->wmem);
        if (wmem == NULL)
        {
            break;
        }

        dxWorldProcessContext *context = wmem->SureGetWorldProcessingContext();
        if (context == NULL)
        {
            break;
        }

        if (!context->EnsureStepperSyncObjectsAreAllocated(world))
        {
            break;
        }

        const dxWorldProcessMemoryReserveInfo *reserveInfo = wmem->SureGetMemoryReserveInfo();
        const dxWorldProcessMemoryManager *memmgr = wmem->SureGetMemoryManager();

        size_t islandsReq = EstimateIslandProcessingMemoryRequirements(world);
        dIASSERT(islandsReq == dEFFICIENT_SIZE(islandsReq));

        dxWorldProcessMemArena *islandsArena = context->ReallocateIslandsMemArena(islandsReq, memmgr, 1.0f, reserveInfo->m_uiReserveMinimum);
        if (islandsArena == NULL)
        {
            break;
        }
        dIASSERT(islandsArena->IsStructureValid());

        size_t stepperReq = BuildIslandsAndEstimateStepperMemoryRequirements(islandsInfo, islandsArena, world, stepSize, stepperEstimate);
        dIASSERT(stepperReq == dEFFICIENT_SIZE(stepperReq));

        size_t stepperReqWithCallContext = stepperReq + dEFFICIENT_SIZE(sizeof(dxSingleIslandCallContext));

        unsigned islandThreadsCount = world->GetThreadingIslandsMaxThreadsCount();
        if (!context->ReallocateStepperMemArenas(world, islandThreadsCount, stepperReqWithCallContext, 
            memmgr, reserveInfo->m_fReserveFactor, reserveInfo->m_uiReserveMinimum))
        {
            break;
        }

        result = true;
    }
    while (false);

    return result;
}

dxWorldProcessMemArena *dxAllocateTemporaryWorldProcessMemArena(
    size_t memreq, const dxWorldProcessMemoryManager *memmgr/*=NULL*/, const dxWorldProcessMemoryReserveInfo *reserveinfo/*=NULL*/)
{
    const dxWorldProcessMemoryManager *surememmgr = memmgr ? memmgr : &g_WorldProcessMallocMemoryManager;
    const dxWorldProcessMemoryReserveInfo *surereserveinfo = reserveinfo ? reserveinfo : &g_WorldProcessDefaultReserveInfo;
    dxWorldProcessMemArena *arena = dxWorldProcessMemArena::ReallocateMemArena(NULL, memreq, surememmgr, surereserveinfo->m_fReserveFactor, surereserveinfo->m_uiReserveMinimum);
    return arena;
}

void dxFreeTemporaryWorldProcessMemArena(dxWorldProcessMemArena *arena)
{
    dxWorldProcessMemArena::FreeMemArena(arena);
}

