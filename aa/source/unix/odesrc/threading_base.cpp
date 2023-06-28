/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
 *                                                                       *
 * Threading base wrapper class implementation file.                     *
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


#include <ode/common.h>
#include "config.h"
#include "error.h"
#include "threading_base.h"


void dxThreadingBase::PostThreadedCallsGroup(
    int *out_summary_fault/*=NULL*/, 
    ddependencycount_t member_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, 
    const char *call_name/*=NULL*/) const
{
    dIASSERT(member_count != 0);

    dThreadingImplementationID impl;
    const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);

    for (unsigned member_index = 0; member_index != member_count; ++member_index) {
        // Post individual group member jobs
        functions->post_call(impl, out_summary_fault, NULL, 0, dependent_releasee, NULL, call_func, call_context, member_index, call_name);
    }
}

void dxThreadingBase::PostThreadedCallsIndexOverridenGroup(int *out_summary_fault/*=NULL*/, 
    ddependencycount_t member_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, unsigned index_override, 
    const char *call_name/*=NULL*/) const
{
    dIASSERT(member_count != 0);

    dThreadingImplementationID impl;
    const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);

    for (unsigned member_index = 0; member_index != member_count; ++member_index) {
        // Post individual group member jobs
        functions->post_call(impl, out_summary_fault, NULL, 0, dependent_releasee, NULL, call_func, call_context, index_override, call_name);
    }
}

void dxThreadingBase::PostThreadedCallForUnawareReleasee(
    int *out_summary_fault/*=NULL*/, 
    dCallReleaseeID *out_post_releasee/*=NULL*/, ddependencycount_t dependencies_count, dCallReleaseeID dependent_releasee/*=NULL*/, 
    dCallWaitID call_wait/*=NULL*/, 
    dThreadedCallFunction *call_func, void *call_context, dcallindex_t instance_index, 
    const char *call_name/*=NULL*/) const
{
    dThreadingImplementationID impl;
    const dxThreadingFunctionsInfo *functions = FindThreadingImpl(impl);

    functions->alter_call_dependencies_count(impl, dependent_releasee, 1);
    functions->post_call(impl, out_summary_fault, out_post_releasee, dependencies_count, dependent_releasee, call_wait, call_func, call_context, instance_index, call_name);
}

const dxThreadingFunctionsInfo *dxThreadingBase::FindThreadingImpl(dThreadingImplementationID &out_impl_found) const
{
    const dxThreadingFunctionsInfo *functions_found = GetFunctionsInfo();

    if (functions_found != NULL)
    {
        out_impl_found = GetThreadingImpl();
    }
    else
    {
        functions_found = m_default_impl_provider->RetrieveThreadingDefaultImpl(out_impl_found);
    }

    return functions_found;
}
