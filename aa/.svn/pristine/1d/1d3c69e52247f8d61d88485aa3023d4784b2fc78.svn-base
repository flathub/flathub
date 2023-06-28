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


#ifndef _ODE_COLLISION_TRIMESH_OPCODE_H_
#define _ODE_COLLISION_TRIMESH_OPCODE_H_


#if dTRIMESH_ENABLED && dTRIMESH_OPCODE

//****************************************************************************
// dxTriMesh class


#include "collision_kernel.h"
#include "collision_trimesh_colliders.h"
#include "collision_util.h"
#include <ode/collision_trimesh.h>

#include "collision_trimesh_internal.h"

#define BAN_OPCODE_AUTOLINK
#include "Opcode.h"
using namespace Opcode;

#include "util.h"


#if !dTRIMESH_OPCODE_USE_OLD_TRIMESH_TRIMESH_COLLIDER

// New trimesh collider hash table types
enum
{
    MAXCONTACT_X_NODE = 4,
    CONTACTS_HASHSIZE = 256
};

struct CONTACT_KEY
{
    dContactGeom * m_contact;
    unsigned int m_key;
};

struct CONTACT_KEY_HASH_NODE
{
    CONTACT_KEY m_keyarray[MAXCONTACT_X_NODE];
    int m_keycount;
};

struct CONTACT_KEY_HASH_TABLE
{
public:
    CONTACT_KEY_HASH_NODE &operator[](unsigned int index) { return m_storage[index]; }

private:
    CONTACT_KEY_HASH_NODE m_storage[CONTACTS_HASHSIZE];
};

#endif // !dTRIMESH_OPCODE_USE_OLD_TRIMESH_TRIMESH_COLLIDER


struct VertexUseCache
{
public:
    VertexUseCache(): m_VertexUseBits(NULL), m_VertexUseElements(0) {}
    ~VertexUseCache() { freeVertexUSEDFlags();  }

    bool resizeAndResetVertexUSEDFlags(unsigned VertexCount)
    {
        bool Result = false;
        size_t VertexNewElements = (VertexCount + 7) / 8;
        if (VertexNewElements <= m_VertexUseElements || reallocVertexUSEDFlags(VertexNewElements)) {
            memset(m_VertexUseBits, 0, VertexNewElements);
            Result = true;
        }
        return Result;
    }

    bool getVertexUSEDFlag(unsigned VertexIndex) const { return (m_VertexUseBits[VertexIndex / 8] & (1 << (VertexIndex % 8))) != 0; }
    void setVertexUSEDFlag(unsigned VertexIndex) { m_VertexUseBits[VertexIndex / 8] |= (1 << (VertexIndex % 8)); }

private:
    bool reallocVertexUSEDFlags(size_t VertexNewElements)
    {
        bool Result = false;
        uint8 *VertexNewBits = (uint8 *)dRealloc(m_VertexUseBits, m_VertexUseElements * sizeof(m_VertexUseBits[0]), VertexNewElements * sizeof(m_VertexUseBits[0]));
        if (VertexNewBits) {
            m_VertexUseBits = VertexNewBits;
            m_VertexUseElements = VertexNewElements;
            Result = true;
        }
        return Result;
    }

    void freeVertexUSEDFlags()
    {
        dFree(m_VertexUseBits, m_VertexUseElements * sizeof(m_VertexUseBits[0]));
        m_VertexUseBits = NULL;
        m_VertexUseElements = 0;
    }

private:
    uint8 *m_VertexUseBits;
    size_t m_VertexUseElements;
};


struct TrimeshCollidersCache
{
    TrimeshCollidersCache()
    {
        initOPCODECaches();
    }

    void initOPCODECaches();
    void clearOPCODECaches();

    // Collider caches
    BVTCache ColCache;

#if !dTRIMESH_OPCODE_USE_OLD_TRIMESH_TRIMESH_COLLIDER
    CONTACT_KEY_HASH_TABLE m_hashcontactset;
#endif

    // Colliders
    /* -- not used -- also uncomment in InitOPCODECaches()
    PlanesCollider _PlanesCollider; -- not used 
    */
    SphereCollider m_SphereCollider;
    OBBCollider m_OBBCollider;
    RayCollider m_RayCollider;
    AABBTreeCollider m_AABBTreeCollider;
    /* -- not used -- also uncomment in InitOPCODECaches()
    LSSCollider _LSSCollider;
    */
    // Trimesh caches
    CollisionFaces m_Faces;
    SphereCache m_DefaultSphereCache;
    OBBCache m_DefaultBoxCache;
    LSSCache m_DefaultCapsuleCache;

    // Trimesh-plane collision vertex use cache
    VertexUseCache m_VertexUses;
};


