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

/*
 * TriMesh code by Erwin de Vries.
 *
 * Trimesh data.
 * This is where the actual vertexdata (pointers), and BV tree is stored.
 * Vertices should be single precision!
 * This should be more sophisticated, so that the user can easyly implement
 * another collision library, but this is a lot of work, and also costs some
 * performance because some data has to be copied.
 */

#ifndef _ODE_COLLISION_TRIMESH_H_
#define _ODE_COLLISION_TRIMESH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data storage for triangle meshes.
 */
struct dxTriMeshData;
typedef struct dxTriMeshData* dTriMeshDataID;


typedef enum 
{
    dMTV__MIN,

    dMTV_FIRST = dMTV__MIN,
    dMTV_SECOND,
    dMTV_THIRD,

    dMTV__MAX,

} dMeshTriangleVertex;

/*
 * These don't make much sense now, but they will later when we add more
 * features.
 */
ODE_API dTriMeshDataID dGeomTriMeshDataCreate(void);
ODE_API void dGeomTriMeshDataDestroy(dTriMeshDataID g);


/*
 * The values of data_id that can be used with dGeomTriMeshDataSet/dGeomTriMeshDataGet
 */
enum 
{
    dTRIMESHDATA__MIN,

    dTRIMESHDATA_FACE_NORMALS = dTRIMESHDATA__MIN,
    dTRIMESHDATA_USE_FLAGS,

    dTRIMESHDATA__MAX,

#ifndef TRIMESH_FACE_NORMALS // Define this name during the header inclusion if you need it for something else
    // Included for backward compatibility -- please use the corrected name above. Sorry.
    TRIMESH_FACE_NORMALS = dTRIMESHDATA_FACE_NORMALS,
#endif
};

/*
 * The flags of the dTRIMESHDATA_USE_FLAGS data elements
 */
enum 
{
    dMESHDATAUSE_EDGE1      = 0x01,
    dMESHDATAUSE_EDGE2      = 0x02,
    dMESHDATAUSE_EDGE3      = 0x04,
    dMESHDATAUSE_VERTEX1    = 0x08,
    dMESHDATAUSE_VERTEX2    = 0x10,
    dMESHDATAUSE_VERTEX3    = 0x20,
};

/*
 *	Set and get the TriMeshData additional data
 * Note: The data is NOT COPIED on assignment
 */
ODE_API void dGeomTriMeshDataSet(dTriMeshDataID g, int data_id, void *in_data);
ODE_API void *dGeomTriMeshDataGet(dTriMeshDataID g, int data_id);
ODE_API void *dGeomTriMeshDataGet2(dTriMeshDataID g, int data_id, size_t *pout_size/*=NULL*/);



/**
 * We need to set the last transform after each time step for 
 * accurate collision response. These functions get and set that transform.
 * It is stored per geom instance, rather than per dTriMeshDataID.
 */
ODE_API void dGeomTriMeshSetLastTransform( dGeomID g, const dMatrix4 last_trans );
ODE_API const dReal* dGeomTriMeshGetLastTransform( dGeomID g );

/*
 * Build a TriMesh data object with single precision vertex data.
 */
ODE_API void dGeomTriMeshDataBuildSingle(dTriMeshDataID g,
                                 const void* Vertices, int VertexStride, int VertexCount, 
                                 const void* Indices, int IndexCount, int TriStride);
/* same again with a normals array (used as trimesh-trimesh optimization) */
ODE_API void dGeomTriMeshDataBuildSingle1(dTriMeshDataID g,
                                  const void* Vertices, int VertexStride, int VertexCount, 
                                  const void* Indices, int IndexCount, int TriStride,
                                  const void* Normals);
/*
* Build a TriMesh data object with double precision vertex data.
*/
ODE_API void dGeomTriMeshDataBuildDouble(dTriMeshDataID g, 
                                 const void* Vertices,  int VertexStride, int VertexCount, 
                                 const void* Indices, int IndexCount, int TriStride);
