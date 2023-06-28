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
// TriMesh caches separation by Oleh Derevenko
// TriMesh storage classes refactoring and face angle computation code by Oleh Derevenko (C) 2016-2017


#ifndef _ODE_COLLISION_TRIMESH_INTERNAL_H_
#define _ODE_COLLISION_TRIMESH_INTERNAL_H_


//****************************************************************************
// dxTriMesh class


#include "collision_kernel.h"
#include "collision_trimesh_colliders.h"
#include "collision_util.h"
#include <ode/collision_trimesh.h>

#if dTLS_ENABLED
#include "odetls.h"
#endif


struct TrimeshCollidersCache;
struct dxTriMeshData;


static inline 
TrimeshCollidersCache *GetTrimeshCollidersCache(unsigned uiTLSKind)
{
#if dTLS_ENABLED
    EODETLSKIND tkTLSKind = (EODETLSKIND)uiTLSKind;
    return COdeTls::GetTrimeshCollidersCache(tkTLSKind);
#else // dTLS_ENABLED
    (void)uiTLSKind; // unused
    extern TrimeshCollidersCache g_ccTrimeshCollidersCache;
    return &g_ccTrimeshCollidersCache;
#endif // dTLS_ENABLED
}


enum FaceAngleStorageMethod
{
    ASM__MIN,

    ASM_BYTE_SIGNED = ASM__MIN,
    ASM_BYTE_POSITIVE,
    ASM_WORD_SIGNED,

    ASM__MAX,

    ASM__INVALID = ASM__MAX,
};

enum FaceAngleDomain
{
    FAD__MIN,

    FAD_CONCAVE = FAD__MIN,

    FAD__SIGNSTORED_IMPLICITVALUE_MIN,

    FAD_FLAT = FAD__SIGNSTORED_IMPLICITVALUE_MIN,

    FAD__SIGNSTORED_IMPLICITVALUE_MAX,

    FAD__BYTEPOS_STORED_MIN = FAD__SIGNSTORED_IMPLICITVALUE_MAX,

    FAD_CONVEX = FAD__BYTEPOS_STORED_MIN,

    FAD__BYTEPOS_STORED_MAX,

    EAD__MAX = FAD__BYTEPOS_STORED_MAX,
};

class IFaceAngleStorageControl
{
public:
    virtual void disposeStorage() = 0;

    virtual bool areNegativeAnglesStored() const = 0;

    // This is to store angles between neighbor triangle normals as positive value for convex and negative for concave edges
    virtual void assignFacesAngleIntoStorage(unsigned triangleIndex, dMeshTriangleVertex vertexIndex, dReal dAngleValue) = 0;
};

class IFaceAngleStorageView
{
public:
    virtual FaceAngleDomain retrieveFacesAngleFromStorage(dReal &out_AngleValue, unsigned triangleIndex, dMeshTriangleVertex vertexIndex) = 0;
};


