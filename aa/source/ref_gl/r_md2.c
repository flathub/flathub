/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2004-2014 COR Entertainment, LLC

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// r_md2.c: MD2 file format loader

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

static mesh_framevbo_t *MD2_GetFrameVBO (void *data, int framenum)
{
	dmdl_t			*pheader = (dmdl_t *)data;
	int				va, i;
	daliasframe_t	*frame;
	dtrivertx_t		*verts;
	dtriangle_t		*tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	static mesh_framevbo_t framevbo[MAX_TRIANGLES * 3];
	
	memset (framevbo, 0, sizeof(framevbo));

	frame = (daliasframe_t *)((byte *)pheader + pheader->ofs_frames + framenum * pheader->framesize);
	verts = frame->verts;

	for (va = 0; va < pheader->num_tris*3; va++)
	{
		int index_xyz = tris[va/3].index_xyz[va%3];
		
		for (i = 0; i < 3; i++)
			framevbo[va].vertex[i] = verts[index_xyz].v[i] * frame->scale[i] + frame->translate[i];
	}
	
	return framevbo;
}

static void MD2_LoadVBO (model_t *mod, dmdl_t *pheader, fstvert_t *st)
{
	int va;
	static nonskeletal_basevbo_t basevbo[MAX_TRIANGLES * 3];
	dtriangle_t *tris;
	
	tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	
	for (va = 0; va < pheader->num_tris*3; va++)
	{
		int index_st = tris[va/3].index_st[va%3];
	
		basevbo[va].st[0] = st[index_st].s;
		basevbo[va].st[1] = st[index_st].t;
	}
	
	R_Mesh_LoadVBO (mod, MESHLOAD_CALC_NORMAL|MESHLOAD_CALC_TANGENT, basevbo, MD2_GetFrameVBO, (void *)pheader);
}

/*
=================
Mod_LoadMD2Model
TODO: the MD2 format actually supports partial indexing, we should use it!
=================
*/
void Mod_LoadMD2Model (model_t *mod, void *buffer)
{
	int					i, j;
	dmdl_t				*pinmodel, *pheader;
	dstvert_t			*pinst, *poutst;
	dtriangle_t			*pintri, *pouttri;
	daliasframe_t		*pinframe, *poutframe, *pframe;
	int					version;
	int					cx;
	float				s, t;
	float				iw, ih;
	fstvert_t			*st;
	char *pstring;
	int count;
	image_t *image;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Com_Printf("%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_VERSION);

	// TODO: we can stop permanently keeping this (change it to normal
	// Z_Malloc) as soon as we find another place to store skin names.
	mod->extradata = Hunk_Begin (0x300000);
	pheader = (dmdl_t *)Hunk_Alloc (LittleLong(pinmodel->ofs_end));

	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dmdl_t)/sizeof(int) ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Com_Printf("model %s has a skin taller than %d at %d", mod->name,
		MAX_LBM_HEIGHT, pheader->skinheight);

	if (pheader->num_xyz <= 0)
		Com_Printf("model %s has no vertices", mod->name);

	if (pheader->num_xyz > MAX_VERTS)
		Com_Printf("model %s has too many vertices", mod->name);

	if (pheader->num_st <= 0)
		Com_Printf("model %s has no st vertices", mod->name);

	if (pheader->num_tris <= 0)
		Com_Printf("model %s has no triangles", mod->name);

	if (pheader->num_frames <= 0)
		Com_Printf("model %s has no frames", mod->name);

//
// load base s and t vertices
//
	pinst = (dstvert_t *) ((byte *)pinmodel + pheader->ofs_st);
	poutst = (dstvert_t *) ((byte *)pheader + pheader->ofs_st);

	for (i=0 ; i<pheader->num_st ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}

//
// load triangle lists
//
	pintri = (dtriangle_t *) ((byte *)pinmodel + pheader->ofs_tris);
	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	for (i=0 ; i<pheader->num_tris ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort (pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort (pintri[i].index_st[j]);
		}
	}

//
// load the frames
//
	for (i=0 ; i<pheader->num_frames ; i++)
	{
		pinframe = (daliasframe_t *) ((byte *)pinmodel
			+ pheader->ofs_frames + i * pheader->framesize);
		poutframe = (daliasframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts,
			pheader->num_xyz*sizeof(dtrivertx_t));

	}

	mod->type = mod_md2;
	mod->typeFlags = MESH_CASTSHADOWMAP | MESH_MORPHTARGET | MESH_DOSHADING;
	mod->num_frames = pheader->num_frames;
	
	// skin names are not always valid or file may not exist
	// do not register skins that cannot be found to eliminate extraneous
	//  file system searching.
	pstring = &((char*)pheader)[ pheader->ofs_skins ];
	count = pheader->num_skins;
	if ( count )
	{ // someday .md2's that do not have skins may have zero for num_skins
		memcpy( pstring, (char *)pinmodel + pheader->ofs_skins, count*MAX_SKINNAME );
		i = 0;
		while ( count-- )
		{
			pstring[MAX_SKINNAME-1] = '\0'; // paranoid
			image = GL_FindImage( pstring, it_skin);
			if ( image != NULL )
				mod->skins[i++] = image;
			else
				pheader->num_skins--; // the important part: adjust skin count
			pstring += MAX_SKINNAME;
		}
		
		// load script
		if (mod->skins[0] != NULL)
			mod->script = mod->skins[0]->script;
		if (mod->script)
			RS_ReadyScript( mod->script );
	}

	cx = pheader->num_st * sizeof(fstvert_t);
	st = (fstvert_t*)Z_Malloc (cx);

	// Calculate texcoords for triangles
	iw = 1.0 / pheader->skinwidth;
	ih = 1.0 / pheader->skinheight;
	for (i=0; i<pheader->num_st ; i++)
	{
		s = poutst[i].s;
		t = poutst[i].t;
		st[i].s = (s + 1.0) * iw; //tweak by one pixel
		st[i].t = (t + 1.0) * ih;
	}

	cx = pheader->num_xyz * pheader->num_frames * sizeof(byte);

	mod->num_triangles = pheader->num_tris;
	mod->numvertexes = 3*mod->num_triangles; // TODO: use MD2's indexing!

	//redo this using max/min from all frames
	pframe = ( daliasframe_t * ) ( ( byte * ) pheader +
									  pheader->ofs_frames);

	/*
	** compute axially aligned mins and maxs
	*/
	for ( i = 0; i < 3; i++ )
	{
		mod->mins[i] = pframe->translate[i];
		mod->maxs[i] = mod->mins[i] + pframe->scale[i]*255;
	}

	/*
	** compute a full bounding box
	*/
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mod->mins[0];
		else
			tmp[0] = mod->maxs[0];

		if ( i & 2 )
			tmp[1] = mod->mins[1];
		else
			tmp[1] = mod->maxs[1];

		if ( i & 4 )
			tmp[2] = mod->mins[2];
		else
			tmp[2] = mod->maxs[2];

		VectorCopy( tmp, mod->bbox[i] );
	}	
	
	MD2_LoadVBO (mod, pheader, st);
	
	Z_Free (st);
	
	mod->extradatasize = Hunk_End ();
}

void MD2_SelectFrame (entity_t *ent, const model_t *mod)
{
	if (ent->frame >= mod->num_frames || ent->frame < 0 ||
	    ent->oldframe >= mod->num_frames || ent->oldframe < 0)
	{
		ent->frame = 0;
		ent->oldframe = 0;
	}

	if (!r_lerpmodels->integer)
		ent->backlerp = 0;
}
