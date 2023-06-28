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


#if dTRIMESH_ENABLED

#include "collision_trimesh_internal.h"
#include "odeou.h"

#include <algorithm>



//////////////////////////////////////////////////////////////////////////

enum EdgeStorageSignInclusion
{
    SSI__MIN,

    SSI_SIGNED_STORED = SSI__MIN,
    SSI_POSITIVE_STORED,

    SSI__MAX,
};

template<typename TStorageType, EdgeStorageSignInclusion t_SignInclusion>
class FaceAngleStorageCodec;

template<typename TStorageType>
class FaceAngleStorageCodec<TStorageType, SSI_SIGNED_STORED>
{
public:
    typedef typename _make_signed<TStorageType>::type storage_type;
    enum
    {
        STORAGE_TYPE_MAX = (typename _make_unsigned<TStorageType>::type)(~(typename _make_unsigned<TStorageType>::type)0) >> 1,
    };

    static bool areNegativeAnglesCoded()
    {
        return true;
    }

    static storage_type encodeForStorage(dReal angleValue)
    {
        unsigned angleAsInt = (unsigned)dFloor(dFabs(angleValue) * (dReal)(STORAGE_TYPE_MAX / M_PI));
        unsigned limitedAngleAsInt = dMACRO_MIN(angleAsInt, STORAGE_TYPE_MAX);
        storage_type result = angleValue < REAL(0.0) ? -(storage_type)limitedAngleAsInt : (storage_type)limitedAngleAsInt; 
        return  result;
    }

    static FaceAngleDomain classifyStorageValue(storage_type storedValue)
    {
        dSASSERT(EAD__MAX == 3);

        return storedValue < 0 ? FAD_CONCAVE : (storedValue == 0 ? FAD_FLAT : FAD_CONVEX);
    }

    static bool isAngleDomainStored(FaceAngleDomain domainValue)
    {
        return !dTMPL_IN_RANGE(domainValue, FAD__SIGNSTORED_IMPLICITVALUE_MIN, FAD__SIGNSTORED_IMPLICITVALUE_MAX);
    }

    static dReal decodeStorageValue(storage_type storedValue)
    {
        return storedValue * (dReal)(M_PI / STORAGE_TYPE_MAX);
    }
};

template<typename TStorageType>
class FaceAngleStorageCodec<TStorageType, SSI_POSITIVE_STORED>
{
public:
    typedef typename _make_unsigned<TStorageType>::type storage_type;
    enum
    {
        STORAGE_TYPE_MIN = 0,
        STORAGE_TYPE_MAX = (storage_type)(~(storage_type)0),
    };

    static bool areNegativeAnglesCoded()
    {
        return false;
    }

    static storage_type encodeForStorage(dReal angleValue)
    {
        storage_type result = STORAGE_TYPE_MIN;

        if (angleValue >= REAL(0.0))
        {
            unsigned angleAsInt = (unsigned)dFloor(angleValue * (dReal)(((STORAGE_TYPE_MAX - STORAGE_TYPE_MIN - 1) / M_PI)));
            result = (STORAGE_TYPE_MIN + 1) + dMACRO_MIN(angleAsInt, STORAGE_TYPE_MAX - STORAGE_TYPE_MIN - 1); 
        }

        return  result;
    }

    static FaceAngleDomain classifyStorageValue(storage_type storedValue)
    {
        dSASSERT(EAD__MAX == 3);

        return storedValue < STORAGE_TYPE_MIN + 1 ? FAD_CONCAVE : (storedValue == STORAGE_TYPE_MIN + 1 ? FAD_FLAT : FAD_CONVEX);
    }

    static bool isAngleDomainStored(FaceAngleDomain domainValue)
    {
        return dTMPL_IN_RANGE(domainValue, FAD__BYTEPOS_STORED_MIN, FAD__BYTEPOS_STORED_MAX);
    }

