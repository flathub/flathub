// qrad.c

#include "qrad.h"
#include "parsecfg.h"

/*

NOTES
-----

every surface must be divided into at least two patches each axis

*/

patch_t		*face_patches[MAX_MAP_FACES];
entity_t	*face_entity[MAX_MAP_FACES];
patch_t		patches[MAX_PATCHES];
unsigned	num_patches;

vec3_t		radiosity[MAX_PATCHES];		// light leaving a patch
vec3_t		illumination[MAX_PATCHES];	// light arriving at a patch

vec3_t		face_offset[MAX_MAP_FACES];		// for rotating bmodels
dplane_t	backplanes[MAX_MAP_PLANES];

char		inbase[32], outbase[32];

int			fakeplanes;					// created planes for origin offset

int		numbounce = 4; // 2010-09 default from 8 to 4
qboolean	extrasamples;

int noblock = false; // when true, disables occlusion testing on light rays (?)

int memory = false;

float patch_cutoff = 0.0f; // set with -radmin 0.0..1.0, see MakeTransfers()

float	subdiv = 64;
qboolean	dumppatches;

void BuildLightmaps (void);
int TestLine (vec3_t start, vec3_t stop);

int		junk;

/*
 * 2010-09 Notes
 * These variables are somewhat confusing. The floating point color values are
 *  in the range 0..255 (floating point color should be 0.0 .. 1.0 IMO.)
 *  The following may or may not be precisely correct.
 *  (There are other variables, surface flags, etc., affecting lighting. What
 *   they do, or whether they work at all is "to be determined")
 *
 * see lightmap.c:FinalLightFace()
 *  sequence: ambient is added, lightscale is applied, RGB is "normalized" to
 *  0..255 range, grayscale is applied, RGB is clamped to maxlight.
 *
 * ambient:
 *  set with -ambient option, 0..255 (but only small numbers are useful)
 *  adds the same value to R, G & B
 *  default is 0
 *
 * lightscale:
 *  set with -scale option, 0.0..1.0
 *  scales lightmap globally.
 *
 * grayscale:
 *  set with -grayscale .or. -greyscale option option, 0.0..1.0
 *  proportionally grayscales the lightmap, 1.0 for completely grayscaled
 *
 * desaturate:
 *  set with -desaturate, 0.0..1.0
 *  proportionally desaturate texture reflectivity
 *
 * direct_scale:
 *  set with -direct option, 0.0..1.0
 *  controls reflection from emissive surfaces, i.e. brushes with a light value
 *  research indicates it is not the usual practice to include this in
 *    radiosity, so the default should be 0.0. (Would be nice to have this
 *    a per-surface option where surfaces are used like point lights.)
 *
 * entity_scale:
 *  set with -entity option, 0.0..1.0
 *  controls point light radiosity, i.e. light entities.
 *  default is 1.0, no attenuation of point lights
 *
 *
 */
 #include <assert.h>
float ambient = 0.0f;
float lightscale = 1.0f;
float maxlight = 255.0f;
// qboolean nocolor = false;
float grayscale = 0.0f;
float desaturate = 0.0f;
float direct_scale = 0.0f;
float entity_scale = 1.0f;

/*
 * 2010-09 Notes:
 * These are controlled by setting keys in the worldspawn entity. A light
 * entity targeting an info_null entity is used to determine the vector for
 * the sun directional lighting; variable name "sun_pos" is a misnomer, i think.
 * Example:
 * "_sun" "sun_target"  # activates sun
 * "_sun_color" "1.0 1.0 0.0"  # for yellow, sets sun_alt_color true
 * "_sun_light" "50" # light value, variable is sun_main
 * "_sun_ambient" "2" # an ambient light value in the sun color. variable is sun_ambient
 *
 * It might or might not be the case:
 *  if there is no info_null for the light entity with the "sun_target" to
 *  target, then {0,0,0} is used for the target. If _sun_color is not specified
 *  in the .map, the color of the light entity is used.
 */
qboolean sun = false;
qboolean sun_alt_color = false;
vec3_t sun_pos = {0.0f, 0.0f, 1.0f};
float sun_main = 250.0f;
float sun_ambient = 0.0f;
vec3_t sun_color = {1, 1, 1};

qboolean	glview;
qboolean	nopvs;
qboolean	save_trace = false;

char		source[1024];


