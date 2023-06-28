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


#ifndef _ODE_GIMPACT_GIM_CONTACT_ACCESSOR_H_
#define _ODE_GIMPACT_GIM_CONTACT_ACCESSOR_H_


struct dxGIMCContactAccessor
{
    dxGIMCContactAccessor(GIM_CONTACT *ptrimeshcontacts, dGeomID g1, dGeomID g2) : m_ptrimeshcontacts(ptrimeshcontacts), m_g1(g1), m_g2(g2), m_gotside2ovr(false), m_side2ovr() {}
    dxGIMCContactAccessor(GIM_CONTACT *ptrimeshcontacts, dGeomID g1, dGeomID g2, int side2ovr) : m_ptrimeshcontacts(ptrimeshcontacts), m_g1(g1), m_g2(g2), m_gotside2ovr(true), m_side2ovr(side2ovr) {}

    dReal RetrieveDepthByIndex(unsigned index) const { return m_ptrimeshcontacts[index].m_depth; }

    void ExportContactGeomByIndex(dContactGeom *pcontact, unsigned index) const
    {
        const GIM_CONTACT *ptrimeshcontact = m_ptrimeshcontacts + index;
        pcontact->pos[0] = ptrimeshcontact->m_point[0];
        pcontact->pos[1] = ptrimeshcontact->m_point[1];
        pcontact->pos[2] = ptrimeshcontact->m_point[2];
        pcontact->pos[3] = REAL(1.0);

        pcontact->normal[0] = ptrimeshcontact->m_normal[0];
        pcontact->normal[1] = ptrimeshcontact->m_normal[1];
        pcontact->normal[2] = ptrimeshcontact->m_normal[2];
        pcontact->normal[3] = 0;

        pcontact->depth = ptrimeshcontact->m_depth;
        pcontact->g1 = m_g1;
        pcontact->g2 = m_g2;
        pcontact->side1 = ptrimeshcontact->m_feature1;
        pcontact->side2 = !m_gotside2ovr ? ptrimeshcontact->m_feature2 : m_side2ovr;
    }

    const GIM_CONTACT *m_ptrimeshcontacts;
    dGeomID         m_g1, m_g2;
    bool            m_gotside2ovr;
    int             m_side2ovr;
};


#endif	//_ODE_GIMPACT_GIM_CONTACT_ACCESSOR_H_
