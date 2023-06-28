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

#include <ode/collision.h>
#include <ccd/ccd.h>
#include "ccdcustom/vec3.h"
#include "ccdcustom/quat.h"
#include "config.h"
#include "odemath.h"
#include "collision_libccd.h"
#include "collision_trimesh_internal.h"
#include "collision_std.h"
#include "collision_util.h"
#include "error.h"


struct _ccd_obj_t {
    ccd_vec3_t pos;
    ccd_quat_t rot, rot_inv;
};
typedef struct _ccd_obj_t ccd_obj_t;

struct _ccd_box_t {
    ccd_obj_t o;
    ccd_real_t dim[3];
};
typedef struct _ccd_box_t ccd_box_t;

struct _ccd_cap_t {
    ccd_obj_t o;
    ccd_real_t radius;
    ccd_vec3_t axis;
    ccd_vec3_t p1;
    ccd_vec3_t p2;
};
typedef struct _ccd_cap_t ccd_cap_t;

struct _ccd_cyl_t {
    ccd_obj_t o;
    ccd_real_t radius;
    ccd_vec3_t axis;
    ccd_vec3_t p1;
    ccd_vec3_t p2;
};
typedef struct _ccd_cyl_t ccd_cyl_t;

struct _ccd_sphere_t {
    ccd_obj_t o;
    ccd_real_t radius;
};
typedef struct _ccd_sphere_t ccd_sphere_t;

struct _ccd_convex_t {
    ccd_obj_t o;
    dxConvex *convex;
};
typedef struct _ccd_convex_t ccd_convex_t;

struct _ccd_triangle_t {
    ccd_obj_t o;
    ccd_vec3_t vertices[3];
};
typedef struct _ccd_triangle_t ccd_triangle_t;

/** Transforms geom to ccd struct */
static void ccdGeomToObj(const dGeomID g, ccd_obj_t *);
static void ccdGeomToBox(const dGeomID g, ccd_box_t *);
static void ccdGeomToCap(const dGeomID g, ccd_cap_t *);
static void ccdGeomToCyl(const dGeomID g, ccd_cyl_t *);
static void ccdGeomToSphere(const dGeomID g, ccd_sphere_t *);
static void ccdGeomToConvex(const dGeomID g, ccd_convex_t *);

/** Support functions */
static void ccdSupportBox(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);
static void ccdSupportCap(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);
static void ccdSupportCyl(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);
static void ccdSupportSphere(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);
static void ccdSupportConvex(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);

/** Center function */
static void ccdCenter(const void *obj, ccd_vec3_t *c);

/** General collide function */
static int ccdCollide(dGeomID o1, dGeomID o2, int flags,
    dContactGeom *contact, int skip,
    void *obj1, ccd_support_fn supp1, ccd_center_fn cen1,
    void *obj2, ccd_support_fn supp2, ccd_center_fn cen2);

static int collideCylCyl(dxGeom *o1, dxGeom *o2, ccd_cyl_t* cyl1, ccd_cyl_t* cyl2, int flags, dContactGeom *contacts, int skip);
static bool testAndPrepareDiscContactForAngle(dReal angle, dReal radius, dReal length, dReal lSum, ccd_cyl_t *priCyl, ccd_cyl_t *secCyl, ccd_vec3_t &p, dReal &out_depth);
// Adds a contact between 2 cylinders
static int addCylCylContact(dxGeom *o1, dxGeom *o2, ccd_vec3_t* axis, dContactGeom *contacts, ccd_vec3_t* p, dReal normaldir, dReal depth, int j, int flags, int skip);

static unsigned addTrianglePerturbedContacts(dxGeom *o1, dxGeom *o2, IFaceAngleStorageView *meshFaceAngleView, 
    const int *indices, unsigned numIndices, int flags, dContactGeom *contacts, int skip,
    ccd_convex_t *c1, ccd_triangle_t *c2, dVector3 *triangle, dContactGeom *contact, unsigned contacCount);
static bool correctTriangleContactNormal(ccd_triangle_t *t, dContactGeom *contact, IFaceAngleStorageView *meshFaceAngleView, const int *indices, unsigned numIndices);
static unsigned addUniqueContact(dContactGeom *contacts, dContactGeom *c, unsigned contactcount, unsigned maxcontacts, int flags, int skip);
static void setObjPosToTriangleCenter(ccd_triangle_t *t); 
static void ccdSupportTriangle(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v);


static 
void ccdGeomToObj(const dGeomID g, ccd_obj_t *o)
{
    const dReal *ode_pos;
    dQuaternion ode_rot;

    ode_pos = dGeomGetPosition(g);
    dGeomGetQuaternion(g, ode_rot);

    ccdVec3Set(&o->pos, ode_pos[0], ode_pos[1], ode_pos[2]);
    ccdQuatSet(&o->rot, ode_rot[1], ode_rot[2], ode_rot[3], ode_rot[0]);

    ccdQuatInvert2(&o->rot_inv, &o->rot);
}

static 
void ccdGeomToBox(const dGeomID g, ccd_box_t *box)
{
    dVector3 dim;

    ccdGeomToObj(g, (ccd_obj_t *)box);

    dGeomBoxGetLengths(g, dim);
    box->dim[0] = (ccd_real_t)(dim[0] * 0.5);
    box->dim[1] = (ccd_real_t)(dim[1] * 0.5);
    box->dim[2] = (ccd_real_t)(dim[2] * 0.5);
}

