// vis.c

#include "vis.h"
#include "threads.h"
#include "stdlib.h"
#include "parsecfg.h"


int			numportals;
int			portalclusters;

char		inbase[32];
char		outbase[32];

portal_t	*portals;
leaf_t		*leafs;

int			c_portaltest, c_portalpass, c_portalcheck;

byte		*uncompressedvis;

byte	*vismap, *vismap_p, *vismap_end;	// past visfile
int		originalvismapsize;

int		leafbytes;				// (portalclusters+63)>>3
int		leaflongs;

int		portalbytes, portallongs;

qboolean		fastvis;
qboolean		nosort;

int			testlevel = 2;
float		maxdist = 0.0f;

int		totalvis;

portal_t	*sorted_portals[MAX_MAP_PORTALS*2];


//=============================================================================

void PlaneFromWinding (winding_t *w, plane_t *plane)
{
	vec3_t		v1, v2;

// calc plane
	VectorSubtract (w->points[2], w->points[1], v1);
	VectorSubtract (w->points[0], w->points[1], v2);
	CrossProduct (v2, v1, plane->normal);
	VectorNormalize (plane->normal, plane->normal);
	plane->dist = DotProduct (w->points[0], plane->normal);
}


/*
==================
NewWinding
==================
*/
winding_t *NewWinding (int points)
{
	winding_t	*w;
	int			size;

	if (points > MAX_POINTS_ON_WINDING)
		Error ("NewWinding: %i points", points);

	size = (int)((winding_t *)0)->points[points];
	w = malloc (size);
	memset (w, 0, size);

	return w;
}



void pw(winding_t *w)
{
	int		i;
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.1f, %5.1f, %5.1f)\n",w->points[i][0], w->points[i][1],w->points[i][2]);
}

void prl(leaf_t *l)
{
	int			i;
	portal_t	*p;
	plane_t		pl;

	for (i=0 ; i<l->numportals ; i++)
	{
		p = l->portals[i];
		pl = p->plane;
		printf ("portal %4i to leaf %4i : %7.1f : (%4.1f, %4.1f, %4.1f)\n",(int)(p-portals),p->leaf,pl.dist, pl.normal[0], pl.normal[1], pl.normal[2]);
	}
}


//=============================================================================

/*
=============
SortPortals

Sorts the portals from the least complex, so the later ones can reuse
the earlier information.
=============
*/
int PComp (const void *a, const void *b)
{
	if ( (*(portal_t **)a)->nummightsee == (*(portal_t **)b)->nummightsee)
		return 0;
	if ( (*(portal_t **)a)->nummightsee < (*(portal_t **)b)->nummightsee)
		return -1;
	return 1;
}

void SortPortals (void)
{
	int		i;

	for (i=0 ; i<numportals*2 ; i++)
		sorted_portals[i] = &portals[i];

	if (nosort)
		return;
	qsort (sorted_portals, numportals*2, sizeof(sorted_portals[0]), PComp);
}


/*
==============
LeafVectorFromPortalVector
==============
*/
int LeafVectorFromPortalVector (byte *portalbits, byte *leafbits)
{
	int			i;
	portal_t	*p;
	int			c_leafs;


	memset (leafbits, 0, leafbytes);

	for (i=0 ; i<numportals*2 ; i++)
	{
		if (portalbits[i>>3] & (1<<(i&7)) )
		{
			p = portals+i;
			leafbits[p->leaf>>3] |= (1<<(p->leaf&7));
		}
	}

	c_leafs = CountBits (leafbits, portalclusters);

	return c_leafs;
}


