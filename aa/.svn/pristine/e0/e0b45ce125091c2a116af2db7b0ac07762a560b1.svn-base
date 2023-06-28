/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading Windows thread pool implementation file.                    *
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
 *  Windows thread pool implementation for built-in threading support provider.
 */


#if defined(_WIN32)

#include <ode/odeconfig.h>
#include <ode/error.h>
#include <ode/threading_impl.h>
#include <ode/odeinit.h>
#include "config.h"
#include "objects.h"
#include "threading_impl_templates.h"


#if dBUILTIN_THREADING_IMPL_ENABLED

#include <Windows.h>
#include <process.h>
#include <new>


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#if dBUILTIN_THREADING_IMPL_ENABLED

#define THREAD_STACK_MAX  ((size_t)(UINT_MAX - 1)) // The absolute maximum would be UINT_MAX but let it be a little bit less to avoid "Comparison is always false" warnings. ;)


struct dxEventObject
{
public:
    dxEventObject(): m_event_handle(NULL) {}
    ~dxEventObject() { FinalizeObject(); }

    bool InitializeObject(bool manual_reset, bool initial_state);
    void FinalizeObject();

    bool WaitInfinitely() { return ::WaitForSingleObject(m_event_handle, INFINITE) == WAIT_OBJECT_0; }
    void SetEvent();
    void ResetEvent();

private:
    HANDLE        m_event_handle;
};

bool dxEventObject::InitializeObject(bool manual_reset, bool initial_state)
{
    dIASSERT(m_event_handle == NULL);

    bool result = false;

    do 
    {
        HANDLE event_handle = ::CreateEvent(NULL, manual_reset, initial_state, NULL);
        if (event_handle == NULL)
        {
            break;
        }

        m_event_handle = event_handle;
        result = true;
    }
    while (false);

    return result;
}

void dxEventObject::FinalizeObject()
{
    HANDLE event_handle = m_event_handle;
    if (event_handle != NULL)
    {
        BOOL close_result = ::CloseHandle(event_handle);
        dICHECK(close_result); // Object destruction should always succeed

        m_event_handle = NULL;
    }
}

void dxEventObject::SetEvent()
{
    BOOL set_result = ::SetEvent(m_event_handle);
    dICHECK(set_result);
}

void dxEventObject::ResetEvent()
{
    BOOL reset_result = ::ResetEvent(m_event_handle);
    dICHECK(reset_result);
}



struct dxThreadPoolThreadInfo
{
public:
    dxThreadPoolThreadInfo();
    ~dxThreadPoolThreadInfo();

    bool Initialize(size_t stack_size, unsigned int ode_data_allocate_flags);

private:
    bool WaitInitStatus();

private:
    void Finalize();
    void WaitAndCloseThreadHandle(HANDLE thread_handle);

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
    static unsigned CALLBACK ThreadProcedure_Callback(void *thread_param);
    void ThreadProcedure();
    void ReportInitStatus(bool init_result);
    void RunCommandHandlingLoop();

    void ThreadedServeImplementation(dThreadingImplementationID impl, dxEventObject *ready_wait_event);
    static void ProcessThreadServeReadiness_Callback(void *context);

private:
    HANDLE      m_thread_handle;

    unsigned int m_ode_data_allocate_flags;
    dxTHREADCOMMAND m_command_code;
    dxEventObject m_command_event;
    dxEventObject m_acknowledgement_event;
    void        *m_command_param;
};


