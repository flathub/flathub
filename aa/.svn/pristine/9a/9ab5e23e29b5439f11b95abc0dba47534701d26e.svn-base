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

// TriMesh storage classes refactoring and face angle computation code by Oleh Derevenko (C) 2016-2017

#include <ode/collision.h>
#include <ode/rotation.h>
#include "config.h"
#include "matrix.h"
#include "odemath.h"
#include "util.h"


#if dTRIMESH_ENABLED && dTRIMESH_GIMPACT

#include "collision_util.h"
#include "collision_trimesh_gimpact.h"
#include "collision_trimesh_internal_impl.h"


//////////////////////////////////////////////////////////////////////////
// dxTriMeshData

bool dxTriMeshData::preprocessData(bool /*buildUseFlags*//*=false*/, FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/)
{
    FaceAngleStorageMethod faceAndgesRequirementToUse = faceAndgesRequirement;

    if (faceAndgesRequirement != ASM__INVALID && haveFaceAnglesBeenBuilt())
    {
        dUASSERT(false, "Another request to build face angles after they had already been built");

        faceAndgesRequirementToUse = ASM__INVALID;
    }

    // If this mesh has already been preprocessed, exit
    bool result = faceAndgesRequirementToUse == ASM__INVALID || retrieveTriangleCount() == 0 
        || meaningfulPreprocessData(faceAndgesRequirementToUse);
    return result;
}

struct TrimeshDataVertexIndexAccessor_GIMPACT
{
    enum
    {
        TRIANGLEINDEX_STRIDE = dxTriMesh::TRIANGLEINDEX_STRIDE,
    };

    explicit TrimeshDataVertexIndexAccessor_GIMPACT(dxTriMeshData *meshData):
        m_TriangleVertexIndices(meshData->retrieveTriangleVertexIndices())
    {
        dIASSERT(meshData->retrieveTriangleStride() == TRIANGLEINDEX_STRIDE);
    }

    void getTriangleVertexIndices(unsigned out_VertexIndices[dMTV__MAX], unsigned triangleIdx) const
    {
        const GUINT32 *triIndicesBegin = m_TriangleVertexIndices;
        const unsigned triStride = TRIANGLEINDEX_STRIDE;

        const GUINT32 *triIndicesOfInterest = (const GUINT32 *)((const uint8 *)triIndicesBegin + (size_t)triangleIdx * triStride);
        std::copy(triIndicesOfInterest, triIndicesOfInterest + dMTV__MAX, out_VertexIndices);
    }

    const GUINT32           *m_TriangleVertexIndices;
};

struct TrimeshDataTrianglePointAccessor_GIMPACT
{
    enum
    {
        VERTEXINSTANCE_STRIDE = dxTriMesh::VERTEXINSTANCE_STRIDE,
        TRIANGLEINDEX_STRIDE = dxTriMesh::TRIANGLEINDEX_STRIDE,
    };

    TrimeshDataTrianglePointAccessor_GIMPACT(dxTriMeshData *meshData):
        m_VertexInstances(meshData->retrieveVertexInstances()),
        m_TriangleVertexIndices(meshData->retrieveTriangleVertexIndices())
    {
        dIASSERT((unsigned)meshData->retrieveVertexStride() == (unsigned)VERTEXINSTANCE_STRIDE);
        dIASSERT((unsigned)meshData->retrieveTriangleStride() == (unsigned)TRIANGLEINDEX_STRIDE);
    }

    void getTriangleVertexPoints(dVector3 out_Points[dMTV__MAX], unsigned triangleIndex) const
    {
        dxTriMeshData::retrieveTriangleVertexPoints(out_Points, triangleIndex, 
            &m_VertexInstances[0][0], VERTEXINSTANCE_STRIDE, m_TriangleVertexIndices, TRIANGLEINDEX_STRIDE);
    }

    const vec3f             *m_VertexInstances;
    const GUINT32           *m_TriangleVertexIndices;
};

bool dxTriMeshData::meaningfulPreprocessData(FaceAngleStorageMethod faceAndgesRequirement/*=ASM__INVALID*/)
{
    const bool buildFaceAngles = true; dIASSERT(faceAndgesRequirement != ASM__INVALID);
    // dIASSERT(buildFaceAngles);
    dIASSERT(/*!buildFaceAngles || */!haveFaceAnglesBeenBuilt());

    bool result = false;

    bool anglesAllocated = false;

    do 
    {
        if (buildFaceAngles)
        {
            if (!allocateFaceAngles(faceAndgesRequirement))
            {
                break;
            }
        }

        anglesAllocated = true;

        const unsigned int numTris = retrieveTriangleCount();
        const unsigned int numVertices = retrieveVertexCount();
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

        TrimeshDataVertexIndexAccessor_GIMPACT indexAccessor(this);
        meaningfulPreprocess_SetupEdgeRecords(edges, numEdges, indexAccessor);

        // Sort the edges, so the ones sharing the same verts are beside each other
        std::sort(edges, edges + numEdges);

        TrimeshDataTrianglePointAccessor_GIMPACT pointAccessor(this);
        const dReal *const externalNormals = retrieveNormals();
        IFaceAngleStorageControl *faceAngles = retrieveFaceAngles();
        meaningfulPreprocess_buildEdgeFlags(NULL, faceAngles, edges, numEdges, vertices, externalNormals, pointAccessor);

        dFree(tempBuffer, totalTempMemoryRequired);

        result = true;
    }
    while (false);

    if (!result)
    {
        if (anglesAllocated)
        {
            if (buildFaceAngles)
            {
                freeFaceAngles();
            }
        }
    }

    return result;
}


