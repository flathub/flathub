/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading implementation templates file.                              *
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
 *  Job list and Mutex group implementation templates for built-in threading 
 *  support provider.
 */


#ifndef _ODE_THREADING_IMPL_TEMPLATES_H_
#define _ODE_THREADING_IMPL_TEMPLATES_H_


#include <ode/common.h>
#include <ode/memory.h>

#include <ode/threading.h>

#include "objects.h"

#include <new>


#define dMAKE_JOBINSTANCE_RELEASEE(job_instance) ((dCallReleaseeID)(job_instance))
#define dMAKE_RELEASEE_JOBINSTANCE(releasee) ((dxThreadedJobInfo *)(releasee))


template <class tThreadMutex>
class dxtemplateMutexGroup
{
private:
    dxtemplateMutexGroup() {}
    ~dxtemplateMutexGroup() {}

public:
    static dxtemplateMutexGroup<tThreadMutex> *AllocateInstance(dmutexindex_t Mutex_count);
    static void FreeInstance(dxtemplateMutexGroup<tThreadMutex> *mutex_group);

private:
    bool InitializeMutexArray(dmutexindex_t Mutex_count);
    void FinalizeMutexArray(dmutexindex_t Mutex_count);

public:
    void LockMutex(dmutexindex_t mutex_index) { dIASSERT(mutex_index < m_un.m_mutex_count); m_Mutex_array[mutex_index].LockMutex(); }
    bool TryLockMutex(dmutexindex_t mutex_index) { dIASSERT(mutex_index < m_un.m_mutex_count); return m_Mutex_array[mutex_index].TryLockMutex(); }
    void UnlockMutex(dmutexindex_t mutex_index) { dIASSERT(mutex_index < m_un.m_mutex_count); m_Mutex_array[mutex_index].UnlockMutex(); }

private:
    union
    {
        dmutexindex_t     m_mutex_count;
        unsigned long     m_reserved_for_allignment[2];

    } m_un;

    tThreadMutex      m_Mutex_array[1];
};

template<class tThreadWakeup>
class dxtemplateCallWait:
    public dBase
{
public:
    dxtemplateCallWait() {}
    ~dxtemplateCallWait() { DoFinalizeObject(); }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject() { return m_wait_wakeup.InitializeObject(); }
    void DoFinalizeObject() { /* Do nothing */ }

public:
    typedef dxtemplateCallWait<tThreadWakeup> dxCallWait;

public:
    void ResetTheWait() { m_wait_wakeup.ResetWakeup(); }
    void SignalTheWait() { m_wait_wakeup.WakeupAllThreads(); }
    bool PerformWaiting(const dThreadedWaitTime *timeout_time_ptr/*=NULL*/) { return m_wait_wakeup.WaitWakeup(timeout_time_ptr); }

public:
    static void AbstractSignalTheWait(void *wait_wakeup_ptr) { ((dxCallWait *)wait_wakeup_ptr)->SignalTheWait(); }

private:
    tThreadWakeup           m_wait_wakeup;
};


#if dBUILTIN_THREADING_IMPL_ENABLED

template<class tThreadWakeup, class tAtomicsProvider, const bool tatomic_test_required>
class dxtemplateThreadedLull
{
public:
    dxtemplateThreadedLull(): m_registrant_count(0), m_alarm_wakeup() {}
    ~dxtemplateThreadedLull() { dIASSERT(m_registrant_count == 0); DoFinalizeObject(); }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject() { return m_alarm_wakeup.InitializeObject(); }
    void DoFinalizeObject() { /* Do nothing */ }

private:
    typedef typename tAtomicsProvider::atomicord_t atomicord_t;

public:
    void RegisterToLull() { tAtomicsProvider::IncrementTargetNoRet(&m_registrant_count); }
    void WaitForLullAlarm() { dIASSERT(m_registrant_count != 0); m_alarm_wakeup.WaitWakeup(NULL); }
    void UnregisterFromLull() { tAtomicsProvider::DecrementTargetNoRet(&m_registrant_count); }

    void SignalLullAlarmIfAnyRegistrants()
    {
        if (tatomic_test_required ? (tAtomicsProvider::QueryTargetValue(&m_registrant_count) != 0) : (m_registrant_count != 0))
        {
            m_alarm_wakeup.WakeupAThread();
        }
    }

private:
    atomicord_t             m_registrant_count;
    tThreadWakeup           m_alarm_wakeup;
};


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


