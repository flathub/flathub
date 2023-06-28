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

#ifndef _ODE_COMMON_H_
#define _ODE_COMMON_H_

#include <ode/odeconfig.h>
#include <ode/error.h>


#ifdef __cplusplus
extern "C" {
#endif


/* configuration stuff */

/* constants */

/* pi and 1/sqrt(2) are defined here if necessary because they don't get
 * defined in <math.h> on some platforms (like MS-Windows)
 */

#ifndef M_PI
#define M_PI REAL(3.1415926535897932384626433832795029)
#endif
#ifndef M_PI_2
#define M_PI_2 REAL(1.5707963267948966192313216916398)
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 REAL(0.7071067811865475244008443621048490)
#endif


/* floating point data type, vector, matrix and quaternion types */

#if defined(dSINGLE)
typedef float dReal;
#ifdef dDOUBLE
#error You can only #define dSINGLE or dDOUBLE, not both.
#endif /* dDOUBLE */
#elif defined(dDOUBLE)
typedef double dReal;
#else
#error You must #define dSINGLE or dDOUBLE
#endif

/* Detect if we've got both trimesh engines enabled. */
#if dTRIMESH_ENABLED
#if dTRIMESH_OPCODE && dTRIMESH_GIMPACT
#error You can only #define dTRIMESH_OPCODE or dTRIMESH_GIMPACT, not both.
#endif
#endif /* dTRIMESH_ENABLED */

/*
 * Define a type for indices, either 16 or 32 bit, based on build option
 * TODO: Currently GIMPACT only supports 32 bit indices.
 */
#if dTRIMESH_16BIT_INDICES
#if dTRIMESH_GIMPACT
typedef duint32 dTriIndex;
#else /* dTRIMESH_GIMPACT */
typedef duint16 dTriIndex;
#endif /* dTRIMESH_GIMPACT */
#else /* dTRIMESH_16BIT_INDICES */
typedef duint32 dTriIndex;
#endif /* dTRIMESH_16BIT_INDICES */

/* round an integer up to a multiple of 4, except that 0 and 1 are unmodified
 * (used to compute matrix leading dimensions)
 */
#define dPAD(a) (((a) > 1) ? (((a) + 3) & (int)(~3)) : (a))

typedef enum {
    dSA__MIN,

    dSA_X = dSA__MIN,
    dSA_Y,
    dSA_Z,

    dSA__MAX,
} dSpaceAxis;

typedef enum {
    dMD__MIN,

    dMD_LINEAR = dMD__MIN,
    dMD_ANGULAR,

    dMD__MAX,
} dMotionDynamics;

typedef enum {
    dDA__MIN,

    dDA__L_MIN = dDA__MIN + dMD_LINEAR * dSA__MAX,

    dDA_LX = dDA__L_MIN + dSA_X,
    dDA_LY = dDA__L_MIN + dSA_Y,
    dDA_LZ = dDA__L_MIN + dSA_Z,

    dDA__L_MAX = dDA__L_MIN + dSA__MAX,

    dDA__A_MIN = dDA__MIN + dMD_ANGULAR * dSA__MAX,

    dDA_AX = dDA__A_MIN + dSA_X,
    dDA_AY = dDA__A_MIN + dSA_Y,
    dDA_AZ = dDA__A_MIN + dSA_Z,

    dDA__A_MAX = dDA__A_MIN + dSA__MAX,

    dDA__MAX = dDA__MIN + dMD__MAX * dSA__MAX,
} dDynamicsAxis;

typedef enum {
    dV3E__MIN,

    dV3E__AXES_MIN = dV3E__MIN,

    dV3E_X = dV3E__AXES_MIN + dSA_X,
    dV3E_Y = dV3E__AXES_MIN + dSA_Y,
    dV3E_Z = dV3E__AXES_MIN + dSA_Z,

    dV3E__AXES_MAX = dV3E__AXES_MIN + dSA__MAX,

    dV3E_PAD = dV3E__AXES_MAX,

    dV3E__MAX,

    dV3E__AXES_COUNT = dV3E__AXES_MAX - dV3E__AXES_MIN,
} dVec3Element;

typedef enum {
    dV4E__MIN,

    dV4E_X = dV4E__MIN + dSA_X,
    dV4E_Y = dV4E__MIN + dSA_Y,
    dV4E_Z = dV4E__MIN + dSA_Z,
    dV4E_O = dV4E__MIN + dSA__MAX,

    dV4E__MAX,
} dVec4Element;

typedef enum {
    dM3E__MIN,

    dM3E__X_MIN = dM3E__MIN + dSA_X * dV3E__MAX,
    
    dM3E__X_AXES_MIN = dM3E__X_MIN + dV3E__AXES_MIN,

    dM3E_XX = dM3E__X_MIN + dV3E_X,
    dM3E_XY = dM3E__X_MIN + dV3E_Y,
    dM3E_XZ = dM3E__X_MIN + dV3E_Z,

    dM3E__X_AXES_MAX = dM3E__X_MIN + dV3E__AXES_MAX,

    dM3E_XPAD = dM3E__X_MIN + dV3E_PAD,

    dM3E__X_MAX = dM3E__X_MIN + dV3E__MAX,

    dM3E__Y_MIN = dM3E__MIN + dSA_Y * dV3E__MAX,

    dM3E__Y_AXES_MIN = dM3E__Y_MIN + dV3E__AXES_MIN,

    dM3E_YX = dM3E__Y_MIN + dV3E_X,
    dM3E_YY = dM3E__Y_MIN + dV3E_Y,
    dM3E_YZ = dM3E__Y_MIN + dV3E_Z,

    dM3E__Y_AXES_MAX = dM3E__Y_MIN + dV3E__AXES_MAX,

    dM3E_YPAD = dM3E__Y_MIN + dV3E_PAD,

    dM3E__Y_MAX = dM3E__Y_MIN + dV3E__MAX,

    dM3E__Z_MIN = dM3E__MIN + dSA_Z * dV3E__MAX,

    dM3E__Z_AXES_MIN = dM3E__Z_MIN + dV3E__AXES_MIN,

    dM3E_ZX = dM3E__Z_MIN + dV3E_X,
    dM3E_ZY = dM3E__Z_MIN + dV3E_Y,
    dM3E_ZZ = dM3E__Z_MIN + dV3E_Z,

    dM3E__Z_AXES_MAX = dM3E__Z_MIN + dV3E__AXES_MAX,

    dM3E_ZPAD = dM3E__Z_MIN + dV3E_PAD,

    dM3E__Z_MAX = dM3E__Z_MIN + dV3E__MAX,

    dM3E__MAX = dM3E__MIN + dSA__MAX * dV3E__MAX,
} dMat3Element;

typedef enum {
    dM4E__MIN,

    dM4E__X_MIN = dM4E__MIN + dV4E_X * dV4E__MAX,

    dM4E_XX = dM4E__X_MIN + dV4E_X,
    dM4E_XY = dM4E__X_MIN + dV4E_Y,
    dM4E_XZ = dM4E__X_MIN + dV4E_Z,
    dM4E_XO = dM4E__X_MIN + dV4E_O,

    dM4E__X_MAX = dM4E__X_MIN + dV4E__MAX,

    dM4E__Y_MIN = dM4E__MIN + dV4E_Y * dV4E__MAX,

    dM4E_YX = dM4E__Y_MIN + dV4E_X,
    dM4E_YY = dM4E__Y_MIN + dV4E_Y,
    dM4E_YZ = dM4E__Y_MIN + dV4E_Z,
    dM4E_YO = dM4E__Y_MIN + dV4E_O,

    dM4E__Y_MAX = dM4E__Y_MIN + dV4E__MAX,

    dM4E__Z_MIN = dM4E__MIN + dV4E_Z * dV4E__MAX,

    dM4E_ZX = dM4E__Z_MIN + dV4E_X,
    dM4E_ZY = dM4E__Z_MIN + dV4E_Y,
    dM4E_ZZ = dM4E__Z_MIN + dV4E_Z,
    dM4E_ZO = dM4E__Z_MIN + dV4E_O,

    dM4E__Z_MAX = dM4E__Z_MIN + dV4E__MAX,

    dM4E__O_MIN = dM4E__MIN + dV4E_O * dV4E__MAX,

    dM4E_OX = dM4E__O_MIN + dV4E_X,
    dM4E_OY = dM4E__O_MIN + dV4E_Y,
    dM4E_OZ = dM4E__O_MIN + dV4E_Z,
    dM4E_OO = dM4E__O_MIN + dV4E_O,

    dM4E__O_MAX = dM4E__O_MIN + dV4E__MAX,

    dM4E__MAX = dM4E__MIN + dV4E__MAX * dV4E__MAX,
} dMat4Element;

typedef enum {
    dQUE__MIN,

    dQUE_R = dQUE__MIN,

    dQUE__AXIS_MIN,

    dQUE_I = dQUE__AXIS_MIN + dSA_X,
    dQUE_J = dQUE__AXIS_MIN + dSA_Y,
    dQUE_K = dQUE__AXIS_MIN + dSA_Z,

    dQUE__AXIS_MAX = dQUE__AXIS_MIN + dSA__MAX,

    dQUE__MAX = dQUE__AXIS_MAX,
} dQuatElement;

/* these types are mainly just used in headers */
typedef dReal dVector3[dV3E__MAX];
typedef dReal dVector4[dV4E__MAX];
typedef dReal dMatrix3[dM3E__MAX];
typedef dReal dMatrix4[dM4E__MAX];
typedef dReal dMatrix6[(dMD__MAX * dV3E__MAX) * (dMD__MAX * dSA__MAX)];
typedef dReal dQuaternion[dQUE__MAX];


/* precision dependent scalar math functions */

#if defined(dSINGLE)

#define REAL(x) (x##f)					/* form a constant */
#define dRecip(x) ((1.0f/(x)))				/* reciprocal */
#define dSqrt(x) (sqrtf(x))			/* square root */
#define dRecipSqrt(x) ((1.0f/sqrtf(x)))		/* reciprocal square root */
#define dSin(x) (sinf(x))				/* sine */
#define dCos(x) (cosf(x))				/* cosine */
#define dFabs(x) (fabsf(x))			/* absolute value */
#define dAtan2(y,x) (atan2f(y,x))		/* arc tangent with 2 args */
#define dAsin(x) (asinf(x))
#define dAcos(x) (acosf(x))
#define dFMod(a,b) (fmodf(a,b))		/* modulo */
#define dFloor(x) floorf(x)			/* floor */
#define dCeil(x) ceilf(x)			/* ceil */
#define dCopySign(a,b) _ode_copysignf(a, b) /* copy value sign */
#define dNextAfter(x, y) _ode_nextafterf(x, y) /* next value after */

#ifdef HAVE___ISNANF
#define dIsNan(x) (__isnanf(x))
#elif defined(HAVE__ISNANF)
#define dIsNan(x) (_isnanf(x))
#elif defined(HAVE_ISNANF)
#define dIsNan(x) (isnanf(x))
#else
  /*
     fall back to _isnan which is the VC way,
     this may seem redundant since we already checked
     for _isnan before, but if isnan is detected by
     configure but is not found during compilation
     we should always make sure we check for __isnanf,
     _isnanf and isnanf in that order before falling
     back to a default
  */
#define dIsNan(x) (_isnan(x))
#endif

#elif defined(dDOUBLE)

#define REAL(x) (x)
#define dRecip(x) (1.0/(x))
#define dSqrt(x) sqrt(x)
#define dRecipSqrt(x) (1.0/sqrt(x))
#define dSin(x) sin(x)
#define dCos(x) cos(x)
#define dFabs(x) fabs(x)
#define dAtan2(y,x) atan2((y),(x))
#define dAsin(x) asin(x)
#define dAcos(x) acos(x)
#define dFMod(a,b) (fmod((a),(b)))
#define dFloor(x) floor(x)
#define dCeil(x) ceil(x)
#define dCopySign(a,b) _ode_copysign(a, b)
#define dNextAfter(x, y) _ode_nextafter(x, y)

#ifdef HAVE___ISNAN
#define dIsNan(x) (__isnan(x))
#elif defined(HAVE__ISNAN)
#define dIsNan(x) (_isnan(x))
#elif defined(HAVE_ISNAN)
#define dIsNan(x) (isnan(x))
#else
#define dIsNan(x) (_isnan(x))
#endif

#else
#error You must #define dSINGLE or dDOUBLE
#endif

ODE_PURE_INLINE dReal dMin(dReal x, dReal y) { return x <= y ? x : y; }
ODE_PURE_INLINE dReal dMax(dReal x, dReal y) { return x <= y ? y : x; }


/* internal object types (all prefixed with `dx') */

struct dxWorld;		/* dynamics world */
struct dxSpace;		/* collision space */
struct dxBody;		/* rigid body (dynamics object) */
struct dxGeom;		/* geometry (collision object) */
struct dxJoint;
struct dxJointNode;
struct dxJointGroup;
struct dxWorldProcessThreadingManager;

typedef struct dxWorld *dWorldID;
typedef struct dxSpace *dSpaceID;
typedef struct dxBody *dBodyID;
typedef struct dxGeom *dGeomID;
typedef struct dxJoint *dJointID;
typedef struct dxJointGroup *dJointGroupID;
typedef struct dxWorldProcessThreadingManager *dWorldStepThreadingManagerID;

/* error numbers */

enum {
  d_ERR_UNKNOWN = 0,		/* unknown error */
  d_ERR_IASSERT,		/* internal assertion failed */
  d_ERR_UASSERT,		/* user assertion failed */
  d_ERR_LCP			/* user assertion failed */
};


/* joint type numbers */

typedef enum {
  dJointTypeNone = 0,		/* or "unknown" */
  dJointTypeBall,
  dJointTypeHinge,
  dJointTypeSlider,
  dJointTypeContact,
  dJointTypeUniversal,
  dJointTypeHinge2,
  dJointTypeFixed,
  dJointTypeNull,
  dJointTypeAMotor,
  dJointTypeLMotor,
  dJointTypePlane2D,
  dJointTypePR,
  dJointTypePU,
  dJointTypePiston,
  dJointTypeDBall,
  dJointTypeDHinge,
  dJointTypeTransmission,
} dJointType;


/* an alternative way of setting joint parameters, using joint parameter
 * structures and member constants. we don't actually do this yet.
 */

/*
typedef struct dLimot {
  int mode;
  dReal lostop, histop;
  dReal vel, fmax;
  dReal fudge_factor;
  dReal bounce, soft;
  dReal suspension_erp, suspension_cfm;
} dLimot;

enum {
  dLimotLoStop		= 0x0001,
  dLimotHiStop		= 0x0002,
  dLimotVel		= 0x0004,
  dLimotFMax		= 0x0008,
  dLimotFudgeFactor	= 0x0010,
  dLimotBounce		= 0x0020,
  dLimotSoft		= 0x0040
};
*/


/* standard joint parameter names. why are these here? - because we don't want
 * to include all the joint function definitions in joint.cpp. hmmmm.
 * MSVC complains if we call D_ALL_PARAM_NAMES_X with a blank second argument,
 * which is why we have the D_ALL_PARAM_NAMES macro as well. please copy and
 * paste between these two.
 */

#define D_ALL_PARAM_NAMES(start) \
  /* parameters for limits and motors */ \
  dParamLoStop = start, \
  dParamHiStop, \
  dParamVel, \
  dParamLoVel, \
  dParamHiVel, \
  dParamFMax, \
  dParamFudgeFactor, \
  dParamBounce, \
  dParamCFM, \
  dParamStopERP, \
  dParamStopCFM, \
  /* parameters for suspension */ \
  dParamSuspensionERP, \
  dParamSuspensionCFM, \
  dParamERP, \

  /*
   * \enum  D_ALL_PARAM_NAMES_X
   *
   * \var dParamGroup This is the starting value of the different group
   *                  (i.e. dParamGroup1, dParamGroup2, dParamGroup3)
   *                  It also helps in the use of parameter
   *                  (dParamGroup2 | dParamFMax) == dParamFMax2
   */
#define D_ALL_PARAM_NAMES_X(start,x) \
  dParamGroup ## x = start, \
  /* parameters for limits and motors */ \
  dParamLoStop ## x = start, \
  dParamHiStop ## x, \
  dParamVel ## x, \
  dParamLoVel ## x, \
  dParamHiVel ## x, \
  dParamFMax ## x, \
  dParamFudgeFactor ## x, \
  dParamBounce ## x, \
  dParamCFM ## x, \
  dParamStopERP ## x, \
  dParamStopCFM ## x, \
  /* parameters for suspension */ \
  dParamSuspensionERP ## x, \
  dParamSuspensionCFM ## x, \
  dParamERP ## x,

enum {
  D_ALL_PARAM_NAMES(0)
  dParamsInGroup,     /* < Number of parameter in a group */
  D_ALL_PARAM_NAMES_X(0x000,1)
  D_ALL_PARAM_NAMES_X(0x100,2)
  D_ALL_PARAM_NAMES_X(0x200,3)

  /* add a multiple of this constant to the basic parameter numbers to get
   * the parameters for the second, third etc axes.
   */
  dParamGroup=0x100
};


/* angular motor mode numbers */

enum {
  dAMotorUser = 0,
  dAMotorEuler = 1
};

/* transmission joint mode numbers */

enum {
  dTransmissionParallelAxes = 0,
  dTransmissionIntersectingAxes = 1,
  dTransmissionChainDrive = 2
};


/* joint force feedback information */

typedef struct dJointFeedback {
  dVector3 f1;		/* force applied to body 1 */
  dVector3 t1;		/* torque applied to body 1 */
  dVector3 f2;		/* force applied to body 2 */
  dVector3 t2;		/* torque applied to body 2 */
} dJointFeedback;


/* private functions that must be implemented by the collision library:
 * (1) indicate that a geom has moved, (2) get the next geom in a body list.
 * these functions are called whenever the position of geoms connected to a
 * body have changed, e.g. with dBodySetPosition(), dBodySetRotation(), or
 * when the ODE step function updates the body state.
 */

void dGeomMoved (dGeomID);
dGeomID dGeomGetBodyNext (dGeomID);

/**
 * dGetConfiguration returns the specific ODE build configuration as
 * a string of tokens. The string can be parsed in a similar way to
 * the OpenGL extension mechanism, the naming convention should be
 * familiar too. The following extensions are reported:
 *
 * ODE
 * ODE_single_precision
 * ODE_double_precision
 * ODE_EXT_no_debug
 * ODE_EXT_trimesh
 * ODE_EXT_opcode
 * ODE_EXT_gimpact
 * ODE_OPC_16bit_indices
 * ODE_OPC_new_collider
 * ODE_EXT_mt_collisions
 * ODE_EXT_threading
 * ODE_THR_builtin_impl
 */
ODE_API const char* dGetConfiguration (void);

/**
 * Helper to check for a token in the ODE configuration string.
 * Caution, this function is case sensitive.
 *
 * @param token A configuration token, see dGetConfiguration for details
 *
 * @return 1 if exact token is present, 0 if not present
 */
ODE_API int dCheckConfiguration( const char* token );

#ifdef __cplusplus
}
#endif

#endif
