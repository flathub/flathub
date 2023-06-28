/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
// cmodel.c -- model loading

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

#include <float.h>

typedef struct
{
	int			contents;				// contents of all children OR'd together
	qboolean	containsgap;
	// largest contiguous empty box volume in the node
	vec3_t		emptymins, emptymaxs;
	cplane_t	*plane;
	int			children[2];			// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t	*plane;
	mapsurface_t	*surface;
} cbrushside_t;

typedef struct
{
	int			contents;
	int			cluster;
	int			area;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	vec3_t		mins, maxs;		// bbox read in from BSP file
} cleaf_t;

typedef struct
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings (FIXME: not threadsafe!)
} cbrush_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
} carea_t;

int			checkcount;

char		map_name[MAX_QPATH];

int			numbrushsides;
cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES];

int			numtexinfo;
mapsurface_t	map_surfaces[MAX_MAP_TEXINFO];

int			numplanes;
cplane_t	map_planes[MAX_MAP_PLANES+6];		// extra for box hull

int			numnodes;
cnode_t		map_nodes[MAX_MAP_NODES+6];		// extra for box hull

int			numleafs = 1;	// allow leaf funcs to be called without a map
cleaf_t		map_leafs[MAX_MAP_LEAFS];
int			emptyleaf, solidleaf;

int			numleafbrushes;
unsigned short	map_leafbrushes[MAX_MAP_LEAFBRUSHES];

int			numcmodels;
cmodel_t	map_cmodels[MAX_MAP_MODELS];

int			numbrushes;
cbrush_t	map_brushes[MAX_MAP_BRUSHES];

int			numvisibility;
byte		map_visibility[MAX_MAP_VISIBILITY];
dvis_t		*map_vis = (dvis_t *)map_visibility;

int			numentitychars;
char		map_entitystring[MAX_MAP_ENTSTRING];

int			numareas = 1;
carea_t		map_areas[MAX_MAP_AREAS];

int			numareaportals;
dareaportal_t map_areaportals[MAX_MAP_AREAPORTALS];

int			numclusters = 1;

mapsurface_t	nullsurface;

int			floodvalid;

qboolean	portalopen[MAX_MAP_AREAPORTALS];

typedef struct
{
	// allplanes is treaded as an array with 10 members. We do this so that
	// the planes get checked in the most efficient order.
	union
	{
		cplane_t	allplanes[10];
		struct
		{
			cplane_t	bboxplanes[6];
			cplane_t	mainplane;
			cplane_t	sideplanes[3];
		} planes;
	};
	vec_t		*verts[3];
	vec3_t		mins, maxs;
	int			neighbors[3];	// neighbors[i] shares verts i and (i+1)%3
	char		neighbors_whichedge[3];
	int			checkcount;		// to avoid repeated testings (FIXME: not threadsafe!)
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
	struct
	{
		vec3_t		normal;
		vec_t		dist;
	} boundingplane; // All tris are on one side of this plane
} cterraingrid_t;

typedef struct
{
	vec3_t			mins, maxs;
	int				numtriangles, numvertices;
	vec_t			*verts;
	cterraintri_t	*tris;
	
	// for calculating light levels
	int				lm_w, lm_h;
	vec3_t			lm_s_axis, lm_t_axis;
	float			lm_mins[2], lm_size[2];
	byte			*lightmaptex;
	
	// For grouping triangles into grids
#define TERRAIN_GRIDSIZE 250
#define NUMGRID(terrainmod) (terrainmod->numgrid[0] * terrainmod->numgrid[1] * terrainmod->numgrid[2])
#define GRIDNUM(terrainmod,coord) \
	(((int)floor(coord[2]-terrainmod->mins[2]) / TERRAIN_GRIDSIZE) * terrainmod->numgrid[0]*terrainmod->numgrid[1] \
	(int)floor((coord[1]-terrainmod->mins[1]) / TERRAIN_GRIDSIZE) * terrainmod->numgrid[0] + \
	(int)floor((coord[0]-terrainmod->mins[0]) / TERRAIN_GRIDSIZE))
	int numgrid[3]; // how many grid planes there are on each axis.
	cterraingrid_t	*grids;
	
	// For exporting all terrain geometry as if it was a single model: we need
	// to know how many vertexes and triangles preceded this one.
	int firsttriangle, firstvertex;
} cterrainmodel_t;

static int		numterrainmodels;
cterrainmodel_t	terrain_models[MAX_MAP_MODELS];


cvar_t		*map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


int		c_pointcontents;
int		c_traces, c_brush_traces;


/**
 * calculate the 3 signbits for a plane normal.
 *
 * @param  plane
 * @return the signbits
 */
static byte signbits_for_plane(const cplane_t *plane)
{
	byte bits;

	bits =  plane->normal[0] < 0.0f ? (byte)0x01 : 0 ;
	bits |= plane->normal[1] < 0.0f ? (byte)0x02 : 0 ;
	bits |= plane->normal[2] < 0.0f ? (byte)0x04 : 0 ;

	return bits;
}


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

byte	*cmod_base;

//Performs a sanity check on the BSP file, making sure all the lumps are nice
//and shipshape and won't lead off a cliff somewhere. This does mean that the 
//BSP file format has been made less extensible, but we've tended to add more
//data using separate files instead of adding lumps to the BSP, so I think we
//can live with that.
qboolean checkLumps (lump_t *l, size_t header_size, int *lump_order, void *_file_base, int num_lumps, int file_len)
{
	int i = 0;
	lump_t *in;
	byte *lumpdata_base;
	byte *file_base = (byte *)_file_base;
	byte *lumpdata_next = file_base+header_size+sizeof(lump_t)*num_lumps;
	byte *file_end = file_base + file_len;
	for (i = 0; i < num_lumps; i++)
	{
		in = l+lump_order[i];
		lumpdata_base = file_base+in->fileofs;
		if (lumpdata_base != lumpdata_next)
			return true;
		lumpdata_next = lumpdata_base+((in->filelen+3)&~3);
		if (lumpdata_next < lumpdata_base)
			return true;
	}
	if (lumpdata_next != file_end)
		return true;
	return false;
}

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	cmodel_t	*out;
	int			i, j, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadSubmodels: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no models");
	if (count > MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "Map has too many models");

	numcmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headnode = LittleLong (in->headnode);
	}
}


/*
=================
CMod_LoadSurfaces
=================
*/
static void CMod_LoadSurfaces (lump_t *l)
{
	texinfo_t	*in;
	mapsurface_t	*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadSurfaces: funny lump size");
	count = l->filelen / sizeof(*in);
	if (count < 1)
		Com_Error (ERR_DROP, "Map with no surfaces");
	if (count > MAX_MAP_TEXINFO)
		Com_Error (ERR_DROP, "Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		strncpy (out->c.name, in->texture, sizeof(out->c.name)-1);
		strncpy (out->rname, in->texture, sizeof(out->rname)-1);
		out->c.flags = LittleLong (in->flags);
		out->c.value = LittleLong (in->value);
	}
}


/*
=================
CMod_LoadNodes
=================
*/
static void RecursiveSubtractLeafBBox (int num, vec3_t emptymins, vec3_t emptymaxs)
{
	int i;
	if (num < 0)
	{
		int		j, start_axis;
		cleaf_t	*leaf = &map_leafs[-1-num];
		
		if (leaf->contents == 0)
			return;
		
		for (i = 0; i < 3; i++)
		{
			if (leaf->mins[i] >= emptymaxs[i])
				return;
			if (leaf->maxs[i] <= emptymins[i])
				return;
		}
		start_axis = 0;
		for (i = 1; i < 3; i++)
		{
			if (leaf->maxs[i] - leaf->mins[i] < leaf->maxs[start_axis] - leaf->mins[start_axis])
				start_axis = i;
		}
		for (i = 0; i < 3; i++)
		{
			qboolean subtract_max, subtract_min;
			
			j = (i + start_axis) % 3;
			
			subtract_max = leaf->mins[j] < emptymaxs[j] && leaf->mins[j] > emptymins[j];
			subtract_min = leaf->maxs[j] > emptymins[j] && leaf->maxs[j] < emptymaxs[j];
			
			if (subtract_max && subtract_min)
			{
				if (emptymaxs[j] - leaf->mins[j] < leaf->maxs[j] - emptymins[j])
					subtract_min = false;
				else
					subtract_max = false;
			}
			if (subtract_max)
			{
				emptymaxs[j] = leaf->mins[j];
				return;
			}
			if (subtract_min)
			{
				emptymins[j] = leaf->maxs[j];
				return;
			}
		}
		VectorClear (emptymins);
		VectorClear (emptymaxs);
	}
	else if (map_nodes[num].contents)
	{
		RecursiveSubtractLeafBBox (map_nodes[num].children[0], emptymins, emptymaxs);
		RecursiveSubtractLeafBBox (map_nodes[num].children[1], emptymins, emptymaxs);
	}
}
static int RecursiveBuildNodeContents (int num)
{
	int i;
	cnode_t		*node;
	
	if (num < 0)
		return map_leafs[-1-num].contents;
	
	node = &map_nodes[num];
	
	RecursiveSubtractLeafBBox (node->children[0], node->emptymins, node->emptymaxs);
	RecursiveSubtractLeafBBox (node->children[1], node->emptymins, node->emptymaxs);
	
	node->containsgap = true;
	for (i = 0; i < 3; i++)
	{
		if (node->emptymins[i] + 8.0f > node->emptymaxs[i])
			node->containsgap = false;
	}
	
	return node->contents =
		RecursiveBuildNodeContents (node->children[0]) |
		RecursiveBuildNodeContents (node->children[1]);
}
static void CMod_LoadNodes (lump_t *l)
{
	dnode_t		*in;
	int			child;
	cnode_t		*out;
	int			i, j, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadNodes: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	if (count > MAX_MAP_NODES)
		Com_Error (ERR_DROP, "Map has too many nodes");

	out = map_nodes;

	numnodes = count;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->plane = map_planes + LittleLong(in->planenum);
		for (j = 0; j < 2; j++)
		{
			child = LittleLong (in->children[j]);
			out->children[j] = child;
		}
		for (j = 0; j < 3; j++)
		{
			// Start out empty volume at node size (will be narrowed down
			// later.)
			out->emptymins[j] = LittleShort (in->mins[j]);
			out->emptymaxs[j] = LittleShort (in->maxs[j]);
		}
		out->contents = ~0; // default contents (for brush models)
	}
	
	for (i = 0; i < numcmodels; i++)
		RecursiveBuildNodeContents (map_cmodels[i].headnode);
	
	for (i = 0 ; i < numnodes ; i++)
	{
		if ((map_nodes[i].contents & CONTENTS_ORIGIN))
			Com_Error (ERR_DROP, "Map has inaccessible nodes");
	}

}

