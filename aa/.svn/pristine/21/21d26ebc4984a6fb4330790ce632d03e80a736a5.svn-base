#include "vis.h"

/*

  each portal will have a list of all possible to see from first portal

  if (!thread->portalmightsee[portalnum])

  portal mightsee

  for p2 = all other portals in leaf
	get sperating planes
	for all portals that might be seen by p2
		mark as unseen if not present in seperating plane
	flood fill a new mightsee
	save as passagemightsee


  void CalcMightSee (leaf_t *leaf, 
*/

int CountBits (byte *bits, int numbits)
{
	int		i;
	int		c;

	c = 0;
	for (i=0 ; i<numbits ; i++)
		if (bits[i>>3] & (1<<(i&7)) )
			c++;

	return c;
}

int		c_fullskip;
int		c_portalskip, c_leafskip;
int		c_vistest, c_mighttest;

int		c_chop, c_nochop;

int		active;

void CheckStack (leaf_t *leaf, threaddata_t *thread)
{
	pstack_t	*p, *p2;

	for (p=thread->pstack_head.next ; p ; p=p->next)
	{
//		printf ("=");
		if (p->leaf == leaf)
			Error ("CheckStack: leaf recursion");
		for (p2=thread->pstack_head.next ; p2 != p ; p2=p2->next)
			if (p2->leaf == p->leaf)
				Error ("CheckStack: late leaf recursion");
	}
//	printf ("\n");
}


winding_t *AllocStackWinding (pstack_t *stack)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (stack->freewindings[i])
		{
			stack->freewindings[i] = 0;
			return &stack->windings[i];
		}
	}

	Error ("AllocStackWinding: failed");

	return NULL;
}

void FreeStackWinding (winding_t *w, pstack_t *stack)
{
	int		i;

	i = w - stack->windings;

	if (i<0 || i>2)
		return;		// not from local

	if (stack->freewindings[i])
		Error ("FreeStackWinding: allready free");
	stack->freewindings[i] = 1;
}

/*
==============
ChopWinding

==============
*/
winding_t	*ChopWinding (winding_t *in, pstack_t *stack, plane_t *split)
{
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t	*neww;

	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[1])
		return in;		// completely on front side
	
	if (!counts[0])
	{
		FreeStackWinding (in, stack);
		return NULL;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	neww = AllocStackWinding (stack);

	neww->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			FreeStackWinding (neww, stack);
			return in;		// can't chop -- fall back to original
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			FreeStackWinding (neww, stack);
			return in;		// can't chop -- fall back to original
		}

	// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	
// free the original winding
	FreeStackWinding (in, stack);
	
	return neww;
}


