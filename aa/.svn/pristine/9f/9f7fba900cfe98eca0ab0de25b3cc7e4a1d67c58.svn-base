/*
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <ode/ode.h>

#define MAX_RAGDOLLS 64 
#define MAX_RAGDOLL_OBJECTS 15
#define MAX_RAGDOLL_JOINTS 15
#define MAX_CONTACTS 32 
#define MAX_FORCES 4
#define MAX_ODESTEPS 120
#define MIN_ODESTEPS 20
#define RAGDOLL_DURATION 10000 //10 seconds

//body id's
#define CHEST 0
#define PELVIS 1
#define HEAD 2
#define RIGHTUPPERLEG 3
#define LEFTUPPERLEG 4
#define RIGHTLOWERLEG 5
#define LEFTLOWERLEG 6
#define RIGHTFOOT 7
#define LEFTFOOT 8
#define RIGHTUPPERARM 9
#define LEFTUPPERARM 10
#define RIGHTFOREARM 11
#define LEFTFOREARM 12
#define RIGHTHAND 13
#define LEFTHAND 14

//joint id's
#define MIDSPINE 0
#define LOWSPINE 1
#define NECK 2
#define RIGHTHIP 3
#define LEFTHIP 4
#define RIGHTKNEE 5
#define LEFTKNEE 6
#define RIGHTANKLE 7
#define LEFTANKLE 8
#define RIGHTSHOULDER 9
#define LEFTSHOULDER 10
#define RIGHTELBOW 11
#define LEFTELBOW 12
#define RIGHTWRIST 13
#define LEFTWRIST 14

//ragdoll dimensions(from bottom(base of foot) center of skeleton)
#define RAGDOLL_DIMS 56
#define ELBOW_X_OFF 0 //elbow joint
#define ELBOW_Y_OFF 1
#define ELBOW_Z_OFF 2
#define WRIST_X_OFF 3 //wrist joint
#define WRIST_Y_OFF 4
#define WRIST_Z_OFF 5
#define FINGERS_X_OFF 6 //base of fingers
#define FINGERS_Y_OFF 7
#define FINGERS_Z_OFF 8
#define FOOT_LEN 9 // ankles to base of ball of foot 
#define HEEL_LEN 10 // ankles to hell

#define HEAD_H 11 //to base of skull
#define NECK_H 12 //to base of neck 
#define SHOULDER_H 13 //to shoulder sockets
#define CHEST_H 14 //to top of chest
#define HIP_H 15 //to hip sockets

#define HEAD_W 16
#define SHOULDER_W 17
#define CHEST_W 18 // actually wider, but we want narrower than shoulders (esp. with large radius)
#define BICEP_W 19 //thickness of bicep
#define FOREARM_W 20 //thickness of forearm
#define HAND_W 21 //width of hand
#define LEG_W 22 // between middles of upper legs
#define PELVIS_W 23 // actually wider, but we want smaller than hip width
#define THIGH_W 24 //thickness of thigh
#define SHIN_W 25 //thickness of shin
#define FOOT_W 26 //width of foot

#define KNEE_X_OFF 27 //knee joint
#define KNEE_Y_OFF 28
#define KNEE_Z_OFF 29
#define ANKLE_X_OFF 30 //ankle joint
#define ANKLE_Y_OFF 31
#define ANKLE_Z_OFF 32

#define GLOBAL_X_OFF 33 //bottom(base of foot) center of skeleton from 0,0,0
#define GLOBAL_Y_OFF 34
#define GLOBAL_Z_OFF 35

//contraint section
#define HIP_LOSTOP1 36
#define HIP_HISTOP1 37
#define HIP_LOSTOP2 38
#define HIP_HISTOP2 39
#define KNEE_LOSTOP 40
#define KNEE_HISTOP 41
#define ANKLE_LOSTOP 42
#define ANKLE_HISTOP 43
#define SHOULDER_LOSTOP1 44
#define SHOULDER_HISTOP1 45
#define SHOULDER_LOSTOP2 46
#define SHOULDER_HISTOP2 47
#define ELBOW_LOSTOP 48
#define ELBOW_HISTOP 49
#define WRIST_LOSTOP 50
#define WRIST_HISTOP 51
#define HEAD_LOSTOP1 52
#define HEAD_HISTOP1 53
#define HEAD_LOSTOP2 54
#define HEAD_HISTOP2 55

dWorldID RagDollWorld;
dSpaceID RagDollSpace;

dJointGroupID contactGroup;

int lastODEUpdate;

typedef struct RagDollBind_s {

    const char *name;
    int object;

} RagDollBind_t;

extern RagDollBind_t RagDollBinds[];
extern int RagDollBindsCount;

typedef struct RagDollObject_s {

	dBodyID body;
	dMass mass;
	dGeomID geom;
    matrix3x4_t initmat;

} RagDollObject_t;

typedef struct RagDollForce_s {

	vec3_t org;
	vec3_t dir;
	float force;
	
	int spawnTime;

	int destroyed;

} RagDollForce_t;

#define GROW_ODE_VERTS 16384
#define GROW_ODE_TRIS 16384
typedef struct RagDollWorld_s {

	dVector3	*ODEVerts;
	dTriIndex	*ODETris;
    int numODEVerts, maxODEVerts;
    int numODETris, maxODETris;
	dTriMeshDataID triMesh;
	dGeomID geom;

} RagDollWorld_t;

typedef struct RagDoll_s {

	char	name[MAX_QPATH];

	RagDollObject_t RagDollObject[MAX_RAGDOLL_OBJECTS];
	dJointID RagDollJoint[MAX_RAGDOLL_JOINTS];

	RagDollForce_t RagDollForces[MAX_FORCES];
	
	// For grabbing the animated position of the mesh just before it goes limp
	matrix3x4_t initframe[SKELETAL_MAX_BONEMATS];
	
	// entity information
	float	angles[3];
	vec3_t	origin;
	vec3_t	curPos;
	float velocity;

	int spawnTime;

	int destroyed;

} RagDoll_t;

RagDoll_t RagDoll[MAX_RAGDOLLS];

// A few values will be copied into this. It's a bit wasteful, but it allows
// us to reuse code that expects them to be in an entity_t struct.
entity_t RagDollEntity[MAX_RAGDOLLS];

//surface for ragdoll to collide
RagDollWorld_t RagDollTriWorld;

//Funcs
void R_SimulateAllRagdolls (void);
void R_ClearAllRagdolls (void);
void RGD_CreateWorldObject (void);
void RGD_DestroyWorldObject (void);
void RGD_AddNewRagdoll (const entity_t *ent, float velocity);
void RGD_DestroyWorldTrimesh (void);
void RGD_BuildWorldTrimesh (void);