static 
void ccdGeomToCap(const dGeomID g, ccd_cap_t *cap)
{
    dReal r, h;
    ccdGeomToObj(g, (ccd_obj_t *)cap);

    dGeomCapsuleGetParams(g, &r, &h);
    cap->radius = r;
    ccdVec3Set(&cap->axis, 0.0, 0.0, h / 2);
    ccdQuatRotVec(&cap->axis, &cap->o.rot);
    ccdVec3Copy(&cap->p1, &cap->axis);
    ccdVec3Copy(&cap->p2, &cap->axis);
    ccdVec3Scale(&cap->p2, -1.0);
    ccdVec3Add(&cap->p1, &cap->o.pos);
    ccdVec3Add(&cap->p2, &cap->o.pos);
}

static 
void ccdGeomToCyl(const dGeomID g, ccd_cyl_t *cyl)
{
    dReal r, h;
    ccdGeomToObj(g, (ccd_obj_t *)cyl);

    dGeomCylinderGetParams(g, &r, &h);
    cyl->radius = r;
    ccdVec3Set(&cyl->axis, 0.0, 0.0, h / 2);
    ccdQuatRotVec(&cyl->axis, &cyl->o.rot);
    ccdVec3Copy(&cyl->p1, &cyl->axis);
    ccdVec3Copy(&cyl->p2, &cyl->axis);
    int cylAxisNormalizationResult = ccdVec3SafeNormalize(&cyl->axis);
    dUVERIFY(cylAxisNormalizationResult == 0, "Invalid cylinder has been passed");
    ccdVec3Scale(&cyl->p2, -1.0);
    ccdVec3Add(&cyl->p1, &cyl->o.pos);
    ccdVec3Add(&cyl->p2, &cyl->o.pos);
}

static 
void ccdGeomToSphere(const dGeomID g, ccd_sphere_t *s)
{
    ccdGeomToObj(g, (ccd_obj_t *)s);
    s->radius = dGeomSphereGetRadius(g);
}

static 
void ccdGeomToConvex(const dGeomID g, ccd_convex_t *c)
{
    ccdGeomToObj(g, (ccd_obj_t *)c);
    c->convex = (dxConvex *)g;
}


static 
void ccdSupportBox(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_box_t *o = (const ccd_box_t *)obj;
    ccd_vec3_t dir;

    ccdVec3Copy(&dir, _dir);
    ccdQuatRotVec(&dir, &o->o.rot_inv);

    ccdVec3Set(v, ccdSign(ccdVec3X(&dir)) * o->dim[0],
        ccdSign(ccdVec3Y(&dir)) * o->dim[1],
        ccdSign(ccdVec3Z(&dir)) * o->dim[2]);

    // transform support vertex
    ccdQuatRotVec(v, &o->o.rot);
    ccdVec3Add(v, &o->o.pos);
}

static 
void ccdSupportCap(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_cap_t *o = (const ccd_cap_t *)obj;

    ccdVec3Copy(v, _dir);
    ccdVec3Scale(v, o->radius);

    if (ccdVec3Dot(_dir, &o->axis) > 0.0){
        ccdVec3Add(v, &o->p1);
    }else{
        ccdVec3Add(v, &o->p2);
    }

}

static 
void ccdSupportCyl(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_cyl_t *cyl = (const ccd_cyl_t *)obj;
    ccd_vec3_t dir;
    ccd_real_t len;
    
    ccd_real_t dot = ccdVec3Dot(_dir, &cyl->axis);
    if (dot > 0.0){
        ccdVec3Copy(v, &cyl->p1);
    } else{
        ccdVec3Copy(v, &cyl->p2);
    }
    // project dir onto cylinder's 'top'/'bottom' plane
    ccdVec3Copy(&dir, &cyl->axis);
    ccdVec3Scale(&dir, -dot);
    ccdVec3Add(&dir, _dir);
    len = CCD_SQRT(ccdVec3Len2(&dir));
    if (!ccdIsZero(len)) {
        ccdVec3Scale(&dir, cyl->radius / len);
        ccdVec3Add(v, &dir);
    }
}

static 
void ccdSupportSphere(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_sphere_t *s = (const ccd_sphere_t *)obj;

    ccdVec3Copy(v, _dir);
    ccdVec3Scale(v, s->radius);
    dIASSERT(dFabs(CCD_SQRT(ccdVec3Len2(_dir)) - REAL(1.0)) < 1e-6); // ccdVec3Scale(v, CCD_ONE / CCD_SQRT(ccdVec3Len2(_dir)));

    ccdVec3Add(v, &s->o.pos);
}

static 
void ccdSupportConvex(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_convex_t *c = (const ccd_convex_t *)obj;
    ccd_vec3_t dir, p;
    ccd_real_t maxdot, dot;
    size_t i;
    const dReal *curp;

    ccdVec3Copy(&dir, _dir);
    ccdQuatRotVec(&dir, &c->o.rot_inv);

    maxdot = -CCD_REAL_MAX;
    curp = c->convex->points;
    for (i = 0; i < c->convex->pointcount; i++, curp += 3){
        ccdVec3Set(&p, curp[0], curp[1], curp[2]);
        dot = ccdVec3Dot(&dir, &p);
        if (dot > maxdot){
            ccdVec3Copy(v, &p);
            maxdot = dot;
        }
    }


    // transform support vertex
    ccdQuatRotVec(v, &c->o.rot);
    ccdVec3Add(v, &c->o.pos);
}

static 
void ccdCenter(const void *obj, ccd_vec3_t *c)
{
    const ccd_obj_t *o = (const ccd_obj_t *)obj;
    ccdVec3Copy(c, &o->pos);
}

