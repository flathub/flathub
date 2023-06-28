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

// TriMesh - Plane collider by David Walters, July 2006

#include <ode/collision.h>
#include <ode/rotation.h>
#include "config.h"
#include "matrix.h"
#include "odemath.h"

#if dTRIMESH_ENABLED

#include "collision_util.h"
#include "collision_std.h"
#include "collision_trimesh_internal.h"


#if dTRIMESH_OPCODE

int dCollideTrimeshPlane( dxGeom *o1, dxGeom *o2, int flags, dContactGeom* contacts, int skip )
{
    dIASSERT( skip >= (int)sizeof( dContactGeom ) );
    dIASSERT( o1->type == dTriMeshClass );
    dIASSERT( o2->type == dPlaneClass );
    dIASSERT ((flags & NUMC_MASK) >= 1);

    // Alias pointers to the plane and trimesh
    dxTriMesh* trimesh = (dxTriMesh*)( o1 );
    dxPlane* plane = (dxPlane*)( o2 );

    int contact_count = 0;

    // Cache the maximum contact count.
    const int contact_max = ( flags & NUMC_MASK );

    // Cache trimesh position and rotation.
    const dVector3& trimesh_pos = *(const dVector3*)dGeomGetPosition( trimesh );
    const dMatrix3& trimesh_R = *(const dMatrix3*)dGeomGetRotation( trimesh );

    //
    // For all triangles.
    //

    VertexPointersEx VPE;
    VertexPointers &VP = VPE.vp;
    ConversionArea VC;
    dReal alpha;
    dVector3 vertex;

#if !defined(dSINGLE) || 1
    dVector3 int_vertex;		// Intermediate vertex for double precision mode.
#endif // dSINGLE

    const unsigned uiTLSKind = trimesh->getParentSpaceTLSKind();
    dIASSERT(uiTLSKind == plane->getParentSpaceTLSKind()); // The colliding spaces must use matching cleanup method
    TrimeshCollidersCache *pccColliderCache = GetTrimeshCollidersCache(uiTLSKind);
    VertexUseCache &vertex_use_cache = pccColliderCache->m_VertexUses;

    // Reallocate vertex use cache if necessary
    const dxTriMeshData *meshData = trimesh->retrieveMeshData();
    const int vertex_count = meshData->m_Mesh.GetNbVertices();
    const bool cache_status = vertex_use_cache.resizeAndResetVertexUSEDFlags(vertex_count);

    // Cache the triangle count.
    const int tri_count = meshData->m_Mesh.GetNbTriangles();

    // For each triangle
    for ( int t = 0; t < tri_count; ++t )
    {
        // Get triangle, which should also use callback.
        bool ex_avail = meshData->m_Mesh.GetExTriangle( VPE, t, VC);

        // For each vertex.
        for ( int v = 0; v < 3; ++v )
        {
            // point already used ?
            if (cache_status && ex_avail)
            {
                unsigned VIndex = VPE.Index[v];
                if (vertex_use_cache.getVertexUSEDFlag(VIndex))
                    continue;
                // mark this point as used
                vertex_use_cache.setVertexUSEDFlag(VIndex);
            }

            //
            // Get Vertex
            //

#if defined(dSINGLE) && 0 // Always assign via intermediate array as otherwise it is an incapsulation violation

            dMultiply0_331( vertex, trimesh_R, (float*)( VP.Vertex[ v ] ) );

#else // dDOUBLE || 1

            // OPCODE data is in single precision format.
            int_vertex[ 0 ] = VP.Vertex[ v ]->x;
            int_vertex[ 1 ] = VP.Vertex[ v ]->y;
            int_vertex[ 2 ] = VP.Vertex[ v ]->z;

            dMultiply0_331( vertex, trimesh_R, int_vertex );

#endif // dSINGLE/dDOUBLE

            vertex[ 0 ] += trimesh_pos[ 0 ];
            vertex[ 1 ] += trimesh_pos[ 1 ];
            vertex[ 2 ] += trimesh_pos[ 2 ];


            //
            // Collision?
            //

            // If alpha < 0 then point is if front of plane. i.e. no contact
            // If alpha = 0 then the point is on the plane
            alpha = plane->p[ 3 ] - dCalcVectorDot3( plane->p, vertex );

            // If alpha > 0 the point is behind the plane. CONTACT!
            if ( alpha > 0 )
            {
                // Alias the contact
                dContactGeom* contact = SAFECONTACT( flags, contacts, contact_count, skip );

                contact->pos[ 0 ] = vertex[ 0 ];
                contact->pos[ 1 ] = vertex[ 1 ];
                contact->pos[ 2 ] = vertex[ 2 ];

                contact->normal[ 0 ] = plane->p[ 0 ];
                contact->normal[ 1 ] = plane->p[ 1 ];
                contact->normal[ 2 ] = plane->p[ 2 ];

                contact->depth = alpha;
                contact->g1 = trimesh;
                contact->g2 = plane;
                contact->side1 = t;
                contact->side2 = -1;

                ++contact_count;

                // All contact slots are full?
                if ( contact_count >= contact_max )
                    return contact_count; // <=== STOP HERE
            }
        }
    }

    // Return contact count.
    return contact_count;
}


#endif // dTRIMESH_OPCODE


#if dTRIMESH_GIMPACT

#include "gimpact_contact_export_helper.h"
#include "gimpact_plane_contact_accessor.h"


int dCollideTrimeshPlane( dxGeom *o1, dxGeom *o2, int flags, dContactGeom* contacts, int skip )
{
    dIASSERT( skip >= (int)sizeof( dContactGeom ) );
    dIASSERT( o1->type == dTriMeshClass );
    dIASSERT( o2->type == dPlaneClass );
    dIASSERT ((flags & NUMC_MASK) >= 1);

    // Alias pointers to the plane and trimesh
    dxTriMesh* trimesh = (dxTriMesh*)( o1 );
    dVector4 plane;
    dGeomPlaneGetParams(o2, plane);

    o1 -> recomputeAABB();
    o2 -> recomputeAABB();

    //Find collision

    GDYNAMIC_ARRAY collision_result;
    GIM_CREATE_TRIMESHPLANE_CONTACTS(collision_result);

    gim_trimesh_plane_collisionODE(&trimesh->m_collision_trimesh,plane,&collision_result);

    if(collision_result.m_size == 0 )
    {
        GIM_DYNARRAY_DESTROY(collision_result);
        return 0;
    }


    vec4f * planecontact_results = GIM_DYNARRAY_POINTER(vec4f, collision_result);
    unsigned int contactcount = collision_result.m_size;
    
    dxPlaneContactAccessor contactaccessor(planecontact_results, plane, o1, o2);
    contactcount = dxGImpactContactsExportHelper::ExportMaxDepthGImpactContacts(contactaccessor, contactcount, flags, contacts, skip);

    GIM_DYNARRAY_DESTROY(collision_result);

    return (int)contactcount;
}


#endif // dTRIMESH_GIMPACT


#endif // dTRIMESH_ENABLED

