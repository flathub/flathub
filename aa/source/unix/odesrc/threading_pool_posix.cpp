/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading POSIX thread pool implementation file.                      *
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
 *  POSIX thread pool implementation for built-in threading support provider.
 */


#if !defined(_WIN32)

#include <ode/odeconfig.h>
#include <ode/error.h>
#include <ode/threading_impl.h>
#include <ode/odeinit.h>
#include "config.h"
#include "objects.h"
#include "threading_impl_templates.h"


#if dBUILTIN_THREADING_IMPL_ENABLED

#include <new>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#if !defined(EOK)
#define EOK   0
#endif


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#if dBUILTIN_THREADING_IMPL_ENABLED

struct dxEventObject
{
public:
    dxEventObject(): m_event_allocated(false), m_event_manual(false), m_event_value(false) {}
    ~dxEventObject() { FinalizeObject(); }

    bool InitializeObject(bool manual_reset, bool initial_state);
    void FinalizeObject();

    // WARNING! To make implementation simpler, event only releases a single thread even for manual mode.
    bool WaitInfinitely();
    void SetEvent();
    void ResetEvent();

private:
    bool              m_event_allocated;
    bool              m_event_manual;
    bool              m_event_value;
    pthread_mutex_t   m_event_mutex;
    pthread_cond_t    m_event_cond;
};

bool dxEventObject::InitializeObject(bool manual_reset, bool initial_state)
{
    dIASSERT(!m_event_allocated);

    bool result = false;

    bool cond_allocated = false;

    do 
    {
        int cond_result = pthread_cond_init(&m_event_cond, NULL);
        if (cond_result != EOK)
        {
            errno = cond_result;
            break;
        }

        cond_allocated = true;

        int mutex_result = pthread_mutex_init(&m_event_mutex, NULL);
        if (mutex_result != EOK)
        {
            errno = mutex_result;
            break;
        }

        m_event_manual = manual_reset;
        m_event_value = initial_state;
        m_event_allocated = true;
        result = true;
    }
    while (false);

    if (!result)
    {
        if (cond_allocated)
        {
            int cond_destroy_result = pthread_cond_destroy(&m_event_cond);
            dIVERIFY(cond_destroy_result == EOK);
        }
    }

    return result;
}

void dxEventObject::FinalizeObject()
{
    if (m_event_allocated)
    {
        int mutex_destroy_result = pthread_mutex_destroy(&m_event_mutex);
        dICHECK(mutex_destroy_result == EOK); // Why would mutex be unable to be destroyed?

        int cond_destroy_result = pthread_cond_destroy(&m_event_cond);
        dICHECK(cond_destroy_result == EOK); // Why would condvar be unable to be destroyed?

        m_event_allocated = false;
    }
}

bool dxEventObject::WaitInfinitely()
{
    bool result = false;

    int lock_result = pthread_mutex_lock(&m_event_mutex);
    dICHECK(lock_result == EOK);

    int wait_result = EOK;
    if (!m_event_value)
    {
        wait_result = pthread_cond_wait(&m_event_cond, &m_event_mutex);
        dICHECK(wait_result != EINTR); // Would caller be so kind to disable signal handling for thread for duration of the call to ODE at least?
    }

    if (wait_result == EOK)
    {
        dIASSERT(m_event_value);

        if (!m_event_manual)
        {
            m_event_value = false;
        }

        result = true;
    }

    int unlock_result = pthread_mutex_unlock(&m_event_mutex);
    dICHECK(unlock_result == EOK);

    return result;
}

void dxEventObject::SetEvent()
{
    int lock_result = pthread_mutex_lock(&m_event_mutex);
    dICHECK(lock_result == EOK);

    if (!m_event_value)
    {
        m_event_value = true;

        // NOTE! Event only releases a single thread even for manual mode to simplify implementation.
        int signal_result = pthread_cond_signal(&m_event_cond);
        dICHECK(signal_result == EOK);
    }

    int unlock_result = pthread_mutex_unlock(&m_event_mutex);
    dICHECK(unlock_result == EOK);
}

void dxEventObject::ResetEvent()
{
    int lock_result = pthread_mutex_lock(&m_event_mutex);
    dICHECK(lock_result == EOK);

    m_event_value = false;

    int unlock_result = pthread_mutex_unlock(&m_event_mutex);
    dICHECK(unlock_result == EOK);
}


struct dxThreadPoolThreadInfo
{
public:
    dxThreadPoolThreadInfo();
    ~dxThreadPoolThreadInfo();

