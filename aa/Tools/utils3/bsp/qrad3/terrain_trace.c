#include "qrad.h"
#include <float.h>
#include <math.h>
#include <assert.h>

#define MAX_MAP_MODELS 1024

typedef struct cterraintri_s
{
	dplane_t	p;
	vec_t		*verts[3], *normals[3];
	vec3_t		mins, maxs;
} cterraintri_t;

// Terrain models are divided up into axis-aligned 3D grids of cube-shaped 
// volumes measuring TERRAIN_GRIDSIZE units on each side. Each grid cell has a
// list of all the triangles that go through it (a triangle may be in multiple
// grid cells.) This is used to speed up tracing/collision detection/occlusion
// by minimizing how many triangles need to be checked on any given trace.
typedef struct 
{
	vec3_t			mins, maxs;
	int				numtris;
	cterraintri_t	**tris;
} cterraingrid_t;

typedef struct
{
	qboolean		active;
	vec3_t			mins, maxs;
	int				numtriangles;
	vec_t			*verts;
	vec_t			*normals;
	cterraintri_t	*tris;
	
	// for generating lightamps light levels
	const char		*lightmap_path;
	int				lm_w, lm_h;
	vec3_t			lm_s_axis, lm_t_axis;
	float			lm_mins[2], lm_size[2];
	
	// For grouping triangles into grids
#define TERRAIN_GRIDSIZE 500
#define NUMGRID(terrainmod) (terrainmod->numgrid[0] * terrainmod->numgrid[1] * terrainmod->numgrid[2])
#define GRIDNUM(terrainmod,coord) \
	(((int)floor(coord[2]-terrainmod->mins[2]) / TERRAIN_GRIDSIZE) * terrainmod->numgrid[0]*terrainmod->numgrid[1] \
	(int)floor((coord[1]-terrainmod->mins[1]) / TERRAIN_GRIDSIZE) * terrainmod->numgrid[0] + \
	(int)floor((coord[0]-terrainmod->mins[0]) / TERRAIN_GRIDSIZE))
	int numgrid[3]; // how many grid planes there are on each axis.
	cterraingrid_t	*grids;
} cterrainmodel_t;

static int		numterrainmodels;
cterrainmodel_t	terrain_models[MAX_MAP_MODELS];

static float RayFromSegment (const vec3_t start, const vec3_t end, vec3_t out_dir)
{
	VectorSubtract (end, start, out_dir);
	return VectorNormalize (out_dir, out_dir);
}

/* 
Fast Ray-Box Intersection
by Andrew Woo
from "Graphics Gems", Academic Press, 1990
The copyright license of "Graphics Gems" permits use of this code.
*/
qboolean RayIntersectsBBox (const vec3_t origin, const vec3_t dir, const vec3_t mins, const vec3_t maxs, float *out_intersection_dist)
{
	qboolean inside = true;
	enum {
		right,
		left,
		middle
	} quadrant[3];
	int i;
	int whichPlane;
	vec3_t maxT;
	vec3_t candidatePlane;
	vec3_t coord;

	/* Find candidate planes; this loop can be avoided if
   	rays cast all from the eye(assume perpsective view) */
	for (i = 0; i < 3; i++)
	{
		if (origin[i] < mins[i])
		{
			quadrant[i] = left;
			candidatePlane[i] = mins[i];
			inside = false;
		}
		else if (origin[i] > maxs[i])
		{
			quadrant[i] = right;
			candidatePlane[i] = maxs[i];
			inside = false;
		}
		else
		{
			quadrant[i] = middle;
		}
	}

	/* Ray origin inside bounding box */
	if(inside)
	{
		*out_intersection_dist = 0;
		return true;
	}
	

	/* Calculate T distances to candidate planes */
	for (i = 0; i < 3; i++)
	{
		if (quadrant[i] != middle && dir[i] != 0.0)
			maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
		else
			maxT[i] = -1.0;
	}

	/* Get largest of the maxT's for final choice of intersection */
	whichPlane = 0;
	for (i = 1; i < 3; i++)
	{
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;
	}

	/* Check final candidate actually inside box */
	if (maxT[whichPlane] < 0.0)
		return false;
	for (i = 0; i < 3; i++)
	{
		if (whichPlane != i)
		{
			coord[i] = origin[i] + maxT[whichPlane] * dir[i];
			if (coord[i] < mins[i] || coord[i] > maxs[i])
				return false;
		}
		else
		{
			coord[i] = candidatePlane[i];
		}
	}
	
	VectorSubtract (coord, origin, coord);
	*out_intersection_dist = VectorLength (coord);
	
	return true;				/* ray hits box */
}