struct dxThreadedJobInfo:
    public dBase
{
    dxThreadedJobInfo() {}
    explicit dxThreadedJobInfo(void *): m_next_job(NULL) {}

    void AssignJobData(ddependencycount_t dependencies_count, dxThreadedJobInfo *dependent_job, void *call_wait, 
        int *fault_accumulator_ptr, dThreadedCallFunction *call_function, void *call_context, dcallindex_t call_index)
    {
        m_dependencies_count = dependencies_count;
        m_dependent_job = dependent_job;
        m_call_wait = call_wait;
        m_fault_accumulator_ptr = fault_accumulator_ptr;

        m_call_fault = 0;
        m_call_function = call_function;
        m_call_context = call_context;
        m_call_index = call_index;
    }

    bool InvokeCallFunction()
    {
        int call_result = m_call_function(m_call_context, m_call_index, dMAKE_JOBINSTANCE_RELEASEE(this));
        return call_result != 0;
    }

    dxThreadedJobInfo       *m_next_job;
    dxThreadedJobInfo       **m_prev_job_next_ptr;

    ddependencycount_t      m_dependencies_count;
    dxThreadedJobInfo       *m_dependent_job;
    void                    *m_call_wait;
    int                     *m_fault_accumulator_ptr;

    int                     m_call_fault;
    dThreadedCallFunction   *m_call_function;
    void                    *m_call_context;
    dcallindex_t            m_call_index;
};


template<class tThreadMutex>
class dxtemplateThreadingLockHelper
{
public:
    dxtemplateThreadingLockHelper(tThreadMutex &mutex_instance): m_mutex_instance(mutex_instance), m_lock_indicator_flag(false) { LockMutex(); }
    ~dxtemplateThreadingLockHelper() { if (m_lock_indicator_flag) { UnlockMutex(); } }

    void LockMutex() { dIASSERT(!m_lock_indicator_flag); m_mutex_instance.LockMutex(); m_lock_indicator_flag = true; }
    void UnlockMutex() { dIASSERT(m_lock_indicator_flag); m_mutex_instance.UnlockMutex(); m_lock_indicator_flag = false; }

private:
    tThreadMutex            &m_mutex_instance;
    bool                    m_lock_indicator_flag;
};

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
class dxtemplateJobListContainer
{
public:
    dxtemplateJobListContainer():
        m_job_list(NULL),
        m_info_pool((atomicptr_t)NULL),
        m_pool_access_lock(),
        m_list_access_lock(),
        m_info_wait_lull(),
        m_info_count_known_to_be_preallocated(0)
    {
    }

    ~dxtemplateJobListContainer()
    {
        dIASSERT(m_job_list == NULL); // Would not it be nice to wait for jobs to complete before deleting the list?

        FreeJobInfoPoolInfos();
        DoFinalizeObject();
    }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject() { return m_pool_access_lock.InitializeObject() && m_list_access_lock.InitializeObject() && m_info_wait_lull.InitializeObject(); }
    void DoFinalizeObject() { /* Do nothing */ }

public:
    typedef tAtomicsProvider dxAtomicsProvider;
    typedef typename tAtomicsProvider::atomicord_t atomicord_t;
    typedef typename tAtomicsProvider::atomicptr_t atomicptr_t;
    typedef tThreadMutex dxThreadMutex;
    typedef dxtemplateThreadingLockHelper<tThreadMutex> dxMutexLockHelper;
    typedef void dWaitSignallingFunction(void *job_call_wait);

public:
    dxThreadedJobInfo *ReleaseAJobAndPickNextPendingOne(
        dxThreadedJobInfo *job_to_release, bool job_result, dWaitSignallingFunction *wait_signal_proc_ptr, 
        bool &out_last_job_flag);

private:
    dxThreadedJobInfo *PickNextPendingJob(bool &out_last_job_flag);
    void ReleaseAJob(dxThreadedJobInfo *job_instance, bool job_result, dWaitSignallingFunction *wait_signal_proc_ptr);

public:
    inline dxThreadedJobInfo *AllocateJobInfoFromPool();
    void QueueJobForProcessing(dxThreadedJobInfo *job_instance);

    void AlterJobProcessingDependencies(dxThreadedJobInfo *job_instance, ddependencychange_t dependencies_count_change, 
        bool &out_job_has_become_ready);

private:
    inline ddependencycount_t SmartAddJobDependenciesCount(dxThreadedJobInfo *job_instance, ddependencychange_t dependencies_count_change);

    inline void InsertJobInfoIntoListHead(dxThreadedJobInfo *job_instance);
    inline void RemoveJobInfoFromList(dxThreadedJobInfo *job_instance);

    dxThreadedJobInfo *ExtractJobInfoFromPoolOrAllocate();
    inline void ReleaseJobInfoIntoPool(dxThreadedJobInfo *job_instance);

private:
    void FreeJobInfoPoolInfos();

public:
    bool EnsureNumberOfJobInfosIsPreallocated(ddependencycount_t required_info_count);

private:
    bool DoPreallocateJobInfos(ddependencycount_t required_info_count);

public:
    bool IsJobListReadyForShutdown() const { return m_job_list == NULL; }

private:
    dxThreadedJobInfo       *m_job_list;
    volatile atomicptr_t    m_info_pool; // dxThreadedJobInfo *
    tThreadMutex            m_pool_access_lock;
    tThreadMutex            m_list_access_lock;
    tThreadLull             m_info_wait_lull;
    ddependencycount_t      m_info_count_known_to_be_preallocated;
};