/*
=================
CMod_LoadBrushes
=================
*/
static void CMod_LoadBrushes (lump_t *l)
{
	dbrush_t	*in;
	cbrush_t	*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadBrushes: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_BRUSHES)
		Com_Error (ERR_DROP, "Map has too many brushes");

	out = map_brushes;

	numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->firstbrushside = LittleLong(in->firstside);
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs (lump_t *l)
{
	int			i, j;
	cleaf_t		*out;
	dleaf_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadLeafs: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafs");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFS)
		Com_Error (ERR_DROP, "Map has too many leafs");

	out = map_leafs;
	numleafs = count;
	numclusters = 0;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->contents = LittleLong (in->contents);
		if (out->contents & CONTENTS_ORIGIN)
			Com_Error (ERR_DROP, "CONTENTS_ORIGIN appears in this BSP!");
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstleafbrush = LittleShort (in->firstleafbrush);
		out->numleafbrushes = LittleShort (in->numleafbrushes);
		for (j = 0; j < 3; j++)
		{
			// We expand the bbox by 1 in each direction because these are
			// stored as integers, and may be up to 0.99999... off.
			out->mins[j] = LittleShort (in->mins[j]) - 1;
			out->maxs[j] = LittleShort (in->maxs[j]) + 1;
		}

		if (out->cluster >= numclusters)
			numclusters = out->cluster + 1;
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");
	solidleaf = 0;
	emptyleaf = -1;
	for (i=1 ; i<numleafs ; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		Com_Error (ERR_DROP, "Map does not have an empty leaf");
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes (lump_t *l)
{
	int			i;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadPlanes: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no planes");
	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Map has too many planes");

	out = map_planes;
	numplanes = count;
	for ( ; count-- ; in++, out++)
	{
		for (i=0 ; i<3 ; i++)
		{
			out->normal[i] = LittleFloat( in->normal[i] );
		}
		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = signbits_for_plane( out );
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes (lump_t *l)
{
	int			i;
	unsigned short	*out;
	unsigned short 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadLeafBrushes: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafbrushes");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFBRUSHES)
		Com_Error (ERR_DROP, "Map has too many leafbrushes");

	out = map_leafbrushes;
	numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = LittleShort (*in);
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides (lump_t *l)
{
	int					i, j;
	cbrushside_t		*out;
	dbrushside_t 		*in;
	int					count;
	unsigned short		num;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadBrushSides: funny lump size");
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_MAP_BRUSHSIDES)
		Com_Error (ERR_DROP, "Map has too many brushsides");

	out = map_brushsides;
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleShort (in->planenum);
		if (num >= numplanes)
			Com_Error (ERR_DROP, "Bad brushside plane");
		out->plane = &map_planes[num];
		j = LittleShort (in->texinfo);
		if (j >= numtexinfo)
			Com_Error (ERR_DROP, "Bad brushside texinfo");
		out->surface = &map_surfaces[j];
	}
}

/*
=================
CMod_LoadAreas
=================
*/
static void CMod_LoadAreas (lump_t *l)
{
	int			i;
	carea_t		*out;
	darea_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadAreas: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAS)
		Com_Error (ERR_DROP, "Map has too many areas");

	out = map_areas;
	numareas = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->numareaportals = LittleLong (in->numareaportals);
		out->firstareaportal = LittleLong (in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
CMod_LoadAreaPortals
=================
*/
static void CMod_LoadAreaPortals (lump_t *l)
{
	int			i;
	dareaportal_t		*out;
	dareaportal_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "CMod_LoadAreaPortals: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_AREAPORTALS)
		Com_Error (ERR_DROP, "Map has too many areaportals");

	out = map_areaportals;
	numareaportals = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleLong (in->portalnum);
		out->otherarea = LittleLong (in->otherarea);
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
static void CMod_LoadVisibility (lump_t *l)
{
	int		i;

	numvisibility = l->filelen;
	if (l->filelen > MAX_MAP_VISIBILITY)
		Com_Error (ERR_DROP, "Map has too large visibility lump");

	memcpy (map_visibility, cmod_base + l->fileofs, l->filelen);

	map_vis->numclusters = LittleLong (map_vis->numclusters);
	for (i=0 ; i<map_vis->numclusters ; i++)
	{
		map_vis->bitofs[i][0] = LittleLong (map_vis->bitofs[i][0]);
		if (map_vis->bitofs[i][0] < 0 || map_vis->bitofs[i][0] >= numvisibility)
			Com_Error (	ERR_DROP, 
				"Map contains invalid PVS bit offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		map_vis->bitofs[i][1] = LittleLong (map_vis->bitofs[i][1]);
		if (map_vis->bitofs[i][1] < 0 || map_vis->bitofs[i][1] >= numvisibility)
			Com_Error (	ERR_DROP, 
				"Map contains invalid PHS bit offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString (lump_t *l)
{
	numentitychars = l->filelen;
	if (l->filelen > MAX_MAP_ENTSTRING)
		Com_Error (ERR_DROP, "Map has too large entity lump");

	memcpy (map_entitystring, cmod_base + l->fileofs, l->filelen);
}



/*
=================
CMod_LoadAlternateEntityData
=================
*/
static void CMod_LoadAlternateEntityData (char *entity_file_name)
{
	char *buf;

	numentitychars = FS_LoadFile (entity_file_name, (void **)&buf);

	if (numentitychars >= MAX_MAP_ENTSTRING)
		Com_Error (ERR_DROP, "Entity data has too large entity lump");

	memcpy (map_entitystring, buf, numentitychars);
	map_entitystring[numentitychars] = '\0';

	FS_FreeFile (buf);
}

//=======================================================================

static float RayFromSegment (const vec3_t start, const vec3_t end, vec3_t out_dir)
{
	VectorSubtract (end, start, out_dir);
	return VectorNormalize (out_dir);
}

/* 
Heavily modified derivative of Fast Ray-Box Intersection algorithm
by Andrew Woo
from "Graphics Gems", Academic Press, 1990
The copyright license of "Graphics Gems" permits use of this code.
*/
static qboolean RayIntersectsBBox (const vec3_t origin, const vec3_t dir, const vec3_t mins, const vec3_t maxs, float *out_intersection_dist)
{
	qboolean inside = true;
	int i;
	int whichPlane = -1;
	vec3_t coord;
	vec_t maxT = 0.0f;

	/* Find plane of intersection (corresponds to highest t value) */
	for (i = 0; i < 3; i++)
	{
		if (origin[i] < mins[i])
		{
			inside = false;
			if (likely (dir[i] != 0.0f))
			{
				vec_t t = (mins[i] - origin[i]) / dir[i];
				if (t > maxT)
				{
					maxT = t;
					whichPlane = i;
					coord[i] = mins[i];
				}
			}
		}
		else if (origin[i] > maxs[i])
		{
			inside = false;
			if (likely (dir[i] != 0.0f))
			{
				vec_t t = (maxs[i] - origin[i]) / dir[i];
				if (t > maxT)
				{
					maxT = t;
					whichPlane = i;
					coord[i] = maxs[i];
				}
			}
		}
	}
	
	/* Ray origin inside bounding box */
	if(inside)
	{
		*out_intersection_dist = 0;
		return true;
	}

	/* Check final candidate actually inside box */
	if (whichPlane == -1)
		return false;
	for (i = 0; i < 3; i++)
	{
		if (whichPlane != i)
		{
			coord[i] = origin[i] + maxT * dir[i];
			if (coord[i] < mins[i] || coord[i] > maxs[i])
				return false;
		}
	}
	
	VectorSubtract (coord, origin, coord);
	*out_intersection_dist = VectorLength (coord);
	
	return true;				/* ray hits box */
}

static qboolean RayIntersectsTriangle (const vec3_t p1, const vec3_t d, const vec3_t v0, const vec3_t v1, const vec3_t v2, float *out_intersection_dist)
{
	vec3_t	e1, e2, P, Q, T;
	float	det, inv_det, u, v, t;
	
	VectorSubtract (v1, v0, e1);
	VectorSubtract (v2, v0, e2);
	
	CrossProduct (d, e2, P);
	det = DotProduct (e1, P);
	
	if (fabs (det) < FLT_EPSILON)
		return false;
	
	inv_det = 1.0f/det;
	
	VectorSubtract (p1, v0, T);
	u = inv_det * DotProduct (P, T);
	
	if (u < 0.0f || u > 1.0f)
		return false;
	
	CrossProduct (T, e1, Q);
	v = inv_det * DotProduct (d, Q);
	
	if (v < 0.0f || u + v > 1.0f)
		return false;
	
	t = inv_det * DotProduct (e2, Q);
	
	*out_intersection_dist = t;
	
	return t > FLT_EPSILON;
}

qboolean TriangleIntersectsBBox
	(const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec3_t mins, const vec3_t maxs, vec3_t out_mins, vec3_t out_maxs)
{
	int i, j, axis;
	float len;
	vec3_t dir;
	qboolean all_in, ret;
	float dist;

#define BBOX_ACCUM(pt) \
{ \
	if (	(out_mins != NULL || out_maxs != NULL) && \
			pt[0] <= maxs[0] && pt[0] >= mins[0] && \
			pt[1] <= maxs[1] && pt[1] >= mins[1] && \
			pt[2] <= maxs[2] && pt[2] >= mins[2] ) \
	{ \
		for (j = 0; j < 3; j++) \
		{ \
			if (out_mins != NULL && pt[j] < out_mins[j]) \
				out_mins[j] = pt[j]; \
			if (out_maxs != NULL && pt[j] > out_maxs[j]) \
				out_maxs[j] = pt[j]; \
		} \
	} \
}

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
	
	BBOX_ACCUM (v0);
	BBOX_ACCUM (v1);
	BBOX_ACCUM (v2);
	
	if (all_in)
		return true; // all triangle points are inside the bbox

#define RAY_INTERSECTED(start,dir,dist) \
{ \
	vec3_t pt; \
	if (!out_mins && !out_maxs) return true; \
	VectorMA (start, dist, dir, pt); \
	BBOX_ACCUM (pt); \
	ret = true; \
}
	
	// check if one of the edges of the bounding box goes through the triangle
	ret = false;
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
		
			if (RayIntersectsTriangle (start, dir, v0, v1, v2, &dist) && dist <= (maxs[axis] - mins[axis]))
				RAY_INTERSECTED (start, dir, dist);
		}
	}
	
	// check if one of the edges of the triangle goes through the bbox
#define _TRISIDE(v0,v1) \
{ \
	len = RayFromSegment (v0, v1, dir); \
	if (RayIntersectsBBox (v0, dir, mins, maxs, &dist) && dist <= len) \
		RAY_INTERSECTED (v0, dir, dist); \
}
#define TRISIDE(v0,v1) \
{ \
	_TRISIDE (v0, v1); \
	if (dist == 0 && (out_mins != NULL || out_maxs != NULL)) \
		_TRISIDE (v1, v0); \
}
	
	TRISIDE (v0, v1);
	TRISIDE (v1, v2);
	TRISIDE (v2, v0);

#undef _TRISIDE
#undef TRISIDE
#undef RAY_INTERSECTED
#undef BBOX_ACCUM
	
	return	ret;
}

void AnglesToMatrix3x3 (const vec3_t angles, float rotation_matrix[3][3])
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

static void CM_LoadTerrain_PopulateGrid (cterrainmodel_t *mod, cterraingrid_t *grid, cterraintri_t **tmp)
{
	int i, j;
	vec3_t tmpmaxs, tmpmins;
	
	grid->numtris = 0;
	
	// This will be the new bounding box; it will grow as the
	// triangles get added.
	VectorCopy (grid->maxs, tmpmins);
	VectorCopy (grid->mins, tmpmaxs);

	for (i = 0; i < mod->numtriangles; i++)
	{
		cterraintri_t *tri = &mod->tris[i];
		if (TriangleIntersectsBBox (tri->verts[0], tri->verts[1], tri->verts[2], grid->mins, grid->maxs, tmpmins, tmpmaxs))
			tmp[grid->numtris++] = tri;
	}
	
	if (grid->numtris == 0)
		return;
	
	VectorCopy (tmpmins, grid->mins);
	VectorCopy (tmpmaxs, grid->maxs);
	
	grid->tris = Z_Malloc (grid->numtris * sizeof(cterraintri_t *));
	for (i = 0; i < grid->numtris; i++)
		grid->tris[i] = tmp[i];
	
	// Create bounding plane
	VectorClear (grid->boundingplane.normal);
	for (i = 0; i < grid->numtris; i++)
		VectorAdd (grid->tris[i]->planes.mainplane.normal, grid->boundingplane.normal, grid->boundingplane.normal);
	VectorNormalize (grid->boundingplane.normal);
	grid->boundingplane.dist = DotProduct (grid->tris[0]->verts[0], grid->boundingplane.normal);
	for (i = 0; i < grid->numtris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			vec_t dist = DotProduct (grid->tris[i]->verts[j], grid->boundingplane.normal);
			if (dist > grid->boundingplane.dist)
				grid->boundingplane.dist = dist;
		}
	}
}

static void CM_LoadTerrainModel (const vec3_t angles, const vec3_t origin,
								 const char *lightmap_path, int num_vertices,
								 const float *vert_positions, int num_triangles,
								 const unsigned int *tri_indices,
								 const vec3_t mins, const vec3_t maxs)
{
	float rotation_matrix[3][3];
	int i, j, k, l;
	cterrainmodel_t *mod;
	cterraintri_t **tmp;
	vec3_t up, lm_mins, lm_maxs;
	
	if (numterrainmodels == MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "CM_LoadTerrainModel: MAX_MAP_MODELS");
	
	mod = &terrain_models[numterrainmodels];
	mod->firsttriangle = CM_NumTriangles ();
	mod->firstvertex = CM_NumVertices ();
	numterrainmodels++;
	
	AnglesToMatrix3x3 (angles, rotation_matrix);
	
	if (lightmap_path != NULL)
		LoadTGA (lightmap_path, &mod->lightmaptex, &mod->lm_w, &mod->lm_h);
	else
		mod->lightmaptex = NULL;
	
	mod->numtriangles = 0;
	
	mod->verts = Z_Malloc (num_vertices*sizeof(vec3_t));
	mod->tris = Z_Malloc (num_triangles*sizeof(cterraintri_t));
	
	// Technically, because the mins can be positive or the maxs can be 
	// negative, this isn't always correct, but it errs on the side of more
	// inclusive, so it's all right.
	VectorCopy (origin, mod->mins);
	VectorCopy (origin, mod->maxs);
	
	for (i = 0; i < num_vertices; i++)
	{
		for (j = 0; j < 3; j++)
		{
			mod->verts[3*i+j] = origin[j];
			for (k = 0; k < 3; k++)
				mod->verts[3*i+j] += vert_positions[3*i+k] * rotation_matrix[j][k];
				
			if (mod->verts[3*i+j] < mod->mins[j])
				mod->mins[j] = mod->verts[3*i+j];
			else if (mod->verts[3*i+j] > mod->maxs[j])
				mod->maxs[j] = mod->verts[3*i+j];
		}
	}
	
	mod->numvertices = num_vertices;
	
	VectorSet (mod->lm_s_axis, rotation_matrix[0][0], rotation_matrix[1][0], rotation_matrix[2][0]);
	VectorSet (mod->lm_t_axis, rotation_matrix[0][1], rotation_matrix[1][1], rotation_matrix[2][1]);
	VectorSet (up, rotation_matrix[0][2], rotation_matrix[1][2], rotation_matrix[2][2]);
	
	for (j = 0; j < 3; j++)
	{
		lm_mins[j] = lm_maxs[j] = origin[j];
		for (k = 0; k < 3; k++)
		{
			lm_mins[j] += mins[k] * rotation_matrix[j][k];
			lm_maxs[j] += maxs[k] * rotation_matrix[j][k];
		}
	}
	
	mod->lm_mins[0] = DotProduct (lm_mins, mod->lm_s_axis);
	mod->lm_mins[1] = DotProduct (lm_mins, mod->lm_t_axis);
	mod->lm_size[0] = DotProduct (lm_maxs, mod->lm_s_axis) - mod->lm_mins[0];
	mod->lm_size[1] = DotProduct (lm_maxs, mod->lm_t_axis) - mod->lm_mins[1];
	
	// pass 1: vertices, main plane of each triangle, filter out downfacing
	for (i = 0; i < num_triangles; i++)
	{
		cterraintri_t *tri;
		vec3_t side1, side2;
		int j, k;
		
		tri = &mod->tris[mod->numtriangles];
		
		for (j = 0; j < 3; j++)
			tri->verts[j] = &mod->verts[3*tri_indices[3*i+j]];

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
		CrossProduct (side2, side1, tri->planes.mainplane.normal);
		VectorNormalize (tri->planes.mainplane.normal);
		for (j = 0; j < 3; j++)
		{
			// cleanup -0
			if (tri->planes.mainplane.normal[j] == -0.0f) tri->planes.mainplane.normal[j] = 0.0f;
		}
		tri->planes.mainplane.dist = DotProduct (tri->verts[0], tri->planes.mainplane.normal);
		tri->planes.mainplane.signbits = signbits_for_plane (&tri->planes.mainplane);
		if (fabsf (tri->planes.mainplane.normal[0]) == 1.0f)
			tri->planes.mainplane.type = PLANE_X;
		else if (fabsf (tri->planes.mainplane.normal[1]) == 1.0f)
			tri->planes.mainplane.type = PLANE_Y;
		else if (fabsf (tri->planes.mainplane.normal[2]) == 1.0f)
			tri->planes.mainplane.type = PLANE_Z;
		else
			tri->planes.mainplane.type = PLANE_ANYZ;
		
		// overwrite and don't use downward facing faces.
		if (DotProduct (tri->planes.mainplane.normal, up) < 0.0f)
			continue;
		
		VectorSet (tri->neighbors, -1, -1, -1);
		
		mod->numtriangles++;
	}
	
	// pass 2: find triangle neighbors
	for (i = 0; i < mod->numtriangles; i++)
	{
		cterraintri_t *tri, *tri2;
		int j, k, l;
		
		tri = &mod->tris[i];
		
		for (j = 0; j < 3; j++)
		{
			if (tri->neighbors[j] != -1)
				continue;
			
			for (k = 0; k < mod->numtriangles; k++)
			{
				if (k == i) continue;
				
				tri2 = &mod->tris[k];
				
				for (l = 0; l < 3; l++)
				{
					if (tri2->neighbors[l] != -1)
						continue;
					
					if (tri2->verts[l] != tri->verts[j] && tri2->verts[l] != tri->verts[(j + 1) % 3])
						continue;
					if (tri2->verts[(l + 1) % 3] != tri->verts[j] && tri2->verts[(l + 1) % 3] != tri->verts[(j + 1) % 3])
						continue;
					
					tri2->neighbors[l] = i;
					tri2->neighbors_whichedge[l] = j;
					tri->neighbors[j] = k;
					tri->neighbors_whichedge[j] = l;
				}
			}
		}
	}
	
	// pass 3: generate the rest of the data for the triangle
	for (i = 0; i < mod->numtriangles; i++)
	{
		cterraintri_t *tri;
		int j;
		
		tri = &mod->tris[i];
		
		for (j = 0; j < 3; j++)
		{
			cterraintri_t *neighbortri;

			// generate a side plane from scratch
			vec3_t side;
			
			VectorSubtract (tri->verts[(j + 1) % 3], tri->verts[j], side);
			CrossProduct (tri->planes.mainplane.normal, side, tri->planes.sideplanes[j].normal);
			VectorNormalize (tri->planes.sideplanes[j].normal); 
			for (k = 0; k < 3; k++)
			{
				// cleanup -0
				if (tri->planes.sideplanes[j].normal[k] == -0.0f) tri->planes.sideplanes[j].normal[k] = 0.0f;
			}
			tri->planes.sideplanes[j].dist = DotProduct (tri->verts[j], tri->planes.sideplanes[j].normal);
			tri->planes.sideplanes[j].signbits = signbits_for_plane (&tri->planes.sideplanes[j]);
			if (fabsf (tri->planes.sideplanes[j].normal[0]) == 1.0f)
				tri->planes.sideplanes[j].type = PLANE_X;
			else if (fabsf (tri->planes.sideplanes[j].normal[1]) == 1.0f)
				tri->planes.sideplanes[j].type = PLANE_Y;
			else if (fabsf (tri->planes.sideplanes[j].normal[2]) == 1.0f)
				tri->planes.sideplanes[j].type = PLANE_Z;
			else
				tri->planes.sideplanes[j].type = PLANE_ANYZ;
			
			if (tri->neighbors[j] == -1)
				continue;
			
			neighbortri = &mod->tris[tri->neighbors[j]];
			
			// If neighbor's angle with this triangle is concave, neighbor's
			// main plane would exclude this triangle's main plane rather
			// than cap it.
			if (DotProduct (neighbortri->planes.mainplane.normal, tri->verts[(j + 2) % 3]) >= neighbortri->planes.mainplane.dist)
				continue;
			
/*			if (DotProduct (tri->planes.sideplanes[j].normal, neighbortri->verts[(tri->neighbors_whichedge[j] + 2) % 3]) >= tri->planes.sideplanes[j].dist)*/
/*				continue;*/
			// use a neighbor's main plane as a side plane if the two tris
			// together form a convex mesh, but *only* if it will be *more*
			// restrictive than the "from-scratch" plane.
			tri->planes.sideplanes[j] = neighbortri->planes.mainplane;
		}
		
		for (j = 0; j < 3; j++)
		{
			tri->planes.bboxplanes[j].type = tri->planes.bboxplanes[j+3].type = PLANE_X + j;
			tri->planes.bboxplanes[j].dist = tri->maxs[j];
			VectorClear (tri->planes.bboxplanes[j].normal);
			tri->planes.bboxplanes[j].normal[j] = 1.0f;
			tri->planes.bboxplanes[j].signbits = 0.0f;
			tri->planes.bboxplanes[j+3].dist = -tri->mins[j];
			VectorClear (tri->planes.bboxplanes[j+3].normal);
			tri->planes.bboxplanes[j+3].normal[j] = -1.0f;
			tri->planes.bboxplanes[j+3].signbits = 1 << j;
		}
	}
	
	if (num_triangles != mod->numtriangles)
		Com_Printf ("WARN: %d downward facing collision polygons in model %d!\n", num_triangles - mod->numtriangles, numterrainmodels-1);
	
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
				cterraingrid_t *grid = &mod->grids[l*mod->numgrid[0]*mod->numgrid[1]+i*mod->numgrid[0]+j];
				
				grid->mins[0] = mod->mins[0] + j*TERRAIN_GRIDSIZE;
				grid->maxs[0] = mod->mins[0] + (j+1)*TERRAIN_GRIDSIZE;
				grid->mins[1] = mod->mins[1] + i*TERRAIN_GRIDSIZE;
				grid->maxs[1] = mod->mins[1] + (i+1)*TERRAIN_GRIDSIZE;
				grid->mins[2] = mod->mins[2] + l*TERRAIN_GRIDSIZE - 0.1; // HACK
				grid->maxs[2] = mod->mins[2] + (l+1)*TERRAIN_GRIDSIZE;
				
				CM_LoadTerrain_PopulateGrid (mod, grid, tmp);
			}
		}
	}
	Z_Free (tmp);
}

static void CM_LoadTerrainModel_FromFile (char *name, const vec3_t angles, const vec3_t origin)
{
	char *buf;
	terraindata_t data;
	
	// Can only handle .terrain meshes so far
	if (Q_strcasecmp (COM_FileExtension (name), "terrain"))
		return;
	
	FS_LoadFile (name, (void**)&buf);
	
	if (!buf)
		Com_Error (ERR_DROP, "CM_LoadTerrainModel: Missing terrain model %s!", name);

	if(!ReadTerrainData (&data, name, false))
	{	
		// This ends up being 1/4 as much detail as is used for rendering. You 
		// need a surprisingly large amount to maintain accurate physics.
		LoadTerrainFile (&data, name, false, 0.5, 8, buf);

		//write this out this time
		WriteTerrainData (&data, name, false);
	}

	//here we should read this collision mesh in from a file that we generate from terraindata_t "data" if it's not already there.
	
	CM_LoadTerrainModel (angles, origin, NULL,
						 data.num_vertices, data.vert_positions,
						 data.num_triangles, data.tri_indices,
						 data.mins, data.maxs);
	
	CleanupTerrainData (&data);
	
	FS_FreeFile (buf);
}

// NOTE: If you update this, you may also want to update
// R_ParseTerrainModelEntity in ref_gl/r_model.c.
static void CM_ParseTerrainModelEntity (char *match, char *block)
{
	int		i;
	vec3_t	angles, origin;
	char	*bl, *tok, *pathname = NULL;
	
	VectorClear (angles);
	VectorClear (origin);
	
	bl = block;
	while (1)
	{
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("model", tok))
		{
			pathname = CopyString (Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("angles", tok))
		{
			for (i = 0; i < 3; i++)
				angles[i] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("angle", tok))
		{
			angles[YAW] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("origin", tok))
		{
			for (i = 0; i < 3; i++)
				origin[i] = atof(Com_ParseExt(&bl, false));
		}
		else
			Com_SkipRestOfLine(&bl);
	}
	
	CM_LoadTerrainModel_FromFile (pathname, angles, origin);
	
	Z_Free (pathname);
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/

cmodel_t *CM_LoadBSP (char *name, qboolean clientload, unsigned *checksum)
{
	unsigned		*buf;
	unsigned int	i, j;
	dheader_t		header;
	int				length;
	static unsigned	last_checksum;
	int				bsp_lump_order[HEADER_LUMPS] = 
	{
		LUMP_PLANES, LUMP_LEAFS, LUMP_VERTEXES, LUMP_NODES, 
		LUMP_TEXINFO, LUMP_FACES, LUMP_BRUSHES,
		LUMP_BRUSHSIDES, LUMP_LEAFFACES, LUMP_LEAFBRUSHES,
		LUMP_SURFEDGES, LUMP_EDGES, LUMP_MODELS, LUMP_AREAS,
		LUMP_AREAPORTALS, LUMP_LIGHTING, LUMP_VISIBILITY,
		LUMP_ENTITIES, LUMP_POP
	};

	map_noareas = Cvar_Get ("map_noareas", "0", 0);

	// Don't need to load the map twice on local servers.
	if (  !strcmp (map_name, name) && (clientload || !Cvar_VariableValue ("flushmap")) )
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			memset (portalopen, 0, sizeof(portalopen));
			FloodAreaConnections ();
		}
		return &map_cmodels[0];		// still have the right version
	}

	// free old stuff
	numplanes = 0;
	numnodes = 0;
	numleafs = 0;
	numcmodels = 0;
	numvisibility = 0;
	numentitychars = 0;
	map_entitystring[0] = 0;
	map_name[0] = 0;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		return &map_cmodels[0];			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//

	length = FS_LoadFile (name, (void **)&buf);
	if (!buf)
		Com_Error (ERR_DROP, "Could not load %s", name);

	last_checksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (i=0 ; i<sizeof(dheader_t)/sizeof(int) ; i++)
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);

	if (header.version != BSPVERSION)
		Com_Error (ERR_DROP, "CMod_LoadBrushModel: %s has wrong version number (%i should be %i)"
		, name, header.version, BSPVERSION);

	cmod_base = (byte *)buf;
	
	if (checkLumps(header.lumps, 2*sizeof(int), bsp_lump_order, cmod_base, HEADER_LUMPS, length))
		Com_Error (ERR_DROP,"CMod_LoadBrushModel: lumps in %s don't add up right!\n"
							"The file is likely corrupt, please obtain a fresh copy.",name);

	// load into heap
	CMod_LoadSurfaces (&header.lumps[LUMP_TEXINFO]);
	CMod_LoadLeafs (&header.lumps[LUMP_LEAFS]);
	CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
	CMod_LoadPlanes (&header.lumps[LUMP_PLANES]);
	CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES]);
	CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES]);
	CMod_LoadSubmodels (&header.lumps[LUMP_MODELS]);
	CMod_LoadNodes (&header.lumps[LUMP_NODES]);
	CMod_LoadAreas (&header.lumps[LUMP_AREAS]);
	CMod_LoadAreaPortals (&header.lumps[LUMP_AREAPORTALS]);
	CMod_LoadVisibility (&header.lumps[LUMP_VISIBILITY]);
	CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES]);

	FS_FreeFile (buf);

	CM_InitBoxHull ();

	memset (portalopen, 0, sizeof(portalopen));
	FloodAreaConnections ();

	strcpy (map_name, name);
	
	// Reset terrain models from last time. 
	// TODO: verify this works in ALL situations, including local and non-
	// local servers, wierd sequences of connects/disconnects, etc.
	for (i = 0; i < numterrainmodels; i++)
	{
		Z_Free (terrain_models[i].verts);
		Z_Free (terrain_models[i].tris);
		for (j = 0; j < NUMGRID((&terrain_models[i])); j++)
		{
			if (terrain_models[i].grids[j].numtris != 0)
				Z_Free (terrain_models[i].grids[j].tris);
		}
		Z_Free (terrain_models[i].grids);
		if (terrain_models[i].lightmaptex != NULL)
			free (terrain_models[i].lightmaptex);
	}
	numterrainmodels = 0;
	
	// Parse new terrain models from the BSP entity data.
	// TODO: entdefs?
	{
		static const char *classnames[] = {"misc_terrainmodel"};
		
		CM_FilterParseEntities ("classname", 1, classnames, CM_ParseTerrainModelEntity);
	}
	
	printf ("\n\nverts %d tris %d\n\n", CM_NumVertices (), CM_NumTriangles ());

	return &map_cmodels[0];
}