static qboolean RayIntersectsTriangle (const vec3_t p1, const vec3_t d, const vec3_t v0, const vec3_t v1, const vec3_t v2, float *out_intersection_dist, float *out_u, float *out_v)
{
	vec3_t	e1, e2, P, Q, T;
	float	det, inv_det, u, v, t;
	
	VectorSubtract (v1, v0, e1);
	VectorSubtract (v2, v0, e2);
	
	CrossProduct (d, e2, P);
	det = DotProduct (e1, P);
	
	if (fabs (det) < FLT_EPSILON)
		return false;
	
	inv_det = 1.0/det;
	
	VectorSubtract (p1, v0, T);
	u = inv_det * DotProduct (P, T);
	
	if (u < 0.0 || u > 1.0)
		return false;
	
	CrossProduct (T, e1, Q);
	v = inv_det * DotProduct (d, Q);
	
	if (v < 0.0 || u + v > 1.0)
		return false;
	
	if (out_u)
		*out_u = u;
	if (out_v)
		*out_v = v;
	
	t = inv_det * DotProduct (e2, Q);
	
	*out_intersection_dist = t;
	
	return t > FLT_EPSILON;
}

// NOTE: this function doesn't need to be fast, it only runs at load time.
qboolean TriangleIntersectsBBox (const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec3_t mins, const vec3_t maxs)
{
	int i, axis;
	float l0, l1, l2;
	vec3_t d0, d1, d2;
	qboolean all_in;
	float dist;
	
	all_in = true;
	for (i = 0; i < 3; i++)
	{
		float trimin, trimax;
		
		trimin = trimax = v0[i];
		
		if (v1[i] < trimin)
			trimin = v1[i];
		else if (v1[i] > trimax)
			trimax = v1[i];
		
		if (v2[i] < trimin)
			trimin = v2[i];
		else if (v2[i] > trimax)
			trimax = v2[i];
		
		if (trimin > maxs[i])
			return false; // all triangle points are outside the bbox
		else if (trimax < mins[i])
			return false; // all triangle points are outside the bbox
		
		if (trimax > maxs[i] || trimin < mins[i])
			all_in = false; // at least one triangle point is outside the bbox
	}
	
	if (all_in)
		return true; // all triangle points are inside the bbox
	
	// check if one of the edges of the bounding box goes through the triangle
	for (axis = 0; axis < 3; axis++)
	{
		for (i = 0; i < 4; i++)
		{
			vec3_t start;
			vec3_t dir = {0, 0, 0};
			
			dir[axis] = -1;
			
			VectorCopy (maxs, start);
			if ((i & 1) != 0)
				start[(axis+1)%3] = mins[(axis+1)%3];
			if ((i & 2) != 0)
				start[(axis+2)%3] = mins[(axis+2)%3];
		
			if (RayIntersectsTriangle (start, dir, v0, v1, v2, &dist, NULL, NULL) && dist <= (maxs[axis] - mins[axis]))
				return true;
		}
	}
	
	// check if one of the edges of the triangle goes through the bbox
	l0 = RayFromSegment (v0, v1, d0);
	l1 = RayFromSegment (v1, v2, d1);
	l2 = RayFromSegment (v2, v0, d2);
	
	return	(RayIntersectsBBox (v0, d0, mins, maxs, &dist) && dist <= l0) ||
			(RayIntersectsBBox (v1, d1, mins, maxs, &dist) && dist <= l1) ||
			(RayIntersectsBBox (v2, d2, mins, maxs, &dist) && dist <= l2);
}

