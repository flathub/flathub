#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include <float.h>

// Maximums
#define DECAL_POLYS MAX_TRIANGLES // This really is a bit high
#define DECAL_VERTS (2*(3*DECAL_POLYS)) // double to make room for temp new verts

// alloc some FBOs for blitting lightmap texture data
// TODO: fix the way FBOs all over the codebase "leak" at vid_restart
static GLuint new_lm_fbo, orig_lm_fbo;
void R_Decals_InitFBO (void)
{
	qglGenFramebuffersEXT (1, &new_lm_fbo);
	qglGenFramebuffersEXT (1, &orig_lm_fbo);
}

extern qboolean TriangleIntersectsBBox
	(const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec3_t mins, const vec3_t maxs, vec3_t out_mins, vec3_t out_maxs);

// Creates a rotation matrix that will *undo* the specified rotation. We must
// apply the rotations in the opposite order or else we won't properly be
// preemptively undoing the rotation transformation applied later in the
// renderer.
void AnglesToMatrix3x3_Backwards (const vec3_t angles, float rotation_matrix[3][3])
{
	int i, j, k, l;
	
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			rotation_matrix[i][j] = i == j;
	}
	for (i = 2; i >= 0; i--)
	{
		vec3_t rotation_angles;
		float prev_rotation_matrix[3][3];
		float temp_matrix[3][3];
		
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 3; k++)
			{
				prev_rotation_matrix[j][k] = rotation_matrix[j][k];
				rotation_matrix[j][k] = 0.0;
			}
		}
		
		VectorClear (rotation_angles);
		rotation_angles[i] = -angles[i];
		AnglesToMatrix3x3 (rotation_angles, temp_matrix);
		
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 3; k++)
			{
				for (l = 0; l < 3; l++)
					rotation_matrix[j][k] += prev_rotation_matrix[l][k] * temp_matrix[j][l];
			}
		}
	}
}

// This structure describes a vertex. Prior to the generation of the new
// lightmap and the recalculation of the lightmap texcoords, the lightmap
// texcoords in this structure are from the original mesh. Later the texcoords
// are rewritten to refer to the new composite lightmap for the decal. The
// vertex is also marked with what lightmap texture it's in. All verts in a
// single triangle must have the same lightmap texture.
typedef struct
 {
	enum {lmsrc_tex, lmsrc_ent} type;
	union
	{
		int			texnum;
		entity_t	*ent;
	};
} lightmapsource_t;
#define srcequal(a,b) (!memcmp (&(a), &(b), sizeof (lightmapsource_t)))
typedef struct
{
	vec3_t	vertex;
	vec2_t	st;
	lightmapsource_t src;
} decalrawvertex_t;

// This structure is used to accumulate 3D geometry that will make up a 
// finished decal mesh. Geometry is taken from the world (including terrain
// meshes and BSP surfaces,) clipped aginst the oriented bounding box of the
// decal, and retesselated as needed.
typedef struct
{
	int npolys;
	int nverts;
	unsigned int polys[DECAL_POLYS][3];
	decalrawvertex_t verts[DECAL_VERTS];
} decalprogress_t;

// This structure contains 3D geometry that might potentially get added to a
// decal. All the triangles in here will be checked against a decal's OBB as
// described above.
typedef struct
{
	int npolys;
	int nverts;
	unsigned int *vtriangles;
	decalrawvertex_t *inputverts;
	// For moving terrain/mesh geometry to its actual position in the world
	vec3_t origin;
	// For rotating terrain/mesh geometry into its actual orientation
	float rotation_matrix[3][3];
} decalinput_t;

// This structure describes the orientation and position of the decal being
// created.
typedef struct
{
	// For transforming world-oriented geometry to the decal's position
	vec3_t origin;
	// For rotating world-oriented geometry into the decal's orientation
	float decal_space_matrix[3][3]; 
	// For clipping decal-oriented geometry against the decal's bounding box
	vec3_t mins, maxs;
} decalorientation_t;

