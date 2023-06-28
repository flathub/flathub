
#include "qrad.h"

vec3_t	texture_reflectivity[MAX_MAP_TEXINFO];

int cluster_neg_one = 0;

/*
===================================================================

  TEXTURE LIGHT VALUES

===================================================================
*/

/*
  CalcTextureReflectivity()

    For each Targa texture, calculate average RGB

*/
void CalcTextureReflectivity (void)
{
	int i, j, count;
	int texels;
	float color[3], cur_color[3], tex_a, a;
	char path[1024];
	float r, c;
	byte* pbuffer;
	byte* ptexel;
	float *fbuffer, *ftexel;
	int width, height;

	// for TGA RGBA texture images

	//TODO Instead of looking at the entire texture, figure out what parts of
	//each texture are shown on each face individually, and calculate the 
	//reflectivity for those. This would allow breaking faces up along 
	//boundaries within the texture (done manually by the mapper) to improve
	//lighting in some areas.
	for( i = 0; i < numtexinfo; i++ )
	{
		// default
		texture_reflectivity[i][0] = 0.5f;
		texture_reflectivity[i][1] = 0.5f;
		texture_reflectivity[i][2] = 0.5f;

#if 1
		// see if an earlier texinfo already got the value
		for (j=0 ; j<i ; j++)
		{
			if (!strcmp (texinfo[i].texture, texinfo[j].texture))
			{
				VectorCopy(texture_reflectivity[j], texture_reflectivity[i]);
				texture_data[i] = texture_data[j];
				texture_sizes[i][0] = texture_sizes[j][0];
				texture_sizes[i][1] = texture_sizes[j][1];
				break;
			}
		}
		if (j != i)
			continue;
#endif

		// buffer is RGBA  (A  set to 255 for 24 bit format)
		// looks in arena/textures and then data1/textures
		sprintf( path, "%sarena/textures/%s.tga", gamedir, texinfo[i].texture );
		if ( FileExists( path ) ) // LoadTGA expects file to exist
		{
			LoadTGA( path, &pbuffer, &width, &height ); // load rgba data
			qprintf("load %s\n", path );
		}
		else
		{
			sprintf( path, "%sdata1/textures/%s.tga", gamedir, texinfo[i].texture );
			if ( FileExists( path ) )
			{
				LoadTGA( path, &pbuffer, &width, &height ); // load rgba data
				qprintf("load %s\n", path );
			}
			else
			{
				qprintf("noload %s\n", path);
				continue;
			}
		}

		//
		// Calculate the "average color" for the texture
		//
		texels =  width * height;
		if (texels <= 0)
		{
			qprintf("tex %i (%s) no rgba data (file broken?)\n", i, path );
			continue; // empty texture, possible bad file
		}
		color[0] = color[1] = color[2] = 0.0f;
		ptexel = pbuffer;
		fbuffer = malloc (texels*4*sizeof(float));
		ftexel = fbuffer;
		for ( count = texels;  count--; )
		{
			cur_color[0] = (float)(*ptexel++); // r
			cur_color[1] = (float)(*ptexel++); // g
			cur_color[2] = (float)(*ptexel++); // b
			tex_a = (float)(*ptexel++);
			if (texinfo[i].flags & (SURF_WARP|SURF_NODRAW))
			{
				a = 0.0;
			}
			else if ((texinfo[i].flags & SURF_TRANS33) && (texinfo[i].flags & SURF_TRANS66))
			{
				a = tex_a/255.0;
			} 
			else if (texinfo[i].flags & SURF_TRANS33)
			{
				a = tex_a/765.0;
			}
			else if (texinfo[i].flags & SURF_TRANS66)
			{
				a = tex_a/382.5;
			}
			else
			{
				a = 1.0;
			}

			for (j = 0; j < 3; j++)
			{
				*ftexel++ = cur_color[j]/255.0;
				color[j] += cur_color[j]*a;
			}
			*ftexel++ = a;
		}

		//never freed but we'll need it up until the end
		texture_data[i] = fbuffer; 
		free (pbuffer);
		
		texture_sizes[i][0] = width;
		texture_sizes[i][1] = height;

		for( j = 0; j < 3; j++ )
		{ // average RGB for the texture to 0.0..1.0 range
			r = color[j] / (float)texels / 255.0f;
			texture_reflectivity[i][j] = r;
		}

		// desaturate reflectivity here (TODO: check that avg. rgb makes sense)
		r = 1.0f - desaturate;
		c = ((texture_reflectivity[i][0]
			+ texture_reflectivity[i][1]
			+ texture_reflectivity[i][2]) / 3.0f) * desaturate;
		texture_reflectivity[i][0] = (texture_reflectivity[i][0] * r) + c;
		texture_reflectivity[i][1] = (texture_reflectivity[i][1] * r) + c;
		texture_reflectivity[i][2] = (texture_reflectivity[i][2] * r) + c;

		qprintf("tex %i (%s) avg rgb [ %f, %f, %f ]\n",
			i, path, texture_reflectivity[i][0],
			texture_reflectivity[i][1],texture_reflectivity[i][2]);

	}
}

