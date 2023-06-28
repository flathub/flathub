/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading support header file.                                        *
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
 *   ODE threading support interfaces	
 */


#ifndef _ODE_THREADING_H_
#define _ODE_THREADING_H_

#include <ode/odeconfig.h>
// Include <time.h> since time_t is used and it is not available by default in some OSes
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif


struct dxThreadingImplementation;
typedef struct dxThreadingImplementation *dThreadingImplementationID;

typedef unsigned dmutexindex_t;
struct dxMutexGroup;
typedef struct dxMutexGroup *dMutexGroupID;

/**
 * @brief Allocates a group of muteces.
 *
 * The Mutex allocated do not need to support recursive locking.
 *
 * The Mutex names are provided to aid in debugging and thread state tracking.
 *
 * @param impl Threading implementation ID
 * @param Mutex_count Number of Mutex to create
 * @Mutex_names_ptr Pointer to optional Mutex names array to be associated with individual Mutex
 * @returns MutexGroup ID or NULL if error occurred.
 *
 * @ingroup threading
 * @see dMutexGroupFreeFunction
 * @see dMutexGroupMutexLockFunction
 * @see dMutexGroupMutexUnlockFunction
 */
typedef dMutexGroupID dMutexGroupAllocFunction (dThreadingImplementationID impl, dmutexindex_t Mutex_count, const char *const *Mutex_names_ptr/*=NULL*/);

/**
 * @brief Deletes a group of muteces.
 *
 * @param impl Threading implementation ID
 * @param mutex_group Mutex group to deallocate
 *
 * @ingroup threading
 * @see dMutexGroupAllocFunction
 * @see dMutexGroupMutexLockFunction
 * @see dMutexGroupMutexUnlockFunction
 */
typedef void dMutexGroupFreeFunction (dThreadingImplementationID impl, dMutexGroupID mutex_group);

/**
 * @brief Locks a mutex in a group of muteces.
 *
 * The function is to block execution until requested mutex can be locked.
 *
 * Note: Mutex provided may not support recursive locking. Calling this function
 * while mutex is already locked by current thread will result in unpredictable behavior.
 *
 * @param impl Threading implementation ID
 * @param mutex_group Mutex group to use for locking
 * @param mutex_index The index of mutex to be locked (0..Mutex_count - 1)
 *
 * @ingroup threading
 * @see dMutexGroupAllocFunction
 * @see dMutexGroupFreeFunction
 * @see dMutexGroupMutexUnlockFunction
 */
typedef void dMutexGroupMutexLockFunction (dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);

/**
 * @brief Attempts to lock a mutex in a group of muteces.
 *
 * The function is to lock the requested mutex if it is unoccupied or 
 * immediately return failure if mutex is already locked by other thread.
 *
 * Note: Mutex provided may not support recursive locking. Calling this function
 * while mutex is already locked by current thread will result in unpredictable behavior.
 *
 * @param impl Threading implementation ID
 * @param mutex_group Mutex group to use for locking
 * @param mutex_index The index of mutex to be locked (0..Mutex_count - 1)
 * @returns 1 for success (mutex is locked) and 0 for failure (mutex is not locked)
 *
 * @ingroup threading
 * @see dMutexGroupAllocFunction
 * @see dMutexGroupFreeFunction
 * @see dMutexGroupMutexLockFunction
 * @see dMutexGroupMutexUnlockFunction
 */
/* typedef int dMutexGroupMutexTryLockFunction (dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);*/

/**
 * @brief Unlocks a mutex in a group of muteces.
 *
 * The function is to unlock the given mutex provided it had been locked before.
 *
 * @param impl Threading implementation ID
 * @param mutex_group Mutex group to use for unlocking
 * @param mutex_index The index of mutex to be unlocked (0..Mutex_count - 1)
 *
 * @ingroup threading
 * @see dMutexGroupAllocFunction
 * @see dMutexGroupFreeFunction
 * @see dMutexGroupMutexLockFunction
 */
typedef void dMutexGroupMutexUnlockFunction (dThreadingImplementationID impl, dMutexGroupID mutex_group, dmutexindex_t mutex_index);


struct dxCallReleasee;
typedef struct dxCallReleasee *dCallReleaseeID;

struct dxCallWait;
typedef struct dxCallWait *dCallWaitID;

typedef size_t ddependencycount_t;
typedef ptrdiff_t ddependencychange_t;
typedef size_t dcallindex_t;
typedef int dThreadedCallFunction(void *call_context, dcallindex_t instance_index, 
  dCallReleaseeID this_releasee);

typedef struct dxThreadedWaitTime
{
  time_t          wait_sec;
  unsigned long   wait_nsec;

} dThreadedWaitTime;


/**
 * @brief Allocates a Wait ID that can be used to wait for a call.
 *
 * @param impl Threading implementation ID
 * @returns Wait ID or NULL if error occurred
 *
 * @ingroup threading
 * @see dThreadedCallWaitResetFunction
 * @see dThreadedCallWaitFreeFunction
 * @see dThreadedCallPostFunction
 * @see dThreadedCallWaitFunction
 */