// This function should be able to handle any indexed triangle geometry.
static void Mod_AddToDecalModel (const decalorientation_t *pos, const decalinput_t *in, decalprogress_t *out)
{
	int i, j, k;
	
	int trinum;
	
	// If a vertex was at index n in the original model, then in the new decal
	// model, it will be at index_table[n]-1. A value of 0 for index_table[n]
	// indicates space for the vertex hasn't been allocated in the new decal
	// model yet.
	unsigned int *index_table;
	
	index_table = Z_Malloc (in->nverts*sizeof(unsigned int));
	
	// Look for all the triangles from the original model that happen to occur
	// inside the oriented bounding box of the decal.
	for (trinum = 0; trinum < in->npolys; trinum++)
	{
		int orig_idx;
		vec3_t verts[3];
		
		for (i = 0; i < 3; i++)
		{
			vec3_t tmp, tmp2;
			
			// First, transform the mesh geometry into world space
			VectorCopy (in->inputverts[in->vtriangles[3*trinum+i]].vertex, tmp);
			VectorClear (tmp2);
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					tmp2[j] += tmp[k] * in->rotation_matrix[j][k];
			}
			VectorAdd (tmp2, in->origin, tmp2);
			
			// Then, transform it into decal space
			VectorSubtract (tmp2, pos->origin, tmp);
			VectorClear (verts[i]);
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					verts[i][j] += tmp[k] * pos->decal_space_matrix[j][k];
			}
		}
		
		if (!TriangleIntersectsBBox (verts[0], verts[1], verts[2], pos->mins, pos->maxs, NULL, NULL))
			continue;
		
		for (i = 0; i < 3; i++)
		{
			orig_idx = in->vtriangles[3*trinum+i];
			if (index_table[orig_idx] == 0)
			{
				index_table[orig_idx] = ++out->nverts;
				if (out->nverts >= DECAL_VERTS)
					Com_Error (ERR_FATAL, "Mod_AddTerrainToDecalModel: DECAL_VERTS");
				VectorCopy (verts[i], out->verts[index_table[orig_idx]-1].vertex);
				Vector2Copy (in->inputverts[in->vtriangles[3*trinum+i]].st, out->verts[index_table[orig_idx]-1].st);
				out->verts[index_table[orig_idx]-1].src = in->inputverts[in->vtriangles[3*trinum+i]].src;
			}
			
			out->polys[out->npolys][i] = index_table[orig_idx]-1;
		}
		out->npolys++;
		if (out->npolys >= DECAL_POLYS)
			Com_Error (ERR_FATAL, "Mod_AddTerrainToDecalModel: DECAL_POLYS");
	}
	
	Z_Free (index_table);
	
}

// Add geometry from a terrain model to the decal
static void Mod_AddTerrainToDecalModel (const decalorientation_t *pos, entity_t *terrainentity, decalprogress_t *out)
{
	int i;
	model_t *terrainmodel;
	mesh_framevbo_t *framevbo;
	void *_basevbo;
	lightmapsource_t src;
	
	GLuint vbo_xyz;
	
	decalinput_t in;
	
	terrainmodel = terrainentity->model;
	
	VectorCopy (terrainentity->origin, in.origin);
	AnglesToMatrix3x3 (terrainentity->angles, in.rotation_matrix);
	in.npolys = terrainmodel->num_triangles;
	in.nverts = terrainmodel->numvertexes;
	in.inputverts = Z_Malloc (in.nverts * sizeof(*in.inputverts));
	
	// get vertex positions
	vbo_xyz = terrainmodel->vboIDs[(terrainmodel->typeFlags & MESH_INDEXED) ? 2 : 1];
	GL_BindVBO (vbo_xyz);
	framevbo = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
	if (framevbo == NULL)
	{
		Com_Printf ("Mod_AddTerrainToDecalModel: qglMapBufferARB on vertex positions: %u\n", qglGetError ());
		goto cleanup;
	}
	
	for (i = 0; i < in.nverts; i++)
		VectorCopy (framevbo[i].vertex, in.inputverts[i].vertex);
	
	qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	
	// Get texcoords. Currently non-BSP meshes don't have separate texcoords
	// for lightmaps, they reuse their main texcoords.
	GL_BindVBO (terrainmodel->vboIDs[0]);
	_basevbo = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
	if (_basevbo == NULL)
	{
		Com_Printf ("Mod_AddTerrainToDecalModel: qglMapBufferARB on vertex texcoords: %u\n", qglGetError ());
		goto cleanup;
	}
	
	if ((terrainmodel->typeFlags & MESH_SKELETAL))
	{
		skeletal_basevbo_t *basevbo = _basevbo;
		for (i = 0; i < in.nverts; i++)
			Vector2Copy (basevbo[i].common.st, in.inputverts[i].st);
	}
	else
	{
		nonskeletal_basevbo_t *basevbo = _basevbo;
		for (i = 0; i < in.nverts; i++)
			Vector2Copy (basevbo[i].st, in.inputverts[i].st);
	}
	
	qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	
	// All verts from the same terrain mesh have the same lightmap texture
	if (terrainmodel->lightmap != NULL)
	{
		src.type = lmsrc_tex;
		src.texnum = terrainmodel->lightmap->texnum;
	}
	else
	{
		src.type = lmsrc_ent;
		src.ent = terrainentity;
	}
	for (i = 0; i < in.nverts; i++)
		in.inputverts[i].src = src;
	
	if ((terrainmodel->typeFlags & MESH_INDEXED))
	{
		GLuint vbo_indices = terrainmodel->vboIDs[1];
		GL_BindVBO (vbo_indices);
		in.vtriangles = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
		if (in.vtriangles == NULL)
		{
			Com_Printf ("Mod_AddTerrainToDecalModel: qglMapBufferARB on vertex indices: %u\n", qglGetError ());
			goto cleanup;
		}
		
		Mod_AddToDecalModel (pos, &in, out);
		
		qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	}
	else
	{
		int i;
		
		// Fake indexing for MD2s.
		in.npolys = in.nverts / 3;
		in.vtriangles = Z_Malloc (in.nverts * sizeof(unsigned int));
		for (i = 0; i < in.nverts; i++)
			in.vtriangles[i] = i;
		
		Mod_AddToDecalModel (pos, &in, out);
		
		Z_Free (in.vtriangles);
	}
	
cleanup:
	GL_BindVBO (0); // Do we need this?
	Z_Free (in.inputverts);
}

