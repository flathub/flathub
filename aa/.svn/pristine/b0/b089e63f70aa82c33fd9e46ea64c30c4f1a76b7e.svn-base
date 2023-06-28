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
#include "slider.h"
#include "joint_internal.h"



//****************************************************************************
// slider

dxJointSlider::dxJointSlider ( dxWorld *w ) :
    dxJoint ( w )
{
    dSetZero ( axis1, 4 );
    axis1[0] = 1;
    dSetZero ( qrel, 4 );
    dSetZero ( offset, 4 );
    limot.init ( world );
}


dReal dJointGetSliderPosition ( dJointID j )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );

    // get axis1 in global coordinates
    dVector3 ax1, q;
    dMultiply0_331 ( ax1, joint->node[0].body->posr.R, joint->axis1 );

    if ( joint->node[1].body )
    {
        // get body2 + offset point in global coordinates
        dMultiply0_331 ( q, joint->node[1].body->posr.R, joint->offset );
        for ( int i = 0; i < 3; i++ )
            q[i] = joint->node[0].body->posr.pos[i]
                - q[i]
                - joint->node[1].body->posr.pos[i];
    }
    else
    {
        q[0] = joint->node[0].body->posr.pos[0] - joint->offset[0];
        q[1] = joint->node[0].body->posr.pos[1] - joint->offset[1];
        q[2] = joint->node[0].body->posr.pos[2] - joint->offset[2];

        if ( joint->flags & dJOINT_REVERSE )
        {
            // N.B. it could have been simplier to only inverse the sign of
            //      the dCalcVectorDot3 result but this case is exceptional and doing
            //      the check for all case can decrease the performance.
            ax1[0] = -ax1[0];
            ax1[1] = -ax1[1];
            ax1[2] = -ax1[2];
        }
    }

    return dCalcVectorDot3 ( ax1, q );
}


dReal dJointGetSliderPositionRate ( dJointID j )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );

    // get axis1 in global coordinates
    dVector3 ax1;
    dMultiply0_331 ( ax1, joint->node[0].body->posr.R, joint->axis1 );

    if ( joint->node[1].body )
    {
        return dCalcVectorDot3 ( ax1, joint->node[0].body->lvel ) -
            dCalcVectorDot3 ( ax1, joint->node[1].body->lvel );
    }
    else
    {
        dReal rate = dCalcVectorDot3 ( ax1, joint->node[0].body->lvel );
        if ( joint->flags & dJOINT_REVERSE ) rate = - rate;
        return rate;
    }
}


void 
dxJointSlider::getSureMaxInfo( SureMaxInfo* info )
{
    info->max_m = 6;
}


void
dxJointSlider::getInfo1 ( dxJoint::Info1 *info )
{
    info->nub = 5;

    // see if joint is powered
    if ( limot.fmax > 0 )
        info->m = 6; // powered slider needs an extra constraint row
    else info->m = 5;

    // see if we're at a joint limit.
    limot.limit = 0;
    if ( ( limot.lostop > -dInfinity || limot.histop < dInfinity ) &&
        limot.lostop <= limot.histop )
    {
        // measure joint position
        dReal pos = dJointGetSliderPosition ( this );
        if ( pos <= limot.lostop )
        {
            limot.limit = 1;
            limot.limit_err = pos - limot.lostop;
            info->m = 6;
        }
        else if ( pos >= limot.histop )
        {
            limot.limit = 2;
            limot.limit_err = pos - limot.histop;
            info->m = 6;
        }
    }
}


