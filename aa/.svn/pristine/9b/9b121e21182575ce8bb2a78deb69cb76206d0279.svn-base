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

// TriMesh base template method implementations by Oleh Derevenko (C) 2016-2017


#ifndef _ODE_COLLISION_TRIMESH_INTERNAL_IMPL_H_
#define _ODE_COLLISION_TRIMESH_INTERNAL_IMPL_H_


#include "collision_trimesh_internal.h"


#if dTRIMESH_ENABLED


template<typename tcoordfloat, typename tindexint>
/*static */
void dxTriDataBase::retrieveTriangleVertexPoints(dVector3 out_Points[dMTV__MAX], unsigned triangleIndex,
    const tcoordfloat *vertexInstances, int vertexStride, const tindexint *triangleVertexIndices, int triangleStride)
{
    const tindexint *triangleIndicesOfInterest = (const tindexint *)((uint8 *)triangleVertexIndices + (size_t)triangleIndex * triangleStride);
    for (unsigned trianglePoint = dMTV__MIN; trianglePoint != dMTV__MAX; ++trianglePoint)
    {
        unsigned vertexIndex = triangleIndicesOfInterest[trianglePoint];
        tcoordfloat *pointVertex = (tcoordfloat *)((uint8 *)vertexInstances + (size_t)vertexIndex * vertexStride);
        dAssignVector3(out_Points[trianglePoint], (dReal)pointVertex[dSA_X], (dReal)pointVertex[dSA_Y], (dReal)pointVertex[dSA_Z]);
        dSASSERT(dSA_X == 0);
        dSASSERT(dSA_Y == 1);
        dSASSERT(dSA_Z == 2);
    }
}


template<class TMeshDataAccessor>
/*static */
void dxTriDataBase::meaningfulPreprocess_SetupEdgeRecords(EdgeRecord *edges, size_t numEdges, const TMeshDataAccessor &dataAccessor)
{
    unsigned vertexIndices[dMTV__MAX];
    // Make a list of every edge in the mesh
    unsigned triangleIdx = 0;
    for (size_t edgeIdx = 0; edgeIdx != numEdges; ++triangleIdx, edgeIdx += dMTV__MAX)
    {
        dataAccessor.getTriangleVertexIndices(vertexIndices, triangleIdx);
        edges[edgeIdx + dMTV_FIRST].setupEdge(dMTV_FIRST, triangleIdx, vertexIndices);
        edges[edgeIdx + dMTV_SECOND].setupEdge(dMTV_SECOND, triangleIdx, vertexIndices);
        edges[edgeIdx + dMTV_THIRD].setupEdge(dMTV_THIRD, triangleIdx, vertexIndices);
    }
}