// Add geometry from the BSP to the decal
static void Mod_AddBSPToDecalModel (const decalorientation_t *pos, decalprogress_t *out)
{
	int surfnum, vertnum, trinum;
	int i, n;
	float *v;
	decalinput_t in;
	
	VectorClear (in.origin);
	AnglesToMatrix3x3 (vec3_origin, in.rotation_matrix);
	
	// FIXME HACK: we generate a separate indexed mesh for each BSP surface.
	// TODO: generate indexed BSP VBOs in r_vbo.c, and then just use the same
	// code as we use for other mesh types.
	for (surfnum = 0; surfnum < r_worldmodel->numsurfaces; surfnum++)
	{
		lightmapsource_t src;
		msurface_t *surf = &r_worldmodel->surfaces[surfnum];
		
		if ((surf->texinfo->flags & (SURF_TRANS33|SURF_SKY|SURF_WARP|SURF_FLOWING|SURF_NODRAW)))
			continue;
		
		in.nverts = surf->polys->numverts;
		in.inputverts = Z_Malloc (in.nverts * sizeof(*in.inputverts));
		
		src.type = lmsrc_tex;
		src.texnum = surf->lightmaptexturenum + TEXNUM_LIGHTMAPS;
		
		for (vertnum = 0, v = surf->polys->verts[0]; vertnum < in.nverts; vertnum++, v += VERTEXSIZE)
		{
			VectorCopy (v, in.inputverts[vertnum].vertex);
			Vector2Copy (v+5, in.inputverts[vertnum].st);
			in.inputverts[vertnum].src = src;
		}
		
		in.npolys = in.nverts - 2;
		in.vtriangles = Z_Malloc (in.npolys * 3 * sizeof(unsigned int));
		
		for (trinum = 1, n = 0; trinum < in.nverts-1; trinum++)
		{
			in.vtriangles[n++] = 0;
		
			for (i = trinum; i < trinum+2; i++)
				in.vtriangles[n++] = i;
		}
		
		Mod_AddToDecalModel (pos, &in, out);
		
		Z_Free (in.inputverts);
		Z_Free (in.vtriangles);
	}
}

