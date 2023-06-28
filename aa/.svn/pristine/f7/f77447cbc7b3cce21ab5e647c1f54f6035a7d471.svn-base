/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010-2014 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
// r_model.c -- model loading and caching

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_iqm.h"
#include "r_ragdoll.h"
#include "client/sound.h"

model_t	*loadmodel;
int		modfilelen;

void Mod_LoadBrushModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

LightGroup_t LightGroups[MAX_LIGHTS];
int r_lightgroups;

int		registration_sequence;

#if defined WIN32_VARIANT
char map_music[MAX_PATH];
char map_music_sec[MAX_PATH];
#else
char map_music[MAX_OSPATH];
char map_music_sec[MAX_OSPATH];
#endif

byte	*mod_base;

void R_RegisterLightGroups (void)
{
	int			i;
	vec3_t		dist;
	int			groupsize = 0, numgrouped = 0;

	for (i = 0; i < r_numWorldLights; i++)
		r_worldLights[i].grouped = false;

	r_lightgroups = 0;
	
	while (numgrouped < r_numWorldLights)
	{
		for (i = 0; i < r_numWorldLights; i++)
		{
			if (r_worldLights[i].grouped)
				continue;

			if (groupsize == 0)
			{
				// none in group yet, first light establishes the initial origin of the group
				VectorCopy (r_worldLights[i].origin, LightGroups[r_lightgroups].group_origin);
				VectorCopy (r_worldLights[i].origin, LightGroups[r_lightgroups].accum_origin);
				LightGroups[r_lightgroups].avg_intensity = r_worldLights[i].intensity;
				r_worldLights[i].grouped = true;
				groupsize++;
				numgrouped++;
				continue;
			}

			VectorSubtract (LightGroups[r_lightgroups].group_origin, r_worldLights[i].origin, dist);
			
			if (VectorLength (dist) >= 256.0f)
				continue;
			
			if (CM_FastTrace (LightGroups[r_lightgroups].group_origin, r_worldLights[i].origin, r_worldmodel->firstnode, MASK_OPAQUE))
			{
				r_worldLights[i].grouped = true;
				VectorAdd (r_worldLights[i].origin, LightGroups[r_lightgroups].accum_origin, LightGroups[r_lightgroups].accum_origin);
				LightGroups[r_lightgroups].avg_intensity += r_worldLights[i].intensity;
				groupsize++;
				numgrouped++;
			}
		}
		
		// finalize current group
		assert (groupsize > 0);
		VectorScale (LightGroups[r_lightgroups].accum_origin, 1.0f/(float)groupsize, LightGroups[r_lightgroups].accum_origin);
		LightGroups[r_lightgroups].avg_intensity /= (float)groupsize;
		VectorCopy (LightGroups[r_lightgroups].accum_origin, LightGroups[r_lightgroups].group_origin);
		
		// start next group
		r_lightgroups++;
		groupsize = 0;
	}
	Com_Printf("Condensed ^2%i worldlights into ^2%i lightgroups\n", r_numWorldLights, r_lightgroups);
}

static void R_ParseLightEntity (char *match, char *block)
{
	int		i;
	char	*tok, *bl;
	vec3_t	origin;
	float	intensity;
	
	if (r_numWorldLights >= MAX_LIGHTS)
		return;
	
	VectorClear(origin);
	intensity = 0;

	bl = block;
	while (1){
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		// exclude sun from light groups
		if (!Q_strcasecmp("targetname", tok)){
			tok = Com_ParseExt(&bl, false);
			if (!Q_strcasecmp("sunspot", tok) || !Q_strcasecmp("moonspot", tok))
		   		return;
		}

		if (!Q_strcasecmp("origin", tok)){
			for (i = 0; i < 3; i++){
				tok = Com_ParseExt(&bl, false);
				origin[i] = atof(tok);
			}
		}
		else if (!Q_strcasecmp("light", tok) || !Q_strcasecmp("_light", tok)){
			tok = Com_ParseExt(&bl, false);
			intensity = atof(tok);
		}
		else
			Com_SkipRestOfLine(&bl);
	}

	if (!intensity)
		intensity = 150;

	// Add it to the list
	VectorCopy(origin, r_worldLights[r_numWorldLights].origin);
	r_worldLights[r_numWorldLights].intensity = intensity/2.0f;
	r_worldLights[r_numWorldLights].surf = NULL;
	r_numWorldLights++;
}

// NOTE: If you update this, you may also want to update
// CM_ParseTerrainModelEntity in qcommon/cmodel.c.
static void R_ParseTerrainModelEntity (char *match, char *block)
{
	entity_t	*ent;
	int			i;
	char		*bl, *tok;
	
	if (num_terrain_entities == MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "R_ParseTerrainEntity: MAX_MAP_MODELS");
	
	ent = &terrain_entities[num_terrain_entities];
	memset (ent, 0, sizeof(*ent));
	ent->number = MAX_EDICTS+num_terrain_entities++;
	
	bl = block;
	while (1)
	{
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("model", tok))
		{
			ent->model = R_RegisterModel (Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("angles", tok))
		{
			for (i = 0; i < 3; i++)
				ent->angles[i] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("angle", tok))
		{
			ent->angles[YAW] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("origin", tok))
		{
			for (i = 0; i < 3; i++)
				ent->origin[i] = atof(Com_ParseExt(&bl, false));
		}
		else
			Com_SkipRestOfLine(&bl);
	}
	
	Mod_LoadTerrainDecorations (ent->model->name, ent->angles, ent->origin);
}

// For sorting the rock entities by mesh. TODO: maybe sort other entity types
// as well?
int compare_entity (void const *_a, void const *_b)
{
	entity_t *a = (entity_t *)_a;
	entity_t *b = (entity_t *)_b;	

	if (a->model > b->model)
		return -1;
	else if (a->model < b->model)
		return 1;
	
	return 0;
}

static void R_ParseTerrainEntities (void)
{
	static const char *classnames[] = {"misc_terrainmodel"};
	static const char *decal_classnames[] = {"misc_decal"};
	
	num_terrain_entities = 0;
	num_rock_entities = 0;
	num_decal_entities = 0;
	CM_FilterParseEntities ("classname", 1, classnames, R_ParseTerrainModelEntity);
	qsort (rock_entities, num_rock_entities, sizeof(rock_entities[0]), compare_entity);
	CM_FilterParseEntities ("classname", 1, decal_classnames, R_ParseDecalEntity);
}