/*
=======================================================================

MAKE FACES

=======================================================================
*/

/*
=============
WindingFromFace
=============
*/
winding_t	*WindingFromFace (dface_t *f)
{
	int			i;
	int			se;
	dvertex_t	*dv;
	int			v;
	winding_t	*w;

	w = AllocWinding (f->numedges);
	w->numpoints = f->numedges;

	for (i=0 ; i<f->numedges ; i++)
	{
		se = dsurfedges[f->firstedge + i];
		if (se < 0)
			v = dedges[-se].v[1];
		else
			v = dedges[se].v[0];

		dv = &dvertexes[v];
		VectorCopy (dv->point, w->p[i]);
	}

	RemoveColinearPoints (w);

	return w;
}

/*
=============
BaseLightForFace
=============
*/
void BaseLightForFace (dface_t *f, vec3_t color)
{
	texinfo_t	*tx;

	//
	// check for light emited by texture
	//
	tx = &texinfo[f->texinfo];

    if (!(tx->flags & SURF_LIGHT) || tx->value == 0)
	{
        if(tx->flags & SURF_LIGHT)
            {
            printf("Surface light has 0 intensity.\n");
            }

		VectorClear (color);
		return;
	}

    VectorScale (texture_reflectivity[f->texinfo], tx->value, color);
}

qboolean IsSky (dface_t *f)
{
	texinfo_t	*tx;

	tx = &texinfo[f->texinfo];
	if (tx->flags & SURF_SKY)
		return true;
	return false;
}

/*
=============
MakePatchForFace
=============
*/
float	totalarea;
void MakePatchForFace (int fn, winding_t *w)
{
	dface_t *f;
	float	area;
	patch_t		*patch;
	dplane_t	*pl;
	int			i;
	vec3_t		color = {1.0f,1.0f,1.0f};
	dleaf_t		*leaf;

	f = &dfaces[fn];

	area = WindingArea (w);
	totalarea += area;

	patch = &patches[num_patches];
	if (num_patches == MAX_PATCHES)
		Error ("num_patches == MAX_PATCHES");
	patch->next = face_patches[fn];
	face_patches[fn] = patch;

	patch->winding = w;

	if (f->side)
		patch->plane = &backplanes[f->planenum];
	else
		patch->plane = &dplanes[f->planenum];
	if (face_offset[fn][0] || face_offset[fn][1] || face_offset[fn][2] )
	{	// origin offset faces must create new planes
		if (numplanes + fakeplanes >= MAX_MAP_PLANES)
			Error ("numplanes + fakeplanes >= MAX_MAP_PLANES");
		pl = &dplanes[numplanes + fakeplanes];
		fakeplanes++;

		*pl = *(patch->plane);
		pl->dist += DotProduct (face_offset[fn], pl->normal);
		patch->plane = pl;
	}

	WindingCenter (w, patch->origin);
	VectorAdd (patch->origin, patch->plane->normal, patch->origin);
	leaf = PointInLeaf(patch->origin);
	patch->cluster = leaf->cluster;
	if (patch->cluster == -1)
	{
		// qprintf ("patch->cluster == -1\n");
		++cluster_neg_one;
	}

	patch->area = area;
	if (patch->area <= 1)
		patch->area = 1;
	patch->sky = IsSky (f);

	VectorCopy (texture_reflectivity[f->texinfo], patch->reflectivity);

	// non-bmodel patches can emit light
	if (fn < dmodels[0].numfaces)
	{
		BaseLightForFace (f, patch->baselight);

		ColorNormalize (patch->reflectivity, color);

		for (i=0 ; i<3 ; i++)
			patch->baselight[i] *= color[i];

		VectorCopy (patch->baselight, patch->totallight);
	}
	num_patches++;
}