/*
===================================================================

MISC

===================================================================
*/


/*
=============
MakeBackplanes
=============
*/
void MakeBackplanes (void)
{
	int		i;

	for (i=0 ; i<numplanes ; i++)
	{
		backplanes[i].dist = -dplanes[i].dist;
		VectorSubtract (vec3_origin, dplanes[i].normal, backplanes[i].normal);
	}
}

int		leafparents[MAX_MAP_LEAFS];
int		nodeparents[MAX_MAP_NODES];

/*
=============
MakeParents
=============
*/
void MakeParents (int nodenum, int parent)
{
	int		i, j;
	dnode_t	*node;

	nodeparents[nodenum] = parent;
	node = &dnodes[nodenum];

	for (i=0 ; i<2 ; i++)
	{
		j = node->children[i];
		if (j < 0)
			leafparents[-j - 1] = nodenum;
		else
			MakeParents (j, nodenum);
	}
}


/*
===================================================================

TRANSFER SCALES

===================================================================
*/

int	PointInLeafnum (vec3_t point)
{
	int		nodenum;
	vec_t	dist;
	dnode_t	*node;
	dplane_t	*plane;

	nodenum = 0;
	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planenum];
		dist = DotProduct (point, plane->normal) - plane->dist;
		if (dist > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}

	return -nodenum - 1;
}

dleaf_t		*PointInLeaf (vec3_t point)
{
	int		num;

	num = PointInLeafnum (point);
	return &dleafs[num];
}


qboolean PvsForOrigin (vec3_t org, byte *pvs)
{
	dleaf_t	*leaf;

	if (!visdatasize)
	{
		memset (pvs, 255, (numleafs+7)/8 );
		return true;
	}

	leaf = PointInLeaf (org);
	if (leaf->cluster == -1)
		return false;		// in solid leaf

	DecompressVis (dvisdata + dvis->bitofs[leaf->cluster][DVIS_PVS], pvs);
	return true;
}


typedef struct tnode_s
{
	int		type;
	vec3_t	normal;
	float	dist;
	int		children[2];
	int		pad;
} tnode_t;

extern tnode_t		*tnodes;

int	total_transfer;

static long total_mem;

static int first_transfer = 1;

#define MAX_TRACE_BUF ((MAX_PATCHES + 7) / 8)

#define TRACE_BYTE(x) (((x)+7) >> 3)
#define TRACE_BIT(x) ((x) & 0x1F)

static byte trace_buf[MAX_TRACE_BUF + 1];
static byte trace_tmp[MAX_TRACE_BUF + 1];
static int trace_buf_size;

int CompressBytes (int size, byte *source, byte *dest)
{
	int		j;
	int		rep;
	byte	*dest_p;

	dest_p = dest + 1;

	for (j=0 ; j<size ; j++)
	{
		*dest_p++ = source[j];

        if ((dest_p - dest - 1) >= size)
		{
            memcpy(dest+1, source, size);
            dest[0] = 0;
            return size + 1;
		}

		if (source[j])
			continue;

		rep = 1;
		for ( j++; j<size ; j++)
			if (source[j] || rep == 255)
				break;
			else
				rep++;
		*dest_p++ = rep;

        if ((dest_p - dest - 1) >= size)
		{
            memcpy(dest+1, source, size);
            dest[0] = 0;
            return size + 1;
		}

		j--;

	}

    dest[0] = 1;
	return dest_p - dest;
}


void DecompressBytes (int size, byte *in, byte *decompressed)
{
	int		c;
	byte	*out;

    if (in[0] == 0) // not compressed
	{
        memcpy(decompressed, in + 1, size);
        return;
	}

	out = decompressed;
    in++;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		if (!c)
			Error ("DecompressBytes: 0 repeat");
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < size);
}

static int trace_bytes = 0;

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