static void R_ParseSunTarget (char *match, char *block)
{
	int		i;
	char	*bl, *tok;
	
	bl = block;
	while (1){
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("origin", tok)){
			for (i = 0; i < 3; i++){
				tok = Com_ParseExt(&bl, false);
				r_sunLight->target[i] = atof(tok);
			}
			//Com_Printf("Found sun target@ : %4.2f %4.2f %4.2f\n", r_sunLight->target[0], r_sunLight->target[1], r_sunLight->target[2]);
		}
		else
			Com_SkipRestOfLine(&bl);
	}
}

static void R_FindSunTarget (void)
{
	static const char *targetnames[] = {"moonspot", "sunspot"};
	
	CM_FilterParseEntities ("targetname", 2, targetnames, R_ParseSunTarget);
}

static void R_ParseSunEntity (char *match, char *block)
{
	int		i;
	char	*bl, *tok;
	
	strcpy(r_sunLight->targetname, match);

	bl = block;
	while (1){
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("origin", tok)){
			for (i = 0; i < 3; i++){
				tok = Com_ParseExt(&bl, false);
				r_sunLight->origin[i] = atof(tok);
				r_sunLight->target[i] = 0; //default to this
			}
			r_sunLight->has_Sun = true;
			//Com_Printf("Found sun @ : %4.2f %4.2f %4.2f\n", r_sunLight->origin[0], r_sunLight->origin[1], r_sunLight->origin[2]);
		}
		else
			Com_SkipRestOfLine(&bl);
	}
}

static void R_FindSunEntity (void)
{
	static const char *targets[] = {"moonspot", "sunspot"};

	if(r_sunLight)
		free(r_sunLight); //free at level load

	r_sunLight = (sunLight_t*)malloc(sizeof(sunLight_t));

	r_sunLight->has_Sun = false;
	
	CM_FilterParseEntities ("target", 2, targets, R_ParseSunEntity);

	if(r_sunLight->has_Sun)
		R_FindSunTarget(); //find target
}

static void R_ParseLightEntities (void)
{
	static const char *classnames[] = {"light"};
	
	CM_FilterParseEntities ("classname", 1, classnames, R_ParseLightEntity);
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	cplane_t	*plane;

	if (!model || !model->nodes)
		Com_Printf ("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}


//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int		i;
	model_t	*mod;
	int		total;

	total = 0;
	Com_Printf ("Loaded models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		Com_Printf ("%8i : %s\n",mod->extradatasize, mod->name);
		total += mod->extradatasize;
	}
	Com_Printf ("Total resident: %i\n", total);
}

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}



/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	unsigned *buf;
	int		i;
	char shortname[MAX_QPATH], nameShortname[MAX_QPATH];
	qboolean is_iqm = false, is_terrain = false, is_decal = false;

	if (!name[0])
		Com_Error (ERR_DROP, "Mod_ForName: NULL name");
	
	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name+1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			Com_Error (ERR_DROP, "bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;

		COM_StripExtension(mod->name, shortname);
		COM_StripExtension(name, nameShortname);

		if (!strcmp (shortname, nameShortname) )
		{
			if (mod->type == mod_md2 || mod->type == mod_iqm || mod->type == mod_terrain || mod->type == mod_decal)
			{
				// Make sure models scripts are definately reloaded between maps
				image_t *img;
				img=mod->skins[0];

				if (img != NULL)
				{
					mod->script = mod->skins[0]->script;
					if (mod->script)
						RS_ReadyScript( mod->script );
				}
			}
			
			return mod;
		}
	}

	//
	// find a free model slot spot
	//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			Com_Error (ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
		mod_numknown++;
	}
	
	memset (mod, 0, sizeof (*mod));
	
	strcpy (mod->name, name);
	
	R_SetSimpleTexnum (mod, name);

	//
	// load the file
	//
	
	//if .md2, check for IQM version first
	COM_StripExtension(mod->name, shortname);
	strcat(shortname, ".iqm");

	modfilelen = FS_LoadFile (shortname, (void**)&buf);

	if(!buf) //could not find iqm
	{
		modfilelen = FS_LoadFile (mod->name, (void**)&buf);
		if (!buf)
		{
			if (crash)
				Com_Error (ERR_DROP, "Mod_NumForName: %s not found", mod->name);
			memset (mod->name, 0, sizeof(mod->name));
			return NULL;
		}
		if (strstr (mod->name, ".terrain"))
			is_terrain = true;
		else if (strstr (mod->name, ".decal"))
			is_decal = true;
	}
	else
	{
		//we have an .iqm
		is_iqm = true;
		strcpy(mod->name, shortname);
	}

	loadmodel = mod;

	if ( developer && developer->integer == 2 )
	{ // tracing for model loading
		Com_DPrintf("Mod_ForName: load: %s\n", loadmodel->name );
	}

	//
	// fill it in
	//

	// call the apropriate loader
	if (is_terrain)
	{
		Mod_LoadTerrainModel (mod, buf);
	}
	else if (is_decal)
	{
		Mod_LoadDecalModel (mod, buf);
	}
	else if (is_iqm)
	{
		if(!Mod_INTERQUAKEMODEL_Load(mod, buf))
			Com_Error (ERR_DROP,"Mod_NumForName: wrong fileid for %s", mod->name);
	}
	else
	{
		switch (LittleLong(*(unsigned *)buf))
		{
		case IDALIASHEADER:
			Mod_LoadMD2Model (mod, buf);
			break;

		case IDBSPHEADER:
			Mod_LoadBrushModel (mod, buf);
			break;

		default:
			Com_Error (ERR_DROP,"Mod_NumForName: unknown fileid for %s", mod->name);
			break;
		}
	}

	FS_FreeFile (buf);
	
	return mod;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte	*mod_base;