    static dReal decodeStorageValue(storage_type storedValue)
    {
        dIASSERT(storedValue >= (STORAGE_TYPE_MIN + 1));

        return (storedValue - (STORAGE_TYPE_MIN + 1)) * (dReal)(M_PI / (STORAGE_TYPE_MAX - STORAGE_TYPE_MIN - 1));
    }
};

template<class TStorageCodec>
class FaceAnglesWrapper:
    public IFaceAngleStorageControl,
    public IFaceAngleStorageView
{
protected:
    FaceAnglesWrapper(unsigned triangleCount) { setAllocatedTriangleCount(triangleCount); }

public:
    virtual ~FaceAnglesWrapper();

    static IFaceAngleStorageControl *allocateInstance(unsigned triangleCount, IFaceAngleStorageView *&out_storageView);

    static bool calculateInstanceSizeRequired(size_t &out_sizeRequired, unsigned triangleCount);

private:
    void freeInstance();

private:
    typedef typename TStorageCodec::storage_type storage_type;
    typedef storage_type TriangleFaceAngles[dMTV__MAX];

    struct StorageRecord
    {
        StorageRecord(): m_triangleCount(0) {}

        unsigned        m_triangleCount;
        TriangleFaceAngles  m_triangleFaceAngles[1];
    };

    static size_t calculateStorageSizeForTriangleCount(unsigned triangleCount)
    {
        const unsigned baseIncludedTriangleCount = dSTATIC_ARRAY_SIZE(FaceAnglesWrapper<TStorageCodec>::StorageRecord, m_triangleFaceAngles);
        const size_t singleTriangleSize = membersize(FaceAnglesWrapper<TStorageCodec>::StorageRecord, m_triangleFaceAngles[0]);
        return sizeof(FaceAnglesWrapper<TStorageCodec>) + (triangleCount > baseIncludedTriangleCount ? (triangleCount - baseIncludedTriangleCount) * singleTriangleSize : 0U);
    }

    static size_t calculateTriangleCountForStorageSize(size_t storageSize)
    {
        dIASSERT(storageSize >= sizeof(FaceAnglesWrapper<TStorageCodec>));

        const unsigned baseIncludedTriangleCount = dSTATIC_ARRAY_SIZE(FaceAnglesWrapper<TStorageCodec>::StorageRecord, m_triangleFaceAngles);
        const size_t singleTriangleSize = membersize(FaceAnglesWrapper<TStorageCodec>::StorageRecord, m_triangleFaceAngles[0]);
        return (storageSize - sizeof(FaceAnglesWrapper<TStorageCodec>)) / singleTriangleSize + baseIncludedTriangleCount;
    }

private: // IFaceAngleStorageControl
    virtual void disposeStorage();

    virtual bool areNegativeAnglesStored() const;

    virtual void assignFacesAngleIntoStorage(unsigned triangleIndex, dMeshTriangleVertex vertexIndex, dReal dAngleValue);

private: // IFaceAngleStorageView
    virtual FaceAngleDomain retrieveFacesAngleFromStorage(dReal &out_angleValue, unsigned triangleIndex, dMeshTriangleVertex vertexIndex);

public:
    void setFaceAngle(unsigned triangleIndex, dMeshTriangleVertex vertexIndex, dReal dAngleValue)
    {
        dIASSERT(dTMPL_IN_RANGE(triangleIndex, 0, getAllocatedTriangleCount()));
        dIASSERT(dTMPL_IN_RANGE(vertexIndex, dMTV__MIN, dMTV__MAX));

        m_record.m_triangleFaceAngles[triangleIndex][vertexIndex] = TStorageCodec::encodeForStorage(dAngleValue);
    }

    FaceAngleDomain getFaceAngle(dReal &out_angleValue, unsigned triangleIndex, dMeshTriangleVertex vertexIndex) const
    {
        dIASSERT(dTMPL_IN_RANGE(triangleIndex, 0, getAllocatedTriangleCount()));
        dIASSERT(dTMPL_IN_RANGE(vertexIndex, dMTV__MIN, dMTV__MAX));

        storage_type storedValue = m_record.m_triangleFaceAngles[triangleIndex][vertexIndex];
        FaceAngleDomain resultDomain = TStorageCodec::classifyStorageValue(storedValue);

        out_angleValue = TStorageCodec::isAngleDomainStored(resultDomain) ? TStorageCodec::decodeStorageValue(storedValue) : REAL(0.0);
        return resultDomain;
    }

private:
    unsigned getAllocatedTriangleCount() const { return m_record.m_triangleCount; }
    void setAllocatedTriangleCount(unsigned triangleCount) { m_record.m_triangleCount = triangleCount; }

private:
    StorageRecord       m_record;
};