static 
int ccdCollide(
    dGeomID o1, dGeomID o2, int flags, dContactGeom *contact, int skip,
    void *obj1, ccd_support_fn supp1, ccd_center_fn cen1,
    void *obj2, ccd_support_fn supp2, ccd_center_fn cen2)
{
    ccd_t ccd;
    int res;
    ccd_real_t depth;
    ccd_vec3_t dir, pos;
    int max_contacts = (flags & NUMC_MASK);

    if (max_contacts < 1)
        return 0;

    CCD_INIT(&ccd);
    ccd.support1 = supp1;
    ccd.support2 = supp2;
    ccd.center1  = cen1;
    ccd.center2  = cen2;
    ccd.max_iterations = 500;
    ccd.mpr_tolerance = (ccd_real_t)1E-6;


    if (flags & CONTACTS_UNIMPORTANT){
        if (ccdMPRIntersect(obj1, obj2, &ccd)){
            return 1;
        }else{
            return 0;
        }
    }

    res = ccdMPRPenetration(obj1, obj2, &ccd, &depth, &dir, &pos);
    if (res == 0){
        contact->g1 = o1;
        contact->g2 = o2;

        contact->side1 = contact->side2 = -1;

        contact->depth = depth;

        contact->pos[0] = ccdVec3X(&pos);
        contact->pos[1] = ccdVec3Y(&pos);
        contact->pos[2] = ccdVec3Z(&pos);

        ccdVec3Scale(&dir, -1.);
        contact->normal[0] = ccdVec3X(&dir);
        contact->normal[1] = ccdVec3Y(&dir);
        contact->normal[2] = ccdVec3Z(&dir);

        return 1;
    }

    return 0;
}

/*extern */
int dCollideBoxCylinderCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_cyl_t cyl;
    ccd_box_t box;

    ccdGeomToBox(o1, &box);
    ccdGeomToCyl(o2, &cyl);

    return ccdCollide(o1, o2, flags, contact, skip,
        &box, ccdSupportBox, ccdCenter,
        &cyl, ccdSupportCyl, ccdCenter);
}

/*extern */
int dCollideCapsuleCylinder(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_cap_t cap;
    ccd_cyl_t cyl;

    ccdGeomToCap(o1, &cap);
    ccdGeomToCyl(o2, &cyl);

    return ccdCollide(o1, o2, flags, contact, skip,
        &cap, ccdSupportCap, ccdCenter,
        &cyl, ccdSupportCyl, ccdCenter);
}

/*extern */
int dCollideConvexBoxCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_box_t box;
    ccd_convex_t conv;

    ccdGeomToConvex(o1, &conv);
    ccdGeomToBox(o2, &box);

    return ccdCollide(o1, o2, flags, contact, skip,
        &conv, ccdSupportConvex, ccdCenter,
        &box, ccdSupportBox, ccdCenter);
}

/*extern */
int dCollideConvexCapsuleCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_cap_t cap;
    ccd_convex_t conv;

    ccdGeomToConvex(o1, &conv);
    ccdGeomToCap(o2, &cap);

    return ccdCollide(o1, o2, flags, contact, skip,
        &conv, ccdSupportConvex, ccdCenter,
        &cap, ccdSupportCap, ccdCenter);
}

/*extern */
int dCollideConvexSphereCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_sphere_t sphere;
    ccd_convex_t conv;

    ccdGeomToConvex(o1, &conv);
    ccdGeomToSphere(o2, &sphere);

    return ccdCollide(o1, o2, flags, contact, skip,
        &conv, ccdSupportConvex, ccdCenter,
        &sphere, ccdSupportSphere, ccdCenter);
}

/*extern */
int dCollideConvexCylinderCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_cyl_t cyl;
    ccd_convex_t conv;

    ccdGeomToConvex(o1, &conv);
    ccdGeomToCyl(o2, &cyl);

    return ccdCollide(o1, o2, flags, contact, skip,
        &conv, ccdSupportConvex, ccdCenter,
        &cyl, ccdSupportCyl, ccdCenter);
}

/*extern */
int dCollideConvexConvexCCD(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_convex_t c1, c2;

    ccdGeomToConvex(o1, &c1);
    ccdGeomToConvex(o2, &c2);

    return ccdCollide(o1, o2, flags, contact, skip,
        &c1, ccdSupportConvex, ccdCenter,
        &c2, ccdSupportConvex, ccdCenter);
}


/*extern */
int dCollideCylinderCylinder(dxGeom *o1, dxGeom *o2, int flags, dContactGeom *contact, int skip)
{
    ccd_cyl_t cyl1, cyl2;
    
    ccdGeomToCyl(o1, &cyl1);
    ccdGeomToCyl(o2, &cyl2);
    
    int numContacts = collideCylCyl(o1, o2, &cyl1, &cyl2, flags, contact, skip);
    if (numContacts < 0) {
        numContacts = ccdCollide(o1, o2, flags, contact, skip,
                                 &cyl1, ccdSupportCyl, ccdCenter,
                                 &cyl2, ccdSupportCyl, ccdCenter);
    }
    return numContacts;
}

