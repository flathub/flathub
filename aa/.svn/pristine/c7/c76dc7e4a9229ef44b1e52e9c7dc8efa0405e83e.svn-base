// map.c

#include "qbsp.h"
#include "ctype.h"

extern qboolean onlyents;

int			nummapbrushes;
mapbrush_t	mapbrushes[MAX_MAP_BRUSHES];

int			nummapbrushsides;
side_t		brushsides[MAX_MAP_SIDES];
brush_texture_t	side_brushtextures[MAX_MAP_SIDES];

int			nummapplanes;
plane_t		mapplanes[MAX_MAP_PLANES];

#define	PLANE_HASHES	1024
plane_t		*planehash[PLANE_HASHES];

vec3_t		map_mins, map_maxs;

// undefine to make plane finding use linear sort
#define	USE_HASHING

void TestExpandBrushes (void);

int		c_boxbevels;
int		c_edgebevels;

int		c_areaportals;

int		c_clipbrushes;

/*
=============================================================================

PLANE FINDING

=============================================================================
*/


/*
=================
PlaneTypeForNormal
=================
*/
int	PlaneTypeForNormal (vec3_t normal)
{
	vec_t	ax, ay, az;
	
// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] >= 1.0 || normal[0] <= -1.0)
		return PLANE_X;
	if (normal[1] >= 1.0 || normal[1] <= -1.0)
		return PLANE_Y;
	if (normal[2] >= 1.0 || normal[2] <= -1.0)
		return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);
	
	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