cmodel_t *CM_LoadMap (char *name, qboolean clientload, unsigned *checksum) {
	char        *buf;
	const char	*line;
	int		    length;
	cmodel_t    *tmp;

	char    *bsp_file = NULL;
	char    *ent_file = NULL;
	char    *token = NULL;


	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buf);
	if (!buf) {
		if (!name[0]) //no real map
			return CM_LoadBSP (name, clientload, checksum);
		else //fallback to just looking for a BSP
			return CM_LoadBSP (va("%s.bsp", name), clientload, checksum);
	}

	// get the name of the BSP and entdef files
	// if we ever add more map data files, be sure to add them here
	line = strtok (buf, ";");
	while (line) {
		token = COM_Parse (&line);
		if (!line && !(line = strtok (NULL, ";")))
			break;

		if (!Q_strcasecmp (token, "bsp")){
			if (bsp_file)
				Z_Free (bsp_file);
			bsp_file = CopyString (COM_Parse (&line));
			if (!line)
				Com_Error (ERR_DROP, "CM_LoadMap: EOL when expecting BSP filename! (File %s is invalid)", name);
		} else if (!Q_strcasecmp (token, "entdef")){
			if (ent_file)
				Z_Free (ent_file);
			ent_file = CopyString (COM_Parse (&line));
			if (!line)
				Com_Error (ERR_DROP, "CM_LoadMap: EOL when expecting entdef filename! (File %s is invalid)", name);
		}

		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		line = strtok (NULL, ";");
	}

	FS_FreeFile (buf);

	if (!bsp_file)
		Com_Error (ERR_DROP, "CM_LoadMap: BSP file not defined! (File %s is invalid)", name);

	tmp = CM_LoadBSP (bsp_file, clientload, checksum);

	if (ent_file){ //overwrite the entity data from another data file
		CMod_LoadAlternateEntityData (ent_file);
		Z_Free (ent_file);
	}

	Z_Free (bsp_file);

	return tmp;
}

