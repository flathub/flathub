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
// TriMesh storage classes refactoring and face angle computation code by Oleh Derevenko (C) 2016-2017

#include <ode/collision.h>
#include <ode/rotation.h>
#include "config.h"
#include "matrix.h"
#include "odemath.h"


#if dTRIMESH_ENABLED && dTRIMESH_OPCODE

#include "collision_util.h"
#include "collision_trimesh_opcode.h"
#include "collision_trimesh_internal_impl.h"
#include <algorithm>


//////////////////////////////////////////////////////////////////////////
// TrimeshCollidersCache

void TrimeshCollidersCache::initOPCODECaches()
{
    m_RayCollider.SetDestination(&m_Faces);

    /* -- not used
    _PlanesCollider.SetTemporalCoherence(true);
    */

    m_SphereCollider.SetTemporalCoherence(true);
    m_SphereCollider.SetPrimitiveTests(false);

    m_OBBCollider.SetTemporalCoherence(true);

    // no first-contact test (i.e. return full contact info)
    m_AABBTreeCollider.SetFirstContact( false );     
    // temporal coherence only works with "first contact" tests
    m_AABBTreeCollider.SetTemporalCoherence(false);
    // Perform full BV-BV tests (true) or SAT-lite tests (false)
    m_AABBTreeCollider.SetFullBoxBoxTest( true );
    // Perform full Primitive-BV tests (true) or SAT-lite tests (false)
    m_AABBTreeCollider.SetFullPrimBoxTest( true );
    const char* msg;
    if ((msg =m_AABBTreeCollider.ValidateSettings()))
    {
        dDebug (d_ERR_UASSERT, msg, " (%s:%d)", __FILE__,__LINE__);
    }

    /* -- not used
    _LSSCollider.SetTemporalCoherence(false);
    _LSSCollider.SetPrimitiveTests(false);
    _LSSCollider.SetFirstContact(false);
    */
}

void TrimeshCollidersCache::clearOPCODECaches()
{
    m_Faces.Empty();
    m_DefaultSphereCache.TouchedPrimitives.Empty();
    m_DefaultBoxCache.TouchedPrimitives.Empty();
    m_DefaultCapsuleCache.TouchedPrimitives.Empty();
}


//////////////////////////////////////////////////////////////////////////
// Trimesh data

dxTriMeshData::~dxTriMeshData()
{
    if ( m_InternalUseFlags != NULL )
    {
        size_t flagsMemoryRequired = calculateUseFlagsMemoryRequirement();
        dFree(m_InternalUseFlags, flagsMemoryRequired);
    }
}

void dxTriMeshData::buildData(const Point *Vertices, int VertexStide, unsigned VertexCount,
    const IndexedTriangle *Indices, unsigned IndexCount, int TriStride,
    const dReal *in_Normals,
    bool Single)
{
    dxTriMeshData_Parent::buildData(Vertices, VertexStide, VertexCount, Indices, IndexCount, TriStride, in_Normals, Single);
    dAASSERT(IndexCount % dMTV__MAX == 0);

    m_Mesh.SetNbTriangles(IndexCount / dMTV__MAX);
    m_Mesh.SetNbVertices(VertexCount);
    m_Mesh.SetPointers(Indices, Vertices);
    m_Mesh.SetStrides(TriStride, VertexStide);
    m_Mesh.SetSingle(Single);

    // Build tree
    // recommended in Opcode User Manual
    //Settings.mRules = SPLIT_COMPLETE | SPLIT_SPLATTERPOINTS | SPLIT_GEOMCENTER;
    // used in ODE, why?
    //Settings.mRules = SPLIT_BEST_AXIS;
    // best compromise?
    BuildSettings Settings(SPLIT_BEST_AXIS | SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER);

    OPCODECREATE TreeBuilder(&m_Mesh, Settings, true, false);

    m_BVTree.Build(TreeBuilder);

    // compute model space AABB
    dVector3 AABBMax, AABBMin;
    calculateDataAABB(AABBMax, AABBMin);

    dAddVectors3(m_AABBCenter, AABBMin, AABBMax);
    dScaleVector3(m_AABBCenter, REAL(0.5));

    dSubtractVectors3(m_AABBExtents, AABBMax, m_AABBCenter);

    // user data (not used by OPCODE)
    dIASSERT(m_InternalUseFlags == NULL);
}