typedef dxTriDataBase dxTriMeshData_Parent;
struct dxTriMeshData:
    public dxTriMeshData_Parent
{
public:
    dxTriMeshData():
        dxTriMeshData_Parent(),
        m_ExternalUseFlags(NULL),
        m_InternalUseFlags(NULL)
    {
    }

    ~dxTriMeshData();

    void buildData(const Point *Vertices, int VertexStide, unsigned VertexCount,
        const IndexedTriangle *Indices, unsigned IndexCount, int TriStride,
        const dReal *in_Normals,
        bool Single);

private:
    void calculateDataAABB(dVector3 &AABBMax, dVector3 &AABBMin);
    template<typename treal>
    void templateCalculateDataAABB(dVector3 &AABBMax, dVector3 &AABBMin);

public:
    /* Setup the UseFlags array and/or build face angles*/
    bool preprocessData(bool buildUseFlags/*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/);

private:
    bool meaningfulPreprocessData(bool buildUseFlags/*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/);

public:
    /* For when app changes the vertices */
    void updateData();

public:
    const Point *retrieveVertexInstances() const { return (const Point *)dxTriMeshData_Parent::retrieveVertexInstances(); }

public:
    void assignNormals(const dReal *normals) { dxTriMeshData_Parent::assignNormals(normals); }
    const dReal *retrieveNormals() const { return (const dReal *)dxTriMeshData_Parent::retrieveNormals(); }
    size_t calculateNormalsMemoryRequirement() const { return retrieveTriangleCount() * (sizeof(dReal) * dSA__MAX); }

public:
    void assignExternalUseFlagsBuffer(uint8 *buffer) { m_ExternalUseFlags = buffer != m_InternalUseFlags ? buffer : NULL; }
    const uint8 *smartRetrieveUseFlags() const { return m_ExternalUseFlags != NULL ? m_ExternalUseFlags : m_InternalUseFlags; }
    bool haveUseFlagsBeenBuilt() const { return m_InternalUseFlags != NULL; }
    size_t calculateUseFlagsMemoryRequirement() const { return m_Mesh.GetNbTriangles() * sizeof(m_InternalUseFlags[0]); }

public:
    Model m_BVTree;
    MeshInterface m_Mesh;

    /* aabb in model space */
    dVector3 m_AABBCenter;
    dVector3 m_AABBExtents;

    // data for use in collision resolution
    uint8 *m_ExternalUseFlags;
    uint8 *m_InternalUseFlags;

};


typedef dxMeshBase dxTriMesh_Parent;
struct dxTriMesh: 
    public dxTriMesh_Parent
{
public:
    // Functions
    dxTriMesh(dxSpace *Space, dxTriMeshData *Data, 
        dTriCallback *Callback, dTriArrayCallback *ArrayCallback, dTriRayCallback *RayCallback):
        dxTriMesh_Parent(Space, Data, Callback, ArrayCallback, RayCallback, false)
    {
        m_SphereContactsMergeOption = (dxContactMergeOptions)MERGE_NORMALS__SPHERE_DEFAULT;

        dZeroMatrix4(m_last_trans);
    }

    ~dxTriMesh();

    void clearTCCache();

    bool controlGeometry(int controlClass, int controlCode, void *dataValue, int *dataSize);

    virtual void computeAABB();

public:
    dxTriMeshData *retrieveMeshData() const { return getMeshData(); }
    const dReal *retrieveMeshNormals() const { return getMeshData()->retrieveNormals(); }
    Model &retrieveMeshBVTreeRef() const { return getMeshData()->m_BVTree; }
    const uint8 *retrieveMeshSmartUseFlags() const { return getMeshData()->smartRetrieveUseFlags(); }

    unsigned getMeshTriangleCount() const { return getMeshData()->m_Mesh.GetNbTriangles(); }
    void fetchMeshTransformedTriangle(dVector3 *const pout_triangle[3], unsigned index)/* const*/;
    void fetchMeshTransformedTriangle(dVector3 out_triangle[3], unsigned index)/* const*/;
    void fetchMeshTriangle(dVector3 *const pout_triangle[3], unsigned index, const dVector3 position, const dMatrix3 rotation) const;
    void fetchMeshTriangle(dVector3 out_triangle[3], unsigned index, const dVector3 position, const dMatrix3 rotation) const;

public:
    void assignLastTransform(const dMatrix4 last_trans) { dCopyMatrix4x4(m_last_trans, last_trans); }
    const dReal *retrieveLastTransform() const { return m_last_trans; }

private:
    enum
    {
        MERGE_NORMALS__SPHERE_DEFAULT = DONT_MERGE_CONTACTS
    };

    bool controlGeometry_SetMergeSphereContacts(int dataValue);
    bool controlGeometry_GetMergeSphereContacts(int &returnValue);

private:
    dxTriMeshData *getMeshData() const { return static_cast<dxTriMeshData *>(dxTriMesh_Parent::getMeshData()); }

public:
    // Some constants
    // Temporal coherence
    struct SphereTC : public SphereCache{
        dxGeom* Geom;
    };

    struct BoxTC : public OBBCache{
        dxGeom* Geom;
    };

    struct CapsuleTC : public LSSCache{
        dxGeom* Geom;
    };

public:
    // Contact merging option
    dxContactMergeOptions m_SphereContactsMergeOption;
    // Instance data for last transform.
    dMatrix4 m_last_trans;

    dArray<SphereTC> m_SphereTCCache;
    dArray<BoxTC> m_BoxTCCache;
    dArray<CapsuleTC> m_CapsuleTCCache;
};


static inline 
Matrix4x4 &MakeMatrix(const dVector3 Position, const dMatrix3 Rotation, Matrix4x4 &Out)
{
    return Out.Set(
        Rotation[0], Rotation[4], Rotation[8], 0.0f,
        Rotation[1], Rotation[5], Rotation[9], 0.0f,
        Rotation[2], Rotation[6], Rotation[10],0.0f,
        Position[0], Position[1], Position[2], 1.0f);
}

static inline 
Matrix4x4 &MakeMatrix(dxGeom* g, Matrix4x4 &Out)
{
    const dVector3 &position = g->buildUpdatedPosition();
    const dMatrix3 &rotation = g->buildUpdatedRotation();
    return MakeMatrix(position, rotation, Out);
}


#endif // #if dTRIMESH_ENABLED && dTRIMESH_OPCODE


#endif	//_ODE_COLLISION_TRIMESH_OPCODE_H_