typedef dBase dxTriDataBase_Parent;
struct dxTriDataBase:
    public dxTriDataBase_Parent
{
public:
    dxTriDataBase():
        dxTriDataBase_Parent(),
        m_vertices(NULL),
        m_vertexStride(0),
        m_vertexCount(0),
        m_indices(NULL),
        m_triangleCount(0),
        m_triStride(0),
        m_single(false),
        m_normals(NULL),
        m_faceAngles(NULL),
        m_faceAngleView(NULL)
    {
#if !dTRIMESH_ENABLED
        dUASSERT(false, "dTRIMESH_ENABLED is not defined. Trimesh geoms will not work");
#endif
    }

    ~dxTriDataBase();

    void buildData(const void *Vertices, int VertexStide, unsigned VertexCount, 
        const void *Indices, unsigned IndexCount, int TriStride, 
        const void *Normals, 
        bool Single);


public:
    unsigned retrieveVertexCount() const { return m_vertexCount; }
    int retrieveVertexStride() const { return m_vertexStride; }

    unsigned retrieveTriangleCount() const { return m_triangleCount; }
    int retrieveTriangleStride() const { return m_triStride; }

protected:
    const void *retrieveVertexInstances() const { return m_vertices; }
    const void *retrieveTriangleVertexIndices() const { return m_indices; }
    bool isSingle() const { return m_single; }

public:
    template<typename tcoordfloat, typename tindexint>
    static void retrieveTriangleVertexPoints(dVector3 out_Points[dMTV__MAX], unsigned triangleIndex,
        const tcoordfloat *vertexInstances, int vertexStride, const tindexint *triangleVertexIndices, int triangleStride);

public:
    void assignNormals(const void *normals) { m_normals = normals; }
    const void *retrieveNormals() const { return m_normals; }

    IFaceAngleStorageControl *retrieveFaceAngles() const { return m_faceAngles; }
    IFaceAngleStorageView *retrieveFaceAngleView() const { return m_faceAngleView; }

protected:
    bool allocateFaceAngles(FaceAngleStorageMethod storageMethod);
    void freeFaceAngles();

    bool haveFaceAnglesBeenBuilt() const { return m_faceAngles != NULL; }

public:
    enum MeshComponentUseFlags
    {
        CUF__USE_EDGES_MIN = 0x01,
        CUF_USE_FIRST_EDGE = CUF__USE_EDGES_MIN << dMTV_FIRST,
        CUF_USE_SECOND_EDGE = CUF__USE_EDGES_MIN << dMTV_SECOND,
        CUF_USE_THIRD_EDGE = CUF__USE_EDGES_MIN << dMTV_THIRD,
        CUF__USE_EDGES_MAX = CUF__USE_EDGES_MIN << dMTV__MAX,
        CUF__USE_ALL_EDGES = CUF_USE_FIRST_EDGE | CUF_USE_SECOND_EDGE | CUF_USE_THIRD_EDGE,

        CUF__USE_VERTICES_MIN = CUF__USE_EDGES_MAX,
        CUF_USE_FIRST_VERTEX = CUF__USE_VERTICES_MIN << dMTV_FIRST,
        CUF_USE_SECOND_VERTEX = CUF__USE_VERTICES_MIN << dMTV_SECOND,
        CUF_USE_THIRD_VERTEX = CUF__USE_VERTICES_MIN << dMTV_THIRD,
        CUF__USE_VERTICES_LAST = CUF__USE_VERTICES_MIN << (dMTV__MAX - 1),
        CUF__USE_VERTICES_MAX = CUF__USE_VERTICES_MIN << dMTV__MAX,
        CUF__USE_ALL_VERTICES = CUF_USE_FIRST_VERTEX | CUF_USE_SECOND_VERTEX | CUF_USE_THIRD_VERTEX,

        CUF__USE_ALL_COMPONENTS = CUF__USE_ALL_VERTICES | CUF__USE_ALL_EDGES,
    };

    // Make sure that the flags match the values declared in public interface
    dSASSERT((unsigned)CUF_USE_FIRST_EDGE == dMESHDATAUSE_EDGE1);
    dSASSERT((unsigned)CUF_USE_SECOND_EDGE == dMESHDATAUSE_EDGE2);
    dSASSERT((unsigned)CUF_USE_THIRD_EDGE == dMESHDATAUSE_EDGE3);
    dSASSERT((unsigned)CUF_USE_FIRST_VERTEX == dMESHDATAUSE_VERTEX1);
    dSASSERT((unsigned)CUF_USE_SECOND_VERTEX == dMESHDATAUSE_VERTEX2);
    dSASSERT((unsigned)CUF_USE_THIRD_VERTEX == dMESHDATAUSE_VERTEX3);

protected:
    struct EdgeRecord
    {
    public:
        void setupEdge(dMeshTriangleVertex edgeIdx, int triIdx, const unsigned vertexIndices[dMTV__MAX]);

        // Get the vertex opposite this edge in the triangle
        dMeshTriangleVertex getOppositeVertexIndex() const
        {
            extern const CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161116DC> g_VertFlagOppositeIndices;

            dMeshTriangleVertex oppositeIndex = g_VertFlagOppositeIndices.Encode(((m_vert1Flags | m_vert2Flags) ^ CUF__USE_ALL_VERTICES) / CUF__USE_VERTICES_MIN - 1);
            dIASSERT(dIN_RANGE(oppositeIndex, dMTV__MIN, dMTV__MAX));

            return oppositeIndex;
        }

        dMeshTriangleVertex getEdgeStartVertexIndex() const
        {
            extern const CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161225E9> g_VertFlagEdgeStartIndices;

            dMeshTriangleVertex startIndex = g_VertFlagEdgeStartIndices.Encode(((m_vert1Flags | m_vert2Flags) ^ CUF__USE_ALL_VERTICES) / CUF__USE_VERTICES_MIN - 1);
            dIASSERT(dIN_RANGE(startIndex, dMTV__MIN, dMTV__MAX));

            return startIndex;
        }

    public:
        bool operator <(const EdgeRecord &anotherEdge) const { return m_vertIdx1 < anotherEdge.m_vertIdx1 || (m_vertIdx1 == anotherEdge.m_vertIdx1 && m_vertIdx2 < anotherEdge.m_vertIdx2); }

    public:
        enum
        {
            AVF_VERTEX_USED             = 0x01,
            AVF_VERTEX_HAS_CONCAVE_EDGE = 0x02,
        };

    public:
        unsigned m_vertIdx1;	// Index into vertex array for this edges vertices
        unsigned m_vertIdx2;
        unsigned m_triIdx;		// Index into triangle array for triangle this edge belongs to

        uint8 m_edgeFlags;	
        uint8 m_vert1Flags;
        uint8 m_vert2Flags;
        uint8 m_absVertexFlags;
    };

    struct VertexRecord
    {
        unsigned m_UsedFromEdgeIndex;
    };

    template<class TMeshDataAccessor>
    static void meaningfulPreprocess_SetupEdgeRecords(EdgeRecord *edges, size_t numEdges, const TMeshDataAccessor &dataAccessor);
    template<class TMeshDataAccessor>
    static void meaningfulPreprocess_buildEdgeFlags(uint8 *useFlags/*=NULL*/, IFaceAngleStorageControl *faceAngles/*=NULL*/, 
        EdgeRecord *edges, size_t numEdges, VertexRecord *vertices, 
        const dReal *externalNormals, const TMeshDataAccessor &dataAccessor);
    static void buildBoundaryEdgeAngle(IFaceAngleStorageControl *faceAngles, EdgeRecord *currEdge);
    template<class TMeshDataAccessor>
    static void buildConcaveEdgeAngle(IFaceAngleStorageControl *faceAngles, bool negativeAnglesStored, 
        EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
        const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
        const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
        const TMeshDataAccessor &dataAccessor);
    template<class TMeshDataAccessor>
    static 
    void buildConvexEdgeAngle(IFaceAngleStorageControl *faceAngles, 
        EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
        const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
        const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
        const TMeshDataAccessor &dataAccessor);
    template<class TMeshDataAccessor>
    static dReal calculateEdgeAngleValidated(unsigned firstVertexStartIndex,
        EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
        const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
        const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
        const TMeshDataAccessor &dataAccessor);

private:
    const void *m_vertices;
    int m_vertexStride;
    unsigned m_vertexCount;
    const void *m_indices;
    unsigned m_triangleCount;
    int m_triStride;
    bool m_single;

private:
    const void *m_normals;
    IFaceAngleStorageControl *m_faceAngles;
    IFaceAngleStorageView *m_faceAngleView; 
};


