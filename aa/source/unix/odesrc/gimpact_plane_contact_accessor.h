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


#ifndef _ODE_GIMPACT_PLANE_CONTACT_ACCESSOR_H_
#define _ODE_GIMPACT_PLANE_CONTACT_ACCESSOR_H_


struct dxPlaneContactAccessor
{
    dxPlaneContactAccessor(const vec4f *planecontact_results, const dReal *plane, dGeomID g1, dGeomID g2) : m_planecontact_results(planecontact_results), m_plane(plane), m_g1(g1), m_g2(g2) {}

    dReal RetrieveDepthByIndex(unsigned index) const { return m_planecontact_results[index][3]; }

    void ExportContactGeomByIndex(dContactGeom *pcontact, unsigned index) const
    {
        const vec4f *planecontact = m_planecontact_results + index;

        pcontact->pos[0] = (*planecontact)[0];
        pcontact->pos[1] = (*planecontact)[1];
        pcontact->pos[2] = (*planecontact)[2];
        pcontact->pos[3] = REAL(1.0);

        const dReal *plane = m_plane;
        pcontact->normal[0] = plane[0];
        pcontact->normal[1] = plane[1];
        pcontact->normal[2] = plane[2];
        pcontact->normal[3] = 0;

        pcontact->depth = (*planecontact)[3];
        pcontact->g1 = m_g1; // trimesh geom
        pcontact->g2 = m_g2; // plane geom
        pcontact->side1 = -1; // note: don't have the triangle index, but OPCODE *does* do this properly
        pcontact->side2 = -1;
    }

    const vec4f     *m_planecontact_results;
    const dReal     *m_plane;
    dGeomID         m_g1, m_g2;
};


#endif	//_ODE_GIMPACT_PLANE_CONTACT_ACCESSOR_H_