static 
int collideCylCyl(dxGeom *o1, dxGeom *o2, ccd_cyl_t* cyl1, ccd_cyl_t* cyl2, int flags, dContactGeom *contacts, int skip) 
{
    int maxContacts = (flags & NUMC_MASK);
    dAASSERT(maxContacts != 0);

    maxContacts = maxContacts > 8 ? 8 : maxContacts;
    
    dReal axesProd = dFabs(ccdVec3Dot(&cyl1->axis, &cyl2->axis));
    // Check if cylinders' axes are in line
    if (REAL(1.0) - axesProd < 1e-3f) {
        ccd_vec3_t p, proj;
        dReal r1, l1;
        dReal r2, l2;
        dGeomCylinderGetParams(o1, &r1, &l1);
        dGeomCylinderGetParams(o2, &r2, &l2);
        l1 *= 0.5f;
        l2 *= 0.5f;
       
        // Determine the cylinder with smaller radius (minCyl) and bigger radius (maxCyl) and their respective properties: radius, length
        bool r1IsMin;
        dReal rmin, rmax;
        ccd_cyl_t *minCyl, *maxCyl;
        if (r1 <= r2) {
            rmin = r1; rmax = r2;
            minCyl = cyl1; maxCyl = cyl2;
            r1IsMin = true;
        }
        else {
            rmin = r2; rmax = r1;
            minCyl = cyl2; maxCyl = cyl1;
            r1IsMin = false;
        }

        dReal lSum = l1 + l2;

        ccdVec3Copy(&p, &minCyl->o.pos);
        ccdVec3Sub(&p, &maxCyl->o.pos);
        dReal dot = ccdVec3Dot(&p, &maxCyl->axis);
        
        // Maximum possible contact depth
        dReal depth_v = lSum - dFabs(dot) + dSqrt(dMax(0, REAL(1.0) - axesProd * axesProd)) * rmin;
        if (depth_v < 0) {
            return 0;
        }

        // Project the smaller cylinder's center onto the larger cylinder's plane
        ccdVec3Copy(&proj, &maxCyl->axis);
        ccdVec3Scale(&proj, -dot);
        ccdVec3Add(&proj, &p);
        dReal radiiDiff = (dReal)sqrt(ccdVec3Len2(&proj));
        dReal depth_h = r1 + r2 - radiiDiff;

        // Check the distance between cylinders' centers
        if (depth_h < 0) {
            return 0;
        }

        // Check if "vertical" contact depth is less than "horizontal" contact depth
        if (depth_v < depth_h) {
            int contactCount = 0;
            dReal dot2 = -ccdVec3Dot(&p, &minCyl->axis);
            // lmin, lmax - distances from cylinders' centers to potential contact points relative to cylinders' axes
            dReal lmax = r1IsMin ? l2 : l1;
            dReal lmin = r1IsMin ? l1 : l2;
            lmin = dot2 < 0 ? -lmin : lmin;
            lmax = dot < 0 ? -lmax : lmax;
            // Contact normal direction, relative to o1's axis
            dReal normaldir = (dot < 0) != r1IsMin ? REAL(1.0) : -REAL(1.0);
            
            if (rmin + radiiDiff <= rmax) {
                // Case 1: The smaller disc is fully contained within the larger one
                // Simply generate N points on the rim of the smaller disc
                dReal maxContactsRecip = (dReal)(0 < maxContacts ? (2.0 * M_PI / maxContacts) : (2.0 * M_PI)); // The 'else' value does not matter. Just try helping the optimizer.
                for (int i = 0; i < maxContacts; i++) {
                    dReal depth;
                    dReal a = maxContactsRecip * i;
                    if (testAndPrepareDiscContactForAngle(a, rmin, lmin, lSum, minCyl, maxCyl, p, depth)) {
                        contactCount = addCylCylContact(o1, o2, &maxCyl->axis, contacts, &p, normaldir, depth, contactCount, flags, skip);
                        if ((flags & CONTACTS_UNIMPORTANT) != 0) {
                            dIASSERT(contactCount != 0);
                            break;
                        }
                    }
                }
                return contactCount;

            } else {
                // Case 2: Discs intersect
                // Firstly, find intersections assuming the larger cylinder is placed at (0,0,0)
                // http://math.stackexchange.com/questions/256100/how-can-i-find-the-points-at-which-two-circles-intersect
                ccd_vec3_t proj2;
                ccdVec3Copy(&proj2, &proj);
                ccdQuatRotVec(&proj, &maxCyl->o.rot_inv);
                dReal d = dSqrt(ccdVec3X(&proj) * ccdVec3X(&proj) + ccdVec3Y(&proj) * ccdVec3Y(&proj));
                dIASSERT(d != REAL(0.0));
                
                dReal dRecip = REAL(1.0) / d;
                dReal rmaxSquare = rmax * rmax, rminSquare = rmin * rmin, dSquare = d * d;

                dReal minA, diffA, minB, diffB;

                {
                    dReal l = (rmaxSquare - rminSquare + dSquare) * (REAL(0.5) * dRecip);
                    dReal h = dSqrt(rmaxSquare - l * l);
                    dReal divLbyD = l * dRecip, divHbyD = h * dRecip;
                    dReal x1 = divLbyD * ccdVec3X(&proj) + divHbyD * ccdVec3Y(&proj);
                    dReal y1 = divLbyD * ccdVec3Y(&proj) - divHbyD * ccdVec3X(&proj);
                    dReal x2 = divLbyD * ccdVec3X(&proj) - divHbyD * ccdVec3Y(&proj);
                    dReal y2 = divLbyD * ccdVec3Y(&proj) + divHbyD * ccdVec3X(&proj);
                    // Map the intersection points to angles
                    dReal ap1 = dAtan2(y1, x1);
                    dReal ap2 = dAtan2(y2, x2);
                    minA = dMin(ap1, ap2);
                    dReal maxA = dMax(ap1, ap2);
                    // If the segment connecting cylinders' centers does not intersect the arc, change the angles
                    dReal a = dAtan2(ccdVec3Y(&proj), ccdVec3X(&proj));
                    if (a < minA || a > maxA) {
                        a = maxA;
                        maxA = (dReal)(minA + M_PI * 2.0);
                        minA = a;
                    }
                    diffA = maxA - minA;
                }
                
                // Do the same for the smaller cylinder assuming it is placed at (0,0,0) now
                ccdVec3Copy(&proj, &proj2);
                ccdVec3Scale(&proj, -1);
                ccdQuatRotVec(&proj, &minCyl->o.rot_inv);
                
                {
                    dReal l = (rminSquare - rmaxSquare + dSquare) * (REAL(0.5) * dRecip);
                    dReal h = dSqrt(rminSquare - l * l);
                    dReal divLbyD = l * dRecip, divHbyD = h * dRecip;
                    dReal x1 = divLbyD * ccdVec3X(&proj) + divHbyD * ccdVec3Y(&proj);
                    dReal y1 = divLbyD * ccdVec3Y(&proj) - divHbyD * ccdVec3X(&proj);
                    dReal x2 = divLbyD * ccdVec3X(&proj) - divHbyD * ccdVec3Y(&proj);
                    dReal y2 = divLbyD * ccdVec3Y(&proj) + divHbyD * ccdVec3X(&proj);
                    dReal ap1 = dAtan2(y1, x1);
                    dReal ap2 = dAtan2(y2, x2);
                    minB = dMin(ap1, ap2);
                    dReal maxB = dMax(ap1, ap2);
                    dReal a = dAtan2(ccdVec3Y(&proj), ccdVec3X(&proj));
                    if (a < minB || a > maxB) {
                        a = maxB;
                        maxB = (dReal)(minB + M_PI * 2.0);
                        minB = a;
                    }
                    diffB = maxB - minB;
                }

                // Find contact point distribution ratio based on arcs lengths
                dReal ratio = diffA * rmax  / (diffA * rmax + diffB  * rmin);
                dIASSERT(ratio <= REAL(1.0)); 
                dIASSERT(ratio >= REAL(0.0));

                int nMax = (int)dFloor(ratio * maxContacts + REAL(0.5));
                int nMin = maxContacts - nMax;
                dIASSERT(nMax <= maxContacts);

                // Make sure there is at least one point on the smaller radius rim
                if (nMin < 1) {
                    nMin = 1; nMax -= 1;
                }
                // Otherwise transfer one point to the larger radius rim as it is going to fill the rim intersection points
                else if (nMin > 1) {
                    nMin -= 1; nMax += 1;
                }

                // Smaller disc first, skipping the overlapping points
                dReal nMinRecip = 0 < nMin ? diffB / (nMin + 1) : diffB; // The 'else' value does not matter. Just try helping the optimizer.
                for (int i = 1; i <= nMin; i++) {
                    dReal depth;
                    dReal a = minB + nMinRecip * i;
                    if (testAndPrepareDiscContactForAngle(a, rmin, lmin, lSum, minCyl, maxCyl, p, depth)) {
                        contactCount = addCylCylContact(o1, o2, &maxCyl->axis, contacts, &p, normaldir, depth, contactCount, flags, skip);
                        if ((flags & CONTACTS_UNIMPORTANT) != 0) {
                            dIASSERT(contactCount != 0);
                            break;
                        }
                    }
                }

                if (contactCount == 0 || (flags & CONTACTS_UNIMPORTANT) == 0) {
                    // Then the larger disc, + additional point as the start/end points of arcs overlap
                    // (or a single contact at the arc middle point if just one is required)
                    dReal nMaxRecip = nMax > 1 ? diffA / (nMax - 1) : diffA; // The 'else' value does not matter. Just try helping the optimizer.
                    dReal adjustedMinA = nMax == 1 ? minA + REAL(0.5) * diffA : minA;

                    for (int i = 0; i < nMax; i++) {
                        dReal depth;
                        dReal a = adjustedMinA + nMaxRecip * i;
                        if (testAndPrepareDiscContactForAngle(a, rmax, lmax, lSum, maxCyl, minCyl, p, depth)) {
                            contactCount = addCylCylContact(o1, o2, &maxCyl->axis, contacts, &p, normaldir, depth, contactCount, flags, skip);
                            if ((flags & CONTACTS_UNIMPORTANT) != 0) {
                                dIASSERT(contactCount != 0);
                                break;
                            }
                        }
                    }
                }

                return contactCount;
            }
        }
    }
    return -1;
}

