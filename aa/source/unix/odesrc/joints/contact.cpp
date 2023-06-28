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
#include "contact.h"
#include "joint_internal.h"



//****************************************************************************
// contact

dxJointContact::dxJointContact( dxWorld *w ) :
    dxJoint( w )
{
}


void 
dxJointContact::getSureMaxInfo( SureMaxInfo* info )
{
  // ...as the actual m is very likely to hit the maximum
  info->max_m = (contact.surface.mode&dContactRolling)?6:3; 
}


void
dxJointContact::getInfo1( dxJoint::Info1 *info )
{
    // make sure mu's >= 0, then calculate number of constraint rows and number
    // of unbounded rows.
    int m = 1, nub = 0;
    int roll = (contact.surface.mode&dContactRolling)!=0;
    
    if ( contact.surface.mu < 0 ) contact.surface.mu = 0;

    // Anisotropic sliding and rolling and spinning friction 
    if ( contact.surface.mode & dContactAxisDep )
    {
        if ( contact.surface.mu2 < 0 ) contact.surface.mu2 = 0;
        if ( contact.surface.mu  > 0 ) m++;
        if ( contact.surface.mu2 > 0 ) m++;
        if ( contact.surface.mu  == dInfinity ) nub ++;
        if ( contact.surface.mu2 == dInfinity ) nub ++;
        if (roll) {
          if ( contact.surface.rho < 0 ) contact.surface.rho = 0;
          else m++;
          if ( contact.surface.rho2 < 0 ) contact.surface.rho2 = 0;
          else m++;
          if ( contact.surface.rhoN < 0 ) contact.surface.rhoN = 0;
          else m++;

          if ( contact.surface.rho  == dInfinity ) nub++;
          if ( contact.surface.rho2 == dInfinity ) nub++;
          if ( contact.surface.rhoN == dInfinity ) nub++;
        }
    }
    else
    {
        if ( contact.surface.mu > 0 ) m += 2;
        if ( contact.surface.mu == dInfinity ) nub += 2;
        if (roll) {
          if ( contact.surface.rho < 0 ) contact.surface.rho = 0;
          else m+=3;
          if ( contact.surface.rho == dInfinity ) nub += 3;
        }
    }

    the_m = m;
    info->m = m;
    info->nub = nub;
}