template<class TStorageCodec>
FaceAnglesWrapper<TStorageCodec>::~FaceAnglesWrapper()
{
}


template<class TStorageCodec>
/*static */
IFaceAngleStorageControl *FaceAnglesWrapper<TStorageCodec>::allocateInstance(unsigned triangleCount, IFaceAngleStorageView *&out_storageView)
{
    FaceAnglesWrapper<TStorageCodec> *result = NULL;

    do
    {
        size_t sizeRequired;
        if (!FaceAnglesWrapper<TStorageCodec>::calculateInstanceSizeRequired(sizeRequired, triangleCount))
        {
            break;
        }

        void *bufferPointer = dAlloc(sizeRequired);
        if (bufferPointer == NULL)
        {
            break;
        }

        result = (FaceAnglesWrapper<TStorageCodec> *)bufferPointer;
        new(result) FaceAnglesWrapper<TStorageCodec>(triangleCount);

        out_storageView = result;
    }
    while (false);

    return result;
}

template<class TStorageCodec>
/*static */
bool FaceAnglesWrapper<TStorageCodec>::calculateInstanceSizeRequired(size_t &out_sizeRequired, unsigned triangleCount)
{
    bool result = false;

    do
    {
        size_t triangleMaximumCount = calculateTriangleCountForStorageSize(SIZE_MAX);
        dIASSERT(triangleCount <= triangleMaximumCount);

        if (triangleCount > triangleMaximumCount) // Check for overflow
        {
            break;
        }

        out_sizeRequired = calculateStorageSizeForTriangleCount(triangleCount); // Trailing alignment is going to be added by memory manager automatically
        result = true;
    }
    while (false);

    return result;
}

template<class TStorageCodec>
void FaceAnglesWrapper<TStorageCodec>::freeInstance()
{
    unsigned triangleCount = getAllocatedTriangleCount();

    this->FaceAnglesWrapper<TStorageCodec>::~FaceAnglesWrapper();

    size_t memoryBlockSize = calculateStorageSizeForTriangleCount(triangleCount);
    dFree(this, memoryBlockSize);
}


template<class TStorageCodec>
/*virtual */
void FaceAnglesWrapper<TStorageCodec>::disposeStorage()
{
    freeInstance();
}

template<class TStorageCodec>
/*virtual */
bool FaceAnglesWrapper<TStorageCodec>::areNegativeAnglesStored() const
{
    return TStorageCodec::areNegativeAnglesCoded();
}

template<class TStorageCodec>
/*virtual */
void FaceAnglesWrapper<TStorageCodec>::assignFacesAngleIntoStorage(unsigned triangleIndex, dMeshTriangleVertex vertexIndex, dReal dAngleValue)
{
    setFaceAngle(triangleIndex, vertexIndex, dAngleValue);
}

template<class TStorageCodec>
/*virtual */
FaceAngleDomain FaceAnglesWrapper<TStorageCodec>::retrieveFacesAngleFromStorage(dReal &out_angleValue, unsigned triangleIndex, dMeshTriangleVertex vertexIndex)
{
    return getFaceAngle(out_angleValue, triangleIndex, vertexIndex);
}


typedef IFaceAngleStorageControl *(FAngleStorageAllocProc)(unsigned triangleCount, IFaceAngleStorageView *&out_storageView);