void MakeTransfers (int i)
{
	int			j;
	vec3_t		delta;
	vec_t		dist, inv_dist, scale;
	float		trans;
	int			itrans;
	patch_t		*patch, *patch2;
	float		total, inv_total;
	dplane_t	plane;
	vec3_t		origin;
	float		transfers[MAX_PATCHES], *all_transfers;
	int			s;
	int			itotal;
	byte		pvs[(MAX_MAP_LEAFS+7)/8];
	int			cluster;
    int			calc_trace, test_trace;

    patch = patches + i;
	total = 0;

	VectorCopy (patch->origin, origin);
	plane = *patch->plane;

	if (!PvsForOrigin (patch->origin, pvs))
		return;

	// find out which patch2s will collect light
	// from patch

	all_transfers = transfers;
	patch->numtransfers = 0;

    calc_trace = (save_trace && memory && first_transfer);
    test_trace = (save_trace && memory && !first_transfer);

    if (calc_trace)
	{
        memset(trace_buf, 0, trace_buf_size);
	}
    else if(test_trace)
	{
        DecompressBytes(trace_buf_size, patch->trace_hit, trace_buf);
	}

	for (j=0, patch2 = patches ; j< num_patches ; j++, patch2++)
	{
    	transfers[j] = 0;

		if (j == i)
			continue;

		// check pvs bit
		if (!nopvs)
		{
			cluster = patch2->cluster;
			if (cluster == -1)
				continue;
			if ( ! ( pvs[cluster>>3] & (1<<(cluster&7)) ) )
				continue;		// not in pvs
		}

        if(test_trace && !(trace_buf[TRACE_BYTE(j)] & TRACE_BIT(j)))
            continue;

		// calculate vector
		VectorSubtract (patch2->origin, origin, delta);
		//dist = VectorNormalize (delta, delta);

        // Not calling normalize function to save function call overhead
        dist = delta[0]*delta[0] + delta[1]*delta[1] + delta[2]*delta[2];

        if (dist == 0)
		{
            continue;
		}
        else
		{
			dist = sqrt ( dist );
            inv_dist = 1.0f / dist;
            delta[0] *= inv_dist;
            delta[1] *= inv_dist;
            delta[2] *= inv_dist;
		}

		// relative angles
		scale = DotProduct (delta, plane.normal);
		scale *= -DotProduct (delta, patch2->plane->normal);
		if (scale <= 0)
			continue;

		// check exact transfer
		trans = scale * patch2->area * inv_dist * inv_dist;

		if (trans > patch_cutoff)
		{
            if (!test_trace && !noblock && 
            	patch2->nodenum != patch->nodenum && 
            	TestLine_r (lowestCommonNode (patch->nodenum, patch2->nodenum), 
            				patch->origin, patch2->origin))
			{
                transfers[j] = 0;
				continue;
			}

    		transfers[j] = trans;

			total += trans;
			patch->numtransfers++;

    	}
	}

    // copy the transfers out and normalize
	// total should be somewhere near PI if everything went right
	// because partial occlusion isn't accounted for, and nearby
	// patches have underestimated form factors, it will usually
	// be higher than PI
	if (patch->numtransfers)
	{
		transfer_t	*t;

        if (patch->numtransfers < 0 || patch->numtransfers > MAX_PATCHES)
			Error ("Weird numtransfers");
		s = patch->numtransfers * sizeof(transfer_t);
		patch->transfers = malloc (s);

        total_mem += s;

		if (!patch->transfers)
			Error ("Memory allocation failure");

		//
		// normalize all transfers so all of the light
		// is transfered to the surroundings
		//
		t = patch->transfers;
		itotal = 0;

        inv_total = 65536.0f / total;

		for (j=0 ; j < num_patches; j++)
		{
			if (transfers[j] <= 0)
				continue;

			itrans = transfers[j]*inv_total;
			itotal += itrans;
			t->transfer = itrans;
			t->patch = j;
			t++;

            if (calc_trace)
			{
                trace_buf[TRACE_BYTE(j)] |= TRACE_BIT(j);
			}
		}
	}

    if (calc_trace)
	{
        j = CompressBytes(trace_buf_size, trace_buf, trace_tmp);
        patch->trace_hit = malloc(j);
        memcpy(patch->trace_hit, trace_tmp, j);

        trace_bytes += j;
	}

	// don't bother locking around this.  not that important.
	total_transfer += patch->numtransfers;
}




/*
=============
FreeTransfers
=============
*/
void FreeTransfers (void)
{
	int		i;

	for (i=0 ; i<num_patches ; i++)
	{
        if(!memory)
		{
			free (patches[i].transfers);
			patches[i].transfers = NULL;
		}
        else if(patches[i].trace_hit != NULL)
		{
			free (patches[i].trace_hit);
			patches[i].trace_hit = NULL;
		}
	}
}