// If a triangle is entirely inside the bounding box, this function does 
// nothing. Otherwise, it clips against one of the bounding planes of the box
// and then (if needed) retesselates the area being kept. If maxborder is 
// false and axisnum is 0, then the plane being clipped against is the plane
// defined by x = mins[0]. If maxborder is true and axisnum is 1, then the
// plane is y = maxs[1].
static void ClipTriangleAgainstBorder (int trinum, const decalorientation_t *pos, int axisnum, qboolean maxborder, decalprogress_t *out)
{
	int new_trinum;
	int i;
	unsigned int outsideidx[3], insideidx[3];
	unsigned newidx[3];
	int outside[3];
	int noutside = 0, ninside = 0;
	
	// For making sure any new triangles are facing the right way.
	vec3_t v0, v1, norm, norm2;
	qboolean flip;
	
	for (i = 0; i < 3; i++)
	{
		qboolean isoutside;
		float coord;
		unsigned int vertidx = out->polys[trinum][i];
		
		coord = out->verts[vertidx].vertex[axisnum];
		
		if (maxborder)
			isoutside = coord > pos->maxs[axisnum];
		else
			isoutside = coord < pos->mins[axisnum];
		
		if (isoutside)
		{
			outside[noutside] = i;
			outsideidx[noutside++] = vertidx;
		}
		else
		{
			insideidx[ninside++] = vertidx;
		}
	}
	
	assert (ninside+noutside == 3);
	
	// No need to clip anything
	if (ninside == 3)
		return;
	
	assert (ninside != 0);
	assert (noutside != 3);
	
	// No need to add a new vertex and retesselate the resulting quad, just
	// clip the two vertices that are outside.
	if (noutside == 2)
	{
		for (i = 0; i < 2; i++)
		{
			float old_len, old_stlen, new_len;
			vec3_t dir;
			vec2_t stdir;
			decalrawvertex_t *new_vert = &out->verts[out->nverts];
			
			if (out->nverts+1 >= DECAL_VERTS)
				Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_VERTS");
			
			VectorSubtract (out->verts[outsideidx[i]].vertex, out->verts[insideidx[0]].vertex, dir);
			old_len = VectorNormalize (dir);
			Vector2Subtract (out->verts[outsideidx[i]].st, out->verts[insideidx[0]].st, stdir);
			old_stlen = Vector2Normalize (stdir);
			
			if (maxborder)
				new_len = out->verts[insideidx[0]].vertex[axisnum] - pos->maxs[axisnum];
			else
				new_len = out->verts[insideidx[0]].vertex[axisnum] - pos->mins[axisnum];
			new_len /= out->verts[insideidx[0]].vertex[axisnum] - out->verts[outsideidx[i]].vertex[axisnum];
			
			VectorMA (out->verts[insideidx[0]].vertex, new_len * old_len, dir, new_vert->vertex);
			Vector2MA (out->verts[insideidx[0]].st, new_len * old_stlen, stdir, new_vert->st);
			new_vert->src = out->verts[outsideidx[i]].src;
			out->polys[trinum][outside[i]] = out->nverts++;
		}
		
		return;
	}
	
	// If we reach this point, noutside == 1
	assert (noutside == 1);
	
	new_trinum = out->npolys++;
	
	if (out->npolys >= DECAL_POLYS)
		Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_POLYS");
	
	for (i = 0; i < 2; i++)
	{
		float old_len, old_stlen, new_len;
		vec3_t dir;
		vec2_t stdir;
		decalrawvertex_t *new_vert = &out->verts[out->nverts];
		
		if (out->nverts+1 >= DECAL_VERTS)
			Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_VERTS");
		
		newidx[i] = out->nverts++;
		
		VectorSubtract (out->verts[outsideidx[0]].vertex, out->verts[insideidx[i]].vertex, dir);
		old_len = VectorNormalize (dir);
		Vector2Subtract (out->verts[outsideidx[0]].st, out->verts[insideidx[i]].st, stdir);
		old_stlen = Vector2Normalize (stdir);
		
		if (maxborder)
			new_len = out->verts[insideidx[i]].vertex[axisnum] - pos->maxs[axisnum];
		else
			new_len = out->verts[insideidx[i]].vertex[axisnum] - pos->mins[axisnum];
		new_len /= out->verts[insideidx[i]].vertex[axisnum] - out->verts[outsideidx[0]].vertex[axisnum];
		
		VectorMA (out->verts[insideidx[i]].vertex, new_len * old_len, dir, new_vert->vertex);
		Vector2MA (out->verts[insideidx[i]].st, new_len * old_stlen, stdir, new_vert->st);
		new_vert->src = out->verts[outsideidx[0]].src;
	}
	
	VectorSubtract (out->verts[out->polys[trinum][1]].vertex, out->verts[out->polys[trinum][0]].vertex, v0);
	VectorSubtract (out->verts[out->polys[trinum][2]].vertex, out->verts[out->polys[trinum][0]].vertex, v1);
	CrossProduct (v0, v1, norm);
	VectorNormalize (norm);
	
	out->polys[trinum][outside[0]] = newidx[0];
	
	for (i = 0; i < 2; i++)
		out->polys[new_trinum][i] = newidx[i];
	out->polys[new_trinum][2] = insideidx[1];
	
	VectorSubtract (out->verts[out->polys[new_trinum][1]].vertex, out->verts[out->polys[new_trinum][0]].vertex, v0);
	VectorSubtract (out->verts[out->polys[new_trinum][2]].vertex, out->verts[out->polys[new_trinum][0]].vertex, v1);
	CrossProduct (v0, v1, norm2);
	VectorNormalize (norm2);
	
	flip = false;
	for (i = 0; i < 3; i++)
	{
		if (norm[i]/norm2[i] < 0.0)
			flip = true;
	}
	
	if (flip)
	{
		unsigned int tmpidx = out->polys[new_trinum][0];
		out->polys[new_trinum][0] = out->polys[new_trinum][1];
		out->polys[new_trinum][1] = tmpidx;
	}
}

static qboolean TriangleOutsideBounds (int trinum, const decalorientation_t *pos, const decalprogress_t *mesh)
{
	int axisnum;
	qboolean maxborder;
	
	for (maxborder = false; maxborder <= true; maxborder++)
	{
		for (axisnum = 0; axisnum < 3; axisnum++)
		{
			int i;
			int noutside = 0;
	
			for (i = 0; i < 3; i++)
			{
				qboolean isoutside;
				float coord;
				unsigned int vertidx = mesh->polys[trinum][i];
		
				coord = mesh->verts[vertidx].vertex[axisnum];
		
				if (maxborder)
					isoutside = coord > pos->maxs[axisnum];
				else
					isoutside = coord < pos->mins[axisnum];
		
				if (isoutside)
					noutside++;
			}
			
			if (noutside == 3)
				return true;
		}
	}
	
	return false;
}

// Sometimes, in the process of clipping a triangle against one border of the
// bounding box, one of the resulting split triangles is now completely
// outside another border. For example:
//            /\ 
//  .========/  \ 
//  |       /|___\ 
//  |        |
//  '========'
// After clipping against the top border, we get a trapezoid-shaped region
// which is tesselated into two triangles. Depending on the tesselation, one
// of those triangles may actually be entirely outside the bounding box.
//
// So for the sake of consistency, we re-cull every terrain triangle we 
// modify before re-clipping it against the next border.
static void ReCullTriangles (const decalorientation_t *pos, decalprogress_t *out)
{
	int outTriangle;
	int inTriangle;
	
	for (inTriangle = outTriangle = 0; inTriangle < out->npolys; inTriangle++)
	{
		if (TriangleOutsideBounds (inTriangle, pos, out))
			continue;
		
		if (outTriangle != inTriangle)
		{
			int i;
			
			for (i = 0; i < 3; i++)
				out->polys[outTriangle][i] = out->polys[inTriangle][i];
		}
		
		outTriangle++;
	}
	
	out->npolys = outTriangle;
}

