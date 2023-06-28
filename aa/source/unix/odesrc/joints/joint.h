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

#ifndef _ODE_JOINT_H_
#define _ODE_JOINT_H_


#include <ode/contact.h>
#include "../common.h"
#include "../objects.h"
#include "../obstack.h"


// joint flags
enum
{
    // if this flag is set, the joint was allocated in a joint group
    dJOINT_INGROUP = 1,

    // if this flag is set, the joint was attached with arguments (0,body).
    // our convention is to treat all attaches as (body,0), i.e. so node[0].body
    // is always nonzero, so this flag records the fact that the arguments were
    // swapped.
    dJOINT_REVERSE = 2,

    // if this flag is set, the joint can not have just one body attached to it,
    // it must have either zero or two bodies attached.
    dJOINT_TWOBODIES = 4,

    dJOINT_DISABLED = 8
};


enum dJointConnectedBody
{
    dJCB__MIN,

    dJCB_FIRST_BODY = dJCB__MIN,
    dJCB_SECOND_BODY,

    dJCB__MAX,

};

static inline 
dJointConnectedBody EncodeJointOtherConnectedBody(dJointConnectedBody cbBodyKind)
{
    dIASSERT(dIN_RANGE(cbBodyKind, dJCB__MIN, dJCB__MAX));
    dSASSERT(dJCB__MAX == 2);

    return (dJointConnectedBody)(dJCB_FIRST_BODY + dJCB_SECOND_BODY - cbBodyKind);
}

/* joint body relativity enumeration */
enum dJointBodyRelativity 
{
    dJBR__MIN,

    dJBR_GLOBAL = dJBR__MIN,

    dJBR__BODIES_MIN,

    dJBR_BODY1 = dJBR__BODIES_MIN + dJCB_FIRST_BODY,
    dJBR_BODY2 = dJBR__BODIES_MIN + dJCB_SECOND_BODY,

    dJBR__BODIES_MAX = dJBR__BODIES_MIN + dJCB__MAX,

    dJBR__MAX,

    dJBR__DEFAULT = dJBR_GLOBAL,
    dJBR__BODIES_COUNT = dJBR__BODIES_MAX - dJBR__BODIES_MIN,

};

ODE_PURE_INLINE int dJBREncodeBodyRelativityStatus(int relativity)
{
    return dIN_RANGE(relativity, dJBR__BODIES_MIN, dJBR__BODIES_MAX);
}

ODE_PURE_INLINE dJointBodyRelativity dJBRSwapBodyRelativity(int relativity)
{
    dIASSERT(dIN_RANGE(relativity, dJBR__BODIES_MIN, dJBR__BODIES_MAX));
    return (dJointBodyRelativity)(dJBR_BODY1 + dJBR_BODY2 - relativity);
}




// there are two of these nodes in the joint, one for each connection to a
// body. these are node of a linked list kept by each body of it's connecting
// joints. but note that the body pointer in each node points to the body that
// makes use of the *other* node, not this node. this trick makes it a bit
// easier to traverse the body/joint graph.

struct dxJointNode
{
    dxJoint *joint;     // pointer to enclosing dxJoint object
    dxBody *body;       // *other* body this joint is connected to
    dxJointNode *next;  // next node in body's list of connected joints
};


struct dxJoint : public dObject
{
    // naming convention: the "first" body this is connected to is node[0].body,
    // and the "second" body is node[1].body. if this joint is only connected
    // to one body then the second body is 0.

    // info returned by getInfo1 function. the constraint dimension is m (<=6).
    // i.e. that is the total number of rows in the jacobian. `nub' is the
    // number of unbounded variables (which have lo,hi = -/+ infinity).

    struct Info1
    {
        // Structure size should not exceed sizeof(pointer) bytes to have 
        // to have good memory pattern in dxQuickStepper()
        uint8 m, nub;
    };

    // info returned by getInfo2 function

    enum
    {
        GI2__J_MIN,
        GI2__JL_MIN = GI2__J_MIN + dDA__L_MIN,

        GI2_JLX = GI2__J_MIN + dDA_LX,
        GI2_JLY = GI2__J_MIN + dDA_LY,
        GI2_JLZ = GI2__J_MIN + dDA_LZ,

        GI2__JL_MAX = GI2__J_MIN + dDA__L_MAX,

        GI2__JA_MIN = GI2__J_MIN + dDA__A_MIN,

        GI2_JAX = GI2__J_MIN + dDA_AX,
        GI2_JAY = GI2__J_MIN + dDA_AY,
        GI2_JAZ = GI2__J_MIN + dDA_AZ,

        GI2__JA_MAX = GI2__J_MIN + dDA__A_MAX,
        GI2__J_MAX = GI2__J_MIN + dDA__MAX,
    };

    enum
    {
        GI2_RHS,
        GI2_CFM,
        GI2__RHS_CFM_MAX,
    };

    enum
    {
        GI2_LO,
        GI2_HI,
        GI2__LO_HI_MAX,
    };

    // info returned by getSureMaxInfo function. 
    // The information is used for memory reservation in calculations.