typedef dCallWaitID dThreadedCallWaitAllocFunction(dThreadingImplementationID impl);

/**
 * @brief Resets a Wait ID so that it could be used to wait for another call.
 *
 * @param impl Threading implementation ID
 * @param call_wait Wait ID to reset
 *
 * @ingroup threading
 * @see dThreadedCallWaitAllocFunction
 * @see dThreadedCallWaitFreeFunction
 * @see dThreadedCallPostFunction
 * @see dThreadedCallWaitFunction
 */
typedef void dThreadedCallWaitResetFunction(dThreadingImplementationID impl, dCallWaitID call_wait);

/**
 * @brief Frees a Wait ID.
 *
 * @param impl Threading implementation ID
 * @param call_wait Wait ID to delete
 *
 * @ingroup threading
 * @see dThreadedCallWaitAllocFunction
 * @see dThreadedCallPostFunction
 * @see dThreadedCallWaitFunction
 */
typedef void dThreadedCallWaitFreeFunction(dThreadingImplementationID impl, dCallWaitID call_wait);


/**
 * @brief Post a function to be called in another thread.
 *
 * A call is scheduled to be executed asynchronously.
 *
 * A @a out_summary_fault variable can be provided for call to accumulate any
 * possible faults from its execution and execution of any possible sub-calls.
 * This variable gets result that @a call_func returns. Also, if dependent calls 
 * are executed after the call already exits, the variable is also going to be 
 * updated with results of all those calls before control is released to master.
 *
 * @a out_post_releasee parameter receives a value of @c dCallReleaseeID that can 
 * later be used for @a dependent_releasee while scheduling sub-calls to make 
 * current call depend on them. The value is only returned if @a dependencies_count 
 * is not zero (i.e. if any dependencies are expected at all). The call is not going 
 * to start until all its dependencies complete.
 *
 * In case if number of dependencies is unknown in advance 1 can be passed on call
 * scheduling. Then @c dThreadedCallDependenciesCountAlterFunction can be used to
 * add one more extra dependencies before scheduling each subcall. And then, after
 * all sub-calls had been scheduled, @c dThreadedCallDependenciesCountAlterFunction
 * can be used again to subtract initial extra dependency from total number.
 * Adding one dependency in advance is necessary to obtain releasee ID and to make 
 * sure the call will not start and will not terminate before all sub-calls are scheduled.
 *
 * Extra dependencies can also be added from the call itself after it has already 
 * been started (with parameter received in @c dThreadedCallFunction). 
 * In that case those dependencies will start immediately or after call returns 
 * but the call's master will not be released/notified until all additional
 * dependencies complete. This can be used to schedule sub-calls from a call and 
 * then pass own job to another sub-call dependent on those initial sub-calls.
 *
 * By using @ call_wait it is possible to assign a Wait ID that can later 
 * be passed into @c dThreadedCallWaitFunction to wait for call completion.
 *
 * If @a call_name is available (and it should!) the string must remain valid until
 * after call completion. In most cases this should be a static string literal.
 * 
 * Since the function is an analogue of normal method call it is not supposed to fail.
 * Any complications with resource allocation on call scheduling should be 
 * anticipated, avoided and worked around by implementation.
 *
 * @param impl Threading implementation ID
 * @param out_summary_fault Optional pointer to variable to be set to 1 if function 
 *        call (or any sub-call) fails internally, or 0 if all calls return success
 * @param out_post_releasee Optional pointer to variable to receive releasee ID 
 *        associated with the call
 * @param dependencies_count Number of dependencies that are going to reference
 *        this call as dependent releasee
 * @param dependent_releasee Optional releasee ID to reference with this call
 * @param call_wait Optional Wait ID that can later be used to wait for the call
 * @param call_func Pointer to function to be called
 * @param call_context Context parameter to be passed into the call
 * @param instance_index Index parameter to be passed into the call
 * @param call_name Optional name to be associated with the call (for debugging and state tracking)
 *
 * @ingroup threading
 * @see dThreadedCallWaitFunction
 * @see dThreadedCallDependenciesCountAlterFunction
 * @see dThreadingImplResourcesForCallsPreallocateFunction
 */
typedef void dThreadedCallPostFunction(dThreadingImplementationID impl, int *out_summary_fault/*=NULL*/, 
  dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
  dCallWaitID call_wait/*=NULL*/, 
  dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
  const char *call_name/*=NULL*/);

/**
 * @brief Add or remove extra dependencies from call that has been scheduled
 * or is in process of execution.
 *
 * Extra dependencies can be added to a call if exact number of sub-calls is
 * not known in advance at the moment the call is scheduled. Also, some dependencies
 * can be removed if sub-calls were planned but then dropped. 
 *
 * In case if total dependency count of a call reaches zero by result of invoking 
 * this function, the call is free to start executing immediately.
 *
 * After the call execution had been started, any additional dependencies can only
 * be added from the call function itself!
 *
 * @param impl Threading implementation ID
 * @param target_releasee ID of releasee to apply dependencies count change to
 * @param dependencies_count_change Number of dependencies to add or remove
 *
 * @ingroup threading
 * @see dThreadedCallPostFunction
 */
