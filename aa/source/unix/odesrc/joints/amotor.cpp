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
#include "common.h"
#include "amotor.h"
#include "joint_internal.h"
#include "odeou.h"


/*extern */
void dJointSetAMotorNumAxes(dJointID j, int num)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(num, dSA__MIN, dSA__MAX + 1));
    checktype(joint, AMotor);

    num = dCLAMP(num, dSA__MIN, dSA__MAX);

    joint->setNumAxes(num);
}

/*extern */
void dJointSetAMotorAxis(dJointID j, int anum, int rel/*=dJointBodyRelativity*/, 
    dReal x, dReal y, dReal z)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    dAASSERT(dIN_RANGE(rel, dJBR__MIN, dJBR__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    joint->setAxisValue(anum, (dJointBodyRelativity)rel, x, y, z);
}

/*extern */
void dJointSetAMotorAngle(dJointID j, int anum, dReal angle)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    joint->setAngleValue(anum, angle);
}

/*extern */
void dJointSetAMotorParam(dJointID j, int parameter, dReal value)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    int anum = parameter >> 8;
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    int limotParam = parameter & 0xff;
    joint->setLimotParameter(anum, limotParam, value);
}

/*extern */
void dJointSetAMotorMode(dJointID j, int mode)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    joint->setOperationMode(mode);
}

/*extern */
int dJointGetAMotorNumAxes(dJointID j)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    return joint->getNumAxes();
}

/*extern */
void dJointGetAMotorAxis(dJointID j, int anum, dVector3 result)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    joint->getAxisValue(result, anum);
}

/*extern */
int dJointGetAMotorAxisRel(dJointID j, int anum)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    int result = joint->getAxisBodyRelativity(anum);
    return result;
}

/*extern */
dReal dJointGetAMotorAngle(dJointID j, int anum)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    dReal result = joint->getAngleValue(anum);
    return result;
}

/*extern */
dReal dJointGetAMotorAngleRate(dJointID j, int anum)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    checktype(joint, AMotor);

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    dReal result = joint->calculateAngleRate(anum);
    return result;
}

/*extern */
dReal dJointGetAMotorParam(dJointID j, int parameter)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    int anum = parameter >> 8;
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));

    anum = dCLAMP(anum, dSA__MIN, dSA__MAX - 1);

    int limotParam = parameter & 0xff;
    dReal result = joint->getLimotParameter(anum, limotParam);
    return result;
}

/*extern */
int dJointGetAMotorMode(dJointID j)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    int result = joint->getOperationMode();
    return result;
}

/*extern */
void dJointAddAMotorTorques(dJointID j, dReal torque1, dReal torque2, dReal torque3)
{
    dxJointAMotor* joint = (dxJointAMotor*)j;
    dAASSERT(joint != NULL);
    checktype(joint, AMotor);

    joint->addTorques(torque1, torque2, torque3);
}


//****************************************************************************

BEGIN_NAMESPACE_OU();
template<>
const dJointBodyRelativity CEnumUnsortedElementArray<dSpaceAxis, dSA__MAX, dJointBodyRelativity, 0x160703D5>::m_aetElementArray[] =
{
    dJBR_BODY1, // dSA_X,
    dJBR_GLOBAL, // dSA_Y,
    dJBR_BODY2, // dSA_Z,
};
END_NAMESPACE_OU();
static const CEnumUnsortedElementArray<dSpaceAxis, dSA__MAX, dJointBodyRelativity, 0x160703D5> g_abrEulerAxisAllowedBodyRelativities;

static inline 
dSpaceAxis EncodeJointConnectedBodyEulerAxis(dJointConnectedBody cbBodyIndex)
{
    dSASSERT(dJCB__MAX == 2); 
    
    return cbBodyIndex == dJCB_FIRST_BODY ? dSA_X : dSA_Z;
}