/* same again with a normals array (used as trimesh-trimesh optimization) */
ODE_API void dGeomTriMeshDataBuildDouble1(dTriMeshDataID g, 
                                  const void* Vertices,  int VertexStride, int VertexCount, 
                                  const void* Indices, int IndexCount, int TriStride,
                                  const void* Normals);

/*
 * Simple build. Single/double precision based on dSINGLE/dDOUBLE!
 */
ODE_API void dGeomTriMeshDataBuildSimple(dTriMeshDataID g,
                                 const dReal* Vertices, int VertexCount,
                                 const dTriIndex* Indices, int IndexCount);
/* same again with a normals array (used as trimesh-trimesh optimization) */
ODE_API void dGeomTriMeshDataBuildSimple1(dTriMeshDataID g,
                                  const dReal* Vertices, int VertexCount,
                                  const dTriIndex* Indices, int IndexCount,
                                  const int* Normals);


/*
 * Data preprocessing build request flags.
 */
enum
{
    dTRIDATAPREPROCESS_BUILD__MIN,

    dTRIDATAPREPROCESS_BUILD_CONCAVE_EDGES = dTRIDATAPREPROCESS_BUILD__MIN, // Used to optimize OPCODE trimesh-capsule collisions; allocates 1 byte per triangle; no extra data associated
    dTRIDATAPREPROCESS_BUILD_FACE_ANGLES,   // Used to aid trimesh-convex collisions; memory requirements depend on extra data
    
    dTRIDATAPREPROCESS_BUILD__MAX,
};

/*
 * Data preprocessing extra values for dTRIDATAPREPROCESS_BUILD_FACE_ANGLES.
 */
enum 
{
    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MIN,

    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_BYTE_POSITIVE = dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MIN, // Build angles for convex edges only and store as bytes; allocates 3 bytes per triangle; stores angles (0..180] in 1/254 fractions leaving two values for the flat and all the concaves
    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_BYTE_ALL, // Build angles for all the edges and store in bytes; allocates 3 bytes per triangle; stores angles [-180..0) and (0..180] in 1/127 fractions plus a value for the flat angle
    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_WORD_ALL, // Build angles for all the edges and store in words; allocates 6 bytes per triangle; stores angles [-180..0) and (0..180] in 1/32767 fractions plus a value for the flat angle

    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__MAX,

    dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA__DEFAULT = dTRIDATAPREPROCESS_FACE_ANGLES_EXTRA_BYTE_POSITIVE, // The default value assumed if the extra data is not provided
};


/*
 * Pre-process the trimesh data according to the request flags.
 *
 * buildRequestFlags is a bitmask of 1U << dTRIDATAPREPROCESS_BUILD_...
 * It is allowed to call the function multiple times provided the bitmasks are different each time.
 *
 * requestExtraData is an optional pointer to array of extra parameters per bitmask bits 
 * (only the elements indexed by positions of raised bits are examined; 
 * defaults are assumed if the pointer is NULL)
 *
 * The function returns a boolean status the only failure reason being insufficient memory.
 */
ODE_API int dGeomTriMeshDataPreprocess2(dTriMeshDataID g, unsigned int buildRequestFlags, const dintptr *requestExtraData/*=NULL | const dintptr (*)[dTRIDATAPREPROCESS_BUILD__MAX]*/);

/*
 * Obsolete. Equivalent to calling dGeomTriMeshDataPreprocess2(g, (1U << dTRIDATAPREPROCESS_BUILD_CONCAVE_EDGES), NULL)
 */
ODE_API int dGeomTriMeshDataPreprocess(dTriMeshDataID g);



/*
 * Get and set the internal preprocessed trimesh data buffer (see the enumerated type above), for loading and saving 
 * These functions are deprecated. Use dGeomTriMeshDataSet/dGeomTriMeshDataGet2 with dTRIMESHDATA_USE_FLAGS instead.
 */
ODE_API_DEPRECATED ODE_API void dGeomTriMeshDataGetBuffer(dTriMeshDataID g, unsigned char** buf, int* bufLen);
ODE_API_DEPRECATED ODE_API void dGeomTriMeshDataSetBuffer(dTriMeshDataID g, unsigned char* buf);