    bool Initialize(size_t stack_size, unsigned int ode_data_allocate_flags);

private:
    bool InitializeThreadAttributes(pthread_attr_t *thread_attr, size_t stack_size);
    void FinalizeThreadAttributes(pthread_attr_t *thread_attr);
    bool WaitInitStatus();

private:
    void Finalize();
    void WaitAndCloseThreadHandle(pthread_t thread_handle);

public:
    enum dxTHREADCOMMAND
    {
        dxTHREAD_COMMAND_EXIT,
        dxTHREAD_COMMAND_NOOP,
        dxTHREAD_COMMAND_SERVE_IMPLEMENTATION,
    };

    struct dxServeImplementationParams
    {
        dxServeImplementationParams(dThreadingImplementationID impl, dxEventObject *ready_wait_event):
    m_impl(impl), m_ready_wait_event(ready_wait_event)
    {
    }

    dThreadingImplementationID m_impl;
    dxEventObject *m_ready_wait_event;
    };

    void ExecuteThreadCommand(dxTHREADCOMMAND command, void *param, bool wait_response);

private:
    static void *ThreadProcedure_Callback(void *thread_param);
    void ThreadProcedure();
    bool DisableSignalHandlers();
    void ReportInitStatus(bool init_result);
    void RunCommandHandlingLoop();

    void ThreadedServeImplementation(dThreadingImplementationID impl, dxEventObject *ready_wait_event);
    static void ProcessThreadServeReadiness_Callback(void *context);

private:
    pthread_t   m_thread_handle;
    bool        m_thread_allocated;

    unsigned int m_ode_data_allocate_flags;
    dxTHREADCOMMAND m_command_code;
    dxEventObject m_command_event;
    dxEventObject m_acknowledgement_event;
    void        *m_command_param;
};


dxThreadPoolThreadInfo::dxThreadPoolThreadInfo():
m_thread_handle(),
m_thread_allocated(false),
m_ode_data_allocate_flags(0),
m_command_code(dxTHREAD_COMMAND_EXIT),
m_command_event(),
m_acknowledgement_event(),
m_command_param(NULL)
{
}

dxThreadPoolThreadInfo::~dxThreadPoolThreadInfo()
{
    Finalize();
}


bool dxThreadPoolThreadInfo::Initialize(size_t stack_size, unsigned int ode_data_allocate_flags)
{
    bool result = false;

    bool command_event_allocated = false, acknowledgement_event_allocated = false;

    do 
    {
        // -- There is no implicit limit on stack size in POSIX implementation
        // if (stack_size > ...)
        // {
        //   errno = EINVAL;
        //   break;
        // }

        if (!m_command_event.InitializeObject(false, false))
        {
            break;
        }

        command_event_allocated = true;

        if (!m_acknowledgement_event.InitializeObject(true, false))
        {
            break;
        }

        acknowledgement_event_allocated = true;

        m_ode_data_allocate_flags = ode_data_allocate_flags;

        pthread_attr_t thread_attr;
        if (!InitializeThreadAttributes(&thread_attr, stack_size))
        {
            break;
        }

        int thread_create_result = pthread_create(&m_thread_handle, &thread_attr, &ThreadProcedure_Callback, (void *)this);

        FinalizeThreadAttributes(&thread_attr);

        if (thread_create_result != EOK)
        {
            errno = thread_create_result;
            break;
        }

        bool thread_init_result = WaitInitStatus();
        if (!thread_init_result)
        {
            WaitAndCloseThreadHandle(m_thread_handle);
            break;
        }

        m_thread_allocated = true;
        result = true;
    }
    while (false);

    if (!result)
    {
        if (command_event_allocated)
        {
            if (acknowledgement_event_allocated)
            {
                m_acknowledgement_event.FinalizeObject();
            }

            m_command_event.FinalizeObject();
        }
    }

    return result;
}