static inline 
dSpaceAxis EncodeOtherEulerAxis(dSpaceAxis saOneAxis)
{
    dIASSERT(saOneAxis == EncodeJointConnectedBodyEulerAxis(dJCB_FIRST_BODY) || saOneAxis == EncodeJointConnectedBodyEulerAxis(dJCB_SECOND_BODY)); 
    dSASSERT(dJCB__MAX == 2); 
    
    return (dSpaceAxis)(dSA_X + dSA_Z - saOneAxis);
}


//****************************************************************************
// angular motor

dxJointAMotor::dxJointAMotor(dxWorld *w) :
    dxJointAMotor_Parent(w),
    m_mode(dAMotorUser),
    m_num(0)
{
    std::fill(m_rel, m_rel + dARRAY_SIZE(m_rel), dJBR__DEFAULT);
    { for (int i = 0; i != dARRAY_SIZE(m_axis); ++i) { dZeroVector3(m_axis[i]); } }
    { for (int i = 0; i != dARRAY_SIZE(m_references); ++i) { dZeroVector3(m_references[i]); } }
    std::fill(m_angle, m_angle + dARRAY_SIZE(m_angle), REAL(0.0));
    { for (int i = 0; i != dARRAY_SIZE(m_limot); ++i) { m_limot[i].init(w); } }
}


/*virtual */
dxJointAMotor::~dxJointAMotor()
{
    // The virtual destructor
}


/*virtual */
void dxJointAMotor::getSureMaxInfo(SureMaxInfo* info)
{
    info->max_m = m_num;
}

/*virtual */
void dxJointAMotor::getInfo1(dxJoint::Info1 *info)
{
    info->m = 0;
    info->nub = 0;

    // compute the axes and angles, if in Euler mode
    if (m_mode == dAMotorEuler)
    {
        dVector3 ax[dSA__MAX];
        computeGlobalAxes(ax);
        computeEulerAngles(ax);
    }

    // see if we're powered or at a joint limit for each axis
    const unsigned num = m_num;
    for (unsigned i = 0; i != num; ++i)
    {
        if (m_limot[i].testRotationalLimit(m_angle[i]) 
            || m_limot[i].fmax > 0)
        {
            info->m++;
        }
    }
}

/*virtual */
void dxJointAMotor::getInfo2(dReal worldFPS, dReal /*worldERP*/, 
    int rowskip, dReal *J1, dReal *J2,
    int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
    int *findex)
{
    // compute the axes (if not global)
    dVector3 ax[dSA__MAX];
    computeGlobalAxes(ax);

    // in Euler angle mode we do not actually constrain the angular velocity
    // along the axes axis[0] and axis[2] (although we do use axis[1]) :
    //
    //    to get   constrain w2-w1 along  ...not
    //    ------   ---------------------  ------
    //    d(angle[0])/dt = 0 ax[1] x ax[2]   ax[0]
    //    d(angle[1])/dt = 0 ax[1]
    //    d(angle[2])/dt = 0 ax[0] x ax[1]   ax[2]
    //
    // constraining w2-w1 along an axis 'a' means that a'*(w2-w1)=0.
    // to prove the result for angle[0], write the expression for angle[0] from
    // GetInfo1 then take the derivative. to prove this for angle[2] it is
    // easier to take the Euler rate expression for d(angle[2])/dt with respect
    // to the components of w and set that to 0.

    dVector3 *axptr[dSA__MAX];
    for (int j = dSA__MIN; j != dSA__MAX; ++j) { axptr[j] = &ax[j]; }

    dVector3 ax0_cross_ax1;
    dVector3 ax1_cross_ax2;
    
    if (m_mode == dAMotorEuler) 
    {
        dCalcVectorCross3(ax0_cross_ax1, ax[dSA_X], ax[dSA_Y]);
        axptr[dSA_Z] = &ax0_cross_ax1;
        dCalcVectorCross3(ax1_cross_ax2, ax[dSA_Y], ax[dSA_Z]);
        axptr[dSA_X] = &ax1_cross_ax2;
    }

    size_t rowTotalSkip = 0, pairTotalSkip = 0;
    
    const unsigned num = m_num;
    for (unsigned i = 0; i != num; ++i) 
    {
        if (m_limot[i].addLimot(this, worldFPS, J1 + rowTotalSkip, J2 + rowTotalSkip, pairRhsCfm + pairTotalSkip, pairLoHi + pairTotalSkip, *(axptr[i]), 1)) 
        {
            rowTotalSkip += rowskip;
            pairTotalSkip += pairskip;
        }
    }
}