void AnglesToMatrix3x3 (vec3_t angles, float rotation_matrix[3][3])
{
	float cosyaw, cospitch, cosroll, sinyaw, sinpitch, sinroll;
	
	cosyaw = cos (DEG2RAD (angles[YAW]));
	cospitch = cos (DEG2RAD (angles[PITCH]));
	cosroll = cos (DEG2RAD (angles[ROLL]));
	sinyaw = sin (DEG2RAD (angles[YAW]));
	sinpitch = sin (DEG2RAD (angles[PITCH]));
	sinroll = sin (DEG2RAD (angles[ROLL]));
	
	rotation_matrix[0][0] = cosyaw*cospitch;
	rotation_matrix[0][1] = cosyaw*sinpitch*sinroll - sinyaw*cosroll;
	rotation_matrix[0][2] = cosyaw*sinpitch*cosroll + sinyaw*sinroll;
	rotation_matrix[1][0] = sinyaw*cospitch;
	rotation_matrix[1][1] = sinyaw*sinpitch*sinroll + cosyaw*cosroll;
	rotation_matrix[1][2] = sinyaw*sinpitch*cosroll - cosyaw*sinroll;
	rotation_matrix[2][0] = -sinpitch;
	rotation_matrix[2][1] = cospitch*sinroll;
	rotation_matrix[2][2] = cospitch*cosroll;
}

