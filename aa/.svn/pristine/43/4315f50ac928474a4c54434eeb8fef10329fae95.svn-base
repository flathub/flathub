// trace.c

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "qrad.h"
#include <assert.h>

#define	ON_EPSILON	0.1

typedef struct tnode_s
{
	int		type;
	vec3_t	normal;
	float	dist;
	int		children[2];
	int		children_leaf[2]; //valid if the corresponding child is a leaf
    int     pad;
} tnode_t;

tnode_t		*tnodes, *tnode_p;

/*
==============
MakeTnode

Converts the disk node structure into the efficient tracing structure
==============
*/
static int tnode_mask;
void MakeTnode (int nodenum)
{
	tnode_t			*t;
	dplane_t		*plane;
	int				i;
	dnode_t 		*node;
	
	t = tnode_p++;

	node = dnodes + nodenum;
	plane = dplanes + node->planenum;

	t->type = plane->type;
	VectorCopy (plane->normal, t->normal);
	t->dist = plane->dist;
	
	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i] < 0)
		{
            t->children[i] = (dleafs[-node->children[i] - 1].contents & tnode_mask) | (1<<31);
        	t->children_leaf[i] = -node->children[i] - 1;
		}
		else
		{
			t->children[i] = tnode_p - tnodes;
			MakeTnode (node->children[i]);
		}
	}
}


/*
=============
MakeTnodes

Loads the node structure out of a .bsp file to be used for light occlusion
=============
*/
void MakeTnodes (dmodel_t *bm)
{
	// 32 byte align the structs
	tnodes = malloc( (numnodes+1) * sizeof(tnode_t));
	tnodes = (tnode_t *)(((int)tnodes + 31)&~31);
	tnode_p = tnodes;
	tnode_mask = CONTENTS_SOLID|CONTENTS_WINDOW;
	//TODO: or-in CONTENTS_WINDOW in response to a command-line argument

	MakeTnode (0);
}

//==========================================================

/*
=============
BuildPolygons

Applies the same basic algorithm as r_surf.c:BSP_BuildPolygonFromSurface to
every surface in the BSP, precomputing the xyz and st coordinates of each 
vertex of each polygon. Skip non-translucent surfaces for now.
=============
*/
#if 0
typedef struct {
	float	point[3];
	int		st[2];
} tvtex_t;

typedef struct {
	int		numverts;
	tvtex_t	*vtexes;
} tpoly_t;

tpoly_t	tpolys[MAX_MAP_FACES];

void BuildPolygon (int facenum)
{
	dface_t		*face;
	tpoly_t		*poly;
	dedge_t		*edge;
	texinfo_t	*tex;
	float 	*vec;
	int		numverts, i, edgenum;
	
	face = &dfaces[facenum];
	numverts = face->numedges;
	poly = &tpolys[facenum];
	poly->numverts = numverts;
	poly->vtexes = malloc (sizeof(tvtex_t)*numverts);
	tex = &texinfo[face->texinfo];
	
	for (i = 0; i < numverts; i++)
	{
		edgenum = face->firstedge;
		if (edgenum > 0)
		{
			edge = &dedges[edgenum];
			vec = dvertexes[edge->v[0]].point;
		}
		else
		{
			edge = &dedges[-edgenum];
			vec = dvertexes[edge->v[1]].point;
		}
		
		poly->vtexes[i].st[0] = 
			(DotProduct (vec, tex->vecs[0]) + tex->vecs[0][3])/
			texture_sizes[face->texinfo][0];
		poly->vtexes[i].st[1] = 
			(DotProduct (vec, tex->vecs[1]) + tex->vecs[1][3])/
			texture_sizes[face->texinfo][1];
		
		VectorCopy (vec, poly->vtexes[i].point);
	}
}

void BuildPolygons (void)
{
	int i;
	for (i = 0; i < numfaces; i++)
		BuildPolygon(i);
}
#endif

int	PointInNodenum (vec3_t point)
{
	int		nodenum, oldnodenum;
	vec_t	dist;
	dnode_t	*node;
	dplane_t	*plane;

	nodenum = 0;
	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planenum];
		dist = DotProduct (point, plane->normal) - plane->dist;
		oldnodenum = nodenum;
		if (dist > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}

	return oldnodenum;
}

/*
================
PointInBrush

Adapted from CM_ClipBoxToBrush
================
*/
int PointInBrush (const vec3_t p1, const vec3_t p2, vec3_t pt,
					const dbrush_t *brush, float *fraction, int curface)
{
	int			i;
	dplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	float		d1, d2;
	qboolean	startout;
	float		f;
	dbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return curface;

	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &dbrushsides[brush->firstside+i];
		plane = &dplanes[side->planenum];

		// FIXME: special case for axial

		dist = plane->dist;

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d1 > 0)
			startout = true; //startpoint is not in solid

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return curface;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-ON_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+ON_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}
	
	if (!startout)
	{	// original point was inside brush
		return curface;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < *fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			for (i = 0; i < 3; i++) 
				pt[i] = p1[i] + (p2[i] - p1[i])*enterfrac;
			*fraction = enterfrac;
			return leadside->texinfo;
		}
	}
	return curface;
}