entity_t *EntityForModel (int modnum)
{
	int		i;
	char	*s;
	char	name[16];

	sprintf (name, "*%i", modnum);
	// search the entities for one using modnum
	for (i=0 ; i<num_entities ; i++)
	{
		s = ValueForKey (&entities[i], "model");
		if (!strcmp (s, name))
			return &entities[i];
	}

	return &entities[0];
}

/*
=============
MakePatches
=============
*/
void MakePatches (void)
{
	int		i, j, k;
	dface_t	*f;
	int		fn;
	winding_t	*w;
	dmodel_t	*mod;
	vec3_t		origin;
	entity_t	*ent;

	qprintf ("%i faces\n", numfaces);

	for (i=0 ; i<nummodels ; i++)
	{
		mod = &dmodels[i];
		ent = EntityForModel (i);
		// bmodels with origin brushes need to be offset into their
		// in-use position
		GetVectorForKey (ent, "origin", origin);
//VectorCopy (vec3_origin, origin);

		for (j=0 ; j<mod->numfaces ; j++)
		{
			fn = mod->firstface + j;
			face_entity[fn] = ent;
			VectorCopy (origin, face_offset[fn]);
			f = &dfaces[fn];
			w = WindingFromFace (f);
			for (k=0 ; k<w->numpoints ; k++)
			{
				VectorAdd (w->p[k], origin, w->p[k]);
			}
			MakePatchForFace (fn, w);
		}
	}

	qprintf ("area = %u (%i square feet)\n", (unsigned int)totalarea, (int)(totalarea/64));
}

/*
=======================================================================

SUBDIVIDE

=======================================================================
*/


void FinishSplit (patch_t *patch, patch_t *newp)
{
	dleaf_t		*leaf;

	VectorCopy (patch->baselight, newp->baselight);
	VectorCopy (patch->totallight, newp->totallight);
	VectorCopy (patch->reflectivity, newp->reflectivity);
	newp->plane = patch->plane;
	newp->sky = patch->sky;

	patch->area = WindingArea (patch->winding);
	newp->area = WindingArea (newp->winding);

	if (patch->area <= 1)
		patch->area = 1;
	if (newp->area <= 1)
		newp->area = 1;

	WindingCenter (patch->winding, patch->origin);
	VectorAdd (patch->origin, patch->plane->normal, patch->origin);
	leaf = PointInLeaf(patch->origin);
	patch->cluster = leaf->cluster;

	if (patch->cluster == -1)
	{ // in solid leaf ???
		// qprintf ("patch->cluster == -1\n");
		++cluster_neg_one;
	}

	WindingCenter (newp->winding, newp->origin);
	VectorAdd (newp->origin, newp->plane->normal, newp->origin);
	leaf = PointInLeaf(newp->origin);
	newp->cluster = leaf->cluster;

	if (newp->cluster == -1)
	{ // in solid leaf ???
		// qprintf ("patch->cluster == -1\n");
		++cluster_neg_one;
	}
}