static 
bool testAndPrepareDiscContactForAngle(dReal angle, dReal radius, dReal length, dReal lSum, ccd_cyl_t *priCyl, ccd_cyl_t *secCyl, ccd_vec3_t &p, dReal &out_depth)
{
    bool ret = false;

    ccd_vec3_t p2;
    ccdVec3Set(&p, dCos(angle) * radius, dSin(angle) * radius, 0);
    ccdQuatRotVec(&p, &priCyl->o.rot);
    ccdVec3Add(&p, &priCyl->o.pos);
    ccdVec3Copy(&p2, &p);
    ccdVec3Sub(&p2, &secCyl->o.pos);
    dReal depth = lSum - dFabs(ccdVec3Dot(&p2, &secCyl->axis));

    if (depth >= 0) {
        ccdVec3Copy(&p2, &priCyl->axis);
        ccdVec3Scale(&p2, length);
        ccdVec3Add(&p, &p2);

        out_depth = depth;
        ret = true;
    }

    return ret;
}

static 
int addCylCylContact(dxGeom *o1, dxGeom *o2, ccd_vec3_t* axis, dContactGeom *contacts,
               ccd_vec3_t* p, dReal normaldir, dReal depth, int j, int flags, int skip)
{
    dIASSERT(depth >= 0);

    dContactGeom* contact = SAFECONTACT(flags, contacts, j, skip);
    contact->g1 = o1;
    contact->g2 = o2;
    contact->side1 = -1;
    contact->side2 = -1;
    contact->normal[0] = normaldir * ccdVec3X(axis);
    contact->normal[1] = normaldir * ccdVec3Y(axis);
    contact->normal[2] = normaldir * ccdVec3Z(axis);
    contact->depth = depth;
    contact->pos[0] = ccdVec3X(p);
    contact->pos[1] = ccdVec3Y(p);
    contact->pos[2] = ccdVec3Z(p);

    return j + 1;
}


