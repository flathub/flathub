/*************************************************************************
 *                                                                       *
 * Open Dynamics Engine, Copyright (C) 2001-2003 Russell L. Smith.       *
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

// TriMesh code by Erwin de Vries.
// Modified for FreeSOLID Compatibility by Rodrigo Hernandez
// Trimesh caches separation by Oleh Derevenko
// TriMesh storage classes refactoring and face angle computation code by Oleh Derevenko (C) 2016-2017


#ifndef _ODE_COLLISION_TRIMESH_GIMPACT_H_
#define _ODE_COLLISION_TRIMESH_GIMPACT_H_


#if dTRIMESH_ENABLED && dTRIMESH_GIMPACT


//****************************************************************************
// dxTriMesh class


#include "collision_kernel.h"
#include "collision_trimesh_colliders.h"
#include "collision_util.h"
#include <ode/collision_trimesh.h>

#include "collision_trimesh_internal.h"
#include <GIMPACT/gimpact.h>


struct TrimeshCollidersCache // Required for compatibility with OPCODE
{
};


typedef dxTriDataBase dxTriMeshData_Parent;
struct dxTriMeshData:
    public dxTriMeshData_Parent
{
public:
    dxTriMeshData():
        dxTriMeshData_Parent()
    {
    }

    ~dxTriMeshData() { /* Do nothing */ }

    using dxTriMeshData_Parent::buildData;
    
    /* Setup the UseFlags array and/or build face angles*/
    bool preprocessData(bool buildUseFlags/*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/);
    
private:
    bool meaningfulPreprocessData(FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/);

public:
    /* For when app changes the vertices */
    void updateData() { /* Do nothing */ }

public:
    const vec3f *retrieveVertexInstances() const { return (const vec3f *)dxTriMeshData_Parent::retrieveVertexInstances(); }
    const GUINT32 *retrieveTriangleVertexIndices() const { return (const GUINT32 *)dxTriMeshData_Parent::retrieveTriangleVertexIndices(); }

public:
    void assignNormals(const dReal *normals) { dxTriMeshData_Parent::assignNormals(normals); }
    const dReal *retrieveNormals() const { return (const dReal *)dxTriMeshData_Parent::retrieveNormals(); }
    size_t calculateNormalsMemoryRequirement() const { return retrieveTriangleCount() * (sizeof(dReal) * dSA__MAX); }
};



#ifdef dDOUBLE
// To use GIMPACT with doubles, we need to patch a couple of the GIMPACT functions to 
// convert arguments to floats before sending them in


/// Convert an gimpact vec3f to a ODE dVector3d:   dVector3[i] = vec3f[i]
#define dVECTOR3_VEC3F_COPY(b,a) { \
    (b)[0] = (a)[0];              \
    (b)[1] = (a)[1];              \
    (b)[2] = (a)[2];              \
    (b)[3] = 0;                   \
}

static inline 
void gim_trimesh_get_triangle_verticesODE(GIM_TRIMESH * trimesh, GUINT32 triangle_index, dVector3 v1, dVector3 v2, dVector3 v3)
{
    vec3f src1, src2, src3;
    GREAL *psrc1 = v1 != NULL ? src1 : NULL;
    GREAL *psrc2 = v2 != NULL ? src2 : NULL;
    GREAL *psrc3 = v3 != NULL ? src3 : NULL;
    gim_trimesh_get_triangle_vertices(trimesh, triangle_index, psrc1, psrc2, psrc3);

    if (v1 != NULL)
    {
        dVECTOR3_VEC3F_COPY(v1, src1);
    }

    if (v2 != NULL)
    {
        dVECTOR3_VEC3F_COPY(v2, src2);
    }

    if (v3 != NULL)
    {
        dVECTOR3_VEC3F_COPY(v3, src3);
    }
}

// Anything calling gim_trimesh_get_triangle_vertices from within ODE 
// should be patched through to the dDOUBLE version above

#define gim_trimesh_get_triangle_vertices gim_trimesh_get_triangle_verticesODE

static inline 
int gim_trimesh_ray_closest_collisionODE( GIM_TRIMESH *mesh, dVector3 origin, dVector3 dir, dReal tmax, GIM_TRIANGLE_RAY_CONTACT_DATA *contact )
{
    vec3f dir_vec3f    = { (GREAL)dir[ 0 ],    (GREAL)dir[ 1 ],    (GREAL)dir[ 2 ]    };
    vec3f origin_vec3f = { (GREAL)origin[ 0 ], (GREAL)origin[ 1 ], (GREAL)origin[ 2 ] };

    return gim_trimesh_ray_closest_collision( mesh, origin_vec3f, dir_vec3f, (GREAL)tmax, contact );
}

static inline 
int gim_trimesh_ray_collisionODE( GIM_TRIMESH *mesh, const dVector3 origin, const dVector3 dir, dReal tmax, GIM_TRIANGLE_RAY_CONTACT_DATA *contact )
{
    vec3f dir_vec3f    = { (GREAL)dir[ 0 ],    (GREAL)dir[ 1 ],    (GREAL)dir[ 2 ]    };
    vec3f origin_vec3f = { (GREAL)origin[ 0 ], (GREAL)origin[ 1 ], (GREAL)origin[ 2 ] };

    return gim_trimesh_ray_collision( mesh, origin_vec3f, dir_vec3f, (GREAL)tmax, contact );
}