void
dxJointSlider::getInfo2 ( dReal worldFPS, dReal worldERP, 
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
    int *findex )
{
    // 3 rows to make body rotations equal
    setFixedOrientation ( this, worldFPS, worldERP, rowskip, J1, J2, pairskip, pairRhsCfm, qrel );

    // pull out pos and R for both bodies. also get the `connection'
    // vector pos2-pos1.
    dVector3 c;
    dReal *pos2 = NULL, *R2 = NULL;

    dReal *pos1 = node[0].body->posr.pos;
    dReal *R1 = node[0].body->posr.R;

    dVector3 ax1; // joint axis in global coordinates (unit length)
    dVector3 p, q; // plane space of ax1
    dMultiply0_331 ( ax1, R1, axis1 );
    dPlaneSpace ( ax1, p, q );

    dxBody *body1 = node[1].body;
    
    if ( body1 )
    {
        R2 = body1->posr.R;
        pos2 = body1->posr.pos;
        dSubtractVectors3( c, pos2, pos1 );
    }

    // remaining two rows. we want: vel2 = vel1 + w1 x c ... but this would
    // result in three equations, so we project along the planespace vectors
    // so that sliding along the slider axis is disregarded. for symmetry we
    // also substitute (w1+w2)/2 for w1, as w1 is supposed to equal w2.
    int currRowSkip = 3 * rowskip, currPairSkip = 3 * pairskip;
    {
        dCopyVector3( J1 + currRowSkip + GI2__JL_MIN, p );

        if ( body1 )
        {
            dVector3 tmp;

            dCopyNegatedVector3(J2 + currRowSkip + GI2__JL_MIN, p);

            dCalcVectorCross3( tmp, c, p );
            dCopyScaledVector3( J1 + currRowSkip + GI2__JA_MIN, tmp, REAL(0.5) );
            dCopyVector3( J2 + currRowSkip + GI2__JA_MIN, J1 + currRowSkip + GI2__JA_MIN );
        }
    }

    currRowSkip += rowskip;
    {
        dCopyVector3( J1 + currRowSkip + GI2__JL_MIN, q );

        if ( body1 )
        {
            dVector3 tmp;

            dCopyNegatedVector3(J2 + currRowSkip + GI2__JL_MIN, q);

            dCalcVectorCross3( tmp, c, q );
            dCopyScaledVector3( J1 + currRowSkip + GI2__JA_MIN, tmp, REAL(0.5) );
            dCopyVector3( J2 + currRowSkip + GI2__JA_MIN, J1 + currRowSkip + GI2__JA_MIN );
        }
    }

    // compute last two elements of right hand side. we want to align the offset
    // point (in body 2's frame) with the center of body 1.
    dReal k = worldFPS * worldERP;

    if ( body1 )
    {
        dVector3 ofs;  // offset point in global coordinates
        dMultiply0_331 ( ofs, R2, offset );
        dAddVectors3(c, c, ofs);
        
        pairRhsCfm[currPairSkip + GI2_RHS] = k * dCalcVectorDot3 ( p, c );

        currPairSkip += pairskip;
        pairRhsCfm[currPairSkip + GI2_RHS] = k * dCalcVectorDot3 ( q, c );
    }
    else
    {
        dVector3 ofs;  // offset point in global coordinates
        dSubtractVectors3(ofs, offset, pos1);
        
        pairRhsCfm[currPairSkip + GI2_RHS] = k * dCalcVectorDot3 ( p, ofs );
        
        currPairSkip += pairskip;
        pairRhsCfm[currPairSkip + GI2_RHS] = k * dCalcVectorDot3 ( q, ofs );

        if ( (flags & dJOINT_REVERSE) != 0 )
        {
            dNegateVector3(ax1);
        }
    }

    // if the slider is powered, or has joint limits, add in the extra row
    currRowSkip += rowskip; currPairSkip += pairskip;
    limot.addLimot ( this, worldFPS, J1 + currRowSkip, J2 + currRowSkip, pairRhsCfm + currPairSkip, pairLoHi + currPairSkip, ax1, 0 );
}


void dJointSetSliderAxis ( dJointID j, dReal x, dReal y, dReal z )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );
    setAxes ( joint, x, y, z, joint->axis1, 0 );

    joint->computeOffset();

    joint->computeInitialRelativeRotation();
}


void dJointSetSliderAxisDelta ( dJointID j, dReal x, dReal y, dReal z, dReal dx, dReal dy, dReal dz )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );
    setAxes ( joint, x, y, z, joint->axis1, 0 );

    joint->computeOffset();

    // compute initial relative rotation body1 -> body2, or env -> body1
    // also compute center of body1 w.r.t body 2
    if ( !(joint->node[1].body) )
    {
        joint->offset[0] += dx;
        joint->offset[1] += dy;
        joint->offset[2] += dz;
    }

    joint->computeInitialRelativeRotation();
}



