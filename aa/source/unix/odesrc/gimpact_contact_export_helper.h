/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
 * All rights reserved.  Email: russ@q12.org   Web: www.q12.org          *
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


#ifndef _ODE_GIMPACT_CONTACT_EXPORT_HELPER_H_
#define _ODE_GIMPACT_CONTACT_EXPORT_HELPER_H_


#include "collision_kernel.h"
#include "collision_util.h"
#include "util.h"


#ifndef ALLOCA
#define ALLOCA(x) dALLOCA16(x)
#endif


struct dxGImpactContactsExportHelper
{
public:
    template<class dxGImpactContactAccessor>
    static unsigned ExportMaxDepthGImpactContacts(dxGImpactContactAccessor &srccontacts, unsigned contactcount,
        int Flags, dContactGeom* Contacts, int Stride)
    {
        unsigned result;

        unsigned maxcontacts = (unsigned)(Flags & NUMC_MASK);
        if (contactcount > maxcontacts)
        {
            ExportExcesssiveContacts(srccontacts, contactcount, Flags, Contacts, Stride);
            result = maxcontacts;
        }
        else
        {
            ExportFitContacts(srccontacts, contactcount, Flags, Contacts, Stride);
            result = contactcount;
        }

        return result;
    }

private:
    template<class dxGImpactContactAccessor>
    static void ExportExcesssiveContacts(dxGImpactContactAccessor &srccontacts, unsigned contactcount,
        int Flags, dContactGeom* Contacts, int Stride);
    template<class dxGImpactContactAccessor>
    static void ExportFitContacts(dxGImpactContactAccessor &srccontacts, unsigned contactcount,
        int Flags, dContactGeom* Contacts, int Stride);
    template<class dxGImpactContactAccessor>
    static dReal FindContactsMarginalDepth(dxGImpactContactAccessor &srccontacts, unsigned contactcount, unsigned maxcontacts);
    static dReal FindContactsMarginalDepth(dReal *pdepths, unsigned contactcount, unsigned maxcontacts, dReal mindepth, dReal maxdepth);
};


template<class dxGImpactContactAccessor> 
/*static */
void dxGImpactContactsExportHelper::ExportExcesssiveContacts(dxGImpactContactAccessor &srccontacts, unsigned contactcount,
    int Flags, dContactGeom* Contacts, int Stride)
{
    unsigned maxcontacts = (unsigned)(Flags & NUMC_MASK);
    dReal marginaldepth = FindContactsMarginalDepth(srccontacts, contactcount, maxcontacts);

    unsigned contactshead = 0, contacttail = maxcontacts;
    for (unsigned i = 0; i < contactcount; i++)
    {
        dReal depth = srccontacts.RetrieveDepthByIndex(i);

        if (depth > marginaldepth)
        {
            dContactGeom *pcontact = SAFECONTACT(Flags, Contacts, contactshead, Stride);
            srccontacts.ExportContactGeomByIndex(pcontact, i);

            if (++contactshead == maxcontacts)
            {
                break;
            }
        }
        else if (depth == marginaldepth && contactshead < contacttail)
        {
            --contacttail;

            dContactGeom *pcontact = SAFECONTACT(Flags, Contacts, contacttail, Stride);
            srccontacts.ExportContactGeomByIndex(pcontact, i);
        }
    }
}

template<class dxGImpactContactAccessor>
/*static */
void dxGImpactContactsExportHelper::ExportFitContacts(dxGImpactContactAccessor &srccontacts, unsigned contactcount,
    int Flags, dContactGeom* Contacts, int Stride)
{
    for (unsigned i = 0; i < contactcount; i++)
    {
        dContactGeom *pcontact = SAFECONTACT(Flags, Contacts, i, Stride);

        srccontacts.ExportContactGeomByIndex(pcontact, i);
    }
}

template<class dxGImpactContactAccessor>
/*static */
dReal dxGImpactContactsExportHelper::FindContactsMarginalDepth(dxGImpactContactAccessor &srccontacts, unsigned contactcount, unsigned maxcontacts)
{
    dReal result;

    dReal *pdepths = (dReal *)ALLOCA(contactcount * sizeof(dReal));
    unsigned marginindex = 0;
    unsigned highindex = marginindex;

    dReal firstdepth = srccontacts.RetrieveDepthByIndex(0);
    dReal mindepth = firstdepth, maxdepth = firstdepth;
    dIASSERT(contactcount > 1);

    for (unsigned i = 1; i < contactcount; i++)
    {
        dReal depth = srccontacts.RetrieveDepthByIndex(i);

        if (depth < firstdepth)
        {
            dReal temp = pdepths[marginindex]; pdepths[highindex++] = temp; pdepths[marginindex++] = depth;
            if (depth < mindepth) { mindepth = depth; }
        }
        else if (depth > firstdepth)
        {
            pdepths[highindex++] = depth;
            if (maxdepth < depth) { maxdepth = depth; }
        }
    }

    unsigned countabove = highindex - marginindex;
    if (maxcontacts < countabove)
    {
        result = FindContactsMarginalDepth(pdepths + marginindex, countabove, maxcontacts, firstdepth, maxdepth);
    }
    else if (maxcontacts == countabove)
    {
        result = dNextAfter(firstdepth, dInfinity);
    }
    else
    {
        unsigned countbelow = marginindex;
        if (maxcontacts <= contactcount - countbelow)
        {
            result = firstdepth;
        }
        else
        {
            result = FindContactsMarginalDepth(pdepths, countbelow, maxcontacts - (contactcount - countbelow), mindepth, firstdepth);
        }
    }

    return result;
}


#endif	//_ODE_GIMPACT_CONTACT_EXPORT_HELPER_H_