#if dTRIMESH_ENABLED

const static float CONTACT_DEPTH_EPSILON = 0.0001f;
const static float CONTACT_POS_EPSILON = 0.0001f;
const static float CONTACT_PERTURBATION_ANGLE = 0.001f;
const static float NORMAL_PROJ_EPSILON = 0.0001f;


/*extern */
unsigned dCollideConvexTrimeshTrianglesCCD(dxGeom *o1, dxGeom *o2, const int *indices, unsigned numIndices, int flags, dContactGeom *contacts, int skip)
{
    ccd_convex_t c1;
    ccd_triangle_t c2;
    dVector3 triangle[dMTV__MAX];
    unsigned maxContacts = (flags & NUMC_MASK);
    unsigned contactCount = 0;
    ccdGeomToConvex(o1, &c1);
    ccdGeomToObj(o2, (ccd_obj_t *)&c2);

    IFaceAngleStorageView *meshFaceAngleView = dxGeomTriMeshGetFaceAngleView(o2);
    dUASSERT(meshFaceAngleView != NULL, "Please preprocess the trimesh data with dTRIDATAPREPROCESS_BUILD_FACE_ANGLES");

    for (unsigned i = 0; i != numIndices; ++i) {
        dContactGeom tempContact;
        dGeomTriMeshGetTriangle(o2, indices[i], &triangle[dMTV_FIRST], &triangle[dMTV_SECOND], &triangle[dMTV_THIRD]);

        for (unsigned j = dMTV__MIN; j != dMTV__MAX; ++j) {
            ccdVec3Set(&c2.vertices[j], (ccd_real_t)triangle[j][dV3E_X], (ccd_real_t)triangle[j][dV3E_Y], (ccd_real_t)triangle[j][dV3E_Z]);
        }

        setObjPosToTriangleCenter(&c2);

        if (ccdCollide(o1, o2, flags, &tempContact, skip, &c1, &ccdSupportConvex, &ccdCenter, &c2, &ccdSupportTriangle, &ccdCenter) == 1) {
            tempContact.side2 = i;
            
            if (meshFaceAngleView == NULL || correctTriangleContactNormal(&c2, &tempContact, meshFaceAngleView, indices, numIndices)) {
                contactCount = addUniqueContact(contacts, &tempContact, contactCount, maxContacts, flags, skip);

                if ((flags & CONTACTS_UNIMPORTANT) != 0) {
                    break;
                }
            }
        }
    }

    if ((flags & CONTACTS_UNIMPORTANT) == 0 && contactCount == 1) {
        dContactGeom *contact = SAFECONTACT(flags, contacts, 0, skip);
        dGeomTriMeshGetTriangle(o2, contact->side2, &triangle[dMTV_FIRST], &triangle[dMTV_SECOND], &triangle[dMTV_THIRD]);
        contactCount = addTrianglePerturbedContacts(o1, o2, meshFaceAngleView, indices, numIndices, flags, contacts, skip, &c1, &c2, triangle, contact, contactCount);
    }

    // Normalize accumulated normals, if necessary
    for (unsigned k = 0; k != contactCount; ) {
        dContactGeom *contact = SAFECONTACT(flags, contacts, k, skip);
        bool stayWithinThisIndex = false;

        // Only the merged contact normals need to be normalized
        if (*_const_type_cast_union<bool>(&contact->normal[dV3E_PAD])) {
        
            if (!dxSafeNormalize3(contact->normal)) {
                // If the contact normals have added up to zero, erase the contact
                // Normally the time step is to be shorter so that the objects do not get into each other that deep
                --contactCount;

                if (k != contactCount) {
                    dContactGeom *lastContact = SAFECONTACT(flags, contacts, contactCount, skip);
                    *contact = *lastContact;
                }

                stayWithinThisIndex = true;
            }
        }

        if (!stayWithinThisIndex) {
            ++k;
        }
    }

    return contactCount;
}