void
dxJointContact::getInfo2( dReal worldFPS, dReal worldERP, 
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
    int *findex)
{
    enum {
        ROW_NORMAL,

        ROW__OPTIONAL_MIN,
    };

    // get normal, with sign adjusted for body1/body2 polarity
    dVector3 normal;
    if ( (flags & dJOINT_REVERSE) != 0 ) {
        dCopyNegatedVector3( normal, contact.geom.normal );
    } else {
        dCopyVector3( normal, contact.geom.normal );
    }

    // c1,c2 = contact points with respect to body PORs
    dVector3 c1, c2 = {0,0,0};

    dxBody *b0 = node[0].body;
    dSubtractVectors3( c1, contact.geom.pos, b0->posr.pos );

    // set Jacobian for normal
    dCopyVector3( J1 + ROW_NORMAL * rowskip + GI2__JL_MIN, normal );
    dCalcVectorCross3( J1 + ROW_NORMAL * rowskip + GI2__JA_MIN, c1, normal );

    dxBody *b1 = node[1].body;
    if ( b1 ) {
        dSubtractVectors3(c2, contact.geom.pos, b1->posr.pos);
        dCopyNegatedVector3(J2 + ROW_NORMAL * rowskip + GI2__JL_MIN, normal);
        dCalcVectorCross3( J2 + ROW_NORMAL * rowskip + GI2__JA_MIN, normal, c2 ); //== dCalcVectorCross3( J2 + GI2__JA_MIN, c2, normal ); dNegateVector3( J2 + GI2__JA_MIN );
    }

    const int surface_mode = contact.surface.mode;
    // set right hand side and cfm value for normal
    dReal erp = (surface_mode & dContactSoftERP) != 0 ? contact.surface.soft_erp : worldERP;
    dReal k = worldFPS * erp;

    dReal depth = contact.geom.depth - world->contactp.min_depth;
    if ( depth < 0 ) depth = 0;

    dReal motionN = (surface_mode & dContactMotionN) != 0 ? contact.surface.motionN : REAL(0.0);
    const dReal pushout = k * depth + motionN;
    // note: this cap should not limit bounce velocity
    const dReal maxvel = world->contactp.max_vel;
    dReal c = pushout > maxvel ? maxvel : pushout;

    // deal with bounce
    if ( (surface_mode & dContactBounce) != 0 ) {
        // calculate outgoing velocity (-ve for incoming contact)
        dReal outgoing = dCalcVectorDot3( J1 + ROW_NORMAL * rowskip + GI2__JL_MIN, node[0].body->lvel )
            + dCalcVectorDot3( J1 + ROW_NORMAL * rowskip + GI2__JA_MIN, node[0].body->avel );
        
        if ( b1 ) {
            outgoing += dCalcVectorDot3( J2 + ROW_NORMAL * rowskip + GI2__JL_MIN, node[1].body->lvel )
                + dCalcVectorDot3( J2 + ROW_NORMAL * rowskip + GI2__JA_MIN, node[1].body->avel );
        }

        outgoing -= motionN;
        // only apply bounce if the outgoing velocity is greater than the
        // threshold, and if the resulting c[rowNormal] exceeds what we already have.
        if ( contact.surface.bounce_vel >= 0 &&
            ( -outgoing ) > contact.surface.bounce_vel ) {
            const dReal newc = - contact.surface.bounce * outgoing + motionN;
            if ( newc > c ) c = newc;
        }
    }

    pairRhsCfm[ROW_NORMAL * pairskip + GI2_RHS] = c;

    if ( (surface_mode & dContactSoftCFM) != 0 ) {
        pairRhsCfm[ROW_NORMAL * pairskip + GI2_CFM] = contact.surface.soft_cfm;
    }

    // set LCP limits for normal
    pairLoHi[ROW_NORMAL * pairskip + GI2_LO] = 0;
    pairLoHi[ROW_NORMAL * pairskip + GI2_HI] = dInfinity;


    if ( the_m > 1 ) { // if no friction, there is nothing else to do
        // now do jacobian for tangential forces
        dVector3 t1, t2; // two vectors tangential to normal

        if ( (surface_mode & dContactFDir1) != 0 ) {   // use fdir1 ?
            dCopyVector3( t1, contact.fdir1 );
            dCalcVectorCross3( t2, normal, t1 );
        } else {
            dPlaneSpace( normal, t1, t2 );
        }

        int row = ROW__OPTIONAL_MIN;
        int currRowSkip = row * rowskip, currPairSkip = row * pairskip;

        // first friction direction
        if ( contact.surface.mu > 0 ) {
            dCopyVector3( J1 + currRowSkip + GI2__JL_MIN, t1 );
            dCalcVectorCross3( J1 + currRowSkip + GI2__JA_MIN, c1, t1 );
            
            if ( node[1].body ) {
                dCopyNegatedVector3( J2 + currRowSkip + GI2__JL_MIN, t1);
                dCalcVectorCross3( J2 + currRowSkip + GI2__JA_MIN, t1, c2 ); //== dCalcVectorCross3( J2 + rowskip + GI2__JA_MIN, c2, t1 ); dNegateVector3( J2 + rowskip + GI2__JA_MIN );
            }

            // set right hand side
            if ( (surface_mode & dContactMotion1) != 0 ) {
                pairRhsCfm[currPairSkip + GI2_RHS] = contact.surface.motion1;
            }
            // set slip (constraint force mixing)
            if ( (surface_mode & dContactSlip1) != 0 ) {
                pairRhsCfm[currPairSkip + GI2_CFM] = contact.surface.slip1;
            }

            // set LCP bounds and friction index. this depends on the approximation
            // mode
            pairLoHi[currPairSkip + GI2_LO] = -contact.surface.mu;
            pairLoHi[currPairSkip + GI2_HI] = contact.surface.mu;

            if ( (surface_mode & dContactApprox1_1) != 0 ) {
                findex[row] = 0;
            }

            ++row;
            currRowSkip += rowskip; currPairSkip += pairskip;
        } 

        const dReal mu2 = (surface_mode & dContactMu2) != 0 ? contact.surface.mu2 : contact.surface.mu;

        // second friction direction
        if ( mu2 > 0 ) {
            dCopyVector3( J1 + currRowSkip + GI2__JL_MIN, t2 );
            dCalcVectorCross3( J1 + currRowSkip + GI2__JA_MIN, c1, t2 );

            if ( node[1].body ) {
                dCopyNegatedVector3( J2 + currRowSkip + GI2__JL_MIN, t2 );
                dCalcVectorCross3( J2 + currRowSkip + GI2__JA_MIN, t2, c2 ); //== dCalcVectorCross3( J2 + currRowSkip + GI2__JA_MIN, c2, t2 ); dNegateVector3( J2 + currRowSkip + GI2__JA_MIN );
            }

            // set right hand side
            if ( (surface_mode & dContactMotion2) != 0 ) {
                pairRhsCfm[currPairSkip + GI2_RHS] = contact.surface.motion2;
            }
            // set slip (constraint force mixing)
            if ( (surface_mode & dContactSlip2) != 0 ) {
                pairRhsCfm[currPairSkip + GI2_CFM] = contact.surface.slip2;
            }

            // set LCP bounds and friction index. this depends on the approximation
            // mode
            pairLoHi[currPairSkip + GI2_LO] = -mu2;
            pairLoHi[currPairSkip + GI2_HI] =  mu2;

            if ( (surface_mode & dContactApprox1_2) != 0 ) {
                findex[row] = 0;
            }

            ++row;
            currRowSkip += rowskip; currPairSkip += pairskip;
        } 

        // Handle rolling/spinning friction
        if ( (surface_mode & dContactRolling) != 0 ) {
            // Get the coefficients
            dReal rho[3];
            rho[0] = contact.surface.rho;
            if ( (surface_mode & dContactAxisDep) != 0 ) {
                rho[1] = contact.surface.rho2;
                rho[2] = contact.surface.rhoN;
            } else {
                rho[1] = rho[0];
                rho[2] = rho[0];
            }

            const dReal* ax[3];
            ax[0] = t1; // Rolling around t1 creates movement parallel to t2
            ax[1] = t2;
            ax[2] = normal; // Spinning axis

            // Should we use proportional force?
            bool approx[3];
            approx[0] = (surface_mode & dContactApprox1_1) != 0;
            approx[1] = (surface_mode & dContactApprox1_2) != 0;
            approx[2] = (surface_mode & dContactApprox1_N) != 0;

            for (int i = 0; i != 3; ++i) {
                if (rho[i] > 0) {
                    // Set the angular axis
                    dCopyVector3(J1 + currRowSkip + GI2__JA_MIN, ax[i]);

                    if ( b1 ) {
                        dCopyNegatedVector3(J2 + currRowSkip + GI2__JA_MIN, ax[i]);
                    }

                    // Set the lcp limits
                    pairLoHi[currPairSkip + GI2_LO] = -rho[i];
                    pairLoHi[currPairSkip + GI2_HI] =  rho[i];
                    
                    // Make limits proportional to normal force
                    if (approx[i]) {
                        findex[row] = 0;
                    }

                    ++row;
                    currRowSkip += rowskip; currPairSkip += pairskip;
                }
            }
        }
    }
}

dJointType
dxJointContact::type() const
{
    return dJointTypeContact;
}


size_t
dxJointContact::size() const
{
    return sizeof( *this );
}

