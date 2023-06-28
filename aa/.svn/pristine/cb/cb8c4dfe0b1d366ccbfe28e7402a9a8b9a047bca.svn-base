/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Builtin ODE threading implementation header.                          *
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
 *  A threading implementation built into ODE for those who does not care to 
 *  or can't implement an own one.
 */

#ifndef _ODE_THREADING_IMPL_H_
#define _ODE_THREADING_IMPL_H_


#include <ode/odeconfig.h>
#include <ode/threading.h>


#ifdef __cplusplus
extern "C" {
#endif


struct dxThreadingThreadPool;
typedef struct dxThreadingThreadPool *dThreadingThreadPoolID;


/**
 * @brief Allocates built-in self-threaded threading implementation object.
 *
 * A self-threaded implementation is a type of implementation that performs 
 * processing of posted calls by means of caller thread itself. This type of 
 * implementation does not need thread pool to serve it.
 * 
 * The processing is arranged in a way to prevent call stack depth growth 
 * as more and more nested calls are posted.
 *
 * Note that it is not necessary to create and assign a self-threaded 
 * implementation to a world, as there is a global one used by default 
 * if no implementation is explicitly assigned. You should only assign 
 * each world an individual threading implementation instance if simulations 
 * need to be run in parallel in multiple threads for the worlds.
 *
 * @returns ID of object allocated or NULL on failure
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingFreeImplementation
 */
ODE_API dThreadingImplementationID dThreadingAllocateSelfThreadedImplementation();

/**
 * @brief Allocates built-in multi-threaded threading implementation object.
 *
 * A multi-threaded implementation is a type of implementation that has to be 
 * served with a thread pool. The thread pool can be either the built-in ODE object
 * or set of external threads that dedicate themselves to this purpose and stay
 * in ODE until implementation releases them.
 * 
 * @returns ID of object allocated or NULL on failure
 * 
 * @ingroup threading
 * @see dThreadingThreadPoolServeMultiThreadedImplementation
 * @see dExternalThreadingServeMultiThreadedImplementation
 * @see dThreadingFreeImplementation
 */
ODE_API dThreadingImplementationID dThreadingAllocateMultiThreadedImplementation();

/**
 * @brief Retrieves the functions record of a built-in threading implementation.
 *
 * The implementation can be the one allocated by ODE (from @c dThreadingAllocateMultiThreadedImplementation). 
 * Do not use this function with self-made custom implementations - 
 * they should be bundled with their own set of functions.
 * 
 * @param impl Threading implementation ID
 * @returns Pointer to associated functions structure
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 */
ODE_API const dThreadingFunctionsInfo *dThreadingImplementationGetFunctions(dThreadingImplementationID impl);

/**
 * @brief Requests a built-in implementation to release threads serving it.
 *
 * The function unblocks threads employed in implementation serving and lets them 
 * return to from where they originate. It's the responsibility of external code 
 * to make sure all the calls to ODE that might be dependent on given threading 
 * implementation object had already returned before this call is made. If threading 
 * implementation is still processing some posted calls while this function is 
 * invoked the behavior is implementation dependent.
 *
 * This call is to be used to request the threads to be released before waiting 
 * for them in host pool or before waiting for them to exit. Implementation object 
 * must not be destroyed before it is known that all the serving threads have already 
 * returned from it. If implementation needs to be reused after this function is called
 * and all the threads have exited from it a call to @c dThreadingImplementationCleanupForRestart
 * must be made to restore internal state of the object.
 *
 * If this function is called for self-threaded built-in threading implementation
 * the call has no effect.
 * 
 * @param impl Threading implementation ID
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingImplementationCleanupForRestart
 */
ODE_API void dThreadingImplementationShutdownProcessing(dThreadingImplementationID impl);

/**
 * @brief Restores built-in implementation's state to let it be reused after shutdown.
 *
 * If a multi-threaded built-in implementation needs to be reused after a call
 * to @c dThreadingImplementationShutdownProcessing this call is to be made to 
 * restore object's internal state. After that the implementation can be served again.
 *
 * If this function is called for self-threaded built-in threading implementation
 * the call has no effect.
 * 
 * @param impl Threading implementation ID
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingImplementationShutdownProcessing
 */
ODE_API void dThreadingImplementationCleanupForRestart(dThreadingImplementationID impl);

/**
 * @brief Deletes an instance of built-in threading implementation.
 *
 * @warning A care must be taken to make sure the implementation is unassigned
 * from all the objects it was assigned to and that there are no more threads 
 * serving it before attempting to call this function.
 *
 * @param impl Threading implementation ID
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 */
ODE_API void dThreadingFreeImplementation(dThreadingImplementationID impl);


typedef void (dThreadReadyToServeCallback)(void *callback_context);

/**
 * @brief An entry point for external threads that would like to serve a built-in 
 * threading implementation object.
 *
 * A thread that calls this function remains blocked in ODE and serves implementation
 * object @p impl until being released with @c dThreadingImplementationShutdownProcessing call.
 * This function can be used to provide external threads instead of ODE's built-in
 * thread pools.
 *
 * The optional callback @readiness_callback is called after the thread has reached 
 * and has registered within the implementation. The implementation should not 
 * be used until all dedicated threads register within it as otherwise it will not
 * have accurate view of the execution resources available.
 *
 * @param impl Threading implementation ID
 * @param readiness_callback Optional readiness callback to be called after thread enters the implementation
 * @param callback_context A value to be passed as parameter to readiness callback
 * 
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingImplementationShutdownProcessing
 */
ODE_API void dExternalThreadingServeMultiThreadedImplementation(dThreadingImplementationID impl, 
  dThreadReadyToServeCallback *readiness_callback/*=NULL*/, void *callback_context/*=NULL*/);


/**
 * @brief Creates an instance of built-in thread pool object that can be used to serve
 * multi-threaded threading implementations.
 *
 * The threads allocated inherit priority of caller thread. Their affinity is not
 * explicitly adjusted and gets the value the system assigns by default. Threads 
 * have their stack memory fully committed immediately on start. On POSIX platforms 
 * threads are started with all the possible signals blocked. Threads execute 
 * calls to @c dAllocateODEDataForThread with @p ode_data_allocate_flags 
 * on initialization.
 *
 * On POSIX platforms this function must be called with signals masked 
 * or other measures must be taken to prevent reception of signals by calling thread 
 * for the duration of the call.
 * 
 * @param thread_count Number of threads to start in pool
 * @param stack_size Size of stack to be used for every thread or 0 for system default value
 * @param ode_data_allocate_flags Flags to be passed to @c dAllocateODEDataForThread on behalf of each thread
 * @returns ID of object allocated or NULL on failure
 *
 * @ingroup threading
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingImplementationShutdownProcessing
 * @see dThreadingFreeThreadPool
 */
ODE_API dThreadingThreadPoolID dThreadingAllocateThreadPool(unsigned thread_count, 
  size_t stack_size, unsigned int ode_data_allocate_flags, void *reserved/*=NULL*/);

/**
 * @brief Commands an instance of built-in thread pool to serve a built-in multi-threaded 
 * threading implementation.
 *
 * A pool can only serve one threading implementation at a time. 
 * Call @c dThreadingImplementationShutdownProcessing to release pool threads 
 * from implementation serving and make them idle. Pool threads must be released 
 * from any implementations before pool is attempted to be deleted.
 *
 * This function waits for threads to register within implementation before returning.
 * So, after the function call exits the implementation can be used immediately.
 * 
 * @param pool Thread pool ID to serve the implementation
 * @param impl Implementation ID of implementation to be served
 *
 * @ingroup threading
 * @see dThreadingAllocateThreadPool
 * @see dThreadingAllocateMultiThreadedImplementation
 * @see dThreadingImplementationShutdownProcessing
 */
ODE_API void dThreadingThreadPoolServeMultiThreadedImplementation(dThreadingThreadPoolID pool, dThreadingImplementationID impl);

/**
 * @brief Waits until all pool threads are released from threading implementation 
 * they might be serving.
 *
 * The function can be used after a call to @c dThreadingImplementationShutdownProcessing
 * to make sure all the threads have already been released by threading implementation 
 * and it can be deleted or it can be cleaned up for restart and served by another pool
 * or this pool's threads can be used to serve another threading implementation.
 *
 * Note that is it not necessary to call this function before pool destruction
 * since @c dThreadingFreeThreadPool performs similar wait operation implicitly on its own.
 * 
 * It is OK to call this function even if pool was not serving any threading implementation
 * in which case the call exits immediately with minimal delay.
 * 
 * @param pool Thread pool ID to wait for
 *
 * @ingroup threading
 * @see dThreadingAllocateThreadPool
 * @see dThreadingImplementationShutdownProcessing
 * @see dThreadingFreeThreadPool
 */
ODE_API void dThreadingThreadPoolWaitIdleState(dThreadingThreadPoolID pool);

/**
 * @brief Deletes a built-in thread pool instance.
 *
 * The pool threads must be released from any implementations they might be serving
 * before this function is called. Otherwise the call is going to block 
 * and wait until pool's threads return.
 * 
 * @param pool Thread pool ID to delete
 *
 * @ingroup threading
 * @see dThreadingAllocateThreadPool
 * @see dThreadingImplementationShutdownProcessing
 */
ODE_API void dThreadingFreeThreadPool(dThreadingThreadPoolID pool);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _ODE_THREADING_IMPL_H_ */