/*virtual */
dJointType dxJointAMotor::type() const
{
    return dJointTypeAMotor;
}

/*virtual */
size_t dxJointAMotor::size() const
{
    return sizeof(*this);
}


void dxJointAMotor::setOperationMode(int mode)
{
    m_mode = mode;

    if (mode == dAMotorEuler)
    {
        m_num = dSA__MAX;
        setEulerReferenceVectors();
    }
}


void dxJointAMotor::setNumAxes(unsigned num)
{
    if (m_mode == dAMotorEuler)
    {
        m_num = dSA__MAX;
    }
    else
    {
        m_num = num;
    }
}


dJointBodyRelativity dxJointAMotor::getAxisBodyRelativity(unsigned anum) const
{
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));

    dJointBodyRelativity rel = m_rel[anum];
    if (dJBREncodeBodyRelativityStatus(rel) && GetIsJointReverse())
    {
        rel = dJBRSwapBodyRelativity(rel); // turns 1 into 2, 2 into 1
    }

    return rel;
}


void dxJointAMotor::setAxisValue(unsigned anum, dJointBodyRelativity rel, 
    dReal x, dReal y, dReal z)
{
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    dAASSERT(m_mode != dAMotorEuler || !dJBREncodeBodyRelativityStatus(rel) || rel == g_abrEulerAxisAllowedBodyRelativities.Encode((dSpaceAxis)anum));

    // x,y,z is always in global coordinates regardless of rel, so we may have
    // to convert it to be relative to a body
    dVector3 r;
    dAssignVector3(r, x, y, z);

    // adjust rel to match the internal body order
    if (dJBREncodeBodyRelativityStatus(rel) && GetIsJointReverse())
    {
        rel = dJBRSwapBodyRelativity(rel); // turns 1 into 2, 2, into 1
    }

    m_rel[anum] = rel;

    bool assigned = false;

    if (dJBREncodeBodyRelativityStatus(rel))
    {
        if (rel == dJBR_BODY1)
        {
            dMultiply1_331(m_axis[anum], this->node[0].body->posr.R, r);
            assigned = true;
        }
        // rel == 2
        else if (this->node[1].body != NULL)
        {
            dIASSERT(rel == dJBR_BODY2);

            dMultiply1_331(m_axis[anum], this->node[1].body->posr.R, r);
            assigned = true;
        }
    }
    
    if (!assigned)
    {
        dCopyVector3(m_axis[anum], r); 
    }
    
    dNormalize3(m_axis[anum]);
    
    if (m_mode == dAMotorEuler) 
    {
        setEulerReferenceVectors();
    }
}

void dxJointAMotor::getAxisValue(dVector3 result, unsigned anum) const
{
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));

    switch (m_mode)
    {
        case dAMotorUser:
        {
            doGetUserAxis(result, anum);
            break;
        }

        case dAMotorEuler:
        {
            doGetEulerAxis(result, anum);
            break;
        }

        default:
        {
            dIASSERT(false);
            break;
        }
    } 
}


