
#include "qbsp.h"


int		c_active_portals;
int		c_peak_portals;
int		c_boundary;
int		c_boundary_sides;

/*
===========
AllocPortal
===========
*/

portal_t *AllocPortal (void)
{
	portal_t	*p;

	if (numthreads == 1)
		c_active_portals++;
	if (c_active_portals > c_peak_portals)
		c_peak_portals = c_active_portals;

	p = malloc (sizeof(portal_t));
	memset (p, 0, sizeof(portal_t));

	return p;
}

void FreePortal (portal_t *p)
{
	if (p->winding)
		FreeWinding (p->winding);
	if (numthreads == 1)
		c_active_portals--;
	free (p);

}

//==============================================================

/*
==============
VisibleContents

Returns the single content bit of the
strongest visible content present
==============
*/
int VisibleContents (int contents)
{
	int		i;

	for (i=1 ; i<=LAST_VISIBLE_CONTENTS ; i<<=1)
		if (contents & i )
			return i;

	return 0;
}


/*
===============
ClusterContents
===============
*/
int ClusterContents (node_t *node)
{
	int		c1, c2, c;

	if (node->planenum == PLANENUM_LEAF)
		return node->contents;

	c1 = ClusterContents(node->children[0]);
	c2 = ClusterContents(node->children[1]);
	c = c1|c2;

	// a cluster may include some solid detail areas, but
	// still be seen into
	if ( ! (c1&CONTENTS_SOLID) || ! (c2&CONTENTS_SOLID) )
		c &= ~CONTENTS_SOLID;
	return c;
}

/*
=============
Portal_VisFlood

Returns true if the portal is empty or translucent, allowing
the PVS calculation to see through it.
The nodes on either side of the portal may actually be clusters,
not leafs, so all contents should be ored together
=============
*/
qboolean Portal_VisFlood (portal_t *p)
{
	int		c1, c2;

	if (!p->onnode)
		return false;	// to global outsideleaf

	c1 = ClusterContents(p->nodes[0]);
	c2 = ClusterContents(p->nodes[1]);

	if (!VisibleContents (c1^c2))
		return true;

	if (c1 & (CONTENTS_TRANSLUCENT|CONTENTS_DETAIL))
		c1 = 0;
	if (c2 & (CONTENTS_TRANSLUCENT|CONTENTS_DETAIL))
		c2 = 0;

	if ( (c1|c2) & CONTENTS_SOLID )
		return false;		// can't see through solid

	if (! (c1 ^ c2))
		return true;		// identical on both sides

	if (!VisibleContents (c1^c2))
		return true;
	return false;
}


/*
===============
Portal_EntityFlood

The entity flood determines which areas are
"outside" on the map, which are then filled in.
Flowing from side s to side !s
===============
*/
qboolean Portal_EntityFlood (portal_t *p, int s)
{
	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF)
		Error ("Portal_EntityFlood: not a leaf");

	// can never cross to a solid
	if ( (p->nodes[0]->contents & CONTENTS_SOLID)
	|| (p->nodes[1]->contents & CONTENTS_SOLID) )
		return false;

	// can flood through everything else
	return true;
}


//=============================================================================

int		c_tinyportals;

/*
=============
AddPortalToNodes
=============
*/
void AddPortalToNodes (portal_t *p, node_t *front, node_t *back)
{
	if (p->nodes[0] || p->nodes[1])
		Error ("AddPortalToNode: allready included");

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;

	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
=============
RemovePortalFromNode
=============
*/
void RemovePortalFromNode (portal_t *portal, node_t *l)
{
	portal_t	**pp, *t;

// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			Error ("RemovePortalFromNode: portal not in leaf");

		if ( t == portal )
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			Error ("RemovePortalFromNode: portal not bounding leaf");
	}

	if (portal->nodes[0] == l)
	{
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	}
	else if (portal->nodes[1] == l)
	{
		*pp = portal->next[1];
		portal->nodes[1] = NULL;
	}
}

//============================================================================

void PrintPortal (portal_t *p)
{
	int			i;
	winding_t	*w;

	w = p->winding;
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.0f,%5.0f,%5.0f)\n",w->p[i][0]
		, w->p[i][1], w->p[i][2]);
}

