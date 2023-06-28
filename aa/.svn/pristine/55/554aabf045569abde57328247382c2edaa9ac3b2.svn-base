#include "qrad.h"

#include <assert.h>

#define	MAX_LSTYLES	256

typedef struct
{
	dface_t		*faces[2];
	qboolean	coplanar;
} edgeshare_t;

edgeshare_t	edgeshare[MAX_MAP_EDGES];

int			facelinks[MAX_MAP_FACES];
int			planelinks[2][MAX_MAP_PLANES];

/*
============
LinkPlaneFaces
============
*/
void LinkPlaneFaces (void)
{
	int		i;
	dface_t	*f;

	f = dfaces;
	for (i=0 ; i<numfaces ; i++, f++)
	{
		facelinks[i] = planelinks[f->side][f->planenum];
		planelinks[f->side][f->planenum] = i;
	}
}

/*
============
PairEdges
============
*/
void PairEdges (void)
{
	int		i, j, k;
	dface_t	*f;
	edgeshare_t	*e;

	f = dfaces;
	for (i=0 ; i<numfaces ; i++, f++)
	{
		for (j=0 ; j<f->numedges ; j++)
		{
			k = dsurfedges[f->firstedge + j];
			if (k < 0)
			{
				e = &edgeshare[-k];
				e->faces[1] = f;
			}
			else
			{
				e = &edgeshare[k];
				e->faces[0] = f;
			}

			if (e->faces[0] && e->faces[1])
			{
				// determine if coplanar
				if (e->faces[0]->planenum == e->faces[1]->planenum)
					e->coplanar = true;
			}
		}
	}
}

/*
=================================================================

  POINT TRIANGULATION

=================================================================
*/

typedef struct triedge_s
{
	int			p0, p1;
	vec3_t		normal;
	vec_t		dist;
	struct triangle_s	*tri;
} triedge_t;

typedef struct triangle_s
{
	triedge_t	*edges[3];
} triangle_t;

#define	MAX_TRI_POINTS		1024
#define	MAX_TRI_EDGES		(MAX_TRI_POINTS*6)
#define	MAX_TRI_TRIS		(MAX_TRI_POINTS*2)

typedef struct
{
	int			numpoints;
	int			numedges;
	int			numtris;
	dplane_t	*plane;
	triedge_t	*edgematrix[MAX_TRI_POINTS][MAX_TRI_POINTS];
	patch_t		*points[MAX_TRI_POINTS];
	triedge_t	edges[MAX_TRI_EDGES];
	triangle_t	tris[MAX_TRI_TRIS];
} triangulation_t;

/*
===============
AllocTriangulation
===============
*/
triangulation_t	*AllocTriangulation (dplane_t *plane)
{
	triangulation_t	*t;

	t = malloc(sizeof(triangulation_t));
	t->numpoints = 0;
	t->numedges = 0;
	t->numtris = 0;

	t->plane = plane;

//	memset (t->edgematrix, 0, sizeof(t->edgematrix));

	return t;
}

/*
===============
FreeTriangulation
===============
*/
void FreeTriangulation (triangulation_t *tr)
{
	free (tr);
}


triedge_t	*FindEdge (triangulation_t *trian, int p0, int p1)
{
	triedge_t	*e, *be;
	vec3_t		v1;
	vec3_t		normal;
	vec_t		dist;

	if (trian->edgematrix[p0][p1])
		return trian->edgematrix[p0][p1];

	if (trian->numedges > MAX_TRI_EDGES-2)
		Error ("trian->numedges > MAX_TRI_EDGES-2");

	VectorSubtract (trian->points[p1]->origin, trian->points[p0]->origin, v1);
	VectorNormalize (v1, v1);
	CrossProduct (v1, trian->plane->normal, normal);
	dist = DotProduct (trian->points[p0]->origin, normal);

	e = &trian->edges[trian->numedges];
	e->p0 = p0;
	e->p1 = p1;
	e->tri = NULL;
	VectorCopy (normal, e->normal);
	e->dist = dist;
	trian->numedges++;
	trian->edgematrix[p0][p1] = e;

	be = &trian->edges[trian->numedges];
	be->p0 = p1;
	be->p1 = p0;
	be->tri = NULL;
	VectorSubtract (vec3_origin, normal, be->normal);
	be->dist = -dist;
	trian->numedges++;
	trian->edgematrix[p1][p0] = be;

	return e;
}

triangle_t	*AllocTriangle (triangulation_t *trian)
{
	triangle_t	*t;

	if (trian->numtris >= MAX_TRI_TRIS)
		Error ("trian->numtris >= MAX_TRI_TRIS");

	t = &trian->tris[trian->numtris];
	trian->numtris++;

	return t;
}

/*
============
TriEdge_r
============
*/
void TriEdge_r (triangulation_t *trian, triedge_t *e)
{
	int		i, bestp = 0;
	vec3_t	v1, v2;
	vec_t	*p0, *p1, *p;
	vec_t	best, ang;
	triangle_t	*nt;

	if (e->tri)
		return;		// allready connected by someone

	// find the point with the best angle
	p0 = trian->points[e->p0]->origin;
	p1 = trian->points[e->p1]->origin;
	best = 1.1;
	for (i=0 ; i< trian->numpoints ; i++)
	{
		p = trian->points[i]->origin;
		// a 0 dist will form a degenerate triangle
		if (DotProduct(p, e->normal) - e->dist < 0)
			continue;	// behind edge
		VectorSubtract (p0, p, v1);
		VectorSubtract (p1, p, v2);
		if (!VectorNormalize (v1,v1))
			continue;
		if (!VectorNormalize (v2,v2))
			continue;
		ang = DotProduct (v1, v2);
		if (ang < best)
		{
			best = ang;
			bestp = i;
		}
	}
	if (best >= 1)
		return;		// edge doesn't match anything

	// make a new triangle
	nt = AllocTriangle (trian);
	nt->edges[0] = e;
	nt->edges[1] = FindEdge (trian, e->p1, bestp);
	nt->edges[2] = FindEdge (trian, bestp, e->p0);
	for (i=0 ; i<3 ; i++)
		nt->edges[i]->tri = nt;
	TriEdge_r (trian, FindEdge (trian, bestp, e->p1));
	TriEdge_r (trian, FindEdge (trian, e->p0, bestp));
}

/*
============
TriangulatePoints
============
*/
void TriangulatePoints (triangulation_t *trian)
{
	vec_t	d, bestd;
	vec3_t	v1;
	int		bp1=0, bp2=0, i, j;
	vec_t	*p1, *p2;
	triedge_t	*e, *e2;

	if (trian->numpoints < 2)
		return;

	// find the two closest points
	bestd = 9999;
	for (i=0 ; i<trian->numpoints ; i++)
	{
		p1 = trian->points[i]->origin;
		for (j=i+1 ; j<trian->numpoints ; j++)
		{
			p2 = trian->points[j]->origin;
			VectorSubtract (p2, p1, v1);
			d = VectorLength (v1);
			if (d < bestd)
			{
				bestd = d;
				bp1 = i;
				bp2 = j;
			}
		}
	}

	e = FindEdge (trian, bp1, bp2);
	e2 = FindEdge (trian, bp2, bp1);
	TriEdge_r (trian, e);
	TriEdge_r (trian, e2);
}