/*
 * Per triangle callback. Allows the user to say if he wants a collision with
 * a particular triangle.
 */
typedef int dTriCallback(dGeomID TriMesh, dGeomID RefObject, int TriangleIndex);
ODE_API void dGeomTriMeshSetCallback(dGeomID g, dTriCallback* Callback);
ODE_API dTriCallback* dGeomTriMeshGetCallback(dGeomID g);

/*
 * Per object callback. Allows the user to get the list of triangles in 1
 * shot. Maybe we should remove this one.
 */
typedef void dTriArrayCallback(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount);
ODE_API void dGeomTriMeshSetArrayCallback(dGeomID g, dTriArrayCallback* ArrayCallback);
ODE_API dTriArrayCallback* dGeomTriMeshGetArrayCallback(dGeomID g);

/*
 * Ray callback.
 * Allows the user to say if a ray collides with a triangle on barycentric
 * coords. The user can for example sample a texture with alpha transparency
 * to determine if a collision should occur.
 */
typedef int dTriRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex, dReal u, dReal v);
ODE_API void dGeomTriMeshSetRayCallback(dGeomID g, dTriRayCallback* Callback);
ODE_API dTriRayCallback* dGeomTriMeshGetRayCallback(dGeomID g);

/*
 * Triangle merging callback.
 * Allows the user to generate a fake triangle index for a new contact generated
 * from merging of two other contacts. That index could later be used by the 
 * user to determine attributes of original triangles used as sources for a 
 * merged contact.
 */
typedef int dTriTriMergeCallback(dGeomID TriMesh, int FirstTriangleIndex, int SecondTriangleIndex);
ODE_API void dGeomTriMeshSetTriMergeCallback(dGeomID g, dTriTriMergeCallback* Callback);
ODE_API dTriTriMergeCallback* dGeomTriMeshGetTriMergeCallback(dGeomID g);

/*
 * Trimesh class
 * Construction. Callbacks are optional.
 */
ODE_API dGeomID dCreateTriMesh(dSpaceID space, dTriMeshDataID Data, dTriCallback* Callback, dTriArrayCallback* ArrayCallback, dTriRayCallback* RayCallback);

ODE_API void dGeomTriMeshSetData(dGeomID g, dTriMeshDataID Data);
ODE_API dTriMeshDataID dGeomTriMeshGetData(dGeomID g);


/* enable/disable/check temporal coherence*/
ODE_API void dGeomTriMeshEnableTC(dGeomID g, int geomClass, int enable);
ODE_API int dGeomTriMeshIsTCEnabled(dGeomID g, int geomClass);

/*
 * Clears the internal temporal coherence caches. When a geom has its
 * collision checked with a trimesh once, data is stored inside the trimesh.
 * With large worlds with lots of seperate objects this list could get huge.
 * We should be able to do this automagically.
 */
ODE_API void dGeomTriMeshClearTCCache(dGeomID g);


/*
 * returns the TriMeshDataID
 */
ODE_API dTriMeshDataID dGeomTriMeshGetTriMeshDataID(dGeomID g);

/*
 * Gets a triangle.
 */
ODE_API void dGeomTriMeshGetTriangle(dGeomID g, int Index, dVector3* v0, dVector3* v1, dVector3* v2);

/*
 * Gets the point on the requested triangle and the given barycentric
 * coordinates.
 */
ODE_API void dGeomTriMeshGetPoint(dGeomID g, int Index, dReal u, dReal v, dVector3 Out);

/*

This is how the strided data works:

struct StridedVertex{
	dVector3 Vertex;
	// Userdata
};
int VertexStride = sizeof(StridedVertex);

struct StridedTri{
	int Indices[3];
	// Userdata
};
int TriStride = sizeof(StridedTri);

*/


ODE_API int dGeomTriMeshGetTriangleCount (dGeomID g);

ODE_API void dGeomTriMeshDataUpdate(dTriMeshDataID g);

#ifdef __cplusplus
}
#endif

#endif	/* _ODE_COLLISION_TRIMESH_H_ */