void dxTriMeshData::calculateDataAABB(dVector3 &AABBMax, dVector3 &AABBMin)
{
    if (isSingle()) 
    {
        templateCalculateDataAABB<float>(AABBMax, AABBMin);
    } 
    else 
    {
        templateCalculateDataAABB<double>(AABBMax, AABBMin);
    }
}

template<typename treal>
void dxTriMeshData::templateCalculateDataAABB(dVector3 &AABBMax, dVector3 &AABBMin)
{
    dIASSERT(isSingle() == (sizeof(treal) == sizeof(float)));

    const Point *vertices = retrieveVertexInstances();
    const int vertexStide = retrieveVertexStride();
    const unsigned vertexCount = retrieveVertexCount();

    AABBMax[dV3E_X] = AABBMax[dV3E_Y] = AABBMax[dV3E_Z] = -dInfinity;
    AABBMin[dV3E_X] = AABBMin[dV3E_Y] = AABBMin[dV3E_Z] = dInfinity;
    dSASSERT(dV3E__AXES_COUNT == 3);

    const uint8 *verts = (const uint8 *)vertices;
    for( unsigned i = 0; i < vertexCount; ++i ) 
    {
        const treal *v = (const treal *)verts;
        if( v[dSA_X] > AABBMax[dV3E_X] ) AABBMax[dV3E_X] = (dReal)v[dSA_X];
        if( v[dSA_X] < AABBMin[dV3E_X] ) AABBMin[dV3E_X] = (dReal)v[dSA_X];
        if( v[dSA_Y] > AABBMax[dV3E_Y] ) AABBMax[dV3E_Y] = (dReal)v[dSA_Y];
        if( v[dSA_Y] < AABBMin[dV3E_Y] ) AABBMin[dV3E_Y] = (dReal)v[dSA_Y];
        if( v[dSA_Z] > AABBMax[dV3E_Z] ) AABBMax[dV3E_Z] = (dReal)v[dSA_Z];
        if( v[dSA_Z] < AABBMin[dV3E_Z] ) AABBMin[dV3E_Z] = (dReal)v[dSA_Z];
        verts += vertexStide;
    }
}