/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
#define	SIDESPACE	8
void MakeHeadnodePortals (tree_t *tree)
{
	vec3_t		bounds[2];
	int			i, j, n;
	portal_t	*p, *portals[6];
	plane_t		bplanes[6], *pl;
	node_t *node;

	node = tree->headnode;

// pad with some space so there will never be null volume leafs
	for (i=0 ; i<3 ; i++)
	{
		bounds[0][i] = tree->mins[i] - SIDESPACE;
		bounds[1][i] = tree->maxs[i] + SIDESPACE;
	}

	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.contents = 0;

	for (i=0 ; i<3 ; i++)
		for (j=0 ; j<2 ; j++)
		{
			n = j*3 + i;

			p = AllocPortal ();
			portals[n] = p;

			pl = &bplanes[n];
			memset (pl, 0, sizeof(*pl));
			if (j)
			{
				pl->normal[i] = -1;
				pl->dist = -bounds[j][i];
			}
			else
			{
				pl->normal[i] = 1;
				pl->dist = bounds[j][i];
			}
			p->plane = *pl;
			p->winding = BaseWindingForPlane (pl->normal, pl->dist);
			AddPortalToNodes (p, node, &tree->outside_node);
		}

// clip the basewindings by all the other planes
	for (i=0 ; i<6 ; i++)
	{
		for (j=0 ; j<6 ; j++)
		{
			if (j == i)
				continue;
			ChopWindingInPlace (&portals[i]->winding, bplanes[j].normal, bplanes[j].dist, ON_EPSILON);
		}
	}
}

//===================================================


/*
================
BaseWindingForNode
================
*/
#define	BASE_WINDING_EPSILON	0.001
#define	SPLIT_WINDING_EPSILON	0.001

winding_t	*BaseWindingForNode (node_t *node)
{
	winding_t	*w;
	node_t		*n;
	plane_t		*plane;
	vec3_t		normal;
	vec_t		dist;

	w = BaseWindingForPlane (mapplanes[node->planenum].normal
		, mapplanes[node->planenum].dist);

	// clip by all the parents
	for (n=node->parent ; n && w ; )
	{
		plane = &mapplanes[n->planenum];

		if (n->children[0] == node)
		{	// take front
			ChopWindingInPlace (&w, plane->normal, plane->dist, BASE_WINDING_EPSILON);
		}
		else
		{	// take back
			VectorSubtract (vec3_origin, plane->normal, normal);
			dist = -plane->dist;
			ChopWindingInPlace (&w, normal, dist, BASE_WINDING_EPSILON);
		}
		node = n;
		n = n->parent;
	}

	return w;
}

//============================================================

qboolean WindingIsTiny (winding_t *w);

/*
==================
MakeNodePortal

create the new portal by taking the full plane winding for the cutting plane
and clipping it by all of parents of this node
==================
*/
void MakeNodePortal (node_t *node)
{
	portal_t	*new_portal, *p;
	winding_t	*w;
	vec3_t		normal;
	float		dist = 0.0f;
	int			side = -1;

	w = BaseWindingForNode (node);

	// clip the portal by all the other portals in the node
	for (p = node->portals ; p && w; p = p->next[side])
	{
		if (p->nodes[0] == node)
		{
			side = 0;
			VectorCopy (p->plane.normal, normal);
			dist = p->plane.dist;
		}
		else if (p->nodes[1] == node)
		{
			side = 1;
			VectorSubtract (vec3_origin, p->plane.normal, normal);
			dist = -p->plane.dist;
		}
		else
			Error ("CutNodePortals_r: mislinked portal");

		ChopWindingInPlace (&w, normal, dist, 0.1);
	}

	if (!w)
	{
		return;
	}

	if (WindingIsTiny (w))
	{
		c_tinyportals++;
		FreeWinding (w);
		return;
	}


	new_portal = AllocPortal ();
	new_portal->plane = mapplanes[node->planenum];
	new_portal->onnode = node;
	new_portal->winding = w;
	AddPortalToNodes (new_portal, node->children[0], node->children[1]);
}


