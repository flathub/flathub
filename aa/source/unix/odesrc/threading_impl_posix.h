/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading POSIX implementation file.                                  *
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
 *  Threading POSIX implementation for built-in threading support provider.
 */


#ifndef _ODE_THREADING_IMPL_POSIX_H_
#define _ODE_THREADING_IMPL_POSIX_H_


#include <ode/common.h>


#if !defined(_WIN32)


#include "threading_impl_templates.h"
#include "threading_fake_sync.h"
#include "threading_atomics_provs.h"


#if dBUILTIN_THREADING_IMPL_ENABLED

#include <pthread.h>
#include <time.h>
#include <errno.h>

#if !defined(EOK)
#define EOK   0
#endif


#if HAVE_PTHREAD_CONDATTR_SETCLOCK

static inline 
int _condvar_clock_gettime(int clock_type, timespec *ts)
{
    return clock_gettime(clock_type, ts);
}


#else // #if !HAVE_PTHREAD_CONDATTR_SETCLOCK

#if defined(__APPLE__)

#if HAVE_GETTIMEOFDAY

#include <sys/time.h>

#if !defined(CLOCK_MONOTONIC)
#define CLOCK_MONOTONIC 2
#endif

static inline 
int _condvar_clock_gettime(int clock_type, timespec *ts)
{
    (void)clock_type; // Unused
    timeval tv;
    return gettimeofday(&tv, NULL) == 0 ? (ts->tv_sec = tv.tv_sec, ts->tv_nsec = tv.tv_usec * 1000, 0) : (-1);
}


#else // #if !HAVE_GETTIMEOFDAY

#error It is necessary to check manuals for the correct way of getting condvar wait time for this Apple system


#endif // #if !HAVE_GETTIMEOFDAY


#else // #if !defined(__APPLE__)


#error It is necessary to check manuals for the correct way of getting condvar wait time for this system


#endif // #if !defined(__APPLE__)


#endif // #if !HAVE_PTHREAD_CONDATTR_SETCLOCK


/************************************************************************/
/* dxCondvarWakeup class implementation                                 */
/************************************************************************/

class dxCondvarWakeup
{
public:
    dxCondvarWakeup(): m_waiters_list(NULL), m_signaled_state(false), m_state_is_permanent(false), m_object_initialized(false) {}
    ~dxCondvarWakeup() { DoFinalizeObject(); }

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
    bool BlockAsAWaiter(const dThreadedWaitTime *timeout_time_ptr);

private:
    struct dxWaiterInfo
    {
        dxWaiterInfo(): m_signal_state(false) {}

        dxWaiterInfo      **m_prev_info_ptr;
        dxWaiterInfo      *m_next_info;
        bool              m_signal_state;
    };

    void RegisterWaiterInList(dxWaiterInfo *waiter_info);
    void UnregisterWaiterFromList(dxWaiterInfo *waiter_info);

    bool MarkSignaledFirstWaiter();
    static bool MarkSignaledFirstWaiterMeaningful(dxWaiterInfo *first_waiter);
    bool MarkSignaledAllWaiters();
    static bool MarkSignaledAllWaitersMeaningful(dxWaiterInfo *first_waiter);

private:
    dxWaiterInfo  *m_waiters_list;
    bool          m_signaled_state;
    bool          m_state_is_permanent;
    bool          m_object_initialized;
    pthread_mutex_t m_wakeup_mutex;
    pthread_cond_t m_wakeup_cond;
};