static inline 
void gim_trimesh_sphere_collisionODE( GIM_TRIMESH *mesh, const dVector3 Position, dReal Radius, GDYNAMIC_ARRAY *contact )
{
    vec3f pos_vec3f = { (GREAL)Position[ 0 ], (GREAL)Position[ 1 ], (GREAL)Position[ 2 ] };
    gim_trimesh_sphere_collision( mesh, pos_vec3f, (GREAL)Radius, contact );
}

static inline 
void gim_trimesh_plane_collisionODE( GIM_TRIMESH *mesh, const dVector4 plane, GDYNAMIC_ARRAY *contact )
{
    vec4f plane_vec4f = { (GREAL)plane[ 0 ], (GREAL)plane[ 1 ], (GREAL)plane[ 2 ], (GREAL)plane[ 3 ] }; \
    gim_trimesh_plane_collision( mesh, plane_vec4f, contact );	    \
}

#define GIM_AABB_COPY( src, dst ) {		\
    (dst)[ 0 ]= (src) -> minX;			\
    (dst)[ 1 ]= (src) -> maxX;			\
    (dst)[ 2 ]= (src) -> minY;			\
    (dst)[ 3 ]= (src) -> maxY;			\
    (dst)[ 4 ]= (src) -> minZ;			\
    (dst)[ 5 ]= (src) -> maxZ;			\
}


#else // #ifdef !dDOUBLE

// With single precision, we can pass native ODE vectors directly to GIMPACT

#define gim_trimesh_ray_closest_collisionODE 	gim_trimesh_ray_closest_collision
#define gim_trimesh_ray_collisionODE 			gim_trimesh_ray_collision
#define gim_trimesh_sphere_collisionODE 		gim_trimesh_sphere_collision
#define gim_trimesh_plane_collisionODE 			gim_trimesh_plane_collision

#define GIM_AABB_COPY( src, dst ) 	memcpy( dst, src, 6 * sizeof( GREAL ) )


#endif // #ifdef !dDOUBLE


typedef dxMeshBase dxTriMesh_Parent;
struct dxTriMesh: 
    public dxTriMesh_Parent
{
public:
    // Functions
    dxTriMesh(dxSpace *Space, dxTriMeshData *Data,
        dTriCallback *Callback, dTriArrayCallback *ArrayCallback, dTriRayCallback *RayCallback):
        dxTriMesh_Parent(Space, NULL, Callback, ArrayCallback, RayCallback, true) // TC has speed/space 'issues' that don't make it a clear win by default on spheres/boxes.
    {
        gim_init_buffer_managers(m_buffer_managers);
        assignMeshData(Data);
    }

    ~dxTriMesh();

    void clearTCCache() { /* do nothing */ }

    virtual void computeAABB();

public:
    dxTriMeshData *retrieveMeshData() const { return getMeshData(); }

    unsigned getMeshTriangleCount() const { return gim_trimesh_get_triangle_count(const_cast<GIM_TRIMESH *>(&m_collision_trimesh)); }

    void fetchMeshTransformedTriangle(dVector3 *const pout_triangle[3], unsigned index)
    {
        gim_trimesh_locks_work_data(&m_collision_trimesh);
        gim_trimesh_get_triangle_vertices(&m_collision_trimesh, (GUINT32)index, *pout_triangle[0], *pout_triangle[1], *pout_triangle[2]);
        gim_trimesh_unlocks_work_data(&m_collision_trimesh);
    }

    void fetchMeshTransformedTriangle(dVector3 out_triangle[3], unsigned index)
    {
        gim_trimesh_locks_work_data(&m_collision_trimesh);
        gim_trimesh_get_triangle_vertices(&m_collision_trimesh, (GUINT32)index, out_triangle[0], out_triangle[1], out_triangle[2]);
        gim_trimesh_unlocks_work_data(&m_collision_trimesh);
    }

private:
    dxTriMeshData *getMeshData() const { return static_cast<dxTriMeshData *>(dxTriMesh_Parent::getMeshData()); }

public:
    enum
    {
        VERTEXINSTANCE_STRIDE = sizeof(vec3f),
        TRIANGLEINDEX_STRIDE = sizeof(GUINT32) * dMTV__MAX,
    };

    void assignMeshData(dxTriMeshData *Data);

public:
    GIM_TRIMESH  m_collision_trimesh;
    GBUFFER_MANAGER_DATA m_buffer_managers[G_BUFFER_MANAGER__MAX];
};


static inline 
void MakeMatrix(const dVector3 position, const dMatrix3 rotation, mat4f m)
{
    m[0][0] = (GREAL)rotation[dM3E_XX];
    m[0][1] = (GREAL)rotation[dM3E_XY];
    m[0][2] = (GREAL)rotation[dM3E_XZ];

    m[1][0] = (GREAL)rotation[dM3E_YX];
    m[1][1] = (GREAL)rotation[dM3E_YY];
    m[1][2] = (GREAL)rotation[dM3E_YZ];

    m[2][0] = (GREAL)rotation[dM3E_ZX];
    m[2][1] = (GREAL)rotation[dM3E_ZY];
    m[2][2] = (GREAL)rotation[dM3E_ZZ];

    m[0][3] = (GREAL)position[dV3E_X];
    m[1][3] = (GREAL)position[dV3E_Y];
    m[2][3] = (GREAL)position[dV3E_Z];
}

static inline 
void MakeMatrix(dxGeom *g, mat4f m)
{
    const dVector3 &position = g->buildUpdatedPosition();
    const dMatrix3 &rotation = g->buildUpdatedRotation();
    MakeMatrix(position, rotation, m);
}


#endif // #if dTRIMESH_ENABLED && dTRIMESH_GIMPACT

#endif	//_ODE_COLLISION_TRIMESH_GIMPACT_H_