bool dxTriMeshData::preprocessData(bool buildUseFlags/*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/)
{
    bool buildUseFlagsToUse = buildUseFlags;
    FaceAngleStorageMethod faceAndgesRequirementToUse = faceAndgesRequirement;

    if (buildUseFlags && haveUseFlagsBeenBuilt())
    {
        dUASSERT(false, "Another request to build edge/vertex use flags after they had already been built");

        buildUseFlagsToUse = false;
    }

    if (faceAndgesRequirement != ASM__INVALID && haveFaceAnglesBeenBuilt())
    {
        dUASSERT(false, "Another request to build face angles after they had already been built");

        faceAndgesRequirementToUse = ASM__INVALID;
    }

    // If this mesh has already been preprocessed, exit
    bool result = (!buildUseFlagsToUse && faceAndgesRequirementToUse == ASM__INVALID) || m_Mesh.GetNbTriangles() == 0 
        || meaningfulPreprocessData(buildUseFlagsToUse, faceAndgesRequirementToUse);
    return result;
}

struct TrimeshDataVertexIndexAccessor_OPCODE
{
    TrimeshDataVertexIndexAccessor_OPCODE(const IndexedTriangle *triIndicesBegin, unsigned triStride):
        m_TriIndicesBegin(triIndicesBegin),
        m_TriStride(triStride)
    {
    }

    void getTriangleVertexIndices(unsigned out_VertexIndices[dMTV__MAX], unsigned triangleIdx) const
    {
        const IndexedTriangle *triIndicesBegin = m_TriIndicesBegin;
        const unsigned triStride = m_TriStride;

        const IndexedTriangle *triIndicesOfInterest = (const IndexedTriangle *)((const uint8 *)triIndicesBegin + triangleIdx * (size_t)triStride);
        std::copy(triIndicesOfInterest->mVRef, triIndicesOfInterest->mVRef + dMTV__MAX, out_VertexIndices);
        dSASSERT(dMTV__MAX == dARRAY_SIZE(triIndicesOfInterest->mVRef));
        dSASSERT(dMTV_FIRST == 0);
        dSASSERT(dMTV_SECOND == 1);
        dSASSERT(dMTV_THIRD == 2);
        dSASSERT(dMTV__MAX == 3);
    }


    const IndexedTriangle   *m_TriIndicesBegin;
    unsigned                m_TriStride;
};

struct TrimeshDataTrianglePointAccessor_OPCODE
{
    TrimeshDataTrianglePointAccessor_OPCODE(const MeshInterface &mesh):
        m_Mesh(mesh)
    {
    }

    void getTriangleVertexPoints(dVector3 out_Points[dMTV__MAX], unsigned triangleIndex) const
    {
        VertexPointers vpTriangle;
        ConversionArea vc;
        m_Mesh.GetTriangle(vpTriangle, triangleIndex, vc);

        for (unsigned pointIndex = 0; pointIndex != 3; ++pointIndex)
        {
            dAssignVector3(out_Points[pointIndex], vpTriangle.Vertex[pointIndex]->x, vpTriangle.Vertex[pointIndex]->y, vpTriangle.Vertex[pointIndex]->z);
        }
        dSASSERT(dMTV_FIRST == 0);
        dSASSERT(dMTV_SECOND == 1);
        dSASSERT(dMTV_THIRD == 2);
        dSASSERT(dMTV__MAX == 3);
    }

    const MeshInterface     &m_Mesh;
};

bool dxTriMeshData::meaningfulPreprocessData(bool buildUseFlags/*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/)
{
    const bool buildFaceAngles = faceAndgesRequirement != ASM__INVALID;
    dIASSERT(buildUseFlags || buildFaceAngles);
    dIASSERT(!buildUseFlags || !haveUseFlagsBeenBuilt());
    dIASSERT(!buildFaceAngles || !haveFaceAnglesBeenBuilt());

    bool result = false;

    uint8 *useFlags = NULL;
    size_t flagsMemoryRequired = 0;
    bool flagsAllocated = false, anglesAllocated = false;

    do 
    {
        if (buildUseFlags)
        {
            flagsMemoryRequired = calculateUseFlagsMemoryRequirement();
            useFlags = (uint8 *)dAlloc(flagsMemoryRequired);

            if (useFlags == NULL)
            {
                break;
            }
        }

        flagsAllocated = true;

        if (buildFaceAngles)
        {
            if (!allocateFaceAngles(faceAndgesRequirement))
            {
                break;
            }
        }

        anglesAllocated = true;

        const unsigned int numTris = m_Mesh.GetNbTriangles();
        const unsigned int numVertices = m_Mesh.GetNbVertices();
        size_t numEdges = (size_t)numTris * dMTV__MAX;
        dIASSERT(numVertices <= numEdges); // Edge records are going to be used for vertex data as well

        const size_t recordsMemoryRequired = dEFFICIENT_SIZE(numEdges * sizeof(EdgeRecord));
        const size_t verticesMemoryRequired = /*dEFFICIENT_SIZE*/(numVertices * sizeof(VertexRecord)); // Skip alignment for the last chunk
        const size_t totalTempMemoryRequired = recordsMemoryRequired + verticesMemoryRequired;
        void *tempBuffer = dAlloc(totalTempMemoryRequired);
        
        if (tempBuffer == NULL)
        {
            break;
        }

        EdgeRecord *edges = (EdgeRecord *)tempBuffer;
        VertexRecord *vertices = (VertexRecord *)((uint8 *)tempBuffer + recordsMemoryRequired);

        // Delay zero-filling until all the allocations succeed
        if (useFlags != NULL)
        {
            memset(useFlags, 0, flagsMemoryRequired);
        }

        const IndexedTriangle *triIndicesBegin = m_Mesh.GetTris();
        unsigned triStride = m_Mesh.GetTriStride();
        TrimeshDataVertexIndexAccessor_OPCODE indexAccessor(triIndicesBegin, triStride);
        meaningfulPreprocess_SetupEdgeRecords(edges, numEdges, indexAccessor);

        // Sort the edges, so the ones sharing the same verts are beside each other
        std::sort(edges, edges + numEdges);

        TrimeshDataTrianglePointAccessor_OPCODE pointAccessor(m_Mesh);
        const dReal *const externalNormals = retrieveNormals();
        IFaceAngleStorageControl *faceAngles = retrieveFaceAngles();
        meaningfulPreprocess_buildEdgeFlags(useFlags, faceAngles, edges, numEdges, vertices, externalNormals, pointAccessor);

        dFree(tempBuffer, totalTempMemoryRequired);
    	
        if (buildUseFlags)
        {
            m_InternalUseFlags = useFlags;
        }

        result = true;
    }
    while (false);

    if (!result)
    {
        if (flagsAllocated)
        {
            if (anglesAllocated)
            {
                if (buildFaceAngles)
                {
                    freeFaceAngles();
                }
            }

            if (buildUseFlags)
            {
                dFree(useFlags, flagsMemoryRequired);
            }
        }
    }

    return result;
}