void dxJointAMotor::doGetUserAxis(dVector3 result, unsigned anum) const
{
    bool retrieved = false;

    if (dJBREncodeBodyRelativityStatus(m_rel[anum])) 
    {
        if (m_rel[anum] == dJBR_BODY1)
        {
            dMultiply0_331(result, this->node[0].body->posr.R, m_axis[anum]);
            retrieved = true;
        }
        else if (this->node[1].body != NULL)
        {
            dMultiply0_331(result, this->node[1].body->posr.R, m_axis[anum]);
            retrieved = true;
        }
    }

    if (!retrieved)
    {
        dCopyVector3(result, m_axis[anum]);
    }
}

void dxJointAMotor::doGetEulerAxis(dVector3 result, unsigned anum) const
{
    // If we're in Euler mode, joint->axis[1] doesn't
    // have anything sensible in it.  So don't just return
    // that, find the actual effective axis.
    // Likewise, the actual axis of rotation for the
    // the other axes is different from what's stored.
    dVector3 axes[dSA__MAX];
    computeGlobalAxes(axes);

    if (anum == dSA_Y) 
    {
        dCopyVector3(result, axes[dSA_Y]);
    } 
    else if (anum < dSA_Y) // Comparing against the same constant lets compiler reuse EFLAGS register for another conditional jump
    {
        dSASSERT(dSA_X < dSA_Y); // Otherwise the condition above is incorrect
        dIASSERT(anum == dSA_X);

        // This won't be unit length in general,
        // but it's what's used in getInfo2
        // This may be why things freak out as
        // the body-relative axes get close to each other.
        dCalcVectorCross3(result, axes[dSA_Y], axes[dSA_Z]);
    } 
    else 
    {
        dSASSERT(dSA_Z > dSA_Y); // Otherwise the condition above is incorrect
        dIASSERT(anum == dSA_Z);

        // Same problem as above.
        dCalcVectorCross3(result, axes[dSA_X], axes[dSA_Y]);
    }
}


void dxJointAMotor::setAngleValue(unsigned anum, dReal angle)
{
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    dAASSERT(m_mode == dAMotorUser); // This only works for the dAMotorUser

    if (m_mode == dAMotorUser)
    {
        m_angle[anum] = angle;
    }
}


dReal dxJointAMotor::calculateAngleRate(unsigned anum) const
{
    dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX));
    dAASSERT(this->node[0].body != NULL); // Don't call for angle rate before the joint is set up

    dVector3 axis;
    getAxisValue(axis, anum);

    // NOTE!
    // For reverse joints, the rate is negated at the function exit to create swapped bodies effect
    dReal rate = dDOT(axis, this->node[0].body->avel);

    if (this->node[1].body != NULL) 
    {
        rate -= dDOT(axis, this->node[1].body->avel);
    }

    // Negating the rate for reverse joints creates an effect of body swapping
    dReal result = !GetIsJointReverse() ? rate : -rate;
    return result;
}


void dxJointAMotor::addTorques(dReal torque1, dReal torque2, dReal torque3)
{
    unsigned num = getNumAxes();
    dAASSERT(dIN_RANGE(num, dSA__MIN, dSA__MAX + 1));

    dVector3 sum;
    dVector3 torqueVector;
    dVector3 axes[dSA__MAX];


    if (num != dSA__MIN)
    {
        computeGlobalAxes(axes);

        if (!GetIsJointReverse())
        {
            dAssignVector3(torqueVector, torque1, torque2, torque3);
        }
        else
        {
            // Negating torques creates an effect of swapped bodies later
            dAssignVector3(torqueVector, -torque1, -torque2, -torque3);
        }
    }

    switch (num)
    {
        case dSA_Z + 1:
        {
            dAddThreeScaledVectors3(sum, axes[dSA_Z], axes[dSA_Y], axes[dSA_X], torqueVector[dSA_Z], torqueVector[dSA_Y], torqueVector[dSA_X]);
            break;
        }

        case dSA_Y + 1:
        {
            dAddScaledVectors3(sum, axes[dSA_Y], axes[dSA_X], torqueVector[dSA_Y], torqueVector[dSA_X]);
            break;
        }

        case dSA_X + 1:
        {
            dCopyScaledVector3(sum, axes[dSA_X], torqueVector[dSA_X]);
            break;
        }
        
        default:
        {
            dSASSERT(dSA_Z > dSA_Y); // Otherwise the addends order needs to be switched
            dSASSERT(dSA_Y > dSA_X);
            
            // Do nothing
            break;
        }
    }

    if (num != dSA__MIN)
    {
        dAASSERT(this->node[0].body != NULL); // Don't add torques unless you set the joint up first!

        // NOTE!
        // For reverse joints, the torqueVector negated at function entry produces the effect of swapped bodies
        dBodyAddTorque(this->node[0].body, sum[dV3E_X], sum[dV3E_Y], sum[dV3E_Z]);
        
        if (this->node[1].body != NULL)
        {
            dBodyAddTorque(this->node[1].body, -sum[dV3E_X], -sum[dV3E_Y], -sum[dV3E_Z]);
        }
    }
}