static void ReUnifyVertexes (decalprogress_t *out)
{
	int trinum, vertnum, maxidx = 0, newidx = 0;
	unsigned int *index_table;
	decalrawvertex_t *new_verts;
	
	// Detect duplicate vertices and substitute the index of the first
	// occurance of each duplicate vertex
	for (trinum = 0; trinum < out->npolys; trinum++)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			int tmpidx, idx;
			
			tmpidx = out->polys[trinum][i];
			for (idx = 0; idx < tmpidx; idx++)
			{
				int j;
				
				for (j = 0; j < 3; j++)
				{
					if (fabs (out->verts[tmpidx].vertex[j] - out->verts[idx].vertex[j]) > FLT_EPSILON)
						goto not_a_duplicate;
				}
				
				for (j = 0; j < 2; j++)
				{
					if (fabs (out->verts[tmpidx].st[j] - out->verts[idx].st[j]) > FLT_EPSILON)
						goto not_a_duplicate;
				}
				
				if (srcequal (out->verts[tmpidx].src, out->verts[idx].src))
					break;
not_a_duplicate:;
			}
			
			out->polys[trinum][i] = idx;
			
			if (maxidx <= idx)
				maxidx = idx+1;
		}
	}
	
	out->nverts = maxidx;
	
	// Prune redundant vertices- generate new indices
	index_table = Z_Malloc (maxidx*sizeof(unsigned int));
	for (trinum = 0; trinum < out->npolys; trinum++)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			if (index_table[out->polys[trinum][i]] == 0)
				index_table[out->polys[trinum][i]] = ++newidx;
			out->polys[trinum][i] = index_table[out->polys[trinum][i]] - 1;
		}
	}
	
	// Prune redundant vertices- rearrange the actual vertex data to match the
	// new indices
	new_verts = Z_Malloc (newidx*sizeof(decalrawvertex_t));
	for (vertnum = 0; vertnum < maxidx; vertnum++)
	{
		if (index_table[vertnum] != 0)
			new_verts[index_table[vertnum]-1] = out->verts[vertnum];
	}
	memcpy (out->verts, new_verts, newidx*sizeof(decalrawvertex_t));
	
	out->nverts = newidx;
	
	Z_Free (index_table);
	Z_Free (new_verts);
}

static entity_t *load_decalentity;

// Vertices are grouped by what lightmap texture the original geometry used. Or
// 0 for none.
typedef struct {
	lightmapsource_t src;
	vec2_t orig_mins, orig_maxs, orig_fpsize;
	vec2_t new_mins, new_maxs, new_fpsize;
	int size_used[2];
	int orig_int_mins[2], orig_int_maxs[2];
	int int_mins[2], int_maxs[2];
} decal_vertgroup_t;