bool dxThreadPoolThreadInfo::InitializeThreadAttributes(pthread_attr_t *thread_attr, size_t stack_size)
{
    bool result = false;

    bool attr_inited = false;

    do 
    {
        int init_result = pthread_attr_init(thread_attr);
        if (init_result != EOK)
        {
            errno = init_result;
            break;
        }

        attr_inited = true;

        int set_result;
        if ((set_result = pthread_attr_setdetachstate(thread_attr, PTHREAD_CREATE_JOINABLE)) != EOK
            || (set_result = pthread_attr_setinheritsched(thread_attr, PTHREAD_INHERIT_SCHED)) != EOK
#if (HAVE_PTHREAD_ATTR_SETSTACKLAZY)
            || (set_result = pthread_attr_setstacklazy(thread_attr, PTHREAD_STACK_NOTLAZY)) != EOK
#endif
            || (stack_size != 0 && (set_result = pthread_attr_setstacksize(thread_attr, stack_size)) != EOK))
        {
            errno = set_result;
            break;
        }

        result = true;
    }
    while (false);

    if (!result)
    {
        if (attr_inited)
        {
            int destroy_result = pthread_attr_destroy(thread_attr);
            dIVERIFY(destroy_result == EOK);
        }
    }

    return result;
}

void dxThreadPoolThreadInfo::FinalizeThreadAttributes(pthread_attr_t *thread_attr)
{
    int destroy_result = pthread_attr_destroy(thread_attr);
    dIVERIFY(destroy_result == EOK);
}

bool dxThreadPoolThreadInfo::WaitInitStatus()
{
    bool acknowledgement_wait_result = m_acknowledgement_event.WaitInfinitely();
    dICHECK(acknowledgement_wait_result);

    int error_code = (int)(size_t)m_command_param;

    bool init_status = error_code == EOK ? true : ((errno = error_code), false);
    return init_status;
}

void dxThreadPoolThreadInfo::Finalize()
{
    if (m_thread_allocated)
    {
        ExecuteThreadCommand(dxTHREAD_COMMAND_EXIT, NULL, false);

        WaitAndCloseThreadHandle(m_thread_handle);
        m_thread_allocated = false;

        m_command_event.FinalizeObject();
        m_acknowledgement_event.FinalizeObject();
    }
}

void dxThreadPoolThreadInfo::WaitAndCloseThreadHandle(pthread_t thread_handle)
{
    int join_result = pthread_join(thread_handle, NULL);
    dICHECK(join_result == EOK);
}

void dxThreadPoolThreadInfo::ExecuteThreadCommand(dxTHREADCOMMAND command, void *param, bool wait_response)
{
    bool acknowledgement_wait_result = m_acknowledgement_event.WaitInfinitely();
    dICHECK(acknowledgement_wait_result);

    m_acknowledgement_event.ResetEvent();

    m_command_code = command;
    m_command_param = param;

    m_command_event.SetEvent();

    if (wait_response)
    {
        bool new_acknowledgement_wait_result = m_acknowledgement_event.WaitInfinitely();
        dICHECK(new_acknowledgement_wait_result);
    }
}

void *dxThreadPoolThreadInfo::ThreadProcedure_Callback(void *thread_param)
{
    dxThreadPoolThreadInfo *thread_info = (dxThreadPoolThreadInfo *)thread_param;
    thread_info->ThreadProcedure();

    return 0;
}

void dxThreadPoolThreadInfo::ThreadProcedure()
{
    bool init_result = dAllocateODEDataForThread(m_ode_data_allocate_flags) != 0
        && DisableSignalHandlers();

    ReportInitStatus(init_result);

    if (init_result)
    {
        RunCommandHandlingLoop();

        // dCleanupODEAllDataForThread(); -- this function can only be called if ODE was initialized for manual cleanup. And that is unknown here...
    }
}

bool dxThreadPoolThreadInfo::DisableSignalHandlers()
{
    bool result = false;

    sigset_t set;
    sigfillset( &set );

    if (sigprocmask( SIG_BLOCK, &set, NULL ) != -1)
    {
        result = true;
    }

    return result;
}

void dxThreadPoolThreadInfo::ReportInitStatus(bool init_result)
{
    m_command_param = (void *)(size_t)(init_result ? EOK : ((errno != EOK) ? errno : EFAULT));

    m_acknowledgement_event.SetEvent();
}

void dxThreadPoolThreadInfo::RunCommandHandlingLoop()
{
    bool exit_requested = false;

    while (!exit_requested)
    {
        bool command_wait_result = m_command_event.WaitInfinitely();
        dICHECK(command_wait_result);

        const dxTHREADCOMMAND command_code = m_command_code;
        switch (command_code)
        {
            case dxTHREAD_COMMAND_EXIT:
            {
                m_acknowledgement_event.SetEvent();

                exit_requested = true;
                break;
            }

            default:
            {
                dIASSERT(false);
                // break; -- proceed to case dxTHREAD_COMMAND_NOOP
            }

            case dxTHREAD_COMMAND_NOOP:
            {
                m_acknowledgement_event.SetEvent();

                // Do nothing
                break;
            }

            case dxTHREAD_COMMAND_SERVE_IMPLEMENTATION:
            {
                const dxServeImplementationParams *serve_params = (const dxServeImplementationParams *)m_command_param;
                dThreadingImplementationID impl = serve_params->m_impl;
                dxEventObject *ready_wait_event = serve_params->m_ready_wait_event;

                m_acknowledgement_event.SetEvent();

                ThreadedServeImplementation(impl, ready_wait_event);
                break;
            }
        }
    }
}