// compute the 3 axes in global coordinates
void dxJointAMotor::computeGlobalAxes(dVector3 ax[dSA__MAX]) const
{
    switch (m_mode)
    {
        case dAMotorUser:
        {
            doComputeGlobalUserAxes(ax);
            break;
        }

        case dAMotorEuler:
        {
            doComputeGlobalEulerAxes(ax);
            break;
        }

        default:
        {
            dIASSERT(false);
            break;
        }
    } 
}

void dxJointAMotor::doComputeGlobalUserAxes(dVector3 ax[dSA__MAX]) const
{
    unsigned num = m_num;
    for (unsigned i = 0; i != num; ++i)
    {
        bool assigned = false;

        if (m_rel[i] == dJBR_BODY1)
        {
            // relative to b1
            dMultiply0_331(ax[i], this->node[0].body->posr.R, m_axis[i]);
            assigned = true;
        }
        else if (m_rel[i] == dJBR_BODY2)
        {
            // relative to b2
            if (this->node[1].body != NULL)
            {
                dMultiply0_331(ax[i], this->node[1].body->posr.R, m_axis[i]);
                assigned = true;
            }
        }

        if (!assigned)
        {
            // global - just copy it
            dCopyVector3(ax[i], m_axis[i]);
        }
    }
}

void dxJointAMotor::doComputeGlobalEulerAxes(dVector3 ax[dSA__MAX]) const
{
    // special handling for Euler mode
    
    dSpaceAxis firstBodyAxis = BuildFirstBodyEulerAxis();
    dMultiply0_331(ax[firstBodyAxis], this->node[0].body->posr.R, m_axis[firstBodyAxis]);

    dSpaceAxis secondBodyAxis = EncodeOtherEulerAxis(firstBodyAxis);

    if (this->node[1].body != NULL)
    {
        dMultiply0_331(ax[secondBodyAxis], this->node[1].body->posr.R, m_axis[secondBodyAxis]);
    }
    else
    {
        dCopyVector3(ax[secondBodyAxis], m_axis[secondBodyAxis]);
    }

    dCalcVectorCross3(ax[dSA_Y], ax[dSA_Z], ax[dSA_X]);
    dNormalize3(ax[dSA_Y]);
}