bool dxCondvarWakeup::DoInitializeObject()
{
    dIASSERT(!m_object_initialized);

    bool init_result = false;

    pthread_condattr_t cond_condattr;
    bool mutex_initialized = false, condattr_initialized = false;

    do
    {
        int mutex_result = pthread_mutex_init(&m_wakeup_mutex, NULL);
        if (mutex_result != EOK)
        {
            errno = mutex_result;
            break;
        }

        mutex_initialized = true;

        int condattr_init_result = pthread_condattr_init(&cond_condattr);
        if (condattr_init_result != EOK)
        {
            errno = condattr_init_result;
            break;
        }

        condattr_initialized = true;

#if HAVE_PTHREAD_CONDATTR_SETCLOCK
        int condattr_clock_result = pthread_condattr_setclock(&cond_condattr, CLOCK_MONOTONIC);
        if (condattr_clock_result != EOK)
        {
            errno = condattr_clock_result;
            break;
        }
#endif // #if HAVE_PTHREAD_CONDATTR_SETCLOCK

        int cond_result = pthread_cond_init(&m_wakeup_cond, &cond_condattr);
        if (cond_result != EOK)
        {
            errno = cond_result;
            break;
        }

        pthread_condattr_destroy(&cond_condattr); // result can be ignored

        m_object_initialized = true;
        init_result = true;
    }
    while (false);

    if (!init_result)
    {
        if (mutex_initialized)
        {
            if (condattr_initialized)
            {
                int condattr_destroy_result = pthread_condattr_destroy(&cond_condattr);
                dICHECK(condattr_destroy_result == EOK || ((errno = condattr_destroy_result), false));
            }

            int mutex_destroy_result = pthread_mutex_destroy(&m_wakeup_mutex);
            dICHECK(mutex_destroy_result == EOK || ((errno = mutex_destroy_result), false));
        }
    }

    return init_result;

}

void dxCondvarWakeup::DoFinalizeObject()
{
    if (m_object_initialized)
    {
        int cond_result = pthread_cond_destroy(&m_wakeup_cond);
        dICHECK(cond_result == EOK || ((errno = cond_result), false));

        int mutex_result = pthread_mutex_destroy(&m_wakeup_mutex);
        dICHECK(mutex_result == EOK || ((errno = mutex_result), false));

        m_object_initialized = false;
    }
}


void dxCondvarWakeup::ResetWakeup()
{
    int lock_result = pthread_mutex_lock(&m_wakeup_mutex);
    dICHECK(lock_result == EOK || ((errno = lock_result), false));

    m_signaled_state = false;
    m_state_is_permanent = false;

    int unlock_result = pthread_mutex_unlock(&m_wakeup_mutex);
    dICHECK(unlock_result == EOK || ((errno = unlock_result), false));
}

void dxCondvarWakeup::WakeupAThread()
{
    int lock_result = pthread_mutex_lock(&m_wakeup_mutex);
    dICHECK(lock_result == EOK || ((errno = lock_result), false));

    dIASSERT(!m_state_is_permanent); // Wakeup should not be used after permanent signal

    if (!m_signaled_state)
    {
        if (MarkSignaledFirstWaiter())
        {
            // All threads must be woken up regardless to the fact that only one waiter is marked.
            // It is not possible to wake up a chosen thread personally 
            // and if a random thread is woken up it can't know if there was a condition signal for it
            // or the sleep was interrupted by POSIX signal.
            // On the other hand, without this it is not possible to guarantee that a thread
            // will be woken up per each WakeupAThread() call if there is more than one waiter
            // and wakeup requests will not accumulate if there are no waiters.
            int broadcast_result = pthread_cond_broadcast(&m_wakeup_cond);
            dICHECK(broadcast_result == EOK || ((errno = broadcast_result), false));
        }
        else
        {
            m_signaled_state = true;
        }
    }

    int unlock_result = pthread_mutex_unlock(&m_wakeup_mutex);
    dICHECK(unlock_result == EOK || ((errno = unlock_result), false));
}

void dxCondvarWakeup::WakeupAllThreads()
{
    int lock_result = pthread_mutex_lock(&m_wakeup_mutex);
    dICHECK(lock_result == EOK || ((errno = lock_result), false));

    m_state_is_permanent = true;

    if (!m_signaled_state)
    {
        m_signaled_state = true;

        if (MarkSignaledAllWaiters())
        {
            int broadcast_result = pthread_cond_broadcast(&m_wakeup_cond);
            dICHECK(broadcast_result == EOK || ((errno = broadcast_result), false));
        }
    }

    int unlock_result = pthread_mutex_unlock(&m_wakeup_mutex);
    dICHECK(unlock_result == EOK || ((errno = unlock_result), false));
}