void CM_LoadTerrainModel (char *name, vec3_t angles, vec3_t origin)
{
	float rotation_matrix[3][3];
	int i, j, k, l, m;
	cterrainmodel_t *mod;
	char newname[4096], extension[32];
	char *buf;
	terraindata_t data;
	cterraintri_t **tmp;
	vec3_t up, lm_mins, lm_maxs;
	
	if (numterrainmodels == MAX_MAP_MODELS)
	{
		printf ("CM_LoadTerrainModel: MAX_MAP_MODELS\n");
		assert (false);
	}
	
	// FIXME HACK!!!
	sprintf (newname, "%s%s", moddir, name);
	LoadFile (newname, (void**)&buf);
	
	if (!buf)
	{
		printf ("CM_LoadTerrainModel: Missing terrain model %s!\n", name);
		assert (false);
	}
	
	ExtractFileExtension (name, extension);
	if (Q_strcasecmp (extension, "terrain"))
	{
		printf ("CM_LoadTerrainModel: Can only handle .terrain meshes so far!\n");
		return;
	}
	
	AnglesToMatrix3x3 (angles, rotation_matrix);
	
	LoadTerrainFile (&data, name, 0.5, 8, buf);
	
	mod = &terrain_models[numterrainmodels++];
	
	if (data.lightmap_path != NULL)
		mod->lightmap_path = CopyString (data.lightmap_path);
	else
		mod->lightmap_path = NULL;
	
	mod->active = true;
	mod->numtriangles = 0;
	
	mod->verts = Z_Malloc (data.num_vertices*sizeof(vec3_t));
	mod->tris = Z_Malloc (data.num_triangles*sizeof(cterraintri_t));
	mod->normals = Z_Malloc (data.num_vertices*sizeof(vec3_t));
	
	// Technically, because the mins can be positive or the maxs can be 
	// negative, this isn't always correct, but it errs on the side of more
	// inclusive, so it's all right.
	VectorCopy (origin, mod->mins);
	VectorCopy (origin, mod->maxs);
	
	for (i = 0; i < data.num_vertices; i++)
	{
		for (j = 0; j < 3; j++)
		{
			mod->verts[3*i+j] = origin[j];
			for (k = 0; k < 3; k++)
				mod->verts[3*i+j] += data.vert_positions[3*i+k] * rotation_matrix[j][k];
				
			if (mod->verts[3*i+j] < mod->mins[j])
				mod->mins[j] = mod->verts[3*i+j];
			else if (mod->verts[3*i+j] > mod->maxs[j])
				mod->maxs[j] = mod->verts[3*i+j];
		}
	}
	
	VectorSet (mod->lm_s_axis, rotation_matrix[0][0], rotation_matrix[1][0], rotation_matrix[2][0]);
	VectorSet (mod->lm_t_axis, rotation_matrix[0][1], rotation_matrix[1][1], rotation_matrix[2][1]);
	VectorSet (up, rotation_matrix[0][2], rotation_matrix[1][2], rotation_matrix[2][2]);
	
	for (j = 0; j < 3; j++)
	{
		lm_mins[j] = lm_maxs[j] = origin[j];
		for (k = 0; k < 3; k++)
		{
			lm_mins[j] += data.mins[k] * rotation_matrix[j][k];
			lm_maxs[j] += data.maxs[k] * rotation_matrix[j][k];
		}
	}
	
	mod->lm_mins[0] = DotProduct (lm_mins, mod->lm_s_axis);
	mod->lm_mins[1] = DotProduct (lm_mins, mod->lm_t_axis);
	mod->lm_size[0] = DotProduct (lm_maxs, mod->lm_s_axis) - mod->lm_mins[0];
	mod->lm_size[1] = DotProduct (lm_maxs, mod->lm_t_axis) - mod->lm_mins[1];
	
	// Output lightmap is the same size as heightmap
	mod->lm_w = data.heightmap_w*terrain_refine;
	mod->lm_h = data.heightmap_h*terrain_refine;
	
	for (i = 0; i < data.num_triangles; i++)
	{
		cterraintri_t *tri;
		vec3_t side1, side2;
		int j, k;
		
		tri = &mod->tris[mod->numtriangles];
		
		for (j = 0; j < 3; j++)
		{
			tri->verts[j] = &mod->verts[3*data.tri_indices[3*i+j]];
			tri->normals[j] = &mod->normals[3*data.tri_indices[3*i+j]];
		}
		
		VectorCopy (tri->verts[0], tri->mins);
		VectorCopy (tri->verts[0], tri->maxs);
		
		for (j = 1; j < 3; j++)
		{
			for (k = 0; k < 3; k++)
			{
				if (tri->verts[j][k] < tri->mins[k])
					tri->mins[k] = tri->verts[j][k];
				else if (tri->verts[j][k] > tri->maxs[k])
					tri->maxs[k] = tri->verts[j][k];
			}
		}
		
		VectorSubtract (tri->verts[1], tri->verts[0], side1);
		VectorSubtract (tri->verts[2], tri->verts[0], side2);
		CrossProduct (side2, side1, tri->p.normal);
		VectorNormalize (tri->p.normal, tri->p.normal);
		tri->p.dist = DotProduct (tri->verts[0], tri->p.normal);
		
		// overwrite and don't use downward facing faces.
		if (DotProduct (tri->p.normal, up) > 0)
			mod->numtriangles++;
		
		for (j = 0; j < 3; j++)
			VectorAdd (tri->normals[j], tri->p.normal, tri->normals[j]);
	}
	
	for (i = 0; i < data.num_vertices; i++)
		VectorNormalize (&mod->normals[3*i], &mod->normals[3*i]);
	
	if (data.num_triangles != mod->numtriangles)
		printf ("WARN: %d downward facing collision polygons in %s!\n", data.num_triangles - mod->numtriangles, name);
	
	for (i = 0; i < 3; i++)
		mod->numgrid[i] = ceil((mod->maxs[i] - mod->mins[i]) / TERRAIN_GRIDSIZE + 0.1);
	
	mod->grids = Z_Malloc (NUMGRID(mod) * sizeof(cterraingrid_t));
	tmp = Z_Malloc (mod->numtriangles * sizeof(cterraintri_t *));
	for (i = 0; i < mod->numgrid[1]; i++)
	{
		for (j = 0; j < mod->numgrid[0]; j++)
		{
			for (l = 0; l < mod->numgrid[2]; l++)
			{
				vec3_t tmpmaxs, tmpmins;
				
				cterraingrid_t *grid = &mod->grids[l*mod->numgrid[0]*mod->numgrid[1]+i*mod->numgrid[0]+j];
				grid->numtris = 0;
				
				grid->mins[0] = mod->mins[0] + j*TERRAIN_GRIDSIZE;
				grid->maxs[0] = mod->mins[0] + (j+1)*TERRAIN_GRIDSIZE;
				grid->mins[1] = mod->mins[1] + i*TERRAIN_GRIDSIZE;
				grid->maxs[1] = mod->mins[1] + (i+1)*TERRAIN_GRIDSIZE;
				grid->mins[2] = mod->mins[2] + l*TERRAIN_GRIDSIZE;
				grid->maxs[2] = mod->mins[2] + (l+1)*TERRAIN_GRIDSIZE;
			
				for (k = 0; k < mod->numtriangles; k++)
				{
					cterraintri_t *tri = &mod->tris[k];
					if (TriangleIntersectsBBox (tri->verts[0], tri->verts[1], tri->verts[2], grid->mins, grid->maxs))
						tmp[grid->numtris++] = tri;
				}
				
				if (grid->numtris == 0)
					continue;
			
				grid->tris = Z_Malloc (grid->numtris * sizeof(cterraintri_t *));
				for (k = 0; k < grid->numtris; k++)
					grid->tris[k] = tmp[k];
				
				// Try shrinking the grid's bounding box
				
				VectorCopy (grid->tris[0]->mins, tmpmins);
				VectorCopy (grid->tris[0]->maxs, tmpmaxs);
				
				for (k = 1; k < grid->numtris; k++)
				{
					for (m = 0; m < 3; m++)
					{
						if (grid->tris[k]->maxs[m] > tmpmaxs[m])
							tmpmaxs[m] = grid->tris[k]->maxs[m];
						if (grid->tris[k]->mins[m] < tmpmins[m])
							tmpmins[m] = grid->tris[k]->mins[m];
					}
				}
				
				for (m = 0; m < 3; m++)
				{
					if (tmpmaxs[m] < grid->maxs[m])
						grid->maxs[m] = tmpmaxs[m];
					if (tmpmins[m] > grid->mins[m])
						grid->mins[m] = tmpmins[m];
				}
			}
		}
	}
	Z_Free (tmp);
	
	CleanupTerrainData (&data);
	
	free (buf);
}