/*
==============
ClipToSeperators

Source, pass, and target are an ordering of portals.

Generates seperating planes canidates by taking two points from source and one
point from pass, and clips target by them.

If target is totally clipped away, that portal can not be seen through.

Normal clip keeps target on the same side as pass, which is correct if the
order goes source, pass, target.  If the order goes pass, source, target then
flipclip should be set.
==============
*/
winding_t	*ClipToSeperators (winding_t *source, winding_t *pass, winding_t *target, qboolean flipclip, pstack_t *stack)
{
	int			i, j, k, l;
	plane_t		plane;
	vec3_t		v1, v2;
	float		d;
	vec_t		length;
	int			counts[3];
	qboolean		fliptest;

// check all combinations	
	for (i=0 ; i<source->numpoints ; i++)
	{
		l = (i+1)%source->numpoints;
		VectorSubtract (source->points[l] , source->points[i], v1);

	// fing a vertex of pass that makes a plane that puts all of the
	// vertexes of pass on the front side and all of the vertexes of
	// source on the back side
		for (j=0 ; j<pass->numpoints ; j++)
		{
			VectorSubtract (pass->points[j], source->points[i], v2);

			plane.normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
			plane.normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
			plane.normal[2] = v1[0]*v2[1] - v1[1]*v2[0];
			
		// if points don't make a valid plane, skip it

			length = plane.normal[0] * plane.normal[0]
			+ plane.normal[1] * plane.normal[1]
			+ plane.normal[2] * plane.normal[2];
			
			if (length < ON_EPSILON)
				continue;

			length = 1/sqrt(length);
			
			plane.normal[0] *= length;
			plane.normal[1] *= length;
			plane.normal[2] *= length;

			plane.dist = DotProduct (pass->points[j], plane.normal);

		//
		// find out which side of the generated seperating plane has the
		// source portal
		//
#if 1
			fliptest = false;
			for (k=0 ; k<source->numpoints ; k++)
			{
				if (k == i || k == l)
					continue;
				d = DotProduct (source->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
				{	// source is on the negative side, so we want all
					// pass and target on the positive side
					fliptest = false;
					break;
				}
				else if (d > ON_EPSILON)
				{	// source is on the positive side, so we want all
					// pass and target on the negative side
					fliptest = true;
					break;
				}
			}
			if (k == source->numpoints)
				continue;		// planar with source portal
#else
			fliptest = flipclip;
#endif
		//
		// flip the normal if the source portal is backwards
		//
			if (fliptest)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}
#if 1
		//
		// if all of the pass portal points are now on the positive side,
		// this is the seperating plane
		//
			counts[0] = counts[1] = counts[2] = 0;
			for (k=0 ; k<pass->numpoints ; k++)
			{
				if (k==j)
					continue;
				d = DotProduct (pass->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
					break;
				else if (d > ON_EPSILON)
					counts[0]++;
				else
					counts[2]++;
			}
			if (k != pass->numpoints)
				continue;	// points on negative side, not a seperating plane
				
			if (!counts[0])
				continue;	// planar with seperating plane
#else
			k = (j+1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;
			k = (j+pass->numpoints-1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;			
#endif
		//
		// flip the normal if we want the back side
		//
			if (flipclip)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}
			
		//
		// clip target by the seperating plane
		//
			target = ChopWinding (target, stack, &plane);
			if (!target)
				return NULL;		// target is not visible
		}
	}
	
	return target;
}



/*
==================
RecursiveLeafFlow

Flood fill through the leafs
If src_portal is NULL, this is the originating leaf
==================
*/
void RecursiveLeafFlow (int leafnum, threaddata_t *thread, pstack_t *prevstack)
{
	pstack_t	stack;
	portal_t	*p;
	plane_t		backplane;
	leaf_t 		*leaf;
	int			i, j;
	long		*test, *might, *vis, more;
	int			pnum;

	thread->c_chains++;

	leaf = &leafs[leafnum];
//	CheckStack (leaf, thread);

	prevstack->next = &stack;

	stack.next = NULL;
	stack.leaf = leaf;
	stack.portal = NULL;

	might = (long *)stack.mightsee;
	vis = (long *)thread->base->portalvis;
	
// check all portals for flowing into other leafs	
	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];
		pnum = p - portals;

		if ( ! (prevstack->mightsee[pnum >> 3] & (1<<(pnum&7)) ) )
		{
			continue;	// can't possibly see it
		}

	// if the portal can't see anything we haven't allready seen, skip it
		if (p->status == stat_done)
		{
			test = (long *)p->portalvis;
		}
		else
		{
			test = (long *)p->portalflood;
		}

		more = 0;
		for (j=0 ; j<portallongs ; j++)
		{
			might[j] = ((long *)prevstack->mightsee)[j] & test[j];
			more |= (might[j] & ~vis[j]);
		}
		
		if (!more && 
			(thread->base->portalvis[pnum>>3] & (1<<(pnum&7))) )
		{	// can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;
		VectorSubtract (vec3_origin, p->plane.normal, backplane.normal);
		backplane.dist = -p->plane.dist;
		
//		c_portalcheck++;
		
		stack.portal = p;
		stack.next = NULL;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;
		
#if 1
{
float d;

	d = DotProduct (p->origin, thread->pstack_head.portalplane.normal);
	d -= thread->pstack_head.portalplane.dist;
	if (d < -p->radius)
	{
		continue;
	}
	else if (d > p->radius)
	{
		stack.pass = p->winding;
	}
	else	
	{
		stack.pass = ChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
		if (!stack.pass)
			continue;
	}
}
#else
		stack.pass = ChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
		if (!stack.pass)
			continue;
#endif

	
#if 1
{
float d;

	d = DotProduct (thread->base->origin, p->plane.normal);
	d -= p->plane.dist;
	if (d > thread->base->radius)
//	if (d > p->radius)
	{
		continue;
	}
//	else if (d < -p->radius)
	else if (d < -thread->base->radius)
	{
		stack.source = prevstack->source;
	}
	else	
	{
		stack.source = ChopWinding (prevstack->source, &stack, &backplane);
		if (!stack.source)
			continue;
	}
}
#else
		stack.source = ChopWinding (prevstack->source, &stack, &backplane);
		if (!stack.source)
			continue;
#endif

		if (!prevstack->pass)
		{	// the second leaf can only be blocked if coplanar

			// mark the portal as visible
			thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

			RecursiveLeafFlow (p->leaf, thread, &stack);
			continue;
		}

		stack.pass = ClipToSeperators (stack.source, prevstack->pass, stack.pass, false, &stack);
		if (!stack.pass)
			continue;
		
		stack.pass = ClipToSeperators (prevstack->pass, stack.source, stack.pass, true, &stack);
		if (!stack.pass)
			continue;

		// mark the portal as visible
		thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

		// flow through it for real
		RecursiveLeafFlow (p->leaf, thread, &stack);
	}	
}


/*
===============
PortalFlow

generates the portalvis bit vector
===============
*/
void PortalFlow (int portalnum)
{
	threaddata_t	data;
	int				i;
	portal_t		*p;
	int				c_might, c_can;

	p = sorted_portals[portalnum];
	p->status = stat_working;

	c_might = CountBits (p->portalflood, numportals*2);

	memset (&data, 0, sizeof(data));
	data.base = p;
	
	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	for (i=0 ; i<portallongs ; i++)
		((long *)data.pstack_head.mightsee)[i] = ((long *)p->portalflood)[i];
	RecursiveLeafFlow (p->leaf, &data, &data.pstack_head);

	p->status = stat_done;

	c_can = CountBits (p->portalvis, numportals*2);

	qprintf ("portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n", 
		(int)(p - portals),	c_might, c_can, data.c_chains);
}


/*
===============================================================================

This is a rough first-order aproximation that is used to trivially reject some
of the final calculations.


Calculates portalfront and portalflood bit vectors

thinking about:

typedef struct passage_s
{
	struct passage_s	*next;
	struct portal_s		*to;
	stryct sep_s		*seperators;
	byte				*mightsee;
} passage_t;

typedef struct portal_s
{
	struct passage_s	*passages;
	int					leaf;		// leaf portal faces into
} portal_s;

leaf = portal->leaf
clear 
for all portals


calc portal visibility
	clear bit vector
	for all passages
		passage visibility


for a portal to be visible to a passage, it must be on the front of
all seperating planes, and both portals must be behind the mew portal

===============================================================================
*/

int		c_flood, c_vis;

char test_leaf[MAX_MAP_LEAFS];

/*
==================
SimpleFlood

==================
*/

int cullerror = 0;

void SimpleFlood (portal_t *srcportal, int leafnum)
{
	int		i;
	leaf_t	*leaf;
	portal_t	*p;
	int		pnum;

	if(cullerror && !test_leaf[leafnum])
		return;

	test_leaf[leafnum] = 0;

	leaf = &leafs[leafnum];

	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];

		pnum = p - portals;

		if ( ! (srcportal->portalfront[pnum>>3] & (1<<(pnum&7)) ) )
			continue;

		if (srcportal->portalflood[pnum>>3] & (1<<(pnum&7)) )
			continue;

		srcportal->portalflood[pnum>>3] |= (1<<(pnum&7));
		
		SimpleFlood (srcportal, p->leaf);
	}
}
/*
==============
BasePortalVis
==============
*/
void BasePortalVis (int portalnum)
{
	int			j, k;
	portal_t	*tp, *p;
	float		d;
	winding_t	*w;
	vec3_t prenormal, normal;
	float p_dot, tp_dot, p_rad, tp_rad;

	p = portals+portalnum;

	p->portalfront = malloc (portalbytes);
	memset (p->portalfront, 0, portalbytes);

	p->portalflood = malloc (portalbytes);
	memset (p->portalflood, 0, portalbytes);
	
	p->portalvis = malloc (portalbytes);
	memset (p->portalvis, 0, portalbytes);

	memset(test_leaf, 0, MAX_MAP_LEAFS);

	for (j=0, tp = portals ; j<numportals*2 ; j++, tp++)
	{
		if (j == portalnum)
			continue;
		else if(cullerror && tp->leaf == p->owner_leaf)
			continue;

		test_leaf[tp->leaf] = 1;

		w = tp->winding;

		for (k=0 ; k<w->numpoints ; k++)
		{
			d = DotProduct (w->points[k], p->plane.normal)
				- p->plane.dist;
			if (d > ON_EPSILON)
				break;
		}
		if (k == w->numpoints)
			continue;	// no points on front

		w = p->winding;
		for (k=0 ; k<w->numpoints ; k++)
		{
			d = DotProduct (w->points[k], tp->plane.normal)
				- tp->plane.dist;
			if (d < -ON_EPSILON)
				break;
		}

		if (k == w->numpoints)
			continue;	// no points on front (or in range if maxdist set)

		if(maxdist > 0.0)
		{
			// This approximation will consider 2 circles in 3d space with the centeer and radius of the
			// polygons on the planes of the polygons.

			prenormal[0] = p->origin[0] - tp->origin[0];
			prenormal[1] = p->origin[1] - tp->origin[1];
			prenormal[2] = p->origin[2] - tp->origin[2];

			d = VectorNormalize(prenormal, normal);

			p_dot = DotProduct(p->plane.normal, normal);

			if(p_dot < 0.0)
				p_rad = -p_dot * p->radius;
			else
				p_rad = p_dot * p->radius;

			tp_dot = DotProduct(tp->plane.normal, normal);

			if(tp_dot < 0.0)
				tp_rad = -tp_dot * tp->radius;
			else
				tp_rad = tp_dot * tp->radius;

			if(d > (maxdist + tp_rad + p_rad))
				continue;
		}

		p->portalfront[j>>3] |= (1<<(j&7));
	}

	SimpleFlood (p, p->leaf);

	p->nummightsee = CountBits (p->portalflood, numportals*2);
//	printf ("portal %i: %i mightsee\n", portalnum, p->nummightsee);
	c_flood += p->nummightsee;
}