/*
==================
CM_InlineModel
==================
*/
cmodel_t	*CM_InlineModel (char *name)
{
	int		num;

	if (!name || name[0] != '*')
		Com_Error (ERR_DROP, "CM_InlineModel: bad name");
	num = atoi (name+1);
	if (num < 1 || num >= numcmodels)
		Com_Error (ERR_DROP, "CM_InlineModel: bad number");

	return &map_cmodels[num];
}

int		CM_NumClusters (void)
{
	return numclusters;
}

int		CM_NumInlineModels (void)
{
	return numcmodels;
}

char	*CM_EntityString (void)
{
	return map_entitystring;
}

int		CM_LeafContents (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Com_Error (ERR_DROP, "CM_LeafContents: bad number");
	return map_leafs[leafnum].contents;
}

int		CM_LeafCluster (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
	{
		Com_Error (ERR_DROP, "CM_LeafCluster: bad number");
		return 0; /* unreachable. quiets bogus compiler array index warning */
	}
	return map_leafs[leafnum].cluster;
}

int		CM_LeafArea (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Com_Error (ERR_DROP, "CM_LeafArea: bad number");
	return map_leafs[leafnum].area;
}

//=======================================================================

// Looks for entities with the given field matched to one of the possible 
// values.
void CM_FilterParseEntities (const char *fieldname, int numvals, const char *vals[], void (*process_ent_callback) (char *match, char *block))
{
	int			i;
	char		*buf, *tok;
	char		block[2048];
	
	buf = CM_EntityString();
	while (1)
	{
		qboolean matched = false;
		
		tok = Com_ParseExt(&buf, true);
		if (!tok[0])
			break;			// End of data

		if (Q_strcasecmp(tok, "{"))
			continue;		// Should never happen!

		// Parse the text inside brackets
		block[0] = 0;
		do {
			tok = Com_ParseExt(&buf, false);
			if (!Q_strcasecmp(tok, "}"))
				break;		// Done

			if (!tok[0])	// Newline
				Q_strcat(block, "\n", sizeof(block));
			else {			// Token
				Q_strcat(block, " ", sizeof(block));
				Q_strcat(block, tok, sizeof(block));
			}
		} while (buf);
		
		// Find the key we're filtering by
		tok = strstr (block, fieldname);
		if (!tok)
			continue;
		
		// Skip key name and whitespace
		tok += strlen (fieldname);
		while (*tok && *tok == ' ')
			tok++;
		
		// Next token will be the value. We want it to be one of the values
		// we're looking for.
		for (i = 0; i < numvals; i++)
		{
			if (!Q_strnicmp(tok, vals[i], strlen (vals[i])))
			{
				matched = true;
				break;
			}
		}
		
		if (!matched)
			continue;
		
		process_ent_callback (tok, block);
	}
}