/*
===============
AddPointToTriangulation
===============
*/
void AddPointToTriangulation (patch_t *patch, triangulation_t *trian)
{
	int			pnum;

	pnum = trian->numpoints;
	if (pnum == MAX_TRI_POINTS)
		Error ("trian->numpoints == MAX_TRI_POINTS");
	trian->points[pnum] = patch;
	trian->numpoints++;
}

/*
===============
LerpTriangle
===============
*/
void	LerpTriangle (triangulation_t *trian, triangle_t *t, vec3_t point, vec3_t color)
{
	patch_t		*p1, *p2, *p3;
	vec3_t		base, d1, d2;
	float		x, y, x1, y1;

	p1 = trian->points[t->edges[0]->p0];
	p2 = trian->points[t->edges[1]->p0];
	p3 = trian->points[t->edges[2]->p0];

	VectorCopy (p1->totallight, base);

	x1 = DotProduct (p3->origin, t->edges[0]->normal) - t->edges[0]->dist;
	y1 = DotProduct (p2->origin, t->edges[2]->normal) - t->edges[2]->dist;

	VectorCopy (base, color);

	if (fabs(x1)>=ON_EPSILON)
	{
		VectorSubtract (p3->totallight, base, d2);
		x = DotProduct (point, t->edges[0]->normal) - t->edges[0]->dist;
		x /= x1;
		VectorMA (color, x, d2, color);
	}
	if (fabs(y1)>=ON_EPSILON)
	{
		VectorSubtract (p2->totallight, base, d1);
		y = DotProduct (point, t->edges[2]->normal) - t->edges[2]->dist;
		y /= y1;
		VectorMA (color, y, d1, color);
	}
}

qboolean PointInTriangle (vec3_t point, triangle_t *t)
{
	int		i;
	triedge_t	*e;
	vec_t	d;

	for (i=0 ; i<3 ; i++)
	{
		e = t->edges[i];
		d = DotProduct (e->normal, point) - e->dist;
		if (d < 0)
			return false;	// not inside
	}

	return true;
}

/*
===============
SampleTriangulation
===============
*/
void SampleTriangulation (vec3_t point, triangulation_t *trian, triangle_t **last_valid, vec3_t color)
{
	triangle_t	*t;
	triedge_t	*e;
	vec_t		d, best;
	patch_t		*p0, *p1;
	vec3_t		v1, v2;
	int			i, j;

	if (trian->numpoints == 0)
	{
		VectorClear (color);
		return;
	}

	if (trian->numpoints == 1)
	{
		VectorCopy (trian->points[0]->totallight, color);
		return;
	}

	// try the last one
	if (*last_valid)
	{
		if (PointInTriangle (point, *last_valid))
		{
			LerpTriangle (trian, *last_valid, point, color);
			return;
		}
	}

	// search for triangles
	for (t = trian->tris, j=0 ; j < trian->numtris ; t++, j++)
	{
		if (t == *last_valid)
			continue;

		if (!PointInTriangle (point, t))
			continue;

		*last_valid = t;
		LerpTriangle (trian, t, point, color);
		return;
	}

	// search for exterior edge
	for (e=trian->edges, j=0 ; j< trian->numedges ; e++, j++)
	{
		if (e->tri)
			continue;		// not an exterior edge

		d = DotProduct (point, e->normal) - e->dist;
		if (d < 0)
			continue;	// not in front of edge

		p0 = trian->points[e->p0];
		p1 = trian->points[e->p1];

		VectorSubtract (p1->origin, p0->origin, v1);
		VectorNormalize (v1, v1);
		VectorSubtract (point, p0->origin, v2);
		d = DotProduct (v2, v1);
		if (d < 0)
			continue;
		if (d > 1)
			continue;
		for (i=0 ; i<3 ; i++)
			color[i] = p0->totallight[i] + d * (p1->totallight[i] - p0->totallight[i]);
		return;
	}

	// search for nearest point
	best = 99999;
	p1 = NULL;
	for (j=0 ; j<trian->numpoints ; j++)
	{
		p0 = trian->points[j];
		VectorSubtract (point, p0->origin, v1);
		d = VectorLength (v1);
		if (d < best)
		{
			best = d;
			p1 = p0;
		}
	}

	if (!p1)
		Error ("SampleTriangulation: no points");

	VectorCopy (p1->totallight, color);
}

/*
=================================================================

  LIGHTMAP SAMPLE GENERATION

=================================================================
*/


#define	SINGLEMAP	(256*256*4)

typedef struct
{
	vec_t	facedist;
	vec3_t	facenormal;

	int		numsurfpt;
	vec3_t	surfpt[SINGLEMAP];

	vec3_t	modelorg;		// for origined bmodels

	vec3_t	texorg;
	vec3_t	worldtotex[2];	// s = (world - texorg) . worldtotex[0]
	vec3_t	textoworld[2];	// world = texorg + s * textoworld[0]

	vec_t	exactmins[2], exactmaxs[2];

	int		texmins[2], texsize[2];
	int		surfnum;
	dface_t	*face;
} lightinfo_t;