void LoadAllTerrain (void)
{
	int 		i;
	entity_t	*e;
	vec3_t 		origin, angles;
	char		*name, *proc_num;
	
	for (i=0 ; i<num_entities ; i++)
	{
		e = &entities[i];
		
		name = ValueForKey (e, "classname");
		if (strncmp (name, "misc_terrainmodel", 18))
			continue;
		
		VectorClear (origin);
		VectorClear (angles);
		
		proc_num = ValueForKey (e, "origin");
		if (strlen(proc_num) > 0)
			GetVectorForKey (e, "origin", origin);
		
		proc_num = ValueForKey (e, "angles");
		if (strlen(proc_num) > 0)
			GetVectorForKey (e, "angles", angles);
		
		proc_num = ValueForKey (e, "angle");
		if (strlen(proc_num) > 0)
			angles[YAW] = atof (proc_num);
		
		proc_num = ValueForKey (e, "model");
		
		CM_LoadTerrainModel (proc_num, angles, origin);
	}
}

static qboolean bbox_in_trace (vec3_t box_mins, vec3_t box_maxs, vec3_t p1, vec3_t p2_extents)
{
	int	i;
	
	
	for (i = 0; i < 3; i++)
	{
		if (	(p1[i] > box_maxs[i] && p2_extents[i] > box_maxs[i]) ||
				(p1[i] < box_mins[i] && p2_extents[i] < box_mins[i]))
			return false;
	}
	
	return true;
}