/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = Hunk_Alloc ( l->filelen);
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (lump_t *l)
{
	int		i;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = Hunk_Alloc ( l->filelen);
	memcpy (loadmodel->vis, mod_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleLong (loadmodel->vis->numclusters);
	for (i=0 ; i<loadmodel->vis->numclusters ; i++)
	{
		loadmodel->vis->bitofs[i][0] = LittleLong (loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleLong (loadmodel->vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength (corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	mmodel_t	*out;
	int			i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = LittleLong (in->headnode);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( (count + 1) * sizeof(*out));

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		int j;
		
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
		
		out->usecount = 0;
		out->first_face_plane = NULL;
		out->iscorner = 0;
		
		for (j = 0; j < 3; j++)
		{
			if (loadmodel->vertexes[out->v[0]].position[j] < loadmodel->vertexes[out->v[1]].position[j])
			{
				out->mins[j] = loadmodel->vertexes[out->v[0]].position[j];
				out->maxs[j] = loadmodel->vertexes[out->v[1]].position[j];
			}
			else
			{
				out->mins[j] = loadmodel->vertexes[out->v[1]].position[j];
				out->maxs[j] = loadmodel->vertexes[out->v[0]].position[j];
			}
		}
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/

int compare_unique_texinfo (const void *_a, const void *_b)
{
	mtexinfo_t *a = *(mtexinfo_t **)_a, *b = *(mtexinfo_t **)_b;
	return a->image->texnum-b->image->texnum;
}

void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in, *in_base;
	mtexinfo_t *out, *step, **unique_temp;
	int 	i, j, count, num_unique;
	char	name[MAX_QPATH];
	char	sv_name[MAX_QPATH];
	int		next;

	in_base = in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<4 ; j++)
		{
			/* note: treating as vecs[0][8] gives warning with gcc -O3 */
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		}
		out->value = LittleLong (in->value);
		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nexttexinfo);
		if (next > 0 && next < loadmodel->numtexinfo)
			out->next = loadmodel->texinfo + next;
		else
		    out->next = NULL;
		
		out->equiv = out;

		Com_sprintf (name, sizeof(name), "textures/%s.wal", in->texture);
		out->image = GL_FindImage (name, it_wall);

		Com_sprintf (name, sizeof(name), "textures/%s", in->texture);
		out->image->script = RS_FindScript(name);

		if (out->image->script)
			RS_ReadyScript(out->image->script);

		//check for normal, height and specular maps
		strcpy(sv_name, name);
		if( ( strlen( name ) + 8 ) <= MAX_QPATH )
		{
			strcat( name, "_nm.tga" );
			out->normalMap = GL_FindImage( name, it_bump );
			if( out->normalMap == NULL )
			{
				out->has_normalmap = false;
				out->normalMap = out->image;
			}
			else
				out->has_normalmap = true;
		}
		else
		{
			out->has_normalmap = false;
			out->normalMap = out->image;
		}

		strcpy(name, sv_name);
		if( ( strlen( name ) + 8 ) <= MAX_QPATH )
		{
			strcat( name, "_hm.tga" );
			out->heightMap = GL_FindImage( name, it_bump );
			if( out->heightMap == NULL )
			{
				out->has_heightmap = false;
				out->heightMap = out->image;
			}
			else
				out->has_heightmap = true;
		}
		else
		{
			out->has_heightmap = false;
			out->heightMap = out->image;
		}
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
			out->numframes++;
	}
	
	//find equivalent texinfos
	num_unique = 0;
	unique_temp = Z_Malloc (sizeof(mtexinfo_t*)*count);
	for (i = 0; i < count; i++)
	{
		for (j = 0; j < i; j++)
		{
			if (in_base[i].flags == in_base[j].flags && 
				in_base[i].nexttexinfo == in_base[j].nexttexinfo && 
				!strcmp (in_base[i].texture, in_base[j].texture))
			{
				loadmodel->texinfo[i].equiv = loadmodel->texinfo[j].equiv;
				break;
			}
		}
		if (j == i)
		{
			unique_temp[num_unique++] = loadmodel->texinfo[i].equiv;
		}
	}
	
	qsort (unique_temp, num_unique, sizeof(mtexinfo_t*), compare_unique_texinfo);
	
	loadmodel->unique_texinfo = Hunk_Alloc (num_unique*sizeof(mtexinfo_t*));
	memcpy (loadmodel->unique_texinfo, unique_temp, num_unique*sizeof(mtexinfo_t*));
	Z_Free (unique_temp);
	loadmodel->num_unique_texinfos = num_unique;
	
	Com_Printf ("Condensed ^2%i texinfos into ^2%i equivalent texinfos\n", count, num_unique);
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (msurface_t *s, qboolean override, int *smax, int *tmax, float *xscale, float *yscale, int firstedge, int numedges)
{
	float	mins[2], maxs[2], val;
	int		i,j, e, vnum;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int64_t bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	if (firstedge < 0 || firstedge+numedges-1 >= loadmodel->numsurfedges)
		Com_Error (ERR_DROP,
			"Map contains invalid value for s->firstedge!\n"
			"The file is likely corrupted, please obtain a fresh copy.");
	for (i=0 ; i<numedges ; i++)
	{
		e = loadmodel->surfedges[firstedge+i];
		if (abs(e) > loadmodel->numedges)
			Com_Error (ERR_DROP,	
				"Map contains invalid edge offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		if (e >= 0)
			vnum = loadmodel->edges[e].v[0];
		else
			vnum = loadmodel->edges[-e].v[1];
		if (vnum < 0 || vnum >= loadmodel->numvertexes)
			Com_Error (ERR_DROP,	
				"Map contains invalid vertex offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		v = &loadmodel->vertexes[vnum];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	if (!override)
	{
#define DEFAULT_LIGHTMAP_SCALE 16 //each lightmap pixel represents 16x16 units
	    *xscale = *yscale = DEFAULT_LIGHTMAP_SCALE;
		for (i=0 ; i<2 ; i++)
	    {
		    bmins[i] = floor(mins[i]/DEFAULT_LIGHTMAP_SCALE);
		    bmaxs[i] = ceil(maxs[i]/DEFAULT_LIGHTMAP_SCALE);

		    s->texturemins[i] = bmins[i] * DEFAULT_LIGHTMAP_SCALE;
		    s->extents[i] = (bmaxs[i] - bmins[i]) * DEFAULT_LIGHTMAP_SCALE;
	    }
	    *smax = (s->extents[0]/DEFAULT_LIGHTMAP_SCALE)+1;
        *tmax = (s->extents[1]/DEFAULT_LIGHTMAP_SCALE)+1;
    }
    else
    {
	    bmins[0] = floor(mins[0]/(*xscale));
	    bmaxs[0] = ceil(maxs[0]/(*xscale));
	    s->texturemins[0] = bmins[0] * (*xscale);
	    s->extents[0] = (bmaxs[0] - bmins[0]) * (*xscale);
	    
	    bmins[1] = floor(mins[1]/(*yscale));
	    bmaxs[1] = ceil(maxs[1]/(*yscale));
	    s->texturemins[1] = bmins[1] * (*yscale);
	    s->extents[1] = (bmaxs[1] - bmins[1]) * (*yscale);
    }
}

void Mod_CalcSurfaceNormals(msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	float	*v;
	int		i;
	vec3_t	v01, temp;
	vec3_t	normal;
	float	s = 0;
	float	*vec;
	
	VectorSubtract (p->verts[1], p->verts[0], v01);
	vec = p->verts[0];
	VectorCopy (surf->plane->normal, normal);

	// FIXME: on some maps, this code doesn't always initialize v02, 
	// leading to Valgrind complaining about use of uninitialized memory.
	// dm-infinity and dm-zorn2k11 are two such maps. Probably not a huge
	// deal, doesn't seem to be causing any issues.
	for (v = p->verts[2], i = 2; i < p->numverts; i++, v += VERTEXSIZE)
	{
		// pick furthest vert we can find from vert 0, for greater numerical
		// precision
		float currentLength;
		vec3_t currentNormal;

		VectorSubtract (p->verts[i], p->verts[0], temp);

		CrossProduct (temp, v01, currentNormal);
		currentLength = VectorLength (currentNormal);

		if (currentLength > s)
		{
			s = currentLength;
			VectorCopy (currentNormal, normal);

			vec = p->verts[i];
		}
	}

	VectorNormalize (normal); //we have the largest normal
	VectorCopy (normal, surf->normal);

	// now get the tangent
	R_CalcTangent (p->verts[0], p->verts[1], vec, &p->verts[0][3], &p->verts[1][3], &vec[3], normal, surf->tangent);
}

void BSP_BuildPolygonFromSurface(msurface_t *fa, float xscale, float yscale, int light_s, int light_t, int firstedge, int lnumverts);
void BSP_CreateSurfaceLightmap (msurface_t *surf, int smax, int tmax, int *light_s, int *light_t);
void BSP_EndBuildingLightmaps (void);
void BSP_BeginBuildingLightmaps (model_t *m);


// FOR REFINE: .lightmap high-detail lightmap override files

ltmp_facelookup_t   lfacelookups[MAX_MAP_FACES];
int					lightdatasize;
byte				override_lightdata[MAX_OVERRIDE_LIGHTING];
byte				*lightmap_header;

// choose up to one override for each map face
void Mod_LoadRefineFaceLookups (lump_t *l)
{
	int i, count;
	ltmp_facelookup_t *in;
	ltmp_facelookup_t *out;
	
	if (!l->filelen)
		return;
	in = (void *)(lightmap_header+l->fileofs);
	printf ("%d / %d\n", l->filelen, sizeof(*in));
	if (l->filelen % sizeof (*in))
		Com_Error (ERR_DROP, "Mod_LoadRefineFaceLookups: funny lump size");
	count = l->filelen / sizeof (*in);
	
	Com_Printf ("%f\n", in->xscale);
	for (i = 0; i < count; i++, in++)
	{
		int facenum, format, offset, size, width, height;

		// Silently skip over pixel formats we don't understand. We do this
		// check first so that in the future it can be used to work around the
		// following two for backward compatibility.
		format = LittleLong(in->format);
		if (format >= LTMP_NUM_SUPPORTED_PIXFMTS)
			continue;
		
		// make a federal case out of bad face offsets
		facenum = LittleLong(in->facenum);
		if (facenum >= MAX_MAP_FACES || facenum < 0)
			Com_Error (ERR_DROP, "Mod_LoadRefineFaceeLookups: bad facenum");

		// make a federal case out of bad pixel data offsets
		offset = LittleLong(in->offset);
		width = LittleLong(in->width);
		height = LittleLong(in->height);
		switch (format)
		{
			case LTMP_PIXFMT_RGB24:
				size = 3*width*height;
				break;
		}
		if (offset < 1 || offset+size > MAX_OVERRIDE_LIGHTING*4)
			Com_Error (ERR_DROP, "Mod_LoadRefineFaceeLookups: bad offset");

		out = &lfacelookups[facenum];
		out->offset = offset;
		out->format = format;
		out->width = width;
		out->height = height;
		out->xscale = LittleFloat(in->xscale);
		out->yscale = LittleFloat(in->yscale);
	}
}

// load only the lightmap data that will actually be used; readjust the 
// pixel data offsets accordingly
void Mod_LoadRefineLighting (lump_t *l)
{
	int					i;
	byte				*in_buffer;
	qboolean			overflowed = false;
	
	if (!l->filelen)
		return;
	if (l->filelen > MAX_OVERRIDE_LIGHTING*4)
		Com_Error (ERR_DROP, "Mod_LoadRefineLighting: too much light data");
	
	lightdatasize = 0;
	in_buffer = lightmap_header + l->fileofs;
	for (i = 0; i < loadmodel->numsurfaces; i++)
	{
		int in_offset = lfacelookups[i].offset;
		if (in_offset)
		{
			int nstyles, size = 0;
			switch (lfacelookups[i].format)
			{
				case LTMP_PIXFMT_RGB24:
					size = 3*lfacelookups[i].width*lfacelookups[i].height;
					break;
			}
			// To Do - disable this block until it is fixed.
			/*for (nstyles = 0; nstyles < MAXLIGHTMAPS && loadmodel->surfaces[i].styles[nstyles] != 255; nstyles++);
			size *= nstyles;
			if (lightdatasize + size >= MAX_OVERRIDE_LIGHTING)
			{
				overflowed = true;
				break;
			}*/
			memcpy (override_lightdata+lightdatasize, in_buffer+in_offset-1, size);
			lfacelookups[i].offset = lightdatasize+1;
			lightdatasize += size;
		}
	}
	
	if (overflowed)
	{
		Com_Printf ("Mod_LoadRefineLighting: MAX_OVERRIDE_LIGHTING overflow!\n");
		Com_Printf ("Disabling HD lightmaps for this map.\n");
		memset (lfacelookups, 0, sizeof(lfacelookups));
	}
}

void Mod_LoadRefineLightmap (char *bsp_name)
{
	byte 				*buf, *uncompressed_buf;
	lightmapheader_t	header;
	int					length;
	char				name[MAX_OSPATH];
	char				*extension;
	int					lightmap_file_lump_order[LTMP_LUMPS] = {
		LTMP_LUMP_FACELOOKUP, LTMP_LUMP_LIGHTING
	};
	
	sizebuf_t			in, out;

#ifndef HAVE_ZLIB
	Com_Printf ("Zlib support must be enabled to use HD lightmaps!\n");
	Com_Printf ("Please recompile with Zlib support.\n");
	return;
#endif
	
	strncpy (name, bsp_name, MAX_OSPATH-1-strlen(".lightmap")+strlen(".bsp"));
	extension = strstr (name, ".bsp");
	if (extension)
		*extension = 0;
	strcat (name, ".lightmap"); 
	
	length = FS_LoadFile (name, (void **)&buf);
	if (!buf)
	{
		Com_Printf ("Could not load %s\n", name);
		return;
	}
	else
		Com_Printf ("Loaded %s\n", name);
	
	SZ_Init (&in, buf, length);
	in.cursize = length;
	uncompressed_buf = malloc (LTMP_MAX_UNCOMPRESSED_DATA);
	if (!uncompressed_buf)
	{
		Com_Printf ("Mod_LoadRefineLightmap: unable to allocate %d bytes!\n", LTMP_MAX_UNCOMPRESSED_DATA);
		return;
	}
	SZ_Init (&out, (byte *)uncompressed_buf, LTMP_MAX_UNCOMPRESSED_DATA);
	qdecompress (&in, &out, compression_zlib_header);
	FS_FreeFile (buf);
	
	if (!out.cursize)
	{
		Com_Printf ("Mod_LoadRefineLightmap: unable to decompress data in %s!\n",name);
		free (uncompressed_buf);
		return;
	}
	
	lightmap_header = (byte *)uncompressed_buf;
	header = *(lightmapheader_t *)uncompressed_buf;
	
	if (header.ident != IDLIGHTMAPHEADER)
	{
		Com_Printf ("Mod_LoadRefineLightmap: invalid magic number in %s!\n"
					"The file is likely corrupt, please obtain a fresh copy.\n", name);
		free (uncompressed_buf);			
		return;
	}
	if (header.version != LTMPVERSION)
	{
		Com_Printf ("Mod_LoadRefineLightmap: invalid major version number in %s!\n"
					"Version %d of the format is not supported. Current major version number is %d\n", 
					name, header.version, LTMPVERSION);
		free (uncompressed_buf);
		return;
	}
	
	if (checkLumps (header.lumps, sizeof(int)*3, lightmap_file_lump_order, uncompressed_buf, LTMP_LUMPS, out.cursize))
	{
		Com_Printf ("Mod_LoadRefineLightmap: lumps in %s don't add up right!\n"
					"The file is likely corrupt, please obtain a fresh copy.\n",name);
		free (uncompressed_buf);
		return;
	}
	
	Mod_LoadRefineFaceLookups (&header.lumps[LTMP_LUMP_FACELOOKUP]);
	Mod_LoadRefineLighting (&header.lumps[LTMP_LUMP_LIGHTING]);
	
	free (uncompressed_buf);
}

/*
=================
Mod_LoadFaces
=================
*/

// We need access to the lighting lump so we can check the bounds of lightmap
// offsets. Otherwise, crafted or corrupted BSPs could crash the client.
void Mod_LoadFaces (lump_t *l, lump_t *lighting)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;
	int			smax, tmax;
	int			light_s, light_t;
	float		xscale, yscale;
	rscript_t	*rs;
	vec3_t		color;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;
	
	currentmodel = loadmodel;
	
	memset (lfacelookups, 0, sizeof(lfacelookups));
	if (r_lightmapfiles->integer)
		Mod_LoadRefineLightmap (loadmodel->name);

	BSP_BeginBuildingLightmaps (loadmodel);

	VB_WorldVCInit();

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		int firstedge = LittleLong(in->firstedge);
		int numedges = LittleShort(in->numedges);
		out->iflags = 0;
		out->batch_flags = 0;
		out->polys = NULL;

		planenum = (unsigned short)LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->iflags |= ISURF_PLANEBACK;

		if (planenum >= loadmodel->numplanes)
			Com_Error (ERR_DROP, 
				"Map has invalid plane offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		out->plane = loadmodel->planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			Com_Error (ERR_DROP, 
				"Map has invalid texinfo offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		out->texinfo = loadmodel->texinfo + ti;

		if (lfacelookups[surfnum].offset)
		{
			smax=lfacelookups[surfnum].width;
			tmax=lfacelookups[surfnum].height;
			xscale=lfacelookups[surfnum].xscale; 
			yscale=lfacelookups[surfnum].yscale;
			CalcSurfaceExtents (out, true, &smax, &tmax, &xscale, &yscale, firstedge, numedges);
		}
		else
		{
			CalcSurfaceExtents (out, false, &smax, &tmax, &xscale, &yscale, firstedge, numedges);
		}

	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);

		if (i < 0 || i >= lighting->filelen)
			out->samples = NULL;
		else if (lfacelookups[surfnum].offset > 0 && lfacelookups[surfnum].offset <= lightdatasize)
			out->samples = override_lightdata + lfacelookups[surfnum].offset-1;
		else
			out->samples = loadmodel->lightdata + i;
		
		// set the drawing flags
		if (out->texinfo->flags & SURF_WARP)
		{
			out->iflags |= ISURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			if(!strcmp(out->texinfo->normalMap->name, out->texinfo->image->name))
				R_SubdivideSurface (out, firstedge, numedges);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		light_s = light_t = 0;
		if ( !SurfaceHasNoLightmap(out) )
		{
			BSP_CreateSurfaceLightmap (out, smax, tmax, &light_s, &light_t);
		}

		if ( (! (out->texinfo->flags & SURF_WARP)) || (strcmp(out->texinfo->normalMap->name, out->texinfo->image->name)))
			BSP_BuildPolygonFromSurface(out, xscale, yscale, light_s, light_t, firstedge, numedges);

		rs = (rscript_t *)out->texinfo->image->script;

		if(rs)	{

			rs_stage_t	*stage = rs->stage;
			do {
				if (r_lensflare->integer && stage->lensflare)
					Mod_AddFlareSurface(out, stage->flaretype);
				if (stage->grass && stage->texture) {
					if(stage->colormap.enabled) {
						color[0] = stage->colormap.red;
						color[1] = stage->colormap.green;
						color[2] = stage->colormap.blue;
					}
					Mod_AddVegetationSurface(out, stage->texture, color, stage->scale.scaleX, stage->texture->bare_name, stage->grasstype);
				}
				if (stage->beam && stage->texture) {
					if(stage->colormap.enabled) {
						color[0] = stage->colormap.red;
						color[1] = stage->colormap.green;
						color[2] = stage->colormap.blue;
					}
					Mod_AddBeamSurface(out, stage->texture->texnum, color, stage->scale.scaleX, stage->texture->bare_name, stage->beamtype,
						stage->xang, stage->yang, stage->rotating);
				}
				if (stage->cube)
					out->texinfo->flags |= SURF_SHINY;
			} while ( (stage = stage->next) );
		}
		Mod_CalcSurfaceNormals(out);
	}
	BSP_EndBuildingLightmaps ();
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		if (p < 0 || p >= loadmodel->numplanes)
			Com_Error (ERR_DROP,
				"Map has invalid plane offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		out->plane = loadmodel->planes + p;

		out->firstsurface = (unsigned short)LittleShort (in->firstface);
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
			{
				if (p >= loadmodel->numnodes)
					Com_Error (ERR_DROP, 
						"Map file has invalid node offsets!\n"
						"The file is likely corrupted, please obtain a fresh copy.");
				out->children[j] = loadmodel->nodes + p;
			}
			else
			{
				if (-1-p >= loadmodel->numleafs)
					Com_Error (ERR_DROP,
						"Map file has invalid leaf offsets!\n"
						"The file is likely corrupted, please obtain a fresh copy.");
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}

	}

	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;
	int			to_subtract = 0; //for removing SURF_NODRAW surfaces

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->cluster = LittleShort(in->cluster);
		out->area = LittleShort(in->area);

		out->firstmarksurface = loadmodel->marksurfaces +
			(unsigned short)(LittleShort(in->firstleafface));
		out->nummarksurfaces = (unsigned short)LittleShort(in->numleaffaces);
		if (out->firstmarksurface < loadmodel->marksurfaces || 
			out->nummarksurfaces < 0 ||
			(unsigned short)LittleShort(in->firstleafface) > 
			loadmodel->nummarksurfaces)
			Com_Error (ERR_DROP,
				"Map file has invalid leaf surface offsets!\n"
				"The file is likely corrupted, please obtain a fresh copy.");
		
		out->firstmarksurface -= to_subtract;
		
		// Remove SURF_NODRAW surfaces. We do this here instead of in 
		// Mod_LoadMarkSurfaces so we can correct the offsets in each leaf.
		for (j = 0; j < out->nummarksurfaces; j++)
		{
			msurface_t *s;
			out->firstmarksurface[j] = out->firstmarksurface[j+to_subtract];
			s = out->firstmarksurface[j];
			if (s->texinfo->flags & SURF_NODRAW)
			{
				to_subtract++;
				j--;
				out->nummarksurfaces--;
			}
		}

		// gl underwater warp
		for (j=0 ; j<out->nummarksurfaces ; j++)
		{
			msurface_t *s = out->firstmarksurface[j];
			if	(	(out->contents & MASK_WATER) || 
					(s->texinfo->flags & SURF_UNDERWATER))
			out->firstmarksurface[j]->iflags |= ISURF_UNDERWATER;
		}
	}
	
	loadmodel->nummarksurfaces -= to_subtract;
	
	Com_Printf ("Eliminated ^2%i invisible surfaces.\n", to_subtract);
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (lump_t *l)
{
	int					i, count;
	unsigned short		*in;
	msurface_t			**out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		unsigned short j = LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Com_Error (ERR_DROP, "Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (lump_t *l)
{
	int		i, count;
	int		*in, *out;


	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_MAP_SURFEDGES)
		Com_Error (ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i",
		loadmodel->name, count);

	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
Mod_SummarizePVS

Gather some statistics from the PVS data which allows the renderer to take 
some shortcuts.
=================
*/
void Mod_SummarizePVS (void)
{
	byte		*vis;
	mleaf_t		*leaf, *leaf2; 
	int			cluster;
	int			i;
	
	loadmodel->num_areas = 0;
	for (i = 0; i < MAX_MAP_AREAS; i++)
	{
		loadmodel->area_min_leaf[i] = loadmodel->numleafs-1;
		loadmodel->area_max_leaf[i] = 0;
	}
	
	for (i=0,leaf=loadmodel->leafs ; i<loadmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		
		vis = Mod_ClusterPVS (cluster, loadmodel);
		
		if (loadmodel->area_min_leaf[leaf->area] > i)
			loadmodel->area_min_leaf[leaf->area] = i;
		if (loadmodel->area_max_leaf[leaf->area] < i)
			loadmodel->area_max_leaf[leaf->area] = i;
		if (leaf->area > loadmodel->num_areas)
			loadmodel->num_areas = leaf->area;
		
		// Two separate loops, one for minPVSleaf and one for maxPVSleaf,
		// each coming from opposite directions, for greater efficiency.
		
		for (	leaf->minPVSleaf = 0, leaf2 = loadmodel->leafs;
				leaf->minPVSleaf < loadmodel->numleafs;
				leaf->minPVSleaf++, leaf2++
			)
		{
			cluster = leaf2->cluster;
			if ((leaf2->contents & CONTENTS_SOLID) || cluster == -1)
				continue;
			if (leaf2->nummarksurfaces == 0)
				continue;
			
			if (vis[cluster>>3] & (1<<(cluster&7)))
				break;
		}
		
		for (	leaf->maxPVSleaf = loadmodel->numleafs-1, leaf2 = &loadmodel->leafs[leaf->maxPVSleaf];
				leaf->maxPVSleaf >= leaf->minPVSleaf;
				leaf->maxPVSleaf--, leaf2--
			)
		{
			cluster = leaf2->cluster;
			if ((leaf2->contents & CONTENTS_SOLID) || cluster == -1)
				continue;
			if (leaf2->nummarksurfaces == 0)
				continue;
			
			if (vis[cluster>>3] & (1<<(cluster&7)))
				break;
		}
	}
	loadmodel->num_areas++;
}

void R_ClearTerrains (void)
{
	model_t	*mod;
	int i;

	for (i = 0, mod = mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->type == mod_terrain)
		{	
			Mod_Free (mod);
		}
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
extern cvar_t *scriptsloaded;
void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i;
	dheader_t	*header;
	mmodel_t 	*bm;
	char		rs_name[MAX_OSPATH], tmp[MAX_QPATH];		// rscript - MrG
	const int 	maxsize = // adds up to about 31 MiB
		sizeof (mvertex_t) * MAX_MAP_VERTS + 
		sizeof (medge_t) * MAX_MAP_EDGES +
		sizeof (int) * MAX_MAP_SURFEDGES + 
		MAX_MAP_LIGHTING +
		sizeof (cplane_t) * MAX_MAP_PLANES + 
		sizeof (mtexinfo_t) * MAX_MAP_TEXINFO +
		sizeof (mtexinfo_t *) * MAX_MAP_TEXINFO + // unique texinfo
		sizeof (msurface_t) * MAX_MAP_FACES +
		sizeof (msurface_t *) * MAX_MAP_LEAFFACES +
		MAX_MAP_VISIBILITY +
		sizeof (mleaf_t) * MAX_MAP_LEAFS +
		sizeof (mnode_t) * MAX_MAP_NODES + 
		sizeof (mmodel_t) * MAX_MAP_MODELS +
		32 * 16; // some extra for the padding added by Hunk_Alloc on win32
	
	mod->extradata = Hunk_Begin (maxsize);

	if (r_lensflare->integer)
		R_ClearFlares();
	R_ClearGrasses();
	R_ClearBeams();

	RS_FreeUnmarked();
	strcpy(tmp,loadmodel->name+5);
	tmp[strlen(tmp)-4]=0;
	Com_sprintf(rs_name,MAX_OSPATH,"scripts/maps/%s.rscript",tmp);
	RS_ScanPathForScripts();		// load all found scripts
	RS_LoadScript(rs_name);
	RS_ReloadImageScriptLinks();
	RS_LoadSpecialScripts();
	Cvar_SetValue("scriptsloaded", 1);

	//ODE - clear out any ragdolls;
	R_ClearAllRagdolls();

	//ODE - create new world(flush out old first)
	RGD_DestroyWorldObject();
	RGD_CreateWorldObject();

	// Terrain - flush out 
	R_ClearTerrains();

	r_numWorldLights = 0;

	loadmodel->type = mod_brush;
	loadmodel->typeFlags = MESH_CASTSHADOWMAP | MESH_DOSHADING; // XXX: does nothing yet for mod_brush
	if (loadmodel != mod_known)
		Com_Error (ERR_DROP, "Loaded a brush model after the world");

	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSPVERSION)
		Com_Error (ERR_DROP, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

	// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/sizeof(int) ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	// load into heap
	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES], &header->lumps[LUMP_LIGHTING]);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[LUMP_NODES]);
	Mod_SummarizePVS ();
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);
	mod->num_frames = 2;		// regular and alternate animation

	//
	// set up the submodels
	//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		model_t	*starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= loadmodel->numnodes)
			Com_Error (ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
	
	mod->extradatasize = Hunk_End ();
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
int R_FindFile (char *filename, FILE **file)
{

	*file = fopen (filename, "rb");
	if (!*file) {
		*file = NULL;
		return -1;
	}
	else
		return 1;

}

//code to precache all base player models and their w_weapons(note - these are specific to Alien Arena and can be changed for other games)
PModelList_t BasePModels[] =
{
	{ "martianenforcer" },
	{ "martiancyborg" },
	{ "martianoverlord" },
	{ "martianwarrior" },
	{ "enforcer" },
	{ "commander" },
	{ "femborg" },
	{ "slashbot" },
	{ "rustbot" }
};
int PModelsCount = (int)(sizeof(BasePModels)/sizeof(BasePModels[0]));

WModelList_t BaseWModels[] =
{
	{ "w_blaster.iqm" },
	{ "w_smartgun.iqm" },
	{ "w_chaingun.iqm" },
	{ "w_grapple.iqm" },
	{ "w_flamethrower.iqm" },
	{ "w_rlauncher.iqm" },
	{ "w_disruptor.iqm" },
	{ "w_beamgun.iqm" },
	{ "w_vaporizer.iqm" },
	{ "w_violator.iqm" },
	{ "w_minderaser.iqm" },
	{ "weapon.iqm" }
};
int WModelsCount = (int)(sizeof(BaseWModels)/sizeof(BaseWModels[0]));

void R_RegisterBasePlayerModels( void )
{
	char	mod_filename[MAX_QPATH];
	char	scratch[MAX_QPATH];
	int i, j;
	// int npms = 0; // unused
	int nskins = 0;
	char **skinnames;
	
	if (is_localhost) {
	    //then the client is also the server so we have this cvar available
	    cvar_t *maxclients = Cvar_Get ("maxclients", 0, 0);
	    if (!maxclients|| maxclients->integer <= 1)
	        return; //don't bother
	}

	//precache all player and weapon models(base only, otherwise could take very long loading a map!)
	for (i = 0; i < PModelsCount; i++)
	{
		Com_Printf("Registering models for: %s\n", BasePModels[i].name);
		Com_sprintf( mod_filename, sizeof(mod_filename), "players/%s/tris.iqm", BasePModels[i].name);
		R_RegisterModel(mod_filename);
		Com_sprintf( mod_filename, sizeof(mod_filename), "players/%s/lod1.iqm", BasePModels[i].name);
		R_RegisterModel(mod_filename);
		Com_sprintf( mod_filename, sizeof(mod_filename), "players/%s/lod2.iqm", BasePModels[i].name);
		R_RegisterModel(mod_filename);

		//register weapon models
		for (j = 0; j < WModelsCount; j++)
		{
			Com_sprintf( mod_filename, sizeof(mod_filename), "players/%s/%s", BasePModels[i].name, BaseWModels[j]);
			R_RegisterModel(mod_filename);
		}
		
		//register standard sounds
		S_RegisterSoundsForPlayer (BasePModels[i].name);

		//register all skins
		Com_sprintf( scratch, sizeof(scratch), "players/%s/*.jpg", BasePModels[i].name);
		skinnames = FS_ListFilesInFS( scratch, &nskins, 0,
		    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if(!skinnames)
			continue;

		for(j = 0; j < nskins; j++)
			R_RegisterSkin (skinnames[j]);

		if(skinnames)
			free(skinnames);
	}
}

void R_RegisterCustomPlayerModels( void )
{
	char	mod_filename[MAX_QPATH];
	char	scratch[MAX_QPATH];
	int i, j;
	int npms = 0;
	int nskins = 0;
	char **dirnames;
	char **skinnames;

	dirnames = FS_ListFilesInFS( "players/*.*", &npms, SFF_SUBDIR, 0 );

	if ( !dirnames )
		return;

	if ( npms > 1024 )
		npms = 1024;

	for(i = 0; i < npms; i++)
	{
		if ( dirnames[i] == 0 )
			continue;

		Com_Printf("Registering custom player model: %s\n", dirnames[i]);
		Com_sprintf( mod_filename, sizeof(mod_filename), "%s/tris.iqm", dirnames[i]);
		if(FS_FileExists(mod_filename))
			R_RegisterModel(mod_filename);
		else
			continue; //invalid player model
		Com_sprintf( mod_filename, sizeof(mod_filename), "%s/lod1.iqm", dirnames[i]);
		if(FS_FileExists(mod_filename))
			R_RegisterModel(mod_filename);
		Com_sprintf( mod_filename, sizeof(mod_filename), "%s/lod2.iqm", dirnames[i]);
		if(FS_FileExists(mod_filename))
			R_RegisterModel(mod_filename);

		//register weapon models
		for (j = 0; j < WModelsCount; j++)
		{
			Com_sprintf( mod_filename, sizeof(mod_filename), "%s/%s", dirnames[i], BaseWModels[j]);
			if(FS_FileExists(mod_filename))
				R_RegisterModel(mod_filename);
		}
		
		//register standard sounds
		S_RegisterSoundsForPlayer (dirnames[i]);

		//register all skins
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/*.jpg" );
		skinnames = FS_ListFilesInFS( scratch, &nskins, 0,
		    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if(!skinnames) {
			// check for .tga, though this is no longer used for current models
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/*.tga" );
			skinnames = FS_ListFilesInFS( scratch, &nskins, 0,
				SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );
		}

		if(!skinnames)
			continue;

		for(j = 0; j < nskins; j++)
			R_RegisterSkin (skinnames[j]);

		if(skinnames)
			free(skinnames);

	}
	if(dirnames)
		free(dirnames);
}

void R_BeginRegistration (char *model)
{
	char	fullname[MAX_OSPATH];
 	char    *path;
	cvar_t	*flushmap;
	FILE	*file;
	int		i;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	r_weather = 0; //default is 0
	r_nosun = 0;

	// check for fog file, using file system search path
	path = NULL;
	map_fog = false;
	r_weather = false;
	r_nosun = false;
	r_sunX = 0;
	r_sunY = 0;
	r_sunZ = 0;
	r_skyangleX = 0;
	r_skyangleY = 0;
	r_skyangleZ = 0;
	r_sunIntens = 1.0;
	for(;;)
	{
		path = FS_NextPath( path );
		if( !path )
		{
			break;
		}
		Com_sprintf(fullname, sizeof(fullname), "%s/maps/scripts/%s.fog", path, model);
		i = 0;
		R_FindFile( fullname, &file ); //does a fog file exist?
		if(file) {
			//read the file, get fog information
			fclose(file);
			R_ReadFogScript(fullname);
			break;
		}
	}

	// check for background music file, , using file system search path
	//defaults below
	strcpy(map_music, "music/menumusic.ogg");
	strcpy(map_music_sec, "music/adrenaline.ogg");
	S_RegisterSound (map_music_sec);
	path = NULL;
	for(;;)
	{
		path = FS_NextPath( path );
		if( !path )
		{
			break;
		}
		Com_sprintf(fullname, sizeof(fullname), "%s/maps/scripts/%s.mus", path, model);
		i = 0;
		R_FindFile( fullname, &file ); //does a music file exist?
		if(file) 
		{
			//read the file, get music information
			fclose( file );
			R_ReadMusicScript( fullname );						
			break;
		}
	}
	//set ctf flags
	r_gotFlag = false;
	r_lostFlag = false;
	Cvar_Set("rs_hasflag", "0");

	Com_sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = Cvar_Get ("flushmap", "0", 0);

	if (strcmp(mod_known[0].name, fullname) || flushmap->integer)
		Mod_Free (&mod_known[0]);
	else
		Mod_Free (&mod_known[0]);	//do it every time to fix shader bugs in AA

	r_worldmodel = Mod_ForName(fullname, true);	
	
	memset (cl_persistent_ents, 0, sizeof(cl_persistent_ents));
	R_ParseLightEntities();
	R_RegisterLightGroups();
	R_ParseTerrainEntities();
	R_FindSunEntity();
	R_FinalizeGrass();
	memset (cl_persistent_ents, 0, sizeof(cl_persistent_ents));

	r_viewcluster = -1;

	r_teamColor = 0;

	//ODE
	RGD_BuildWorldTrimesh ();

	//VBO
	VB_BuildWorldVBO();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/

struct model_s *R_RegisterModel (char *name)
{
	model_t	*mod;
	int		i;

	dmdl_t		*pheader;

	mod = Mod_ForName (name, false);
	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == mod_md2)
		{
			pheader = (dmdl_t *)mod->extradata;
			for (i=0 ; i<pheader->num_skins ; i++)
				mod->skins[i] = GL_FindImage ((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
//PGM
			mod->num_frames = pheader->num_frames;
//PGM
		}
		else if (mod->type == mod_iqm)
		{
			mod->skins[0] = GL_FindImage (mod->skinname, it_skin);
		}
		else if (mod->type == mod_brush)
		{
			for (i=0 ; i<mod->numtexinfo ; i++) {
				mod->texinfo[i].image->registration_sequence = registration_sequence;
				mod->texinfo[i].normalMap->registration_sequence = registration_sequence;
			}
		}
	}
	return mod;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
	int		i;
	model_t	*mod;

	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free (mod);
		}
	}

	GL_FreeUnusedImages ();
}


//=============================================================================


/*
================
Mod_Free - should be able to handle every model type, including uninitialized
models.
================
*/
void R_Mesh_FreeVBO (model_t *mod);
void Mod_Free (model_t *mod)
{
	// mod_bad (uninitialized models memsetted to 0) will have nothing done to
	// them here
	if (mod->type > mod_brush)
		R_Mesh_FreeVBO (mod);
	
	if (mod->extradatasize)
		Hunk_Free (mod->extradata);
	
	memset (mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void)
{
	int		i;

	for (i=0 ; i<mod_numknown ; i++)
		Mod_Free (&mod_known[i]);
}