/*
==============
SplitNodePortals

Move or split the portals that bound node so that the node's
children have portals instead of node.
==============
*/
void SplitNodePortals (node_t *node)
{
	portal_t	*p, *next_portal, *new_portal;
	node_t		*f, *b, *other_node;
	int			side = -1;
	plane_t		*plane;
	winding_t	*frontwinding, *backwinding;

	plane = &mapplanes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for (p = node->portals ; p ; p = next_portal)
	{
		if (p->nodes[0] == node)
			side = 0;
		else if (p->nodes[1] == node)
			side = 1;
		else
			Error ("CutNodePortals_r: mislinked portal");
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);

//
// cut the portal into two portals, one on each side of the cut plane
//
		ClipWindingEpsilon (p->winding, plane->normal, plane->dist,
			SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if (frontwinding && WindingIsTiny(frontwinding))
		{
			FreeWinding (frontwinding);
			frontwinding = NULL;
			c_tinyportals++;
		}

		if (backwinding && WindingIsTiny(backwinding))
		{
			FreeWinding (backwinding);
			backwinding = NULL;
			c_tinyportals++;
		}

		if (!frontwinding && !backwinding)
		{	// tiny windings on both sides
			continue;
		}

		if (!frontwinding)
		{
			FreeWinding (backwinding);
			if (side == 0)
				AddPortalToNodes (p, b, other_node);
			else
				AddPortalToNodes (p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			FreeWinding (frontwinding);
			if (side == 0)
				AddPortalToNodes (p, f, other_node);
			else
				AddPortalToNodes (p, other_node, f);
			continue;
		}

	// the winding is split
		new_portal = AllocPortal ();
		*new_portal = *p;
		new_portal->winding = backwinding;
		FreeWinding (p->winding);
		p->winding = frontwinding;

		if (side == 0)
		{
			AddPortalToNodes (p, f, other_node);
			AddPortalToNodes (new_portal, b, other_node);
		}
		else
		{
			AddPortalToNodes (p, other_node, f);
			AddPortalToNodes (new_portal, other_node, b);
		}
	}

	node->portals = NULL;
}


/*
================
CalcNodeBounds
================
*/
void CalcNodeBounds (node_t *node)
{
	portal_t	*p;
	int			s;
	int			i;

	// calc mins/maxs for both leafs and nodes
	ClearBounds (node->mins, node->maxs);
	for (p = node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);
		for (i=0 ; i<p->winding->numpoints ; i++)
			AddPointToBounds (p->winding->p[i], node->mins, node->maxs);
	}
}


/*
==================
MakeTreePortals_r
==================
*/
void MakeTreePortals_r (node_t *node)
{
	int		i;

	CalcNodeBounds (node);
	if (node->mins[0] >= node->maxs[0])
	{
		printf ("WARNING: node without a volume\n");
		printf("  Bounds: %g %g %g -> %g %g %g\n",
			node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
	}

	for (i=0 ; i<3 ; i++)
	{
		if (node->mins[i] < -mapsize*1.5 || node->maxs[i] > mapsize*1.5)
		{
			printf ("WARNING: node with unbounded volume\n");
			printf("  Bounds: %g %g %g -> %g %g %g\n",
				node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
			break;
		}
	}
	if (node->planenum == PLANENUM_LEAF)
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	MakeTreePortals_r (node->children[0]);
	MakeTreePortals_r (node->children[1]);
}

/*
==================
MakeTreePortals
==================
*/
void MakeTreePortals (tree_t *tree)
{
	MakeHeadnodePortals (tree);
	MakeTreePortals_r (tree->headnode);
}

/*
=========================================================

FLOOD ENTITIES

=========================================================
*/

/*
=============
FloodPortals_r
=============
*/
void FloodPortals_r (node_t *node, int dist)
{
	portal_t	*p;
	int			s;

	node->occupied = dist;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);

		if (p->nodes[!s]->occupied)
			continue;

		if (!Portal_EntityFlood (p, s))
			continue;

		FloodPortals_r (p->nodes[!s], dist+1);
	}
}