/*
===============
ClusterMerge

Merges the portal visibility for a leaf
===============
*/
void ClusterMerge (int leafnum)
{
	leaf_t		*leaf;
	byte		portalvector[MAX_PORTALS/8];
	byte		uncompressed[MAX_MAP_LEAFS/8];
	byte		compressed[MAX_MAP_LEAFS/8];
	int			i, j;
	int			numvis;
	byte		*dest;
	portal_t	*p;
	int			pnum;

	// OR together all the portalvis bits

	memset (portalvector, 0, portalbytes);
	leaf = &leafs[leafnum];
	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];
		if (p->status != stat_done)
			Error ("portal not done");
		for (j=0 ; j<portallongs ; j++)
			((long *)portalvector)[j] |= ((long *)p->portalvis)[j];
		pnum = p - portals;
		portalvector[pnum>>3] |= 1<<(pnum&7);
	}

	// convert portal bits to leaf bits
	numvis = LeafVectorFromPortalVector (portalvector, uncompressed);

	if (uncompressed[leafnum>>3] & (1<<(leafnum&7)))
		printf ("WARNING: Leaf portals saw into leaf (%d)\n", leafnum);

	uncompressed[leafnum>>3] |= (1<<(leafnum&7));
	numvis++;		// count the leaf itself

	// save uncompressed for PHS calculation
	memcpy (uncompressedvis + leafnum*leafbytes, uncompressed, leafbytes);

//
// compress the bit string
//
	qprintf ("cluster %4i : %4i visible\n", leafnum, numvis);
	totalvis += numvis;

	i = CompressVis (uncompressed, compressed);

	dest = vismap_p;
	vismap_p += i;

	if (vismap_p > vismap_end)
		Error ("Vismap expansion overflow");

	dvis->bitofs[leafnum][DVIS_PVS] = dest-vismap;

	memcpy (dest, compressed, i);
}


/*
==================
CalcPortalVis
==================
*/
void CalcPortalVis (void)
{
	int		i;

// fastvis just uses mightsee for a very loose bound
	if (fastvis)
	{
		for (i=0 ; i<numportals*2 ; i++)
		{
			portals[i].portalvis = portals[i].portalflood;
			portals[i].status = stat_done;
		}
		return;
	}

	RunThreadsOnIndividual (numportals*2, true, PortalFlow);

}


/*
==================
CalcVis
==================
*/
void CalcVis (void)
{
	int		i;

	RunThreadsOnIndividual (numportals*2, true, BasePortalVis);

//	RunThreadsOnIndividual (numportals*2, true, BetterPortalVis);

	SortPortals ();

	CalcPortalVis ();

//
// assemble the leaf vis lists by oring and compressing the portal lists
//
	for (i=0 ; i<portalclusters ; i++)
		ClusterMerge (i);

	printf ("Average clusters visible: %i\n", totalvis / portalclusters);
}


void SetPortalSphere (portal_t *p)
{
	int		i;
	vec3_t	total, dist;
	winding_t	*w;
	float	r, bestr;

	w = p->winding;
	VectorCopy (vec3_origin, total);
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorAdd (total, w->points[i], total);
	}

	for (i=0 ; i<3 ; i++)
		total[i] /= w->numpoints;

	bestr = 0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorSubtract (w->points[i], total, dist);
		r = VectorLength (dist);
		if (r > bestr)
			bestr = r;
	}
	VectorCopy (total, p->origin);
	p->radius = bestr;
}