//=======================================================================


cplane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
cleaf_t		*box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = numnodes;
	box_planes = &map_planes[numplanes];
	if (numnodes+6 > MAX_MAP_NODES
		|| numbrushes+1 > MAX_MAP_BRUSHES
		|| numleafbrushes+1 > MAX_MAP_LEAFBRUSHES
		|| numbrushsides+6 > MAX_MAP_BRUSHSIDES
		|| numplanes+12 > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = 	map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->contents = CONTENTS_MONSTER;
		c->plane = map_planes + (numplanes+i*2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - numleafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;
		p->signbits = signbits_for_plane( p );

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
		p->signbits = signbits_for_plane( p );
	}
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r

==================
*/
static int CM_PointLeafnum_r (vec3_t p, int num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (vec3_t p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}



/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

static void CM_BoxLeafnums_r (int nodenum)
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}

		node = &map_nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}

	}
}

static int	CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mins, maxs, list,
		listsize, map_cmodels[0].headnode, topnode);
}



/*
==================
CM_PointContents

==================
*/
int CM_PointContents (vec3_t p, int headnode)
{
	int		l;

	if (!numnodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headnode);

	return map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;
	int			l;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode &&
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (p_l, headnode);

	return map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125f)

static vec3_t		trace_start, trace_end;
static vec3_t		trace_mins, trace_maxs;
static vec3_t		trace_extents;