BEGIN_NAMESPACE_OU();
template<>
FAngleStorageAllocProc *const CEnumUnsortedElementArray<FaceAngleStorageMethod, ASM__MAX, FAngleStorageAllocProc *, 0x161211AD>::m_aetElementArray[] =
{
    &FaceAnglesWrapper<FaceAngleStorageCodec<uint8, SSI_SIGNED_STORED> >::allocateInstance, // ASM_BYTE_SIGNED,
    &FaceAnglesWrapper<FaceAngleStorageCodec<uint8, SSI_POSITIVE_STORED> >::allocateInstance, // ASM_BYTE_POSITIVE,
    &FaceAnglesWrapper<FaceAngleStorageCodec<uint16, SSI_SIGNED_STORED> >::allocateInstance, // ASM_WORD_SIGNED,
};
END_NAMESPACE_OU();
static const CEnumUnsortedElementArray<FaceAngleStorageMethod, ASM__MAX, FAngleStorageAllocProc *, 0x161211AD> g_AngleStorageAllocProcs;


//////////////////////////////////////////////////////////////////////////

dxTriDataBase::~dxTriDataBase()
{
    freeFaceAngles();
}


void dxTriDataBase::buildData(const void *vertices, int vertexStride, unsigned vertexCount,
    const void *indices, unsigned indexCount, int triStride,
    const void *normals,
    bool single)
{
    dIASSERT(vertices);
    dIASSERT(indices);
    dIASSERT(vertexStride);
    dIASSERT(triStride);
    dIASSERT(indexCount);
    dIASSERT(indexCount % dMTV__MAX == 0);

    m_vertices = vertices;
    m_vertexStride = vertexStride;
    m_vertexCount = vertexCount;
    m_indices = indices;
    m_triangleCount = indexCount / dMTV__MAX;
    m_triStride = triStride;
    m_single = single;

    m_normals = normals;
}


bool dxTriDataBase::allocateFaceAngles(FaceAngleStorageMethod storageMethod)
{
    bool result = false;

    dIASSERT(m_faceAngles == NULL);
    
    IFaceAngleStorageView *storageView;

    unsigned triangleCount = m_triangleCount;

    FAngleStorageAllocProc *allocProc = g_AngleStorageAllocProcs.Encode(storageMethod);
    IFaceAngleStorageControl *storageInstance = allocProc(triangleCount, storageView);

    if (storageInstance != NULL)
    {
        m_faceAngles = storageInstance;
        m_faceAngleView = storageView;
        result = true;
    }

    return result;
}

void dxTriDataBase::freeFaceAngles()
{
    if (m_faceAngles != NULL)
    {
        m_faceAngles->disposeStorage();
        m_faceAngles = NULL;
        m_faceAngleView = NULL;
    }
}


void dxTriDataBase::EdgeRecord::setupEdge(dMeshTriangleVertex edgeIdx, int triIdx, const unsigned vertexIndices[dMTV__MAX])
{
    if (edgeIdx < dMTV_SECOND)
    {
        dIASSERT(edgeIdx == dMTV_FIRST);

        m_edgeFlags  = dxTriMeshData::CUF_USE_FIRST_EDGE;
        m_vert1Flags = dxTriMeshData::CUF_USE_FIRST_VERTEX;
        m_vert2Flags = dxTriMeshData::CUF_USE_SECOND_VERTEX;
        m_vertIdx1 = vertexIndices[dMTV_FIRST];
        m_vertIdx2 = vertexIndices[dMTV_SECOND];
    }
    else if (edgeIdx == dMTV_SECOND)
    {
        m_edgeFlags  = dxTriMeshData::CUF_USE_SECOND_EDGE;
        m_vert1Flags = dxTriMeshData::CUF_USE_SECOND_VERTEX;
        m_vert2Flags = dxTriMeshData::CUF_USE_THIRD_VERTEX;
        m_vertIdx1 = vertexIndices[dMTV_SECOND];
        m_vertIdx2 = vertexIndices[dMTV_THIRD];
    }
    else
    {
        dIASSERT(edgeIdx == dMTV_THIRD);

        m_edgeFlags  = dxTriMeshData::CUF_USE_THIRD_EDGE;
        m_vert1Flags = dxTriMeshData::CUF_USE_THIRD_VERTEX;
        m_vert2Flags = dxTriMeshData::CUF_USE_FIRST_VERTEX;
        m_vertIdx1 = vertexIndices[dMTV_THIRD];
        m_vertIdx2 = vertexIndices[dMTV_FIRST];
    }

    // Make sure vertex index 1 is less than index 2 (for easier sorting)
    if (m_vertIdx1 > m_vertIdx2)
    {
        dxSwap(m_vertIdx1, m_vertIdx2);
        dxSwap(m_vert1Flags, m_vert2Flags);
    }

    m_triIdx = triIdx;
    m_absVertexFlags = 0;
}