typedef void (dxThreadReadyToServeCallback)(void *callback_context);


#if dBUILTIN_THREADING_IMPL_ENABLED

template<class tThreadWakeup, class tJobListContainer>
class dxtemplateJobListThreadedHandler
{
public:
    dxtemplateJobListThreadedHandler(tJobListContainer *list_container_ptr):
        m_job_list_ptr(list_container_ptr),
        m_processing_wakeup(),
        m_active_thread_count(0),
        m_shutdown_requested(0)
    {
    }

    ~dxtemplateJobListThreadedHandler()
    {
        dIASSERT(m_active_thread_count == 0);

        DoFinalizeObject();
    }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject() { return m_processing_wakeup.InitializeObject(); }
    void DoFinalizeObject() { /* Do nothing */ }

public:
    typedef dxtemplateCallWait<tThreadWakeup> dxCallWait;

public:
    inline void ProcessActiveJobAddition();
    inline void PrepareForWaitingAJobCompletion();

public:
    inline unsigned RetrieveActiveThreadsCount();
    inline void StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/);

private:
    void PerformJobProcessingUntilShutdown();
    void PerformJobProcessingSession();

    void BlockAsIdleThread();
    void ActivateAnIdleThread();

public:
    inline void ShutdownProcessing();
    inline void CleanupForRestart();

private:
    bool IsShutdownRequested() const { return m_shutdown_requested != 0; }

private:
    typedef typename tJobListContainer::dxAtomicsProvider dxAtomicsProvider;
    typedef typename tJobListContainer::atomicord_t atomicord_t;

    atomicord_t GetActiveThreadsCount() const { return m_active_thread_count; }
    void RegisterAsActiveThread() { dxAtomicsProvider::template AddValueToTarget<sizeof(atomicord_t)>((volatile void *)&m_active_thread_count, 1); }
    void UnregisterAsActiveThread() { dxAtomicsProvider::template AddValueToTarget<sizeof(atomicord_t)>((volatile void *)&m_active_thread_count, -1); }

private:
    tJobListContainer       *m_job_list_ptr;
    tThreadWakeup           m_processing_wakeup;
    volatile atomicord_t    m_active_thread_count;
    int                     m_shutdown_requested;
};


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


template<class tThreadWakeup, class tJobListContainer>
class dxtemplateJobListSelfHandler
{
public:
    dxtemplateJobListSelfHandler(tJobListContainer *list_container_ptr):
        m_job_list_ptr(list_container_ptr)
    {
    }

    ~dxtemplateJobListSelfHandler()
    {
        // Do nothing
    }

    bool InitializeObject() { return true; }

public:
    typedef dxtemplateCallWait<tThreadWakeup> dxCallWait;

public:
    inline void ProcessActiveJobAddition();
    inline void PrepareForWaitingAJobCompletion();

public:
    inline unsigned RetrieveActiveThreadsCount();
    inline void StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/);

private:
    void PerformJobProcessingUntilExhaustion();
    void PerformJobProcessingSession();

public:
    inline void ShutdownProcessing();
    inline void CleanupForRestart();

private:
    tJobListContainer       *m_job_list_ptr;
};


struct dIMutexGroup;
struct dxICallWait;

class dxIThreadingImplementation
{
public:
    virtual void FreeInstance() = 0;

public:
    virtual dIMutexGroup *AllocMutexGroup(dmutexindex_t Mutex_count) = 0;
    virtual void FreeMutexGroup(dIMutexGroup *mutex_group) = 0;
    virtual void LockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index) = 0;
    // virtual bool TryLockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index) = 0;
    virtual void UnlockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index) = 0;

public:
    virtual dxICallWait *AllocACallWait() = 0;
    virtual void ResetACallWait(dxICallWait *call_wait) = 0;
    virtual void FreeACallWait(dxICallWait *call_wait) = 0;

public:
    virtual bool PreallocateJobInfos(ddependencycount_t max_simultaneous_calls_estimate) = 0;
    virtual void ScheduleNewJob(int *fault_accumulator_ptr/*=NULL*/, 
        dCallReleaseeID *out_post_releasee_ptr/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dxICallWait *call_wait/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index) = 0;
    virtual void AlterJobDependenciesCount(dCallReleaseeID target_releasee, ddependencychange_t dependencies_count_change) = 0;
    virtual void WaitJobCompletion(int *out_wait_status_ptr/*=NULL*/, 
        dxICallWait *call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/) = 0;