static trace_t		trace_trace;
static int			trace_contents;
// optimized case: no extents
static qboolean		trace_ispoint;
// optimized case: stop at any intersection point at all
// otherwise, look for the *closest* intersection point
static qboolean		trace_fast;

static float CM_OffsetPlaneDist (const cplane_t *plane, const vec3_t mins, const vec3_t maxs)
{
	switch ( plane->type )
	{
	case PLANE_X:
		if ( plane->signbits == 0x01 )
			return plane->dist - (maxs[0]*plane->normal[0]);
		else
			return plane->dist - (mins[0]*plane->normal[0]);

	case PLANE_Y:
		if ( plane->signbits == 0x02 )
			return plane->dist - (maxs[1]*plane->normal[1]);
		else
			return plane->dist - (mins[1]*plane->normal[1]);

	case PLANE_Z:
		if ( plane->signbits == 0x04 )
			return plane->dist - (maxs[2]*plane->normal[2]);
		else
			return plane->dist - (mins[2]*plane->normal[2]);

	default:
		switch ( plane->signbits ) /* bit2=z<0, bit1=y<0, bit0=x<0 */
		{
		case 0: /* 000b */
			return plane->dist - (mins[0]*plane->normal[0] +
					mins[1]*plane->normal[1] + mins[2]*plane->normal[2]);
		case 1: /* 001b */
			return plane->dist - (maxs[0]*plane->normal[0] +
					mins[1]*plane->normal[1] + mins[2]*plane->normal[2]);
		case 2: /* 010b */
			return plane->dist - (mins[0]*plane->normal[0] +
					maxs[1]*plane->normal[1] + mins[2]*plane->normal[2]);
		case 3: /* 011b */
			return plane->dist - (maxs[0]*plane->normal[0] +
					maxs[1]*plane->normal[1] + mins[2]*plane->normal[2]);
		case 4: /* 100b */
			return plane->dist - (mins[0]*plane->normal[0] +
					mins[1]*plane->normal[1] + maxs[2]*plane->normal[2]);
		case 5: /* 101b */
			return plane->dist - (maxs[0]*plane->normal[0] +
					mins[1]*plane->normal[1] + maxs[2]*plane->normal[2]);
		case 6: /* 110b */
			return plane->dist - (mins[0]*plane->normal[0] +
					maxs[1]*plane->normal[1] + maxs[2]*plane->normal[2]);
		case 7: /* 111b */
			return plane->dist - (maxs[0]*plane->normal[0] +
					maxs[1]*plane->normal[1] + maxs[2]*plane->normal[2]);

		default:
			Com_Error( ERR_DROP, "CM_OffsetPlaneDist: bad plane signbits\n");
			return 0.0f; // unreachable. suppress bogus compiler warning
		}
	}
}

static void CM_ClipBoxToBrush( vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
		trace_t *trace, cbrush_t *brush )
{
	int           i;
	cplane_t     *plane, *clipplane;
	float         dist;
	float         enterfrac, leavefrac;
	float         d1, d2;
	qboolean      getout, startout;
	float         f;
	cbrushside_t *side, *leadside;

	enterfrac = -1.0f;
	leavefrac = 1.0f;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;
		if (trace_ispoint)
			dist = plane->dist; // special point case
		else
			dist = CM_OffsetPlaneDist (plane, mins, maxs);

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if ( d2 > 0.0f )
			getout = true; // endpoint is not in solid
		if ( d1 > 0.0f )
			startout = true;

		// if completely in front of face, no intersection
		if ( d1 > 0.0f && d2 >= d1 )
			return;

		if ( d1 <= 0.0f && d2 <= 0.0f )
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else if ( d1 < d2 )
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
		/* else d1 == d2, line segment is parallel to plane, cannot intersect.
		   formerly processed like d1 < d2. now just continue to next
		   brushside plane.  */
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1.0f && enterfrac < trace->fraction)
		{
			if (enterfrac < 0.0f)
				enterfrac = 0.0f;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

static qboolean CM_ClipBoxToTerrainTri (const vec3_t mins, const vec3_t maxs, const vec3_t p1, const vec3_t p2, trace_t *trace, const cterraintri_t *tri)
{
	static csurface_t placeholder_surface; // no flags, no value
	int           i;
	const cplane_t     *plane, *clipplane;
	float         dist;
	float         enterfrac, leavefrac;
	float         d1, d2;
	qboolean      getout, startout;
	float         f;

	enterfrac = -1.0f;
	leavefrac = 1.0f;
	clipplane = NULL;

	getout = false;
	startout = false;

	for (i=0 ; i<10 ; i++)
	{
		plane = &tri->allplanes[i];
	
		if (trace_ispoint)
			dist = plane->dist; // special point case
		else
			dist = CM_OffsetPlaneDist (plane, mins, maxs);

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if ( d2 > 0.0f )
			getout = true; // endpoint is not in solid
		if ( d1 > 0.0f )
			startout = true;

		// if completely in front of face, no intersection
		if ( d1 > 0.0f && d2 >= d1 )
			return false;

		if ( d1 <= 0.0f && d2 <= 0.0f )
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
			}
		}
		else if ( d1 < d2 )
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
		/* else d1 == d2, line segment is parallel to plane, cannot intersect.

		   formerly processed like d1 < d2. now just continue to next
		   brushside plane.  */
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return false;
	}
	
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1.0f && enterfrac < trace->fraction)
		{
			if (enterfrac < 0.0f)
				enterfrac = 0.0f;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &placeholder_surface;
			trace->contents = CONTENTS_SOLID;
			return true;
		}
	}
	
	return false;
}

/*
================
CM_TestBoxInBrush
================
*/
static void CM_TestBoxInBrush (const vec3_t mins, const vec3_t maxs, const vec3_t p1,
					  trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > (DIST_EPSILON * 0.5f) )
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}

static void CM_TestBoxInTerrainTri (const vec3_t mins, const vec3_t maxs, const vec3_t p1,
					  trace_t *trace, cterraintri_t *tri)
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	vec3_t		pmins, pmaxs;
	
	for (i=0 ; i<10 ; i++)
	{
		plane = &tri->allplanes[i];

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0.0f)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > (DIST_EPSILON * 0.5f) )
			return;

	}
	
	VectorAdd (mins, p1, pmins);
	VectorAdd (maxs, p1, pmaxs);
	if (TriangleIntersectsBBox (tri->verts[0], tri->verts[1], tri->verts[2], pmins, pmaxs, NULL, NULL))
	{
		// inside this brush
		trace->startsolid = trace->allsolid = true;
		trace->fraction = 0;
		trace->contents = CONTENTS_SOLID;
	}
}


/*
================
CM_TraceToLeaf
================
*/
static void CM_TraceToLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
		if (trace_fast && trace_trace.fraction != 1.0f)
			return;
	}

}


/*
================
CM_TestInLeaf
================
*/
static void CM_TestInLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
CM_RecursiveHullCheck
==================
*/
static void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, const vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	vec3_t		mid;
	int			side;
	float		midf;
	vec3_t		p1_copy;
	vec3_t		p_delta;