BEGIN_NAMESPACE_OU();
template<>
const dMeshTriangleVertex CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161116DC>::m_aetElementArray[] = 
{
    dMTV_FIRST, // kVert0 / kVert_Base
    dMTV_SECOND, // kVert1 / kVert_Base
    dMTV__MAX,
    dMTV_THIRD, // kVert2 / kVert_Base
};
END_NAMESPACE_OU();
/*extern */const CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161116DC> g_VertFlagOppositeIndices;

BEGIN_NAMESPACE_OU();
template<>
const dMeshTriangleVertex CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161225E9>::m_aetElementArray[] = 
{
    dMTV_SECOND, // kVert0 / kVert_Base
    dMTV_THIRD, // kVert1 / kVert_Base
    dMTV__MAX,
    dMTV_FIRST, // kVert2 / kVert_Base
};
END_NAMESPACE_OU();
/*extern */const CEnumUnsortedElementArray<unsigned, dxTriDataBase::CUF__USE_VERTICES_LAST / dxTriDataBase::CUF__USE_VERTICES_MIN, dMeshTriangleVertex, 0x161225E9> g_VertFlagEdgeStartIndices;


//////////////////////////////////////////////////////////////////////////

/*extern ODE_API */
void dGeomTriMeshDataBuildSimple1(dTriMeshDataID g,
    const dReal* Vertices, int VertexCount, 
    const dTriIndex* Indices, int IndexCount,
    const int *Normals)
{
#ifdef dSINGLE
    dGeomTriMeshDataBuildSingle1(g,
        Vertices, 4 * sizeof(dReal), VertexCount, 
        Indices, IndexCount, 3 * sizeof(dTriIndex),
        Normals);
#else
    dGeomTriMeshDataBuildDouble1(g, Vertices, 4 * sizeof(dReal), VertexCount, 
        Indices, IndexCount, 3 * sizeof(dTriIndex),
        Normals);
#endif
}


/*extern ODE_API */
void dGeomTriMeshDataBuildSingle(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride)
{
    dGeomTriMeshDataBuildSingle1(g, Vertices, VertexStride, VertexCount,
        Indices, IndexCount, TriStride, (const void *)NULL);
}

/*extern ODE_API */
void dGeomTriMeshDataBuildDouble(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride)
{
    dGeomTriMeshDataBuildDouble1(g, Vertices, VertexStride, VertexCount,
        Indices, IndexCount, TriStride, NULL);
}

/*extern ODE_API */
void dGeomTriMeshDataBuildSimple(dTriMeshDataID g,
    const dReal* Vertices, int VertexCount, 
    const dTriIndex* Indices, int IndexCount)
{
    dGeomTriMeshDataBuildSimple1(g,
        Vertices, VertexCount, Indices, IndexCount,
        (int *)NULL);
}


/*extern ODE_API */
int dGeomTriMeshDataPreprocess(dTriMeshDataID g)
{
    unsigned buildRequestFlags = (1U << dTRIDATAPREPROCESS_BUILD_CONCAVE_EDGES);
    return dGeomTriMeshDataPreprocess2(g, buildRequestFlags, NULL);
}