public:
    virtual unsigned RetrieveActiveThreadsCount() = 0;
    virtual void StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/) = 0;
    virtual void ShutdownProcessing() = 0;
    virtual void CleanupForRestart() = 0;
};


template<class tJobListContainer, class tJobListHandler>
class dxtemplateThreadingImplementation:
    public dBase,
    public dxIThreadingImplementation
{
public:
    dxtemplateThreadingImplementation():
        dBase(),
        m_list_container(),
        m_list_handler(&m_list_container)
    {
    }

    virtual ~dxtemplateThreadingImplementation()
    {
        DoFinalizeObject();
    }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject() { return m_list_container.InitializeObject() && m_list_handler.InitializeObject(); }
    void DoFinalizeObject() { /* Do nothing */ }

protected:
    virtual void FreeInstance();

private:
    typedef dxtemplateMutexGroup<typename tJobListContainer::dxThreadMutex> dxMutexGroup;
    typedef typename tJobListHandler::dxCallWait dxCallWait;

protected:
    virtual dIMutexGroup *AllocMutexGroup(dmutexindex_t Mutex_count);
    virtual void FreeMutexGroup(dIMutexGroup *mutex_group);
    virtual void LockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index);
    // virtual bool TryLockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index);
    virtual void UnlockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index);

protected:
    virtual dxICallWait *AllocACallWait();
    virtual void ResetACallWait(dxICallWait *call_wait);
    virtual void FreeACallWait(dxICallWait *call_wait);

protected:
    virtual bool PreallocateJobInfos(ddependencycount_t max_simultaneous_calls_estimate);
    virtual void ScheduleNewJob(int *fault_accumulator_ptr/*=NULL*/, 
        dCallReleaseeID *out_post_releasee_ptr/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
        dxICallWait *call_wait/*=NULL*/, 
        dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index);
    virtual void AlterJobDependenciesCount(dCallReleaseeID target_releasee, ddependencychange_t dependencies_count_change);
    virtual void WaitJobCompletion(int *out_wait_status_ptr/*=NULL*/, 
        dxICallWait *call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/);

protected:
    virtual unsigned RetrieveActiveThreadsCount();
    virtual void StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/);
    virtual void ShutdownProcessing();
    virtual void CleanupForRestart();

private:
    tJobListContainer     m_list_container;
    tJobListHandler       m_list_handler;
};


/************************************************************************/
/* Implementation of dxtemplateMutexGroup                               */
/************************************************************************/

template<class tThreadMutex>
/*static */dxtemplateMutexGroup<tThreadMutex> *dxtemplateMutexGroup<tThreadMutex>::AllocateInstance(dmutexindex_t Mutex_count)
{
    dAASSERT(Mutex_count != 0);

    const dxtemplateMutexGroup<tThreadMutex> *const dummy_group = (dxtemplateMutexGroup<tThreadMutex> *)(size_t)8;
    const size_t size_requited = ((size_t)(&dummy_group->m_Mutex_array) - (size_t)dummy_group) + Mutex_count * sizeof(tThreadMutex);
    dxtemplateMutexGroup<tThreadMutex> *mutex_group = (dxtemplateMutexGroup<tThreadMutex> *)dAlloc(size_requited);

    if (mutex_group != NULL)
    {
        mutex_group->m_un.m_mutex_count = Mutex_count;

        if (!mutex_group->InitializeMutexArray(Mutex_count))
        {
            dFree((void *)mutex_group, size_requited);
            mutex_group = NULL;
        }
    }

    return mutex_group;
}

template<class tThreadMutex>
/*static */void dxtemplateMutexGroup<tThreadMutex>::FreeInstance(dxtemplateMutexGroup<tThreadMutex> *mutex_group)
{
    if (mutex_group != NULL)
    {
        dmutexindex_t Mutex_count = mutex_group->m_un.m_mutex_count;
        mutex_group->FinalizeMutexArray(Mutex_count);

        const size_t anyting_not_zero = 2 * sizeof(size_t);
        const dxtemplateMutexGroup<tThreadMutex> *const dummy_group = (dxtemplateMutexGroup<tThreadMutex> *)anyting_not_zero;
        const size_t size_requited = ((size_t)(&dummy_group->m_Mutex_array) - (size_t)dummy_group) + Mutex_count * sizeof(tThreadMutex);
        dFree((void *)mutex_group, size_requited);
    }
}

template<class tThreadMutex>
bool dxtemplateMutexGroup<tThreadMutex>::InitializeMutexArray(dmutexindex_t Mutex_count)
{
    bool any_fault = false;

    dmutexindex_t mutex_index = 0;
    for (; mutex_index != Mutex_count; ++mutex_index)
    {
        tThreadMutex *mutex_storage = m_Mutex_array + mutex_index;

        new(mutex_storage) tThreadMutex;

        if (!mutex_storage->InitializeObject())
        {
            mutex_storage->tThreadMutex::~tThreadMutex();

            any_fault = true;
            break;
        }
    }

    if (any_fault)
    {
        FinalizeMutexArray(mutex_index);
    }

    bool init_result = !any_fault;
    return init_result;
}