static 
unsigned addTrianglePerturbedContacts(dxGeom *o1, dxGeom *o2, IFaceAngleStorageView *meshFaceAngleView, 
    const int *indices, unsigned numIndices, int flags, dContactGeom *contacts, int skip,
    ccd_convex_t *c1, ccd_triangle_t *c2, dVector3 *triangle, dContactGeom *contact, unsigned contacCount)
{
    unsigned maxContacts = (flags & NUMC_MASK);
    
    dVector3 pos;
    dCopyVector3(pos, contact->pos);

    dQuaternion q1[2], q2[2];
    dReal perturbationAngle = CONTACT_PERTURBATION_ANGLE;

    dVector3 upAxis;
    bool upAvailable = false;
    if (fabs(contact->normal[dV3E_Y]) > 0.7) {
        dAssignVector3(upAxis, 0, 0, 1);
    }
    else {
        dAssignVector3(upAxis, 0, 1, 0);
    }

    dVector3 cross;
    dCalcVectorCross3(cross, contact->normal, upAxis);
    
    if (dSafeNormalize3(cross)) {
        dCalcVectorCross3(upAxis, cross, contact->normal);

        if (dSafeNormalize3(upAxis)) {
            upAvailable = true;
        }
    }

    for (unsigned j = upAvailable ? 0 : 2; j != 2; ++j) {
        dQFromAxisAndAngle(q1[j], upAxis[dV3E_X], upAxis[dV3E_Y], upAxis[dV3E_Z], perturbationAngle);
        dQFromAxisAndAngle(q2[j], cross[dV3E_X], cross[dV3E_Y], cross[dV3E_Z], perturbationAngle);
        perturbationAngle = -perturbationAngle;
    }

    for (unsigned k = upAvailable ? 0 : 4; k != 4; ++k) {
        dQuaternion qr;
        dQMultiply0(qr, q1[k % 2], q2[k / 2]);

        for (unsigned j = dMTV__MIN; j != dMTV__MAX; ++j) {
            dVector3 p, perturbed;
            dSubtractVectors3(p, triangle[j], pos);
            dQuatTransform(qr, p, perturbed);
            dAddVectors3(perturbed, perturbed, pos);

            ccdVec3Set(&c2->vertices[j], (ccd_real_t)perturbed[dV3E_X], (ccd_real_t)perturbed[dV3E_Y], (ccd_real_t)perturbed[dV3E_Z]);
        }

        dContactGeom perturbedContact;
        setObjPosToTriangleCenter(c2);

        if (ccdCollide(o1, o2, flags, &perturbedContact, skip, c1, &ccdSupportConvex, &ccdCenter, c2, &ccdSupportTriangle, &ccdCenter) == 1) {
            perturbedContact.side2 = contact->side2;
            
            if (meshFaceAngleView == NULL || correctTriangleContactNormal(c2, &perturbedContact, meshFaceAngleView, indices, numIndices)) {
                contacCount = addUniqueContact(contacts, &perturbedContact, contacCount, maxContacts, flags, skip);
            }
        }
    }

    return contacCount;
}

static 
bool correctTriangleContactNormal(ccd_triangle_t *t, dContactGeom *contact, 
    IFaceAngleStorageView *meshFaceAngleView, const int *indices, unsigned numIndices) 
{
    dIASSERT(meshFaceAngleView != NULL);

    bool anyFault = false;

    ccd_vec3_t cntOrigNormal, cntNormal;
    ccdVec3Set(&cntNormal, contact->normal[0], contact->normal[1], contact->normal[2]);
    ccdVec3Copy(&cntOrigNormal, &cntNormal);

    // Check if the contact point is located close to any edge - move it back and forth
    // and check the resulting segment for intersection with the edge plane
    ccd_vec3_t cntScaledNormal;
    ccdVec3CopyScaled(&cntScaledNormal, &cntNormal, contact->depth);

    ccd_vec3_t edges[dMTV__MAX];
    ccdVec3Sub2(&edges[dMTV_THIRD], &t->vertices[0], &t->vertices[2]);
    ccdVec3Sub2(&edges[dMTV_SECOND], &t->vertices[2], &t->vertices[1]);
    ccdVec3Sub2(&edges[dMTV_FIRST], &t->vertices[1], &t->vertices[0]);
    dSASSERT(dMTV__MAX == 3);

    bool contactGenerated = false, contactPreserved = false;
    // Triangle face normal
    ccd_vec3_t triNormal;
    ccdVec3Cross(&triNormal, &edges[dMTV_FIRST], &edges[dMTV_SECOND]);
    if (ccdVec3SafeNormalize(&triNormal) != 0) {
        anyFault = true;
    }

    // Check the edges to see if one of them is involved
    for (unsigned testEdgeIndex = !anyFault ? dMTV__MIN : dMTV__MAX; testEdgeIndex != dMTV__MAX; ++testEdgeIndex) {
        ccd_vec3_t edgeNormal, vertexToPos, v;
        ccd_vec3_t &edgeAxis = edges[testEdgeIndex]; 
        
        // Edge axis
        if (ccdVec3SafeNormalize(&edgeAxis) != 0) {
            // This should not happen normally as in the case on of edges is degenerated
            // the triangle normal calculation would have to fail above. If for some
            // reason the above calculation succeeds and this one would not, it is
            // OK to break as this point as well.
            anyFault = true;
            break;
        }
        
        // Edge Normal
        ccdVec3Cross(&edgeNormal, &edgeAxis, &triNormal);
        // ccdVec3Normalize(&edgeNormal); -- the two vectors above were already normalized and perpendicular

        // Check if the contact point is located close to any edge - move it back and forth
        // and check the resulting segment for intersection with the edge plane
        ccdVec3Set(&vertexToPos, contact->pos[0], contact->pos[1], contact->pos[2]);
        ccdVec3Sub(&vertexToPos, &t->vertices[testEdgeIndex]);
        ccdVec3Sub2(&v, &vertexToPos, &cntScaledNormal);
        
        if (ccdVec3Dot(&edgeNormal, &v) < 0) {
            ccdVec3Add2(&v, &vertexToPos, &cntScaledNormal);
            
            if (ccdVec3Dot(&edgeNormal, &v) > 0) {
                // This is an edge contact
    
                ccd_real_t x = ccdVec3Dot(&triNormal, &cntNormal);
                ccd_real_t y = ccdVec3Dot(&edgeNormal, &cntNormal);
                ccd_real_t contactNormalToTriangleNormalAngle = CCD_ATAN2(y, x);

                dReal angleValueAsDRead;
                FaceAngleDomain angleDomain = meshFaceAngleView->retrieveFacesAngleFromStorage(angleValueAsDRead, contact->side2, (dMeshTriangleVertex)testEdgeIndex);
                ccd_real_t angleValue = (ccd_real_t)angleValueAsDRead;

                ccd_real_t targetAngle;
                contactGenerated = false, contactPreserved = false; // re-assign to make optimizer's task easier

                if (angleDomain != FAD_CONCAVE) {
                    // Convex or flat - ensure the contact normal is within the allowed range
                    // formed by the two triangles' normals.
                    if (contactNormalToTriangleNormalAngle < CCD_ZERO) {
                        targetAngle = CCD_ZERO;
                    }
                    else if (contactNormalToTriangleNormalAngle > angleValue) {
                        targetAngle = angleValue;
                    }
                    else {
                        contactPreserved = true;
                    }
                }
                else {
                    // Concave - rotate the contact normal to the face angle bisect plane
                    // (or to triangle normal-edge plane if negative angles are not stored)
                    targetAngle = angleValue != 0 ? CCD_REAL(0.5) * angleValue : CCD_ZERO;
                    // There is little chance the normal will initially match the correct plane, but still, a small check could save lots of calculations
                    if (contactNormalToTriangleNormalAngle == targetAngle) {
                        contactPreserved = true;
                    }
                }

                if (!contactPreserved) {
                    ccd_quat_t q;
                    ccdQuatSetAngleAxis(&q, targetAngle - contactNormalToTriangleNormalAngle, &edgeAxis);
                    ccdQuatRotVec2(&cntNormal, &cntNormal, &q);
                    contactGenerated = true;
                }

                // Calculated successfully
                break;
            }
        }
    }

    if (!anyFault && !contactPreserved) {
        // No edge contact detected, set contact normal to triangle normal
        const ccd_vec3_t &cntNormalToUse = !contactGenerated ? triNormal : cntNormal;

        contact->normal[dV3E_X] = ccdVec3X(&cntNormalToUse);
        contact->normal[dV3E_Y] = ccdVec3Y(&cntNormalToUse);
        contact->normal[dV3E_Z] = ccdVec3Z(&cntNormalToUse);
        contact->depth *= CCD_FMAX(0.0, ccdVec3Dot(&cntOrigNormal, &cntNormalToUse));
    }

    bool result = !anyFault;
    return result;
}