dxThreadPoolThreadInfo::dxThreadPoolThreadInfo():
m_thread_handle(NULL),
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

    HANDLE thread_handle = NULL;

    do 
    {
        if (stack_size > THREAD_STACK_MAX)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

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

        thread_handle = (HANDLE)_beginthreadex(NULL, (unsigned)stack_size, &ThreadProcedure_Callback, (void *)this, 0, NULL);
        if (thread_handle == NULL) // Not a bug!!! _beginthreadex() returns NULL on failure
        {
            break;
        }

        // It is OK to alter priority for thread without creating it in suspended state as
        // it is anyway going to be waited for (waited for its init result) and 
        // will not be issues commands until after that.
        int own_priority = GetThreadPriority(GetCurrentThread());
        if (own_priority != THREAD_PRIORITY_ERROR_RETURN)
        {
            if (!SetThreadPriority(thread_handle, own_priority))
            {
                // own_priority = THREAD_PRIORITY_ERROR_RETURN; -- Well, if priority inheritance fails - just ignore it :-/
            }
        }

        bool thread_init_result = WaitInitStatus();
        if (!thread_init_result)
        {
            DWORD error_save = GetLastError();
            WaitAndCloseThreadHandle(thread_handle);
            SetLastError(error_save);
            break;
        }

        m_thread_handle = thread_handle;
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

bool dxThreadPoolThreadInfo::WaitInitStatus()
{
    bool acknowledgement_wait_result = m_acknowledgement_event.WaitInfinitely();
    dICHECK(acknowledgement_wait_result);

    DWORD error_code = (DWORD)(size_t)m_command_param;

    bool init_status = error_code == ERROR_SUCCESS ? true : (SetLastError(error_code), false);
    return init_status;
}

void dxThreadPoolThreadInfo::Finalize()
{
    HANDLE thread_handle = m_thread_handle;
    if (thread_handle != NULL)
    {
        ExecuteThreadCommand(dxTHREAD_COMMAND_EXIT, NULL, false);

        WaitAndCloseThreadHandle(thread_handle);
        m_thread_handle = NULL;

        m_command_event.FinalizeObject();
        m_acknowledgement_event.FinalizeObject();
    }
}

void dxThreadPoolThreadInfo::WaitAndCloseThreadHandle(HANDLE thread_handle)
{
    DWORD thread_wait_result = WaitForSingleObject(thread_handle, INFINITE);
    dICHECK(thread_wait_result == WAIT_OBJECT_0);

    BOOL thread_close_result = CloseHandle(thread_handle);
    dIVERIFY(thread_close_result);

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

unsigned CALLBACK dxThreadPoolThreadInfo::ThreadProcedure_Callback(void *thread_param)
{
    dxThreadPoolThreadInfo *thread_info = (dxThreadPoolThreadInfo *)thread_param;
    thread_info->ThreadProcedure();

    return 0;
}

void dxThreadPoolThreadInfo::ThreadProcedure()
{
    bool init_result = dAllocateODEDataForThread(m_ode_data_allocate_flags) != 0;

    ReportInitStatus(init_result);

    if (init_result)
    {
        RunCommandHandlingLoop();

        // dCleanupODEAllDataForThread(); -- this function can only be called if ODE was initialized for manual cleanup. And that is unknown here...
    }
}

void dxThreadPoolThreadInfo::ReportInitStatus(bool init_result)
{
    DWORD error_code;
    m_command_param = (void *)(size_t)(init_result ? ERROR_SUCCESS : ((error_code = GetLastError()) != ERROR_SUCCESS ? error_code : ERROR_INTERNAL_ERROR));

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
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED

    return (dThreadingThreadPoolID)thread_pool;
}

/*extern */void dThreadingThreadPoolServeMultiThreadedImplementation(dThreadingThreadPoolID pool, dThreadingImplementationID impl)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    thread_pool->ServeThreadingImplementation(impl);
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}

/*extern */void dThreadingThreadPoolWaitIdleState(dThreadingThreadPoolID pool)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    thread_pool->WaitIdleState();
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}

/*extern */void dThreadingFreeThreadPool(dThreadingThreadPoolID pool)
{
#if dBUILTIN_THREADING_IMPL_ENABLED
    dxThreadingThreadPool *thread_pool = (dxThreadingThreadPool *)pool;
    delete thread_pool;
#endif // #if dBUILTIN_THREADING_IMPL_ENABLED
}


#endif // #if defined(_WIN32)