bool dxCondvarWakeup::WaitWakeup(const dThreadedWaitTime *timeout_time_ptr)
{
    bool wait_result;

    int lock_result = pthread_mutex_lock(&m_wakeup_mutex);
    dICHECK(lock_result == EOK || ((errno = lock_result), false));

    if (!m_signaled_state)
    {
        if (!timeout_time_ptr || timeout_time_ptr->wait_nsec != 0 || timeout_time_ptr->wait_sec != 0)
        {
            wait_result = BlockAsAWaiter(timeout_time_ptr);
        }
        else
        {
            wait_result = false;
        }
    }
    else
    {
        m_signaled_state = m_state_is_permanent;
        wait_result = true;
    }

    int unlock_result = pthread_mutex_unlock(&m_wakeup_mutex);
    dICHECK(unlock_result == EOK || ((errno = unlock_result), false));

    return wait_result;
}

bool dxCondvarWakeup::BlockAsAWaiter(const dThreadedWaitTime *timeout_time_ptr)
{
    bool wait_result = false;

    dxWaiterInfo waiter_info;
    RegisterWaiterInList(&waiter_info);

    timespec wakeup_time;

    if (timeout_time_ptr != NULL)
    {
        timespec current_time;

        int clock_result = _condvar_clock_gettime(CLOCK_MONOTONIC, &current_time);
        dICHECK(clock_result != -1);

        time_t wakeup_sec = current_time.tv_sec + timeout_time_ptr->wait_sec;
        unsigned long wakeup_nsec = current_time.tv_nsec + timeout_time_ptr->wait_nsec;

        if (wakeup_nsec >= 1000000000)
        {
            wakeup_nsec -= 1000000000;
            wakeup_sec += 1;
        }

        wakeup_time.tv_sec = wakeup_sec;
        wakeup_time.tv_nsec = wakeup_nsec;
    }

    while (true)
    {
        int cond_result = (timeout_time_ptr != NULL) 
            ? pthread_cond_timedwait(&m_wakeup_cond, &m_wakeup_mutex, &wakeup_time) 
            : pthread_cond_wait(&m_wakeup_cond, &m_wakeup_mutex);
        dICHECK(cond_result == EOK || cond_result == ETIMEDOUT || ((errno = cond_result), false));

        if (waiter_info.m_signal_state)
        {
            wait_result = true;
            break;
        }

        if (cond_result == ETIMEDOUT)
        {
            dIASSERT(timeout_time_ptr != NULL);
            break;
        }
    }

    UnregisterWaiterFromList(&waiter_info);

    return wait_result;
}


void dxCondvarWakeup::RegisterWaiterInList(dxWaiterInfo *waiter_info)
{
    dxWaiterInfo *const first_waiter = m_waiters_list;

    if (first_waiter == NULL)
    {
        waiter_info->m_next_info = waiter_info;
        waiter_info->m_prev_info_ptr = &waiter_info->m_next_info;
        m_waiters_list = waiter_info;
    }
    else
    {
        waiter_info->m_next_info = first_waiter;
        waiter_info->m_prev_info_ptr = first_waiter->m_prev_info_ptr;
        *first_waiter->m_prev_info_ptr = waiter_info;
        first_waiter->m_prev_info_ptr = &waiter_info->m_next_info;
    }
}

void dxCondvarWakeup::UnregisterWaiterFromList(dxWaiterInfo *waiter_info)
{
    dxWaiterInfo *next_info = waiter_info->m_next_info;

    if (next_info == waiter_info)
    {
        m_waiters_list = NULL;
    }
    else
    {
        next_info->m_prev_info_ptr = waiter_info->m_prev_info_ptr;
        *waiter_info->m_prev_info_ptr = next_info;

        if (waiter_info == m_waiters_list)
        {
            m_waiters_list = next_info;
        }
    }
}


bool dxCondvarWakeup::MarkSignaledFirstWaiter()
{
    bool waiter_found = false;

    dxWaiterInfo *const first_waiter = m_waiters_list;

    if (first_waiter)
    {
        waiter_found = MarkSignaledFirstWaiterMeaningful(first_waiter);
    }

    return waiter_found;
}

