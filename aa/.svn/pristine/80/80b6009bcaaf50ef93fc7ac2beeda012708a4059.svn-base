/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading Windows implementation file.                                *
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
 *  Threading Windows implementation for built-in threading support provider.
 */


#ifndef _ODE_THREADING_IMPL_WIN_H_
#define _ODE_THREADING_IMPL_WIN_H_


#include <ode/common.h>


#if defined(_WIN32)

#if dBUILTIN_THREADING_IMPL_ENABLED

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#include "threading_impl_templates.h"
#include "threading_fake_sync.h"
#include "threading_atomics_provs.h"


#if dBUILTIN_THREADING_IMPL_ENABLED

/************************************************************************/
/* dxEventWakeup class implementation                                   */
/************************************************************************/

class dxEventWakeup
{
public:
    dxEventWakeup(): m_state_is_permanent(false), m_event_handle(NULL) {}
    ~dxEventWakeup() { DoFinalizeObject(); }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject();
    void DoFinalizeObject();

public:
    void ResetWakeup();
    void WakeupAThread();
    void WakeupAllThreads();

    bool WaitWakeup(const dThreadedWaitTime *timeout_time_ptr);

private:
    bool          m_state_is_permanent;
    HANDLE        m_event_handle;  
};


bool dxEventWakeup::DoInitializeObject()
{
    dIASSERT(m_event_handle == NULL);

    bool init_result = false;

    do
    {
        HANDLE event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (event_handle == NULL)
        {
            break;
        }

        m_event_handle = event_handle;
        init_result = true;
    }
    while (false);

    return init_result;
}

void dxEventWakeup::DoFinalizeObject()
{
    HANDLE event_handle = m_event_handle;

    if (event_handle != NULL)
    {
        BOOL close_result = CloseHandle(event_handle);
        dICHECK(close_result != FALSE);

        m_event_handle = NULL;
    }
}


void dxEventWakeup::ResetWakeup()
{
    // Order of assignment and resetting event is not important but it is preferable to be performed this way.
    m_state_is_permanent = false;

    BOOL event_set_result = ResetEvent(m_event_handle);
    dICHECK(event_set_result);
}

void dxEventWakeup::WakeupAThread()
{
    dIASSERT(!m_state_is_permanent); // Wakeup should not be used after permanent signal

    BOOL event_reset_result = SetEvent(m_event_handle);
    dICHECK(event_reset_result);
}

void dxEventWakeup::WakeupAllThreads()
{
    // Order of assignment and setting event is important!
    m_state_is_permanent = true;

    BOOL event_set_result = SetEvent(m_event_handle);
    dICHECK(event_set_result);
}


bool dxEventWakeup::WaitWakeup(const dThreadedWaitTime *timeout_time_ptr)
{
    bool wait_result;

    if (timeout_time_ptr == NULL)
    {
        DWORD event_wait_result = WaitForSingleObject(m_event_handle, INFINITE);
        dICHECK(event_wait_result == WAIT_OBJECT_0);

        wait_result = true;
    }
    else if (timeout_time_ptr->wait_sec == 0 && timeout_time_ptr->wait_nsec == 0)
    {
        DWORD event_wait_result = WaitForSingleObject(m_event_handle, 0);

        wait_result = event_wait_result == WAIT_OBJECT_0;
        dICHECK(wait_result || event_wait_result == WAIT_TIMEOUT);
    }
    else
    {
        dIASSERT(timeout_time_ptr->wait_nsec < 1000000000UL);

        const DWORD max_wait_seconds_in_a_shot = ((INFINITE - 1) / 1000U) - 1;

        time_t timeout_seconds_remaining = timeout_time_ptr->wait_sec;
        DWORD wait_timeout = timeout_time_ptr->wait_nsec != 0 ? ((timeout_time_ptr->wait_nsec + 999999UL) / 1000000UL) : 0;

        while (true)
        {
            if (timeout_seconds_remaining >= (time_t)max_wait_seconds_in_a_shot)
            {
                wait_timeout += max_wait_seconds_in_a_shot * 1000U;
                timeout_seconds_remaining -= max_wait_seconds_in_a_shot;
            }
            else
            {
                wait_timeout += (DWORD)timeout_seconds_remaining * 1000U;
                timeout_seconds_remaining = 0;
            }

            DWORD event_wait_result = WaitForSingleObject(m_event_handle, wait_timeout);

            if (event_wait_result == WAIT_OBJECT_0)
            {
                wait_result = true;
                break;
            }

            dICHECK(event_wait_result == WAIT_TIMEOUT);

            if (timeout_seconds_remaining == 0)
            {
                wait_result = false;
                break;
            }

            wait_timeout = 0;
        }
    }

    if (wait_result && m_state_is_permanent)
    {
        // Since event is automatic it is necessary to set it back for the upcoming waiters
        BOOL event_set_result = SetEvent(m_event_handle);
        dICHECK(event_set_result);
    }

    return wait_result;
}


/************************************************************************/
/* dxCriticalSectionMutex class implementation                          */
/************************************************************************/

class dxCriticalSectionMutex
{
public:
    dxCriticalSectionMutex() { InitializeCriticalSection(&m_critical_section); }
    ~dxCriticalSectionMutex() { DeleteCriticalSection(&m_critical_section); }

    bool InitializeObject() { return true; }

public:
    void LockMutex() { EnterCriticalSection(&m_critical_section); }
    bool TryLockMutex() { return TryEnterCriticalSection(&m_critical_section) != FALSE; }
    void UnlockMutex() { LeaveCriticalSection(&m_critical_section); }

private:
    CRITICAL_SECTION      m_critical_section;
};


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


/************************************************************************/
/* Self-threaded job list definition                                    */
/************************************************************************/

typedef dxtemplateJobListContainer<dxFakeLull, dxFakeMutex, dxFakeAtomicsProvider> dxSelfThreadedJobListContainer;
typedef dxtemplateJobListSelfHandler<dxSelfWakeup, dxSelfThreadedJobListContainer> dxSelfThreadedJobListHandler;
typedef dxtemplateThreadingImplementation<dxSelfThreadedJobListContainer, dxSelfThreadedJobListHandler> dxSelfThreadedThreading;


#if dBUILTIN_THREADING_IMPL_ENABLED

/************************************************************************/
/* Multi-threaded job list definition                                   */
/************************************************************************/

typedef dxtemplateJobListContainer<dxtemplateThreadedLull<dxEventWakeup, dxOUAtomicsProvider, false>, dxCriticalSectionMutex, dxOUAtomicsProvider> dxMultiThreadedJobListContainer;
typedef dxtemplateJobListThreadedHandler<dxEventWakeup, dxMultiThreadedJobListContainer> dxMultiThreadedJobListHandler;
typedef dxtemplateThreadingImplementation<dxMultiThreadedJobListContainer, dxMultiThreadedJobListHandler> dxMultiThreadedThreading;


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#endif // #if defined(_WIN32)


#endif // #ifndef _ODE_THREADING_IMPL_WIN_H_