/*
=============
PlaceOccupant
=============
*/
qboolean PlaceOccupant (node_t *headnode, vec3_t origin, entity_t *occupant)
{
	node_t	*node;
	vec_t	d;
	plane_t	*plane;

	// find the leaf to start in
	node = headnode;
	while (node->planenum != PLANENUM_LEAF)
	{
		plane = &mapplanes[node->planenum];
		d = DotProduct (origin, plane->normal) - plane->dist;
		if (d >= 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	if (node->contents == CONTENTS_SOLID)
		return false;
	node->occupant = occupant;

	FloodPortals_r (node, 1);

	return true;
}

/*
=============
FloodEntities

Marks all nodes that can be reached by entites
=============
*/
qboolean FloodEntities (tree_t *tree)
{
	int		i;
	vec3_t	origin;
	char	*cl;
	qboolean	inside;
	node_t *headnode;

	headnode = tree->headnode;
	qprintf ("--- FloodEntities ---\n");
	inside = false;
	tree->outside_node.occupied = 0;

	for (i=1 ; i<num_entities ; i++)
	{
		GetVectorForKey (&entities[i], "origin", origin);
		if (VectorCompare(origin, vec3_origin))
			continue;

		cl = ValueForKey (&entities[i], "classname");
		origin[2] += 1;	// so objects on floor are ok

		// nudge playerstart around if needed so clipping hulls allways
		// have a vlaid point
		if (!strcmp (cl, "info_player_start"))
		{
			int	x, y;

			for (x=-16 ; x<=16 ; x += 16)
			{
				for (y=-16 ; y<=16 ; y += 16)
				{
					origin[0] += x;
					origin[1] += y;
					if (PlaceOccupant (headnode, origin, &entities[i]))
					{
						inside = true;
						goto gotit;
					}
					origin[0] -= x;
					origin[1] -= y;
				}
			}
gotit: ;
		}
		else
		{
			if (PlaceOccupant (headnode, origin, &entities[i]))
				inside = true;
		}
	}

	if (!inside)
	{
		qprintf ("no entities in open -- no filling\n");
	}
	else if (tree->outside_node.occupied)
	{
		qprintf ("entity reached from outside -- no filling\n");
	}

	return (qboolean)(inside && !tree->outside_node.occupied);
}

/*
=========================================================

FLOOD AREAS

=========================================================
*/

int		c_areas;

/*
=============
FloodAreas_r
=============
*/
void FloodAreas_r (node_t *node)
{
	portal_t	*p;
	int			s;
	bspbrush_t	*b;
	entity_t	*e;

	if (node->contents == CONTENTS_AREAPORTAL)
	{
		// this node is part of an area portal
		b = node->brushlist;
		e = &entities[b->original->entitynum];

		// if the current area has allready touched this
		// portal, we are done
		if (e->portalareas[0] == c_areas || e->portalareas[1] == c_areas)
			return;

		// note the current area as bounding the portal
		if (e->portalareas[1])
		{
			printf ("WARNING: areaportal entity %i touches > 2 areas\n  Node Bounds: %g %g %g -> %g %g %g\n", b->original->entitynum,
				node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
			return;
		}
		if (e->portalareas[0])
			e->portalareas[1] = c_areas;
		else
			e->portalareas[0] = c_areas;

		return;
	}

	if (node->area)
		return;		// allready got it
	node->area = c_areas;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);
#if 0
		if (p->nodes[!s]->occupied)
			continue;
#endif
		if (!Portal_EntityFlood (p, s))
			continue;

		FloodAreas_r (p->nodes[!s]);
	}
}

/*
=============
FindAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void FindAreas_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FindAreas_r (node->children[0]);
		FindAreas_r (node->children[1]);
		return;
	}

	if (node->area)
		return;		// allready got it

	if (node->contents & CONTENTS_SOLID)
		return;

	if (!node->occupied)
		return;			// not reachable by entities

	// area portals are allways only flooded into, never
	// out of
	if (node->contents == CONTENTS_AREAPORTAL)
		return;

	c_areas++;
	FloodAreas_r (node);
}

/*
=============
SetAreaPortalAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void SetAreaPortalAreas_r (node_t *node)
{
	bspbrush_t	*b;
	entity_t	*e;

	if (node->planenum != PLANENUM_LEAF)
	{
		SetAreaPortalAreas_r (node->children[0]);
		SetAreaPortalAreas_r (node->children[1]);
		return;
	}

	if (node->contents == CONTENTS_AREAPORTAL)
	{
		if (node->area)
			return;		// allready set

		b = node->brushlist;
		e = &entities[b->original->entitynum];
		node->area = e->portalareas[0];
		if (!e->portalareas[1])
		{
			printf ("WARNING: areaportal entity %i doesn't touch two areas\n  Node Bounds: %g %g %g -> %g %g %g\n", b->original->entitynum,
					node->mins[0], node->mins[1], node->mins[2], node->maxs[0], node->maxs[1], node->maxs[2]);
			return;
		}
	}
}

/*
=============
EmitAreaPortals

=============
*/
void EmitAreaPortals (node_t *headnode)
{
	int				i, j;
	entity_t		*e;
	dareaportal_t	*dp;

	if (c_areas > MAX_MAP_AREAS)
		Error ("MAX_MAP_AREAS");
	numareas = c_areas+1;
	numareaportals = 1;		// leave 0 as an error

	for (i=1 ; i<=c_areas ; i++)
	{
		dareas[i].firstareaportal = numareaportals;
		for (j=0 ; j<num_entities ; j++)
		{
			e = &entities[j];
			if (!e->areaportalnum)
				continue;
			dp = &dareaportals[numareaportals];
			if (e->portalareas[0] == i)
			{
				dp->portalnum = e->areaportalnum;
				dp->otherarea = e->portalareas[1];
				numareaportals++;
			}
			else if (e->portalareas[1] == i)
			{
				dp->portalnum = e->areaportalnum;
				dp->otherarea = e->portalareas[0];
				numareaportals++;
			}
		}
		dareas[i].numareaportals = numareaportals - dareas[i].firstareaportal;
	}

	qprintf ("%5i numareas\n", numareas);
	qprintf ("%5i numareaportals\n", numareaportals);
}

/*
=============
FloodAreas

Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
=============
*/
void FloodAreas (tree_t *tree)
{
	qprintf ("--- FloodAreas ---\n");
	FindAreas_r (tree->headnode);
	SetAreaPortalAreas_r (tree->headnode);
	qprintf ("%5i areas\n", c_areas);
}

//======================================================

int		c_outside;
int		c_inside;
int		c_solid;

void FillOutside_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r (node->children[0]);
		FillOutside_r (node->children[1]);
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if (!node->occupied)
	{
		if (node->contents != CONTENTS_SOLID)
		{
			c_outside++;
			node->contents = CONTENTS_SOLID;
		}
		else
			c_solid++;
	}
	else
		c_inside++;

}