template<class TMeshDataAccessor>
/*static */
void dxTriDataBase::meaningfulPreprocess_buildEdgeFlags(uint8 *useFlags/*=NULL*/, IFaceAngleStorageControl *faceAngles/*=NULL*/, 
    EdgeRecord *edges, size_t numEdges, VertexRecord *vertices, 
    const dReal *externalNormals/*=NULL*/, const TMeshDataAccessor &dataAccessor)
{
    dIASSERT(useFlags != NULL || faceAngles != NULL);
    dIASSERT(numEdges != 0);

    const bool negativeAnglesStored = faceAngles != NULL && faceAngles->areNegativeAnglesStored();

    // Go through the sorted list of edges and flag all the edges and vertices that we need to use
    EdgeRecord *const lastEdge = edges + (numEdges - 1);
    for (EdgeRecord *currEdge = edges; ; ++currEdge)
    {
        // Handle the last edge separately to have an optimizer friendly loop
        if (currEdge >= lastEdge)
        {
            // This is a boundary edge
            if (currEdge == lastEdge)
            {
                if (faceAngles != NULL)
                {
                    buildBoundaryEdgeAngle(faceAngles, currEdge);
                }

                if (useFlags != NULL)
                {
                    // For the last element EdgeRecord::kAbsVertexUsed assignment can be skipped as noone is going to need it any more
                    useFlags[currEdge[0].m_triIdx] |= ((edges[currEdge[0].m_vertIdx1].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0 ? currEdge[0].m_vert1Flags : 0) 
                        | ((edges[currEdge[0].m_vertIdx2].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0 ? currEdge[0].m_vert2Flags : 0)
                        | currEdge[0].m_edgeFlags;
                }
            }

            break;
        }

        unsigned vertIdx1 = currEdge[0].m_vertIdx1;
        unsigned vertIdx2 = currEdge[0].m_vertIdx2;

        if (vertIdx2 == currEdge[1].m_vertIdx2 // Check second vertex first as it is more likely to change taking the sorting rules into account
            && vertIdx1 == currEdge[1].m_vertIdx1)
        {
            // We let the dot threshold for concavity get slightly negative to allow for rounding errors
            const float kConcaveThreshold = 0.000001f;

            const dVector3 *pSecondTriangleEdgeToUse = NULL, *pFirstTriangleToUse = NULL;
            dVector3 secondTriangleMatchingEdge;
            dVector3 firstTriangle[dMTV__MAX];
            dVector3 secondOppositeVertexSegment, triangleNormal;
            dReal lengthSquareProduct, secondOppositeSegmentLengthSquare;

            // Calculate orthogonal vector from the matching edge of the second triangle to its opposite point
            {
                dVector3 secondTriangle[dMTV__MAX];
                dataAccessor.getTriangleVertexPoints(secondTriangle, currEdge[1].m_triIdx);

                // Get the vertex opposite this edge in the second triangle
                dMeshTriangleVertex secondOppositeVertex = currEdge[1].getOppositeVertexIndex();
                dMeshTriangleVertex secondEdgeStart = secondOppositeVertex + 1 != dMTV__MAX ? (dMeshTriangleVertex)(secondOppositeVertex + 1) : dMTV__MIN;
                dMeshTriangleVertex secondEdgeEnd = (dMeshTriangleVertex)(dMTV_FIRST + dMTV_SECOND + dMTV_THIRD - secondEdgeStart - secondOppositeVertex);

                dSubtractVectors3(secondTriangleMatchingEdge, secondTriangle[secondEdgeEnd], secondTriangle[secondEdgeStart]);

                if (dSafeNormalize3(secondTriangleMatchingEdge))
                {
                    pSecondTriangleEdgeToUse = &secondTriangleMatchingEdge;

                    dVector3 secondTriangleOppositeEdge;
                    dSubtractVectors3(secondTriangleOppositeEdge, secondTriangle[secondOppositeVertex], secondTriangle[secondEdgeStart]);
                    dReal dProjectionLength = dCalcVectorDot3(secondTriangleOppositeEdge, secondTriangleMatchingEdge);
                    dAddVectorScaledVector3(secondOppositeVertexSegment, secondTriangleOppositeEdge, secondTriangleMatchingEdge, -dProjectionLength);
                }
                else
                {
                    dSubtractVectors3(secondOppositeVertexSegment, secondTriangle[secondOppositeVertex], secondTriangle[secondEdgeStart]);
                }

                secondOppositeSegmentLengthSquare = dCalcVectorLengthSquare3(secondOppositeVertexSegment);
            }

            // Either calculate the normal from triangle vertices...
            if (externalNormals == NULL)
            {
                // Get the normal of the first triangle
                dataAccessor.getTriangleVertexPoints(firstTriangle, currEdge[0].m_triIdx);
                pFirstTriangleToUse = &firstTriangle[dMTV__MIN];

                dVector3 firstEdge, secondEdge;
                dSubtractVectors3(secondEdge, firstTriangle[dMTV_THIRD], firstTriangle[dMTV_SECOND]);
                dSubtractVectors3(firstEdge, firstTriangle[dMTV_FIRST], firstTriangle[dMTV_SECOND]);
                dCalcVectorCross3(triangleNormal, secondEdge, firstEdge);
                dReal normalLengthSuqare = dCalcVectorLengthSquare3(triangleNormal);
                lengthSquareProduct = secondOppositeSegmentLengthSquare * normalLengthSuqare;
            }
            // ...or use the externally supplied normals
            else
            {
                const dReal *pTriangleExternalNormal = externalNormals + currEdge[0].m_triIdx * dSA__MAX;
                dAssignVector3(triangleNormal, pTriangleExternalNormal[dSA_X], pTriangleExternalNormal[dSA_Y], pTriangleExternalNormal[dSA_Z]);
                // normalLengthSuqare = REAL(1.0);
                dUASSERT(dFabs(dCalcVectorLengthSquare3(triangleNormal) - REAL(1.0)) < REAL(0.25) * kConcaveThreshold * kConcaveThreshold, "Mesh triangle normals must be normalized");

                lengthSquareProduct = secondOppositeSegmentLengthSquare/* * normalLengthSuqare*/;
            }

            dReal normalSegmentDot = dCalcVectorDot3(triangleNormal, secondOppositeVertexSegment);

            // This is a concave edge, leave it for the next pass
            // OD: This is the "dot >= kConcaveThresh" check, but since the vectros were not normalized to save on roots and divisions,
            // the check against zero is performed first and then the dot product is squared and compared against the threshold multiplied by lengths' squares
            // OD: Originally, there was dot > -kConcaveThresh check, but this does not seem to be a good idea
            // as it can mark all edges on potentially large (nearly) flat surfaces concave.
            if (normalSegmentDot > REAL(0.0) && normalSegmentDot * normalSegmentDot >= kConcaveThreshold * kConcaveThreshold * lengthSquareProduct)
            {
                if (faceAngles != NULL)
                {
                    buildConcaveEdgeAngle(faceAngles, negativeAnglesStored, currEdge, normalSegmentDot, lengthSquareProduct,
                        triangleNormal, secondOppositeVertexSegment,
                        pSecondTriangleEdgeToUse, pFirstTriangleToUse, dataAccessor);
                }

                if (useFlags != NULL)
                {
                    // Mark the vertices of a concave edge to prevent their use
                    unsigned absVertexFlags1 = edges[vertIdx1].m_absVertexFlags;
                    edges[vertIdx1].m_absVertexFlags |= absVertexFlags1 | EdgeRecord::AVF_VERTEX_HAS_CONCAVE_EDGE | EdgeRecord::AVF_VERTEX_USED;

                    if ((absVertexFlags1 & (EdgeRecord::AVF_VERTEX_HAS_CONCAVE_EDGE | EdgeRecord::AVF_VERTEX_USED)) == EdgeRecord::AVF_VERTEX_USED)
                    {
                        // If the vertex was already used from other triangles but then discovered 
                        // to have a concave edge, unmark the previous use
                        unsigned usedFromEdgeIndex = vertices[vertIdx1].m_UsedFromEdgeIndex;
                        const EdgeRecord *usedFromEdge = edges + usedFromEdgeIndex;
                        unsigned usedInTriangleIndex = usedFromEdge->m_triIdx;
                        uint8 usedVertFlags = usedFromEdge->m_vertIdx1 == vertIdx1 ? usedFromEdge->m_vert1Flags : usedFromEdge->m_vert2Flags;
                        useFlags[usedInTriangleIndex] ^= usedVertFlags;
                        dIASSERT((useFlags[usedInTriangleIndex] & usedVertFlags) == 0);
                    }

                    unsigned absVertexFlags2 = edges[vertIdx2].m_absVertexFlags;
                    edges[vertIdx2].m_absVertexFlags = absVertexFlags2 | EdgeRecord::AVF_VERTEX_HAS_CONCAVE_EDGE | EdgeRecord::AVF_VERTEX_USED;

                    if ((absVertexFlags2 & (EdgeRecord::AVF_VERTEX_HAS_CONCAVE_EDGE | EdgeRecord::AVF_VERTEX_USED)) == EdgeRecord::AVF_VERTEX_USED)
                    {
                        // Similarly unmark the possible previous use of the edge's second vertex
                        unsigned usedFromEdgeIndex = vertices[vertIdx2].m_UsedFromEdgeIndex;
                        const EdgeRecord *usedFromEdge = edges + usedFromEdgeIndex;
                        unsigned usedInTriangleIndex = usedFromEdge->m_triIdx;
                        uint8 usedVertFlags = usedFromEdge->m_vertIdx1 == vertIdx2 ? usedFromEdge->m_vert1Flags : usedFromEdge->m_vert2Flags;
                        useFlags[usedInTriangleIndex] ^= usedVertFlags;
                        dIASSERT((useFlags[usedInTriangleIndex] & usedVertFlags) == 0);
                    }
                }
            }
            // If this is a convex edge, mark its vertices and edge as used
            else
            {
                if (faceAngles != NULL)
                {
                    buildConvexEdgeAngle(faceAngles, currEdge, normalSegmentDot, lengthSquareProduct,
                        triangleNormal, secondOppositeVertexSegment,
                        pSecondTriangleEdgeToUse, pFirstTriangleToUse, dataAccessor);
                }

                if (useFlags != NULL)
                {
                    EdgeRecord *edgeToUse = currEdge;
                    unsigned triIdx = edgeToUse[0].m_triIdx;
                    unsigned triIdx1 = edgeToUse[1].m_triIdx;
                    
                    unsigned triUseFlags = useFlags[triIdx];
                    unsigned triUseFlags1 = useFlags[triIdx1];

                    // Choose to add flags to the bitmask that already has more edges
                    // (to group flags in selected triangles rather than scattering them evenly)
                    if ((triUseFlags1 & CUF__USE_ALL_EDGES) > (triUseFlags & CUF__USE_ALL_EDGES))
                    {
                        triIdx = triIdx1;
                        triUseFlags = triUseFlags1;
                        edgeToUse = edgeToUse + 1;
                    }

                    if ((edges[vertIdx1].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0)
                    {
                        // Only add each vertex once and set a mark to prevent further additions
                        edges[vertIdx1].m_absVertexFlags |= EdgeRecord::AVF_VERTEX_USED;
                        // Also remember the index the vertex flags are going to be applied to 
                        // to allow easily clear the vertex from the use flags if any concave edges are found to connect to it
                        vertices[vertIdx1].m_UsedFromEdgeIndex = (unsigned)(edgeToUse - edges);
                        triUseFlags |= edgeToUse[0].m_vert1Flags;
                    }

                    // Same processing for the second vertex...
                    if ((edges[vertIdx2].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0)
                    {
                        edges[vertIdx2].m_absVertexFlags |= EdgeRecord::AVF_VERTEX_USED;
                        vertices[vertIdx2].m_UsedFromEdgeIndex = (unsigned)(edgeToUse - edges);
                        triUseFlags |= edgeToUse[0].m_vert2Flags;
                    }

                    // And finally store the use flags adding the edge flags in
                    useFlags[triIdx] = triUseFlags | edgeToUse[0].m_edgeFlags;
                }
            }

            // Skip the second edge
            ++currEdge;
        }
        // This is a boundary edge
        else
        {
            if (faceAngles != NULL)
            {
                buildBoundaryEdgeAngle(faceAngles, currEdge);
            }

            if (useFlags != NULL)
            {
                unsigned triIdx = currEdge[0].m_triIdx;
                unsigned triUseExtraFlags = 0;
                
                if ((edges[vertIdx1].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0)
                {
                    edges[vertIdx1].m_absVertexFlags |= EdgeRecord::AVF_VERTEX_USED;
                    vertices[vertIdx1].m_UsedFromEdgeIndex = (unsigned)(currEdge - edges);
                    triUseExtraFlags |= currEdge[0].m_vert1Flags;
                }

                if ((edges[vertIdx2].m_absVertexFlags & EdgeRecord::AVF_VERTEX_USED) == 0)
                {
                    edges[vertIdx2].m_absVertexFlags |= EdgeRecord::AVF_VERTEX_USED;
                    vertices[vertIdx2].m_UsedFromEdgeIndex = (unsigned)(currEdge - edges);
                    triUseExtraFlags |= currEdge[0].m_vert2Flags;
                }

                useFlags[triIdx] |= triUseExtraFlags | currEdge[0].m_edgeFlags;
            }
        }
    }
}

/*static */
void dxTriDataBase::buildBoundaryEdgeAngle(IFaceAngleStorageControl *faceAngles, 
    EdgeRecord *currEdge)
{
    const dReal faceAngle = REAL(0.0);

    dMeshTriangleVertex firstVertexStartIndex = currEdge[0].getEdgeStartVertexIndex();
    faceAngles->assignFacesAngleIntoStorage(currEdge[0].m_triIdx, firstVertexStartIndex, faceAngle);
    // -- For boundary edges, only the first element is valid
    // dMeshTriangleVertex secondVertexStartIndex = currEdge[1].getEdgeStartVertexIndex();
    // faceAngles->assignFacesAngleIntoStorage(currEdge[1].m_TriIdx, secondVertexStartIndex, faceAngle);
}

template<class TMeshDataAccessor>
/*static */
void dxTriDataBase::buildConcaveEdgeAngle(IFaceAngleStorageControl *faceAngles, bool negativeAnglesStored, 
    EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
    const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
    const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
    const TMeshDataAccessor &dataAccessor)
{
    dReal faceAngle;
    dMeshTriangleVertex firstVertexStartIndex = currEdge[0].getEdgeStartVertexIndex();

    // Check if concave angles are stored at all
    if (negativeAnglesStored)
    {
        // The length square product can become zero due to precision loss
        // when both the normal and the opposite edge vectors are very small.
        if (lengthSquareProduct != REAL(0.0))
        {
            faceAngle = -calculateEdgeAngleValidated(firstVertexStartIndex,
                currEdge, normalSegmentDot, lengthSquareProduct, triangleNormal, secondOppositeVertexSegment,
                pSecondTriangleMatchingEdge, pFirstTriangle, dataAccessor);
        }
        else
        {
            faceAngle = REAL(0.0);
        }
    }
    else
    {
        // If concave angles ate not stored, set an arbitrary negative value
        faceAngle = -(dReal)M_PI;
    }

    faceAngles->assignFacesAngleIntoStorage(currEdge[0].m_triIdx, firstVertexStartIndex, faceAngle);
    dMeshTriangleVertex secondVertexStartIndex = currEdge[1].getEdgeStartVertexIndex();
    faceAngles->assignFacesAngleIntoStorage(currEdge[1].m_triIdx, secondVertexStartIndex, faceAngle);
}

template<class TMeshDataAccessor>
/*static */
void dxTriDataBase::buildConvexEdgeAngle(IFaceAngleStorageControl *faceAngles, 
    EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
    const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
    const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
    const TMeshDataAccessor &dataAccessor)
{
    dReal faceAngle;
    dMeshTriangleVertex firstVertexStartIndex = currEdge[0].getEdgeStartVertexIndex();

    // The length square product can become zero due to precision loss
    // when both the normal and the opposite edge vectors are very small.
    if (normalSegmentDot < REAL(0.0) && lengthSquareProduct != REAL(0.0))
    {
        faceAngle = calculateEdgeAngleValidated(firstVertexStartIndex,
            currEdge, -normalSegmentDot, lengthSquareProduct, triangleNormal, secondOppositeVertexSegment,
            pSecondTriangleMatchingEdge, pFirstTriangle, dataAccessor);
    }
    else
    {
        faceAngle = REAL(0.0);
    }

    faceAngles->assignFacesAngleIntoStorage(currEdge[0].m_triIdx, firstVertexStartIndex, faceAngle);
    dMeshTriangleVertex secondVertexStartIndex = currEdge[1].getEdgeStartVertexIndex();
    faceAngles->assignFacesAngleIntoStorage(currEdge[1].m_triIdx, secondVertexStartIndex, faceAngle);
}

template<class TMeshDataAccessor>
/*static */
dReal dxTriDataBase::calculateEdgeAngleValidated(unsigned firstVertexStartIndex,
    EdgeRecord *currEdge, const dReal &normalSegmentDot, const dReal &lengthSquareProduct,
    const dVector3 &triangleNormal, const dVector3 &secondOppositeVertexSegment,
    const dVector3 *pSecondTriangleMatchingEdge/*=NULL*/, const dVector3 *pFirstTriangle/*=NULL*/, 
    const TMeshDataAccessor &dataAccessor)
{
    dIASSERT(lengthSquareProduct >= REAL(0.0));

    dReal result;
    dReal angleCosine = normalSegmentDot / dSqrt(lengthSquareProduct);

    if (angleCosine < REAL(1.0))
    {
        dVector3 normalSecondOppositeSegmentCross;
        dCalcVectorCross3(normalSecondOppositeSegmentCross, triangleNormal, secondOppositeVertexSegment);

        dReal secondTriangleEdgeDirectionCheck;

        if (pSecondTriangleMatchingEdge != NULL)
        {
            // Check the cross product against the second triangle edge, if possible...
            secondTriangleEdgeDirectionCheck = dCalcVectorDot3(normalSecondOppositeSegmentCross, *pSecondTriangleMatchingEdge);
        }
        else
        {
            // ...if not, calculate the supposed direction of the second triangle's edge 
            // as negative of first triangle edge. For that cross-multiply the precomputed
            // first triangle normal by vector from the degenerate edge to its opposite vertex.

            // Retrieve the first triangle points if necessary
            dVector3 firstTriangleStorage[dMTV__MAX];
            const dVector3 *pFirstTriangleToUse = pFirstTriangle;

            if (pFirstTriangle == NULL)
            {
                dataAccessor.getTriangleVertexPoints(firstTriangleStorage, currEdge[0].m_triIdx);
                pFirstTriangleToUse = &firstTriangleStorage[dMTV__MIN];
            }

            // Calculate the opposite vector
            unsigned firstTriangleOppositeIndex = firstVertexStartIndex != dMTV__MIN ? firstVertexStartIndex - 1 : dMTV__MAX - 1;

            dVector3 firstOppositeVertexSegment;
            dSubtractVectors3(firstOppositeVertexSegment, pFirstTriangleToUse[firstTriangleOppositeIndex], pFirstTriangleToUse[firstVertexStartIndex]);

            dVector3 normalFirstOppositeSegmentCross;
            dCalcVectorCross3(normalFirstOppositeSegmentCross, triangleNormal, firstOppositeVertexSegment);

            // And finally calculate the dot product to compare vector directions
            secondTriangleEdgeDirectionCheck = dCalcVectorDot3(normalSecondOppositeSegmentCross, normalFirstOppositeSegmentCross);
        }

        // Negative product means the angle absolute value is less than M_PI_2, positive - greater.
        result = secondTriangleEdgeDirectionCheck < REAL(0.0) ? dAsin(angleCosine) : (dReal)M_PI_2 + dAcos(angleCosine);
    }
    else
    {
        result = (dReal)M_PI_2;
        dIASSERT(angleCosine - REAL(1.0) < 1e-4); // The computational error can not be too high because the dot product had been verified to be greater than the concave threshold above
    }

    return result;
}


#endif // #if dTRIMESH_ENABLED


#endif // #ifndef _ODE_COLLISION_TRIMESH_INTERNAL_IMPL_H_