bool dxCondvarWakeup::MarkSignaledFirstWaiterMeaningful(dxWaiterInfo *first_waiter)
{
    bool waiter_found = false;

    dxWaiterInfo *current_waiter = first_waiter;

    while (true)
    {
        if (!current_waiter->m_signal_state)
        {
            current_waiter->m_signal_state = true;
            waiter_found = true;
            break;
        }

        current_waiter = current_waiter->m_next_info;
        if (current_waiter == first_waiter)
        {
            break;
        }
    }

    return waiter_found;
}

bool dxCondvarWakeup::MarkSignaledAllWaiters()
{
    bool waiter_found = false;

    dxWaiterInfo *const first_waiter = m_waiters_list;

    if (first_waiter)
    {
        waiter_found = MarkSignaledAllWaitersMeaningful(first_waiter);
    }

    return waiter_found;
}

bool dxCondvarWakeup::MarkSignaledAllWaitersMeaningful(dxWaiterInfo *first_waiter)
{
    bool waiter_found = false;

    dxWaiterInfo *current_waiter = first_waiter;

    while (true)
    {
        if (!current_waiter->m_signal_state)
        {
            current_waiter->m_signal_state = true;
            waiter_found = true;
        }

        current_waiter = current_waiter->m_next_info;
        if (current_waiter == first_waiter)
        {
            break;
        }
    }

    return waiter_found;
}


/************************************************************************/
/* dxMutexMutex class implementation                          */
/************************************************************************/

class dxMutexMutex
{
public:
    dxMutexMutex(): m_mutex_allocated(false) {}
    ~dxMutexMutex() { DoFinalizeObject(); }

    bool InitializeObject() { return DoInitializeObject(); }

private:
    bool DoInitializeObject();
    void DoFinalizeObject();

public:
    void LockMutex();
    bool TryLockMutex();
    void UnlockMutex();

private:
    pthread_mutex_t     m_mutex_instance;
    bool                m_mutex_allocated;
};


bool dxMutexMutex::DoInitializeObject()
{
    dIASSERT(!m_mutex_allocated);

    bool init_result = false;

    do
    {
        int mutex_result = pthread_mutex_init(&m_mutex_instance, NULL);
        if (mutex_result != EOK)
        {
            errno = mutex_result;
            break;
        }

        m_mutex_allocated = true;
        init_result = true;
    }
    while (false);

    return init_result;
}

void dxMutexMutex::DoFinalizeObject()
{
    if (m_mutex_allocated)
    {
        int mutex_result = pthread_mutex_destroy(&m_mutex_instance);
        dICHECK(mutex_result == EOK || ((errno = mutex_result), false));

        m_mutex_allocated = false;
    }
}


void dxMutexMutex::LockMutex()
{
    int lock_result = pthread_mutex_lock(&m_mutex_instance);
    dICHECK(lock_result == EOK || ((errno = lock_result), false));
}

bool dxMutexMutex::TryLockMutex()
{
    int trylock_result = pthread_mutex_trylock(&m_mutex_instance);
    dICHECK(trylock_result == EOK || trylock_result == EBUSY || ((errno = trylock_result), false));

    return trylock_result == EOK;
}

void dxMutexMutex::UnlockMutex()
{
    int unlock_result = pthread_mutex_unlock(&m_mutex_instance);
    dICHECK(unlock_result == EOK || ((errno = unlock_result), false));
}


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

typedef dxtemplateJobListContainer<dxtemplateThreadedLull<dxCondvarWakeup, dxOUAtomicsProvider, false>, dxMutexMutex, dxOUAtomicsProvider> dxMultiThreadedJobListContainer;
typedef dxtemplateJobListThreadedHandler<dxCondvarWakeup, dxMultiThreadedJobListContainer> dxMultiThreadedJobListHandler;
typedef dxtemplateThreadingImplementation<dxMultiThreadedJobListContainer, dxMultiThreadedJobListHandler> dxMultiThreadedThreading;


#endif // #if dBUILTIN_THREADING_IMPL_ENABLED


#endif // #if !defined(_WIN32)


#endif // #ifndef _ODE_THREADING_IMPL_POSIX_H_