static decal_vertgroup_t *GenerateLightmapTexcoords (decalprogress_t *out, model_t *mod, int *out_numgroups, int final_size[2])
{
	int i, j, k;
	decal_vertgroup_t *groups;
	int *group_given_vertex;
	int *allocated;
	int max_group_tex_size[2] = {0, 0}, total_group_tex_size[2] = {0, 0};
	int numgroups = 0;
	qboolean transpose;
	
	// Build a complete list of lightmap textures used
	groups = Z_Malloc (out->nverts * sizeof(*groups));
	group_given_vertex = Z_Malloc (out->nverts * sizeof(int));
	
	for (i = 0; i < out->nverts; i++)
	{
		for (j = 0; j < numgroups; j++)
		{
			if (srcequal (groups[j].src, out->verts[i].src))
			{
				for (k = 0; k < 2; k++)
				{
					if (out->verts[i].st[k] > groups[j].orig_maxs[k])
						groups[j].orig_maxs[k] = out->verts[i].st[k];
					else if (out->verts[i].st[k] < groups[j].orig_mins[k])
						groups[j].orig_mins[k] = out->verts[i].st[k];
				}
				group_given_vertex[i] = j;
				break;
			}
		}
		
		if (j == numgroups)
		{
			groups[numgroups].src = out->verts[i].src;
			Vector2Copy (out->verts[i].st, groups[numgroups].orig_maxs);
			Vector2Copy (out->verts[i].st, groups[numgroups].orig_mins);
			group_given_vertex[i] = numgroups++;
		}
	}
	
	// Figure out how many pixels out of each texture are actually used. Each
	// of the original lightmap textures contributes one rectangular block to
	// the new composite lightmap. This part of the code figures out the
	// smallest single rectangle of each of the textures that contains all the
	// pixels we need.
	for (i = 0; i < numgroups; i++)
	{
		int texsize[2] = {0, 0};
		lightmapsource_t src = groups[i].src;
		
		Vector2Subtract (groups[i].orig_maxs, groups[i].orig_mins, groups[i].orig_fpsize);
		
		if (src.type == lmsrc_ent)
		{
			// for geometry without an underlying lightmap, we use a fraction of
			// the size of the decal texture
			if (mod->skins[0] != NULL)
			{
				texsize[0] = mod->skins[0]->width / 2;
				texsize[1] = mod->skins[0]->height / 2;
			}
		}
		else if (src.texnum >= TEXNUM_LIGHTMAPS && src.texnum < TEXNUM_LIGHTMAPS + MAX_LIGHTMAPS)
		{
			// BSP lightmap
			texsize[0] = texsize[1] = LIGHTMAP_SIZE;
		}
		else if (src.texnum >= TEXNUM_IMAGES && src.texnum < TEXNUM_IMAGES + MAX_GLTEXTURES)
		{
			// underlying lightmap texture is allocated as a regular texture (as
			// it would be on a terrain mesh.)
			texsize[0] = gltextures[src.texnum - TEXNUM_IMAGES].width;
			texsize[1] = gltextures[src.texnum - TEXNUM_IMAGES].height;
		}
		
		for (j = 0; j < 2; j++)
		{
			groups[i].orig_int_mins[j] = floorf (groups[i].orig_mins[j] * (float)texsize[j]);
			groups[i].orig_int_maxs[j] = ceilf (groups[i].orig_maxs[j] * (float)texsize[j]);
			groups[i].size_used[j] = groups[i].orig_int_maxs[j] - groups[i].orig_int_mins[j];
			if (groups[i].size_used[j] > max_group_tex_size[j])
				max_group_tex_size[j] = groups[i].size_used[j];
			total_group_tex_size[j] += groups[i].size_used[j];
		}
	}
	
	// The largest single dimension (height or width) of any of the blocks will
	// dictate the corresponding dimension (height or width) of the new lightmap
	// texture. The packing algorithm requires that we know at least one
	// of the dimensions in advance (for the size of the "allocated" array) and
	// this is how we figure it out. If "transpose" is true, then each element
	// in "allocated" corresponds to the furthest-right allocated pixel in a
	// row, otherwise it's the furthest-down allocated pixel in a column.
	transpose = max_group_tex_size[1] > max_group_tex_size[0];
	
	final_size[transpose] = max_group_tex_size[transpose];
	final_size[!transpose] = 0;
	
	// We figure out how the pixels from the original lightmap textures are
	// going to be packed into the new lightmap, using the same algorithm as
	// used for building BSP lightmaps.
	allocated = Z_Malloc (final_size[transpose] * sizeof(int));
	
	for (k = 0; k < numgroups; k++)
	{
		int x, y, newh;
		int	best, best2;
		
		// upper bound on the new lightmap's height
		best = total_group_tex_size[!transpose] + 1;

		for (i = 0; i <= final_size[transpose] - groups[k].size_used[transpose]; i++)
		{
			best2 = 0;

			for (j = 0; j < groups[k].size_used[transpose]; j++)
			{
				
				if (allocated[i+j] >= best)
					break;
				if (allocated[i+j] > best2)
					best2 = allocated[i+j];
			}
			if (j == groups[k].size_used[transpose])
			{	// this is a valid spot
				x = i;
				y = best = best2;
			}
		}

		newh = best + groups[k].size_used[!transpose];
		
		for (i = 0; i < groups[k].size_used[transpose]; i++)
			allocated[x + i] = newh;
		
		if (newh > final_size[!transpose])
			final_size[!transpose] = newh;
		
		groups[k].int_mins[0] = transpose ? y : x;
		groups[k].int_mins[1] = transpose ? x : y;
		
		for (j = 0; j < 2; j++)
			groups[k].int_maxs[j] = groups[k].int_mins[j] + groups[k].size_used[j];
	}
	
	Z_Free (allocated);
	
	for (i = 0; i < numgroups; i++)
	{
		for (j = 0; j < 2; j++)
		{
			groups[i].new_mins[j] = (float)groups[i].int_mins[j] / (float)final_size[j];
			groups[i].new_maxs[j] = (float)(groups[i].int_mins[j] + groups[i].size_used[j]) / (float)final_size[j];
			groups[i].new_fpsize[j] = groups[i].new_maxs[j] - groups[i].new_mins[j];
		}
	}
	
	// Final step is to regenerate the lightmap texcoords for all the vertices
	for (i = 0; i < out->nverts; i++)
	{
		int g = group_given_vertex[i];
		for (j = 0; j < 2; j++)
			out->verts[i].st[j] = groups[g].new_mins[j] + groups[g].new_fpsize[j] * (out->verts[i].st[j] - groups[g].orig_mins[j]) / groups[g].orig_fpsize[j];
	}
	
	Z_Free (group_given_vertex);
	
	*out_numgroups = numgroups;
	return groups;
}

