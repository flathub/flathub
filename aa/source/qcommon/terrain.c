#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

#include "binheap.h"
#include "libgarland.h"

// This describes a single decoration type that has been parsed out of the
// .terrain file.
typedef struct decoration_channel_s
{
	struct decoration_channel_s	*next;
	int							type;
	// identify a texture channel for this decoration type
	int							channel_num;
	char						*texture_path;
	// list of texture or mesh paths
#define MAX_DECORATION_VARIANTS 64
	int							num_variants;
	char						*variant_paths[MAX_DECORATION_VARIANTS];
	float						gridsz; // average spacing for the decorations
	float						sizecoef; // scaling for each decoration
	// output fields, to be set by the loader
	int							out_num_decorations;
	terraindec_t				*out_decorations;
} decoration_channel_t;

// Create structures for the legacy hardcoded decoration types (TODO: remove
// before release?)
static const decoration_channel_t legacy_shrub_channel = 
{
	NULL,		// no next
	2,
	0,			// red channel indicates shrubbery
	NULL,		// must override texture path before using!
	2,			// 2 different shrub textures
	{
		"gfx/bush1.tga",
		"gfx/bush2.tga"
	},
	64.0f,
	1.0f/16.0f,
	0, NULL
};
static const decoration_channel_t legacy_grass_channel = 
{
	NULL,		// no next
	0,
	1,			// green channel indicates grass
	NULL,		// must override texture path before using!
	1,			// only 1 grass texture
	{
		"gfx/grass.tga"
	},
	64.0f,
	1.0f/16.0f,
	0, NULL
};
static const decoration_channel_t legacy_weed_channel = 
{
	NULL,		// no next
	0,
	2,			// blue channel indicates weeds
	NULL,		// must override texture path before using!
	4,			// 4 different weed textures
	{
		"gfx/weed1.tga",
		"gfx/weed2.tga",
		"gfx/weed3.tga",
		"gfx/weed4.tga"
	},
	64.0f,
	1.0f/16.0f,
	0, NULL
};
static const decoration_channel_t legacy_rocks_channel = 
{
	NULL,		// no next
	0,
	0,			// red channel indicates rocks
	NULL,		// must override texture path before using!
	5,			// 5 different rock meshes
	{
		"maps/meshes/rocks/rock1.md2",
		"maps/meshes/rocks/rock2.md2",
		"maps/meshes/rocks/rock3.md2",
		"maps/meshes/rocks/rock4.md2",
		"maps/meshes/rocks/rock5.md2"
	},
	64.0f,
	1.0,		// meshes can't yet be scaled anyway
	0, NULL
};


static float grayscale_sample (const byte *texture, int tex_w, int tex_h, float u, float v, float *out_alpha)
{
    vec4_t res;
    
    bilinear_sample (texture, tex_w, tex_h, u, v, res);
    
    if (out_alpha != NULL)
    	*out_alpha = res[3];
    
    return (res[0] + res[1] + res[2]) / 3.0;
}

static void LoadTerrainDecorationType
	(	decoration_channel_t *channel, const vec3_t mins, const vec3_t scale,
		const byte *hmtexdata, float hm_w, float hm_h
	)
{
	byte *texdata_orig = NULL;
	byte *texdata_resampled = NULL;
	terraindec_t *ret = NULL;
	int i, j, w, h, w_orig, h_orig;
	int counter = 0;
	
	LoadTGA (channel->texture_path, &texdata_orig, &w_orig, &h_orig);
	
	if (texdata_orig == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", channel->texture_path);
	
	Z_Free (channel->texture_path);
	
	// resample decorations texture to new resolution
	// this is to make decoration density independent of decoration texture
	w = ceilf (scale[0] / channel->gridsz);
	h = ceilf (scale[1] / channel->gridsz);
	texdata_resampled = Z_Malloc (w*h);
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			float s, t;
			vec4_t res;
			
			s = ((float)j)/(float)w;
			t = ((float)i)/(float)h;
			bilinear_sample (texdata_orig, w_orig, h_orig, s, t, res);
			
			texdata_resampled[i*w+j] = floorf (res[channel->channel_num] * 255.0f);
			
			// Count how many decorations we should allocate.
			counter += texdata_resampled[i*w+j] != 0;
		}
	}
	
	free (texdata_orig);
	
	channel->out_num_decorations = counter;
	
	if (counter == 0)
	{
		Z_Free (texdata_resampled);
		return;
	}
	
	ret = Z_Malloc (counter * sizeof(terraindec_t));
	
	counter = 0;
	
	// Fill in the decorations
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			float x, y, z, s, t, xrand, yrand;
			byte size;
		
			size = texdata_resampled[(h-i-1)*w+j];
			if (size == 0)
				continue;
	
			xrand = 2.0*(frand()-0.5);
			yrand = 2.0*(frand()-0.5);
	
			s = ((float)j+xrand)/(float)w;
			t = 1.0 - ((float)i+yrand)/(float)h;
	
			x = scale[0]*((float)j/(float)w) + mins[0] + scale[0]*xrand/(float)w;
			y = scale[1]*((float)i/(float)h) + mins[1] + scale[1]*yrand/(float)h;
			z = scale[2] * grayscale_sample (hmtexdata, hm_h, hm_w, s, t, NULL) + mins[2];
	
			VectorSet (ret[counter].origin, x, y, z);
			ret[counter].size = (float)size * channel->sizecoef;
			ret[counter].path = channel->variant_paths[rand () % channel->num_variants];
			ret[counter].type = channel->type;
	
			counter++;
		}
	}
	
	assert (counter == channel->out_num_decorations);
	
	Z_Free (texdata_resampled);
	
	channel->out_decorations = ret;
}