void dJointGetSliderAxis ( dJointID j, dVector3 result )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    dUASSERT ( result, "bad result argument" );
    checktype ( joint, Slider );
    getAxis ( joint, result, joint->axis1 );
}


void dJointSetSliderParam ( dJointID j, int parameter, dReal value )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );
    joint->limot.set ( parameter, value );
}


dReal dJointGetSliderParam ( dJointID j, int parameter )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );
    return joint->limot.get ( parameter );
}


void dJointAddSliderForce ( dJointID j, dReal force )
{
    dxJointSlider* joint = ( dxJointSlider* ) j;
    dVector3 axis;
    dUASSERT ( joint, "bad joint argument" );
    checktype ( joint, Slider );

    if ( joint->flags & dJOINT_REVERSE )
        force = -force;

    getAxis ( joint, axis, joint->axis1 );
    axis[0] *= force;
    axis[1] *= force;
    axis[2] *= force;

    if ( joint->node[0].body != 0 )
        dBodyAddForce ( joint->node[0].body, axis[0], axis[1], axis[2] );
    if ( joint->node[1].body != 0 )
        dBodyAddForce ( joint->node[1].body, -axis[0], -axis[1], -axis[2] );

    if ( joint->node[0].body != 0 && joint->node[1].body != 0 )
    {
        // linear torque decoupling:
        // we have to compensate the torque, that this slider force may generate
        // if body centers are not aligned along the slider axis

        dVector3 ltd; // Linear Torque Decoupling vector (a torque)

        dVector3 c;
        c[0] = REAL ( 0.5 ) * ( joint->node[1].body->posr.pos[0] - joint->node[0].body->posr.pos[0] );
        c[1] = REAL ( 0.5 ) * ( joint->node[1].body->posr.pos[1] - joint->node[0].body->posr.pos[1] );
        c[2] = REAL ( 0.5 ) * ( joint->node[1].body->posr.pos[2] - joint->node[0].body->posr.pos[2] );
        dCalcVectorCross3( ltd, c, axis );

        dBodyAddTorque ( joint->node[0].body, ltd[0], ltd[1], ltd[2] );
        dBodyAddTorque ( joint->node[1].body, ltd[0], ltd[1], ltd[2] );
    }
}


dJointType
dxJointSlider::type() const
{
    return dJointTypeSlider;
}


size_t
dxJointSlider::size() const
{
    return sizeof ( *this );
}


void
dxJointSlider::setRelativeValues()
{
    computeOffset();
    computeInitialRelativeRotation();
}



/// Compute initial relative rotation body1 -> body2, or env -> body1
void
dxJointSlider::computeInitialRelativeRotation()
{
    if ( node[0].body )
    {
        // compute initial relative rotation body1 -> body2, or env -> body1
        // also compute center of body1 w.r.t body 2
        if ( node[1].body )
        {
            dQMultiply1 ( qrel, node[0].body->q, node[1].body->q );
        }
        else
        {
            // set qrel to the transpose of the first body's q
            qrel[0] =  node[0].body->q[0];
            qrel[1] = -node[0].body->q[1];
            qrel[2] = -node[0].body->q[2];
            qrel[3] = -node[0].body->q[3];
        }
    }
}


/// Compute center of body1 w.r.t body 2
void
dxJointSlider::computeOffset()
{
    if ( node[1].body )
    {
        dVector3 c;
        c[0] = node[0].body->posr.pos[0] - node[1].body->posr.pos[0];
        c[1] = node[0].body->posr.pos[1] - node[1].body->posr.pos[1];
        c[2] = node[0].body->posr.pos[2] - node[1].body->posr.pos[2];

        dMultiply1_331 ( offset, node[1].body->posr.R, c );
    }
    else if ( node[0].body )
    {
        offset[0] = node[0].body->posr.pos[0];
        offset[1] = node[0].body->posr.pos[1];
        offset[2] = node[0].body->posr.pos[2];
    }
}