void dxThreadPoolThreadInfo::ThreadedServeImplementation(dThreadingImplementationID impl, dxEventObject *ready_wait_event)
{
    ((dxIThreadingImplementation *)impl)->StickToJobsProcessing(&ProcessThreadServeReadiness_Callback, (void *)ready_wait_event);
}

void dxThreadPoolThreadInfo::ProcessThreadServeReadiness_Callback(void *context)
{
    dxEventObject *ready_wait_event = (dxEventObject *)context;

    ready_wait_event->SetEvent();
}



struct dxThreadingThreadPool:
    public dBase
{
public:
    dxThreadingThreadPool();
    ~dxThreadingThreadPool();

    bool InitializeThreads(size_t thread_count, size_t stack_size, unsigned int ode_data_allocate_flags);

private:
    void FinalizeThreads();

    bool InitializeIndividualThreadInfos(dxThreadPoolThreadInfo *thread_infos, size_t thread_count, size_t stack_size, unsigned int ode_data_allocate_flags);
    void FinalizeIndividualThreadInfos(dxThreadPoolThreadInfo *thread_infos, size_t thread_count);

    bool InitializeSingleThreadInfo(dxThreadPoolThreadInfo *thread_info, size_t stack_size, unsigned int ode_data_allocate_flags);
    void FinalizeSingleThreadInfo(dxThreadPoolThreadInfo *thread_info);

public:
    void ServeThreadingImplementation(dThreadingImplementationID impl);
    void WaitIdleState();

private:
    dxThreadPoolThreadInfo  *m_thread_infos;
    size_t                  m_thread_count;
    dxEventObject           m_ready_wait_event;
};


dxThreadingThreadPool::dxThreadingThreadPool():
m_thread_infos(NULL),
m_thread_count(0),
m_ready_wait_event()
{
}

dxThreadingThreadPool::~dxThreadingThreadPool()
{
    FinalizeThreads();
}


bool dxThreadingThreadPool::InitializeThreads(size_t thread_count, size_t stack_size, unsigned int ode_data_allocate_flags)
{
    dIASSERT(m_thread_infos == NULL);

    bool result = false;

    bool wait_event_allocated = false;

    dxThreadPoolThreadInfo *thread_infos = NULL;
    bool thread_infos_allocated = false;

    do
    {
        if (!m_ready_wait_event.InitializeObject(false, false))
        {
            break;
        }

        wait_event_allocated = true;

        thread_infos = (dxThreadPoolThreadInfo *)dAlloc(thread_count * sizeof(dxThreadPoolThreadInfo));
        if (thread_infos == NULL)
        {
            break;
        }

        thread_infos_allocated = true;

        if (!InitializeIndividualThreadInfos(thread_infos, thread_count, stack_size, ode_data_allocate_flags))
        {
            break;
        }

        m_thread_infos = thread_infos;
        m_thread_count = thread_count;
        result = true;
    }
    while (false);

    if (!result)
    {
        if (wait_event_allocated)
        {
            if (thread_infos_allocated)
            {
                dFree(thread_infos, thread_count * sizeof(dxThreadPoolThreadInfo));
            }

            m_ready_wait_event.FinalizeObject();
        }
    }

    return result;
}

void dxThreadingThreadPool::FinalizeThreads()
{
    dxThreadPoolThreadInfo *thread_infos = m_thread_infos;
    if (thread_infos != NULL)
    {
        size_t thread_count = m_thread_count;

        FinalizeIndividualThreadInfos(thread_infos, thread_count);
        dFree(thread_infos, thread_count * sizeof(dxThreadPoolThreadInfo));

        m_ready_wait_event.FinalizeObject();
    }
}