//===================================================================

/*
=============
WriteWorld
=============
*/
void WriteWorld (char *name)
{
	int		i, j;
	FILE		*out;
	patch_t		*patch;
	winding_t	*w;

	out = fopen (name, "w");
	if (!out)
		Error ("Couldn't open %s", name);

	for (j=0, patch=patches ; j<num_patches ; j++, patch++)
	{
		w = patch->winding;
		fprintf (out, "%i\n", w->numpoints);
		for (i=0 ; i<w->numpoints ; i++)
		{
			fprintf (out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n",
				w->p[i][0],
				w->p[i][1],
				w->p[i][2],
				patch->totallight[0],
				patch->totallight[1],
				patch->totallight[2]);
		}
		fprintf (out, "\n");
	}

	fclose (out);
}

/*
=============
WriteGlView
=============
*/
void WriteGlView (void)
{
	char	name[1024];
	FILE	*f;
	int		i, j;
	patch_t	*p;
	winding_t	*w;

	strcpy (name, source);
	StripExtension (name);
	strcat (name, ".glr");

	f = fopen (name, "w");
	if (!f)
		Error ("Couldn't open %s", f);

	for (j=0 ; j<num_patches ; j++)
	{
		p = &patches[j];
		w = p->winding;
		fprintf (f, "%i\n", w->numpoints);
		for (i=0 ; i<w->numpoints ; i++)
		{
			fprintf (f, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n",
				w->p[i][0],
				w->p[i][1],
				w->p[i][2],
				p->totallight[0]/128,
				p->totallight[1]/128,
				p->totallight[2]/128);
		}
		fprintf (f, "\n");
	}

	fclose (f);
}


//==============================================================

/*
=============
CollectLight
=============
*/
float CollectLight (void)
{
	int		i, j;
	patch_t	*patch;
	vec_t	total;

	total = 0;

	for (i=0, patch=patches ; i<num_patches ; i++, patch++)
	{
		// skys never collect light, it is just dropped
		if (patch->sky)
		{
			VectorClear (radiosity[i]);
			VectorClear (illumination[i]);
			continue;
		}

		for (j=0 ; j<3 ; j++)
		{
			patch->totallight[j] += illumination[i][j] / patch->area;
			radiosity[i][j] = illumination[i][j] * patch->reflectivity[j];
		}

		total += radiosity[i][0] + radiosity[i][1] + radiosity[i][2];
		VectorClear (illumination[i]);
	}

	return total;
}


/*
=============
ShootLight

Send light out to other patches
  Run multi-threaded
=============
*/
int c_progress;
int p_progress;

void ShootLight (int patchnum)
{
	int			k, l;
	transfer_t	*trans;
	int			num;
	patch_t		*patch;
	vec3_t		send;

	// this is the amount of light we are distributing
	// prescale it so that multiplying by the 16 bit
	// transfer values gives a proper output value
	for (k=0 ; k<3 ; k++)
		send[k] = radiosity[patchnum][k] / 0x10000;
	patch = &patches[patchnum];

    if(memory)
	{
        c_progress = 10 * patchnum / num_patches;

        if(c_progress != p_progress)
		{
			printf ("%i...", c_progress);
            p_progress = c_progress;
		}

        MakeTransfers(patchnum);
	}

	trans = patch->transfers;
	num = patch->numtransfers;

    for (k=0 ; k<num ; k++, trans++)
	{
		for (l=0 ; l<3 ; l++)
			illumination[trans->patch][l] += send[l]*trans->transfer;
	}

    if(memory)
	{
		free (patches[patchnum].transfers);
		patches[patchnum].transfers = NULL;
	}
}



/*
=============
BounceLight
=============
*/
void BounceLight (void)
{
	int		i, j, start=0, stop;
	float	added;
	char	name[64];
	patch_t	*p;

	for (i=0 ; i<num_patches ; i++)
	{
		p = &patches[i];
		for (j=0 ; j<3 ; j++)
		{
//			p->totallight[j] = p->samplelight[j];
			radiosity[i][j] = p->samplelight[j] * p->reflectivity[j] * p->area;
		}
	}

    if (memory)
        trace_buf_size = (num_patches + 7) / 8;

	for (i=0 ; i<numbounce ; i++)
	{
        if(memory)
		{
            p_progress = -1;
            start = I_FloatTime();
            printf("[%d remaining]  ", numbounce - i);
            total_mem = 0;
		}

		RunThreadsOnIndividual (num_patches, false, ShootLight);

        first_transfer = 0;

        if(memory)
		{
            stop = I_FloatTime();
			printf (" (%i)\n", stop-start);
		}

		added = CollectLight ();

		qprintf ("bounce:%i added:%f\n", i, added);
		if ( dumppatches && (i==0 || i == numbounce-1) )
		{
			sprintf (name, "bounce%i.txt", i);
			WriteWorld (name);
		}
	}
}