template<class tThreadMutex>
void dxtemplateMutexGroup<tThreadMutex>::FinalizeMutexArray(dmutexindex_t Mutex_count)
{
    for (dmutexindex_t mutex_index = 0; mutex_index != Mutex_count; ++mutex_index)
    {
        tThreadMutex *mutex_storage = m_Mutex_array + mutex_index;

        mutex_storage->tThreadMutex::~tThreadMutex();
    }
}

/************************************************************************/
/* Implementation of dxtemplateJobListContainer                         */
/************************************************************************/

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
dxThreadedJobInfo *dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::ReleaseAJobAndPickNextPendingOne(
    dxThreadedJobInfo *job_to_release, bool job_result, dWaitSignallingFunction *wait_signal_proc_ptr, bool &out_last_job_flag)
{
    if (job_to_release != NULL)
    {
        ReleaseAJob(job_to_release, job_result, wait_signal_proc_ptr);
    }

    dxMutexLockHelper list_access(m_list_access_lock);

    dxThreadedJobInfo *picked_job = PickNextPendingJob(out_last_job_flag);
    return picked_job;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
dxThreadedJobInfo *dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::PickNextPendingJob(
    bool &out_last_job_flag)
{
    dxThreadedJobInfo *current_job = m_job_list;
    bool last_job_flag = false;

    while (current_job != NULL)
    {
        if (current_job->m_dependencies_count == 0)
        {
            // It is OK to assign in unsafe manner - dependencies count should not be changed
            // after the job has become ready for execution
            current_job->m_dependencies_count = 1;
            last_job_flag = current_job->m_next_job == NULL;

            RemoveJobInfoFromList(current_job);
            break;
        }

        current_job = current_job->m_next_job;
    }

    out_last_job_flag = last_job_flag;
    return current_job;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::ReleaseAJob(
    dxThreadedJobInfo *job_instance, bool job_result, dWaitSignallingFunction *wait_signal_proc_ptr)
{
    dxThreadedJobInfo *current_job = job_instance;

    if (!job_result)
    {
        // Accumulate call fault (be careful to not reset it!!!)
        current_job->m_call_fault = 1;
    }

    bool job_dequeued = true;
    dIASSERT(current_job->m_prev_job_next_ptr == NULL);

    while (true)
    {
        dIASSERT(current_job->m_dependencies_count != 0);

        ddependencycount_t new_dependencies_count = SmartAddJobDependenciesCount(current_job, -1);

        if (new_dependencies_count != 0 || !job_dequeued)
        {
            break;
        }

        void *job_call_wait = current_job->m_call_wait;

        if (job_call_wait != NULL)
        {
            wait_signal_proc_ptr(job_call_wait);
        }

        int call_fault = current_job->m_call_fault;

        if (current_job->m_fault_accumulator_ptr)
        {
            *current_job->m_fault_accumulator_ptr = call_fault;
        }

        dxThreadedJobInfo *dependent_job = current_job->m_dependent_job;
        ReleaseJobInfoIntoPool(current_job);

        if (dependent_job == NULL)
        {
            break;
        }

        if (call_fault)
        {
            // Accumulate call fault (be careful to not reset it!!!)
            dependent_job->m_call_fault = 1;
        }

        current_job = dependent_job;
        job_dequeued = dependent_job->m_prev_job_next_ptr == NULL;
    }
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
dxThreadedJobInfo *dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::AllocateJobInfoFromPool()
{
    // No locking is necessary
    dxThreadedJobInfo *job_instance = ExtractJobInfoFromPoolOrAllocate();
    return job_instance;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::QueueJobForProcessing(dxThreadedJobInfo *job_instance)
{
    dxMutexLockHelper list_access(m_list_access_lock);

    InsertJobInfoIntoListHead(job_instance);
}


template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::AlterJobProcessingDependencies(dxThreadedJobInfo *job_instance, ddependencychange_t dependencies_count_change, 
                                                                                                             bool &out_job_has_become_ready)
{
    // Dependencies should not be changed when job has already become ready for execution
    dIASSERT(job_instance->m_dependencies_count != 0);
    // It's OK that access is not atomic - that is to be handled by external logic
    dIASSERT(dependencies_count_change < 0 ? (job_instance->m_dependencies_count >= (ddependencycount_t)(-dependencies_count_change)) : ((ddependencycount_t)(-(ddependencychange_t)job_instance->m_dependencies_count) > (ddependencycount_t)dependencies_count_change));

    ddependencycount_t new_dependencies_count = SmartAddJobDependenciesCount(job_instance, dependencies_count_change);
    out_job_has_become_ready = new_dependencies_count == 0;
}


template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
ddependencycount_t dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::SmartAddJobDependenciesCount(
    dxThreadedJobInfo *job_instance, ddependencychange_t dependencies_count_change)
{
    ddependencycount_t new_dependencies_count = tAtomicsProvider::template AddValueToTarget<sizeof(ddependencycount_t)>((volatile void *)&job_instance->m_dependencies_count, dependencies_count_change) + dependencies_count_change;
    return new_dependencies_count;
}


template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::InsertJobInfoIntoListHead(
    dxThreadedJobInfo *job_instance)
{
    dxThreadedJobInfo *job_list_head = m_job_list;
    job_instance->m_next_job = job_list_head;

    if (job_list_head != NULL)
    {
        job_list_head->m_prev_job_next_ptr = &job_instance->m_next_job;
    }

    job_instance->m_prev_job_next_ptr = &m_job_list;
    m_job_list = job_instance;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::RemoveJobInfoFromList(
    dxThreadedJobInfo *job_instance)
{
    if (job_instance->m_next_job)
    { 
        job_instance->m_next_job->m_prev_job_next_ptr = job_instance->m_prev_job_next_ptr;
    }

    *job_instance->m_prev_job_next_ptr = job_instance->m_next_job;
    // Assign NULL to m_prev_job_next_ptr as an indicator that instance has been dequeued
    job_instance->m_prev_job_next_ptr = NULL;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
dxThreadedJobInfo *dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::ExtractJobInfoFromPoolOrAllocate()
{
    dxThreadedJobInfo *result_info;

    bool waited_lull = false;
    m_info_wait_lull.RegisterToLull();

    while (true)
    {
        dxThreadedJobInfo *raw_head_info = (dxThreadedJobInfo *)m_info_pool;

        if (raw_head_info == NULL)
        {
            result_info = new dxThreadedJobInfo();

            if (result_info != NULL)
            {
                break;
            }

            m_info_wait_lull.WaitForLullAlarm();
            waited_lull = true;
        }

        // Extraction must be locked so that other thread does not "steal" head info,
        // use it and then reinsert back with a different "next"
        dxMutexLockHelper pool_access(m_pool_access_lock);

        dxThreadedJobInfo *head_info = (dxThreadedJobInfo *)m_info_pool; // Head info must be re-read after mutex had been locked

        if (head_info != NULL)
        {
            dxThreadedJobInfo *next_info = head_info->m_next_job;
            if (tAtomicsProvider::CompareExchangeTargetPtr(&m_info_pool, (atomicptr_t)head_info, (atomicptr_t)next_info))
            {
                result_info = head_info;
                break;
            }
        }
    }

    m_info_wait_lull.UnregisterFromLull();

    if (waited_lull)
    {
        // It is necessary to re-signal lull alarm if current thread was waiting as
        // there might be other threads waiting which might have not received alarm signal.
        m_info_wait_lull.SignalLullAlarmIfAnyRegistrants();
    }

    return result_info;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::ReleaseJobInfoIntoPool(
    dxThreadedJobInfo *job_instance)
{
    while (true)
    {
        dxThreadedJobInfo *next_info = (dxThreadedJobInfo *)m_info_pool;
        job_instance->m_next_job = next_info;

        if (tAtomicsProvider::CompareExchangeTargetPtr(&m_info_pool, (atomicptr_t)next_info, (atomicptr_t)job_instance))
        {
            break;
        }
    }

    m_info_wait_lull.SignalLullAlarmIfAnyRegistrants();
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
void dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::FreeJobInfoPoolInfos()
{
    dxThreadedJobInfo *current_info = (dxThreadedJobInfo *)m_info_pool;

    while (current_info != NULL)
    {
        dxThreadedJobInfo *info_save = current_info;
        current_info = current_info->m_next_job;

        delete info_save;
    }

    m_info_pool = (atomicptr_t)NULL;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
bool dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::EnsureNumberOfJobInfosIsPreallocated(ddependencycount_t required_info_count)
{
    bool result = required_info_count <= m_info_count_known_to_be_preallocated 
        || DoPreallocateJobInfos(required_info_count);
    return result;
}

template<class tThreadLull, class tThreadMutex, class tAtomicsProvider>
bool dxtemplateJobListContainer<tThreadLull, tThreadMutex, tAtomicsProvider>::DoPreallocateJobInfos(ddependencycount_t required_info_count)
{
    dIASSERT(required_info_count > m_info_count_known_to_be_preallocated); // Also ensures required_info_count > 0

    bool allocation_failure = false;

    dxThreadedJobInfo *info_pool = (dxThreadedJobInfo *)m_info_pool;

    ddependencycount_t info_index = 0;
    for (dxThreadedJobInfo **current_info_ptr = &info_pool; ; )
    {
        dxThreadedJobInfo *current_info = *current_info_ptr;

        if (current_info == NULL)
        {
            current_info = new dxThreadedJobInfo(NULL);

            if (current_info == NULL)
            {
                allocation_failure = true;
                break;
            }

            *current_info_ptr = current_info;
        }

        if (++info_index == required_info_count)
        {
            m_info_count_known_to_be_preallocated = info_index;
            break;
        }

        current_info_ptr = &current_info->m_next_job;
    }

    // Make sure m_info_pool was not changed
    dIASSERT(m_info_pool == NULL || m_info_pool == (atomicptr_t)info_pool);

    m_info_pool = (atomicptr_t)info_pool;

    bool result = !allocation_failure;
    return result;
}


#if dBUILTIN_THREADING_IMPL_ENABLED

/************************************************************************/
/* Implementation of dxtemplateJobListThreadedHandler                   */
/************************************************************************/

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::ProcessActiveJobAddition()
{
    ActivateAnIdleThread();
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::PrepareForWaitingAJobCompletion()
{
    // Do nothing
}

template<class tThreadWakeup, class tJobListContainer>
unsigned dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::RetrieveActiveThreadsCount()
{
    return GetActiveThreadsCount();
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/)
{
    RegisterAsActiveThread();

    if (readiness_callback != NULL)
    {
        (*readiness_callback)(callback_context);
    }

    PerformJobProcessingUntilShutdown();

    UnregisterAsActiveThread();
}


template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::PerformJobProcessingUntilShutdown()
{
    while (true)
    {
        // It is expected that new jobs will not be queued any longer after shutdown had been requested
        if (IsShutdownRequested() && m_job_list_ptr->IsJobListReadyForShutdown())
        {
            break;
        }

        PerformJobProcessingSession();

        // It is expected that new jobs will not be queued any longer after shutdown had been requested
        if (IsShutdownRequested() && m_job_list_ptr->IsJobListReadyForShutdown())
        {
            break;
        }

        BlockAsIdleThread();
    }
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::PerformJobProcessingSession()
{
    dxThreadedJobInfo *current_job = NULL;
    bool job_result = false;

    while (true)
    {
        bool last_job_flag;
        current_job = m_job_list_ptr->ReleaseAJobAndPickNextPendingOne(current_job, job_result, &dxCallWait::AbstractSignalTheWait, last_job_flag);

        if (!current_job)
        {
            break;
        }

        if (!last_job_flag)
        {
            ActivateAnIdleThread();
        }

        job_result = current_job->InvokeCallFunction();
    }
}


template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::BlockAsIdleThread()
{
    m_processing_wakeup.WaitWakeup(NULL);
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::ActivateAnIdleThread()
{
    m_processing_wakeup.WakeupAThread();
}


template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::ShutdownProcessing()
{
    m_shutdown_requested = true;
    m_processing_wakeup.WakeupAllThreads();
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListThreadedHandler<tThreadWakeup, tJobListContainer>::CleanupForRestart()
{
    m_shutdown_requested = false;
    m_processing_wakeup.ResetWakeup();
}


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


/************************************************************************/
/* Implementation of dxtemplateJobListSelfHandler                       */
/************************************************************************/

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::ProcessActiveJobAddition()
{
    // Do nothing
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::PrepareForWaitingAJobCompletion()
{
    PerformJobProcessingUntilExhaustion();
}


template<class tThreadWakeup, class tJobListContainer>
unsigned dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::RetrieveActiveThreadsCount()
{
    return 1U; // Self-Handling is always performed by a single thread
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/)
{
    (void)readiness_callback; // unused
    (void)callback_context; // unused
    dIASSERT(false); // This method is not expected to be called for Self-Handler
}


template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::PerformJobProcessingUntilExhaustion()
{
    PerformJobProcessingSession();
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::PerformJobProcessingSession()
{
    dxThreadedJobInfo *current_job = NULL;
    bool job_result = false;

    while (true)
    {
        bool dummy_last_job_flag;
        current_job = m_job_list_ptr->ReleaseAJobAndPickNextPendingOne(current_job, job_result, &dxCallWait::AbstractSignalTheWait, dummy_last_job_flag);

        if (!current_job)
        {
            break;
        }

        job_result = current_job->InvokeCallFunction();
    }
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::ShutdownProcessing()
{
    // Do nothing
}

template<class tThreadWakeup, class tJobListContainer>
void dxtemplateJobListSelfHandler<tThreadWakeup, tJobListContainer>::CleanupForRestart()
{
    // Do nothing
}


/************************************************************************/
/* Implementation of dxtemplateThreadingImplementation                          */
/************************************************************************/

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::FreeInstance()
{
    delete this;
}


template<class tJobListContainer, class tJobListHandler>
dIMutexGroup *dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::AllocMutexGroup(dmutexindex_t Mutex_count)
{
    dxMutexGroup *mutex_group = dxMutexGroup::AllocateInstance(Mutex_count);
    return (dIMutexGroup *)mutex_group;
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::FreeMutexGroup(dIMutexGroup *mutex_group)
{
    dxMutexGroup::FreeInstance((dxMutexGroup *)mutex_group);
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::LockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index)
{
    ((dxMutexGroup *)mutex_group)->LockMutex(mutex_index);
}

// template<class tJobListContainer, class tJobListHandler>
// bool dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::TryLockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index)
// {
//   return ((dxMutexGroup *)mutex_group)->TryLockMutex(mutex_index);
// }

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::UnlockMutexGroupMutex(dIMutexGroup *mutex_group, dmutexindex_t mutex_index)
{
    ((dxMutexGroup *)mutex_group)->UnlockMutex(mutex_index);
}


template<class tJobListContainer, class tJobListHandler>
dxICallWait *dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::AllocACallWait()
{
    dxCallWait *call_wait = new dxCallWait();

    if (call_wait != NULL && !call_wait->InitializeObject())
    {
        delete call_wait;
        call_wait = NULL;
    }

    return (dxICallWait *)call_wait;
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::ResetACallWait(dxICallWait *call_wait)
{
    ((dxCallWait *)call_wait)->ResetTheWait();
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::FreeACallWait(dxICallWait *call_wait)
{
    delete ((dxCallWait *)call_wait);
}


template<class tJobListContainer, class tJobListHandler>
bool dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::PreallocateJobInfos(ddependencycount_t max_simultaneous_calls_estimate)
{
    // No multithreading protection here!
    // Resources are to be preallocated before jobs start to be scheduled
    // as otherwise there is no way to implement the preallocation.
    bool result = m_list_container.EnsureNumberOfJobInfosIsPreallocated(max_simultaneous_calls_estimate);
    return result;
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::ScheduleNewJob(
    int *fault_accumulator_ptr/*=NULL*/, 
    dCallReleaseeID *out_post_releasee_ptr/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dxICallWait *call_wait/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index)
{
    dxThreadedJobInfo *new_job = m_list_container.AllocateJobInfoFromPool();
    dIASSERT(new_job != NULL);

    new_job->AssignJobData(dependencies_count, dMAKE_RELEASEE_JOBINSTANCE(dependent_releasee), (dxCallWait *)call_wait, fault_accumulator_ptr, call_func, call_context, instance_index);

    if (out_post_releasee_ptr != NULL)
    {
        *out_post_releasee_ptr = dMAKE_JOBINSTANCE_RELEASEE(new_job);
    }

    m_list_container.QueueJobForProcessing(new_job);

    if (dependencies_count == 0)
    {
        m_list_handler.ProcessActiveJobAddition();
    }
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::AlterJobDependenciesCount(
    dCallReleaseeID target_releasee, ddependencychange_t dependencies_count_change)
{
    dIASSERT(dependencies_count_change != 0);

    dxThreadedJobInfo *job_instance = dMAKE_RELEASEE_JOBINSTANCE(target_releasee);

    bool job_has_become_ready;
    m_list_container.AlterJobProcessingDependencies(job_instance, dependencies_count_change, job_has_become_ready);

    if (job_has_become_ready)
    {
        m_list_handler.ProcessActiveJobAddition();
    }
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::WaitJobCompletion(
    int *out_wait_status_ptr/*=NULL*/, 
    dxICallWait *call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/)
{
    dIASSERT(call_wait != NULL);

    m_list_handler.PrepareForWaitingAJobCompletion();

    bool wait_status = ((dxCallWait *)call_wait)->PerformWaiting(timeout_time_ptr);
    dIASSERT(timeout_time_ptr != NULL || wait_status);

    if (out_wait_status_ptr)
    {
        *out_wait_status_ptr = wait_status;
    }
}


template<class tJobListContainer, class tJobListHandler>
unsigned dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::RetrieveActiveThreadsCount()
{
    return m_list_handler.RetrieveActiveThreadsCount();
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::StickToJobsProcessing(dxThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/)
{
    m_list_handler.StickToJobsProcessing(readiness_callback, callback_context);
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::ShutdownProcessing()
{
    m_list_handler.ShutdownProcessing();
}

template<class tJobListContainer, class tJobListHandler>
void dxtemplateThreadingImplementation<tJobListContainer, tJobListHandler>::CleanupForRestart()
{
    m_list_handler.CleanupForRestart();
}


#endif // #ifndef _ODE_THREADING_IMPL_TEMPLATES_H_