/*
=============
GetNodeFace

Adapted from CM_TraceToLeaf
=============
*/
int GetNodeFace (int leafnum, const vec3_t start, const vec3_t end, vec3_t pt)
{
	int			k;
	int			brushnum;
	dleaf_t		*leaf;
	float 		fraction;
	int 		curface;

	leaf = &dleafs[leafnum];
	fraction = 1.1;
	curface = dfaces[leaf->firstleafface].texinfo;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = dleafbrushes[leaf->firstleafbrush+k];
		curface = PointInBrush (start, end, pt, &dbrushes[brushnum], &fraction, curface);
		if (!fraction)
			return curface;
	}
	return curface;
}

/*
=============
GetRGBASample

Gets the RGBA value within the texture of the polygon of a 3D point on the 
polygon.
=============
*/
const float opaque[4] = {0.0, 0.0, 0.0, 1.0};
const float transparent[4] = {0.0, 0.0, 0.0, 0.0};
#ifdef WIN32
__inline const float *GetRGBASample (int node_leaf, const vec3_t orig_start, const vec3_t orig_stop)
#else
inline const float *GetRGBASample (int node_leaf, const vec3_t orig_start, const vec3_t orig_stop)
#endif
{
	float		fs, 	ft;
	int 		s, t, smax, tmax, i;
	int			texnum;
	vec3_t		point, start_to_point, start_to_stop;
	texinfo_t 	*tex;
	
	texnum = GetNodeFace (node_leaf, orig_start, orig_stop, point);
	VectorSubtract(point, orig_start, start_to_point);
	VectorNormalize(start_to_point, start_to_point);
	VectorSubtract(orig_stop, orig_start, start_to_stop);
	VectorNormalize(start_to_stop, start_to_stop);
	for (i = 0; i < 3; i++)
		if (fabs(start_to_point[i]-start_to_stop[i]) > 0.001)
			return transparent; //FIXME: where are these coming from?
	if (texnum < 0)
	    return opaque; //FIXME: is this the right thing to do? Where is this coming from?
	tex = &texinfo[texnum];
	
	smax = texture_sizes[texnum][0];
	tmax = texture_sizes[texnum][1];
	
	fs = fmodf (DotProduct (point, tex->vecs[0]) + tex->vecs[0][3], smax) + smax;
	s = (int)fs%smax;
	
	ft = fmodf (DotProduct (point, tex->vecs[1]) + tex->vecs[1][3], tmax) + tmax;
	t = (int)ft%tmax;

	
	return texture_data[texnum]+(t*smax+s)*4;
}

//==========================================================

//with texture checking
int TestLine_r_texcheck (int node, int node_leaf, vec3_t orig_start, vec3_t orig_stop, vec3_t set_start, vec3_t stop, vec3_t occluded)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid, _start;
	vec_t *start;
	float	frac;
	int		side;
	int		r;
	int		i;

    start = set_start;

re_test:

	r = 0;
	if (node & (1<<31))
	{
		const float *rgba_sample;
		float occluded_len_squared = 0;
		if ((r = node & ~(1<<31)) != CONTENTS_WINDOW)
		{
			return r;
		}
		//translucent, so check texture
		//occluded starts out as {1.0,1.0,1.0} and has each color reduced as
		//the trace passes through more translucent textures.
		rgba_sample = GetRGBASample (node_leaf, orig_start, orig_stop);
		if (rgba_sample[3] == 0.0)
			return 0; //not occluded
		if (rgba_sample[3] == 1.0)
			return 1; //occluded
		for (i = 0; i < 3; i++) {
			occluded[i] *= rgba_sample[i]*(1.0-rgba_sample[3]);
			occluded_len_squared += occluded[i]*occluded[i];
		}
		if (occluded_len_squared < 0.001)
		{
			return 1; //occluded
		}
		return 0; //not occluded
	}

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}
	
	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
	{
        node = tnode->children[0];
        node_leaf = tnode->children_leaf[0];
        goto re_test;
	}
	
	if (front < ON_EPSILON && back < ON_EPSILON)
	{
        node = tnode->children[1];
        node_leaf = tnode->children_leaf[1];
        goto re_test;
	}

	side = front < 0;

    frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;


    if ((r = TestLine_r_texcheck (tnode->children[side], tnode->children_leaf[side], orig_start, orig_stop, start, mid, occluded)))
		return r;

    node = tnode->children[!side];
    node_leaf = tnode->children_leaf[!side];

    start = _start;
    start[0] = mid[0];
    start[1] = mid[1];
    start[2] = mid[2];

    goto re_test;
}

//without texture checking
int TestLine_r (int node, vec3_t set_start, vec3_t stop)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid, _start;
	vec_t *start;
	float	frac;
	int		side;
	int		r;

    start = set_start;