/*
================
CalcFaceExtents

Fills in s->texmins[] and s->texsize[]
also sets exactmins[] and exactmaxs[]
================
*/
void CalcFaceExtents (lightinfo_t *l)
{
	dface_t *s;
	vec_t	mins[2], maxs[2], val;
	int		i,j, e;
	int 	facenum;
	dvertex_t	*v;
	texinfo_t	*tex;
	vec3_t		vt;

	s = l->face;
	facenum = s-dfaces;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = &texinfo[s->texinfo];

	for (i=0 ; i<s->numedges ; i++)
	{
		e = dsurfedges[s->firstedge+i];
		if (e >= 0)
			v = dvertexes + dedges[e].v[0];
		else
			v = dvertexes + dedges[-e].v[1];

//		VectorAdd (v->point, l->modelorg, vt);
		VectorCopy (v->point, vt);

		for (j=0 ; j<2 ; j++)
		{
			val = DotProduct (vt, tex->vecs[j]) + tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		l->exactmins[i] = mins[i];
		l->exactmaxs[i] = maxs[i];

		mins[i] = floor((mins[i]*refine_amt)/16);
		maxs[i] = ceil((maxs[i]*refine_amt)/16);

		l->texmins[i] = mins[i];
		l->texsize[i] = maxs[i] - mins[i];
	}
	lfacelookups[facenum].width = l->texsize[0]+1;
	lfacelookups[facenum].height = l->texsize[1]+1;

	if (l->texsize[0] * l->texsize[1] > SINGLEMAP/4)	// div 4 for extrasamples
	{
		char s[3] = {'X', 'Y', 'Z'};

		for (i=0 ; i<2 ; i++)
		{
			printf("Axis: %c\n", s[i]);

			l->exactmins[i] = mins[i];
			l->exactmaxs[i] = maxs[i];

			mins[i] = floor((mins[i]*refine_amt)/16);
			maxs[i] = ceil((maxs[i]*refine_amt)/16);

			l->texmins[i] = mins[i];
			l->texsize[i] = maxs[i] - mins[i];

			printf("  Mins = %10.3f, Maxs = %10.3f,  Size = %10.3f\n", (double)mins[i], (double)maxs[i], (double)(maxs[i] - mins[i]));
		}

		Error ("Surface to large to map");
	}
}

/*
================
CalcFaceVectors

Fills in texorg, worldtotex. and textoworld
================
*/
void CalcFaceVectors (lightinfo_t *l)
{
	texinfo_t	*tex;
	int			i, j;
	vec3_t	texnormal;
	vec_t	distscale;
	vec_t	dist, len;
	int			w, h;

	tex = &texinfo[l->face->texinfo];

// convert from float to double
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			l->worldtotex[i][j] = tex->vecs[i][j];

// calculate a normal to the texture axis.  points can be moved along this
// without changing their S/T
	texnormal[0] = tex->vecs[1][1]*tex->vecs[0][2]
		- tex->vecs[1][2]*tex->vecs[0][1];
	texnormal[1] = tex->vecs[1][2]*tex->vecs[0][0]
		- tex->vecs[1][0]*tex->vecs[0][2];
	texnormal[2] = tex->vecs[1][0]*tex->vecs[0][1]
		- tex->vecs[1][1]*tex->vecs[0][0];
	VectorNormalize (texnormal, texnormal);

// flip it towards plane normal
	distscale = DotProduct (texnormal, l->facenormal);
	if (!distscale)
	{
		qprintf ("WARNING: Texture axis perpendicular to face\n");
		distscale = 1;
	}
	if (distscale < 0)
	{
		distscale = -distscale;
		VectorSubtract (vec3_origin, texnormal, texnormal);
	}

// distscale is the ratio of the distance along the texture normal to
// the distance along the plane normal
	distscale = 1/distscale;

	for (i=0 ; i<2 ; i++)
	{
		len = VectorLength (l->worldtotex[i]);
		dist = DotProduct (l->worldtotex[i], l->facenormal);
		dist *= distscale;
		VectorMA (l->worldtotex[i], -dist, texnormal, l->textoworld[i]);
		VectorScale (l->textoworld[i], (1/len)*(1/len), l->textoworld[i]);
	}


// calculate texorg on the texture plane
	for (i=0 ; i<3 ; i++)
		l->texorg[i] = -tex->vecs[0][3]* l->textoworld[0][i] - tex->vecs[1][3] * l->textoworld[1][i];

// project back to the face plane
	dist = DotProduct (l->texorg, l->facenormal) - l->facedist - 1;
	dist *= distscale;
	VectorMA (l->texorg, -dist, texnormal, l->texorg);

	// compensate for org'd bmodels
	VectorAdd (l->texorg, l->modelorg, l->texorg);

	// total sample count
	h = l->texsize[1]+1;
	w = l->texsize[0]+1;
	l->numsurfpt = w * h;
}

/*
=================
CalcPoints

For each texture aligned grid point, back project onto the plane
to get the world xyz value of the sample point
=================
*/
void CalcPoints (lightinfo_t *l, float sofs, float tofs)
{
	int		i;
	int		s, t, j;
	int		w, h, step;
	vec_t	starts, startt, us, ut;
	vec_t	*surf;
	vec_t	mids, midt;
	vec3_t	facemid;
	dleaf_t	*leaf;

	surf = l->surfpt[0];
	mids = (l->exactmaxs[0] + l->exactmins[0])/2;
	midt = (l->exactmaxs[1] + l->exactmins[1])/2;

	for (j=0 ; j<3 ; j++)
		facemid[j] = l->texorg[j] + l->textoworld[0][j]*mids + l->textoworld[1][j]*midt;

	h = l->texsize[1]+1;
	w = l->texsize[0]+1;
	l->numsurfpt = w * h;

	step = 16/refine_amt;
	starts = l->texmins[0]*step;
	startt = l->texmins[1]*step;


	for (t=0 ; t<h ; t++)
	{
		for (s=0 ; s<w ; s++, surf+=3)
		{
			us = starts + (s+sofs)*step;
			ut = startt + (t+tofs)*step;


		// if a line can be traced from surf to facemid, the point is good
			for (i=0 ; i<6 ; i++)
			{
			// calculate texture point
				for (j=0 ; j<3 ; j++)
					surf[j] = l->texorg[j] + l->textoworld[0][j]*us
					+ l->textoworld[1][j]*ut;

				leaf = PointInLeaf (surf);
				if (leaf->contents != CONTENTS_SOLID)
				{
					if (!TestLine (facemid, surf))
						break;	// got it
				}

				// nudge it
				if (i & 1)
				{
					if (us > mids)
					{
						us -= 8;
						if (us < mids)
							us = mids;
					}
					else
					{
						us += 8;
						if (us > mids)
							us = mids;
					}
				}
				else
				{
					if (ut > midt)
					{
						ut -= 8;
						if (ut < midt)
							ut = midt;
					}
					else
					{
						ut += 8;
						if (ut > midt)
							ut = midt;
					}
				}
			}
		}
	}

}


//==============================================================


#define	MAX_STYLES	32
typedef struct
{
	int			numsamples;
	float		*origins;
	int			numstyles;
	int			stylenums[MAX_STYLES];
	sample_t	*samples[MAX_STYLES];
	sample_blur_t	*blur_amt;
} facelight_t;

directlight_t	*directlights[MAX_MAP_LEAFS];
facelight_t		facelight[MAX_MAP_FACES];
int				numdlights;

/*
==================
FindTargetEntity
==================
*/
entity_t *FindTargetEntity (char *target)
{
	int		i;
	char	*n;

	for (i=0 ; i<num_entities ; i++)
	{
		n = ValueForKey (&entities[i], "targetname");
		if (!strcmp (n, target))
			return &entities[i];
	}

	return NULL;
}

//#define	DIRECT_LIGHT	3000
#define	DIRECT_LIGHT	3

/*
=============
CreateDirectLights
=============
*/
void CreateDirectLights (void)
{
	int		i;
	patch_t	*p;
	directlight_t	*dl;
	dleaf_t	*leaf;
	int		cluster;
	entity_t	*e, *e2;
	char	*name;
	char	*target;
	float	angle;

	vec3_t	dest;
	char	*_color, *_channels;
	float	intensity;
	char	*sun_target = NULL;
	char	*proc_num;
	qboolean sun_light;

	//
	// entities
	//
	for (i=0 ; i<num_entities ; i++)
	{
		e = &entities[i];
		name = ValueForKey (e, "classname");
		if (strncmp (name, "light", 5))
		{
			if (!strncmp (name, "worldspawn", 10))
			{
				sun_target = ValueForKey(e, "_sun");
				if(strlen(sun_target) > 0)
				{
					printf("Sun activated.\n");
					sun = true;
				}

				proc_num = ValueForKey(e, "_sun_ambient");
				if(strlen(proc_num) > 0)
				{
					sun_ambient = atof(proc_num);
				}

				proc_num = ValueForKey(e, "_sun_light");
				if(strlen(proc_num) > 0)
				{
					sun_main = atof(proc_num);
				}

				proc_num = ValueForKey(e, "_sun_color");
				if(strlen(proc_num) > 0)
				{
					GetVectorForKey (e, "_sun_color", sun_color);

					sun_alt_color = true;
					ColorNormalize (sun_color, sun_color);
				}
			}

			continue;
		}

		sun_light = false;

		target = ValueForKey (e, "target");

		if(strlen(target) >= 1 && !strcmp(target, sun_target))
		{
			vec3_t sun_s, sun_t;

			GetVectorForKey(e, "origin", sun_s);

			sun_light = true;

			e2 = FindTargetEntity (target);

			if (!e2)
			{
				printf ("WARNING: sun missing target, 0,0,0 used\n");

				sun_t[0] = 0;
				sun_t[1] = 0;
				sun_t[2] = 0;
			}
			else
			{
				GetVectorForKey (e2, "origin", sun_t);
			}

			VectorSubtract (sun_s, sun_t, sun_pos);
			VectorNormalize (sun_pos, sun_pos);

			continue;
		}

		numdlights++;
		dl = malloc(sizeof(directlight_t));
		memset (dl, 0, sizeof(*dl));

		GetVectorForKey (e, "origin", dl->origin);
		dl->style = FloatForKey (e, "_style");
		if (!dl->style)
			dl->style = FloatForKey (e, "style");
		if (dl->style < 0 || dl->style >= MAX_LSTYLES)
			dl->style = 0;

		dl->nodenum = PointInNodenum (dl->origin);
		leaf = PointInLeaf (dl->origin);
		cluster = leaf->cluster;

		dl->next = directlights[cluster];
		directlights[cluster] = dl;

		proc_num = ValueForKey(e, "_wait");
		if(strlen(proc_num) > 0)
			dl->wait = atof(proc_num);
		else
		{
			proc_num = ValueForKey(e, "wait");

			if(strlen(proc_num) > 0)
				dl->wait = atof(proc_num);
			else
				dl->wait = 1.0f;
		}

		if (dl->wait <= 0.001)
			dl->wait = 1.0f;

		proc_num = ValueForKey(e, "_angwait");
		if(strlen(proc_num) > 0)
			dl->adjangle = atof(proc_num);
		else
			dl->adjangle = 1.0f;

		intensity = FloatForKey (e, "light");
		if (!intensity)
			intensity = FloatForKey (e, "_light");
		if (!intensity)
			intensity = 300;

		_color = ValueForKey (e, "_color");
		if (_color && _color[0])
		{
			sscanf (_color, "%f %f %f", &dl->color[0],&dl->color[1],&dl->color[2]);
			ColorNormalize (dl->color, dl->color);
		}
		else
			dl->color[0] = dl->color[1] = dl->color[2] = 1.0;

		_channels = ValueForKey (e, "_channel_proportions");
		if (_channels && _channels[0])
		{
			sscanf (_channels, "%f %f %f", &dl->proportion_direct, &dl->proportion_directsun, &dl->proportion_indirect);
		}
		else
		{
			dl->proportion_direct = 1.0f;
			dl->proportion_directsun = dl->proportion_indirect = 0.0f;
		}

		dl->intensity = intensity * entity_scale;
		dl->type = emit_point;

		target = ValueForKey (e, "target");

		if (!strcmp (name, "light_spot") || target[0])
		{
			dl->type = emit_spotlight;
			dl->stopdot = FloatForKey (e, "_cone");
			if (!dl->stopdot)
				dl->stopdot = 10;
			dl->stopdot = cos(dl->stopdot/180*3.14159);
			if (target[0])
			{	// point towards target
				e2 = FindTargetEntity (target);
				if (!e2)
					printf ("WARNING: light at (%i %i %i) has missing target\n",
					(int)dl->origin[0], (int)dl->origin[1], (int)dl->origin[2]);
				else
				{
					GetVectorForKey (e2, "origin", dest);
					VectorSubtract (dest, dl->origin, dl->normal);
					VectorNormalize (dl->normal, dl->normal);
				}
			}
			else
			{	// point down angle
				angle = FloatForKey (e, "angle");
				if (angle == ANGLE_UP)
				{
					dl->normal[0] = dl->normal[1] = 0;
					dl->normal[2] = 1;
				}
				else if (angle == ANGLE_DOWN)
				{
					dl->normal[0] = dl->normal[1] = 0;
					dl->normal[2] = -1;
				}
				else
				{
					dl->normal[2] = 0;
					dl->normal[0] = cos (angle/180*3.14159);
					dl->normal[1] = sin (angle/180*3.14159);
				}
			}
		}
	}


	//
	// surfaces
	//
	for (i=0, p=patches ; i<num_patches ; i++, p++)
	{
		vec3_t trace_start, tmp, tmp2;
		
		if ((!sun || !p->sky) && p->totallight[0] < DIRECT_LIGHT
			&& p->totallight[1] < DIRECT_LIGHT
			&& p->totallight[2] < DIRECT_LIGHT)
			continue;
		
		// If any light is underneath some terrain, it is "underground" and 
		// therefore should be ignored. This check is necessary because 
		// terrain is not used when constructing the BSP tree, so an area may
		// be solid or outside the map even if the BSP tree says it isn't.
		VectorCopy (p->origin, trace_start);
		trace_start[2] += 2048;
		if (!Terrain_Trace (trace_start, p->origin, tmp, tmp2))
			continue;

		numdlights++;
		dl = malloc(sizeof(directlight_t));
		memset (dl, 0, sizeof(*dl));

		VectorCopy (p->origin, dl->origin);

		leaf = PointInLeaf (dl->origin);
		cluster = leaf->cluster;
		dl->next = directlights[cluster];
		directlights[cluster] = dl;

		VectorCopy (p->plane->normal, dl->normal);

		if(sun && p->sky)
		{
			dl->leaf = leaf;
			dl->plane = p->plane;
			dl->type = emit_sky;
			dl->intensity = 1.0f;
		}
		else
		{
			dl->type = emit_surface;
			// surfaces are 50/50 indirect and direct for now. FIXME: we can do
			// better than this.
			dl->proportion_direct = dl->proportion_indirect = 0.5f;
			dl->proportion_directsun = 0.0f;
			dl->intensity = ColorNormalize (p->totallight, dl->color);
			dl->intensity *= p->area * direct_scale;
		}

		VectorClear (p->totallight);	// all sent now
	}


	qprintf ("%i direct lights\n", numdlights);
}

/*
 * Mar. 2009
 * Lighting equations for distance attenuation have been changed to
 * (what appears to be) the form of a standard lighting equation.
 * Coefficients need to be set empirically based on artistic criteria.
 * Helps to plot the curve. Real-world-correct inverse square law is to
 * extreme.
 */

/*
 * PointLight()
 *
 *  note: point lights are affected by -entity (entity_scale)
 */
float PointLight(float dot1, float intensity, float dist)
{
	float scale = 0.0f;

	if( dist < 32.0f ) // gets plenty bright at this distance
		dist = 32.0f;
	scale = 1.0f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
	scale *= intensity * dot1;

	return scale;
}

/*
 * SurfaceLight()
 *
 * note: surface lights are affected by -direct (direct_scale)
 *
 */
float SurfaceLight(float dot1, float intensity, float dist, float dot2, float invdist )
{
	float scale = 0.0f;

	if( dot2 > 0.001f )
	{ // point is not behind the surface light
		scale = 0.001f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
		scale *= intensity * dot1 * dot2;
	}

	return scale;
}

/*
 * SpotLight()
 *
 * note: spot lights are affected by -entity (entity_scale)
 */
float SpotLight( float dot1, float intensity, float dist, float dot2, float stopdot )
{
	float scale = 0.0f;

	if( dot2 >= stopdot )
	{ // surface point is within the spotlight cone
		// light to surface distance attenuation
		scale = 4.0f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
		// surface normal attenuation
		scale *= dot1;
		// spot center to surface point attenuation
		scale *= powf( dot2, 50.0f ); // dot2 range is limited, so exponent is big.
		// this term is not really necessary, could have spots with sharp cutoff
		//   and for fuzzy edges use multiple lights with different cones.

		scale *= intensity;
	}

	return scale;
}

#ifdef WIN32
static __inline int lowestCommonNode (int nodeNum1, int nodeNum2)
#else
static inline int lowestCommonNode (int nodeNum1, int nodeNum2)
#endif
{
	dnode_t *node;
	int child1, tmp, headNode = 0;
	
	if (nodeNum1 > nodeNum2)
	{
		tmp = nodeNum1;
		nodeNum1 = nodeNum2;
		nodeNum2 = tmp;
	}
	
re_test:
	//headNode is guaranteed to be <= nodeNum1 and nodeNum1 is < nodeNum2
	if (headNode == nodeNum1)
		return headNode;

	child1 = (node = dnodes+headNode)->children[1];
	
	if (nodeNum2 < child1) 
		//Both nodeNum1 and nodeNum2 are less than child1.
		//In this case, child0 is always a node, not a leaf, so we don't need
		//to check to make sure.
		headNode = node->children[0];
	else if (nodeNum1 < child1)
		//Child1 sits between nodeNum1 and nodeNum2. 
		//This means that headNode is the lowest node which contains both 
		//nodeNum1 and nodeNum2. 
		return headNode;
	else if (child1 > 0)
		//Both nodeNum1 and nodeNum2 are greater than child1.
		//If child1 is a node, that means it contains both nodeNum1 and 
		//nodeNum2.
		headNode = child1; 
	else
		//Child1 is a leaf, therefore by process of elimination child0 must be
		//a node and must contain boste nodeNum1 and nodeNum2.
		headNode = node->children[0];
	//goto instead of while(1) because it makes the CPU branch predict easier
	goto re_test; 
}

/*
=============
LightContributionToPoint
=============
*/
void LightContributionToPoint	(	directlight_t *l, vec3_t pos, int nodenum,
									vec3_t normal,
									sample_t *out_color,
									float lightscale2, // adjust for multisamples, -extra cmd line arg
									qboolean *sun_main_once, 
									qboolean *sun_ambient_once,
									sample_blur_t blur,
									occlusioncache_t *cache
								)
{
	vec3_t			delta, target, occluded, color_nodist;
	float			dot, dot2;
	float			dist;
	float			direct_scale = 0.0f, directsun_scale = 0.0f, indirect_scale = 0.0f, temp_scale = 0.0f;
	float			inv;
	int				i;
	int				lcn;
	float			combined_lightweight;
	vec_t			*curr_color = l->color;

	VectorClear (out_color->direct);
	VectorClear (out_color->directsun);
	VectorClear (out_color->indirect);
	
	if (l->type == emit_sky && *sun_main_once)
		return;
	
	VectorSubtract (l->origin, pos, delta);
	dist = DotProduct (delta, delta);
	
	if (dist == 0)
		return;
	
	dist = sqrt (dist);
	inv = 1.0f / dist;
	delta[0] *= inv;
	delta[1] *= inv;
	delta[2] *= inv;

	dot = DotProduct (delta, normal);
	if (dot <= 0.001)
		return;		// behind sample surface
	
	if( l->type == emit_sky )
	{  // this might be the sun ambient and it might be directional
		qboolean add_ambient, add_direct;
		
		// Should only add the direct light once per point. If we've added the
		// direct light, we've added the ambient light too, so there's nothing
		// left to do.
		if( *sun_main_once )
			return;

		dot2 = -DotProduct (delta, l->normal);
		if( dot2 <= 0.001f )
			return; // behind light surface
		
		// Should only add the ambient light once per point. If direct light
		// is added, then ambient light is added too, unless ambient light was
		// already added previously.
		add_ambient = !*sun_ambient_once; 

		dot2 = DotProduct (sun_pos, normal); // sun_pos from target entity
		
		// If this surface faces the sun, we could possibly add direct light.
        // Only if the trace directly to the sun's position fails do we fall
        // back on tracing to the directlight_t-- if a directlight_t is for
        // sunlight, we only trace to it for ambient light, and ambient light
        // is implied by direct light anyway.
		add_direct =	dot2 > 0.001f && (noblock ||
							(	RayPlaneIntersect (	l->plane->normal,
													l->plane->dist, pos,
													sun_pos, target ) &&
								!TestLine_color (0, pos, target, occluded, cache)));
		if (add_direct)
		{
			*sun_main_once = *sun_ambient_once = true;
			directsun_scale = sun_main * dot2;
		}
		else
		{
			if (!add_ambient)
				return;
			lcn = lowestCommonNode(nodenum, l->nodenum);
			if (!noblock && TestLine_color (lcn, pos, l->origin, occluded, cache))
				return;		// occluded
		}
		
		if (add_ambient)
		{
			*sun_ambient_once = true;
			indirect_scale = sun_ambient;
		}
		
		if (sun_alt_color) // set in .map
			curr_color = sun_color;
	}
	else
	{
		lcn = lowestCommonNode(nodenum, l->nodenum);
		if (!noblock && TestLine_color (lcn, pos, l->origin, occluded, cache))
			return;		// occluded
		
		switch ( l->type )
		{
		case emit_point:
			temp_scale = PointLight (dot, l->intensity, dist);
			break;

		case emit_surface:
			dot2 = -DotProduct (delta, l->normal);
			temp_scale = SurfaceLight (dot, l->intensity, dist, dot2, inv);
			break;

		case emit_spotlight:
			dot2 = -DotProduct(delta, l->normal);
			temp_scale = SpotLight (dot, l->intensity, dist, dot2, l->stopdot);
			break;

		default:
			Error("Invalid light entity type.\n" );
			break;
		} /* switch() */

		direct_scale = temp_scale * l->proportion_direct;
		directsun_scale = temp_scale * l->proportion_directsun;
		indirect_scale = temp_scale * l->proportion_indirect;
	}
	
	VectorScale (curr_color, direct_scale * lightscale2, out_color->direct);
	VectorScale (curr_color, directsun_scale * lightscale2, out_color->directsun);
	VectorScale (curr_color, indirect_scale * lightscale2, out_color->indirect);
	
	// 441.67 = roughly (sqrt(3*255^2))
	VectorScale (l->color, l->intensity/441.67, color_nodist);
	
	for (i = 0; i < 3; i++)
	{
		color_nodist[i] *= occluded[i];
		out_color->direct[i] *= occluded[i];
		out_color->directsun[i] *= occluded[i];
		out_color->indirect[i] *= occluded[i];
	}
	
	if (doing_blur)
	{
		float combined_lightweight = VectorLength (color_nodist)/sqrt(3.0);
		blur[0].total_blur += combined_lightweight;
		blur[0].num_lights++;
		if (l->proportion_direct != 0.0f)
		{
			blur[1].total_blur += combined_lightweight * l->proportion_direct;
			blur[1].num_lights++;
		}
		if (l->proportion_directsun != 0.0f)
		{
			blur[2].total_blur += combined_lightweight * l->proportion_directsun;
			blur[2].num_lights++;
		}
		if (l->proportion_indirect != 0.0f)
		{
			blur[3].total_blur = combined_lightweight * l->proportion_indirect;
			blur[3].num_lights++;
		}
	}
}

/*
=============
GatherSampleLight

Lightscale2 is the normalizer for multisampling, -extra cmd line arg
=============
*/

static void GatherSampleLight (vec3_t pos, sample_blur_t blur, vec3_t normal,
			sample_t **styletable, int offset, int mapsize, float lightscale2,
			qboolean *sun_main_once, qboolean *sun_ambient_once)
{
	int				i;
	directlight_t	*l;
	byte			pvs[(MAX_MAP_LEAFS+7)/8];
	sample_t		*dest;
	int				nodenum;
	occlusioncache_t cache;

	// get the PVS for the pos to limit the number of checks
	if (!PvsForOrigin (pos, pvs))
	{
		return;
	}
	nodenum = PointInNodenum(pos);
	
	memset (&cache, 0, sizeof(cache));

	for (i = 0 ; i<dvis->numclusters ; i++)
	{
		if ( ! (pvs[ i>>3] & (1<<(i&7))) )
			continue;

		for (l=directlights[i] ; l ; l=l->next)
		{
			sample_t sample;
			LightContributionToPoint (l, pos, nodenum, normal, &sample, lightscale2, sun_main_once, sun_ambient_once, blur, &cache);
			
			// no contribution
			if (VectorCompare (sample.direct, vec3_origin) && VectorCompare (sample.directsun, vec3_origin) && VectorCompare (sample.indirect, vec3_origin))
				continue;

			// if this style doesn't have a table yet, allocate one
			if (!styletable[l->style])
			{
				styletable[l->style] = malloc (mapsize);
				memset (styletable[l->style], 0, mapsize);
			}

			dest = styletable[l->style] + offset;
			VectorAdd (dest->direct, sample.direct, dest->direct);
			VectorAdd (dest->directsun, sample.directsun, dest->directsun);
			VectorAdd (dest->indirect, sample.indirect, dest->indirect);
		}
	}

}


/*
=============
AddSampleToPatch

Take the sample's collected light and
add it back into the apropriate patch
for the radiosity pass.

The sample is added to all patches that might include
any part of it.  They are counted and averaged, so it
doesn't generate extra light.
=============
*/
void AddSampleToPatch (vec3_t pos, vec3_t color, int facenum)
{
	patch_t	*patch;
	vec3_t	mins, maxs;
	int		i;

	if (numbounce == 0)
		return;
	if (color[0] + color[1] + color[2] < 3)
		return;

	for (patch = face_patches[facenum] ; patch ; patch=patch->next)
	{
		// see if the point is in this patch (roughly)
		WindingBounds (patch->winding, mins, maxs);
		for (i=0 ; i<3 ; i++)
		{
			if (mins[i] > pos[i] + 16/refine_amt)
				goto nextpatch;
			if (maxs[i] < pos[i] - 16/refine_amt)
				goto nextpatch;
		}

		// add the sample to the patch
		patch->samples++;
		VectorAdd (patch->samplelight, color, patch->samplelight);
nextpatch:;
	}

}


/*
=============
BuildFacelights
=============
*/
float	sampleofs[5][2] =
{  {0,0}, {-0.25, -0.25}, {0.25, -0.25}, {0.25, 0.25}, {-0.25, 0.25} };


void BuildFacelights (int facenum)
{
	dface_t	*this_face;
#ifdef STACK_CONSTRAINED
	lightinfo_t *liteinfo; //damn you, MSVC dinky stack!
#else
	lightinfo_t	liteinfo[5];
#endif
	sample_t	*styletable[MAX_LSTYLES];
	int			i, j;
	patch_t		*patch;
	int			numsamples;
	int			tablesize;
	facelight_t		*fl;
	qboolean	sun_main_once, sun_ambient_once;

	this_face = &dfaces[facenum];

	if ( texinfo[this_face->texinfo].flags & (SURF_WARP|SURF_SKY) )
		return;		// non-lit texture

#ifdef STACK_CONSTRAINED
	liteinfo = malloc(sizeof(lightinfo_t)*5);
#endif

	memset (styletable, 0, sizeof(styletable));

	if (extrasamples) // set with -extra option
		numsamples = 5;
	else
		numsamples = 1;
	for (i=0 ; i<numsamples ; i++)
	{
		memset (&liteinfo[i], 0, sizeof(liteinfo[i]));
		liteinfo[i].surfnum = facenum;
		liteinfo[i].face = this_face;
		VectorCopy (dplanes[this_face->planenum].normal, liteinfo[i].facenormal);
		liteinfo[i].facedist = dplanes[this_face->planenum].dist;
		if (this_face->side)
		{
			VectorSubtract (vec3_origin, liteinfo[i].facenormal, liteinfo[i].facenormal);
			liteinfo[i].facedist = -liteinfo[i].facedist;
		}

		// get the origin offset for rotating bmodels
		VectorCopy (face_offset[facenum], liteinfo[i].modelorg);

		CalcFaceVectors (&liteinfo[i]);
		CalcFaceExtents (&liteinfo[i]);
		CalcPoints (&liteinfo[i], sampleofs[i][0], sampleofs[i][1]);
	}

	tablesize = liteinfo[0].numsurfpt * sizeof(sample_t);
	styletable[0] = malloc(tablesize);
	memset (styletable[0], 0, tablesize);

	fl = &facelight[facenum];
	fl->numsamples = liteinfo[0].numsurfpt;
	fl->origins = malloc (tablesize);
	fl->blur_amt = malloc (liteinfo[0].numsurfpt * sizeof(sample_blur_t));
	memset (fl->blur_amt, 0, liteinfo[0].numsurfpt * sizeof(sample_blur_t));
	memcpy (fl->origins, liteinfo[0].surfpt, tablesize);

	for (i=0 ; i<liteinfo[0].numsurfpt ; i++)
	{
		sun_ambient_once = false;
		sun_main_once = false;

		for (j=0 ; j<numsamples ; j++)
		{
			GatherSampleLight (liteinfo[j].surfpt[i], fl->blur_amt[i], liteinfo[0].facenormal, styletable,
				i, tablesize, 1.0/numsamples, &sun_main_once, &sun_ambient_once);
		}

		// contribute the sample to one or more patches
		AddSampleToPatch (liteinfo[0].surfpt[i], styletable[0][i].direct, facenum);
		AddSampleToPatch (liteinfo[0].surfpt[i], styletable[0][i].directsun, facenum);
		AddSampleToPatch (liteinfo[0].surfpt[i], styletable[0][i].indirect, facenum);
	}

	// average up the direct light on each patch for radiosity
	for (patch = face_patches[facenum] ; patch ; patch=patch->next)
	{
		if (patch->samples)
		{
			VectorScale (patch->samplelight, 1.0/patch->samples, patch->samplelight);
		}
//		else
//		{
//			printf ("patch with no samples\n");
//		}
	}

	for (i=0 ; i<MAX_LSTYLES ; i++)
	{
		if (!styletable[i])
			continue;
		if (fl->numstyles == MAX_STYLES)
			break;
		fl->samples[fl->numstyles] = styletable[i];
		fl->stylenums[fl->numstyles] = i;
		fl->numstyles++;
	}

#ifdef STACK_CONSTRAINED
	free (liteinfo);
#endif
}


/*
=============
PostProcessLightSample

Apply -ambient, -maxlight, -scale, and -grayscale options to light sample, add
in radiosity light if applicable
=============
*/
void PostProcessLightSample (const sample_t *in, const vec3_t radiosity_add, sample_t *out, vec3_t out_combined_color)
{
	int i;
	vec_t max, newmax;
	vec3_t direct_proportion, directsun_proportion, indirect_proportion;

	memcpy (out, in, sizeof (sample_t));

	VectorAdd (out->indirect, radiosity_add, out->indirect);

	/*
	 * to allow experimenting, ambient and lightscale are not limited
	 *  to reasonable ranges.
	 */
	if (ambient >= -255.0f && ambient <= 255.0f)
	{ // add fixed white ambient.
		for (i = 0; i < 3; i++)
			out->indirect[i] += ambient;
	}
	if (lightscale > 0.0f)
	{ // apply lightscale, scale down or up
		VectorScale (out->direct, lightscale, out->direct);
		VectorScale (out->directsun, lightscale, out->directsun);
		VectorScale (out->indirect, lightscale, out->indirect);
	}
	// negative values not allowed
	for (i = 0; i < 3; i++)
	{
		out->direct[0] = (out->direct[0] < 0.0f) ? 0.0f : out->direct[0];
		out->directsun[0] = (out->directsun[0] < 0.0f) ? 0.0f : out->directsun[0];
		out->indirect[0] = (out->indirect[0] < 0.0f) ? 0.0f : out->indirect[0];
	}

	// Create combined color
	VectorCopy (out->direct, out_combined_color);
	VectorAdd (out->directsun, out_combined_color, out_combined_color);
	VectorAdd (out->indirect, out_combined_color, out_combined_color);
	for (i = 0; i < 3; i++)
	{
		if (out_combined_color[i] != 0.0f)
		{
			direct_proportion[i] = out->direct[i] / out_combined_color[i];
			directsun_proportion[i] = out->directsun[i] / out_combined_color[i];
			indirect_proportion[i] = out->indirect[i] / out_combined_color[i];
		}
		else
		{
			direct_proportion[i] = directsun_proportion[i] = indirect_proportion[i] = 0.0f;
		}
	}

	// determine max of R,G,B
	max = out_combined_color[0] > out_combined_color[1] ? out_combined_color[0] : out_combined_color[1];
	max = max > out_combined_color[2] ? max : out_combined_color[2];

	if (grayscale > 0.0f && grayscale <= 1.0f)
	{ // reduce color per NTSC model on combined color
		max = (0.299f * out_combined_color[0]) + (0.587f * out_combined_color[1]) * (0.144f * out_combined_color[2]);
		VectorScale (out_combined_color, 1.0f - grayscale, out_combined_color);
		for (i = 0; i < 3; i++)
			out_combined_color[i] += max * grayscale;
	}
	if (max < 1.0f)
		max = 1.0f;

	// note that maxlight based scaling is per-sample based on
	//  highest value of R, G, and B
	// adjust for -maxlight option
	newmax = max;
	if ( max > maxlight ) {
		newmax = maxlight;
		// scale into 0.0..maxlight range
		VectorScale (out_combined_color, newmax / max, out_combined_color);
	}

	// break processed combined color back into separate channels
	for (i = 0; i < 3; i++)
	{
		out->direct[i] = direct_proportion[i] * out_combined_color[i];
		out->directsun[i] = directsun_proportion[i] * out_combined_color[i];
		out->indirect[i] = indirect_proportion[i] * out_combined_color[i];
	}
}


/*
=============
FinalLightFace

Add the indirect lighting on top of the direct lighting
=============
*/
static void FinalLightFace (float *dest_bufs[4], int facenum)
{
	dface_t		*f;
	int			i, j, /*k,*/ st;
	patch_t		*patch;
	triangulation_t	*trian = NULL;
	facelight_t	*fl;
	// float		minlight;
	float		*dest_combined, *dest_direct, *dest_directsun, *dest_indirect;
	triangle_t	*last_valid;
	int			pfacenum;
	vec3_t		facemins, facemaxs;

	f = &dfaces[facenum];
	fl = &facelight[facenum];

	if ( texinfo[f->texinfo].flags & (SURF_WARP|SURF_SKY) )
		return;		// non-lit texture

	ThreadLock ();
	f->lightofs = lightdatasize;
	lightdatasize += fl->numstyles*(fl->numsamples*3);

	if (refine_setting == 0 && lightdatasize > MAX_MAP_LIGHTING)
	{
		printf ("face %d of %d\n", facenum, numfaces);
		Error ("MAX_MAP_LIGHTING");
	}
	if (refine_setting > 0 && lightdatasize > MAX_OVERRIDE_LIGHTING)
	{
		printf ("face %d of %d\n", facenum, numfaces);
		Error ("MAX_OVERRIDE_LIGHTING");
	}
	ThreadUnlock ();

	f->styles[0] = 0;
	f->styles[1] = f->styles[2] = f->styles[3] = 0xff;

	//
	// set up the triangulation
	//
	if (numbounce > 0)
	{
		ClearBounds (facemins, facemaxs);
		for (i=0 ; i<f->numedges ; i++)
		{
			int		ednum;

			ednum = dsurfedges[f->firstedge+i];
			if (ednum >= 0)
				AddPointToBounds (dvertexes[dedges[ednum].v[0]].point,
				facemins, facemaxs);
			else
				AddPointToBounds (dvertexes[dedges[-ednum].v[1]].point,
				facemins, facemaxs);
		}

		trian = AllocTriangulation (&dplanes[f->planenum]);

		// for all faces on the plane, add the nearby patches
		// to the triangulation
		for (pfacenum = planelinks[f->side][f->planenum]
			; pfacenum ; pfacenum = facelinks[pfacenum])
		{
			for (patch = face_patches[pfacenum] ; patch ; patch=patch->next)
			{
				for (i=0 ; i < 3 ; i++)
				{
					if (facemins[i] - patch->origin[i] > subdiv*2)
						break;
					if (patch->origin[i] - facemaxs[i] > subdiv*2)
						break;
				}
				if (i != 3)
					continue;	// not needed for this face
				AddPointToTriangulation (patch, trian);
			}
		}
		for (i=0 ; i<trian->numpoints ; i++)
			memset (trian->edgematrix[i], 0, trian->numpoints*sizeof(trian->edgematrix[0][0]) );
		TriangulatePoints (trian);
	}
	// _minlight allows models that have faces that would not be
	// illuminated to receive a mottled light pattern instead of
	// black
/*
	// 2010-09 - probably not used, too crude
 	minlight = FloatForKey (face_entity[facenum], "_minlight") * 128;
*/
	dest_combined = dest_bufs[0];
	dest_direct = dest_bufs[1];
	dest_directsun = dest_bufs[2];
	dest_indirect = dest_bufs[3];

	if (fl->numstyles > MAXLIGHTMAPS)
	{
		fl->numstyles = MAXLIGHTMAPS;
		printf ("face with too many lightstyles: (%f %f %f)\n",
			face_patches[facenum]->origin[0],
			face_patches[facenum]->origin[1],
			face_patches[facenum]->origin[2]
			);
	}

//	qprintf("Lightmap output: Styles: %d : Samples/Style: %d\n",
//			fl->numstyles, fl->numsamples );

	for (st=0 ; st<fl->numstyles ; st++)
	{
		last_valid = NULL;
		f->styles[st] = fl->stylenums[st];

		for (j=0 ; j<fl->numsamples ; j++)
		{
			sample_t processed;
			vec3_t combined_color, radiosity_add;

			VectorClear (radiosity_add);

			if (numbounce > 0 && st == 0)
				SampleTriangulation (fl->origins + j*3, trian, &last_valid, radiosity_add);

			PostProcessLightSample (fl->samples[st] + j, radiosity_add, &processed, combined_color);

			// and output to 8:8:8 RGB
			for (i = 0; i < 3; i++)
			{
				*dest_combined++ = combined_color[i];
				*dest_direct++ = processed.direct[i];
				*dest_directsun++ = processed.directsun[i];
				*dest_indirect++ = processed.indirect[i];
			}
		}
	}

	if (numbounce > 0)
		FreeTriangulation (trian);
}

/*
=============
BlurFace

Weighted blur for each face
=============
*/
static void BlurFace (float *src_bufs[4], float *dest_bufs[4], int facenum)
{
	dface_t		*f;
	facelight_t	*fl;
	const float	*src_buf, *src;
	float		*dest_buf, *dest;
	int			width, height, s, t;
	int			blur_radius;
	int			lightchannel;
	
	f = &dfaces[facenum];
	fl = &facelight[facenum];

	for (lightchannel = 0; lightchannel < 4; lightchannel++)
	{
		src_buf = src_bufs[lightchannel];
		dest_buf = dest_bufs[lightchannel];

		width = lfacelookups[facenum].width;
		height = lfacelookups[facenum].height;

		if (refine_amt == 16)
			blur_radius = 2;
		else
			blur_radius = 1;

		for (t = 0; t < height; t++)
		{
			for (s = 0; s < width; s++)
			{
				int i, j;
				double red = 0, green = 0, blue = 0, nsamples, blur_avg;

				if (fl->blur_amt[t*width+s][lightchannel].num_lights == 0)
				{
					// Special case-- this is for samples that have no direct 
					// illumination, and are only lit up by radiosity bounces, if
					// at all. Since radiosity isn't done at a high enough 
					// resolution to actually cast shadows, it makes no sense to
					// factor radiosity bouncing into the weighted average light
					// distance, so we don't. Which means, for this sample, there
					// *is* no weighted average light distance, so we don't do any
					// blurring.
					dest = &dest_buf[(t*width+s)*3];
					src = &src_buf[(t*width+s)*3];
					*dest++ = *src++;
					*dest++ = *src++;
					*dest++ = *src++;
					continue;
				}

				//blurring by distance needs a little work
				blur_avg = sqrt(sqrt(fl->blur_amt[t*width+s][lightchannel].total_blur/(double)fl->blur_amt[t*width+s][lightchannel].num_lights));
				nsamples = 2.0*(2.0-blur_avg);

				src = &src_buf[(t*width+s)*3];
				red = nsamples*src[0];
				green = nsamples*src[1];
				blue = nsamples*src[2];
		
#define SAMPLE(s,t,weight) \
				src = &src_buf[(t*width+s)*3];\
				nsamples += weight;\
				red += weight*src[0];\
				green += weight*src[1];\
				blue += weight*src[2];
	
#define SAMPLES(sd,td) \
				if (	((sd < 0 && s+sd >= 0) || (sd > 0 && s+sd < width) || !sd) &&\
						((td < 0 && t+td >= 0) || (td > 0 && t+td < height) || !td)) \
				{\
					SAMPLE((s+sd),(t+td),(blur_avg/sqrt(sd*sd+td*td)))\
				}

				for (i = -blur_radius; i < blur_radius+1; i++)
				{
					for (j = -blur_radius; j < blur_radius+1; j++)
					{
						if (i != 0 || j != 0)
						{
							SAMPLES(i,j);
						}
					}
				}

				dest = &dest_buf[(t*width+s)*3];
				dest[0] = red / nsamples;
				dest[1] = green / nsamples;
				dest[2] = blue / nsamples;
			}
		}
	}
}

/*
=============
QuantizeFace

Convert floating-point pixel data to integer and save into final map format
=============
*/
static void QuantizeFace (float *src_bufs[4], int facenum)
{
	dface_t		*f;
	facelight_t	*fl;
	float		*src;
	byte		*dest;
	int			i, size, lightchannel;

	f = &dfaces[facenum];
	fl = &facelight[facenum];

	for (lightchannel = 0; lightchannel < 4; lightchannel++)
	{
		src = src_bufs[lightchannel];
		dest = &dlightdata[lightchannel][f->lightofs];

		size = lfacelookups[facenum].width * lfacelookups[facenum].height * fl->numstyles * 3;
		for (i = 0; i < size; i++)
			*dest++ = (byte)((*src++) + 0.5f);
	}
}

void FinalLightFace_Worker (int threadnum)
{
	int channelnum, facenum;
	float *nonblurred_bufs[4], *blurred_bufs[4];

	// allocate scratch memory space for this thread
	for (channelnum = 0; channelnum < 4; channelnum++)
	{
		nonblurred_bufs[channelnum] = malloc (sizeof (float) * SINGLEMAP/4*3);
		if (doing_blur)
			blurred_bufs[channelnum] = malloc (sizeof (float) * SINGLEMAP/4*3);
	}

	while (1)
	{
		facenum = GetThreadWork ();
		if (facenum == -1)
			break;
		FinalLightFace (nonblurred_bufs, facenum);
		if (doing_blur)
		{
			BlurFace (nonblurred_bufs, blurred_bufs, facenum);
			QuantizeFace (blurred_bufs, facenum);
		}
		else
		{
			QuantizeFace (nonblurred_bufs, facenum);
		}
	}

	for (channelnum = 0; channelnum < 4; channelnum++)
	{
		free (nonblurred_bufs[channelnum]);
		if (doing_blur)
			free (blurred_bufs[channelnum]);
	}
}


/*
=============
DetectUniformColor

Detect if a face is all one color, and if so, shrink it to a 2x2 pixel block.
It is impossible to have any lightmap block smaller than 2 in any direction, 
because the game engine wants to save half a pixel on each side as a border.
Nonetheless, even a reducing it to 4 pixels saves considerable space in video
memory, allowing fewer lightmap textures to be used.

TODO: actually exclude the redundant data from the file. Not so important to 
do this, as file sizes are less important than final video memory usage, but
may be worthwhile anyway.
=============
*/
void DetectUniformColor (int facenum)
{
	byte		*sample_buf, *sample;
	int			width, height, s, t, style, newsize, oldsize, lightchannel;
	
	width = lfacelookups[facenum].width;
	height = lfacelookups[facenum].height;
	oldsize = width * height * 3;

	if (width < 2 || height < 2)
		return;
	
	for (lightchannel = 0; lightchannel < 4; lightchannel++)
	{
		sample_buf = &dlightdata[lightchannel][dfaces[facenum].lightofs];

		for (style = 0; style < MAXLIGHTMAPS && style < facelight[facenum].numstyles; style++)
		{
			for (t = 0; t < height; t++)
			{
				for (s = 0; s < width; s++)
				{
					sample = &sample_buf[(t*width+s)*3];
					if (sample[0] != sample_buf[0] || sample[1] != sample_buf[1] || sample[2] != sample_buf[2])
						return;
				}
			}
			sample_buf += oldsize;
		}
	}
	
	// just use a really big number
	lfacelookups[facenum].xscale = 10e20;
	lfacelookups[facenum].yscale = 10e20;
	
	lfacelookups[facenum].width = 2;
	lfacelookups[facenum].height = 2;

	// make sure the samples are in the right place for other lightstyles
	newsize = lfacelookups[facenum].width * lfacelookups[facenum].height * 3;
	assert (newsize <= oldsize);
	for (lightchannel = 0; lightchannel < 4; lightchannel++)
	{
		sample_buf = &dlightdata[lightchannel][dfaces[facenum].lightofs];
		for (style = 1; style < MAXLIGHTMAPS && style < facelight[facenum].numstyles; style++)
		{
			memcpy (sample_buf + style * newsize, sample_buf + style * oldsize, newsize);
			sample_buf += newsize;
		}
	}
}