//////////////////////////////////////////////////////////////////////////
// Trimesh

dxTriMesh::~dxTriMesh()
{
    //Terminate Trimesh
    gim_trimesh_destroy(&m_collision_trimesh);
    gim_terminate_buffer_managers(m_buffer_managers);
}


/*virtual */
void dxTriMesh::computeAABB()
{
    //update trimesh transform
    mat4f transform;
    IDENTIFY_MATRIX_4X4(transform);
    MakeMatrix(this, transform);
    gim_trimesh_set_tranform(&m_collision_trimesh, transform);

    //Update trimesh boxes
    gim_trimesh_update(&m_collision_trimesh);

    GIM_AABB_COPY( &m_collision_trimesh.m_aabbset.m_global_bound, aabb );
}


void dxTriMesh::assignMeshData(dxTriMeshData *Data)
{
    // GIMPACT only supports stride 12, so we need to catch the error early.
    dUASSERT(
        (unsigned int)Data->retrieveVertexStride() == (unsigned)VERTEXINSTANCE_STRIDE 
        && (unsigned int)Data->retrieveTriangleStride() == (unsigned)TRIANGLEINDEX_STRIDE,
        "Gimpact trimesh only supports a stride of 3 float/int\n"
        "This means that you cannot use dGeomTriMeshDataBuildSimple() with Gimpact.\n"
        "Change the stride, or use Opcode trimeshes instead.\n"
    );

    dxTriMesh_Parent::assignMeshData(Data);

    //Create trimesh
    const vec3f *vertexInstances = Data->retrieveVertexInstances();
    if ( vertexInstances != NULL )
    {
        const GUINT32 *triangleVertexIndices = Data->retrieveTriangleVertexIndices();

        size_t vertexInstanceCount = Data->retrieveVertexCount();
        size_t triangleVertexCount = (size_t)Data->retrieveTriangleCount() * dMTV__MAX;

        gim_trimesh_create_from_data(
            m_buffer_managers,
            &m_collision_trimesh,                           // gimpact mesh
            const_cast<vec3f *>(vertexInstances),           // vertices
            dCAST_TO_SMALLER(GUINT32, vertexInstanceCount), // nr of verts
            0,                                              // copy verts?
            const_cast<GUINT32 *>(triangleVertexIndices),   // indices
            dCAST_TO_SMALLER(GUINT32, triangleVertexCount), // nr of indices
            0,                                              // copy indices?
            1                                               // transformed reply
        );
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
    dxTriMeshData *data = g;
    delete data;
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

        case dTRIMESHDATA_USE_FLAGS: // Not used for GIMPACT
        {
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

static void *geomTriMeshDataGet(dTriMeshDataID g, int dataId, size_t *pOutDataSize) ;

/*extern */
void *dGeomTriMeshDataGet(dTriMeshDataID g, int dataId) 
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

        case dTRIMESHDATA_USE_FLAGS: // Not not used for GIMPACT
        {
            if (pOutDataSize != NULL)
            {
                *pOutDataSize = 0;
            }

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
    dAASSERT(Vertices);
    dAASSERT(Indices);

    dxTriMeshData *data = g;

    data->buildData(Vertices, VertexStride, VertexCount,
        Indices, IndexCount, TriStride,
        Normals,
        true);
}

/*extern */
void dGeomTriMeshDataBuildDouble1(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount,
    const void* Indices, int IndexCount, int TriStride,
    const void* Normals)
{
    dUASSERT(g, "The argument is not a trimesh data");
    dAASSERT(Vertices);
    dAASSERT(Indices);

    dxTriMeshData *data = g;

    data->buildData(Vertices, VertexStride, VertexCount,
        Indices, IndexCount, TriStride,
        Normals,
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

    //stub
}

/*extern */
const dReal *dGeomTriMeshGetLastTransform(dGeomID g)
{
    dAASSERT(g);
    dUASSERT(g->type == dTriMeshClass, "The geom is not a trimesh");

    return NULL; // stub
}


#endif // #if dTRIMESH_ENABLED && dTRIMESH_GIMPACT