//==============================================================

void CheckPatches (void)
{
    int i;
	patch_t	*patch;

	for (i=0 ; i<num_patches ; i++)
	{
		patch = &patches[i];
		if (patch->totallight[0] < 0 || patch->totallight[1] < 0 || patch->totallight[2] < 0)
			Error ("negative patch totallight\n");
	}
}

/*
=============
RadWorld
=============
*/
static void RadWorld (const char *mapname)
{

	if (numnodes == 0 || numfaces == 0)
		Error ("Empty map");
	MakeBackplanes ();
	MakeParents (0, -1);
	MakeTnodes (&dmodels[0]);

	// turn each face into a single patch
	MakePatches ();

	// subdivide patches to a maximum dimension
	SubdividePatches ();

	// create directlights out of patches and lights
	CreateDirectLights ();
	
	GenerateAllTerrainLightmaps (mapname);
	
	// build initial facelights
	RunThreadsOnIndividual (numfaces, true, BuildFacelights);
	doing_texcheck = false;
	
	if (numbounce > 0)
	{
		// build transfer lists
		if(!memory)
		{
			RunThreadsOnIndividual (num_patches, true, MakeTransfers);
			qprintf ("transfer lists: %5.1f megs\n"
				, (float)total_transfer * sizeof(transfer_t) / (1024*1024));
		}
		
		numthreads = 1;
		
		// spread light around
		BounceLight ();

        FreeTransfers ();

		CheckPatches ();
	}
	else
		numthreads = 1;

    if(memory)
	{
        printf ("Non-memory conservation would require %4.1f\n",
            (float)(total_mem - trace_bytes) / 1048576.0f);
        printf ("    megabytes more memory then currently used\n");
	}

	if (glview)
		WriteGlView ();

	// blend bounced light into direct light and save
	PairEdges ();
	LinkPlaneFaces ();

	lightdatasize = 0;
	RunThreadsOn (numfaces, true, FinalLightFace_Worker);
}


// finalize any other changes to the lfacelookup data to make it ready to 
// write to disk
void TranslateRefine (void)
{
	int i;
	for (i = 0; i < numfaces; i++)
	{
	    lfacelookups[i].facenum = i;
	    lfacelookups[i].format = LTMP_PIXFMT_RGB24;
		lfacelookups[i].offset = dfaces[i].lightofs+1;
		lfacelookups[i].xscale = lfacelookups[i].yscale = 16.0/refine_amt;
	}
}

