/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading fake synchronization objects file.                          *
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
 *  Self-wakeup implementation for built-in threading support provider.
 *  Fake mutex implementation for built-in threading support provider.
 *
 *  The classes have been moved into a separate header as they are to be used 
 *  in both WIN and POSIX implementations.
 */


#ifndef _ODE_THREADING_FAKE_SYNC_H_
#define _ODE_THREADING_FAKE_SYNC_H_


#include <ode/odeconfig.h>
#include <ode/error.h>


/************************************************************************/
/* dxSelfWakeup class definition                                        */
/************************************************************************/

class dxSelfWakeup
{
public:
    dxSelfWakeup():
        m_wakeup_state(false),
        m_state_is_permanent(false)
    {
    }

    bool InitializeObject() { return true; }

public:
    void ResetWakeup() { m_wakeup_state = false; m_state_is_permanent = false; }
    void WakeupAThread() { dIASSERT(!m_state_is_permanent); m_wakeup_state = true; } // Wakeup should not be used after permanent signal
    void WakeupAllThreads() { m_wakeup_state = true; m_state_is_permanent = true; }

    bool WaitWakeup(const dThreadedWaitTime *timeout_time_ptr);

private:
    bool          m_wakeup_state;
    bool          m_state_is_permanent;
};


bool dxSelfWakeup::WaitWakeup(const dThreadedWaitTime *timeout_time_ptr)
{
    (void)timeout_time_ptr; // unused
    bool wait_result = m_wakeup_state;

    if (m_wakeup_state)
    {
        m_wakeup_state = m_state_is_permanent;
    }
    else
    {
        dICHECK(false); // Self-wakeup should only be used in cases when waiting is called after object is signaled
    }

    return wait_result;
}


/************************************************************************/
/* Fake mutex class implementation                                      */
/************************************************************************/

class dxFakeMutex
{
public:
    dxFakeMutex() {}

    bool InitializeObject() { return true; }

public:
    void LockMutex() { /* Do nothing */ }
    bool TryLockMutex() { /* Do nothing */ return true; }
    void UnlockMutex() { /* Do nothing */ }
};


/************************************************************************/
/* Fake lull class implementation                                      */
/************************************************************************/

class dxFakeLull
{
public:
    dxFakeLull() {}

    bool InitializeObject() { return true; }

public:
    void RegisterToLull() { /* Do nothing */ }
    void WaitForLullAlarm() { dICHECK(false); } // Fake lull can't be waited
    void UnregisterFromLull() { /* Do nothing */ }

    void SignalLullAlarmIfAnyRegistrants() { /* Do nothing */ }
};


#endif // #ifndef _ODE_THREADING_FAKE_SYNC_H_