re_test:
	
	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	node = map_nodes + num;
	
	if (!(node->contents & trace_contents))
		return; // This volume contains no leafs we're interested in
	
	// TODO: add bbox and gap stuff to BoxHull, change to trace_ispoint
	if (	trace_fast &&
			node->containsgap && 
			p1[0] < node->emptymaxs[0] && p2[0] < node->emptymaxs[0] &&
			p1[1] < node->emptymaxs[1] && p2[1] < node->emptymaxs[1] &&
			p1[2] < node->emptymaxs[2] && p2[2] < node->emptymaxs[2] &&
			p1[0] > node->emptymins[0] && p2[0] > node->emptymins[0] &&
			p1[1] > node->emptymins[1] && p2[1] > node->emptymins[1] &&
			p1[2] > node->emptymins[2] && p2[2] > node->emptymins[2])
		return;
	
	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		num = node->children[0];
		goto re_test;
	}
	if (t1 <= -offset && t2 <= -offset)
	{
		num = node->children[1];
		goto re_test;
	}

	// Split the trace into two segments along the dividing plane. Each
	// segment has an offset-sized "tail" on the "wrong" side of the plane.
	// Obstacles on one side might still stop the trace on the other side, as
	// long as the distance from the obstacle to the trace endpoint <= offset.
	if (t1 < t2)
	{
		idist = 1.0f/(t1-t2);
		side = 1;
		frac2 = (t1 + offset)*idist;
		frac = (t1 - offset)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0f/(t1-t2);
		side = 0;
		frac2 = (t1 - offset)*idist;
		frac = (t1 + offset)*idist;
	}
	else
	{
		// Trace is parallel to the dividing plane. This is not an uncommon
		// case, and we handle it by tracing the full distance in both sub-
		// nodes.
		side = 0; // This is arbitrary
		frac = 1.0f; // Trace the whole distance in the 1st sub-node
		frac2 = 0.0f; // Trace the whole distance in the 2nd sub-node too
	}

	// move up to the node
	if (frac < 0.0f)
		frac = 0.0f;
	if (frac > 1.0f)
		frac = 1.0f;

	midf = p1f + (p2f - p1f)*frac;
	VectorSubtract (p2, p1, p_delta);
	mid[0] = p1[0] + frac * p_delta[0];
	mid[1] = p1[1] + frac * p_delta[1];
	mid[2] = p1[2] + frac * p_delta[2];
	
	// so deeper recursions can modify their p1 arguments
	VectorCopy (p1, p1_copy);

	// this is the only case where the function actually recurses
	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1_copy, mid);

	if (trace_fast && trace_trace.fraction != 1.0f)
		return;		// already hit anything at all

	// go past the node
	if (frac2 < 0.0f)
		frac2 = 0.0f;
	if (frac2 > 1.0f)
		frac2 = 1.0f;

	p1f += (p2f - p1f)*frac2;
	
	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	p1[0] += frac2 * p_delta[0];
	p1[1] += frac2 * p_delta[1];
	p1[2] += frac2 * p_delta[2];
	
	num = node->children[side^1];
	goto re_test;
}


static qboolean bbox_in_trace (const vec3_t box_mins, const vec3_t box_maxs, const vec3_t p1, const vec3_t p2, const vec3_t trace_mins, const vec3_t trace_maxs)
{
	vec3_t offset_maxs, offset_mins;

	VectorSubtract (box_maxs, trace_mins, offset_maxs);
	VectorSubtract (box_mins, trace_maxs, offset_mins);

	return !(	(p1[0] > offset_maxs[0] && p2[0] > offset_maxs[0]) ||
				(p1[0] < offset_mins[0] && p2[0] < offset_mins[0]) ||
				(p1[1] > offset_maxs[1] && p2[1] > offset_maxs[1]) ||
				(p1[1] < offset_mins[1] && p2[1] < offset_mins[1]) ||
				(p1[2] > offset_maxs[2] && p2[2] > offset_maxs[2]) ||
				(p1[2] < offset_mins[2] && p2[2] < offset_mins[2]));
}

static void CM_TerrainGridSwath (const cterrainmodel_t *mod, const vec3_t mincoords, const vec3_t maxcoords, const vec3_t dir, int mingrid[3], int maxgrid[3], int griddir[3])
{
	int i;
	
	for (i = 0; i < 3; i++)
	{
		mingrid[i] = (int)floor((mincoords[i]-mod->mins[i]) / TERRAIN_GRIDSIZE);
		if (mingrid[i] < 0)
			mingrid[i] = 0;
		
		maxgrid[i] = (int)ceil((maxcoords[i]-mod->mins[i]) / TERRAIN_GRIDSIZE + 0.1f);
		if (maxgrid[i] > mod->numgrid[i])
			maxgrid[i] = mod->numgrid[i];
		
		if (dir[i] < 0.0f)
		{
			float tmp;
			griddir[i] = -1;
			tmp = mingrid[i] - 1;
			mingrid[i] = maxgrid[i] - 1;
			maxgrid[i] = tmp;
		}
		else
		{
			griddir[i] = 1;
		}
	}
}

// FIXME: It's still quite possible to fall through a terrain mesh.
// FIXME: Also, the physics feel like crap on terrain.
// Returns -1 for no intersection, otherwise it returns the index within 
// terrain_models where the intersection was found (used by
// CM_TerrainLightPoint.)
static int CM_TerrainTrace (const vec3_t p1, const vec3_t end)
{
	vec3_t		p2;
	int			i, j, k, x, y, z;
	vec3_t		dir;
	int			ret = -1;
	
	VectorSubtract (trace_end, trace_start, dir);
	VectorMA (p1, trace_trace.fraction, dir, p2);
	
	for (i = 0; i < numterrainmodels; i++)
	{
		vec3_t mincoords, maxcoords;
		int mingrid[3], maxgrid[3], griddir[3];
		cterrainmodel_t	*mod = &terrain_models[i];
		
		if (!bbox_in_trace (mod->mins, mod->maxs, p1, p2, trace_mins, trace_maxs))
			continue;
		
		for (k = 0; k < 3; k++)
		{
			mincoords[k] = maxcoords[k] = p1[k];
			if (p2[k] < mincoords[k])
				mincoords[k] = p2[k];
			if (p2[k] > maxcoords[k])
				maxcoords[k] = p2[k];
			mincoords[k] += trace_mins[k];
			maxcoords[k] += trace_maxs[k];
		}
		
		// We iterate through the grid layers, rows, and columns in order from
		// closest to furthest. 
		CM_TerrainGridSwath (mod, mincoords, maxcoords, dir, mingrid, maxgrid, griddir);
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
					
					// TODO: eventually make this trace_ispoint when we can
					// be confident that this works
					if (	trace_fast &&
							DotProduct (p1, grid->boundingplane.normal) > grid->boundingplane.dist &&
							DotProduct (p2, grid->boundingplane.normal) > grid->boundingplane.dist)
						continue;
					
					if (grid->numtris > 2)
					{
						if (!bbox_in_trace (grid->mins, grid->maxs, p1, p2, trace_mins, trace_maxs))
							continue;
					
						if (trace_ispoint && !RayIntersectsBBox (p1, dir, grid->mins, grid->maxs, &tmp))
							continue;
					}
		
					for (j = 0; j < grid->numtris; j++)
					{
						cterraintri_t	*tri = grid->tris[j];
			
						// order the tests from least expensive to most
						
						if (tri->checkcount == checkcount) 
							continue; // already checked this triangle in another cell
						tri->checkcount = checkcount;
						
						if (!CM_ClipBoxToTerrainTri (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, tri))
							continue;
						
						// At this point, we've found a new closest intersection point
						VectorMA (p1, trace_trace.fraction, dir, p2);
						ret = i;
						
						if (trace_fast)
							goto done;
					}
					
					// Any grid cells we check after this one are further away
					// than this one, so they won't contain any triangles that
					// are actually *closer* that we haven't checked already.
					// Thus, if we've found any intersections in the previous
					// grid cell, there won't be any closer intersections in
					// the next one, so we can stop the tracing right here.
					if (trace_ispoint && ret == i)
						goto done_with_this_model;
				}
			}
		}
done_with_this_model:;
	}
	
done:
	return ret;
}

void CM_TerrainLightPoint (vec3_t in_point, vec3_t out_point, vec3_t out_color)
{
	int modnum;
	float s, t;
	vec4_t out_color_withalpha;
	cterrainmodel_t	*mod;
	
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	
	VectorCopy (in_point, trace_start);
	trace_start[2] += 2048;
	VectorCopy (in_point, trace_end);
	trace_end[2] -= 2048;
	
	trace_ispoint = true;
	trace_fast = false;
	checkcount++;
	VectorClear (trace_extents);
	
	out_color[0] = out_color[1] = out_color[2] = 1.0f;
	
	modnum = CM_TerrainTrace (trace_start, trace_end);
	
	VectorCopy (trace_start, out_point);
	out_point[2] = trace_start[2] - 4096.0 * trace_trace.fraction;
	
	if (modnum == -1)
		return;
	
	mod = &terrain_models[modnum];
	
	if (mod->lightmaptex == NULL)
	{
		out_point[2] = trace_end[2];
		return;
	}
	
	s = (DotProduct (out_point, mod->lm_s_axis) - mod->lm_mins[0]) / mod->lm_size[0];
	t = 1.0 - (DotProduct (out_point, mod->lm_t_axis) - mod->lm_mins[1]) / mod->lm_size[1];
	
	bilinear_sample (mod->lightmaptex, mod->lm_w, mod->lm_h, s, t, out_color_withalpha);
	VectorCopy (out_color_withalpha, out_color);
}



//======================================================================