/*
============
LoadPortals
============
*/
static int rslt;
void LoadPortals (char *name)
{
	int			i, j;
	portal_t	*p;
	leaf_t		*l;
	char		magic[80];
	FILE		*f;
	int			numpoints;
	winding_t	*w;
	int			leafnums[2];
	plane_t		plane;

	if (!strcmp(name,"-"))
		f = stdin;
	else
	{
		f = fopen(name, "r");
		if (!f)
			Error ("LoadPortals: couldn't read %s\n",name);
	}

	if (fscanf (f,"%79s\n%i\n%i\n",magic, &portalclusters, &numportals) != 3)
		Error ("LoadPortals: failed to read header");
	if (strcmp(magic,PORTALFILE))
		Error ("LoadPortals: not a portal file");

	printf ("%4i portalclusters\n", portalclusters);
	printf ("%4i numportals\n", numportals);

	// these counts should take advantage of 64 bit systems automatically
	leafbytes = ((portalclusters+63)&~63)>>3;
	leaflongs = leafbytes/sizeof(long);

	portalbytes = ((numportals*2+63)&~63)>>3;
	portallongs = portalbytes/sizeof(long);

// each file portal is split into two memory portals
	portals = malloc(2*numportals*sizeof(portal_t));
	memset (portals, 0, 2*numportals*sizeof(portal_t));

	leafs = malloc(portalclusters*sizeof(leaf_t));
	memset (leafs, 0, portalclusters*sizeof(leaf_t));

	originalvismapsize = portalclusters*leafbytes;
	uncompressedvis = malloc(originalvismapsize);

	vismap = vismap_p = dvisdata;
	dvis->numclusters = portalclusters;
	vismap_p = (byte *)&dvis->bitofs[portalclusters];

	vismap_end = vismap + MAX_MAP_VISIBILITY;

	for (i=0, p=portals ; i<numportals ; i++)
	{
		if (fscanf (f, "%i %i %i ", &numpoints, &leafnums[0], &leafnums[1])
			!= 3)
			Error ("LoadPortals: reading portal %i", i);
		if (numpoints > MAX_POINTS_ON_WINDING)
			Error ("LoadPortals: portal %i has too many points", i);
		if ( (unsigned)leafnums[0] > portalclusters
		|| (unsigned)leafnums[1] > portalclusters)
			Error ("LoadPortals: reading portal %i", i);

		w = p->winding = NewWinding (numpoints);
		w->original = true;
		w->numpoints = numpoints;

		for (j=0 ; j<numpoints ; j++)
		{
			double	v[3];
			int		k;

			// scanf into double, then assign to vec_t
			// so we don't care what size vec_t is
			if (fscanf (f, "(%lf %lf %lf ) "
			, &v[0], &v[1], &v[2]) != 3)
				Error ("LoadPortals: reading portal %i", i);
			for (k=0 ; k<3 ; k++)
				w->points[j][k] = v[k];
		}
		rslt = fscanf (f, "\n");

	// calc plane
		PlaneFromWinding (w, &plane);

	// create forward portal
		l = &leafs[leafnums[0]];
		if (l->numportals == MAX_PORTALS_ON_LEAF)
			Error ("Leaf with too many portals");
		l->portals[l->numportals] = p;
		l->numportals++;

		p->winding = w;
		VectorSubtract (vec3_origin, plane.normal, p->plane.normal);
		p->plane.dist = -plane.dist;
		p->leaf = leafnums[1];
		p->owner_leaf = leafnums[0];
		SetPortalSphere (p);
		p++;

	// create backwards portal
		l = &leafs[leafnums[1]];
		if (l->numportals == MAX_PORTALS_ON_LEAF)
			Error ("Leaf with too many portals");
		l->portals[l->numportals] = p;
		l->numportals++;

		p->winding = NewWinding(w->numpoints);
		p->winding->numpoints = w->numpoints;
		for (j=0 ; j<w->numpoints ; j++)
		{
			VectorCopy (w->points[w->numpoints-1-j], p->winding->points[j]);
		}

		p->plane = plane;
		p->leaf = leafnums[0];
		p->owner_leaf = leafnums[1];
		SetPortalSphere (p);
		p++;

	}

	fclose (f);
}


/*
================
CalcPHS

Calculate the PHS (Potentially Hearable Set)
by ORing together all the PVS visible from a leaf
================
*/
void CalcPHS (void)
{
	int		i, j, k, l, index;
	int		bitbyte;
	long	*dest, *src;
	byte	*scan;
	int		count;
	byte	uncompressed[MAX_MAP_LEAFS/8];
	byte	compressed[MAX_MAP_LEAFS/8];

	printf ("Building PHS...\n");

	count = 0;
	for (i=0 ; i<portalclusters ; i++)
	{
		scan = uncompressedvis + i*leafbytes;
		memcpy (uncompressed, scan, leafbytes);
		for (j=0 ; j<leafbytes ; j++)
		{
			bitbyte = scan[j];
			if (!bitbyte)
				continue;
			for (k=0 ; k<8 ; k++)
			{
				if (! (bitbyte & (1<<k)) )
					continue;
				// OR this pvs row into the phs
				index = ((j<<3)+k);
				if (index >= portalclusters)
					Error ("Bad bit in PVS");	// pad bits should be 0
				src = (long *)(uncompressedvis + index*leafbytes);
				dest = (long *)uncompressed;
				for (l=0 ; l<leaflongs ; l++)
					((long *)uncompressed)[l] |= src[l];
			}
		}
		for (j=0 ; j<portalclusters ; j++)
			if (uncompressed[j>>3] & (1<<(j&7)) )
				count++;

	//
	// compress the bit string
	//
		j = CompressVis (uncompressed, compressed);

		dest = (long *)vismap_p;
		vismap_p += j;

		if (vismap_p > vismap_end)
			Error ("Vismap expansion overflow");

		dvis->bitofs[i][DVIS_PHS] = (byte *)dest-vismap;

		memcpy (dest, compressed, j);
	}

	printf ("Average clusters hearable: %i\n", count/portalclusters);
}