// FIXME: It's still quite possible to fall through a terrain mesh.
// FIXME: Also, the physics feel like crap on terrain.
// Returns -1 for no intersection, otherwise it returns the index within 
// terrain_models where the intersection was found (used by
// CM_TerrainLightPoint.)
int CM_TerrainTrace (vec3_t p1, vec3_t end, vec3_t out_normal, float *out_fraction, cterraintri_t **out_occluder)
{
	vec3_t		p2;
	int			i, j, k, x, y, z;
	dplane_t	*plane;
	vec3_t		dir;
	float		orig_dist;
	vec3_t		p2_extents;
	int			ret = -1;
	
	for (i = 0; i < 3; i++)
		p2[i] = p1[i] + *out_fraction * (end[i] - p1[i]);
	
	VectorSubtract (p2, p1, dir);
	orig_dist = VectorNormalize (dir, dir); // Update this every time p2 changes
	
	// Update this every time p2 changes
	for (k = 0; k < 3; k++) p2_extents[k] = p2[k];
	
	for (i = 0; i < numterrainmodels; i++)
	{
		int mingrid[3], maxgrid[3], griddir[3];
		cterrainmodel_t	*mod = &terrain_models[i];
		
		if (!mod->active)
			continue;
		
		if (!bbox_in_trace (mod->mins, mod->maxs, p1, p2_extents))
			continue;
		
		// We iterate through the grid layers, rows, and columns in order from
		// closest to furthest. 
		for (k = 0; k < 3; k++)
		{
			float mincoord, maxcoord, tmp;
			
			mincoord = maxcoord = p1[k];
			if (p2_extents[k] < mincoord)
				mincoord = p2_extents[k];
			else if (p2_extents[k] > maxcoord)
				maxcoord = p2_extents[k];
			
			mingrid[k] = (int)floor((mincoord-mod->mins[k]) / TERRAIN_GRIDSIZE);
			if (mingrid[k] < 0)
				mingrid[k] = 0;
			
			maxgrid[k] = (int)ceil((maxcoord-mod->mins[k]) / TERRAIN_GRIDSIZE + 0.1);
			if (maxgrid[k] > mod->numgrid[k])
				maxgrid[k] = mod->numgrid[k];
			
			if (dir[k] < 0.0)
			{
				griddir[k] = -1;
				tmp = mingrid[k] - 1;
				mingrid[k] = maxgrid[k] - 1;
				maxgrid[k] = tmp;
			}
			else
			{
				griddir[k] = 1;
			}
		}
		
		for (z = mingrid[2]; z != maxgrid[2]; z += griddir[2])
		{
			for (y = mingrid[1]; y != maxgrid[1]; y += griddir[1])
			{
				for (x = mingrid[0]; x != maxgrid[0]; x += griddir[0])
				{
					float tmp;
					cterraingrid_t *grid;
					
					grid = &mod->grids[z*mod->numgrid[0]*mod->numgrid[1]+y*mod->numgrid[0]+x];
					
					if (grid->numtris == 0)
						continue;
					
					if (!bbox_in_trace (grid->mins, grid->maxs, p1, p2_extents))
						continue;
					
					if (!RayIntersectsBBox (p1, dir, grid->mins, grid->maxs, &tmp))
						continue;
		
					for (j = 0; j < grid->numtris; j++)
					{
						float			intersection_dist, u, v;
						cterraintri_t	*tri = grid->tris[j];
			
						plane = &tri->p;
			
						// order the tests from least expensive to most
			
						if (DotProduct (dir, plane->normal) > 0)
							continue;
			
						if (!bbox_in_trace (tri->mins, tri->maxs, p1, p2_extents))
							continue;
			
						if (!RayIntersectsTriangle (p1, dir, tri->verts[0], tri->verts[1], tri->verts[2], &intersection_dist, &u, &v))
							continue;
			
						if (intersection_dist > orig_dist)
							continue;
			
						// At this point, we've found a new closest intersection point
						*out_fraction *= intersection_dist / orig_dist;
						VectorMA (p1, intersection_dist, dir, p2);
						orig_dist = intersection_dist;
						
						// Generate an interpolated normal for the point. This
						// results in smooth shading.
						for (k = 0; k < 3; k++)
							out_normal[k] = (1.0 - u - v) * tri->normals[0][k] + u * tri->normals[1][k] + v * tri->normals[2][k];
						VectorNormalize (out_normal, out_normal);
						
						for (k = 0; k < 3; k++) p2_extents[k] = p2[k];
						ret = i;
						*out_occluder = tri;
					}
					
					// Any grid cells we check after this one are further away
					// than this one, so they won't contain any triangles that
					// are actually *closer* that we haven't checked already.
					// Thus, if we've found any intersections in the previous
					// grid cell, there won't be any closer intersections in
					// the next one, so we can stop the tracing right here.
					if (ret != -1)
						goto done;
				}
			}
		}
	}
	
done:
	return ret;
}

qboolean Terrain_Trace (vec3_t start, vec3_t end, vec3_t out_end, vec3_t out_normal)
{
	vec3_t dir;
	float orig_dist;
	cterraintri_t *tmp;
	float fraction = 1.0;
	
	CM_TerrainTrace (start, end, out_normal, &fraction, &tmp);
	
	if (fraction == 1.0)
		return true;
	
	VectorSubtract (end, start, dir);
	orig_dist = VectorNormalize (dir, dir);
	VectorMA (start, orig_dist * fraction, dir, out_end);
	
	return false;
}

