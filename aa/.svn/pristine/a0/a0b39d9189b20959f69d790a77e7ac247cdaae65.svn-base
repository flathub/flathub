#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "qrad.h"

#include "binheap.h"
#include "libgarland.h"

static float grayscale_sample (const byte *texture, int tex_w, int tex_h, float u, float v, float *out_alpha)
{
    vec4_t res;
    
    bilinear_sample (texture, tex_w, tex_h, u, v, res);
    
    if (out_alpha != NULL)
    	*out_alpha = res[3];
    
    return (res[0] + res[1] + res[2]) / 3.0;
}

void *Z_Malloc (size_t sz)
{
	void *ret = malloc (sz);
	memset (ret, 0, sz);
	return ret;
}

char *CopyString (const char *in)
{
	void *ret;
	int len = strlen(in)+1;
	ret = Z_Malloc (len);
	strcpy (ret, in);
	return ret;
}

void LoadTerrainFile (terraindata_t *out, const char *name, float oversampling_factor, int reduction_amt, char *buf)
{
	int i, j, va, w, h, vtx_w, vtx_h;
	char	*hmtex_path = NULL;
	byte	*alphamask; // Bitmask of "holes" in terrain from alpha channel
	mesh_t	mesh;
	vec3_t	scale;
	char	*token;
	byte	*texdata;
	char newname[4096];
	
	memset (out, 0, sizeof(*out));
	
	buf = strtok (buf, ";");
	while (buf)
	{
		token = COM_Parse (&buf);
		if (!buf && !(buf = strtok (NULL, ";")))
			break;

#define FILENAME_ATTR(cmd_name,out) \
		if (!Q_strcasecmp (token, cmd_name)) \
		{ \
			sprintf (newname, "%s%s", moddir, COM_Parse (&buf)); \
			out = CopyString (newname); \
			if (!buf) \
			{ \
				printf ("LoadTerrainFile: EOL when expecting " cmd_name " filename! (File %s is invalid)\n", name); \
				assert (false); \
			} \
		} 
		
		FILENAME_ATTR ("heightmap", hmtex_path)
		FILENAME_ATTR ("texture", out->texture_path)
		FILENAME_ATTR ("lightmap", out->lightmap_path)
		
#undef FILENAME_ATTR
		
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				out->mins[i] = atof (COM_Parse (&buf));
				if (!buf)
				{
					printf ("LoadTerrainFile: EOL when expecting mins %c axis! (File %s is invalid)\n", "xyz"[i], name);
					assert (false);
				}
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				out->maxs[i] = atof (COM_Parse (&buf));
				if (!buf)
				{
					printf ("LoadTerrainFile: EOL when expecting maxs %c axis! (File %s is invalid)\n", "xyz"[i], name);
					assert (false);
				}
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		buf = strtok (NULL, ";");
	}
	
	if (!hmtex_path)
	{
		printf ("LoadTerrainFile: Missing heightmap texture in %s!\n", name);
		assert (false);
	}
	
	if (!out->texture_path)
	{
		printf ("LoadTerrainFile: Missing heightmap texture in %s!\n", name);
		assert (false);
	}
	
	VectorSubtract (out->maxs, out->mins, scale);
	
	LoadTGA (hmtex_path, &texdata, &w, &h);
	out->heightmap_w = w;
	out->heightmap_h = h;
	
	if (texdata == NULL)
	{
		printf ("LoadTerrainFile: Can't find file %s\n", hmtex_path);
		assert (false);
	}
	
	Z_Free (hmtex_path);
	
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
	
	simplify_mesh (&mesh, out->num_triangles/reduction_amt);
	
	out->num_triangles = mesh.num_verts;
	out->num_triangles = mesh.num_tris;
}

void CleanupTerrainData (terraindata_t *dat)
{
#define CLEANUPFIELD(field) \
	if (dat->field != NULL) \
	{ \
		Z_Free (dat->field); \
		dat->field = NULL; \
	}
	
	CLEANUPFIELD (texture_path)
	CLEANUPFIELD (lightmap_path)
	CLEANUPFIELD (vert_positions)
	CLEANUPFIELD (vert_texcoords)
	CLEANUPFIELD (tri_indices)
	
#undef CLEANUPFIELD
}
