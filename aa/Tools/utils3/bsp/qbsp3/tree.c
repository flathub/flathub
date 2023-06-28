#include "qbsp.h"

extern	int	c_nodes;

void RemovePortalFromNode (portal_t *portal, node_t *l);

node_t *NodeForPoint (node_t *node, vec3_t origin)
{
	plane_t	*plane;
	vec_t	d;

	while (node->planenum != PLANENUM_LEAF)
	{
		plane = &mapplanes[node->planenum];
		d = DotProduct (origin, plane->normal) - plane->dist;
		if (d >= 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return node;
}



/*
=============
FreeTreePortals_r
=============
*/

void FreeTreePortals_r (node_t *node)
{
	portal_t	*p, *nextp;
	int			s;

	// free children
	if (node->planenum != PLANENUM_LEAF || node->pruned)
	{
		FreeTreePortals_r (node->children[0]);
		FreeTreePortals_r (node->children[1]);
	}

	// free portals
	for (p=node->portals ; p ; p=nextp)
	{
		s = (p->nodes[1] == node);
		nextp = p->next[s];

		RemovePortalFromNode (p, p->nodes[!s]);
		FreePortal (p);
	}
	node->portals = NULL;
}

/*
=============
FreeTree_r
=============
*/
void FreeTree_r (node_t *node)
{
	face_t		*f, *nextf;

	// free children
	if (node->planenum != PLANENUM_LEAF || node->pruned)
	{
		FreeTree_r (node->children[0]);
		FreeTree_r (node->children[1]);
	}

	// free bspbrushes

	FreeBrushList (node->brushlist);

	// free faces
	for (f=node->faces ; f ; f=nextf)
	{
		nextf = f->next;
		FreeFace (f);
	}

	// free the node
	if (node->volume)
		FreeBrush (node->volume);

	if (numthreads == 1)
		c_nodes--;
	free (node);
}


/*
=============
FreeTree
=============
*/

void FreeTree (tree_t *tree)
{
	FreeTreePortals_r (tree->headnode);
	FreeTree_r (tree->headnode);
	free (tree);
}

//===============================================================

void PrintTree_r (node_t *node, int depth)
{
	int		i;
	plane_t	*plane;
	bspbrush_t	*bb;

	for (i=0 ; i<depth ; i++)
		printf ("  ");
	if (node->planenum == PLANENUM_LEAF)
	{
		if (!node->brushlist)
			printf ("NULL\n");
		else
		{
			for (bb=node->brushlist ; bb ; bb=bb->next)
				printf ("%i ", bb->original->brushnum);
			printf ("\n");
		}
		return;
	}

	plane = &mapplanes[node->planenum];
	printf ("#%i (%5.2f %5.2f %5.2f):%5.2f\n", node->planenum,
		plane->normal[0], plane->normal[1], plane->normal[2],
		plane->dist);
	PrintTree_r (node->children[0], depth+1);
	PrintTree_r (node->children[1], depth+1);
}

/*
=========================================================

NODES THAT DON'T SEPERATE DIFFERENT CONTENTS CAN BE PRUNED

=========================================================
*/

int	c_pruned;

/*
============
PruneNodes_r
============
*/
void PruneNodes_r (node_t *node)
{
	bspbrush_t		*b, *next;
//	portal_t	*p, *nextp;
//	int			s;

	if (node->planenum == PLANENUM_LEAF)
		return;
	PruneNodes_r (node->children[0]);
	PruneNodes_r (node->children[1]);

	if ( (node->children[0]->contents & CONTENTS_SOLID)
	&& (node->children[1]->contents & CONTENTS_SOLID) )
	{
		if (node->faces)
			Error ("node->faces seperating CONTENTS_SOLID");
		if (node->children[0]->faces || node->children[1]->faces)
			Error ("!node->faces with children");

		// FIXME: free stuff

		node->pruned = true;
		node->planenum = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;
		node->detail_seperator = false;

		if (node->brushlist)
			Error ("PruneNodes: node->brushlist");

		// combine brush lists
		node->brushlist = node->children[1]->brushlist;

		for (b=node->children[0]->brushlist ; b ; b=next)
		{
			next = b->next;
			b->next = node->brushlist;
			node->brushlist = b;
		}

		node->children[0]->brushlist = NULL;
		node->children[1]->brushlist = NULL;

		if(node->children[0]->volume != NULL)
		{
			FreeBrush(node->children[0]->volume);
			node->children[0]->volume = NULL;
		}

		if(node->children[1]->volume != NULL)
		{
			FreeBrush(node->children[1]->volume);
			node->children[1]->volume = NULL;
		}

		c_pruned++;
	}
}


void PruneNodes (node_t *node)
{
	qprintf ("--- PruneNodes ---\n");
	c_pruned = 0;
	PruneNodes_r (node);
	qprintf ("%5i pruned nodes\n", c_pruned);
}

//===========================================================