/*
==================
CM_BoxTrace
==================
*/
static trace_t	CM_BoxTrace_Core (const vec3_t start, const vec3_t end,
								  vec3_t mins, vec3_t maxs,
								  int headnode, int brushmask,
								  qboolean enable_terrain)
{
	int		i, j, x, y, z;
	vec3_t	local_start, local_end;

	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!numnodes)	// map not loaded
		return trace_trace;
	
	trace_fast = false;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);
	
	if ( !(start == end) &&
			(start[0] != end[0] || start[1] != end[1] || start[2] != end[2]) )
	{
		if (( mins == maxs ) ||
				( mins[0] == 0.0f && mins[1] == 0.0f && mins[2] == 0.0f &&
				maxs[0] == 0.0f && maxs[1] == 0.0f && maxs[2] == 0.0f ))
		{ // point special case
			trace_ispoint = true;
			VectorClear (trace_extents);
		}
		else
		{ // general axis-aligned box case
			trace_ispoint = false;
			trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
			trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
			trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
		}
		//
		// general sweeping through world
		//
		VectorCopy (start, local_start); // so the function can modify these in-place
		VectorCopy (end, local_end);
		CM_RecursiveHullCheck (headnode, 0, 1, local_start, local_end);
		
		if (enable_terrain)
			CM_TerrainTrace (start, end);
		
		if (trace_trace.fraction == 1)
		{
			VectorCopy (end, trace_trace.endpos);
		}
		else
		{
			for (i=0 ; i<3 ; i++)
				trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
		}
	}
	else
	{ // position test special case
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;
		
		VectorCopy (start, trace_trace.endpos);

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				return trace_trace;
		}
		
		if (!(CONTENTS_SOLID & trace_contents))
			return trace_trace;
		
		if (!enable_terrain)
		    return trace_trace;
		
		for (i = 0; i < numterrainmodels; i++)
		{
			int mingrid[3], maxgrid[3], griddir[3];
			cterrainmodel_t	*mod = &terrain_models[i];
		
			for (j = 0; j < 3; j++)
			{
				if (c1[j] > mod->maxs[j] || c2[j] < mod->mins[j])
					goto skip_mod;
			}
		
			// We iterate through the grid layers, rows, and columns in order from
			// closest to furthest. 
			CM_TerrainGridSwath (mod, c1, c2, vec3_origin, mingrid, maxgrid, griddir);
			
			for (z = mingrid[2]; z != maxgrid[2]; z += griddir[2])
			{
				for (y = mingrid[1]; y != maxgrid[1]; y += griddir[1])
				{
					for (x = mingrid[0]; x != maxgrid[0]; x += griddir[0])
					{
						cterraingrid_t *grid;
					
						grid = &mod->grids[z*mod->numgrid[0]*mod->numgrid[1]+y*mod->numgrid[0]+x];
						for (j = 0; j < 3; j++)
						{
							if (c1[j] > mod->maxs[j] || c2[j] < mod->mins[j])
								goto skip_grid;
						}
						
						for (j = 0; j < grid->numtris; j++)
						{
							cterraintri_t	*tri = grid->tris[j];
							
							CM_TestBoxInTerrainTri (trace_mins, trace_maxs, trace_start, &trace_trace, tri);
							
							if (trace_trace.allsolid)
								return trace_trace;
						}
skip_grid:;
					}
				}
			}
skip_mod:;
		}
	}

	return trace_trace;
}

trace_t		CM_BoxTrace (const vec3_t start, const vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask)
{
	return CM_BoxTrace_Core (start, end, mins, maxs, headnode, brushmask, true);
}

// Returns true if there is nothing blocking the path from start to end.
// Provides no way to know what the obstacle was if there was one. Faster than
// standard BoxTrace. If start == end, will always return true.
qboolean CM_FastTrace (const vec3_t start, const vec3_t end, int headnode, int brushmask)
{
	vec3_t	local_start, local_end;

	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	if (!numnodes)	// map not loaded
		return true;
	
	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);
	
	trace_fast = true;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorClear (trace_mins);
	VectorClear (trace_maxs);
	trace_ispoint = true;
	VectorClear (trace_extents);
	
	if ( !(start == end) &&
			(start[0] != end[0] || start[1] != end[1] || start[2] != end[2]) )
	{
		qboolean ret;
		//
		// general sweeping through world
		//
		VectorCopy (start, local_start); // so the function can modify these in-place
		VectorCopy (end, local_end);
		CM_RecursiveHullCheck (headnode, 0, 1, local_start, local_end);
		
		CM_TerrainTrace (start, end);
		
		ret = trace_trace.fraction == 1.0f;
		
		if (fasttrace_verify->integer)
		{
			CM_BoxTrace (start, end, vec3_origin, vec3_origin, headnode, brushmask);
			assert (ret == (trace_trace.fraction == 1.0f));
		}
		
		return ret;
	}
	else
	{ 
		return true;
	}
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
/*
#ifdef _WIN32
#pragma optimize( "", off )
#endif
*/


trace_t		CM_TransformedBoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask,
						  vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode &&
	(angles[0] || angles[1] || angles[2]) )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace_Core (start_l, end_l, mins, maxs, headnode, brushmask, false);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

/*
#ifdef _WIN32
#pragma optimize( "", on )
#endif
*/



/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (numclusters+7)>>3;
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf ("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

byte	pvsrow[MAX_MAP_LEAFS/8];
byte	phsrow[MAX_MAP_LEAFS/8];

byte	*CM_ClusterPVS (int cluster)
{
	static int old_cluster = -1;
	if (cluster == old_cluster)
		return pvsrow;
	old_cluster = cluster;
	
	if (cluster == -1)
		memset (pvsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
	return pvsrow;
}

byte	*CM_ClusterPHS (int cluster)
{
	static int old_cluster = -1;
	if (cluster == old_cluster)
		return phsrow;
	old_cluster = cluster;

	if (cluster == -1)
		memset (phsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_vis->bitofs[cluster][DVIS_PHS], phsrow);
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r (carea_t *area, int floodnum)
{
	int		i;
	dareaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaportals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (portalopen[p->portalnum])
			FloodArea_r (&map_areas[p->otherarea], floodnum);
	}
}

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections (void)
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void	CM_SetAreaPortalState (int portalnum, qboolean open)
{
	if (portalnum > numareaportals)
		Com_Error (ERR_DROP, "areaportal > numareaportals");

	portalopen[portalnum] = open;
	FloodAreaConnections ();
}

qboolean	CM_AreasConnected (int area1, int area2)
{
	if (map_noareas->value)
		return true;

	if (area1 > numareas || area2 > numareas)
		Com_Error (ERR_DROP, "area > numareas");

	if (map_areas[area1].floodnum == map_areas[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (numareas+7)>>3;

	if (map_noareas->value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = map_areas[area].floodnum;
		for (i=0 ; i<numareas ; i++)
		{
			if (map_areas[i].floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
/*
// unused
void	CM_WritePortalState (FILE *f)
{
	fwrite (portalopen, sizeof(portalopen), 1, f);
}
*/

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
/*
// unused
void	CM_ReadPortalState (FILE *f)
{
	FS_Read (portalopen, sizeof(portalopen), f);
	FloodAreaConnections ();
}
*/

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}


/*
=================
CM_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean CM_inPVS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return false;
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks sight
	return true;
}

//similar to CM_inPVS but with leafnums
qboolean CM_inPVS_leafs (int leafnum1, int leafnum2)
{
	int		cluster;
	int		area1, area2;
	byte	*mask;

	cluster = CM_LeafCluster (leafnum1);
	area1 = CM_LeafArea (leafnum1);
	mask = CM_ClusterPVS (cluster);

	cluster = CM_LeafCluster (leafnum2);
	area2 = CM_LeafArea (leafnum2);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return false;
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks sight
	return true;
}

/*
=================
CM_inPHS

Also checks portalareas so that doors block sound
=================
*/
qboolean CM_inPHS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPHS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return false;		// more than one bounce away
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks hearing

	return true;
}

/*
=================
Functions related to exporting collision geometry to other parts of the code,
for example the ragdoll physics. Currently just does terrain meshes, but someday
we could unify BSP geometry into this system as well. Then the ragdoll code
would be able to ignore the difference between terrain and BSP geometry.
=================
*/

int CM_NumVertices (void)
{
	cterrainmodel_t *mod;
	if (numterrainmodels == 0)
		return 0;
	
	mod = &terrain_models[numterrainmodels - 1];
	return mod->firstvertex + mod->numvertices;
}

int CM_NumTriangles (void)
{
	cterrainmodel_t *mod;
	if (numterrainmodels == 0)
		return 0;
	
	mod = &terrain_models[numterrainmodels - 1];
	return mod->firsttriangle + mod->numtriangles;
}

void CM_GetVertex (int num, vec3_t out)
{
	int i;
	
	for (i = 0; i < numterrainmodels; i++)
	{
		int num_adjusted;
		cterrainmodel_t *mod = &terrain_models[i];
		
		num_adjusted = num - mod->firstvertex;
		if (num_adjusted >= 0 && num_adjusted < mod->numvertices)
		{
			VectorCopy (&mod->verts[3*num_adjusted], out);
			return;
		}
	}
}

void CM_GetTriangle (int num, int out[3])
{
	int i, j;
	
	for (i = 0; i < numterrainmodels; i++)
	{
		int num_adjusted;
		cterrainmodel_t *mod = &terrain_models[i];
		
		num_adjusted = num - mod->firsttriangle;
		if (num_adjusted >= 0 && num_adjusted < mod->numtriangles)
		{
			for (j = 0; j < 3; j++)
				out[j] = (mod->tris[num_adjusted].verts[j] - mod->verts) / 3 + mod->firstvertex;
			return;
		}
	}
}