BEGIN_NAMESPACE_OU();
template<>
const FaceAngleStorageMethod CEnumUnsortedElementArray<unsigned, dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MAX, FaceAngleStorageMethod, 0x17010902>::m_aetElementArray[] = 
{
    ASM_BYTE_POSITIVE, // dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_BYTE_POSITIVE,
    ASM_BYTE_SIGNED, // dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_BYTE_ALL,
    ASM_WORD_SIGNED, // dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_WORD_ALL,
};
END_NAMESPACE_OU();
static const CEnumUnsortedElementArray<unsigned, dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MAX, FaceAngleStorageMethod, 0x17010902> g_TriMeshDataPreprocess_FaceAndlesExtraDataAngleStorageMethods;

/*extern ODE_API */
int dGeomTriMeshDataPreprocess2(dTriMeshDataID g, unsigned int buildRequestFlags, const dintptr *requestExtraData/*=NULL | const dintptr (*)[dTRIDATAPREPROCESS_BUILD__MAX]*/)
{
    dUASSERT(g, "The argument is not a trimesh data");
    dAASSERT((buildRequestFlags & (1U << dTRIDATAPREPROCESS_BUILD_FACE_ANGLES)) == 0 || requestExtraData == NULL || dIN_RANGE(requestExtraData[dTRIDATAPREPROCESS_BUILD_FACE_ANGLES], dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MIN, dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MAX));

    dxTriMeshData *data = g;

    bool buildUseFlags = (buildRequestFlags & (1U << dTRIDATAPREPROCESS_BUILD_CONCAVE_EDGES)) != 0;
    FaceAngleStorageMethod faceAnglesRequirement = (buildRequestFlags & (1U << dTRIDATAPREPROCESS_BUILD_FACE_ANGLES)) != 0
        ? g_TriMeshDataPreprocess_FaceAndlesExtraDataAngleStorageMethods.Encode(requestExtraData != NULL && dIN_RANGE(requestExtraData[dTRIDATAPREPROCESS_BUILD_FACE_ANGLES], dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MIN, dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MAX) ? (unsigned)requestExtraData[dTRIDATAPREPROCESS_BUILD_FACE_ANGLES] : dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__DEFAULT)
        : ASM__INVALID;
    return data->preprocessData(buildUseFlags, faceAnglesRequirement);
}

/*extern ODE_API */
void dGeomTriMeshDataUpdate(dTriMeshDataID g) 
{
    dUASSERT(g, "The argument is not a trimesh data");

    dxTriMeshData *data = g;
    data->updateData();
}


//////////////////////////////////////////////////////////////////////////

/*extern ODE_API */
void dGeomTriMeshSetCallback(dGeomID g, dTriCallback* Callback)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignCallback(Callback);
}

/*extern ODE_API */
dTriCallback* dGeomTriMeshGetCallback(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveCallback();
}

/*extern ODE_API */
void dGeomTriMeshSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignArrayCallback(ArrayCallback);
}

/*extern ODE_API */
dTriArrayCallback *dGeomTriMeshGetArrayCallback(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveArrayCallback();
}

/*extern ODE_API */
void dGeomTriMeshSetRayCallback(dGeomID g, dTriRayCallback* Callback)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignRayCallback(Callback);
}

/*extern ODE_API */
dTriRayCallback* dGeomTriMeshGetRayCallback(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");	

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveRayCallback();
}

/*extern ODE_API */
void dGeomTriMeshSetTriMergeCallback(dGeomID g, dTriTriMergeCallback* Callback)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignTriMergeCallback(Callback);
}

/*extern ODE_API */
dTriTriMergeCallback *dGeomTriMeshGetTriMergeCallback(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");	

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveTriMergeCallback();
}

/*extern ODE_API */
void dGeomTriMeshSetData(dGeomID g, dTriMeshDataID Data)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->assignMeshData(Data);
}

/*extern ODE_API */
dTriMeshDataID dGeomTriMeshGetData(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveMeshData();
}


