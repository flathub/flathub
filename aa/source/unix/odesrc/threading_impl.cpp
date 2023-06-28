/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading subsystem implementation file.                              *
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
 *  Subsystem APIs implementation for built-in threading support provider.
 */


#include <ode/common.h>
#include <ode/threading_impl.h>
#include "config.h"
#include "threading_impl_posix.h"
#include "threading_impl_win.h"
#include "threading_impl.h"


static dMutexGroupID AllocMutexGroup(dThreadingImplementationID impl, dmutexindex_t Mutex_count, const char *const *Mutex_names_ptr/*=NULL*/);
static void FreeMutexGroup(dThreadingImplementationID impl, dMutexGroupID mutex_group);
static void LockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);
// static int TryLockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);
static void UnlockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);

static dCallWaitID AllocThreadedCallWait(dThreadingImplementationID impl);
static void ResetThreadedCallWait(dThreadingImplementationID impl, dCallWaitID call_wait);
static void FreeThreadedCallWait(dThreadingImplementationID impl, dCallWaitID call_wait);

static void PostThreadedCall(
    dThreadingImplementationID impl, int *out_summary_fault/*=NULL*/, 
    dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dCallWaitID call_wait/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
    const char *call_name/*=NULL*/);
static void AlterThreadedCallDependenciesCount(
    dThreadingImplementationID impl, dCallReleaseeID target_releasee, 
    ddependencychange_t dependencies_count_change);
static void WaitThreadedCall(
    dThreadingImplementationID impl, int *out_wait_status/*=NULL*/, 
    dCallWaitID call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/, 
    const char *wait_name/*=NULL*/);

static unsigned RetrieveThreadingThreadCount(dThreadingImplementationID impl);
static int PreallocateResourcesForThreadedCalls(dThreadingImplementationID impl, ddependencycount_t max_simultaneous_calls_estimate);


static const dxThreadingFunctionsInfo g_builtin_threading_functions = 
{
    sizeof(dxThreadingFunctionsInfo), // unsigned struct_size;

    &AllocMutexGroup, // dMutexGroupAllocFunction *alloc_mutex_group;
    &FreeMutexGroup, // dMutexGroupFreeFunction *free_mutex_group;
    &LockMutexGroupMutex, // dMutexGroupMutexLockFunction *lock_group_mutex;
    &UnlockMutexGroupMutex, // dMutexGroupMutexUnlockFunction *unlock_group_mutex;

    &AllocThreadedCallWait, // dThreadedCallWaitAllocFunction *alloc_call_wait;
    &ResetThreadedCallWait, // dThreadedCallWaitResetFunction *reset_call_wait;
    &FreeThreadedCallWait, // dThreadedCallWaitFreeFunction *free_call_wait;

    &PostThreadedCall, // dThreadedCallPostFunction *post_call;
    &AlterThreadedCallDependenciesCount, // dThreadedCallDependenciesCountAlterFunction *alter_call_dependencies_count;
    &WaitThreadedCall, // dThreadedCallWaitFunction *wait_call;

    &RetrieveThreadingThreadCount, // dThreadingImplThreadCountRetrieveFunction *retrieve_thread_count;
    &PreallocateResourcesForThreadedCalls, // dThreadingImplResourcesForCallsPreallocateFunction *preallocate_resources_for_calls;

    // &TryLockMutexGroupMutex, // dMutexGroupMutexTryLockFunction *trylock_group_mutex;
};


/*extern */dThreadingImplementationID dThreadingAllocateSelfThreadedImplementation()
{
    dxSelfThreadedThreading *threading = new dxSelfThreadedThreading();

    if (threading != NULL && !threading->InitializeObject())
    {
        delete threading;
        threading = NULL;
    }

    dxIThreadingImplementation *impl = threading;
    return (dThreadingImplementationID)impl;
}

/*extern */dThreadingImplementationID dThreadingAllocateMultiThreadedImplementation()
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxMultiThreadedThreading *threading = new dxMultiThreadedThreading();

    if (threading != NULL && !threading->InitializeObject())
    {
        delete threading;
        threading = NULL;
    }
#else
    dxIThreadingImplementation *threading = NULL;
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

    dxIThreadingImplementation *impl = threading;
    return (dThreadingImplementationID)impl;
}

/*extern */const dThreadingFunctionsInfo *dThreadingImplementationGetFunctions(dThreadingImplementationID impl)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dAASSERT(impl != NULL);
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

    const dThreadingFunctionsInfo *functions = NULL;

#if !dBUILTIN_THREADING_IMPL_ENABLED
    if (impl != NULL)
#endif // #if !dBUILTIN_THREADING_IMPL_ENABLED
    {
        functions = &g_builtin_threading_functions;
    }

    return functions;
}

/*extern */void dThreadingImplementationShutdownProcessing(dThreadingImplementationID impl)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dAASSERT(impl != NULL);
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

#if !dBUILTIN_THREADING_IMPL_ENABLED
    if (impl != NULL)