static 
unsigned addUniqueContact(dContactGeom *contacts, dContactGeom *c, unsigned contactcount, unsigned maxcontacts, int flags, int skip)
{
    dReal minDepth = c->depth;
    unsigned index = contactcount;
    bool isDuplicate = false;

    dReal c_posX = c->pos[dV3E_X], c_posY = c->pos[dV3E_Y], c_posZ = c->pos[dV3E_Z];
    for (unsigned k = 0; k != contactcount; k++) {
        dContactGeom* pc = SAFECONTACT(flags, contacts, k, skip);
        
        if (fabs(c_posX - pc->pos[dV3E_X]) < CONTACT_POS_EPSILON
            && fabs(c_posY - pc->pos[dV3E_Y]) < CONTACT_POS_EPSILON
            && fabs(c_posZ - pc->pos[dV3E_Z]) < CONTACT_POS_EPSILON) {
                dSASSERT(dV3E__AXES_MAX - dV3E__AXES_MIN == 3);

                // Accumulate similar contacts
                dAddVectors3(pc->normal, pc->normal, c->normal);
                pc->depth = dMax(pc->depth, c->depth);
                *_type_cast_union<bool>(&pc->normal[dV3E_PAD]) = true; // Mark the contact as a merged one

                isDuplicate = true;
                break;
        }
        
        if (contactcount == maxcontacts && pc->depth < minDepth) {
            minDepth = pc->depth;
            index = k;
        }
    }

    if (!isDuplicate && index < maxcontacts) {
        dContactGeom* contact = SAFECONTACT(flags, contacts, index, skip);
        contact->g1 = c->g1;
        contact->g2 = c->g2;
        contact->depth = c->depth;
        contact->side1 = c->side1;
        contact->side2 = c->side2;
        dCopyVector3(contact->pos, c->pos);
        dCopyVector3(contact->normal, c->normal);
        *_type_cast_union<bool>(&contact->normal[dV3E_PAD]) = false; // Indicates whether the contact is merged or not
        contactcount = index == contactcount ? contactcount + 1 : contactcount;
    }

    return contactcount;
}

static 
void setObjPosToTriangleCenter(ccd_triangle_t *t) 
{
    ccdVec3Set(&t->o.pos, 0, 0, 0);
    for (int j = 0; j < 3; j++) {
        ccdVec3Add(&t->o.pos, &t->vertices[j]);
    }
    ccdVec3Scale(&t->o.pos, 1.0f / 3.0f);
}

static 
void ccdSupportTriangle(const void *obj, const ccd_vec3_t *_dir, ccd_vec3_t *v)
{
    const ccd_triangle_t* o = (ccd_triangle_t *) obj;
    ccd_real_t maxdot, dot;
    maxdot = -CCD_REAL_MAX;
    for (unsigned i = 0; i != 3; i++) {
        dot = ccdVec3Dot(_dir, &o->vertices[i]);
        if (dot > maxdot) {
            ccdVec3Copy(v, &o->vertices[i]);
            maxdot = dot;
        }
    }
}


#endif // dTRIMESH_ENABLED