typedef void dThreadedCallDependenciesCountAlterFunction(dThreadingImplementationID impl, dCallReleaseeID target_releasee, 
  ddependencychange_t dependencies_count_change);

/**
 * @brief Wait for a posted call to complete.
 *
 * Function blocks until a call identified by @a call_wait completes or
 * timeout elapses.
 *
 * IT IS ILLEGAL TO INVOKE THIS FUNCTION FROM WITHIN A THREADED CALL!
 * This is because doing so will block a physical thread and will require 
 * increasing worker thread count to avoid starvation. Use call dependencies 
 * if it is necessary make sure sub-calls have been completed instead!
 *
 * If @a timeout_time_ptr is NULL, the function waits without time limit. If @a timeout_time_ptr
 * points to zero value, the function only checks status and does not block.
 *
 * If @a wait_name is available (and it should!) the string must remain valid for
 * the duration of wait. In most cases this should be a static string literal.
 * 
 * Function is not expected to return failures caused by system call faults as 
 * those are hardly ever possible to be handled in this case anyway. In event of 
 * system call fault the function is supposed to terminate application.
 *
 * @param impl Threading implementation ID
 * @param out_wait_status Optional pointer to variable to receive 1 if waiting succeeded
 *        or 0 in case of timeout
 * @param call_wait Wait ID that had been passed to scheduling a call that needs to be waited for
 * @param timeout_time_ptr Optional pointer to time specification the wait must not
 *        last longer than (pass NULL for infinite timeout)
 * @param wait_name Optional name to be associated with the wait (for debugging and state tracking)
 *
 * @ingroup threading
 * @see dThreadedCallPostFunction
 */
typedef void dThreadedCallWaitFunction(dThreadingImplementationID impl, int *out_wait_status/*=NULL*/, 
  dCallWaitID call_wait, const dThreadedWaitTime *timeout_time_ptr/*=NULL*/, 
  const char *wait_name/*=NULL*/);

/**
 * @brief Retrieve number of active threads that serve the implementation.
 *
 * @param impl Threading implementation ID
 * @returns Number of active threads
 *
 * @ingroup threading
 */
typedef unsigned dThreadingImplThreadCountRetrieveFunction(dThreadingImplementationID impl);

/**
 * @brief Preallocate resources to handle posted calls.
 *
 * The function is intended to make sure enough resources is preallocated for the
 * implementation to be able to handle posted calls. Then @c max_simultaneous_calls_estimate
 * is an estimate of how many posted calls can potentially be active or scheduled 
 * at the same time. The value is usually derived from the way the calls are posted 
 * in library code and dependencies between them.
 * 
 * @warning While working on an implementation be prepared that the estimate provided 
 * yet rarely but theoretically can be exceeded due to unpredictability of thread execution.
 *
 * This function is normally going to be invoked by library each time it is entered
 * from outside to do the job but before any threaded calls are going to be posted.
 *
 * @param impl Threading implementation ID
 * @param max_simultaneous_calls_estimate An estimated number of calls that can be posted simultaneously
 * @returns 1 or 0 to indicate success or failure
 *
 * @ingroup threading
 * @see dThreadedCallPostFunction
 */
typedef int dThreadingImplResourcesForCallsPreallocateFunction(dThreadingImplementationID impl, 
  ddependencycount_t max_simultaneous_calls_estimate);


/**
 * @brief An interface structure with function pointers to be provided by threading implementation.
 */
typedef struct dxThreadingFunctionsInfo
{
  unsigned struct_size;
  
  dMutexGroupAllocFunction *alloc_mutex_group;
  dMutexGroupFreeFunction *free_mutex_group;
  dMutexGroupMutexLockFunction *lock_group_mutex;
  dMutexGroupMutexUnlockFunction *unlock_group_mutex;

  dThreadedCallWaitAllocFunction *alloc_call_wait;
  dThreadedCallWaitResetFunction *reset_call_wait;
  dThreadedCallWaitFreeFunction *free_call_wait;

  dThreadedCallPostFunction *post_call;
  dThreadedCallDependenciesCountAlterFunction *alter_call_dependencies_count;
  dThreadedCallWaitFunction *wait_call;

  dThreadingImplThreadCountRetrieveFunction *retrieve_thread_count;
  dThreadingImplResourcesForCallsPreallocateFunction *preallocate_resources_for_calls; 

  /* 
   * Beware of Jon Watte's anger if you dare to uncomment this!
   * May cryptic text below be you a warning!
   * Стародавні легенди розказують, що кожного сміливця, хто наважиться порушити табу 
   * і відкрити заборонений код, спіткає страшне прокляття і він відразу почне робити 
   * одні лиш помилки.
   *
   * dMutexGroupMutexTryLockFunction *trylock_group_mutex;
   */

} dThreadingFunctionsInfo;


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ODE_THREADING_H_ */