re_test:

	r = 0;
	if (node & (1<<31))
	{
		if ((r = node & ~(1<<31)) != CONTENTS_WINDOW)
		{
			return r;
		}
		return 0;
	}

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}
	
	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
	{
        node = tnode->children[0];
        goto re_test;
	}
	
	if (front < ON_EPSILON && back < ON_EPSILON)
	{
        node = tnode->children[1];
        goto re_test;
	}

	side = front < 0;

    frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;


    if ((r = TestLine_r (tnode->children[side], start, mid)))
		return r;

    node = tnode->children[!side];

    start = _start;
    start[0] = mid[0];
    start[1] = mid[1];
    start[2] = mid[2];

    goto re_test;
}


// Checking terrain in this one is not appropriate
int TestLine (vec3_t start, vec3_t stop)
{
    vec3_t occluded;
    
    occluded[0] = occluded[1] = occluded[2] = 1.0;
	if (doing_texcheck)
		return TestLine_r_texcheck (0, 0, start, stop, start, stop, occluded);
	else
		return TestLine_r (0, start, stop);
}

// Checking terrain in this one *is* appropriate
int TestLine_color (int node, vec3_t start, vec3_t stop, vec3_t occluded, occlusioncache_t *cache)
{
	qboolean bsp_occluded;
	
	occluded[0] = occluded[1] = occluded[2] = 1.0;
	
	if (!Fast_Terrain_Trace_Try_Cache (start, stop, cache))
		return true;
	
	if (doing_texcheck)
		bsp_occluded = TestLine_r_texcheck (node, 0, start, stop, start, stop, occluded);
	else
		bsp_occluded = TestLine_r (node, start, stop);
	
	if (bsp_occluded)
		return true;
	
	return !Fast_Terrain_Trace_Cache_Miss (start, stop, cache);
}

/*
==============================================================================

LINE TRACING

The major lighting operation is a point to point visibility test, performed
by recursive subdivision of the line by the BSP tree.

==============================================================================
*/

typedef struct
{
	vec3_t	backpt;
	int		side;
	int		node;
} tracestack_t;


/*
==============
TestLine
==============
*/
qboolean _TestLine (vec3_t start, vec3_t stop)
{
	int				node;
	float			front, back;
	tracestack_t	*tstack_p;
	int				side;
	float 			frontx,fronty, frontz, backx, backy, backz;
	tracestack_t	tracestack[64];
	tnode_t			*tnode;
	
	frontx = start[0];
	fronty = start[1];
	frontz = start[2];
	backx = stop[0];
	backy = stop[1];
	backz = stop[2];
	
	tstack_p = tracestack;
	node = 0;
	
	while (1)
	{
		if (node == CONTENTS_SOLID)
		{
#if 0
			float	d1, d2, d3;

			d1 = backx - frontx;
			d2 = backy - fronty;
			d3 = backz - frontz;

			if (d1*d1 + d2*d2 + d3*d3 > 1)
#endif
				return false;	// DONE!
		}
		
		while (node < 0)
		{
		// pop up the stack for a back side
			tstack_p--;
			if (tstack_p < tracestack)
				return true;
			node = tstack_p->node;
			
		// set the hit point for this plane
			
			frontx = backx;
			fronty = backy;
			frontz = backz;
			
		// go down the back side

			backx = tstack_p->backpt[0];
			backy = tstack_p->backpt[1];
			backz = tstack_p->backpt[2];
			
			node = tnodes[tstack_p->node].children[!tstack_p->side];
		}

		tnode = &tnodes[node];
		
		switch (tnode->type)
		{
		case PLANE_X:
			front = frontx - tnode->dist;
			back = backx - tnode->dist;
			break;
		case PLANE_Y:
			front = fronty - tnode->dist;
			back = backy - tnode->dist;
			break;
		case PLANE_Z:
			front = frontz - tnode->dist;
			back = backz - tnode->dist;
			break;
		default:
			front = (frontx*tnode->normal[0] + fronty*tnode->normal[1] + frontz*tnode->normal[2]) - tnode->dist;
			back = (backx*tnode->normal[0] + backy*tnode->normal[1] + backz*tnode->normal[2]) - tnode->dist;
			break;
		}

		if (front > -ON_EPSILON && back > -ON_EPSILON)
//		if (front > 0 && back > 0)
		{
			node = tnode->children[0];
			continue;
		}
		
		if (front < ON_EPSILON && back < ON_EPSILON)
//		if (front <= 0 && back <= 0)
		{
			node = tnode->children[1];
			continue;
		}

		side = front < 0;
		
		front = front / (front-back);
	
		tstack_p->node = node;
		tstack_p->side = side;
		tstack_p->backpt[0] = backx;
		tstack_p->backpt[1] = backy;
		tstack_p->backpt[2] = backz;
		
		tstack_p++;
		
		backx = frontx + front*(backx-frontx);
		backy = fronty + front*(backy-fronty);
		backz = frontz + front*(backz-frontz);
		
		node = tnode->children[side];		
	}	
}
