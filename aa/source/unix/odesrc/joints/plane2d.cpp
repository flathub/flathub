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
#include "plane2d.h"
#include "joint_internal.h"



//****************************************************************************
// Plane2D
/*
This code is part of the Plane2D ODE joint
by psero@gmx.de
Wed Apr 23 18:53:43 CEST 2003
*/


static const dReal   Midentity[3][3] =
{
    {   1,  0,  0   },
    {   0,  1,  0   },
    {   0,  0,  1,  }
};


dxJointPlane2D::dxJointPlane2D( dxWorld *w ) :
    dxJoint( w )
{
    motor_x.init( world );
    motor_y.init( world );
    motor_angle.init( world );
}


void 
dxJointPlane2D::getSureMaxInfo( SureMaxInfo* info )
{
    info->max_m = 6;
}


void
dxJointPlane2D::getInfo1( dxJoint::Info1 *info )
{
    info->nub = 3;
    info->m = 3;

    if ( motor_x.fmax > 0 )
        row_motor_x = info->m++;
    else
        row_motor_x = 0;

    if ( motor_y.fmax > 0 )
        row_motor_y = info->m++;
    else
        row_motor_y = 0;

    if ( motor_angle.fmax > 0 )
        row_motor_angle = info->m++;
    else
        row_motor_angle = 0;
}



void
dxJointPlane2D::getInfo2( dReal worldFPS, dReal worldERP, 
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
    int *findex )
{
    dReal eps = worldFPS * worldERP;

    /*
        v = v1, w = omega1
        (v2, omega2 not important (== static environment))

        constraint equations:
            vz = 0
            wx = 0
            wy = 0

        <=> ( 0 0 1 ) (vx)   ( 0 0 0 ) (wx)   ( 0 )
            ( 0 0 0 ) (vy) + ( 1 0 0 ) (wy) = ( 0 )
            ( 0 0 0 ) (vz)   ( 0 1 0 ) (wz)   ( 0 )
            J1/J1l           Omega1/J1a
    */

    // fill in linear and angular coeff. for left hand side:

    J1[GI2_JLZ] = 1;
    J1[rowskip + GI2_JAX] = 1;
    J1[2 * rowskip + GI2_JAY] = 1;

    // error correction (against drift):

    // a) linear vz, so that z (== pos[2]) == 0
    pairRhsCfm[GI2_RHS] = eps * -node[0].body->posr.pos[2];

# if 0
    // b) angular correction? -> left to application !!!
    dReal       *body_z_axis = &node[0].body->R[8];
    pairRhsCfm[pairskip + GI2_RHS] = eps * + atan2( body_z_axis[1], body_z_axis[2] );  // wx error
    pairRhsCfm[2 * pairskip + GI2_RHS] = eps * -atan2( body_z_axis[0], body_z_axis[2] );  // wy error
# endif

    // if the slider is powered, or has joint limits, add in the extra row:

    if ( row_motor_x > 0 )
    {
        int currRowSkip = row_motor_x * rowskip, currPairSkip = row_motor_x * pairskip;
        motor_x.addLimot( this, worldFPS, J1 + currRowSkip, J2 + currRowSkip, pairRhsCfm + currPairSkip, pairLoHi + currPairSkip, Midentity[0], 0 );
    }

    if ( row_motor_y > 0 )
    {
        int currRowSkip = row_motor_y * rowskip, currPairSkip = row_motor_y * pairskip;
        motor_y.addLimot( this, worldFPS, J1 + currRowSkip, J2 + currRowSkip, pairRhsCfm + currPairSkip, pairLoHi + currPairSkip, Midentity[1], 0 );
    }

    if ( row_motor_angle > 0 )
    {
        int currRowSkip = row_motor_angle * rowskip, currPairSkip = row_motor_angle * pairskip;
        motor_angle.addLimot( this, worldFPS, J1 + currRowSkip, J2 + currRowSkip, pairRhsCfm + currPairSkip, pairLoHi + currPairSkip, Midentity[2], 1 );
    }
}


dJointType
dxJointPlane2D::type() const
{
    return dJointTypePlane2D;
}


size_t
dxJointPlane2D::size() const
{
    return sizeof( *this );
}



void dJointSetPlane2DXParam( dxJoint *joint,
                            int parameter, dReal value )
{
    dUASSERT( joint, "bad joint argument" );
    checktype( joint, Plane2D );
    dxJointPlane2D* joint2d = ( dxJointPlane2D* )( joint );
    joint2d->motor_x.set( parameter, value );
}


void dJointSetPlane2DYParam( dxJoint *joint,
                            int parameter, dReal value )
{
    dUASSERT( joint, "bad joint argument" );
    checktype( joint, Plane2D );
    dxJointPlane2D* joint2d = ( dxJointPlane2D* )( joint );
    joint2d->motor_y.set( parameter, value );
}



void dJointSetPlane2DAngleParam( dxJoint *joint,
                                int parameter, dReal value )
{
    dUASSERT( joint, "bad joint argument" );
    checktype( joint, Plane2D );
    dxJointPlane2D* joint2d = ( dxJointPlane2D* )( joint );
    joint2d->motor_angle.set( parameter, value );
}