/*
=============
SubdividePatch

Chops the patch only if its local bounds exceed the max size
=============
*/
void	SubdividePatch (patch_t *patch)
{
	winding_t *w, *o1, *o2;
	vec3_t	mins, maxs, total;
	vec3_t	split;
	vec_t	dist;
	int		i, j;
	vec_t	v;
	patch_t	*newp;

	w = patch->winding;
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = w->p[i][j];
			if (v < mins[j])
				mins[j] = v;
			if (v > maxs[j])
				maxs[j] = v;
		}
	}
	VectorSubtract (maxs, mins, total);
	for (i=0 ; i<3 ; i++)
		if (total[i] > (subdiv+1) )
			break;
	if (i == 3)
	{
		// no splitting needed
		return;
	}

	//
	// split the winding
	//
	VectorCopy (vec3_origin, split);
	split[i] = 1;
	dist = (mins[i] + maxs[i])*0.5;
	ClipWindingEpsilon (w, split, dist, ON_EPSILON, &o1, &o2);

	//
	// create a new patch
	//
	if (num_patches == MAX_PATCHES)
		Error ("MAX_PATCHES");
	newp = &patches[num_patches];
	num_patches++;

	newp->next = patch->next;
	patch->next = newp;

	patch->winding = o1;
	newp->winding = o2;

	FinishSplit (patch, newp);

	SubdividePatch (patch);
	SubdividePatch (newp);
}


/*
=============
DicePatch

Chops the patch by a global grid
=============
*/
void	DicePatch (patch_t *patch)
{
	winding_t *w, *o1, *o2;
	vec3_t	mins, maxs;
	vec3_t	split;
	vec_t	dist;
	int		i;
	patch_t	*newp;
	float local_subdiv = subdiv;
	
	// Subdivide sky less finely to prevent compile times for maps with
	// sunlight from getting out of hand.
	if (patch->sky)
	    local_subdiv *= 3.0;

	w = patch->winding;
	WindingBounds (w, mins, maxs); // 3D AABB for polygon

	for (i=0 ; i<3 ; i++)
	{ // if an AABB dimension > subdiv then split (?)
		if (floor((mins[i]+1)/local_subdiv) < floor((maxs[i]-1)/local_subdiv))
			break;
	}

	if (i == 3)
	{ // no splitting needed
		return;
	}

	//
	// split the winding
	//
	VectorCopy (vec3_origin, split);
	split[i] = 1;
	dist = local_subdiv * ( 1 + floor( (mins[i]+1) / local_subdiv) );
	// {split,dist} is the dividing plane (?)
	ClipWindingEpsilon (w, split, dist, ON_EPSILON, &o1, &o2);

	//
	// create a new patch
	//
	if (num_patches == MAX_PATCHES)
		Error ("MAX_PATCHES");
	newp = &patches[num_patches];
	num_patches++;

	newp->next = patch->next;
	patch->next = newp;

	patch->winding = o1;
	newp->winding = o2;

	FinishSplit (patch, newp);

	DicePatch (patch);
	DicePatch (newp);
}

/*
=============
SubdividePatches
=============
*/
void SubdividePatches (void)
{
	int		i, num;

	if (subdiv < 1)
		return;

	num = num_patches;	// because the list will grow
	for (i=0 ; i<num ; i++)
	{
//		SubdividePatch (&patches[i]);
		DicePatch (&patches[i]);
	}
	for (i=0; i<num_patches; i++)
		patches[i].nodenum = PointInNodenum (patches[i].origin);
	qprintf ("%i patches after subdivision\n", num_patches);
	qprintf( "[? patch->cluster=-1 count is %i  ?in solid leaf?]\n", cluster_neg_one );
}

//=====================================================================
