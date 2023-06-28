/*
Copyright (C) 2011-2014 COR Entertainment

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
// r_vbo.c: vertex buffer object managment

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

static GLuint bsp_vboId = 0;
GLuint bsp_iboId = 0, bsp_outlines_iboId;
GLuint minimap_vboId = 0;
static GLuint vegetation_vboId = 0;
static GLuint lensflare_vboId = 0;
static GLuint collision_vboId = 0, collision_iboId = 0;
static int num_collision_tris;

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
void *			(APIENTRY * qglMapBufferARB)(GLenum target, GLenum access);
GLboolean		(APIENTRY * qglUnmapBufferARB)(GLenum target);

#define AppendToVBO(idx,sz,ptr) \
{ \
	qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)idx, sz, ptr); \
	idx += sz; \
}
	

static void VB_VCInit (void);
void R_LoadVBOSubsystem(void)
{
	if ( GL_QueryExtension("GL_ARB_vertex_buffer_object") )
	{
		qglBindBufferARB = (void *)qwglGetProcAddress("glBindBufferARB");
		qglDeleteBuffersARB = (void *)qwglGetProcAddress("glDeleteBuffersARB");
		qglGenBuffersARB = (void *)qwglGetProcAddress("glGenBuffersARB");
		qglBufferDataARB = (void *)qwglGetProcAddress("glBufferDataARB");
		qglBufferSubDataARB = (void *)qwglGetProcAddress("glBufferSubDataARB");
		qglMapBufferARB = (void *)qwglGetProcAddress("glMapBufferARB");
		qglUnmapBufferARB = (void *)qwglGetProcAddress("glUnmapBufferARB");

		if (qglGenBuffersARB && qglBindBufferARB && qglBufferDataARB && qglDeleteBuffersARB && qglMapBufferARB && qglUnmapBufferARB)
		{
			Com_Printf("...using GL_ARB_vertex_buffer_object\n");
			VB_VCInit();
		}
	}
	else
	{
		Com_Error (ERR_FATAL, "...GL_ARB_vertex_buffer_object not found\n");
	}
}

#define MAX_VBO_XYZs		65536
static int VB_AddWorldSurfaceToVBO (msurface_t *surf, int currVertexNum)
{
	glpoly_t *p;
	float	*v;
	int		i, j;
	int		n;
	float	map[MAX_VBO_XYZs];
	int surf_num_verts = 0;
	
	n = 0;
	
	for (p = surf->polys; p != NULL; p = p->next)
	{
		for (i = 0; i < p->numverts; i++)
		{
			v = p->verts[i];
	
			// copy in vertex data
			for (j = 0; j < 7; j++)
				map[n++] = v[j];
			
			// normals and tangents are the same through the whole surface
			for (j = 0; j < 3; j++)
				map[n++] = surf->normal[j];
			for (j = 0; j < 4; j++)
				map[n++] = surf->tangent[j];
		}
		
		surf_num_verts += p->numverts;
	}
	
	qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, currVertexNum * 14 * sizeof(float), n * sizeof(float), &map);
	
	surf->vbo_first_vert = currVertexNum;
	return surf->vbo_last_vert = currVertexNum + surf_num_verts;
}

static int VB_AddWorldSurfaceToIBO (msurface_t *surf, int currIndexNum)
{
	glpoly_t *p;
	int		i;
	int		n;
	int		baseidx;
	int		trinum;
	unsigned int	map[MAX_VBO_XYZs];
	
	n = 0;
	baseidx = surf->vbo_first_vert;
	
	for (p = surf->polys; p != NULL; p = p->next)
	{
		for (trinum = 1; trinum < p->numverts-1; trinum++)
		{
			map[n++] = baseidx;
		
			for (i = trinum; i < trinum+2; i++)
				map[n++] = baseidx + i;
		}
		
		baseidx += p->numverts;
	}
	
	qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, currIndexNum * sizeof(unsigned int), n * sizeof(unsigned int), &map);
	
	surf->ibo_first_idx = currIndexNum;
	return surf->ibo_last_idx = currIndexNum + n;
}

static int VB_AddWorldSurfaceToOutlineIBO (msurface_t *surf, int currIndexNum)
{
	glpoly_t *p;
	int		i;
	int		n;
	int		baseidx;
	unsigned int	map[MAX_VBO_XYZs];
	
	n = 0;
	baseidx = surf->vbo_first_vert;
	
	for (p = surf->polys; p != NULL; p = p->next)
	{
		for (i = 0; i < p->numverts; i++)
		{
			map[n++] = baseidx + i;
			map[n++] = baseidx + (i + 1) % p->numverts;
		}
		
		baseidx += p->numverts;
	}
	
	qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, currIndexNum * sizeof(unsigned int), n * sizeof(unsigned int), &map);
	
	surf->ibo_first_outline_idx = currIndexNum;
	return surf->ibo_last_outline_idx = currIndexNum + n;
}

static void VB_BuildWorldSurfaceVBO (void)
{
	msurface_t *surf, *surfs, *prevsurf, *vbo_first_surf;
	int i, firstsurf, lastsurf;
	int	totalVBObufferSize = 0, totalIBObufferSize = 0, totalOutlineIBObufferSize = 0;
	int currVertexNum = 0;
	int currIndexNum = 0;
	
	// just to keep the lines of code short
	surfs = r_worldmodel->surfaces;
	firstsurf = 0;
	lastsurf = r_worldmodel->numsurfaces;
	
	for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
	{
		glpoly_t *p;
		if (surf->texinfo->flags & (SURF_SKY|SURF_NODRAW))
			continue;
		for (p = surf->polys; p != NULL; p = p->next)
		{
			totalIBObufferSize += 3*(p->numverts-2);
			totalVBObufferSize += 14*p->numverts;
			totalOutlineIBObufferSize += 2*p->numverts;
		}
	}
	
	qglGenBuffersARB (1, &bsp_vboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, bsp_vboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	vbo_first_surf = prevsurf = NULL;
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		if (currentmodel->unique_texinfo[i]->flags & (SURF_SKY|SURF_NODRAW))
			continue;
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					(surf->iflags & ISURF_PLANEBACK))
				continue;
			surf->vboprev = prevsurf;
			if (prevsurf)
				prevsurf->vbonext = surf;
			else
				vbo_first_surf = surf;
			prevsurf = surf;
			currVertexNum = VB_AddWorldSurfaceToVBO (surf, currVertexNum);
		}
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					!(surf->iflags & ISURF_PLANEBACK))
				continue;
			surf->vboprev = prevsurf;
			if (prevsurf)
				prevsurf->vbonext = surf;
			else
				vbo_first_surf = surf;
			prevsurf = surf;
			currVertexNum = VB_AddWorldSurfaceToVBO (surf, currVertexNum);
		}
	}
	if (prevsurf)
		prevsurf->vbonext = NULL;
	
	qglGenBuffersARB (1, &bsp_iboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, bsp_iboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalIBObufferSize*sizeof(unsigned int), 0, GL_STATIC_DRAW_ARB);
	
	for	(surf = vbo_first_surf; surf != NULL; surf = surf->vbonext)
		currIndexNum = VB_AddWorldSurfaceToIBO (surf, currIndexNum);
	
	qglGenBuffersARB (1, &bsp_outlines_iboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, bsp_outlines_iboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalOutlineIBObufferSize*sizeof(unsigned int), 0, GL_STATIC_DRAW_ARB);
	
	currIndexNum = 0;
	for	(surf = vbo_first_surf; surf != NULL; surf = surf->vbonext)
		currIndexNum = VB_AddWorldSurfaceToOutlineIBO (surf, currIndexNum);
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}

static void VB_BuildMinimapVBO (void)
{
	int i, j;
	int totalVBObufferSize = 0;
	int currVertexNum = 0;
	float *vbo_idx = 0;
	
	for (i = 1; i < r_worldmodel->numedges; i++)
	{
		if (r_worldmodel->edges[i].iscorner)
			totalVBObufferSize += (3+2)*2;
	}
	
	qglGenBuffersARB (1, &minimap_vboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, minimap_vboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (i = 1; i < r_worldmodel->numedges; i++)
	{
		medge_t *edge = &r_worldmodel->edges[i];
		
		if (!edge->iscorner)
			continue;
		
		edge->vbo_start = currVertexNum;
		currVertexNum += 2;
		
		for (j = 0; j < 2; j++)
		{
			vec_t *v = r_worldmodel->vertexes[edge->v[j]].position;
			
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)vbo_idx, 3 * sizeof(float), v);
			vbo_idx += 3;
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(vbo_idx++), sizeof(float), &edge->sColor);
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(vbo_idx++), sizeof(float), &edge->alpha);
		}
	}
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void GL_SetupWorldVBO (void)
{
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, bsp_vboId);

	R_VertexPointer (3, 14*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 14*sizeof(float), (void *)(3*sizeof(float)));
	R_TexCoordPointer (1, 14*sizeof(float), (void *)(5*sizeof(float)));
	R_NormalPointer (14*sizeof(float), (void *)(7*sizeof(float)));
	R_AttribPointer (ATTR_TANGENT_IDX, 4, GL_FLOAT, GL_FALSE, 14*sizeof(float), (void *)(10*sizeof(float)));
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}

static void VB_BuildCollisionVBO (void)
{
	int numvertices, numtriangles, i;
	
	numvertices = CM_NumVertices ();
	num_collision_tris = numtriangles = CM_NumTriangles ();
	
	qglGenBuffersARB (1, &collision_vboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, collision_vboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, numvertices*3*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (i = 0; i < numvertices; i++)
	{
		vec3_t tmp;
		
		CM_GetVertex (i, tmp);
			
		qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(3 * i * sizeof(float)), 3 * sizeof(float), tmp);
	}
	
	qglGenBuffersARB (1, &collision_iboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, collision_iboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, numtriangles*3*sizeof(unsigned int), 0, GL_STATIC_DRAW_ARB);
	
	for	(i = 0; i < numtriangles; i++)
	{
		int tmp[3];
		
		CM_GetTriangle (i, tmp);
		
		qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(i * 3 * sizeof(unsigned int)), 3 * sizeof(unsigned int), tmp);
	}
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void GL_DrawCollisionMesh (void)
{
	GL_BindVBO (collision_vboId);
	R_VertexPointer (3, 0, (void *)0);
	GL_BindVBO (0);
	GL_BindIBO (collision_iboId);
	
	qglDrawElements (GL_TRIANGLES, 3 * num_collision_tris, GL_UNSIGNED_INT, (const GLvoid *)0);
	
	GL_BindIBO (0);
	R_KillVArrays ();
}

static void VB_BuildVegetationVBO (void)
{
	int i, j, ng;
	int totalVBObufferSize = 0;
	int currVertexNum = 0;
	int vbo_idx = 0;
	grass_t *grass;
	
	for (grass = r_grasses, i = 0; i < r_numgrasses; i++, grass++)
		totalVBObufferSize += (grass->type == 1 ? 1 : 3) * 4 * (3 + 2 + 1 + 1 + 1);
	
	qglGenBuffersARB (1, &vegetation_vboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, vegetation_vboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (grass = r_grasses, i = 0; i < r_numgrasses; i++, grass++)
	{
		vec3_t origin;
		vec3_t angle;
		vec3_t up;
		int gcount = grass->type == 1 ? 1 : 3;
		float upscale, rightscale, scale = 10.0f * grass->size;
		
		// up and right appear to be reversed, but actually the
		// vegetation textures are all sideways.
		upscale = scale * (float)grass->tex->crop_width/(float)grass->tex->upload_width;
		rightscale = scale * (float)grass->tex->crop_height/(float)grass->tex->upload_height;
		
		VectorCopy (grass->origin, origin);
		VectorClear (angle);
		VectorSet (up, 0, 0, upscale);
		
		grass->vbo_first_vert = currVertexNum;
		grass->vbo_num_verts = gcount * 4;
		currVertexNum += grass->vbo_num_verts;
		
		for (ng = 0; ng < gcount; ng++)
		{
			int side, v;
			float swayCoef, addright = 0.0f, addup = 0.0f, sumz = 0.0f;
			trace_t r_trace;
			vec3_t right, vertex[2][2];
			vec2_t st;
			AngleVectors (angle, NULL, right, NULL);
			VectorScale (right, rightscale, right);
			
			if (grass->type == 1)
			{
				for (j = 0; j < 4; j++)
					VectorCopy (origin, vertex[j/2][j%2]);
			}
			
			for (side = 1; side >= 0; side--)
			{
				if (grass->type != 1)
				{
					VectorMA (origin, side ? 0.5f : -0.5f, right, vertex[side][0]);
					VectorAdd (vertex[side][0], up, vertex[side][1]);
					VectorSubtract (vertex[side][0], up, vertex[side][0]);
					r_trace = CM_BoxTrace (vertex[side][1], vertex[side][0], vec3_origin, vec3_origin, r_worldmodel->firstnode, MASK_SOLID);
					VectorMA (vertex[side][1], -2.0*r_trace.fraction, up, vertex[side][0]);
					VectorAdd (vertex[side][0], up, vertex[side][1]);
					sumz += vertex[side][1][2]; // Used later for weighted mean
				}
			}
			
			for (side = 1; side >= 0; side--)
			{
				st[1] = side ? grass->tex->sh : grass->tex->sl;
				
				// Level out the top of the sprite. A lower value of
				// STRETCH_WEIGHT means more leveling. 1 is totally level,
				// don't go less than 1. A higher value means the top is more
				// parallel to the bottom.
				#define STRETCH_WEIGHT 1.0f
				if (grass->type != 1)
					vertex[side][1][2] = (vertex[side][1][2] * (STRETCH_WEIGHT - 1.0f) + sumz) / (STRETCH_WEIGHT + 1.0f);
				#undef STRETCH_WEIGHT
				
				for (v = 1; v >= 0; v--)
				{
					st[0] = side != v ? grass->tex->tl : grass->tex->th;
					
					AppendToVBO (vbo_idx, 3 * sizeof(float), vertex[side][side != v]);
					AppendToVBO (vbo_idx, 2 * sizeof(float), st);
					swayCoef = (grass->type == 1 ? 3 : 2) * v;
					AppendToVBO (vbo_idx, sizeof(float), &swayCoef);
					if (grass->type == 1)
					{
						addup = ((side != v) - 0.5f) * rightscale;
						addright = (0.5f - side) * rightscale;
					}
					AppendToVBO (vbo_idx, sizeof(float), &addup);
					AppendToVBO (vbo_idx, sizeof(float), &addright);
				}
			}
			
			angle[1] += 60.0f;
		}
	}
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}

static void VB_BuildLensFlareVBO (void)
{
	int i, j;
	int totalVBObufferSize = 0;
	int currVertexNum = 0;
	int vbo_idx = 0;
	flare_t *flare;
	
	for (flare = r_flares, i = 0; i < r_numflares; i++, flare++)
		totalVBObufferSize += 4 * (3 + 2 + 1 + 1 + 1);
	
	qglGenBuffersARB (1, &lensflare_vboId);
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, lensflare_vboId);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (flare = r_flares, i = 0; i < r_numflares; i++, flare++)
	{
		static float texcoords_and_attributes[4][4] = 
		{
		// 	texcoords	addup	addright
			{0, 1,		-1,		1},
			{0, 0,		-1,		-1},
			{1, 0,		1,		-1},
			{1, 1,		1,		1}
		};
		
		flare->vbo_first_vert = currVertexNum;
		currVertexNum += 4;
		
		for (j = 0; j < 4; j++)
		{
			AppendToVBO (vbo_idx, 3 * sizeof(float), flare->origin);
			AppendToVBO (vbo_idx, 2 * sizeof(float), &texcoords_and_attributes[j][0]);
			AppendToVBO (vbo_idx, sizeof(float), &flare->size);
			AppendToVBO (vbo_idx, 2 * sizeof(float), &texcoords_and_attributes[j][2]);
		}
	}
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}

void VB_BuildWorldVBO (void)
{
	VB_BuildWorldSurfaceVBO ();
	VB_BuildMinimapVBO ();
	VB_BuildVegetationVBO ();
	VB_BuildLensFlareVBO ();
	VB_BuildCollisionVBO ();
	fflush (stdout);
}

void GL_SetupVegetationVBO (void)
{
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, vegetation_vboId);
	
	R_VertexPointer (3, 8*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 8*sizeof(float), (void *)(3*sizeof(float)));
	R_AttribPointer (ATTR_SWAYCOEF_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(5*sizeof(float)));
	R_AttribPointer (ATTR_ADDUP_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(6*sizeof(float)));
	R_AttribPointer (ATTR_ADDRIGHT_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(7*sizeof(float)));
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}

void GL_SetupLensFlareVBO (void)
{
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, lensflare_vboId);
	
	R_VertexPointer (3, 8*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 8*sizeof(float), (void *)(3*sizeof(float)));
	R_AttribPointer (ATTR_SIZE_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(5*sizeof(float)));
	R_AttribPointer (ATTR_ADDUP_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(6*sizeof(float)));
	R_AttribPointer (ATTR_ADDRIGHT_DATA_IDX, 1, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(7*sizeof(float)));
	
	qglBindBufferARB (GL_ARRAY_BUFFER_ARB, 0);
}



// "Special" VBOs: snippets of static geometry that stay unchanged between
// maps.


static GLuint skybox_vbo;

static void VB_BuildSkyboxVBO (void)
{
	// s, t, 1 (the 1 is for convenience)
	static const float skyquad_texcoords[4][3] = 
	{
		{0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}
	};
	
	// 1 = s, 2 = t, 3 = SKYDIST
	static const int	side_to_vec[6][3] =
	{
		{3,-1,-2}, {-3,1,-2},
		{1,3,-2}, {-1,-3,-2},
		{2,-1,3}, {-2,-1,-3}
	};
	
	int i, side, axis, vertnum, coordidx;
	float vertbuf[6 * 4][5], coordcoef;
	
	for (i = 0; i < 4 * 6; i++)
	{
		side = i / 4; 
		vertnum = i % 4;
		for (axis = 0; axis < 3; axis++)
		{
			coordidx = abs (side_to_vec[side][axis]) - 1;
			coordcoef = side_to_vec[side][axis] < 0 ? -SKYDIST : SKYDIST;
			vertbuf[i][axis] = coordcoef * (2.0f * skyquad_texcoords[vertnum][coordidx] - 1.0);
		}
		for (; axis < 5; axis++)
			vertbuf[i][axis] = skyquad_texcoords[vertnum][axis - 3];
	}
	
	qglGenBuffersARB (1, &skybox_vbo);
	GL_BindVBO (skybox_vbo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(vertbuf), vertbuf, GL_STATIC_DRAW_ARB);
	GL_BindVBO (0);
}

void GL_SetupSkyboxVBO (void)
{
	GL_BindVBO (skybox_vbo);
	R_VertexPointer (3, 5*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 5*sizeof(float), (void *)(3*sizeof(float)));
	GL_BindVBO (0);
}


// For fullscreen postprocessing passes, etc. To be rendered in a glOrtho
// coordinate space from 0 to 1 on each axis.
static GLuint wholescreen2D_vbo;

static void VB_BuildWholeScreen2DVBO (void)
{
	static const float vertbuf[4][2][2] = 
	{
	//	vertex coords/texcoords		vertically flipped texcoords
		{{0, 0},					{0, 1}},
		{{1, 0},					{1, 1}},
		{{1, 1},					{1, 0}},
		{{0, 1},					{0, 0}}
	};
	
	qglGenBuffersARB (1, &wholescreen2D_vbo);
	GL_BindVBO (wholescreen2D_vbo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(vertbuf), vertbuf, GL_STATIC_DRAW_ARB);
	GL_BindVBO (0);
}

void GL_SetupWholeScreen2DVBO (wholescreen_drawtype_t drawtype)
{
	GL_BindVBO (wholescreen2D_vbo);
	R_VertexPointer (2, 4*sizeof(float), (void *)0);
	switch (drawtype)
	{
	case wholescreen_blank:
		break;
	case wholescreen_textured:
		R_TexCoordPointer (0, 4*sizeof(float), (void *)0);
		break;
	case wholescreen_fliptextured:
		R_TexCoordPointer (0, 4*sizeof(float), (void *)(2*sizeof(float)));
		break;
	}
	GL_BindVBO (0);
}


// The "diamond" shape used as a default model and also as a marker for 
// debugging/visualization.
static GLuint nullmodel_vbo, nullmodel_ibo;

static void VB_BuildNullModelVBO (void)
{
	static const float vertbuf[6][3] = 
	{
		{0, 0, -1},
		{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0},
		{0, 0, 1}
	};
	
	static const unsigned int indices[3*8] =
	{
		0, 1, 2,	0, 2, 3,	0, 3, 4,	0, 4, 1,
		5, 1, 4,	5, 4, 3,	5, 3, 2,	5, 2, 1
	};
	
	qglGenBuffersARB (1, &nullmodel_vbo);
	GL_BindVBO (nullmodel_vbo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(vertbuf), vertbuf, GL_STATIC_DRAW_ARB);
	qglGenBuffersARB (1, &nullmodel_ibo);
	GL_BindVBO (nullmodel_ibo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(indices), indices, GL_STATIC_DRAW_ARB);
	GL_BindVBO (0);
}

void GL_DrawNullModel (void)
{
	GL_BindVBO (nullmodel_vbo);
	R_VertexPointer (3, 0, (void *)0);
	GL_BindVBO (0);
	GL_BindIBO (nullmodel_ibo);
	qglDrawElements (GL_TRIANGLES, 3*8, GL_UNSIGNED_INT, 0);
	GL_BindIBO (0);
}


void VB_WorldVCInit (void)
{
	//clear out previous buffer
	qglDeleteBuffersARB (1, &bsp_vboId);
	qglDeleteBuffersARB (1, &bsp_iboId);
	qglDeleteBuffersARB (1, &bsp_outlines_iboId);
	qglDeleteBuffersARB (1, &minimap_vboId);
}

static void VB_VCInit (void)
{
	VB_WorldVCInit ();
	
	// "Special" VBOs
	VB_BuildSkyboxVBO ();
	VB_BuildWholeScreen2DVBO ();
	VB_BuildNullModelVBO ();
}

void R_VCShutdown (void)
{
	// delete buffers
	VB_WorldVCInit ();
	qglDeleteBuffersARB (1, &skybox_vbo);
	qglDeleteBuffersARB (1, &wholescreen2D_vbo);
	qglDeleteBuffersARB (1, &nullmodel_vbo);
	qglDeleteBuffersARB (1, &nullmodel_ibo);
}
