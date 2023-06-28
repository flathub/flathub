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

#include <ode/collision.h>
#include "config.h"


#if dTRIMESH_ENABLED && dTRIMESH_GIMPACT

#include "gimpact_contact_export_helper.h"
#include "error.h"


/*static */
dReal dxGImpactContactsExportHelper::FindContactsMarginalDepth(dReal *pdepths, unsigned contactcount, unsigned maxcontacts, dReal mindepth, dReal maxdepth)
{
    dReal result;

    while (true)
    {
        dReal firstdepth = REAL(0.5) * (mindepth + maxdepth);
        dReal lowdepth = maxdepth, highdepth = mindepth;

        unsigned marginindex = 0;
        unsigned highindex = marginindex;
        dIASSERT(contactcount != 0);

        for (unsigned i = 0; i < contactcount; i++)
        {
            dReal depth = pdepths[i];

            if (depth < firstdepth)
            {
                dReal temp = pdepths[marginindex]; pdepths[highindex++] = temp; pdepths[marginindex++] = depth;
                if (highdepth < depth) { highdepth = depth; }
            }
            else if (depth > firstdepth)
            {
                pdepths[highindex++] = depth;
                if (depth < lowdepth) { lowdepth = depth; }
            }
        }

        unsigned countabove = highindex - marginindex;
        if (maxcontacts < countabove)
        {
            contactcount = countabove;
            pdepths += marginindex;
            mindepth = lowdepth;
        }
        else if (maxcontacts == countabove)
        {
            result = dNextAfter(firstdepth, dInfinity);
            break;
        }
        else
        {
            unsigned countbelow = marginindex;
            if (maxcontacts <= contactcount - countbelow)
            {
                result = firstdepth;
                break;
            }

            maxcontacts -= contactcount - countbelow;
            contactcount = countbelow;
            maxdepth = highdepth;
        }
    }

    return result;
}


#endif // #if dTRIMESH_ENABLED && dTRIMESH_GIMPACT