/*
================
PlaneEqual
================
*/
#define	NORMAL_EPSILON	0.00001
#define	DIST_EPSILON	0.01
qboolean	PlaneEqual (plane_t *p, vec3_t normal, vec_t dist)
{
#if 1
	if (
	   fabs(p->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(p->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(p->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(p->dist - dist) < DIST_EPSILON )
		return true;
#else
	if (p->normal[0] == normal[0]
		&& p->normal[1] == normal[1]
		&& p->normal[2] == normal[2]
		&& p->dist == dist)
		return true;
#endif
	return false;
}

/*
================
AddPlaneToHash
================
*/
void	AddPlaneToHash (plane_t *p)
{
	int		hash;

	hash = (int)fabs(p->dist) / 8;
	hash &= (PLANE_HASHES-1);

	p->hash_chain = planehash[hash];
	planehash[hash] = p;
}

/*
================
CreateNewFloatPlane
================
*/

static int show_points = 0;
float *last_points[3];

int CreateNewFloatPlane (vec3_t normal, vec_t dist)
{
	plane_t	*p, temp;

	float len = VectorLength(normal);

	if ((!badnormal_check && len < 0.5) || (badnormal_check &&
		(len + badnormal < 1.0f || len - badnormal < -1.0f)))
	{
		if(show_points)
			Error ("FloatPlane: bad normal\n  Plane Points: ( %g %g %g) ( %g %g %g) (%g %g %g)\n"
			"  Brush: %s\n",
				last_points[0][0],last_points[0][1],last_points[0][2],
				last_points[1][0],last_points[1][1],last_points[1][2],
				last_points[2][0],last_points[2][1],last_points[2][2],
				brush_info);
		else
			Error ("FloatPlane: bad normal");
	}
	// create a new plane
	if (nummapplanes+2 > MAX_MAP_PLANES)
		Error ("MAX_MAP_PLANES");

	p = &mapplanes[nummapplanes];
	VectorCopy (normal, p->normal);
	p->dist = dist;
	p->type = (p+1)->type = PlaneTypeForNormal (p->normal);

	VectorSubtract (vec3_origin, normal, (p+1)->normal);
	(p+1)->dist = -dist;

	nummapplanes += 2;

	// allways put axial planes facing positive first
	if (p->type < 3)
	{
		if (p->normal[0] < 0 || p->normal[1] < 0 || p->normal[2] < 0)
		{
			// flip order
			temp = *p;
			*p = *(p+1);
			*(p+1) = temp;

			AddPlaneToHash (p);
			AddPlaneToHash (p+1);
			return nummapplanes - 1;
		}
	}

	AddPlaneToHash (p);
	AddPlaneToHash (p+1);
	return nummapplanes - 2;
}

/*
==============
SnapVector
==============
*/
void	SnapVector (vec3_t normal)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(normal[i] - 1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = 1;
			break;
		}
		if ( fabs(normal[i] - -1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = -1;
			break;
		}
	}
}

/*
==============
SnapPlane
==============
*/
void	SnapPlane (vec3_t normal, vec_t *dist)
{
	SnapVector (normal);

	if (fabs(*dist-Q_rint(*dist)) < DIST_EPSILON)
		*dist = Q_rint(*dist);
}

/*
=============
FindFloatPlane

=============
*/
#ifndef USE_HASHING
int		FindFloatPlane (vec3_t normal, vec_t dist)
{
	int		i;
	plane_t	*p;

	SnapPlane (normal, &dist);
	for (i=0, p=mapplanes ; i<nummapplanes ; i++, p++)
	{
		if (PlaneEqual (p, normal, dist))
			return i;
	}

	return CreateNewFloatPlane (normal, dist);
}
#else
int		FindFloatPlane (vec3_t normal, vec_t dist)
{
	int		i;
	plane_t	*p;
	int		hash, h;

	SnapPlane (normal, &dist);
	hash = (int)fabs(dist) / 8;
	hash &= (PLANE_HASHES-1);

	// search the border bins as well
	for (i=-1 ; i<=1 ; i++)
	{
		h = (hash+i)&(PLANE_HASHES-1);
		for (p = planehash[h] ; p ; p=p->hash_chain)
		{
			if (PlaneEqual (p, normal, dist))
				return p-mapplanes;
		}
	}

	return CreateNewFloatPlane (normal, dist);
}
#endif

/*
================
PlaneFromPoints
================
*/

int PlaneFromPoints (float *p0, float *p1, float *p2)
{
	vec3_t	t1, t2, normal;
	vec_t	dist;

	last_points[0] = p0;
	last_points[1] = p1;
	last_points[2] = p2;

	VectorSubtract (p0, p1, t1);
	VectorSubtract (p2, p1, t2);
	CrossProduct (t1, t2, normal);
	VectorNormalize (normal, normal);

	dist = DotProduct (p0, normal);

	show_points = 1;
	return FindFloatPlane (normal, dist);
	show_points = 0;
}


//====================================================================


/*
===========
BrushContents
===========
*/
int	BrushContents (mapbrush_t *b)
{
	int			contents;
	side_t		*s;
	int			i;
	int			trans;

	s = &b->original_sides[0];
	contents = s->contents;
	trans = texinfo[s->texinfo].flags;
	for (i=1 ; i<b->numsides ; i++, s++)
    {
		s = &b->original_sides[i];
		trans |= texinfo[s->texinfo].flags;
		if (s->contents != contents)
		{
			printf ("Entity %i, Brush %i: mixed face contents\n  Brush: %s\n"
				, b->entitynum, b->brushnum, brush_info);
			break;
		}
	}

	// if any side is translucent, mark the contents
	// and change solid to window
	if ( trans & (SURF_TRANS33|SURF_TRANS66) )
	{
		contents |= CONTENTS_TRANSLUCENT;
		if (contents & CONTENTS_SOLID)
		{
			contents &= ~CONTENTS_SOLID;
			contents |= CONTENTS_WINDOW;
		}
	}
	
	if (trans & SURF_NODRAW)
	{
		if (contents & CONTENTS_DETAIL)
		{
			contents &= ~CONTENTS_SOLID;
			contents |= CONTENTS_WINDOW;
		}
	}

	return contents;
}


//============================================================================

/*
=================
AddBrushBevels

Adds any additional planes necessary to allow the brush to be expanded
against axial bounding boxes
=================
*/
winding_t *t_w;
int exceed = 0;
void AddBrushBevels (mapbrush_t *b)
{
	int		axis, dir;
	int		i, j, k, l, order;
	side_t	sidetemp;
	brush_texture_t	tdtemp;
	side_t	*s, *s2;
	vec3_t	normal;
	float	dist;
	winding_t	*w, *w2;
	vec3_t	vec, vec2;
	float	d;

	//
	// add the axial planes
	//
	order = 0;
	for (axis=0 ; axis <3 ; axis++)
	{
		for (dir=-1 ; dir <= 1 ; dir+=2, order++)
		{
			// see if the plane is allready present
			for (i=0, s=b->original_sides ; i<b->numsides ; i++,s++)
			{
				if (mapplanes[s->planenum].normal[axis] == dir)
					break;
			}

			if (i == b->numsides)
			{	// add a new side
				if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
					Error ("MAX_MAP_BRUSHSIDES");
				nummapbrushsides++;
				b->numsides++;

				VectorClear (normal);
				normal[axis] = dir;
				if (dir == 1)
					dist = b->maxs[axis];
				else
					dist = -b->mins[axis];
				s->planenum = FindFloatPlane (normal, dist);
				s->texinfo = b->original_sides[0].texinfo;
				s->contents = b->original_sides[0].contents;
				s->bevel = true;
				c_boxbevels++;
			}

			// if the plane is not in it canonical order, swap it
			if (i != order)
			{
				sidetemp = b->original_sides[order];
				b->original_sides[order] = b->original_sides[i];
				b->original_sides[i] = sidetemp;

				j = b->original_sides - brushsides;
				tdtemp = side_brushtextures[j+order];
				side_brushtextures[j+order] = side_brushtextures[j+i];
				side_brushtextures[j+i] = tdtemp;
			}
		}
	}

	//
	// add the edge bevels
	//
	if (b->numsides == 6)
		return;		// pure axial

	// test the non-axial plane edges
	for (i=6 ; i<b->numsides ; i++)
	{
		s = b->original_sides + i;
		w = s->winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			k = (j+1)%w->numpoints;
			VectorSubtract (w->p[j], w->p[k], vec);
			if (VectorNormalize (vec, vec) < 0.5)
				continue;
			SnapVector (vec);
			for (k=0 ; k<3 ; k++)
				if ( vec[k] == -1 || vec[k] == 1)
					break;	// axial
			if (k != 3)
				continue;	// only test non-axial edges

			// try the six possible slanted axials from this edge
			for (axis=0 ; axis <3 ; axis++)
			{
				for (dir=-1 ; dir <= 1 ; dir+=2)
				{
					// construct a plane
					VectorClear (vec2);
					vec2[axis] = dir;
					CrossProduct (vec, vec2, normal);
					if (VectorNormalize (normal, normal) < 0.5)
						continue;
					dist = DotProduct (w->p[j], normal);

					// if all the points on all the sides are
					// behind this plane, it is a proper edge bevel
					for (k=0 ; k<b->numsides ; k++)
					{
						// if this plane has allready been used, skip it
						if (PlaneEqual (&mapplanes[b->original_sides[k].planenum]
							, normal, dist) )
							break;

						w2 = b->original_sides[k].winding;
						if (!w2)
							continue;
						for (l=0 ; l<w2->numpoints ; l++)
						{
							d = DotProduct (w2->p[l], normal) - dist;
							if (d > 0.1)
								break;	// point in front
						}
						if (l != w2->numpoints)
							break;
					}

					if (k != b->numsides)
						continue;	// wasn't part of the outer hull
					// add this plane
					if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
						Error ("MAX_MAP_BRUSHSIDES");
					nummapbrushsides++;
					s2 = &b->original_sides[b->numsides];
					s2->planenum = FindFloatPlane (normal, dist);
					s2->texinfo = b->original_sides[0].texinfo;
					s2->contents = b->original_sides[0].contents;
					s2->bevel = true;
					c_edgebevels++;
					b->numsides++;

				}
			}
		}
	}
}

/*
================
MakeBrushWindings

makes basewindigs for sides and mins / maxs for the brush
================
*/

qboolean MakeBrushWindings (mapbrush_t *ob)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane;

	ClearBounds (ob->mins, ob->maxs);

	for (i=0 ; i<ob->numsides ; i++)
	{
		plane = &mapplanes[ob->original_sides[i].planenum];
		w = BaseWindingForPlane (plane->normal, plane->dist);

		for (j=0 ; j<ob->numsides && w; j++)
		{
			if (i == j)
				continue;
			if (ob->original_sides[j].bevel)
				continue;
			plane = &mapplanes[ob->original_sides[j].planenum^1];

			ChopWindingInPlace (&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		}

		side = &ob->original_sides[i];
		side->winding = w;
		if (w)
		{
			side->visible = true;
			for (j=0 ; j<w->numpoints ; j++)
				AddPointToBounds (w->p[j], ob->mins, ob->maxs);
		}
	}

	for (i=0 ; i<3 ; i++)
	{
		if (ob->mins[i] < -mapsize || ob->maxs[i] > mapsize)
		{
			printf ("entity %i, brush %i: bounds out of range\n", ob->entitynum, ob->brushnum);
			printf("  Brush: %s\n", brush_info);
			printf("  Bounds: %g %g %g -> %g %g %g\n",
				ob->mins[0], ob->mins[1], ob->mins[2], ob->maxs[0], ob->maxs[1], ob->maxs[2]);

			return true;
		}
		if (ob->mins[i] > mapsize || ob->maxs[i] < -mapsize)
		{
			printf ("entity %i, brush %i: no visible sides on brush\n", ob->entitynum, ob->brushnum);
			printf("  Brush: %s\n", brush_info);
			printf("  Bounds: %g %g %g -> %g %g %g\n",
				ob->mins[0], ob->mins[1], ob->mins[2], ob->maxs[0], ob->maxs[1], ob->maxs[2]);

			return true;
		}
	}

	return true;
}

/*
=================
ParseBrush
=================
*/
void ParseBrush (entity_t *mapent)
{
	char *tl;
	mapbrush_t		*b;
	int			i,j, k;
	int			mt;
	side_t		*side, *s2;
	int			planenum;
	brush_texture_t	td;
	float			planepts[3][3];

	MarkBrushBegin();

	if (nummapbrushes == MAX_MAP_BRUSHES)
		Error ("nummapbrushes == MAX_MAP_BRUSHES");

	b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = num_entities-1;
	b->brushnum = nummapbrushes - mapent->firstbrush;

	do
	{
		if (!GetToken (true))
			break;
		if (!strcmp (token, "}") )
			break;

		if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
			Error ("MAX_MAP_BRUSHSIDES");
		side = &brushsides[nummapbrushsides];

		// read the three point plane definition
		for (i=0 ; i<3 ; i++)
		{
			if (i != 0)
				GetToken (true);
			if (strcmp (token, "(") )
				Error ("parsing brush\n  Brush: %s", i+1, brush_info);
			
			for (j=0 ; j<3 ; j++)
			{
				GetToken (false);
				planepts[i][j] = atof(token);
			}
			
			GetToken (false);
			if (strcmp (token, ")") )
				Error ("parsing brush\n  Brush: %s", i+1, brush_info);
				
		}

		//
		// read the texturedef
		//
		GetToken (false);
        strcpy (td.name, token);

		tl = td.name;

		for(tl = td.name; *tl != 0; tl++)
			*tl = tolower(*tl);

		GetToken (false);
		td.shift[0] = atoi(token);
		GetToken (false);
		td.shift[1] = atoi(token);
		GetToken (false);
		td.rotate = atoi(token);	
		GetToken (false);
		td.scale[0] = atof(token);
		GetToken (false);
		td.scale[1] = atof(token);

		// find default flags and values
		mt = FindMiptex (td.name);
		td.flags = textureref[mt].flags;
		td.value = textureref[mt].value;
		side->contents = textureref[mt].contents;
		side->surf = td.flags = textureref[mt].flags;

		if (TokenAvailable())
		{
			GetToken (false);
			side->contents = atoi(token);
			GetToken (false);
			side->surf = td.flags = atoi(token);
			GetToken (false);
			td.value = atoi(token);
		}

		// translucent objects are automatically classified as detail
		if (side->surf & (SURF_TRANS33|SURF_TRANS66|SURF_NODRAW) )
			side->contents |= CONTENTS_DETAIL;
		if (side->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
			side->contents |= CONTENTS_DETAIL;
		if (fulldetail)
			side->contents &= ~CONTENTS_DETAIL;
		if (!(side->contents & ((LAST_VISIBLE_CONTENTS-1) 
			| CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_MIST)  ) )
			side->contents |= CONTENTS_SOLID;

		// hints and skips are never detail, and have no content
		if (side->surf & (SURF_HINT|SURF_SKIP) )
		{
			//side->contents = 0;
			//side->surf &= ~CONTENTS_DETAIL;
			side->contents &= CONTENTS_DETAIL;
		}

		//
		// find the plane number
		//
		planenum = PlaneFromPoints (planepts[0], planepts[1], planepts[2]);
		if (planenum == -1)
		{
			printf ("Entity %i, Brush %i: plane with no normal\n  Brush: %s\n"
				, b->entitynum, b->brushnum, brush_info);
			continue;
		}

		//
		// see if the plane has been used already
		//
		for (k=0 ; k<b->numsides ; k++)
		{
			s2 = b->original_sides + k;
			if (s2->planenum == planenum)
			{
				printf ("Entity %i, Brush %i: duplicate plane\n  Brush: %s\n"
					, b->entitynum, b->brushnum, brush_info);
				break;
			}
			if ( s2->planenum == (planenum^1) )
			{
				printf ("Entity %i, Brush %i: mirrored plane\n  Brush: %s\n"
					, b->entitynum, b->brushnum, brush_info);
				break;
			}
		}
		if (k != b->numsides)
			continue;		// duplicated

		//
		// keep this side
		//

		side = b->original_sides + b->numsides;
		side->planenum = planenum;
		side->texinfo = TexinfoForBrushTexture (&mapplanes[planenum],
			&td, vec3_origin);

		// save the td off in case there is an origin brush and we
		// have to recalculate the texinfo
		side_brushtextures[nummapbrushsides] = td;

		nummapbrushsides++;
		b->numsides++;
	} while (1);

	// get the content for the entire brush
	b->contents = BrushContents (b);

	// allow detail brushes to be removed 
	if (nodetail && (b->contents & CONTENTS_DETAIL) )
	{
		b->numsides = 0;
		return;
	}

	// allow water brushes to be removed
	if (nowater && (b->contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) )
	{
		b->numsides = 0;
		return;
	}

	// create windings for sides and bounds for brush

	//MEM_LEAK
	MakeBrushWindings (b);
	t_w = b->original_sides[0].winding;

	// brushes that will not be visible at all will never be
	// used as bsp splitters
	if (b->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
	{
		c_clipbrushes++;
		for (i=0 ; i<b->numsides ; i++)
			b->original_sides[i].texinfo = TEXINFO_NODE;
	}

	//
	// origin brushes are removed, but they set
	// the rotation origin for the rest of the brushes
	// in the entity.  After the entire entity is parsed,
	// the planenums and texinfos will be adjusted for
	// the origin brush
	//
	if (b->contents & CONTENTS_ORIGIN)
	{
		char	string[32];
		vec3_t	origin;

		if (num_entities == 1)
		{
			Error ("Entity %i, Brush %i: origin brushes not allowed in world\n  BrushL %s",
				b->entitynum, b->brushnum, brush_info);
			return;
		}

		VectorAdd (b->mins, b->maxs, origin);
		VectorScale (origin, 0.5, origin);

		sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue (&entities[b->entitynum], "origin", string);

		VectorCopy (origin, entities[b->entitynum].origin);

		// don't keep this brush
		b->numsides = 0;

		return;
	}

	AddBrushBevels (b);

	nummapbrushes++;
	mapent->numbrushes++;		
}

/*
================
MoveBrushesToWorld

Takes all of the brushes from the current entity and
adds them to the world's brush list.

Used by func_group and func_areaportal
================
*/
void MoveBrushesToWorld (entity_t *mapent)
{
	int			newbrushes;
	int			worldbrushes;
	mapbrush_t	*temp;
	int			i;

	// this is pretty gross, because the brushes are expected to be
	// in linear order for each entity

	newbrushes = mapent->numbrushes;
	worldbrushes = entities[0].numbrushes;

	temp = malloc(newbrushes*sizeof(mapbrush_t));
	memcpy (temp, mapbrushes + mapent->firstbrush, newbrushes*sizeof(mapbrush_t));

#if	0		// let them keep their original brush numbers
	for (i=0 ; i<newbrushes ; i++)
		temp[i].entitynum = 0;
#endif

	// make space to move the brushes (overlapped copy)
	memmove (mapbrushes + worldbrushes + newbrushes,
		mapbrushes + worldbrushes,
		sizeof(mapbrush_t) * (nummapbrushes - worldbrushes - newbrushes) );

	// copy the new brushes down
	memcpy (mapbrushes + worldbrushes, temp, sizeof(mapbrush_t) * newbrushes);

	// fix up indexes
	entities[0].numbrushes += newbrushes;
	for (i=1 ; i<num_entities ; i++)
		entities[i].firstbrush += newbrushes;
	free (temp);

	mapent->numbrushes = 0;
}

/*
================
ParseMapEntity
================
*/

qboolean	ParseMapEntity (void)
{
	entity_t	*mapent;
	epair_t		*e;
	side_t		*s;
	int			i, j;
	int			startbrush, startsides;
	vec_t		newdist;
	mapbrush_t	*b;

    if (!GetToken (true))
		return false;

	if (strcmp (token, "{") )
		Error ("ParseEntity: { not found\n  Last brush: %s", brush_info);
	
	if (num_entities == MAX_MAP_ENTITIES)
		Error ("num_entities == MAX_MAP_ENTITIES");

	startbrush = nummapbrushes;
	startsides = nummapbrushsides;

	mapent = &entities[num_entities];
	num_entities++;
	memset (mapent, 0, sizeof(*mapent));
	mapent->firstbrush = nummapbrushes;
	mapent->numbrushes = 0;
//	mapent->portalareas[0] = -1;
//	mapent->portalareas[1] = -1;

	do
	{
		if (!GetToken (true))
			Error ("ParseEntity: EOF without closing brace\n  Last brush: %s", brush_info);
		if (!strcmp (token, "}") )
			break;
		if (!strcmp (token, "{") )
            {
            ParseBrush (mapent);
            }
		else
		{
			e = ParseEpair ();
			e->next = mapent->epairs;
			mapent->epairs = e;
		}
	} while (1);

	GetVectorForKey (mapent, "origin", mapent->origin);

	//
	// if there was an origin brush, offset all of the planes and texinfo
	//
	if (mapent->origin[0] || mapent->origin[1] || mapent->origin[2])
	{
		for (i=0 ; i<mapent->numbrushes ; i++)
		{
			b = &mapbrushes[mapent->firstbrush + i];
			for (j=0 ; j<b->numsides ; j++)
			{
				s = &b->original_sides[j];
				newdist = mapplanes[s->planenum].dist -
					DotProduct (mapplanes[s->planenum].normal, mapent->origin);
				s->planenum = FindFloatPlane (mapplanes[s->planenum].normal, newdist);
				s->texinfo = TexinfoForBrushTexture (&mapplanes[s->planenum],
					&side_brushtextures[s-brushsides], mapent->origin);
			}

			MakeBrushWindings (b);
		}
	}

	// group entities are just for editor convenience
	// toss all brushes into the world entity
	if (!strcmp ("func_group", ValueForKey (mapent, "classname")))
	{
		MoveBrushesToWorld (mapent);
		mapent->numbrushes = 0;
		return true;
	}

	// areaportal entities move their brushes, but don't eliminate
	// the entity
	if (!strcmp ("func_areaportal", ValueForKey (mapent, "classname")))
	{
		char	str[128];

		if (mapent->numbrushes != 1)
			Error ("Entity %i: func_areaportal can only be a single brush\n  Last Brush: %s", num_entities-1, brush_info);

		b = &mapbrushes[nummapbrushes-1];
		b->contents = CONTENTS_AREAPORTAL;
		c_areaportals++;
		mapent->areaportalnum = c_areaportals;
		// set the portal number as "style"
		sprintf (str, "%i", c_areaportals);
		SetKeyValue (mapent, "style", str);
		MoveBrushesToWorld (mapent);
		return true;
	}

	return true;
}

//===================================================================

/*
================
LoadMapFile
================
*/
void LoadMapFile (char *filename)
{		
	int		i;

	qprintf ("--- LoadMapFile ---\n");

	LoadScriptFile (filename);

	nummapbrushsides = 0;
	num_entities = 0;
	
	while (ParseMapEntity ())
	{
	}

	ClearBounds (map_mins, map_maxs);
	for (i=0 ; i<entities[0].numbrushes ; i++)
	{
		if (mapbrushes[i].mins[0] > mapsize)
			continue;	// no valid points
		AddPointToBounds (mapbrushes[i].mins, map_mins, map_maxs);
		AddPointToBounds (mapbrushes[i].maxs, map_mins, map_maxs);
	}

	qprintf ("%5i brushes\n", nummapbrushes);
	qprintf ("%5i clipbrushes\n", c_clipbrushes);
	qprintf ("%5i total sides\n", nummapbrushsides);
	qprintf ("%5i boxbevels\n", c_boxbevels);
	qprintf ("%5i edgebevels\n", c_edgebevels);
	qprintf ("%5i entities\n", num_entities);
	qprintf ("%5i planes\n", nummapplanes);
	qprintf ("%5i areaportals\n", c_areaportals);
	qprintf ("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", map_mins[0],map_mins[1],map_mins[2],
		map_maxs[0],map_maxs[1],map_maxs[2]);

//	TestExpandBrushes ();
}


//====================================================================


/*
================
TestExpandBrushes

Expands all the brush planes and saves a new map out
================
*/
void TestExpandBrushes (void)
{
	FILE	*f;
	side_t	*s;
	int		i, j, bn;
	winding_t	*w;
	char	*name = "expanded.map";
	mapbrush_t	*brush;
	vec_t	dist;

	printf ("writing %s\n", name);
	f = fopen (name, "wb");
	if (!f)
		Error ("Can't write %s\b", name);

	fprintf (f, "{\n\"classname\" \"worldspawn\"\n");

	for (bn=0 ; bn<nummapbrushes ; bn++)
	{
		brush = &mapbrushes[bn];
		fprintf (f, "{\n");
		for (i=0 ; i<brush->numsides ; i++)
		{
			s = brush->original_sides + i;
			dist = mapplanes[s->planenum].dist;
			for (j=0 ; j<3 ; j++)
				dist += fabs( 16 * mapplanes[s->planenum].normal[j] );

			w = BaseWindingForPlane (mapplanes[s->planenum].normal, dist);

			fprintf (f,"( %g %g %g ) ", w->p[0][0], w->p[0][1], w->p[0][2]);
			fprintf (f,"( %g %g %g ) ", w->p[1][0], w->p[1][1], w->p[1][2]);
			fprintf (f,"( %g %g %g ) ", w->p[2][0], w->p[2][1], w->p[2][2]);

			fprintf (f, "%s 0 0 0 1 1\n", texinfo[s->texinfo].texture);
			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

	Error ("can't proceed after expanding brushes");
}