/*
===========
main
===========
*/
int main (int argc, char **argv)
{
	char	portalfile[1024];
	char		source[1024];
	char		name[1024];
    char *param, *param2=NULL;
	double		start, end;

	printf ("--- Alien Arena QVIS3 ---\n");

    LoadConfigurationFile("qvis3", 0);
    LoadConfiguration(argc-1, argv+1);

	verbose = false;

    while((param = WalkConfiguration()) != NULL)
	{
		if (!strcmp(param,"-threads"))
		{
            param2 = WalkConfiguration();
			numthreads = atoi (param2);
		}
		else if (!strcmp(param, "-fast"))
		{
			printf ("fastvis = true\n");
			fastvis = true;
		}
		else if (!strcmp(param, "-level"))
		{
            param2 = WalkConfiguration();
			testlevel = atoi (param2);
			printf ("Test level not used in qvis3\n");
		}
		else if (!strcmp(param, "-maxdist"))
		{
            param2 = WalkConfiguration();
			maxdist = atof (param2);

			printf ("maxdist = %.2f\n", maxdist);
		}
		else if (!strcmp(param, "-maxfovdist"))
		{
			float fov, tmpmaxdist;

            param2 = WalkConfiguration();
			fov = atof (param2);
            param2 = WalkConfiguration();
			tmpmaxdist = atof (param2);

			maxdist = tmpmaxdist / (float)cos(atan(tan(fov * Q_PI / 360.0) * 5.0 / 4.0));

			printf ("maxfovdist = %.2f %.2f -> %2f\n", fov, tmpmaxdist, maxdist);
		}
		else if (!strcmp(param, "-v"))
		{
			printf ("verbose = true\n");
			verbose = true;
		}
		else if (!strcmp (param,"-nosort"))
		{
			printf ("nosort = true\n");
			nosort = true;
		}
		else if (!strcmp (param,"-cullerror"))
		{
			printf ("cullerror = true\n");
			cullerror = true;
		}
		else if (!strcmp (param,"-tmpin"))
			strcpy (inbase, "/tmp");
		else if (!strcmp (param,"-tmpout"))
			strcpy (outbase, "/tmp");
		else if (param[0] == '+')
            LoadConfigurationFile(param+1, 1);
		else if (param[0] == '-')
			Error ("Unknown option \"%s\"", param);
		else
			break;
	}

    if(param != NULL)
        param2 = WalkConfiguration();

    if (param == NULL || param2 != NULL)
		Error ("usage: vis [-threads #] [-fast] [-nosort] [-cullerror] [-v] [-maxdist distance] [-maxfovdist fov distance] bspfile");

    while(param2)  // make sure list is clean
        param2 = WalkConfiguration();

	start = I_FloatTime ();

	ThreadSetDefault ();

//    if(!IsFullPath(param))
//	    SetQdirFromPath (param);

	strcpy (source, ExpandArg(param));
	StripExtension (source);
	DefaultExtension (source, ".bsp");

	sprintf (name, "%s%s", inbase, source);
	printf ("reading %s\n", name);
	LoadBSPFile (name);
	if (numnodes == 0 || numfaces == 0)
		Error ("Empty map");

	sprintf (portalfile, "%s%s", inbase, ExpandArg(param));
	StripExtension (portalfile);
	strcat (portalfile, ".prt");

	printf ("reading %s\n", portalfile);
	LoadPortals (portalfile);

	CalcVis ();
	
	CalcPHS ();

	visdatasize = vismap_p - dvisdata;
	printf ("visdatasize:%i  compressed from %i\n", visdatasize, originalvismapsize*2);

	sprintf (name, "%s%s", outbase, source);
	printf ("writing %s\n", name);
	WriteBSPFile (name);

	end = I_FloatTime ();
	printf ("%5.1f seconds elapsed\n", end-start);

	return 0;
}