/*
=============
FillOutside

Fill all nodes that can't be reached by entities
=============
*/
void FillOutside (node_t *headnode)
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	qprintf ("--- FillOutside ---\n");
	FillOutside_r (headnode);
	qprintf ("%5i solid leafs\n", c_solid);
	qprintf ("%5i leafs filled\n", c_outside);
	qprintf ("%5i inside leafs\n", c_inside);
}


//==============================================================

/*
============
FindPortalSide

Finds a brush side to use for texturing the given portal
============
*/
void FindPortalSide (portal_t *p)
{
	int			viscontents;
	bspbrush_t	*bb;
	mapbrush_t	*brush;
	node_t		*n;
	int			i,j;
	int			planenum;
	side_t		*side, *bestside;
	float		dot, bestdot;
	plane_t		*p1, *p2;

	// decide which content change is strongest
	// solid > lava > water, etc
	viscontents = VisibleContents (p->nodes[0]->contents ^ p->nodes[1]->contents);
	if (!viscontents)
		return;

	planenum = p->onnode->planenum;
	bestside = NULL;
	bestdot = 0;

	for (j=0 ; j<2 ; j++)
	{
		n = p->nodes[j];
		p1 = &mapplanes[p->onnode->planenum];
		for (bb=n->brushlist ; bb ; bb=bb->next)
		{
			brush = bb->original;
			if ( !(brush->contents & viscontents) )
				continue;
			for (i=0 ; i<brush->numsides ; i++)
			{
				side = &brush->original_sides[i];
				if (side->bevel)
					continue;
				if (side->texinfo == TEXINFO_NODE)
					continue;		// non-visible
				if ((side->planenum&~1) == planenum)
				{	// exact match
					bestside = &brush->original_sides[i];
					goto gotit;
				}
				// see how close the match is
				p2 = &mapplanes[side->planenum&~1];
				dot = DotProduct (p1->normal, p2->normal);
				if (dot > bestdot)
				{
					bestdot = dot;
					bestside = side;
				}
			}
		}
	}

gotit:
	if (!bestside)
		qprintf ("WARNING: side not found for portal %p %f %f %f %f\n", p, p->plane.normal[0], p->plane.normal[1], p->plane.normal[2], p->plane.dist);

	p->sidefound = true;
	p->side = bestside;
}


/*
===============
MarkVisibleSides_r

===============
*/
void MarkVisibleSides_r (node_t *node)
{
	portal_t	*p;
	int			s;

	if (node->planenum != PLANENUM_LEAF)
	{
		MarkVisibleSides_r (node->children[0]);
		MarkVisibleSides_r (node->children[1]);
		return;
	}

	// empty leafs are never boundary leafs
	if (!node->contents)
		return;

	// see if there is a visible face
	for (p=node->portals ; p ; p = p->next[!s])
	{
		s = (p->nodes[0] == node);
		if (!p->onnode)
			continue;		// edge of world
		if (!p->sidefound)
			FindPortalSide (p);
		if (p->side)
			p->side->visible = true;
	}

}

/*
=============
MarkVisibleSides

=============
*/
void MarkVisibleSides (tree_t *tree, int startbrush, int endbrush)
{
	int		i, j;
	mapbrush_t	*mb;
	int		numsides;

	qprintf ("--- MarkVisibleSides ---\n");

	// clear all the visible flags
	for (i=startbrush ; i<endbrush ; i++)
	{
		mb = &mapbrushes[i];

		numsides = mb->numsides;
		for (j=0 ; j<numsides ; j++)
			mb->original_sides[j].visible = false;
	}

	// set visible flags on the sides that are used by portals
	MarkVisibleSides_r (tree->headnode);
}