void dxTriMeshData::updateData()
{
    m_BVTree.Refit();
}



//////////////////////////////////////////////////////////////////////////
// dxTriMesh

dxTriMesh::~dxTriMesh()
{
    //
}

void dxTriMesh::clearTCCache()
{
    /* dxTriMesh::ClearTCCache uses dArray's setSize(0) to clear the caches -
    but the destructor isn't called when doing this, so we would leak.
    So, call the previous caches' containers' destructors by hand first. */
    int i, n;

    n = m_SphereTCCache.size();
    for( i = 0; i != n; ++i ) 
    {
        m_SphereTCCache[i].~SphereTC();
    }
    m_SphereTCCache.setSize(0);

    n = m_BoxTCCache.size();
    for( i = 0; i != n; ++i ) 
    {
        m_BoxTCCache[i].~BoxTC();
    }
    m_BoxTCCache.setSize(0);

    n = m_CapsuleTCCache.size();
    for( i = 0; i != n; ++i ) 
    {
        m_CapsuleTCCache[i].~CapsuleTC();
    }
    m_CapsuleTCCache.setSize(0);
}


bool dxTriMesh::controlGeometry(int controlClass, int controlCode, void *dataValue, int *dataSize)
{
    if (controlClass == dGeomColliderControlClass) 
    {
        if (controlCode == dGeomCommonAnyControlCode) 
        {
            return checkControlValueSizeValidity(dataValue, dataSize, 0);
        }
        else if (controlCode == dGeomColliderSetMergeSphereContactsControlCode) 
        {
            return checkControlValueSizeValidity(dataValue, dataSize, sizeof(int)) 
                && controlGeometry_SetMergeSphereContacts(*(int *)dataValue);
        }
        else if (controlCode == dGeomColliderGetMergeSphereContactsControlCode) 
        {
            return checkControlValueSizeValidity(dataValue, dataSize, sizeof(int)) 
                && controlGeometry_GetMergeSphereContacts(*(int *)dataValue);
        }
    }

    return dxTriMesh_Parent::controlGeometry(controlClass, controlCode, dataValue, dataSize);
}

bool dxTriMesh::controlGeometry_SetMergeSphereContacts(int dataValue)
{
    if (dataValue == dGeomColliderMergeContactsValue__Default) 
    {
        m_SphereContactsMergeOption = (dxContactMergeOptions)MERGE_NORMALS__SPHERE_DEFAULT;
    }
    else if (dataValue == dGeomColliderMergeContactsValue_None) 
    {
        m_SphereContactsMergeOption = DONT_MERGE_CONTACTS;
    }
    else if (dataValue == dGeomColliderMergeContactsValue_Normals) 
    {
        m_SphereContactsMergeOption = MERGE_CONTACT_NORMALS;
    }
    else if (dataValue == dGeomColliderMergeContactsValue_Full) 
    {
        m_SphereContactsMergeOption = MERGE_CONTACTS_FULLY;
    }
    else 
    {
        dAASSERT(false && "Invalid contact merge control value");
        return false;
    }

    return true;
}

bool dxTriMesh::controlGeometry_GetMergeSphereContacts(int &returnValue)
{
    if (m_SphereContactsMergeOption == DONT_MERGE_CONTACTS) {
        returnValue = dGeomColliderMergeContactsValue_None;
    }
    else if (m_SphereContactsMergeOption == MERGE_CONTACT_NORMALS) {
        returnValue = dGeomColliderMergeContactsValue_Normals;
    }
    else if (m_SphereContactsMergeOption == MERGE_CONTACTS_FULLY) {
        returnValue = dGeomColliderMergeContactsValue_Full;
    }
    else {
        dIASSERT(false && "Internal error: unexpected contact merge option field value");
        return false;
    }

    return true;
}