qboolean Fast_Terrain_Trace_Try_Cache (vec3_t start, vec3_t end, occlusioncache_t *cache)
{
	float u, v, intersection_dist, orig_dist;
	vec3_t dir;
	
	VectorSubtract (end, start, dir);
	orig_dist = VectorNormalize (dir, dir);
	
	if (cache->mru != NULL)
	{
		if (	RayIntersectsTriangle (	start, dir,
										cache->mru->verts[0],
										cache->mru->verts[1],
										cache->mru->verts[2],
										&intersection_dist, &u, &v) && 
				intersection_dist < orig_dist )
			return false;
	}
	
	if (cache->lru != NULL)
	{
		if (	RayIntersectsTriangle (	start, dir,
										cache->lru->verts[0],
										cache->lru->verts[1],
										cache->lru->verts[2],
										&intersection_dist, &u, &v) && 
				intersection_dist < orig_dist )
		{
			cterraintri_t *tmp = cache->mru;
			cache->mru = cache->lru;
			cache->lru = tmp;
			return false;
		}
	}
	
	return true;
}

qboolean Fast_Terrain_Trace_Cache_Miss (vec3_t start, vec3_t end, occlusioncache_t *cache)
{
	vec3_t normal;
	float fraction = 1.0;
	
	cache->lru = cache->mru;
	CM_TerrainTrace (start, end, normal, &fraction, &cache->mru);
	
	return fraction == 1.0;
}

// On maps with sunlight, a grid of light sources is created along each sky
// surface. However, when computing the amount of illumination from sunlight,
// only one light source per sample ends up actually mattering. If a sample
// point can "see" a sky surface, it gets the ambient sunlight amount added to
// it. If it can "see" a sky surface in the direction of the sun entity, it
// gets the ambient *and* direct sunlight amounts added to it. To avoid having
// to do occlusion checks on each and every sun/sky light source for each and
// every sample point, we cache the most recent sun light source that had an
// effect and check that first. If that fails, we check other lights in the
// cluster of the most recent light. Only if that fails to we check the lights
// in the other clusters.
typedef struct
{
	directlight_t	*light;
	int				clusternum;
} suncache_t;

static void TerrainPointLight_TryCluster (	int clusternum,
											vec3_t surface_pos, vec3_t normal,
											sample_t *out_color,
											occlusioncache_t *cache,
											suncache_t *suncache,
											qboolean *sun_main_once,
											qboolean *sun_ambient_once )
{
	directlight_t	*l;
	sample_blur_t	blur; //TODO: do something with this!
	sample_t		sample;
	
	for (l=directlights[clusternum]; l != NULL; l=l->next)
	{
		qboolean sun_main_already = *sun_main_once;
		
		// Make sure not to double-count any lights.
		if (l == suncache->light)
			continue;
		
		LightContributionToPoint (l, surface_pos, 0, normal, &sample, 1.0, sun_main_once, sun_ambient_once, blur, cache);
		
		// If this light is a sky/sun light and it illuminated this
		// sample, cache it. We may "evict" a light that only does ambient
		// illumination in favor of one that does direct illumination too,
		// since those are better to cache.
		if ((!sun_main_already && *sun_main_once) || (*sun_ambient_once && suncache->light == NULL))
		{
			suncache->light = l;
			suncache->clusternum = clusternum;
		}
		
		VectorAdd (out_color->direct, sample.direct, out_color->direct);
		VectorAdd (out_color->directsun, sample.directsun, out_color->directsun);
		VectorAdd (out_color->indirect, sample.indirect, out_color->indirect);
	}
}

