/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001,2002 Russell L. Smith.       *
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


#include <ode/odeconfig.h>
#include "config.h"
#include "dhinge.h"
#include "joint_internal.h"

/*
 * Double Hinge joint
 */

dxJointDHinge::dxJointDHinge(dxWorld* w) :
    dxJointDBall(w)
{
    dSetZero(axis1, 3);
    dSetZero(axis2, 3);
}


void
dxJointDHinge::getSureMaxInfo( SureMaxInfo* info )
{
    info->max_m = 4;
}


void
dxJointDHinge::getInfo1( dxJoint::Info1* info )
{
    info->m = 4;
    info->nub = 4;
}


void
dxJointDHinge::getInfo2( dReal worldFPS, dReal worldERP, 
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
    int *findex )
{
    dxJointDBall::getInfo2( worldFPS, worldERP, rowskip, J1, J2, pairskip, pairRhsCfm, pairLoHi, findex ); // sets row0
    
    dVector3 globalAxis1;
    dBodyVectorToWorld(node[0].body, axis1[0], axis1[1], axis1[2], globalAxis1);

    dxBody *body1 = node[1].body;

    // angular constraints, perpendicular to axis
    dVector3 p, q;
    dPlaneSpace(globalAxis1, p, q);

    dCopyVector3(J1 + rowskip + GI2__JA_MIN, p);
    if ( body1 ) {
        dCopyNegatedVector3(J2 + rowskip + GI2__JA_MIN, p);
    }

    dCopyVector3(J1 + 2 * rowskip + GI2__JA_MIN, q);
    if ( body1 ) {
        dCopyNegatedVector3(J2 + 2 * rowskip + GI2__JA_MIN, q);
    }

    dVector3 globalAxis2;
    if ( body1 ) {
        dBodyVectorToWorld(body1, axis2[0], axis2[1], axis2[2], globalAxis2);
    } else {
        dCopyVector3(globalAxis2, axis2);
    }
    
    // similar to the hinge joint
    dVector3 u;
    dCalcVectorCross3(u, globalAxis1, globalAxis2);

    const dReal k = worldFPS * this->erp;
    pairRhsCfm[pairskip + GI2_RHS] = k * dCalcVectorDot3( u, p );
    pairRhsCfm[2 * pairskip + GI2_RHS] = k * dCalcVectorDot3( u, q );




    /*
     * Constraint along the axis: translation along it should couple angular movement.
     * This is just the ball-and-socket derivation, projected onto the hinge axis,
     * producing a single constraint at the end.
     *
     * The choice of "ball" position can be arbitrary; we could place it at the center
     * of one of the bodies, canceling out its rotational jacobian; or we could make
     * everything symmetrical by just placing at the midpoint between the centers.
     *
     * I like symmetry, so I'll use the second approach here. I'll call the midpoint h.
     *
     * Of course, if the second body is NULL, the first body is pretty much locked
     * along this axis, and the linear constraint is enough.
     */

    int rowskip_mul_3 = 3 * rowskip;
    dCopyVector3(J1 + rowskip_mul_3 + GI2__JL_MIN, globalAxis1);

    if ( body1 ) {
        dVector3 h;
        dAddScaledVectors3(h, node[0].body->posr.pos, body1->posr.pos, -0.5, 0.5);

        dCalcVectorCross3(J1 + rowskip_mul_3 + GI2__JA_MIN, h, globalAxis1);

        dCopyNegatedVector3(J2 + rowskip_mul_3 + GI2__JL_MIN, globalAxis1);
        dCopyVector3(J2 + rowskip_mul_3 + GI2__JA_MIN, J1 + rowskip_mul_3 + GI2__JA_MIN);
    }

    // error correction: both anchors should lie on the same plane perpendicular to the axis
    dVector3 globalA1, globalA2;
    dBodyGetRelPointPos(node[0].body, anchor1[0], anchor1[1], anchor1[2], globalA1);

    if ( body1 ) {
        dBodyGetRelPointPos(body1, anchor2[0], anchor2[1], anchor2[2], globalA2);
    } else {
        dCopyVector3(globalA2, anchor2);
    }

    dVector3 d;
    dSubtractVectors3(d, globalA1, globalA2); // displacement error
    pairRhsCfm[3 * pairskip + GI2_RHS] = -k * dCalcVectorDot3(globalAxis1, d);
}

void dJointSetDHingeAxis( dJointID j, dReal x, dReal y, dReal z )
{
    dxJointDHinge* joint = static_cast<dxJointDHinge*>(j);
    dUASSERT( joint, "bad joint argument" );

    dBodyVectorFromWorld(joint->node[0].body, x, y, z, joint->axis1);
    if (joint->node[1].body)
        dBodyVectorFromWorld(joint->node[1].body, x, y, z, joint->axis2);
    else {
        joint->axis2[0] = x;
        joint->axis2[1] = y;
        joint->axis2[2] = z;
    }
    dNormalize3(joint->axis1);
    dNormalize3(joint->axis2);
}

void dJointGetDHingeAxis( dJointID j, dVector3 result )
{
    dxJointDHinge* joint = static_cast<dxJointDHinge*>(j);
    dUASSERT( joint, "bad joint argument" );

    dBodyVectorToWorld(joint->node[0].body, joint->axis1[0], joint->axis1[1], joint->axis1[2], result);
}


void dJointSetDHingeAnchor1( dJointID j, dReal x, dReal y, dReal z )
{
    dJointSetDBallAnchor1(j, x, y, z);
}


void dJointSetDHingeAnchor2( dJointID j, dReal x, dReal y, dReal z )
{
    dJointSetDBallAnchor2(j, x, y, z);
}

dReal dJointGetDHingeDistance(dJointID j)
{
    return dJointGetDBallDistance(j);
}


void dJointGetDHingeAnchor1( dJointID j, dVector3 result )
{
    dJointGetDBallAnchor1(j, result);
}


void dJointGetDHingeAnchor2( dJointID j, dVector3 result )
{
    dJointGetDBallAnchor2(j, result);
}


void dJointSetDHingeParam( dJointID j, int parameter, dReal value )
{
    dJointSetDBallParam(j, parameter, value);
}


dReal dJointGetDHingeParam( dJointID j, int parameter )
{
    return dJointGetDBallParam(j, parameter);
}

dJointType
dxJointDHinge::type() const
{
    return dJointTypeDHinge;
}

size_t
dxJointDHinge::size() const
{
    return sizeof( *this );
}