    struct SureMaxInfo
    {
        // The value of `max_m' must ALWAYS be not less than the value of `m'
        // the getInfo1 call can generate in current joint state. Another 
        // requirement is that the value should be provided very quickly, 
        // without the excessive calculations.
        // If it is hard/impossible to quickly predict the maximal value of `m'
        // (which is the case for most joint types) the maximum for current 
        // joint type in general should be returned. If it can be known the `m'
        // will be smaller, it can save a bit of memory from being reserved 
        // for calculations if that smaller value is returned.

        uint8 max_m; // Estimate of maximal `m' in Info1
    };


    unsigned flags;             // dJOINT_xxx flags
    dxJointNode node[2];        // connections to bodies. node[1].body can be 0
    dJointFeedback *feedback;   // optional feedback structure
    dReal lambda[6];            // lambda generated by last step


    dxJoint( dxWorld *w );
    virtual ~dxJoint();

    bool GetIsJointReverse() const { return (this->flags & dJOINT_REVERSE) != 0; }

    virtual void getInfo1( Info1* info ) = 0;

    // integrator parameters
    virtual void getInfo2( 
        // fps=frames per second (1/stepsize), erp=default error reduction parameter (0..1)
        dReal worldFPS, dReal worldERP, 
        // elements to jump from one row to the next in J's
        int rowskip,
        // for the first and second body, pointers to two (linear and angular)
        // n*3 jacobian sub matrices, stored by rows. these matrices will have
        // been initialized to 0 on entry. if the second body is zero then the
        // J2xx pointers may be 0.
        dReal *J1, dReal *J2,
        // elements to jump from one pair of scalars to the next
        int pairskip,
        // right hand sides of the equation J*v = c + cfm * lambda. cfm is the
        // "constraint force mixing" vector. c is set to zero on entry, cfm is
        // set to a constant value (typically very small or zero) value on entry.
        dReal *pairRhsCfm,
        // lo and hi limits for variables (set to -/+ infinity on entry).
        dReal *pairLoHi,
        // findex vector for variables. see the LCP solver interface for a
        // description of what this does. this is set to -1 on entry.
        // note that the returned indexes are relative to the first index of
        // the constraint.
        int *findex) = 0;
    // This call quickly!!! estimates maximum value of "m" that could be returned by getInfo1()
    // See comments at definition of SureMaxInfo for details.
    virtual void getSureMaxInfo( SureMaxInfo* info ) = 0;
    virtual dJointType type() const = 0;
    virtual size_t size() const = 0;

    /// Set values which are relative with respect to bodies.
    /// Each dxJoint should redefine it if needed.
    virtual void setRelativeValues();

    // Test if this joint should be used in the simulation step
    // (has the enabled flag set, and is attached to at least one dynamic body)
    bool isEnabled() const;
};


// joint group. NOTE: any joints in the group that have their world destroyed
// will have their world pointer set to 0.

struct dxJointGroup : public dBase
{
    dxJointGroup(): m_num(0), m_stack() {}

    template<class T>
    T *alloc(dWorldID w)
    {
        T *j = (T *)m_stack.alloc(sizeof(T));
        if (j != NULL) {
            ++m_num;
            new(j) T(w);
            j->flags |= dJOINT_INGROUP;
        }
        return j;
    }

    size_t getJointCount() const { return m_num; }
    size_t exportJoints(dxJoint **jlist);

    void *beginEnum() { return m_stack.rewind(); }
    void *continueEnum(size_t num_bytes) { return m_stack.next(num_bytes); }

    void freeAll();

private:
    size_t m_num;        // number of joints on the stack
    dObStack m_stack; // a stack of (possibly differently sized) dxJoint objects.
};

// common limit and motor information for a single joint axis of movement
struct dxJointLimitMotor
{
    dReal vel, fmax;        // powered joint: velocity, max force
    dReal lostop, histop;   // joint limits, relative to initial position
    dReal fudge_factor;     // when powering away from joint limits
    dReal normal_cfm;       // cfm to use when not at a stop
    dReal stop_erp, stop_cfm; // erp and cfm for when at joint limit
    dReal bounce;           // restitution factor
    // variables used between getInfo1() and getInfo2()
    int limit;          // 0=free, 1=at lo limit, 2=at hi limit
    dReal limit_err;    // if at limit, amount over limit

    void init( dxWorld * );
    void set( int num, dReal value );
    dReal get( int num ) const;
    bool testRotationalLimit( dReal angle );

    enum
    {
        GI2__JL_MIN = dxJoint::GI2__JL_MIN,
        GI2__JA_MIN = dxJoint::GI2__JA_MIN,
        GI2_JAX = dxJoint::GI2_JAX,
        GI2_JAY = dxJoint::GI2_JAY,
        GI2_JAZ = dxJoint::GI2_JAZ,
        GI2_RHS = dxJoint::GI2_RHS,
        GI2_CFM = dxJoint::GI2_CFM,
        GI2_LO = dxJoint::GI2_LO,
        GI2_HI = dxJoint::GI2_HI,
    };

    bool addLimot( dxJoint *joint, dReal fps, 
        dReal *J1, dReal *J2, dReal *pairRhsCfm, dReal *pairLoHi,
        const dVector3 ax1, int rotational );
    bool addTwoPointLimot( dxJoint *joint, dReal fps,
        dReal *J1, dReal *J2, dReal *pairRhsCfm, dReal *pairLoHi,
        const dVector3 ax1, const dVector3 pt1, const dVector3 pt2 );
};


#endif


// Local Variables:
// mode:c++
// c-basic-offset:4
// End:
