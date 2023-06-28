/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading base wrapper class header file.                             *
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
 * Threading base class to be used for inheritance by dxWorld, dxSpace and others 
 * to take advantage of threaded execution.
 */


#ifndef _ODE_THREADING_BASE_H_
#define _ODE_THREADING_BASE_H_


#include <ode/threading.h>


struct dxThreadingBase;

struct dxIThreadingDefaultImplProvider
{
public:
    virtual const dxThreadingFunctionsInfo *RetrieveThreadingDefaultImpl(dThreadingImplementationID &out_default_impl) = 0;
};


struct dxThreadingBase
{
protected:
    dxThreadingBase():
         m_default_impl_provider(NULL),
         m_functions_info(NULL), 
         m_threading_impl(NULL)
     {
     }

     // This ought to be done via constructor, but passing 'this' in base class initializer emits a warning in MSVC :(
     void SetThreadingDefaultImplProvider(dxIThreadingDefaultImplProvider *default_impl_provider) { m_default_impl_provider = default_impl_provider; }

public:
    void AssignThreadingImpl(const dxThreadingFunctionsInfo *functions_info, dThreadingImplementationID threading_impl)
    {
        dAASSERT((functions_info == NULL) == (threading_impl == NULL));

        m_functions_info = functions_info;
        m_threading_impl = threading_impl;
    }

public:
    dMutexGroupID AllocMutexGroup(dmutexindex_t Mutex_count, const char *const *Mutex_names_ptr/*=NULL*/) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        return functions->alloc_mutex_group(impl, Mutex_count, Mutex_names_ptr);
    }

    void FreeMutexGroup(dMutexGroupID mutex_group) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->free_mutex_group(impl, mutex_group);
    }

    void LockMutexGroupMutex(dMutexGroupID mutex_group, dmutexindex_t mutex_index) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->lock_group_mutex(impl, mutex_group, mutex_index);
    }

//     bool TryLockMutexGroupMutex(dMutexGroupID mutex_group, dmutexindex_t mutex_index) const
//     {
//         dThreadingImplementationID impl;
//         const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
//         return functions->trylock_group_mutex(impl, mutex_group, mutex_index) != 0;
//     }

    void UnlockMutexGroupMutex(dMutexGroupID mutex_group, dmutexindex_t mutex_index) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->unlock_group_mutex(impl, mutex_group, mutex_index);
    }

    dCallWaitID AllocThreadedCallWait() const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        return functions->alloc_call_wait(impl);
    }

    void ResetThreadedCallWait(dCallWaitID call_wait) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->reset_call_wait(impl, call_wait);
    }

    void FreeThreadedCallWait(dCallWaitID call_wait) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->free_call_wait(impl, call_wait);
    }

    void PostThreadedCall(int *out_summary_fault/*=NULL*/, 
        dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dCallWaitID call_wait/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
        const char *call_name/*=NULL*/) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->post_call(impl, out_summary_fault, out_post_releasee, dependencies_count, dependent_releasee, call_wait, call_func, call_context, instance_index, call_name);
    }

    void AlterThreadedCallDependenciesCount(dCallReleaseeID target_releasee, 
        ddependencychange_t dependencies_count_change) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->alter_call_dependencies_count(impl, target_releasee, dependencies_count_change);
    }

    void WaitThreadedCallExclusively(int *out_wait_status/*=NULL*/, 
        dCallWaitID call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/, 
        const char *wait_name/*=NULL*/) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->wait_call(impl, out_wait_status, call_wait, timeout_time_ptr, wait_name);
        functions->reset_call_wait(impl, call_wait);
    }

    void WaitThreadedCallCollectively(int *out_wait_status/*=NULL*/, 
        dCallWaitID call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/, 
        const char *wait_name/*=NULL*/) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        functions->wait_call(impl, out_wait_status, call_wait, timeout_time_ptr, wait_name);
    }

    unsigned RetrieveThreadingThreadCount() const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        return functions->retrieve_thread_count(impl);
    }

    bool PreallocateResourcesForThreadedCalls(unsigned max_simultaneous_calls_estimate) const
    {
        dThreadingImplementationID impl;
        const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);
        return functions->preallocate_resources_for_calls(impl, max_simultaneous_calls_estimate) != 0;
    }

public:
    void PostThreadedCallsGroup(int *out_summary_fault/*=NULL*/, 
        ddependencycount_t member_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, 
        const char *call_name/*=NULL*/) const;
    void PostThreadedCallsIndexOverridenGroup(int *out_summary_fault/*=NULL*/, 
        ddependencycount_t member_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, unsigned index_override, 
        const char *call_name/*=NULL*/) const;
    void PostThreadedCallForUnawareReleasee(int *out_summary_fault/*=NULL*/, 
        dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dCallWaitID call_wait/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
        const char *call_name/*=NULL*/) const;

protected:
    const dxThreadingFunctionsInfo *FindThreadingImpl(dThreadingImplementationID &out_impl_found) const;

private:
    const dxThreadingFunctionsInfo *GetFunctionsInfo() const { return m_functions_info; }
    dThreadingImplementationID GetThreadingImpl() const { return m_threading_impl; }

private:
    dxIThreadingDefaultImplProvider   *m_default_impl_provider;
    const dxThreadingFunctionsInfo    *m_functions_info;
    dThreadingImplementationID        m_threading_impl;
};

class dxMutexGroupLockHelper
{
public:
    dxMutexGroupLockHelper(dxThreadingBase *threading_base, dMutexGroupID mutex_group, dmutexindex_t mutex_index):
        m_threading_base(threading_base),
        m_mutex_group(mutex_group),
        m_mutex_index(mutex_index),
        m_mutex_locked(true)
    {
        threading_base->LockMutexGroupMutex(mutex_group, mutex_index);
    }

    ~dxMutexGroupLockHelper()
    {
        if (m_mutex_locked)
        {
            m_threading_base->UnlockMutexGroupMutex(m_mutex_group, m_mutex_index);
        }
    }

    void UnlockMutex()
    {
        dIASSERT(m_mutex_locked);

        m_threading_base->UnlockMutexGroupMutex(m_mutex_group, m_mutex_index);
        m_mutex_locked = false;
    }

    void RelockMutex()
    {
        dIASSERT(!m_mutex_locked);

        m_threading_base->LockMutexGroupMutex(m_mutex_group, m_mutex_index);
        m_mutex_locked = true;
    }

private:
    dxThreadingBase                   *m_threading_base;
    dMutexGroupID                   m_mutex_group;
    dmutexindex_t                     m_mutex_index;
    bool                              m_mutex_locked;
};

#endif // #ifndef _ODE_THREADING_BASE_H_