#endif // #if !dBUILTIN_THREADING_IMPL_ENABLED
    {
        ((dxIThreadingImplementation *)impl)->ShutdownProcessing();
    }
}

/*extern */void dThreadingImplementationCleanupForRestart(dThreadingImplementationID impl)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dAASSERT(impl != NULL);
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

#if !dBUILTIN_THREADING_IMPL_ENABLED
    if (impl != NULL)
#endif // #if !dBUILTIN_THREADING_IMPL_ENABLED
    {
        ((dxIThreadingImplementation *)impl)->CleanupForRestart();
    }
}

/*extern */void dThreadingFreeImplementation(dThreadingImplementationID impl)
{
    if (impl != NULL)
    {
        ((dxIThreadingImplementation *)impl)->FreeInstance();
    }
}


/*extern */void dExternalThreadingServeMultiThreadedImplementation(dThreadingImplementationID impl, 
                                                                   dThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dAASSERT(impl != NULL);
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

#if !dBUILTIN_THREADING_IMPL_ENABLED
    if (impl != NULL)
#endif // #if !dBUILTIN_THREADING_IMPL_ENABLED
    {
        ((dxIThreadingImplementation *)impl)->StickToJobsProcessing(readiness_callback, callback_context);
    }
}


//////////////////////////////////////////////////////////////////////////

static dMutexGroupID AllocMutexGroup(dThreadingImplementationID impl, dmutexindex_t Mutex_count, const char *const *Mutex_names_ptr/*=NULL*/)
{
    (void)Mutex_names_ptr; // unused
    dIMutexGroup *mutex_group = ((dxIThreadingImplementation *)impl)->AllocMutexGroup(Mutex_count);
    return (dMutexGroupID)mutex_group;
}

static void FreeMutexGroup(dThreadingImplementationID impl, dMutexGroupID mutex_group)
{
    ((dxIThreadingImplementation *)impl)->FreeMutexGroup((dIMutexGroup *)mutex_group);
}

static void LockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index)
{
    ((dxIThreadingImplementation *)impl)->LockMutexGroupMutex((dIMutexGroup *)mutex_group, mutex_index);
}

// static int TryLockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index)
// {
//   bool trylock_result = ((dxIThreadingImplementation *)impl)->TryLockMutexGroupMutex((dIMutexGroup *)mutex_group, mutex_index);
//   return trylock_result;
// }

static void UnlockMutexGroupMutex(dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index)
{
    ((dxIThreadingImplementation *)impl)->UnlockMutexGroupMutex((dIMutexGroup *)mutex_group, mutex_index);
}


static dCallWaitID AllocThreadedCallWait(dThreadingImplementationID impl)
{
    dxICallWait *call_wait = ((dxIThreadingImplementation *)impl)->AllocACallWait();
    return (dCallWaitID)call_wait;
}

static void ResetThreadedCallWait(dThreadingImplementationID impl, dCallWaitID call_wait)
{
    ((dxIThreadingImplementation *)impl)->ResetACallWait((dxICallWait *)call_wait);
}

static void FreeThreadedCallWait(dThreadingImplementationID impl, dCallWaitID call_wait)
{
    ((dxIThreadingImplementation *)impl)->FreeACallWait((dxICallWait *)call_wait);
}


static void PostThreadedCall(
    dThreadingImplementationID impl, int *out_summary_fault/*=NULL*/, 
    dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dCallWaitID call_wait/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
    const char *call_name/*=NULL*/)
{
    (void)call_name; // unused
    ((dxIThreadingImplementation *)impl)->ScheduleNewJob(out_summary_fault, out_post_releasee, 
        dependencies_count, dependent_releasee, (dxICallWait *)call_wait, call_func, call_context, instance_index);
}

static void AlterThreadedCallDependenciesCount(
    dThreadingImplementationID impl, dCallReleaseeID target_releasee, 
    ddependencychange_t dependencies_count_change)
{
    ((dxIThreadingImplementation *)impl)->AlterJobDependenciesCount(target_releasee, dependencies_count_change);
}

static void WaitThreadedCall(
    dThreadingImplementationID impl, int *out_wait_status/*=NULL*/, 
    dCallWaitID call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/, 
    const char *wait_name/*=NULL*/)
{
    (void)wait_name; // unused
    ((dxIThreadingImplementation *)impl)->WaitJobCompletion(out_wait_status, (dxICallWait *)call_wait, timeout_time_ptr);
}


static unsigned RetrieveThreadingThreadCount(dThreadingImplementationID impl)
{
    return ((dxIThreadingImplementation *)impl)->RetrieveActiveThreadsCount();
}

static int PreallocateResourcesForThreadedCalls(dThreadingImplementationID impl, ddependencycount_t max_simultaneous_calls_estimate)
{
    return ((dxIThreadingImplementation *)impl)->PreallocateJobInfos(max_simultaneous_calls_estimate);
}