typedef dxGeom dxMeshBase_Parent;
struct dxMeshBase:
    public dxMeshBase_Parent
{
public:
    dxMeshBase(dxSpace *Space, dxTriDataBase *Data, 
        dTriCallback *Callback, dTriArrayCallback *ArrayCallback, dTriRayCallback *RayCallback, 
        bool doTCs=false):
        dxMeshBase_Parent(Space, 1),
        m_Callback(Callback),
        m_ArrayCallback(ArrayCallback),
        m_RayCallback(RayCallback),
        m_TriMergeCallback(NULL),
        m_Data(Data)
    {
        std::fill(m_DoTCs, m_DoTCs + dARRAY_SIZE(m_DoTCs), doTCs);
        type = dTriMeshClass;
    }

    bool invokeCallback(dxGeom *Object, int TriIndex)
    {
        return m_Callback == NULL || m_Callback(this, Object, TriIndex) != 0;
    }

public:
    enum TRIMESHTC
    {
        TTC__MIN,

        TTC_SPHERE = TTC__MIN,
        TTC_BOX,
        TTC_CAPSULE,

        TTC__MAX,
    };

public:
    void assignCallback(dTriCallback *value) { m_Callback = value; }
    dTriCallback *retrieveCallback() const { return m_Callback; }

    void assignArrayCallback(dTriArrayCallback *value) { m_ArrayCallback = value; }
    dTriArrayCallback *retrieveArrayCallback() const { return m_ArrayCallback; }

    void assignRayCallback(dTriRayCallback *value) { m_RayCallback = value; }
    dTriRayCallback *retrieveRayCallback() const { return m_RayCallback; }

    void assignTriMergeCallback(dTriTriMergeCallback *value) { m_TriMergeCallback = value; }
    dTriTriMergeCallback *retrieveTriMergeCallback() const { return m_TriMergeCallback; }

    void assignMeshData(dxTriDataBase *instance)
    {
        setMeshData(instance);
        // I changed my data -- I know nothing about my own AABB anymore.
        markAABBBad();
    }
    dxTriDataBase *retrieveMeshData() const { return getMeshData(); }

    IFaceAngleStorageControl *retrieveFaceAngleStorage() const { return m_Data->retrieveFaceAngles(); }
    IFaceAngleStorageView *retrieveFaceAngleView() const { return m_Data->retrieveFaceAngleView(); }

    void assignDoTC(TRIMESHTC tc, bool value) { setDoTC(tc, value); }
    bool retrieveDoTC(TRIMESHTC tc) const { return getDoTC(tc); }

public:
    void setDoTC(TRIMESHTC tc, bool value) { dIASSERT(dIN_RANGE(tc, TTC__MIN, TTC__MAX)); m_DoTCs[tc] = value; }
    bool getDoTC(TRIMESHTC tc) const { dIASSERT(dIN_RANGE(tc, TTC__MIN, TTC__MAX)); return m_DoTCs[tc]; }

private:
    void setMeshData(dxTriDataBase *Data) { m_Data = Data; }

protected:
    dxTriDataBase *getMeshData() const { return m_Data; }

public:
    // Callbacks
    dTriCallback *m_Callback;
    dTriArrayCallback *m_ArrayCallback;
    dTriRayCallback *m_RayCallback;
    dTriTriMergeCallback *m_TriMergeCallback;

private:
    // Data types
    dxTriDataBase *m_Data;

public:
    bool m_DoTCs[TTC__MAX];
};


IFaceAngleStorageView *dxGeomTriMeshGetFaceAngleView(dxGeom *triMeshGeom);


#include "collision_trimesh_gimpact.h"
#include "collision_trimesh_opcode.h"


#endif	//_ODE_COLLISION_TRIMESH_INTERNAL_H_