BEGIN_NAMESPACE_OU();
template<>
const int CEnumSortedElementArray<dxTriMesh::TRIMESHTC, dxTriMesh::TTC__MAX, int, 0x161003D5>::m_aetElementArray[] =
{
    dSphereClass, // TTC_SPHERE,
    dBoxClass, // TTC_BOX,
    dCapsuleClass, // TTC_CAPSULE,
};
END_NAMESPACE_OU();
static const CEnumSortedElementArray<dxTriMesh::TRIMESHTC, dxTriMesh::TTC__MAX, int, 0x161003D5> g_asiMeshTCGeomClasses;

/*extern ODE_API */
void dGeomTriMeshEnableTC(dGeomID g, int geomClass, int enable)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);

    dxTriMesh::TRIMESHTC tc = g_asiMeshTCGeomClasses.Decode(geomClass);

    if (g_asiMeshTCGeomClasses.IsValidDecode(tc))
    {
        mesh->assignDoTC(tc, enable != 0);
    }
}

/*extern ODE_API */
int dGeomTriMeshIsTCEnabled(dGeomID g, int geomClass)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);

    dxTriMesh::TRIMESHTC tc = g_asiMeshTCGeomClasses.Decode(geomClass);

    bool result = g_asiMeshTCGeomClasses.IsValidDecode(tc) 
        && mesh->retrieveDoTC(tc);
    return result;
}


/*extern ODE_API */
dTriMeshDataID dGeomTriMeshGetTriMeshDataID(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    return mesh->retrieveMeshData();
}


/*extern ODE_API */
void dGeomTriMeshClearTCCache(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    mesh->clearTCCache();
}


/*extern ODE_API */
int dGeomTriMeshGetTriangleCount(dGeomID g)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    const dxTriMesh *mesh = static_cast<dxTriMesh *>(g);
    unsigned result = mesh->getMeshTriangleCount();
    return result;
}


/*extern ODE_API */
void dGeomTriMeshGetTriangle(dGeomID g, int index, dVector3 *v0/*=NULL*/, dVector3 *v1/*=NULL*/, dVector3 *v2/*=NULL*/)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");
    dUASSERT(v0 != NULL || v1 != NULL || v2 != NULL, "A meaningless call");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);

    dVector3 *pv[3] = { v0, v1, v2 };
    mesh->fetchMeshTransformedTriangle(pv, index);
}

/*extern ODE_API */
void dGeomTriMeshGetPoint(dGeomID g, int index, dReal u, dReal v, dVector3 Out)
{
    dUASSERT(g && g->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(g);

    dVector3 dv[3];
    mesh->fetchMeshTransformedTriangle(dv, index);

    GetPointFromBarycentric(dv, u, v, Out);
}


/*extern */
IFaceAngleStorageView *dxGeomTriMeshGetFaceAngleView(dxGeom *triMeshGeom)
{
    dUASSERT(triMeshGeom && triMeshGeom->type == dTriMeshClass, "The argument is not a trimesh");

    dxTriMesh *mesh = static_cast<dxTriMesh *>(triMeshGeom);
    return mesh->retrieveFaceAngleView();
}


#endif // #if dTRIMESH_ENABLED


//////////////////////////////////////////////////////////////////////////
// Deprecated functions

/*extern */
void dGeomTriMeshDataGetBuffer(dTriMeshDataID g, unsigned char **buf, int *bufLen)
{
    size_t dataSizeStorage;
    void *dataPointer = dGeomTriMeshDataGet2(g, dTRIMESHDATA_USE_FLAGS, (bufLen != NULL ? &dataSizeStorage : NULL));

    if (bufLen != NULL)
    {
        *bufLen = (int)dataSizeStorage;
    }

    if (buf != NULL)
    {
        *buf = (unsigned char *)dataPointer;
    }
}

/*extern */
void dGeomTriMeshDataSetBuffer(dTriMeshDataID g, unsigned char* buf)
{
    dGeomTriMeshDataSet(g, dTRIMESHDATA_USE_FLAGS, (void *)buf);
}