/*
===============================================================================

This is a second order aproximation 

Calculates portalvis bit vector

WAAAAAAY too slow.

===============================================================================
*/

/*
==================
RecursiveLeafBitFlow

==================
*/
void RecursiveLeafBitFlow (int leafnum, byte *mightsee, byte *cansee)
{
	portal_t	*p;
	leaf_t 		*leaf;
	int			i, j;
	long		more;
	int			pnum;
	byte		newmight[MAX_PORTALS/8];

	leaf = &leafs[leafnum];
	
// check all portals for flowing into other leafs	
	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];
		pnum = p - portals;

		// if some previous portal can't see it, skip
		if (! (mightsee[pnum>>3] & (1<<(pnum&7)) ) )
			continue;

		// if this portal can see some portals we mightsee, recurse
		more = 0;
		for (j=0 ; j<portallongs ; j++)
		{
			((long *)newmight)[j] = ((long *)mightsee)[j] 
				& ((long *)p->portalflood)[j];
			more |= ((long *)newmight)[j] & ~((long *)cansee)[j];
		}

		if (!more)
			continue;	// can't see anything new

		cansee[pnum>>3] |= (1<<(pnum&7));

		RecursiveLeafBitFlow (p->leaf, newmight, cansee);
	}	
}

/*
==============
BetterPortalVis
==============
*/
void BetterPortalVis (int portalnum)
{
	portal_t	*p;

	p = portals+portalnum;

	RecursiveLeafBitFlow (p->leaf, p->portalflood, p->portalvis);

	// build leaf vis information
	p->nummightsee = CountBits (p->portalvis, numportals*2);
	c_vis += p->nummightsee;
}