static void TerrainPointLight (vec3_t pos, sample_t *out_color, occlusioncache_t *cache, suncache_t *suncache)
{
	int				i;
	qboolean		sun_main_once = false, sun_ambient_once = false;
	sample_blur_t	blur; // FIXME: do something with this!
	vec3_t			normal;
	vec3_t			start, end;
	vec3_t			surface_pos;
	
	VectorCopy (pos, start);
	start[2] += 2048;
	VectorCopy (pos, end);
	end[2] -= 2048;
	
	Terrain_Trace (start, end, surface_pos, normal);
	
	// This eliminates some pixel checkerboard/aliasing patterns. We want to
	// avoid having the traces "skim" across the surface of the terrain and 
	// intersect or miss it incorrectly due to FP inaccuracy.
	VectorAdd (surface_pos, normal, surface_pos);
	
	memset (out_color, 0, sizeof (*out_color));
	
	if (suncache->light != NULL)
	{
		LightContributionToPoint (suncache->light, surface_pos, 0, normal, out_color, 1.0, &sun_main_once, &sun_ambient_once, blur, cache);
		if (!sun_ambient_once)
			suncache->light = NULL; // The light did nothing
	}
	
	TerrainPointLight_TryCluster (suncache->clusternum, surface_pos, normal, out_color, cache, suncache, &sun_main_once, &sun_ambient_once);
	
	for (i = 0 ; i<dvis->numclusters ; i++)
	{
		if (i == suncache->clusternum)
			continue;
		TerrainPointLight_TryCluster (i, surface_pos, normal, out_color, cache, suncache, &sun_main_once, &sun_ambient_once);
	}
}

static cterrainmodel_t *generate_mod;
static byte *tex[4];
static void GenerateTerrainModelLightmapRow (int t)
{
	int s, i;
	vec3_t oloop_pos;
	occlusioncache_t cache;
	suncache_t suncache;
	cterrainmodel_t *mod = generate_mod;
	
	memset (&cache, 0, sizeof(cache));
	memset (&suncache, 0, sizeof(suncache));
	
	VectorMA (mod->mins, mod->lm_size[1]*(float)t/(float)mod->lm_h, mod->lm_t_axis, oloop_pos);
	for (s = 0; s < mod->lm_w; s++)
	{
		vec3_t iloop_pos, combined_color;
		sample_t sample, processed;

		VectorMA (oloop_pos, mod->lm_size[0]*(float)s/(float)mod->lm_w, mod->lm_s_axis, iloop_pos);

		TerrainPointLight (iloop_pos, &sample, &cache, &suncache);

		PostProcessLightSample (&sample, vec3_origin, &processed, combined_color);

		for (i = 0; i < 3; i++)
		{
			tex[0][3*(mod->lm_w*t+s)+i] = (byte)(combined_color[i] + 0.5);
			tex[1][3*(mod->lm_w*t+s)+i] = (byte)(processed.direct[i] + 0.5);
			tex[2][3*(mod->lm_w*t+s)+i] = (byte)(processed.directsun[i] + 0.5);
			tex[3][3*(mod->lm_w*t+s)+i] = (byte)(processed.indirect[i] + 0.5);
		}
	}
}

// FIXME: more code sharing with CRX engine would eliminate some of this
static void GenerateTerrainModelLightmap (cterrainmodel_t *mod, const char *mapname, int modelnum)
{
    int i;
    char template[1024], lightmap_base[1024], generated_name[1024];
    const char *channel_names[4] = {"combined", "direct", "directsun", "indirect"};

	if (mod->lightmap_path == NULL)
		return;
	
	generate_mod = mod;
	for (i = 0; i < 4; i++)
		tex[i] = Z_Malloc (mod->lm_h*mod->lm_w*3);
	
	// parallelize across rows
	RunThreadsOnIndividual (mod->lm_h, true, GenerateTerrainModelLightmapRow);

	strcpy (lightmap_base, mod->lightmap_path);
	StripExtension (lightmap_base);
	sprintf	(template, "%s_%s_%d", lightmap_base, mapname, modelnum);
	for (i = 0; i < 4; i++)
	{
		sprintf (generated_name, "%s_%s.tga", template, channel_names[i]);
		printf ("Creating %s\n", generated_name);
		assert (strlen (generated_name) - strlen (moddir) < 64 && "TGA name is too long! Consider setting the \"lightmap\" value in the .terrain file to something shorter!");
		SaveTGA (tex[i], mod->lm_w, mod->lm_h, generated_name);
	}
}

void GenerateAllTerrainLightmaps (const char *mapname)
{
	int i;
	
	for (i = 0; i < numterrainmodels; i++)
		GenerateTerrainModelLightmap (&terrain_models[i], mapname, i+1);
}