/*
========
main

light modelfile
========
*/
int main (int argc, char **argv)
{
	int		/*n,*/ full_help;
	double	start, end;
	char	name[1024], short_mapname_buf[1024], *short_mapname;
    char	game_path[1024] = "";
    char	refine_fname[1024];
    char	*param, *param2 = NULL;
    int		i;
#ifdef USE_SETRLIMIT
	const rlim_t kStackSize = 16L * 1024L * 1024L; //16 megs
	struct rlimit rl;
    int getrlimit_result;

	getrlimit_result = getrlimit(RLIMIT_STACK, &rl);
	if (getrlimit_result == 0)
	{
		if (rl.rlim_cur < kStackSize)
		{
			rl.rlim_cur = kStackSize;
			getrlimit_result = setrlimit(RLIMIT_STACK, &rl);
			if (getrlimit_result != 0)
			{
				fprintf(stderr, "setrlimit returned result = %d\n", getrlimit_result);
			}
		}
	}
#endif

	printf ("--- Alien Arena QRAD3 ---\n");

	verbose = false;
    full_help = false;
    numthreads = 4;
    refine_amt = 1;
    terrain_refine = 4;

    LoadConfigurationFile("qrad3", 0);
    LoadConfiguration(argc-1, argv+1);
    CreateConfigurationString ();

    while((param = WalkConfiguration()) != NULL)
	{
		if (!strcmp(param,"-dump"))
			dumppatches = true;
		else if (!strcmp(param,"-gamedir"))
		{
            param2 = WalkConfiguration();
			strncpy(game_path, param2, 1024);
		}
//		else if (!strcmp(param,"-moddir"))
//		{
//            param2 = WalkConfiguration();
//			strncpy(moddir, param2, 1024);
//		}
		else if (!strcmp(param,"-help"))
		{
			full_help = true;
		}
		else if (!strcmp(param,"-bounce"))
		{
            param2 = WalkConfiguration();
			numbounce = atoi (param2);
			// qprintf("number of bounces set to %d", numbounce );
		}
		else if (!strcmp(param,"-v"))
		{
			verbose = true;
		}
		else if (!strcmp(param,"-extra"))
		{
			extrasamples = true;
			// qprintf ("extrasamples set to true\n");
		}
		else if (!strcmp(param,"-threads"))
		{
            param2 = WalkConfiguration();
			numthreads = atoi (param2);
		}
		else if (!strcmp(param,"-subdiv"))
		{
            param2 = WalkConfiguration();
			subdiv = atof (param2);
		}
		else if (!strcmp(param,"-radmin"))
		{
            param2 = WalkConfiguration();
			patch_cutoff = atof (param2);
			// qprintf("radiosity minimum set to %f\n", patch_cutoff);
		}
		else if (!strcmp(param,"-scale"))
		{
            param2 = WalkConfiguration();
			lightscale = atof (param2);
			// qprintf ("light scaling set to %f\n", lightscale);
		}
		else if (!strcmp(param,"-direct"))
		{
            param2 = WalkConfiguration();
			direct_scale = atof (param2);
			// qprintf ("direct light scaling set to %f\n", direct_scale);
		}
		else if (!strcmp(param,"-entity"))
		{
            param2 = WalkConfiguration();
			entity_scale = atof(param2);
			// qprintf ("entity light scaling set to %f\n", entity_scale);
		}
		else if (!strcmp(param, "-grayscale") || !strcmp(param, "-greyscale") )
		{
			param2 = WalkConfiguration();
			grayscale = atof( param2 );
			// qprintf("grayscale set to %f\n", grayscale );
		}
		else if (!strcmp(param, "-desaturate"))
		{
			param2 = WalkConfiguration();
			desaturate = atof( param2 );
			// qprintf("desaturate set to %f\n", desaturate );
		}
		else if (!strcmp(param,"-glview"))
		{
			glview = true;
			printf ("glview = true\n");
		}
		else if (!strcmp(param,"-noblock"))
		{
			noblock = true;
			printf ("noblock = true\n");
		}
		else if (!strcmp(param,"-texcheck"))
		{
			doing_texcheck = true;
		}
		else if (!strcmp(param,"-blur"))
		{
			doing_blur = true;
		}
//		else if (!strcmp(param,"-memory"))
//		{
//			memory = true;
//			printf ("memory = true\n");
//		}
		else if (!strcmp(param,"-savetrace"))
		{
//			memory = true;
			save_trace = true;
			printf ("savetrace = true (memory set to true)\n");
		}
		else if (!strcmp(param,"-nopvs"))
		{
			nopvs = true;
			printf ("nopvs = true\n");
		}
		else if (!strcmp(param,"-ambient"))
		{
            param2 = WalkConfiguration();
			ambient = atof (param2);
		}
		else if (!strcmp(param,"-maxlight"))
		{
            param2 = WalkConfiguration();
			maxlight = atof (param2);
		}
		else if (!strcmp(param,"-refine"))
		{
			param2 = WalkConfiguration();
			refine_amt = refine_setting = atoi (param2);
			switch (refine_amt)
			{
				case 1: case 2: case 4: case 8:
				case 16: //heaven forbid
					break;
				default:
					Error ("Valid refine settings are 1, 2, 4, and 8");
			}
		}
		else if (!strcmp(param,"-terrainrefine"))
		{
			param2 = WalkConfiguration();
			terrain_refine = atoi (param2);
			switch (terrain_refine)
			{
				case 1: case 2: case 4: case 8: case 16:
				case 32: //heaven forbid
					break;
				default:
					Error ("Valid terrain refine settings are 1, 2, 4, and 8");
			}
		}
		else if (!strcmp (param,"-tmpin"))
			strcpy (inbase, "/tmp");
		else if (!strcmp (param,"-tmpout"))
			strcpy (outbase, "/tmp");
		else if (param[0] == '+')
            LoadConfigurationFile(param+1, 1);
		else
			break;
	}
	
	if (doing_blur && refine_amt < 8)
		printf ("Recommend refine setting of 8 or 16 for -blur!\n");

    printf("-------------------------\n");
	printf("ambient      : %f\n", ambient );
	printf("scale        : %f\n", lightscale );
	printf("maxlight     : %f\n", maxlight );
	printf("entity       : %f\n", entity_scale );
	printf("direct       : %f\n", direct_scale );
	printf("grayscale    : %f\n", grayscale );
	printf("desaturate   : %f\n", desaturate );
	printf("bounce       : %d\n", numbounce );
	printf("radmin       : %f\n", patch_cutoff );
	printf("subdiv       : %f\n", subdiv );
	printf("refine       : %d\n", refine_amt );
	printf("terrainrefine: %d\n", terrain_refine );
	if ( extrasamples )
		printf("with extra samples\n");
	if ( doing_blur )
		printf("with blurring\n");
    printf("-------------------------\n");

//    if (memory)
//        numthreads = 1;

//	ThreadSetDefault ();

	if (maxlight > 255.0f)
		maxlight = 255.0f;

    if (param != NULL)
        param2 = WalkConfiguration();

    if (param == NULL || param2 != NULL)
	{
        if(full_help)
		{
            printf ("usage: qrad3 [options] bspfile\n\n"
				"    -ambient #       -glview               -radmin #\n"
                "    -bounce #        -help                 -savetrace\n"
                "    -subdiv #        -maxlight #           -scale #\n"
                "    -direct #\n"
                "    -dump            -moddir <path>        -tmpin\n"
                "    -entity #        -noblock              -tmpout\n"
                "    -extra           -grayscale #          -v\n"
                "    -gamedir <path>  -nopvs\n"
                );

            exit(1);
		}
        else
		{
			Error ("usage: qrad3 [options] bspfile\n\n"
                "    qrad3 -help for full help\n");
		}
	}

    while(param2)  // make sure list is clean
        param2 = WalkConfiguration();

	start = I_FloatTime ();

    /*
     * param is the map/bsp file
     */
    SetQdirFromPath( param );
    printf("qdir = %s\n", qdir );
    printf("gamedir = %s\n", gamedir );

    // param is map/bsp file name
	strcpy (source, ExpandArg(param));
	StripExtension (source);
	strcpy (short_mapname_buf, source);
	strcpy (refine_fname, source);
	DefaultExtension (source, ".bsp");
	DefaultExtension (refine_fname, ".lightmap");

	// FIXME: duplicates COM_SkipPath.
	short_mapname = short_mapname_buf;
	while (true)
	{
		char *tmp = strchr (short_mapname, '/');
		if (!tmp)
			break;
		short_mapname = tmp + 1;
	}
	while (true)
	{
		char *tmp = strchr (short_mapname, '\\');
		if (!tmp)
			break;
		short_mapname = tmp + 1;
	}

//	ReadLightFile ();

	sprintf (name, "%s%s", inbase, source);
	printf ("reading %s\n", name);
	LoadBSPFile (name);
	ParseEntities ();
	LoadAllTerrain ();
	CalcTextureReflectivity ();

	if (!visdatasize)
	{
		printf ("No vis information, direct lighting only.\n");
		numbounce = 0;
		ambient = 16; // 2010-09 was 0.1, makes no sense, but 16 may not either.
 	}

	RadWorld (short_mapname);
	
	if (refine_setting > 0)
	{
		sprintf (name, "%s%s", outbase, refine_fname);
	}
	else
	{
		sprintf (name, "%s%s", outbase, source);
	}
	printf ("writing %s\n", name);
	if (refine_setting > 0)
	{
		TranslateRefine ();
		RunThreadsOnIndividual (numfaces, false, DetectUniformColor);
		WriteLTMPFile (name);
	}
	else
	{
		WriteBSPFile (name);
	}
	
	end = I_FloatTime ();
	printf ("%5.0f seconds elapsed\n", end-start);

	return 0;
}