static void GenerateLightmapTexture (model_t *mod, decal_vertgroup_t *groups, int numgroups, int final_size[2])
{
	int i;
	char lm_tex_name[MAX_QPATH];
	image_t *lightmap_aux;
	static int num_lm_texes = 0;

	// We render into this texture first, then render into the final texture
	// with the defringe filter.
	lightmap_aux = GL_FindFreeImage ("***decal_lightmap_temp***", final_size[0], final_size[1], it_lightmap);

	GL_SelectTexture (0);
	GL_Bind (lightmap_aux->texnum);
	
	qglTexImage2D (GL_TEXTURE_2D, 0, gl_tex_alpha_format, final_size[0], final_size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	qglBindFramebufferEXT (GL_DRAW_FRAMEBUFFER_EXT, new_lm_fbo);
	qglFramebufferTexture2DEXT (GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, lightmap_aux->texnum, 0);
	qglViewport (0, 0, final_size[0], final_size[1]);
	qglClearColor (0.75, 0.75, 0.75, 0.0);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglClearColor (0.0, 0.0, 0.0, 1.0);
	
	qglBindFramebufferEXT (GL_READ_FRAMEBUFFER_EXT, orig_lm_fbo);
	
	// uncomment to make every block of the lightmap a different bright color.
	/*qglEnable (GL_SCISSOR_TEST);
	for (i = 0; i < numgroups; i++)
	{
		qglScissor (
			groups[i].int_mins[0], groups[i].int_mins[1],
			groups[i].size_used[0], groups[i].size_used[1]
		);
		
		i++;
		qglClearColor ((i & 1) ? 1.0 : 0.0, (i & 2) ? 1.0 : 0.0, (i & 4) ? 1.0 : 0.0, 1.0);
		i--;
		qglClear (GL_COLOR_BUFFER_BIT);
		
		continue;
	}
	qglDisable (GL_SCISSOR_TEST);*/
	
	for (i = 0; i < numgroups; i++)
	{
		if (groups[i].src.type == lmsrc_ent)
		{
			qglEnable (GL_SCISSOR_TEST);
			qglDisable (GL_DEPTH_TEST);
			qglViewport (
				groups[i].int_mins[0] - groups[i].orig_int_mins[0],
				groups[i].int_mins[1] - groups[i].orig_int_mins[1],
				mod->skins[0]->width / 2, mod->skins[0]->height / 2
			);
			qglScissor (
				groups[i].int_mins[0], groups[i].int_mins[1],
				groups[i].size_used[0], groups[i].size_used[1]
			);
			R_Mesh_ExtractLightmap (groups[i].src.ent, groups[i].src.ent->model);
			qglDisable (GL_SCISSOR_TEST);
			qglEnable (GL_DEPTH_TEST);
			continue;
		}
		
		qglFramebufferTexture2DEXT (GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, groups[i].src.texnum, 0);
		qglBlitFramebufferEXT (
				groups[i].orig_int_mins[0], groups[i].orig_int_mins[1],
				groups[i].orig_int_maxs[0], groups[i].orig_int_maxs[1],
				groups[i].int_mins[0], groups[i].int_mins[1],
				groups[i].int_maxs[0], groups[i].int_maxs[1],
				GL_COLOR_BUFFER_BIT, GL_NEAREST
		);
	}

	Com_sprintf (lm_tex_name, sizeof (lm_tex_name), "***decal_lightmap_%d***", num_lm_texes++);
	mod->lightmap = GL_FindFreeImage (lm_tex_name, final_size[0], final_size[1], it_lightmap);

	GL_Bind (mod->lightmap->texnum);
	
	qglTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, final_size[0], final_size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	
	// TODO: get rid for all lightmaps?
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->integer);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	GL_Bind (lightmap_aux->texnum);
	qglFramebufferTexture2DEXT (GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mod->lightmap->texnum, 0);
	qglViewport (0, 0, final_size[0], final_size[1]);

	glUseProgramObjectARB (g_defringeprogramObj);
	glUniform1iARB (defringe_uniforms.source, 0);
	glUniform2fARB (defringe_uniforms.scale, final_size[0], final_size[1]);
	GL_SetupWholeScreen2DVBO (wholescreen_fliptextured);

	qglMatrixMode (GL_PROJECTION);
	qglPushMatrix ();
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -10, 100);
	qglMatrixMode (GL_MODELVIEW);
	qglPushMatrix ();
	qglLoadIdentity ();

	qglDisable (GL_DEPTH_TEST);
	R_DrawVarrays (GL_QUADS, 0, 4);
	qglEnable (GL_DEPTH_TEST);

	qglPopMatrix ();
	qglMatrixMode (GL_PROJECTION);
	qglPopMatrix ();
	qglMatrixMode (GL_MODELVIEW);

	glUseProgramObjectARB (0);
	R_KillVArrays ();

	GL_Bind (mod->lightmap->texnum);
	qglGenerateMipmapEXT (GL_TEXTURE_2D);
	
	R_SetupViewport ();
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
	
	Z_Free (groups);
	GL_Bind (0);
	GL_FreeImage (lightmap_aux);
}