void dxJointAMotor::computeEulerAngles(dVector3 ax[dSA__MAX])
{
    // assumptions:
    //   global axes already calculated --> ax
    //   axis[0] is relative to body 1 --> global ax[0]
    //   axis[2] is relative to body 2 --> global ax[2]
    //   ax[1] = ax[2] x ax[0]
    //   original ax[0] and ax[2] are perpendicular
    //   reference1 is perpendicular to ax[0] (in body 1 frame)
    //   reference2 is perpendicular to ax[2] (in body 2 frame)
    //   all ax[] and reference vectors are unit length

    // calculate references in global frame
    dVector3 refs[dJCB__MAX];
    dMultiply0_331(refs[dJCB_FIRST_BODY], this->node[0].body->posr.R, m_references[dJCB_FIRST_BODY]);

    if (this->node[1].body != NULL)
    {
        dMultiply0_331(refs[dJCB_SECOND_BODY], this->node[1].body->posr.R, m_references[dJCB_SECOND_BODY]);
    }
    else
    {
        dCopyVector3(refs[dJCB_SECOND_BODY], m_references[dJCB_SECOND_BODY]);
    }


    // get q perpendicular to both ax[0] and ref1, get first euler angle
    dVector3 q;
    dJointConnectedBody firstAxisBody = BuildFirstEulerAxisBody();

    dCalcVectorCross3(q, ax[dSA_X], refs[firstAxisBody]);
    m_angle[dSA_X] = -dAtan2(dCalcVectorDot3(ax[dSA_Z], q), dCalcVectorDot3(ax[dSA_Z], refs[firstAxisBody]));

    // get q perpendicular to both ax[0] and ax[1], get second euler angle
    dCalcVectorCross3(q, ax[dSA_X], ax[dSA_Y]);
    m_angle[dSA_Y] = -dAtan2(dCalcVectorDot3(ax[dSA_Z], ax[dSA_X]), dCalcVectorDot3(ax[dSA_Z], q));

    dJointConnectedBody secondAxisBody = EncodeJointOtherConnectedBody(firstAxisBody);

    // get q perpendicular to both ax[1] and ax[2], get third euler angle
    dCalcVectorCross3(q, ax[dSA_Y], ax[dSA_Z]);
    m_angle[dSA_Z] = -dAtan2(dCalcVectorDot3(refs[secondAxisBody], ax[dSA_Y]), dCalcVectorDot3(refs[secondAxisBody], q));
}


// set the reference vectors as follows:
//   * reference1 = current axis[2] relative to body 1
//   * reference2 = current axis[0] relative to body 2
// this assumes that:
//    * axis[0] is relative to body 1
//    * axis[2] is relative to body 2

void dxJointAMotor::setEulerReferenceVectors()
{
    if (/*this->node[0].body != NULL && */this->node[1].body != NULL)
    {
        dIASSERT(this->node[0].body != NULL);

        dVector3 r;  // axis[2] and axis[0] in global coordinates

        dSpaceAxis firstBodyAxis = BuildFirstBodyEulerAxis();
        dMultiply0_331(r, this->node[0].body->posr.R, m_axis[firstBodyAxis]);
        dMultiply1_331(m_references[dJCB_SECOND_BODY], this->node[1].body->posr.R, r);

        dSpaceAxis secondBodyAxis = EncodeOtherEulerAxis(firstBodyAxis);
        dMultiply0_331(r, this->node[1].body->posr.R, m_axis[secondBodyAxis]);
        dMultiply1_331(m_references[dJCB_FIRST_BODY], this->node[0].body->posr.R, r);
    } 
    else 
    {
        // We want to handle angular motors attached to passive geoms
        // Replace missing node.R with identity
        if (this->node[0].body != NULL) 
        {
            dSpaceAxis firstBodyAxis = BuildFirstBodyEulerAxis();
            dMultiply0_331(m_references[dJCB_SECOND_BODY], this->node[0].body->posr.R, m_axis[firstBodyAxis]);

            dSpaceAxis secondBodyAxis = EncodeOtherEulerAxis(firstBodyAxis);
            dMultiply1_331(m_references[dJCB_FIRST_BODY], this->node[0].body->posr.R, m_axis[secondBodyAxis]);
        } 
    }
}

/*inline */
dSpaceAxis dxJointAMotor::BuildFirstBodyEulerAxis() const
{
    return EncodeJointConnectedBodyEulerAxis(BuildFirstEulerAxisBody());
}

/*inline */
dJointConnectedBody dxJointAMotor::BuildFirstEulerAxisBody() const
{
    return !GetIsJointReverse() ? dJCB_FIRST_BODY : dJCB_SECOND_BODY;
}