/*virtual */
void dxTriMesh::computeAABB() 
{
    const dxTriMeshData *meshData = getMeshData();
    dVector3 c;
    const dMatrix3& R = final_posr->R;
    const dVector3& pos = final_posr->pos;

    dMultiply0_331( c, R, meshData->m_AABBCenter );

    dReal xrange = dFabs(R[0] * meshData->m_AABBExtents[0]) +
        dFabs(R[1] * meshData->m_AABBExtents[1]) + 
        dFabs(R[2] * meshData->m_AABBExtents[2]);
    dReal yrange = dFabs(R[4] * meshData->m_AABBExtents[0]) +
        dFabs(R[5] * meshData->m_AABBExtents[1]) + 
        dFabs(R[6] * meshData->m_AABBExtents[2]);
    dReal zrange = dFabs(R[8] * meshData->m_AABBExtents[0]) +
        dFabs(R[9] * meshData->m_AABBExtents[1]) + 
        dFabs(R[10] * meshData->m_AABBExtents[2]);

    aabb[0] = c[0] + pos[0] - xrange;
    aabb[1] = c[0] + pos[0] + xrange;
    aabb[2] = c[1] + pos[1] - yrange;
    aabb[3] = c[1] + pos[1] + yrange;
    aabb[4] = c[2] + pos[2] - zrange;
    aabb[5] = c[2] + pos[2] + zrange;
}


void dxTriMesh::fetchMeshTransformedTriangle(dVector3 *const pout_triangle[3], unsigned index)
{
    const dVector3 &position = buildUpdatedPosition();
    const dMatrix3 &rotation = buildUpdatedRotation();
    fetchMeshTriangle(pout_triangle, index, position, rotation);
}

void dxTriMesh::fetchMeshTransformedTriangle(dVector3 out_triangle[3], unsigned index)
{
    const dVector3 &position = buildUpdatedPosition();
    const dMatrix3 &rotation = buildUpdatedRotation();
    fetchMeshTriangle(out_triangle, index, position, rotation);
}

void dxTriMesh::fetchMeshTriangle(dVector3 *const pout_triangle[3], unsigned index, const dVector3 position, const dMatrix3 rotation) const
{
    dIASSERT(dIN_RANGE(index, 0, getMeshTriangleCount()));

    VertexPointers VP;
    ConversionArea VC;

    const dxTriMeshData *meshData = getMeshData();
    meshData->m_Mesh.GetTriangle(VP, index, VC);

    for (unsigned i = 0; i != 3; ++i)
    {
        if (pout_triangle[i] != NULL)
        {
            dVector3 v;
            v[dV3E_X] = VP.Vertex[i]->x;
            v[dV3E_Y] = VP.Vertex[i]->y;
            v[dV3E_Z] = VP.Vertex[i]->z;

            dVector3 &out_triangle = *(pout_triangle[i]);
            dMultiply0_331(out_triangle, rotation, v);
            dAddVectors3(out_triangle, out_triangle, position);
            out_triangle[dV3E_PAD] = REAL(0.0);
        }
    }
}

void dxTriMesh::fetchMeshTriangle(dVector3 out_triangle[3], unsigned index, const dVector3 position, const dMatrix3 rotation) const
{
    dIASSERT(dIN_RANGE(index, 0, getMeshTriangleCount()));

    VertexPointers VP;
    ConversionArea VC;

    const dxTriMeshData *meshData = getMeshData();
    meshData->m_Mesh.GetTriangle(VP, index, VC);

    for (unsigned i = 0; i != 3; ++i)
    {
        dVector3 v;
        v[dV3E_X] = VP.Vertex[i]->x;
        v[dV3E_Y] = VP.Vertex[i]->y;
        v[dV3E_Z] = VP.Vertex[i]->z;

        dMultiply0_331(out_triangle[i], rotation, v);
        dAddVectors3(out_triangle[i], out_triangle[i], position);
        out_triangle[i][dV3E_PAD] = REAL(0.0);
    }
}


//////////////////////////////////////////////////////////////////////////

/*extern */
dTriMeshDataID dGeomTriMeshDataCreate()
{
    return new dxTriMeshData();
}

/*extern */
void dGeomTriMeshDataDestroy(dTriMeshDataID g)
{
    dxTriMeshData *mesh = g;
    delete mesh;
}


