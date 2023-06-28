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

#include <ode/collision.h>
#include "config.h"
#include "matrix.h"


#if !dTRIMESH_ENABLED

#include "collision_util.h"
#include "collision_trimesh_internal.h"


static const dMatrix4 identity = 
{
    REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ),
    REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ),
    REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ),
    REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ), REAL( 0.0 ) 
};


typedef dxMeshBase dxDisabledTriMesh_Parent;
struct dxDisabledTriMesh: 
    public dxDisabledTriMesh_Parent
{
public:
    // Functions
    dxDisabledTriMesh(dxSpace *Space, 
        dTriCallback *Callback, dTriArrayCallback *ArrayCallback, dTriRayCallback *RayCallback):
        dxDisabledTriMesh_Parent(Space, NULL, Callback, ArrayCallback, RayCallback, false)
    {
    }

    virtual void computeAABB(); // This is an abstract method in the base class
};

/*virtual */
void dxDisabledTriMesh::computeAABB()
{
    // Do nothing
}


//////////////////////////////////////////////////////////////////////////
// Stub functions for trimesh calls

/*extern */
dTriMeshDataID dGeomTriMeshDataCreate(void)
{
    return NULL;
}

/*extern */
void dGeomTriMeshDataDestroy(dTriMeshDataID g)
{
    // Do nothing
}


/*extern */
void dGeomTriMeshDataSet(dTriMeshDataID g, int data_id, void* in_data)
{
    // Do nothing
}

/*extern */
void *dGeomTriMeshDataGet(dTriMeshDataID g, int data_id)
{
    return NULL;
}

/*extern */
void *dGeomTriMeshDataGet2(dTriMeshDataID g, int data_id, size_t *pout_size/*=NULL*/)
{
    if (pout_size != NULL)
    {
        *pout_size = 0;
    }

    return NULL;
}


/*extern */
void dGeomTriMeshSetLastTransform( dGeomID g, const dMatrix4 last_trans )
{
    // Do nothing
}

/*extern */
const dReal *dGeomTriMeshGetLastTransform( dGeomID g )
{
    return identity;
}


/*extern */
dGeomID dCreateTriMesh(dSpaceID space, 
    dTriMeshDataID Data,
    dTriCallback* Callback,
    dTriArrayCallback* ArrayCallback,
    dTriRayCallback* RayCallback)
{
    return new dxDisabledTriMesh(space, Callback, ArrayCallback, RayCallback); // Oleh_Derevenko: I'm not sure if a NULL can be returned here -- keep on returning an object for backward compatibility
}


/*extern */
void dGeomTriMeshSetData(dGeomID g, dTriMeshDataID Data)
{
    // Do nothing
}

/*extern */
dTriMeshDataID dGeomTriMeshGetData(dGeomID g)
{
    return NULL;
}


/*extern */
void dGeomTriMeshDataBuildSingle(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride)
{
    // Do nothing
}

/*extern */
void dGeomTriMeshDataBuildSingle1(dTriMeshDataID g,
    const void* Vertices, int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride,
    const void* Normals)
{
    // Do nothing
}

/*extern */
void dGeomTriMeshDataBuildDouble(dTriMeshDataID g, 
    const void* Vertices,  int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride)
{
    // Do nothing
}

/*extern */
void dGeomTriMeshDataBuildDouble1(dTriMeshDataID g, 
    const void* Vertices,  int VertexStride, int VertexCount, 
    const void* Indices, int IndexCount, int TriStride,
    const void* Normals)
{
    // Do nothing
}

/*extern */
void dGeomTriMeshDataBuildSimple(dTriMeshDataID g,
    const dReal* Vertices, int VertexCount,
    const dTriIndex* Indices, int IndexCount)
{
    // Do nothing
}

/*extern */
void dGeomTriMeshDataBuildSimple1(dTriMeshDataID g,
    const dReal* Vertices, int VertexCount,
    const dTriIndex* Indices, int IndexCount,
    const int* Normals)
{
    // Do nothing
}


/*extern ODE_API */
int dGeomTriMeshDataPreprocess(dTriMeshDataID g)
{
    // Do nothing
    return 1;
}

/*extern ODE_API */
int dGeomTriMeshDataPreprocess2(dTriMeshDataID g, unsigned int buildRequestFlags, const dintptr *requestExtraData/*=NULL | const dintptr (*)[dTRIDATAPREPROCESS_BUILD__MAX]*/)
{
    // Do nothing
    return 1;
}

/*extern */
void dGeomTriMeshSetCallback(dGeomID g, dTriCallback* Callback)
{
    // Do nothing
}

/*extern */
dTriCallback* dGeomTriMeshGetCallback(dGeomID g)
{
    return NULL;
}


/*extern */
void dGeomTriMeshSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback)
{
    // Do nothing
}

/*extern */
dTriArrayCallback* dGeomTriMeshGetArrayCallback(dGeomID g)
{
    return NULL;
}


/*extern */
void dGeomTriMeshSetRayCallback(dGeomID g, dTriRayCallback* Callback)
{
    // Do nothing
}

/*extern */
dTriRayCallback* dGeomTriMeshGetRayCallback(dGeomID g)
{
    return NULL;
}


/*extern */
void dGeomTriMeshSetTriMergeCallback(dGeomID g, dTriTriMergeCallback* Callback)
{
    // Do nothing
}

/*extern */
dTriTriMergeCallback* dGeomTriMeshGetTriMergeCallback(dGeomID g)
{
    return NULL;
}


/*extern */
void dGeomTriMeshEnableTC(dGeomID g, int geomClass, int enable)
{
    // Do nothing
}

/*extern */
int dGeomTriMeshIsTCEnabled(dGeomID g, int geomClass)
{
    return 0;
}


/*extern */
void dGeomTriMeshClearTCCache(dGeomID g)
{
    // Do nothing
}


/*extern */
dTriMeshDataID dGeomTriMeshGetTriMeshDataID(dGeomID g)
{
    return NULL;
}


/*extern */
int dGeomTriMeshGetTriangleCount (dGeomID g)
{
    return 0;
}

/*extern */
void dGeomTriMeshDataUpdate(dTriMeshDataID g)
{
    // Do nothing
}


#endif // !dTRIMESH_ENABLED


