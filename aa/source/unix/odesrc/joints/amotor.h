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

#ifndef _ODE_JOINT_AMOTOR_H_
#define _ODE_JOINT_AMOTOR_H_

#include "joint.h"


// angular motor

typedef dxJoint dxJointAMotor_Parent;
class dxJointAMotor:
    public dxJointAMotor_Parent
{
public:
    dxJointAMotor(dxWorld *w);
    virtual ~dxJointAMotor();

public:
    virtual void getSureMaxInfo(SureMaxInfo* info);
    virtual void getInfo1(Info1* info);
    virtual void getInfo2(dReal worldFPS, dReal worldERP, 
        int rowskip, dReal *J1, dReal *J2,
        int pairskip, dReal *pairRhsCfm, dReal *pairLoHi, 
        int *findex);
    virtual dJointType type() const;
    virtual size_t size() const;

public:
    void setOperationMode(int mode);
    int getOperationMode() const { return m_mode; }

    void setNumAxes(unsigned num);
    int getNumAxes() const { return m_num; }

    dJointBodyRelativity getAxisBodyRelativity(unsigned anum) const;

    void setAxisValue(unsigned anum, dJointBodyRelativity rel, dReal x, dReal y, dReal z);
    void getAxisValue(dVector3 result, unsigned anum) const;

private:
    void doGetUserAxis(dVector3 result, unsigned anum) const;
    void doGetEulerAxis(dVector3 result, unsigned anum) const;

public:
    void setAngleValue(unsigned anum, dReal angle);
    dReal getAngleValue(unsigned anum) const { dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX)); return m_angle[anum]; }

    dReal calculateAngleRate(unsigned anum) const;

    void setLimotParameter(unsigned anum, int limotParam, dReal value) { dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX)); m_limot[anum].set(limotParam, value); }
    dReal getLimotParameter(unsigned anum, int limotParam) const { dAASSERT(dIN_RANGE(anum, dSA__MIN, dSA__MAX)); return m_limot[anum].get(limotParam); }

public:
    void addTorques(dReal torque1, dReal torque2, dReal torque3);

private:
    void computeGlobalAxes(dVector3 ax[dSA__MAX]) const;
    void doComputeGlobalUserAxes(dVector3 ax[dSA__MAX]) const;
    void doComputeGlobalEulerAxes(dVector3 ax[dSA__MAX]) const;

    void computeEulerAngles(dVector3 ax[dSA__MAX]);
    void setEulerReferenceVectors();

private:
    inline dSpaceAxis BuildFirstBodyEulerAxis() const;
    inline dJointConnectedBody BuildFirstEulerAxisBody() const;

private:
    friend struct dxAMotorJointPrinter;

private:
    int m_mode;                                   // a dAMotorXXX constant
    unsigned m_num;                               // number of axes (0..3)
    dJointBodyRelativity m_rel[dSA__MAX];         // what the axes are relative to (global,b1,b2)
    dVector3 m_axis[dSA__MAX];                    // three axes
    // these vectors are used for calculating Euler angles
    dVector3 m_references[dJCB__MAX];             // original axis[2], relative to body 1; original axis[0], relative to body 2
    dReal m_angle[dSA__MAX];                      // user-supplied angles for axes
    dxJointLimitMotor m_limot[dJBR__MAX];         // limit+motor info for axes
};


#endif