// TODO: separate function for decorations, currently this is kind of hacky.
#ifdef MESH_SIMPLIFICATION_VISUALIZER
static mesh_t mesh;
#endif
void LoadTerrainFile (terraindata_t *out, const char *name, qboolean decorations_only, float oversampling_factor, int reduction_amt, char *buf)
{
	int i, j, va, w, h, vtx_w, vtx_h;
	char	*vegtex_path = NULL, *rocktex_path = NULL;
	byte	*alphamask; // Bitmask of "holes" in terrain from alpha channel
#ifndef MESH_SIMPLIFICATION_VISUALIZER
	mesh_t	mesh;
#endif
	vec3_t	scale;
	const char *line;
	char	*token;
	byte	*texdata;
	int		start_time;
	const char *prev_decoration_texture = NULL;
	decoration_channel_t *decoration_channels = NULL, *c, *cnext;
	char	*variant_path_cursor;
	int		total_variant_path_len;
	terraindec_t *decoration_cursor;
	
	memset (out, 0, sizeof(*out));
	
	line = strtok (buf, ";");
	while (line)
	{
		token = COM_Parse (&line);
		if (!line && !(line = strtok (NULL, ";")))
			break;

#define FILENAME_ATTR(cmd_name,out) \
		if (!Q_strcasecmp (token, cmd_name)) \
		{ \
			out = CopyString (COM_Parse (&line)); \
			if (!line) \
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting " cmd_name " filename! (File %s is invalid)", name); \
		} 
		
		FILENAME_ATTR ("heightmap", out->hmtex_path)
		FILENAME_ATTR ("texture", out->texture_path)
		FILENAME_ATTR ("lightmap", out->lightmap_path)
		FILENAME_ATTR ("vegetation", vegtex_path)
		FILENAME_ATTR ("rocks", rocktex_path);
		
#undef FILENAME_ATTR
		
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				out->mins[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting mins %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				out->maxs[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting maxs %c axis! (File %s is invalid)", "xyz"[i], name);
			}
		}
		if (!Q_strcasecmp (token, "decoration"))
		{
			const char *cur_decoration_texture, *cur_variant_path, *prev_variant_path = NULL;
			decoration_channel_t *new_channel = Z_Malloc (sizeof (decoration_channel_t));
			new_channel->next = decoration_channels;
			decoration_channels = new_channel;
#define DECORATION_ATTR(attr_name,transformation) \
			new_channel->attr_name = transformation (COM_Parse (&line)); \
			if (!line) \
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting %s for decoration! (File %s is invalid)", #attr_name, name);
			cur_decoration_texture = COM_Parse (&line);
			if (!line)
				Com_Error (ERR_DROP, "LoadTerrainFile: EOL when expecting texture_path for decoration! (File %s is invalid)", name);
			if (!Q_strcasecmp (cur_decoration_texture, "DITTO"))
			{
				if (prev_decoration_texture != NULL)
					cur_decoration_texture = prev_decoration_texture;
				else
					Com_Error (ERR_DROP, "LoadTerrainFile: \"DITTO\" cannot be used as the first texture_path! (File %s is invalid)", name);
			}
			prev_decoration_texture = new_channel->texture_path = CopyString (cur_decoration_texture);
			DECORATION_ATTR (channel_num, atoi)
			DECORATION_ATTR (type, atoi)
			DECORATION_ATTR (gridsz, atof)
			DECORATION_ATTR (sizecoef, atof)
#undef DECORATION_ATTR
			new_channel->num_variants = 0;
			cur_variant_path = COM_Parse (&line);
			while (line && new_channel->num_variants < MAX_DECORATION_VARIANTS)
			{
				if (!Q_strcasecmp (cur_variant_path, "DITTO"))
				{
					if (prev_variant_path != NULL)
						cur_variant_path = prev_variant_path;
					else
						Com_Error (ERR_DROP, "LoadTerrainFile: \"DITTO\" cannot be used as the first variant path name! (File %s is invalid)", name);
				}
				prev_variant_path = new_channel->variant_paths[new_channel->num_variants++] = CopyString (cur_variant_path);
				cur_variant_path = COM_Parse (&line);
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		line = strtok (NULL, ";");
	}
	
	if (!out->hmtex_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	if (!out->texture_path)
		Com_Error (ERR_DROP, "LoadTerrainFile: Missing heightmap texture in %s!", name);
	
	VectorSubtract (out->maxs, out->mins, scale);
	
	LoadTGA (out->hmtex_path, &texdata, &w, &h);
	
	if (texdata == NULL)
		Com_Error (ERR_DROP, "LoadTerrainFile: Can't find file %s\n", out->hmtex_path);
	
	// preprocess decorations to move all variant paths to the same buffer for
	// easier freeing later
	total_variant_path_len = 0;
	for (c = decoration_channels; c != NULL; c = c->next)
	{
		for (i = 0; i < c->num_variants; i++)
			total_variant_path_len += strlen (c->variant_paths[i]) + 1;
	}
	variant_path_cursor = out->decoration_variant_paths = Z_Malloc (total_variant_path_len);
	for (c = decoration_channels; c != NULL; c = c->next)
	{
		for (i = 0; i < c->num_variants; i++)
		{
			int sz = strlen (c->variant_paths[i]) + 1;
			memcpy (variant_path_cursor, c->variant_paths[i], sz);
			Z_Free (c->variant_paths[i]);
			c->variant_paths[i] = variant_path_cursor;
			variant_path_cursor += sz;
		}
	}

	// legacy channels use static strings so don't need preprocessing.
#define ADD_LEGACY_CHANNEL(template,path) \
{ \
	decoration_channel_t *new_channel = Z_Malloc (sizeof (decoration_channel_t)); \
	memcpy (new_channel, &template, sizeof (decoration_channel_t)); \
	new_channel->texture_path = CopyString (path); \
	new_channel->next = decoration_channels; \
	decoration_channels = new_channel; \
}
	
	if (vegtex_path != NULL)
	{
		ADD_LEGACY_CHANNEL (legacy_shrub_channel, vegtex_path);
		ADD_LEGACY_CHANNEL (legacy_grass_channel, vegtex_path);
		ADD_LEGACY_CHANNEL (legacy_weed_channel, vegtex_path);
		Z_Free (vegtex_path);
	}
	
	if (rocktex_path != NULL)
	{
		ADD_LEGACY_CHANNEL (legacy_rocks_channel, rocktex_path);
		Z_Free (rocktex_path);
	}

#undef ADD_LEGACY_CHANNEL
	
	// compile a list of all decorations that should be added to the map for
	// each decoration type
	out->num_decorations = 0;
	for (c = decoration_channels; c != NULL; c = c->next)
	{
		LoadTerrainDecorationType (c, out->mins, scale, texdata, h, w);
		out->num_decorations += c->out_num_decorations;
	}
	// concatenate all decoration lists together
	decoration_cursor = out->decorations = Z_Malloc (out->num_decorations * sizeof (terraindec_t));
	for (c = decoration_channels; c != NULL; c = cnext)
	{
		if (c->out_num_decorations)
		{
			memcpy (decoration_cursor, c->out_decorations, c->out_num_decorations * sizeof (terraindec_t));
			Z_Free (c->out_decorations);
		}
		decoration_cursor += c->out_num_decorations;
		cnext = c->next;
		Z_Free (c);
	}
	
	if (decorations_only)
	{
		free (texdata);
		return;
	}
	
	vtx_w = oversampling_factor*w+1;
	vtx_h = oversampling_factor*h+1;
	
	alphamask = Z_Malloc (vtx_w * vtx_h / 8 + 1);
	
	out->num_vertices = vtx_w*vtx_h;
	out->num_triangles = 2*(vtx_w-1)*(vtx_h-1);
	
	out->vert_texcoords = Z_Malloc (out->num_vertices*sizeof(vec2_t));
	out->vert_positions = Z_Malloc (out->num_vertices*sizeof(vec3_t));
	out->tri_indices = Z_Malloc (out->num_triangles*3*sizeof(unsigned int));
	
	for (i = 0; i < vtx_h; i++)
	{
		for (j = 0; j < vtx_w; j++)
		{
			float x, y, z, s, t, alpha;
			
			s = (float)j/(float)vtx_w;
			t = 1.0 - (float)i/(float)vtx_h;
			
			x = scale[0]*((float)j/(float)vtx_w) + out->mins[0];
			y = scale[1]*((float)i/(float)vtx_h) + out->mins[1];
			z = scale[2] * grayscale_sample (texdata, h, w, s, t, &alpha) + out->mins[2];
			
			// If the alpha of the heightmap texture is less than 1.0, the 
			// terrain has a "hole" in it and any tris that would have
			// included this vertex should be skipped.
			alphamask[(i*vtx_w+j)/8] |= (alpha == 1.0) << ((i*vtx_w+j)%8);
			
			VectorSet (&out->vert_positions[(i*vtx_w+j)*3], x, y, z);
			out->vert_texcoords[(i*vtx_w+j)*2] = s;
			out->vert_texcoords[(i*vtx_w+j)*2+1] = t;
		}
	}
	
	va = 0;
	for (i = 0; i < vtx_h-1; i++)
	{
		for (j = 0; j < vtx_w-1; j++)
		{
// Yeah, this was actually the least embarrassing way to do it
#define IDX_NOT_HOLE(idx) ((alphamask[(idx) / 8] & (1 << ((idx) % 8))) != 0)
#define ADDTRIANGLE(idx1,idx2,idx3) \
			if (IDX_NOT_HOLE (idx1) && IDX_NOT_HOLE (idx2) && IDX_NOT_HOLE (idx3)) \
			{ \
				out->tri_indices[va++] = idx1; \
				out->tri_indices[va++] = idx2; \
				out->tri_indices[va++] = idx3; \
			}
			
			if ((i+j)%2 == 1)
			{
				ADDTRIANGLE ((i+1)*vtx_w+j, i*vtx_w+j+1, i*vtx_w+j);
				ADDTRIANGLE (i*vtx_w+j+1, (i+1)*vtx_w+j, (i+1)*vtx_w+j+1);
			}
			else
			{
				ADDTRIANGLE ((i+1)*vtx_w+j+1, i*vtx_w+j+1, i*vtx_w+j);
				ADDTRIANGLE (i*vtx_w+j, (i+1)*vtx_w+j, (i+1)*vtx_w+j+1);
			}
		}
	}
	
	assert (va % 3 == 0 && va / 3 <= out->num_triangles);
	
	free (texdata);
	
	Z_Free (alphamask);
	
	mesh.num_verts = out->num_vertices;
	mesh.num_tris = va / 3;
	mesh.vcoords = out->vert_positions;
	mesh.vtexcoords = out->vert_texcoords;
	mesh.tris = out->tri_indices;
	
#ifndef MESH_SIMPLIFICATION_VISUALIZER
	start_time = Sys_Milliseconds ();
	simplify_mesh (&mesh, out->num_triangles/reduction_amt);
	Com_Printf ("Simplified mesh in %f seconds.\n", (float)(Sys_Milliseconds () - start_time)/1000.0f);
	Com_Printf ("%d to %d\n", out->num_triangles, mesh.num_tris);
	
	out->num_vertices = mesh.num_verts;
	out->num_triangles = mesh.num_tris;
#else
    simplify_init (&mesh);
#endif
}

#ifdef MESH_SIMPLIFICATION_VISUALIZER
void visualizer_step (terraindata_t *out, qboolean export)
{
    simplify_step (&mesh, out->num_triangles - 1);
    if (export)
        export_mesh (&mesh);
    out->num_vertices = mesh.num_verts;
	out->num_triangles = mesh.simplified_num_tris;
}
#endif

// writes out entire terraindata_t struct to file
void WriteTerrainData (terraindata_t *in, const char *name, int forRender)
{
	FILE* file;
	char collisionFile[MAX_QPATH];
	int		i, pathLen;
	size_t sz;

	if(forRender)
		sprintf(collisionFile, "%s/%sr", BASE_GAMEDATA, name);
	else
		sprintf(collisionFile, "%s/%sx", BASE_GAMEDATA, name);

    file = fopen(collisionFile, "wb");
    if (file != NULL) 
	{
		in->num_vertices = LittleLong(in->num_vertices);
		sz = fwrite(&in->num_vertices,sizeof(int), 1, file); 
		for(i = 0; i < in->num_vertices*2; i++)
		{
			in->vert_texcoords[i] = LittleFloat(in->vert_texcoords[i]);
			sz = fwrite(&in->vert_texcoords[i],sizeof(float), 1, file); 
		}

		for(i = 0; i < in->num_vertices*3; i++)
		{
			in->vert_positions[i] = LittleFloat(in->vert_positions[i]);
			sz = fwrite(&in->vert_positions[i],sizeof(float), 1, file); 
		}		
	
		in->num_triangles = LittleLong(in->num_triangles);
		sz = fwrite(&in->num_triangles,sizeof(int), 1, file); 
		for(i = 0; i < in->num_triangles*3; i++)
		{
			in->tri_indices[i] = LittleLong(in->tri_indices[i]);
			sz = fwrite(&in->tri_indices[i],sizeof(unsigned int), 1, file); 
		}

		for(i = 0; i < 3; i++)
		{
			in->maxs[i] = LittleFloat(in->maxs[i]);
			sz = fwrite(&in->maxs[i], sizeof(float), 1, file);
			in->mins[i] = LittleFloat(in->mins[i]);
			sz = fwrite(&in->mins[i], sizeof(float), 1, file);
		}

		if(forRender)
		{
			//Use 128(Windows MAX_OSPATH) for string length
			sz = fwrite (in->hmtex_path, sizeof(char) * 128, 1, file);
			sz = fwrite (in->texture_path, sizeof(char) * 128, 1, file);
			sz = fwrite (in->lightmap_path, sizeof(char) * 128, 1, file);

			pathLen = strlen(in->decoration_variant_paths);
			pathLen = LittleLong(pathLen);
			sz = fwrite (&pathLen, sizeof(int), 1, file);
			sz = fwrite (in->decoration_variant_paths, sizeof(char) * pathLen, 1, file);

			sz = fwrite (&in->num_decorations, sizeof(int), 1, file);
			for(i = 0; i < in->num_decorations; i++)
			{
				//origin
				in->decorations[i].origin[0] = LittleFloat(in->decorations[i].origin[0]);
				sz = fwrite (&in->decorations[i].origin[0], sizeof(float), 1, file);
				in->decorations[i].origin[1] = LittleFloat(in->decorations[i].origin[1]);
				sz = fwrite (&in->decorations[i].origin[1], sizeof(float), 1, file);
				in->decorations[i].origin[2] = LittleFloat(in->decorations[i].origin[2]);
				sz = fwrite (&in->decorations[i].origin[2], sizeof(float), 1, file);

				in->decorations[i].size = LittleFloat(in->decorations[i].size);
				sz = fwrite (&in->decorations[i].size, sizeof(float), 1, file);
				in->decorations[i].type = LittleLong(in->decorations[i].type);
				sz = fwrite (&in->decorations[i].type, sizeof(int), 1, file);
			}
		}

        fclose(file);
    }
}
// read in terraindata_t struct from file
qboolean ReadTerrainData (terraindata_t *out, const char *name, int forRender)
{
	FILE* file;
	char collisionFile[256];
	int		i, pathLen;
	size_t sz;
	char    *path;

	path = NULL;

	for(;;)
	{
		path = FS_NextPath( path );
		if( !path )
		{
			break;
		}

		if(forRender)
			sprintf(collisionFile, "%s/%sr", path, name);
		else
		{
			sprintf(collisionFile, "%s/%sx", path, name);
			Com_Printf("Searching for collision file at %s\n", collisionFile);
		}

		i = 0;
		file = fopen(collisionFile, "rb");
		if(file != NULL) 
		{
			memset (out, 0, sizeof(*out));

			sz = fread(&out->num_vertices, sizeof(int), 1, file);	
			out->num_vertices = LittleLong(out->num_vertices);

			out->vert_texcoords = (float *)Z_Malloc (out->num_vertices*2*sizeof(float));
			out->vert_positions = (float *)Z_Malloc (out->num_vertices*3*sizeof(float));

			for(i = 0; i < out->num_vertices*2; i++)
			{ 
				sz = fread(&out->vert_texcoords[i],sizeof(float), 1, file); 
				out->vert_texcoords[i] = LittleFloat(out->vert_texcoords[i]);
			}

			for(i = 0; i < out->num_vertices*3; i++)
			{ 
				sz = fread(&out->vert_positions[i],sizeof(float), 1, file);
				out->vert_positions[i] = LittleFloat(out->vert_positions[i]);
			}
					
			sz = fread(&out->num_triangles,sizeof(int), 1, file); 
			out->num_triangles = LittleLong(out->num_triangles);

			out->tri_indices = Z_Malloc (out->num_triangles*3*sizeof(unsigned int));

			for(i = 0; i < out->num_triangles*3; i++)
			{
				sz = fread(&out->tri_indices[i],sizeof(unsigned int), 1, file); 
				out->tri_indices[i] = LittleLong(out->tri_indices[i]);
			}

			for(i = 0; i < 3; i++)
			{
				sz = fread(&out->maxs[i], sizeof(float), 1, file);
				out->maxs[i] = LittleFloat(out->maxs[i]);
				sz = fread(&out->mins[i], sizeof(float), 1, file);
				out->mins[i] = LittleFloat(out->mins[i]);
			}

			if(forRender)
			{			
				out->hmtex_path = (char *)Z_Malloc (128 * sizeof(char));
				out->texture_path = (char *)Z_Malloc (128 * sizeof(char));
				out->lightmap_path = (char *)Z_Malloc (128 * sizeof(char));

				sz = fread (out->hmtex_path, sizeof(char) * 128, 1, file);
				sz = fread (out->texture_path, sizeof(char) * 128, 1, file);
				sz = fread (out->lightmap_path, sizeof(char) * 128, 1, file);

				sz = fread (&pathLen, sizeof(int), 1, file);
				pathLen = LittleLong(pathLen);

				out->decoration_variant_paths = (char *)Z_Malloc (sizeof(char) * pathLen);
				sz = fread (out->decoration_variant_paths, sizeof(char) * pathLen, 1, file);

				sz = fread (&out->num_decorations, sizeof(int), 1, file);
				out->num_decorations = LittleLong(out->num_decorations);

				if(out->num_decorations > 0)
				{
					out->decorations = (terraindec_t *)Z_Malloc (out->num_decorations * sizeof(terraindec_t));
					for(i = 0; i < out->num_decorations; i++)
					{
						//origin
						sz = fread (&out->decorations[i].origin[0], sizeof(float), 1, file);
						out->decorations[i].origin[0] = LittleFloat(out->decorations[i].origin[0]);
						sz = fread (&out->decorations[i].origin[1], sizeof(float), 1, file);
						out->decorations[i].origin[1] = LittleFloat(out->decorations[i].origin[1]);
						sz = fread (&out->decorations[i].origin[2], sizeof(float), 1, file);
						out->decorations[i].origin[2] = LittleFloat(out->decorations[i].origin[2]);

						sz = fread (&out->decorations[i].size, sizeof(float), 1, file);
						out->decorations[i].size = LittleFloat(out->decorations[i].size);
						sz = fread (&out->decorations[i].type, sizeof(int), 1, file);
						out->decorations[i].type = LittleLong(out->decorations[i].type);

						out->decorations[i].path = out->decoration_variant_paths;
					}
				}
			}
			fclose(file);
			Com_Printf("Terrain file %s loaded sucessfully!\n", collisionFile);
			return true;
		}
	}	
		
	if(!forRender)
		Com_Printf("Could not locate terrain collision mesh!\n");
	return false;
}

void CleanupTerrainData (terraindata_t *dat)
{
#define CLEANUPFIELD(field) \
	if (dat->field != NULL) \
	{ \
		Z_Free (dat->field); \
		dat->field = NULL; \
	}
	
	CLEANUPFIELD (hmtex_path)
	CLEANUPFIELD (texture_path)
	CLEANUPFIELD (lightmap_path)
	CLEANUPFIELD (vert_positions)
	CLEANUPFIELD (vert_texcoords)
	CLEANUPFIELD (tri_indices)
	CLEANUPFIELD (decorations)
	CLEANUPFIELD (decoration_variant_paths)
	
#undef CLEANUPFIELD
}