void Mod_LoadDecalModel (model_t *mod, void *_buf)
{
	decalprogress_t data;
	int i, j;
	const char *tex_path = NULL;
	nonskeletal_lm_basevbo_t *basevbo = NULL;
	mesh_framevbo_t *framevbo = NULL;
	image_t	*tex = NULL;
	char	*token;
	qboolean maxborder;
	char *buf = (char *)_buf;
	const char *line;
	decalorientation_t pos;
	int final_lightmap_size[2];
	decal_vertgroup_t *groups;
	int numgroups;
	
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
				Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting " cmd_name " filename! (File %s is invalid)", mod->name); \
		} 
		
		FILENAME_ATTR ("texture", tex_path)
		
#undef FILENAME_ATTR
		
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				mod->mins[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting mins %c axis! (File %s is invalid)", "xyz"[i], mod->name);
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				mod->maxs[i] = atof (COM_Parse (&line));
				if (!line)
					Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting maxs %c axis! (File %s is invalid)", "xyz"[i], mod->name);
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		line = strtok (NULL, ";");
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
	
	mod->type = mod_decal;
	mod->typeFlags = MESH_INDEXED | MESH_DECAL | MESH_LM_SEPARATE_COORDS;
	
	if (!tex_path)
		Com_Error (ERR_DROP, "Mod_LoadDecalFile: Missing texture in %s!", mod->name);
	tex = GL_FindImage (tex_path, it_wall);
	mod->skins[0] = tex;
	if (mod->skins[0] != NULL)
	{
		//load shader for skin
		mod->script = mod->skins[0]->script;
		if (mod->script)
			RS_ReadyScript ((rscript_t *)mod->script);
	}
	
	memset (&data, 0, sizeof(data));
	
	VectorCopy (mod->mins, pos.mins);
	VectorCopy (mod->maxs, pos.maxs);
	VectorCopy (load_decalentity->origin, pos.origin);
	// Create a rotation matrix to apply to world/terrain geometry to
	// transform it into decal space.
	AnglesToMatrix3x3_Backwards (load_decalentity->angles, pos.decal_space_matrix);
	
	for (i = 0; i < num_terrain_entities; i++)
		Mod_AddTerrainToDecalModel (&pos, &terrain_entities[i], &data);
	
	Mod_AddBSPToDecalModel (&pos, &data);
	
	for (maxborder = false; maxborder <= true; maxborder++)
	{
		for (j = 0; j < 3; j++)
		{
			int lim = data.npolys;
			for (i = 0; i < lim; i++)
				ClipTriangleAgainstBorder (i, &pos, j, maxborder, &data);
			ReCullTriangles (&pos, &data);
		}
	}
	
	ReUnifyVertexes (&data);
	groups = GenerateLightmapTexcoords (&data, mod, &numgroups, final_lightmap_size);
	
	mod->numvertexes = data.nverts;
	mod->num_triangles = data.npolys;
	
	basevbo = Z_Malloc (mod->numvertexes * sizeof(*basevbo));
	framevbo = Z_Malloc (mod->numvertexes * sizeof(*framevbo));
	
	for (i = 0; i < data.nverts; i++)
	{
		for (j = 0; j < 2; j++)
		{
			basevbo[i].lm_st[j] = data.verts[i].st[j];
			basevbo[i].common.st[!j] = 1.0-(data.verts[i].vertex[j] - mod->mins[j]) / (mod->maxs[j] - mod->mins[j]);
		}
		VectorCopy (data.verts[i].vertex, framevbo[i].vertex);
	}
	
	R_Mesh_LoadVBO (mod, MESHLOAD_CALC_NORMAL|MESHLOAD_CALC_TANGENT, basevbo, &data.polys[0][0], framevbo);
	
	Z_Free (framevbo);
	Z_Free (basevbo);
	
	GenerateLightmapTexture (mod, groups, numgroups, final_lightmap_size);
}

void R_ParseDecalEntity (char *match, char *block)
{
	entity_t	*ent;
	int			i;
	char		*bl, *tok;
	
	char model_path[MAX_QPATH];
	
	// This is used to make sure each loaded decal entity gets a unique copy
	// of the model. No need to reset this or anything.
	char fake_path[MAX_QPATH];
	static int	num_decals = 0; 
	
	if (num_decal_entities == MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "R_ParseDecalEntity: MAX_MAP_MODELS");
	
	ent = &decal_entities[num_decal_entities];
	memset (ent, 0, sizeof(*ent));
	ent->number = MAX_EDICTS+MAX_MAP_MODELS+MAX_ROCKS+num_decal_entities++;
	load_decalentity = ent;
	ent->flags = RF_FULLBRIGHT;
	
	Com_sprintf (fake_path, sizeof(fake_path), "*=decal%d=*", num_decals++);
	
	bl = block;
	while (1)
	{
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("model", tok))
		{
			// Must insure the entity's angles and origin are loaded first
			strncpy (model_path, Com_ParseExt(&bl, false), sizeof(model_path));
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
	
	ent->model = R_RegisterModel (model_path);
	if ( ent->model )
		strncpy (ent->model->name, fake_path, sizeof(fake_path));
	else
		Com_DPrintf("Missing decal file: %s\n", model_path);
}