bool dxThreadingThreadPool::InitializeIndividualThreadInfos(dxThreadPoolThreadInfo *thread_infos, size_t thread_count, size_t stack_size, unsigned int ode_data_allocate_flags)
{
    bool any_fault = false;

    dxThreadPoolThreadInfo *const infos_end = thread_infos + thread_count;
    for (dxThreadPoolThreadInfo *current_info = thread_infos; current_info != infos_end; ++current_info)
    {
        if (!InitializeSingleThreadInfo(current_info, stack_size, ode_data_allocate_flags))
        {
            FinalizeIndividualThreadInfos(thread_infos, current_info - thread_infos);

            any_fault = true;
            break;
        }
    }

    bool result = !any_fault;
    return result;
}

void dxThreadingThreadPool::FinalizeIndividualThreadInfos(dxThreadPoolThreadInfo *thread_infos, size_t thread_count)
{
    dxThreadPoolThreadInfo *const infos_end = thread_infos + thread_count;
    for (dxThreadPoolThreadInfo *current_info = thread_infos; current_info != infos_end; ++current_info)
    {
        FinalizeSingleThreadInfo(current_info);
    }
}


bool dxThreadingThreadPool::InitializeSingleThreadInfo(dxThreadPoolThreadInfo *thread_info, size_t stack_size, unsigned int ode_data_allocate_flags)
{
    bool result = false;

    new(thread_info) dxThreadPoolThreadInfo();

    if (thread_info->Initialize(stack_size, ode_data_allocate_flags))
    {
        result = true;
    }
    else
    {
        thread_info->dxThreadPoolThreadInfo::~dxThreadPoolThreadInfo();
    }

    return result;
}

void dxThreadingThreadPool::FinalizeSingleThreadInfo(dxThreadPoolThreadInfo *thread_info)
{
    if (thread_info != NULL)
    {
        thread_info->dxThreadPoolThreadInfo::~dxThreadPoolThreadInfo();
    }
}


void dxThreadingThreadPool::ServeThreadingImplementation(dThreadingImplementationID impl)
{
    dxThreadPoolThreadInfo::dxServeImplementationParams params(impl, &m_ready_wait_event);

    dxThreadPoolThreadInfo *const infos_end = m_thread_infos + m_thread_count;
    for (dxThreadPoolThreadInfo *current_info = m_thread_infos; current_info != infos_end; ++current_info)
    {
        current_info->ExecuteThreadCommand(dxThreadPoolThreadInfo::dxTHREAD_COMMAND_SERVE_IMPLEMENTATION, &params, true);

        bool ready_wait_result = m_ready_wait_event.WaitInfinitely();
        dICHECK(ready_wait_result);
    }
}

void dxThreadingThreadPool::WaitIdleState()
{
    dxThreadPoolThreadInfo *const infos_end = m_thread_infos + m_thread_count;
    for (dxThreadPoolThreadInfo *current_info = m_thread_infos; current_info != infos_end; ++current_info)
    {
        current_info->ExecuteThreadCommand(dxThreadPoolThreadInfo::dxTHREAD_COMMAND_NOOP, NULL, true);
    }
}


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


/*extern */dThreadingThreadPoolID dThreadingAllocateThreadPool(unsigned thread_count, 
                                                               size_t stack_size, unsigned int ode_data_allocate_flags, void *reserved/*=NULL*/)
{
    dAASSERT(thread_count != 0);

#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = new dxThreadingThreadPool();
    if (thread_pool != NULL)
    {
        if (thread_pool->InitializeThreads(thread_count, stack_size, ode_data_allocate_flags))
        {
            // do nothing
        }
        else
        {
            delete thread_pool;
            thread_pool = NULL;
        }
    }
#else
    dThreadingThreadPoolID thread_pool = NULL;
    (void)stack_size; // unused
    (void)ode_data_allocate_flags; // unused
    (void)reserved; // unused
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

    return (dThreadingThreadPoolID)thread_pool;
}

/*extern */void dThreadingThreadPoolServeMultiThreadedImplementation(dThreadingThreadPoolID pool, dThreadingImplementationID impl)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    thread_pool->ServeThreadingImplementation(impl);
#else
    (void)pool; // unused
    (void)impl; // unused
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}

/*extern */void dThreadingThreadPoolWaitIdleState(dThreadingThreadPoolID pool)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    thread_pool->WaitIdleState();
#else
    (void)pool; // unused
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}

/*extern */void dThreadingFreeThreadPool(dThreadingThreadPoolID pool)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    delete thread_pool;
#else
    (void)pool; // unused
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}


#endif // #if !defined(_WIN32)