/*extern */
void dGeomTriMeshDataSet(dTriMeshDataID g, int dataId, void *pDataLocation)
{
    dUASSERT(g, "The argument is not a trimesh data");

    dxTriMeshData *data = g;

    switch (dataId)
    {
        case dTRIMESHDATA_FACE_NORMALS:
        {
            data->assignNormals((const dReal *)pDataLocation);
            break;
        }

        case dTRIMESHDATA_USE_FLAGS:
        {
            data->assignExternalUseFlagsBuffer((uint8 *)pDataLocation);
            break;
        }

        // case dTRIMESHDATA__MAX: -- To be located by Find in Files
        default:
        {
            dUASSERT(dataId, "invalid data type");
            break;
        }
    }
}

static void *geomTriMeshDataGet(dTriMeshDataID g, int dataId, size_t *pOutDataSize);

/*extern */
void *dGeomTriMeshDataGet(dTriMeshDataID g, int dataId, size_t *pOutDataSize)
{
    return geomTriMeshDataGet(g, dataId, NULL);
}

/*extern */
void *dGeomTriMeshDataGet2(dTriMeshDataID g, int dataId, size_t *pOutDataSize)
{
    return geomTriMeshDataGet(g, dataId, pOutDataSize);
}

static 
void *geomTriMeshDataGet(dTriMeshDataID g, int dataId, size_t *pOutDataSize)
{
    dUASSERT(g, "The argument is not a trimesh data");

    const dxTriMeshData *data = g;

    void *result = NULL;

    switch (dataId)
    {
        case dTRIMESHDATA_FACE_NORMALS:
        {
            if (pOutDataSize != NULL)
            {
                *pOutDataSize = data->calculateNormalsMemoryRequirement();
            }

            result = (void *)data->retrieveNormals();
            break;
        }

        case dTRIMESHDATA_USE_FLAGS:
        {
            if (pOutDataSize != NULL)
            {
                *pOutDataSize = data->calculateUseFlagsMemoryRequirement();
            }

            result = const_cast<uint8 *>(data->smartRetrieveUseFlags());
            break;
        }

        // case dTRIMESHDATA__MAX: -- To be located by Find in Files
        default:
        {
            if (pOutDataSize != NULL)
            {
                *pOutDataSize = 0;
            }

            dUASSERT(dataId, "invalid data type");
            break;
        }
    }

    return result;
}


/*extern */
void dGeomTriMeshDataBuildSingle1(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride,
    const void* Normals)
{
    dUASSERT(g, "The argument is not a trimesh data");

    dxTriMeshData *data = g;
    data->buildData((const Point *)Vertices, VertexStride, VertexCount, 
        (const IndexedTriangle *)Indices, IndexCount, TriStride, 
        (const dReal *)Normals, 
        true);
}

/*extern */
void dGeomTriMeshDataBuildDouble1(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride,
    const void* Normals)
{
    dUASSERT(g, "The argument is not a trimesh data");

    g->buildData((const Point *)Vertices, VertexStride, VertexCount, 
        (const IndexedTriangle *)Indices, IndexCount, TriStride, 
        (const dReal *)Normals, 
        false);
}


//////////////////////////////////////////////////////////////////////////

/*extern */
dGeomID dCreateTriMesh(dSpaceID space, 
    dTriMeshDataID Data,
    dTriCallback* Callback,
    dTriArrayCallback* ArrayCallback,
    dTriRayCallback* RayCallback)
{
    dxTriMesh *mesh = new dxTriMesh(space, Data, Callback, ArrayCallback, RayCallback);
    return mesh;
}


/*extern */
void dGeomTriMeshSetLastTransform(dGeomID g, const dMatrix4 last_trans )
{
    dAASSERT(g);
    dUASSERT(g->type == dTriMeshClass, "The geom is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignLastTransform(last_trans);
}

/*extern */
const dReal *dGeomTriMeshGetLastTransform(dGeomID g)
{
    dAASSERT(g);
    dUASSERT(g->type == dTriMeshClass, "The geom is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveLastTransform();
}


//////////////////////////////////////////////////////////////////////////

// Cleanup for allocations when shutting down ODE
/*extern */
void opcode_collider_cleanup()
{
#if !dTLS_ENABLED

    // Clear TC caches
    TrimeshCollidersCache *pccColliderCache = GetTrimeshCollidersCache(0);
    pccColliderCache->clearOPCODECaches();

#endif // dTLS_ENABLED
}


#endif // dTRIMESH_ENABLED && dTRIMESH_OPCODE

