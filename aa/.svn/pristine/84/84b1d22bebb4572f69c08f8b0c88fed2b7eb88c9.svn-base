/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2011-2014 COR Entertainment, LLC.

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
// R_SURF.C: surface-related refresh code

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "r_local.h"

static vec3_t	modelorg;		// relative to viewpoint

vec3_t	r_worldLightVec;

#define LIGHTMAP_BYTES 4

int		c_visible_lightmaps;
int		c_visible_textures;

// This is supposed to be faster on some older hardware.
#define GL_LIGHTMAP_FORMAT GL_BGRA 

typedef struct
{
	int	current_lightmap_texture;

	// For each column, what is the last row where a pixel is used
	int			allocated[LIGHTMAP_SIZE];

	// Lightmap texture data (RGBA, alpha not used)
	byte		lightmap_buffer[4*LIGHTMAP_SIZE*LIGHTMAP_SIZE];
} gllightmapstate_t;

// TODO: dynamically allocate this so we can free it for RAM savings? It's
// using over 16 megs. 
static gllightmapstate_t gl_lms; 

static void		LM_InitBlock( void );
static void		LM_UploadBlock( );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap (msurface_t *surf, byte *dest, int smax, int tmax, int stride);

/*
===============
BSP_TextureAnimation

Returns the proper texture for a given time and base texture
XXX: AFAIK this is only used for the old .wal textures, and is a bit redundant
with the rscript system, although it is implemented more efficiently. Maybe 
merge the two systems somehow?
===============
*/
static image_t *BSP_TextureAnimation (mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}



/*
=========================================

VBO batching

This system allows contiguous sequences of polygons to be merged into batches,
even if they are added out of order.

A batch is a sequence of surfaces which are contiguous in VBO allocation order
and can thus be rendered with a single draw call. The first surface of a batch
has a pointer to the last surface, and the last surface of a batch has a
pointer to the first surface.

There are no pointers to/from intermmediate surfaces of a batch, they are
implicitly included in the batch because they are allocated between the first
and last surface within the VBO.

A batch with only one surface in it will have its batch_end and batch_start
pointers pointing to itself.

There is a double-linked non-circular list of all batches of surfaces. The
batches are not added to the linked list in any particular order. However,
within a batch, the surfaces are ordered quite strictly.

The first surface of a batch is responsible for containing a pointer to last
surface of the the previous batch. The last surface of a batch is responsible
for containing a pointer to the first surface of the next batch.

=========================================
*/

// pointer to the first surface of the first batch
static msurface_t *first_vbobatch_start = NULL;
// if false, drawVBOAccum will set up the VBO GL state
qboolean	r_vboOn = false; 
// for the rspeed_vbobatches HUD gauge
int			c_vbo_batches;

// Call this if you've bound the vertex/texture pointers to something else and
// they will need to be re-bound to the BSP VBO.
void BSP_InvalidateVBO (void)
{
	r_vboOn = false;
}

// render all accumulated surfaces
void BSP_DrawVBOAccum (void)
{
	extern GLuint bsp_iboId;
	msurface_t *batch = first_vbobatch_start;
	
	if (!batch)
		return;
	
	if (!r_vboOn)
	{
		GL_SetupWorldVBO ();
		r_vboOn = true;
	}
	
	GL_BindIBO (bsp_iboId);
	
	for (; batch; batch = batch->batch_end->batch_next)
	{
		qglDrawElements (GL_TRIANGLES, batch->batch_end->ibo_last_idx - batch->ibo_first_idx, GL_UNSIGNED_INT, (const GLvoid *)(sizeof (unsigned int) * batch->ibo_first_idx));
		c_vbo_batches++;
	}
	
	GL_BindIBO (0);
}

static void BSP_DrawVBOAccum_Outlines (void)
{
	extern GLuint bsp_outlines_iboId;
	msurface_t *batch = first_vbobatch_start;
	
	if (!batch)
		return;
	
	if (!r_vboOn)
	{
		GL_SetupWorldVBO ();
		r_vboOn = true;
	}
	
	GL_BindIBO (bsp_outlines_iboId);
	
	for (; batch; batch = batch->batch_end->batch_next)
	{
		qglDrawElements (GL_LINES, batch->batch_end->ibo_last_outline_idx - batch->ibo_first_outline_idx, GL_UNSIGNED_INT, (const GLvoid *)(sizeof (unsigned int) * batch->ibo_first_outline_idx));
		c_vbo_batches++;
	}
	
	GL_BindIBO (0);
}

// clear all accumulated surfaces without rendering
void BSP_ClearVBOAccum (void)
{
	msurface_t *batch = first_vbobatch_start;
	
	if (!batch)
		return;
	
	for (; batch; batch = batch->batch_end->batch_next)
	{
		batch->batch_flags = 0;
		batch->batch_end->batch_flags = 0;
	}
	
	first_vbobatch_start = NULL;
}

// render all accumulated surfaces, then clear them
// XXX: assumes that global OpenGL state is correct for whatever surfaces are
// in the accumulator, so don't change state until you've called this!
void BSP_FlushVBOAccum (void)
{
	BSP_DrawVBOAccum ();
	BSP_ClearVBOAccum ();
}

// Add a new surface to the VBO batch accumulator. If its vertex data is next
// to any other surfaces within the VBO, opportunistically merge the surfaces
// together into a larger "batch," so they can be rendered with one draw call. 
// Whenever two batches touch each other, they are merged. Hundreds of
// surfaces can be rendered with a single call, which is easier on the OpenGL
// pipeline.
void BSP_AddSurfToVBOAccum (msurface_t *surf)
{
	// the start and end of the new batch we will be inserting
	msurface_t *new_batch_start, *new_batch_end;
	// surfaces immediately before and after this in VBO alloc order
	msurface_t *prev = surf->vboprev, *next = surf->vbonext;
	
	// If the surface is right after an existing batch, append this surface to
	// that batch.
	if (prev != NULL && (prev->batch_flags & BATCH_END))
	{
		// unlink modified batch from list of batches
		if (prev->batch_start->batch_prev)
			prev->batch_start->batch_prev->batch_next = prev->batch_next;
		if (prev->batch_next)
			prev->batch_next->batch_prev = prev->batch_start->batch_prev;
		if (prev->batch_start == first_vbobatch_start)
			first_vbobatch_start = prev->batch_next;
		// mark "prev" as no longer the end of a batch
		prev->batch_flags &= ~BATCH_END;
		// new batch will start where this old batch used to start
		new_batch_start = prev->batch_start;
	}
	else
	{
		// Current surface is not right after an existing batch, therefore it
		// will become the beginning of a new batch.
		new_batch_start = surf;
	}
	
	// If the surface is right before an existing batch, prepend this surface
	// to that batch.
	if (next != NULL && (next->batch_flags & BATCH_START))
	{
		// unlink modified batch from list of batches
		if (next->batch_end->batch_next)
			next->batch_end->batch_next->batch_prev = next->batch_prev;
		if (next->batch_prev)
			next->batch_prev->batch_next = next->batch_end->batch_next;
		if (next == first_vbobatch_start)
			first_vbobatch_start = next->batch_end->batch_next;
		// mark "next" as no longer the start of a batch
		next->batch_flags &= ~BATCH_START;
		// new batch will end where this old batch used to end
		new_batch_end = next->batch_end;
	}
	else
	{
		// Current surface is not right before an existing batch, therefore it
		// will become the end of a new batch.
		new_batch_end = surf;
	}
	
	// Mark start and end of new batch
	new_batch_start->batch_flags |= BATCH_START;
	new_batch_end->batch_flags |= BATCH_END;
	// Link start and end of new batch
	new_batch_start->batch_end = new_batch_end;
	new_batch_end->batch_start = new_batch_start;
	// Insert new batch at the beginning of the linked list of all batches
	if (first_vbobatch_start)
		first_vbobatch_start->batch_prev = new_batch_end;
	new_batch_end->batch_next = first_vbobatch_start;
	new_batch_start->batch_prev = NULL;
	first_vbobatch_start = new_batch_start;
}



/*
=========================================

Textureless Surface Rendering
Used by the shadow system

=========================================
*/

static void BSP_DrawTexturelessInlineBModel (entity_t *e)
{
	int			i;
	msurface_t	*psurf;

	BSP_InvalidateVBO (); // FIXME: why doesn't it work without this?
	
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{
		// draw the polygon
		BSP_AddSurfToVBOAccum (psurf);
		psurf->visframe = r_framecount;
	}
	
	BSP_FlushVBOAccum ();
}

void BSP_DrawTexturelessBrushModel (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, currentmodel->mins, mins);
		VectorAdd (e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs)) {
		return;
	}

	VectorSubtract (r_newrefdef.vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	qglPushMatrix ();
	R_RotateForEntity (e);

	BSP_DrawTexturelessInlineBModel (e);

	qglPopMatrix ();
}



/*
=========================================

BSP Surface Rendering
Common between brush and world models

=========================================
*/


/*
=========================================
Special surfaces - Somewhat less common, require more work to render
 - Translucent ("alpha") surfaces
   These are special because they have to be rendered all in one pass, despite
   consisting of several different types of surfaces, so the code can't make 
   too many assumptions about the surface.
 - Rscript surfaces (those with material shaders)
   Rscript surfaces are first rendered through the "ordinary" path, then 
   this one.
 - Wavy, rippling ("warp") surfaces
   The code to actually render these is in r_warp.c.
=========================================
*/


// The "special" surfaces use these for linked lists.
// The reason to have linked lists for surfaces from brush model entities
// separate from the linked lists for world surfaces is that the world
// surface linked lists can be preserved between frames if r_optimize is on,
// whereas the entity linked lists must be cleared each time an entity is
// drawn.
static surfchain_t	r_alpha_surfaces;
static surfchain_t	r_warp_surfaces;
static msurface_t  *r_caustic_surfaces; // no brush models can have caustic surfs

// This is a chain of surfaces that may need to have their lightmaps updated.
// They are not rendered in the order of this chain and will be linked into
// other chains for rendering.
static msurface_t	*r_flicker_surfaces;


/*
================
BSP_SetScrolling
================
*/
static void BSP_SetScrolling (qboolean enable)
{
	extern qboolean rs_in_group; // Sigh
	GL_SelectTexture (0);
	if (!rs_in_group)
		qglMatrixMode (GL_TEXTURE);
	qglLoadIdentity ();
	if (enable)
	{
		float scroll;
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
		qglTranslatef (scroll, 0, 0);
	}
	if (!rs_in_group)
		qglMatrixMode (GL_MODELVIEW);
}


/*
================
BSP_DrawWarpSurfaces
================
*/
static void BSP_DrawWarpSurfaces (qboolean forEnt)
{
	msurface_t	*surf;
	image_t		*image;
	
	if (forEnt)
		surf = r_warp_surfaces.entchain;
	else
		surf = r_warp_surfaces.worldchain;
	
	if (surf == NULL)
		return;
	
	// no lightmaps rendered on these surfaces
	GL_EnableTexture (1, false);
	GL_MTexEnv (0, GL_MODULATE);
	qglColor4f( gl_state.inverse_intensity,
				gl_state.inverse_intensity,
				gl_state.inverse_intensity,
				1.0F );
	while (surf)
	{
		c_brush_polys++;
		image = BSP_TextureAnimation (surf->texinfo);
		GL_MBind (0, image->texnum);
		BSP_SetScrolling ((surf->texinfo->flags & SURF_FLOWING) != 0);
		R_RenderWaterPolys (surf);
		surf = surf->texturechain;
	}
	
	BSP_SetScrolling (0);
	
	if (forEnt)
		r_warp_surfaces.entchain = NULL;
	
	GL_EnableTexture (1, true);
	GL_MTexEnv (0, GL_REPLACE);
	R_KillVArrays ();
}

/*
================
BSP_DrawAlphaPoly
================
*/
static void BSP_DrawAlphaPoly (msurface_t *surf)
{
	BSP_AddSurfToVBOAccum (surf);
	BSP_FlushVBOAccum (); // Must flush to guarantee draw order :(
}

/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.

Annoyingly, because alpha surfaces have to be drawn from back to front, 
everything transparent-- water, rscripted surfs, and non-rscripted surfs-- has
to be drawn in a single pass. This is an inherently inefficient process.

The BSP tree is walked front to back, so unwinding the chain of alpha surfaces
will draw back to front, giving proper ordering FOR BSP SURFACES! 

It's a bit wrong for entity surfaces (i.e. glass doors.) Because they are in
separate linked lists, the entity surfaces must be either always behind or
always in front of the world surfaces. I chose always in front.

Be sure to double-check alphasurftest THOROUGHLY when changing this code!
================
*/
extern rscript_t *rs_caustics;
static void R_DrawAlphaSurfaces_chain (msurface_t *chain)
{
	msurface_t	*s;
	float		intens;
	int			last_surf_type = -1;
	mtexinfo_t	*last_texinfo = NULL;

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = gl_state.inverse_intensity;
	
	qglDepthMask ( GL_FALSE );
	GLSTATE_ENABLE_BLEND
	GL_MTexEnv (0, GL_MODULATE);
	BSP_InvalidateVBO ();
	
	for (s=chain ; s ; s=s->texturechain)
	{
		int			current_surf_type;
		mtexinfo_t	*current_texinfo = s->texinfo;
		
		GL_MBind (0, s->texinfo->image->texnum);
		c_brush_polys++;

		//moving trans brushes
		if (s->entity)
		{
			qglLoadMatrixf (r_world_matrix); //moving trans brushes
			R_RotateForEntity (s->entity);
		}
		
		if (s->iflags & ISURF_DRAWTURB)
			current_surf_type = 0;
		else if (r_shaders->integer && s->texinfo->image->script && !(s->texinfo->flags & SURF_FLOWING)) 
			current_surf_type = 1;
		else
			current_surf_type = 2;
		
		if (current_surf_type != last_surf_type || current_texinfo != last_texinfo)
		{
			GLSTATE_ENABLE_BLEND
			GL_MTexEnv (0, GL_MODULATE);
			BSP_SetScrolling ((s->texinfo->flags & SURF_FLOWING) != 0);
			
			if (s->texinfo->flags & SURF_TRANS33)
				qglColor4f (intens, intens, intens, 0.33);
			else if (s->texinfo->flags & SURF_TRANS66)
				qglColor4f (intens, intens, intens, 0.66);
			else
				qglColor4f (intens, intens, intens, 1);
			
			last_surf_type = current_surf_type;
			last_texinfo = current_texinfo;
		}
		
		if (current_surf_type == 0)
			R_RenderWaterPolys (s);
		else if (current_surf_type == 1)
			RS_DrawSurface (s, s->texinfo->image->script);
		else
			BSP_DrawAlphaPoly (s);
		
		if ((s->iflags & ISURF_UNDERWATER) != 0)
		{
			RS_DrawSurface (s, rs_caustics);
			// HACK: since we just called RS_DrawSurface, it's as if this
			// surface type was an rscript surface, even if it wasn't.
			last_surf_type = 1;
		}
	}

	R_KillVArrays ();
	BSP_SetScrolling (0);
	GL_MTexEnv (0, GL_MODULATE);
	qglColor4f (1,1,1,1);
	GLSTATE_DISABLE_BLEND
	qglDepthMask (GL_TRUE);
}

void R_DrawAlphaSurfaces (void)
{
	if (r_drawworld->integer)
		R_DrawAlphaSurfaces_chain (r_alpha_surfaces.worldchain);
	if (r_drawentities->integer)
		R_DrawAlphaSurfaces_chain (r_alpha_surfaces.entchain);
	qglLoadMatrixf (r_world_matrix); //moving trans brushes
	r_alpha_surfaces.entchain = NULL;
}

/*
================
R_DrawRSSurfaces

Draw shader surfaces
TODO: rscript surfaces on brush models?
================
*/
static void R_DrawRSSurfaces (void)
{
	int i;
	
	qglDepthMask(false);
	qglShadeModel (GL_SMOOTH);

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(-3, -2);
	
	BSP_InvalidateVBO ();
	RS_Begin_Group (currententity);
	
	if (r_caustic_surfaces != NULL)
	{
		int currLMTex; 
		msurface_t	*s = r_caustic_surfaces;
		
		while (s != NULL)
		{
			currLMTex = s->lightmaptexturenum;
			
			for (; s && s->lightmaptexturenum == currLMTex; s = s->causticchain)
				BSP_AddSurfToVBOAccum (s);
			
			RS_Draw (	rs_caustics, currententity,
						gl_state.lightmap_textures + currLMTex,
						vec3_origin, vec3_origin, false,
						rs_lightmap_separate_texcoords, 
						false, false, BSP_DrawVBOAccum );
			
			BSP_ClearVBOAccum ();
		}
	}

	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		rscript_t	*rs;
		msurface_t	*s;
		
		rs = currentmodel->unique_texinfo[i]->image->script;
		if (!r_shaders->integer || rs == NULL || !(rs->flags & RS_CONTAINS_DRAWN))
			continue;
		
		BSP_SetScrolling ((currentmodel->unique_texinfo[i]->flags & SURF_FLOWING) != 0);
		
		if ((rs->flags & RS_PREVENT_BATCH) != 0)
		{
			// fall back on drawing the surfaces one at a time
			for (s = currentmodel->unique_texinfo[i]->standard_surfaces.worldchain; s; s = s->texturechain)
				RS_DrawSurface (s, rs);
			for (s = currentmodel->unique_texinfo[i]->dynamic_surfaces.worldchain; s; s = s->texturechain)
				RS_DrawSurface (s, rs);
		}
		else
		{
			int j;
			int currLMTex; // still need to separate batches by lightmap texture
			
			for (j = 0; j < 2; j++)
			{
				if (j)
					s = currentmodel->unique_texinfo[i]->standard_surfaces.worldchain;
				else
					s = currentmodel->unique_texinfo[i]->dynamic_surfaces.worldchain;
			
				while (s != NULL)
				{
					currLMTex = s->lightmaptexturenum;
				
					for (; s && s->lightmaptexturenum == currLMTex; s = s->texturechain)
						BSP_AddSurfToVBOAccum (s);
				
					RS_Draw (	rs, currententity,
								gl_state.lightmap_textures + currLMTex,
								vec3_origin, vec3_origin, false,
								rs_lightmap_separate_texcoords, 
								true, true, BSP_DrawVBOAccum );
				
					BSP_ClearVBOAccum ();
				}
			}
		}
	}
	
	BSP_SetScrolling (0);
	
	RS_End_Group ();
	R_KillVArrays ();

	qglDisable(GL_POLYGON_OFFSET_FILL);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST

	qglDepthMask(true);
}



/*
=========================================
Ordinary surfaces (fixed-function, normalmapped, and dynamically lit)
These are the most commonly used types of surfaces, so the rendering code path
for each is more optimized-- surfaces grouped by texinfo, VBOs, VBO batching,
etc.
=========================================
*/

// The "ordinary" surfaces do not use global variables for linked lists. The
// linked lists are in the texinfo struct, so the textures can be grouped by
// texinfo. Like the "special" surfaces, there are separate linked lists for
// entity surfaces and world surfaces.

// State variables for detecting changes from one surface to the next. If any
// of these change, the current batch of polygons to render is flushed. This
// helps minimize GL state change calls and draw calls.
int 		r_currLMTex = -9999; // only bind a lightmap texture if it is not
								 // the same as previous surface
mtexinfo_t	*r_currTexInfo = NULL; //texinfo struct

/*
================
R_SetLightingMode

Setup the fixed-function pipeline with texture combiners to enable rendering
of lightmapped surfaces. For GLSL renders, this is unnecessary, as the shader
handles this job.
================
*/
void R_SetLightingMode (void)
{
	GL_SelectTexture (0);
	GL_TexEnv ( GL_COMBINE_EXT );
	
	qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
	qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
	qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
	qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );

	GL_SelectTexture (1);
	GL_TexEnv (GL_COMBINE_EXT);
	if (gl_lightmap->integer) 
	{
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
	} 
	else 
	{
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
	}

	if (r_overbrightbits->value != 0.0)
		qglTexEnvi ( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, r_overbrightbits->value );
}

/*
================
BSP_TexinfoChanged

Update GL state as needed so we can draw a new batch of surfaces for the 
provided texinfo 
================
*/
static void BSP_TexinfoChanged (mtexinfo_t *texinfo, qboolean glsl, int dynamic)
{
	int		texnum;
	
	BSP_FlushVBOAccum ();
	
	if (TexinfoIsAlphaMasked (texinfo))
	{
		if (!r_currTexInfo || !TexinfoIsAlphaMasked (r_currTexInfo))
			GLSTATE_ENABLE_ALPHATEST
	}
	else
	{
		if (!r_currTexInfo || TexinfoIsAlphaMasked (r_currTexInfo))
			GLSTATE_DISABLE_ALPHATEST
	}
	
	if (glsl)
		// no texture animation for normalmapped surfaces, for some reason
		texnum = texinfo->image->texnum;
	else
		// do this here so only have to do it once instead of for each surface
		texnum = BSP_TextureAnimation( texinfo )->texnum;
	
	GL_MBind (0, texnum);
	
	// scrolling is done using the texture matrix
	if (	!r_currTexInfo || (texinfo->flags & SURF_FLOWING) ||
			(r_currTexInfo->flags & SURF_FLOWING))
		BSP_SetScrolling ((texinfo->flags & SURF_FLOWING) != 0);
	
	if (!glsl)
	{
		r_currTexInfo = texinfo;
		return;
	}
	
	GL_MBind (2, texinfo->heightMap->texnum);
	GL_MBind (3, texinfo->normalMap->texnum);
	
		if (dynamic)
	{
		if(r_worldnormalmaps->integer && texinfo->has_heightmap) 
		{
			if (!r_currTexInfo || !r_currTexInfo->has_heightmap)
			{
				glUniform1iARB (worldsurf_uniforms[dynamic].parallax, 1);
			}
		}
		else
		{
			if (!r_currTexInfo || r_currTexInfo->has_heightmap)
			{
				glUniform1iARB (worldsurf_uniforms[dynamic].parallax, 0);
			}
		}
	}
	
	if (!r_worldnormalmaps->integer)
	{
		if (!r_currTexInfo)
		{
			glUniform1iARB (worldsurf_uniforms[dynamic].liquid, 0);
			glUniform1iARB (worldsurf_uniforms[dynamic].shiny, 0);
		}
	}
	else if	(r_currTexInfo &&
			(texinfo->flags & (SURF_BLOOD|SURF_WATER|SURF_SHINY)) == 
			(r_currTexInfo->flags & (SURF_BLOOD|SURF_WATER|SURF_SHINY)))
	{
		//no change to GL state is needed
	}
	else if (texinfo->flags & SURF_BLOOD) 
	{
		//need to bind the blood drop normal map, and set flag, and time
		glUniform1iARB (worldsurf_uniforms[dynamic].liquid, 8); //blood type 8, water 1
		glUniform1iARB (worldsurf_uniforms[dynamic].shiny, 0);
		glUniform1fARB (worldsurf_uniforms[dynamic].rsTime, rs_realtime);
		glUniform1iARB (worldsurf_uniforms[dynamic].liquidTexture, 4); //for blood we are going to need to send a diffuse texture with it
		GL_MBind (4, r_blooddroplets->texnum);
		glUniform1iARB (worldsurf_uniforms[dynamic].liquidNormTex, 5); 
		GL_MBind (5, r_blooddroplets_nm->texnum);
	}
	else if (texinfo->flags & SURF_WATER) 
	{
		//need to bind the water drop normal map, and set flag, and time
		glUniform1iARB (worldsurf_uniforms[dynamic].liquid, 1); 
		glUniform1iARB (worldsurf_uniforms[dynamic].shiny, 0);
		glUniform1fARB (worldsurf_uniforms[dynamic].rsTime, rs_realtime);
		glUniform1iARB (worldsurf_uniforms[dynamic].liquidNormTex, 4); //for blood we are going to need to send a diffuse texture with it(maybe even height!)
		GL_MBind (4, r_droplets->texnum);
	}
	else if (texinfo->flags & SURF_SHINY)
	{
		glUniform1iARB (worldsurf_uniforms[dynamic].liquid, 0);
		glUniform1iARB (worldsurf_uniforms[dynamic].shiny, 1);

		glUniform1iARB (worldsurf_uniforms[dynamic].chromeTex, 4);
		GL_MBind (4, r_mirrorspec->texnum);
	}
	else if (!r_currTexInfo || r_currTexInfo->flags & (SURF_BLOOD|SURF_WATER|SURF_SHINY))
	{
		glUniform1iARB (worldsurf_uniforms[dynamic].liquid, 0);
		glUniform1iARB (worldsurf_uniforms[dynamic].shiny, 0);
	}
	
	r_currTexInfo = texinfo;
}

/*
================
BSP_RenderLightmappedPoly

Main polygon rendering routine (all standard surfaces)
================
*/
static void BSP_RenderLightmappedPoly( msurface_t *surf, qboolean glsl)
{
	unsigned lmtex = surf->lightmaptexturenum;
		
	c_brush_polys++;
	
	if (lmtex != r_currLMTex)
	{
		BSP_FlushVBOAccum ();
		GL_MBind (1, gl_state.lightmap_textures + lmtex);
		r_currLMTex = lmtex;
	}
	
	// If we've gotten this far, it's because the surface is not translucent,
	// warped, sky, or nodraw, thus it *will* be in the VBO.
	BSP_AddSurfToVBOAccum (surf);
}

static void BSP_DrawNonGLSLSurfaces (qboolean forEnt)
{
	int		 i;

	// reset VBO batching state
	r_currLMTex = -99999;
	r_currTexInfo = NULL;
	
	BSP_FlushVBOAccum ();
	BSP_InvalidateVBO ();
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		msurface_t	*s;
		
		if (r_worldnormalmaps->integer &&
			currentmodel->unique_texinfo[i]->has_heightmap &&
			currentmodel->unique_texinfo[i]->has_normalmap)
			continue;
		
		if (forEnt)
		{
			s = currentmodel->unique_texinfo[i]->standard_surfaces.entchain;
			currentmodel->unique_texinfo[i]->standard_surfaces.entchain = NULL;
		}
		else
		{
			s = currentmodel->unique_texinfo[i]->standard_surfaces.worldchain;
		}
		if (!s)
			continue;
		BSP_TexinfoChanged (s->texinfo->equiv, false, 0);
		for (; s; s = s->texturechain)
			BSP_RenderLightmappedPoly(s, false);
	}
	
	BSP_FlushVBOAccum ();
	
	GLSTATE_DISABLE_ALPHATEST
}

// FIXME: figure out what this does and clean it up!
static void BSP_SetupGLSL (int dynamic)
{
	glUseProgramObjectARB (g_worldprogramObj[dynamic]);
	glUniform1iARB (worldsurf_uniforms[dynamic].fog, map_fog);
	glUniform3fARB (worldsurf_uniforms[dynamic].staticLightPosition, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);
	glUniform1iARB (worldsurf_uniforms[dynamic].surfTexture, 0);
	glUniform1iARB (worldsurf_uniforms[dynamic].lmTexture, 1);
	glUniform1iARB (worldsurf_uniforms[dynamic].heightTexture, 2);
	glUniform1iARB (worldsurf_uniforms[dynamic].normalTexture, 3);

	R_SetShadowmapUniforms (&worldsurf_uniforms[dynamic].shadowmap_uniforms, 6, true);
	if (dynamic)
		R_SetDlightUniforms (&worldsurf_uniforms[dynamic].dlight_uniforms);

	glUniform1iARB (worldsurf_uniforms[dynamic].parallax, 1);
}

static void BSP_DrawGLSLSurfaces (qboolean forEnt)
{
	int		 i;

	if (!r_worldnormalmaps->integer)
	{
		return;
	}
	
	// reset VBO batching state
	r_currLMTex = -99999;
	r_currTexInfo = NULL;
	
	BSP_ClearVBOAccum ();
	
	BSP_SetupGLSL (0);
	
	BSP_InvalidateVBO ();
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		msurface_t	*s;
		
		if (!r_worldnormalmaps->integer ||
			!currentmodel->unique_texinfo[i]->has_heightmap ||
			!currentmodel->unique_texinfo[i]->has_normalmap)
			continue;
		
		if (forEnt)
		{
			s = currentmodel->unique_texinfo[i]->standard_surfaces.entchain;
			currentmodel->unique_texinfo[i]->standard_surfaces.entchain = NULL;
		}
		else
		{
			s = currentmodel->unique_texinfo[i]->standard_surfaces.worldchain;
		}
		if (!s)
			continue;
		BSP_TexinfoChanged (s->texinfo->equiv, true, 0);
		for (; s; s = s->texturechain)
			BSP_RenderLightmappedPoly(s, true);
	}
	
	BSP_FlushVBOAccum ();

	GLSTATE_DISABLE_ALPHATEST
}

static void BSP_DrawGLSLDynamicSurfaces (qboolean forEnt)
{
	int		 i;
	
	if (gl_dynamic->integer == 0)
	{
		return;
	}
	
	// reset VBO batching state
	r_currLMTex = -99999;
	r_currTexInfo = NULL;
	
	BSP_ClearVBOAccum ();
	
	BSP_SetupGLSL (CUR_NUM_DLIGHTS);
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		msurface_t	*s;
		if (forEnt)
		{
			s = currentmodel->unique_texinfo[i]->dynamic_surfaces.entchain;
			currentmodel->unique_texinfo[i]->dynamic_surfaces.entchain = NULL;
		}
		else
		{
			s = currentmodel->unique_texinfo[i]->dynamic_surfaces.worldchain;
		}
		if (!s)
			continue;
		BSP_TexinfoChanged (s->texinfo->equiv, true, CUR_NUM_DLIGHTS);
		for (; s; s = s->texturechain)
			BSP_RenderLightmappedPoly(s, true);
	}
	
	BSP_FlushVBOAccum ();
	
	GLSTATE_DISABLE_ALPHATEST
}

static void BSP_DrawSurfaceWireframes (qboolean forEnt)
{
	int		 i;
	
	/*
	 * Mapping tool. Outline the light-mapped polygons.
	 *  gl_showpolys == 1 : perform depth test.
	 *  gl_showpolys == 2 : disable depth test. everything in "visible set"
	 * gl_showtris is identical, but it shows the fully triangulated mesh
	 * instead of surface outlines. Both are restricted to servers with
	 * maxclients == 1.
	 */
	if (!gl_showpolys->integer && !gl_showtris->integer)
		return;
		
	BSP_FlushVBOAccum ();
	BSP_InvalidateVBO ();
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		msurface_t	*s;
		
		if (forEnt)
			s = currentmodel->unique_texinfo[i]->standard_surfaces.entchain;
		else
			s = currentmodel->unique_texinfo[i]->standard_surfaces.worldchain;
		for (; s; s = s->texturechain)
			BSP_AddSurfToVBOAccum (s);
		
		if (forEnt)
			s = currentmodel->unique_texinfo[i]->dynamic_surfaces.entchain;
		else
			s = currentmodel->unique_texinfo[i]->dynamic_surfaces.worldchain;
		for (; s; s = s->texturechain)
			BSP_AddSurfToVBOAccum (s);
	}
	
	qglColor4f (1.0f, 1.0f, 1.0f, 1.0f);
	qglDisable (GL_TEXTURE_2D);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	
	if (gl_showtris->integer)
	{
		if (gl_showtris->integer >= 2) qglDisable (GL_DEPTH_TEST);
		qglLineWidth (gl_showtris->integer >= 2 ? 2.5f : 1.5f); // when there are lots of lines, make them narrower
		
		BSP_DrawVBOAccum ();
		
		if (gl_showtris->integer >= 2) qglEnable (GL_DEPTH_TEST);
	}
	
	if (gl_showpolys->integer)
	{
		if (gl_showpolys->integer >= 2) qglDisable (GL_DEPTH_TEST);
		qglLineWidth (gl_showtris->integer >= 2 ? 3.0f : 2.0f); // when there are lots of lines, make them narrower
		
		BSP_DrawVBOAccum_Outlines ();
		
		if (gl_showpolys->integer >= 2) qglEnable (GL_DEPTH_TEST);
	}
	
	BSP_ClearVBOAccum ();

	qglLineWidth (1.0f);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglEnable (GL_TEXTURE_2D);
}




/*
=========================================
This is the "API" for the BSP surface renderer backend, hiding most of the
complexity of the previous functions. 
=========================================
*/


/*
================
BSP_ClearWorldTextureChains

Reset linked lists for the world (non-entity/non-brushmodel) surfaces. This 
need not be called every frame if r_optimize is on. No equivalent function is
needed for entity surfaces because they are reset automatically when they are
drawn.
================
*/
static void BSP_ClearWorldTextureChains (void)
{
	int i;
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		currentmodel->unique_texinfo[i]->standard_surfaces.worldchain = NULL;
		currentmodel->unique_texinfo[i]->dynamic_surfaces.worldchain = NULL;
	}
	
	r_warp_surfaces.worldchain = NULL;
	r_alpha_surfaces.worldchain = NULL;
	r_caustic_surfaces = NULL;
	r_flicker_surfaces = NULL;
}

/*
================
BSP_AddToTextureChain

Call this on a surface (and indicate whether it's an entity surface) and it 
will figure out which texture chain to add it to; this function is responsible
for deciding if the surface is "ordinary" or somehow "special," whether it
should be normalmapped and/or dynamically lit, etc.

This function will be repeatedly called on all the surfaces in the current 
brushmodel entity or in the world's static geometry, followed by a single call
to BSP_DrawTextureChains to render them all in one fell swoop.
================
*/
static void BSP_UpdateSurfaceLightmap (msurface_t *surf);
static void BSP_AddToTextureChain(msurface_t *surf, qboolean forEnt)
{
	int			map;
	msurface_t	**chain;
	qboolean	is_dynamic = false;
	
	// Since we now draw the whole skybox all the time anyway, no need to do
	// anything special with these surfaces.
	if (surf->texinfo->flags & SURF_SKY)
		return;
	
// The do-while is an old preprocessor trick to allow it to be controlled by
// if-statements and ended with a semicolon.
#define AddToChainPair(chainpair) \
	do { \
		chain = (forEnt?\
					&((chainpair).entchain):\
					&((chainpair).worldchain));\
		surf->texturechain = *chain; \
		*chain = surf; \
	} while (0) 
	
	if (SurfaceIsTranslucent(surf) && !SurfaceIsAlphaMasked (surf))
	{	// add to the translucent chain
		AddToChainPair (r_alpha_surfaces);
		return;
	}
	
	if (surf->iflags & ISURF_DRAWTURB)
	{	// add to the warped surfaces chain
		AddToChainPair (r_warp_surfaces);
		return;
	}
	
	
	// The rest of the function handles most ordinary surfaces: normalmapped,
	// non-normalmapped, and dynamically lit surfaces. As these three cases 
	// are the most common, they are the most optimized-- grouped by texinfo,
	// etc. Note that with alpha, warp, and sky surfaces out of the way, all
	// the remaining surfaces have lightmaps.

	// XXX: we could require r_worldnormalmaps here, but that would result in
	// weird inconsistency with only meshes lighting up. Better to fall back
	// on GLSL for dynamically lit surfaces, even with r_worldnormalmaps 0.
	if(r_newrefdef.num_dlights && gl_dynamic->integer)
	{
		// Dynamic surfaces must have normalmaps, as the old fixed-function
		// texture-based dynamic lighting system is deprecated.
		is_dynamic = (surf->dlightframe == r_framecount && surf->texinfo->has_normalmap);
	}
	
	// reviving the ancient lightstyle system
	for ( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
	{
		// Chain of surfaces that may need to have their lightmaps updated
		// in future frames (for dealing with r_optimize)
		if (surf->styles[map] != 0)
		{
			if (!forEnt)
			{
				surf->flickerchain = r_flicker_surfaces;
				r_flicker_surfaces = surf;
				break;
			}
			if ( r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			{
				BSP_UpdateSurfaceLightmap (surf);
				break;
			}
		}
	}

	if (is_dynamic)
		AddToChainPair (surf->texinfo->equiv->dynamic_surfaces);
	else 
		AddToChainPair (surf->texinfo->equiv->standard_surfaces);
	
	// Add to the caustics chain if surface is submerged
	// TODO: investigate the possibility of doing this for brush models as 
	// well-- might cause problems with caustics and brush models only half
	// submerged?
	if (!forEnt && r_shaders->integer && (surf->iflags & ISURF_UNDERWATER))
	{
		surf->causticchain = r_caustic_surfaces;
		r_caustic_surfaces = surf;
	}
#undef AddToChainPair
}

/*
================
BSP_DrawTextureChains

Draw ALL surfaces for either the current entity or the world model. If drawing
entity surfaces, reset the linked lists afterward.
================
*/
static void BSP_DrawTextureChains (qboolean forEnt)
{
	msurface_t	*flickersurf;
	int			map;
	
	if (!forEnt)
	{
		R_KillVArrays (); // TODO: check if necessary
		
		for (flickersurf = r_flicker_surfaces; flickersurf != NULL; flickersurf = flickersurf->flickerchain)
		{
			for ( map = 0; map < MAXLIGHTMAPS && flickersurf->styles[map] != 255; map++ )
			{
				if ( r_newrefdef.lightstyles[flickersurf->styles[map]].white != flickersurf->cached_light[map])
				{
					BSP_UpdateSurfaceLightmap (flickersurf);
					break;
				}
			}
		}
	}

	// Setup GL state for lightmap render 
	// (TODO: only necessary for fixed-function pipeline?)
	GL_EnableTexture (1, true);
	R_SetLightingMode ();

	// render all fixed-function surfaces
	BSP_DrawNonGLSLSurfaces(forEnt);

	// render all GLSL surfaces, including normalmapped and dynamically lit
	if(gl_dynamic->integer || r_worldnormalmaps->integer)
	{
		BSP_DrawGLSLSurfaces (forEnt); 
		BSP_DrawGLSLDynamicSurfaces (forEnt);
		glUseProgramObjectARB (0);
	}
	
	BSP_SetScrolling (0);
	
	BSP_DrawSurfaceWireframes (forEnt);
	
	// this has to come last because it messes with GL state
	BSP_DrawWarpSurfaces (forEnt);
	
	GL_EnableTexture (1, false);
	GL_SelectTexture (0); // FIXME: make unnecessary
}



/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
BSP_DrawInlineBModel

Picks which of the current entity's surfaces face toward the camera, and calls
BSP_AddToTextureChain on those.
=================
*/
static void BSP_DrawInlineBModel ( void )
{
	int			i;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;

	R_PushDlightsForBModel ( currententity );

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		GLSTATE_ENABLE_BLEND
		qglColor4f (1,1,1,0.25);
		GL_MTexEnv (0, GL_MODULATE);
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{
		if (psurf->texinfo->flags & SURF_NODRAW)
			continue; //can skip dot product stuff
			// TODO: remove these at load-time for inline models?
		
		// find which side of the plane we are on
		pplane = psurf->plane;
		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->iflags & ISURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->iflags & ISURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			// TODO: do this once at load time
			psurf->entity = currententity;
			
			BSP_AddToTextureChain( psurf, true );

			psurf->visframe = r_framecount;
		}
	}
	
	BSP_DrawTextureChains (true);
	
	GLSTATE_DISABLE_BLEND
	qglColor4f (1,1,1,1);
	GL_MTexEnv (0, GL_REPLACE);
	
	R_KillVArrays ();
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel ( void )
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = currententity->origin[i] - currentmodel->radius;
			maxs[i] = currententity->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (currententity->origin, currentmodel->mins, mins);
		VectorAdd (currententity->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs)) {
		return;
	}

	qglColor3f (1,1,1);

	VectorSubtract (r_newrefdef.vieworg, currententity->origin, modelorg);

	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (currententity->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	qglPushMatrix ();
	R_RotateForEntity (currententity);

	BSP_DrawInlineBModel ();

	qglPopMatrix ();
}



/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
=================
R_CullBox

Returns true if the box is completely outside the frustum

Variant: uses clipflags
=================
*/
static qboolean R_CullBox_ClipFlags (vec3_t mins, vec3_t maxs, int clipflags)
{
	int		i;
	cplane_t *p;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if (!(clipflags  & (1<<i)))
			continue;
		switch (p->signbits)
		{
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		default:
			return false;
		}
	}

	return false;
}

/*
================
BSP_RecursiveWorldNode

Goes through the entire world (the BSP tree,) picks out which surfaces should
be drawn, and calls BSP_AddToTextureChain on each.

node: the BSP node to work on
clipflags: indicate which planes of the frustum may intersect the node

Since each node is inside its parent in 3D space, if a frustum plane can be
shown not to intersect a node at all, then it won't intersect either of its
children. 
================
*/
static void BSP_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c, side;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	
	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	
	// if a leaf node, draw stuff (pt 1)
	c = 0;
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;
		
		if (! (c = pleaf->nummarksurfaces) )
			return;

		// check for door connected areas
		if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
			return;		// not visible

		mark = pleaf->firstmarksurface;
	}

	if (!r_nocull->integer && clipflags)
	{
		int i, clipped;
		cplane_t *clipplane;

		for (i=0,clipplane=frustum ; i<4 ; i++,clipplane++)
		{
			if (!(clipflags  & (1<<i)))
				continue;
			clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);

			if (clipped == 1)
				clipflags &= ~(1<<i);	// node is entirely on screen
			else if (clipped == 2)
				return;					// fully clipped
		}
	}

	//if a leaf node, draw stuff (pt 2)
	if (c != 0)
	{
		do
		{
			(*mark++)->visframe = r_framecount;
		} while (--c);

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0];
		break;
	case PLANE_Y:
		dot = modelorg[1];
		break;
	case PLANE_Z:
		dot = modelorg[2];
		break;
	default:
		dot = DotProduct (modelorg, plane->normal);
		break;
	}

	side = dot < plane->dist;

	// recurse down the children, front side first
	BSP_RecursiveWorldNode (node->children[side], clipflags);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		/* XXX: this doesn't seem to cull any surfaces AT ALL when positioned
		 * here! The surf->visframe check seems to catch any back-facing 
		 * surfaces, but the back-facing surface check seems to allow some
		 * surfaces which are later caught by the surf->visframe check. So the
		 * visframe check renders the planeback check redundant and useless.
		 * I'm pretty sure it's because the map compiler structures the BSP 
		 * tree in such a way as to avoid back-facing surfaces being drawn.
		 * -M
		if ( (surf->flags & SURF_PLANEBACK) != side )
			continue;		// wrong side
		*/

		if (clipflags != 0 && !( surf->iflags & ISURF_DRAWTURB ))
		{
			if (!r_nocull->integer && R_CullBox_ClipFlags (surf->mins, surf->maxs, clipflags)) 
				continue;
		}

		// the polygon is visible, so add it to the appropriate linked list
		BSP_AddToTextureChain( surf, false );
	}

	// recurse down the back side
	BSP_RecursiveWorldNode (node->children[!side], clipflags);
}

/*
=============
R_CalcWorldLights - this is the fallback for non deluxmapped bsp's
=============
*/
static void R_CalcWorldLights( void )
{	
	int		i, j;
	vec3_t	lightAdd, temp;
	float	dist, weight;
	int		numlights = 0;

	if(gl_dynamic->integer || r_worldnormalmaps->integer)
	{
		//get light position relative to player's position
		VectorClear(lightAdd);
		for (i = 0; i < r_lightgroups; i++) 
		{
			VectorSubtract(r_origin, LightGroups[i].group_origin, temp);
			dist = VectorLength(temp);
			if(dist == 0)
				dist = 1;
			dist = dist*dist;
			weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
			for(j = 0; j < 3; j++)
				lightAdd[j] += LightGroups[i].group_origin[j]*weight;
			numlights+=weight;
		}

		if(numlights > 0.0) 
		{
			for(i = 0; i < 3; i++)
				r_worldLightVec[i] = (lightAdd[i]/numlights + r_origin[i])/2.0;
		}
	}
}

/*
=============
R_DrawWorldSurfs
=============
*/
void R_DrawWorldSurfs (void)
{
	if (!r_drawworld->integer)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;
	
	if (r_newrefdef.areabits == NULL)
	{
		Com_Printf ("WARN: No area bits!\n");
		return;
	}
	
	qglColor3f (1,1,1);

	BSP_DrawTextureChains ( false );	

	R_InitSun();

	qglDepthMask(0);
	R_DrawSkyBox();
	qglDepthMask(1);
	
	R_DrawRSSurfaces();
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	static byte	*vis;
	static byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;
	static int minleaf_allareas, maxleaf_allareas;
	int minleaf, maxleaf;
	
	if	(	r_oldviewcluster == r_viewcluster && 
			r_oldviewcluster2 == r_viewcluster2 && 
			!r_novis->integer && r_viewcluster != -1 &&
			!r_newrefdef.areabits_changed)
		return;
	r_newrefdef.areabits_changed = false;
	
	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->integer)
		return;

	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->integer || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		r_visframecount++;
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	minleaf_allareas = r_viewleaf->minPVSleaf;
	maxleaf_allareas = r_viewleaf->maxPVSleaf;
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		if (r_viewleaf2->minPVSleaf < minleaf_allareas)
			minleaf_allareas = r_viewleaf2->minPVSleaf;
		if (r_viewleaf2->maxPVSleaf > maxleaf_allareas)
			maxleaf_allareas = r_viewleaf2->maxPVSleaf;
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	r_visframecount++;
	
	minleaf = minleaf_allareas;
	maxleaf = maxleaf_allareas;
	
	//restrict leaf range further to the range leafs in connected areas.
	{
		int areamax = 0;
		int areamin = r_worldmodel->numleafs;
		for (i = 0; i < r_worldmodel->num_areas; i++)
		{
			if (r_newrefdef.areabits[i>>3] & (1<<(i&7)))
			{
				if (r_worldmodel->area_max_leaf[i] > areamax)
					areamax = r_worldmodel->area_max_leaf[i];
				if (r_worldmodel->area_min_leaf[i] < areamin)
					areamin = r_worldmodel->area_min_leaf[i];
			}
		}
		if (areamin < areamax)
		{
			if (areamin > minleaf)
				minleaf = areamin;
			if (areamax < maxleaf)
				maxleaf = areamax;
		}
	}
	
	for (i=minleaf,leaf=r_worldmodel->leafs+minleaf ; i<=maxleaf ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}

/*
===============
R_MarkWorldSurfs

Mark all surfaces that will need to be drawn this frame
===============
*/
void R_MarkWorldSurfs (void)
{
	static entity_t	ent;
	static int		old_visframecount, old_dlightcount, last_bsp_time;
	static vec3_t	old_origin, old_angle;
	vec3_t			delta_origin, delta_angle;
	qboolean		do_bsp;
	int				cur_ms;
	
	if (!r_drawworld->integer)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;
	
	if (r_newrefdef.areabits == NULL)
	{
		Com_Printf ("WARN: No area bits!\n");
		return;
	}
	
	R_MarkLeaves ();

	currentmodel = r_worldmodel;

	VectorCopy (r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);
	currententity = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	R_CalcWorldLights();

	// r_optimize: only re-recurse the BSP tree if the player has moved enough
	// to matter.
	VectorSubtract (r_origin, old_origin, delta_origin);
	VectorSubtract (r_newrefdef.viewangles, old_angle, delta_angle);
	do_bsp =	old_visframecount != r_visframecount ||
				VectorLength (delta_origin) > 5.0 ||
				VectorLength (delta_angle) > 2.0 ||
				r_newrefdef.num_dlights != 0 ||
				old_dlightcount != 0 ||
				!r_optimize->integer || draw_sun;
	
	cur_ms = Sys_Milliseconds ();
	
	// After a certain amount of time, increase the sensitivity to movement
	// and angle. If we go too long without re-recursing the BSP tree, it 
	// means the player is either moving very slowly or not moving at all. If
	// the player is moving slowly enough, it can catch the r_optimize code
	// napping and cause artefacts, so we should be extra vigilant just in 
	// case. Something you basically have to do on purpose, but we go the 
	// extra mile.
	if (r_optimize->integer && !do_bsp)
	{
		// be sure to handle integer overflow of the millisecond counter
		if (cur_ms < last_bsp_time || last_bsp_time+100 < cur_ms)
			do_bsp =	VectorLength (delta_origin) > 0.5 ||
						VectorLength (delta_angle) > 0.2;
			
	}
	
	if (do_bsp)
	{
		BSP_ClearWorldTextureChains ();
		BSP_RecursiveWorldNode (r_worldmodel->nodes, 15);
		
		old_visframecount = r_visframecount;
		VectorCopy (r_origin, old_origin);
		VectorCopy (r_newrefdef.viewangles, old_angle);
		last_bsp_time = cur_ms;
	}
	old_dlightcount = r_newrefdef.num_dlights;
}



/*
=============================================================================

  LIGHTMAP ALLOCATION
  
  TODO: move this to another file, this is load-time stuff that doesn't run
  every frame.

=============================================================================
*/

static void LM_InitBlock (void)
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

// Upload the current lightmap data to OpenGL, then clear it and prepare for
// the next lightmap texture to be filled with data. 
// TODO: With HD lightmaps, are mipmaps a good idea here?
// TODO: Opportunistically make lightmap texture smaller if not all is used?
// Will require delaying lightmap texcoords for all surfaces until after
// upload.
static void LM_UploadBlock (void)
{
	int texture = gl_lms.current_lightmap_texture;

	GL_SelectTexture (0);
	GL_Bind( gl_state.lightmap_textures + texture );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	qglTexImage2D( GL_TEXTURE_2D,
				   0,
				   GL_RGBA,
				   LIGHTMAP_SIZE, LIGHTMAP_SIZE,
				   0,
				   GL_LIGHTMAP_FORMAT,
				   GL_UNSIGNED_INT_8_8_8_8_REV,
				   gl_lms.lightmap_buffer );
	
#if 0
	height = 0;
	for (i = 0; i < LIGHTMAP_SIZE; i++)
	{
		if (gl_lms.allocated[i] > height)
			height = gl_lms.allocated[i];
	}
	
	Com_Printf (" LIGHTMAP %d HEIGHT %d\n", gl_lms.current_lightmap_texture, height);
#endif
	
	if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
		Com_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
}

// LM_AllocBlock - given a certain size rectangle, allocates space inside the
// lightmap texture atlas.
// Returns a texture number and the position inside it.
// TODO: there are some clever tricks I can think of to pack these more
// tightly: opportunistically rotating lightmap textures by 90 degrees, 
// wrapping the lightmap textures around the edges of the atlas, and even
// recognizing that not all surfaces are rectangular.
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = LIGHTMAP_SIZE;

	for (i=0 ; i<LIGHTMAP_SIZE-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LIGHTMAP_SIZE)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
BSP_BuildPolygonFromSurface
================
*/
void BSP_BuildPolygonFromSurface(msurface_t *fa, float xscale, float yscale, int light_s, int light_t, int firstedge, int lnumverts)
{
	int			i, lindex;
	medge_t		*r_pedge;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;
	vec3_t		center;

	vec3_t total = {0, 0, 0};
	vec3_t surfmaxs = {-99999999, -99999999, -99999999};
	vec3_t surfmins = {99999999, 99999999, 99999999};

	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	assert (fa->polys == NULL); // only warp surfaces will have multiple polys
	poly->next = fa->polys;
	poly->numverts = lnumverts;
	fa->polys = poly;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &currentmodel->edges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &currentmodel->edges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		
		if ((fa->texinfo->flags & SURF_SKY) == 0)
		{
			if (fa->texinfo->flags & (SURF_WARP|SURF_FLOWING|SURF_TRANS33|SURF_TRANS66))
				r_pedge->sColor += 0.5;
			
			if (r_pedge->usecount == 0)
			{
				r_pedge->first_face_plane = fa->plane;
			}
			else if (fa->plane != r_pedge->first_face_plane)
			{
				r_pedge->iscorner = true;
				r_pedge->alpha = 1.0 - fabs (DotProduct (fa->plane->normal, r_pedge->first_face_plane->normal));
			}
			
			r_pedge->usecount++;
		}

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		// set bbox for the surf used for culling
		if (vec[0] > surfmaxs[0]) surfmaxs[0] = vec[0];
		if (vec[1] > surfmaxs[1]) surfmaxs[1] = vec[1];
		if (vec[2] > surfmaxs[2]) surfmaxs[2] = vec[2];

		if (vec[0] < surfmins[0]) surfmins[0] = vec[0];
		if (vec[1] < surfmins[1]) surfmins[1] = vec[1];
		if (vec[2] < surfmins[2]) surfmins[2] = vec[2];

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += light_s*xscale;
		s += xscale/2.0;
		s /= LIGHTMAP_SIZE*xscale;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += light_t*yscale;
		t += yscale/2.0;
		t /= LIGHTMAP_SIZE*yscale;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;

		//to do - check if needed
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= 128;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= 128;

		poly->verts[i][7] = s;
		poly->verts[i][8] = t;
		
	}
#if 0 //SO PRETTY and useful for checking how much lightmap data is used up
	if (lnumverts == 4)
	{
		poly->verts[0][5] = 1;
		poly->verts[0][6] = 1;
		poly->verts[1][5] = 1;
		poly->verts[1][6] = 0;
		poly->verts[2][5] = 0;
		poly->verts[2][6] = 0;
		poly->verts[3][5] = 0;
		poly->verts[3][6] = 1;
	}
#endif

	// store out the completed bbox
	VectorCopy (surfmins, fa->mins);
	VectorCopy (surfmaxs, fa->maxs);
	
	VectorScale (total, 1.0f/(float)lnumverts, center);
	
	// These are used by the rscript "rotate" keyword.
	fa->c_s = (DotProduct (center, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3])
				/ fa->texinfo->image->width;
	fa->c_t = (DotProduct (center, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3])
				/ fa->texinfo->image->height;
	
	// These are used by the code that detects light levels for entities, so
	// they can sample HD lightmaps.
	fa->lightmap_xscale = xscale;
	fa->lightmap_yscale = yscale;
}

/*
========================
BSP_CreateSurfaceLightmap
========================
*/
void BSP_CreateSurfaceLightmap (msurface_t *surf, int smax, int tmax, int *light_s, int *light_t)
{
	byte	*base;

	if (!surf->samples)
		return;

	if (surf->texinfo->flags & (SURF_SKY|SURF_WARP))
		return; //may not need this?

	if ( !LM_AllocBlock( smax, tmax, light_s, light_t ) )
	{
		LM_UploadBlock( );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, light_s, light_t ) )
		{
			Com_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;
	
	surf->lightmins[0] = *light_s;
	surf->lightmins[1] = *light_t;
	surf->lightmaxs[0] = smax;
	surf->lightmaxs[1] = tmax;

	base = gl_lms.lightmap_buffer;
	base += ((*light_t) * LIGHTMAP_SIZE + *light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, smax, tmax, LIGHTMAP_SIZE*LIGHTMAP_BYTES);
}

static void BSP_UpdateSurfaceLightmap (msurface_t *surf)
{
	R_SetCacheState (surf);
	R_BuildLightMap (surf, gl_lms.lightmap_buffer, surf->lightmaxs[0], surf->lightmaxs[1], surf->lightmaxs[0]*LIGHTMAP_BYTES);
	
	GL_SelectTexture (0);
	GL_Bind( gl_state.lightmap_textures + surf->lightmaptexturenum );
	qglTexSubImage2D( GL_TEXTURE_2D, 
					  0,
					  surf->lightmins[0], surf->lightmins[1],
					  surf->lightmaxs[0], surf->lightmaxs[1],
					  GL_LIGHTMAP_FORMAT,
					  GL_UNSIGNED_INT_8_8_8_8_REV,
					  gl_lms.lightmap_buffer );
}

/*
==================
BSP_BeginBuildingLightmaps
==================
*/
void BSP_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;

	LM_InitBlock ();

	r_framecount = 1;		// no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
		gl_state.lightmap_textures	= TEXNUM_LIGHTMAPS;

	gl_lms.current_lightmap_texture = 1;

}

/*
=======================
BSP_EndBuildingLightmaps
=======================
*/
void BSP_EndBuildingLightmaps (void)
{
	LM_UploadBlock ();
}



/*
=============================================================================

  MINI MAP
  
  Draws a little 2D map in the corner of the HUD.

=============================================================================
*/

static void R_DrawRadarEdges (void)
{
	int i;
	float distance;
	
	if(r_minimap_zoom->value>=0.1) 
		distance = 1024.0/r_minimap_zoom->value;
	else
		distance = 1024.0;
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, minimap_vboId);
	R_VertexPointer (3, 5*sizeof(float), (void *)0);
	R_AttribPointer (ATTR_MINIMAP_DATA_IDX, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void *)(3*sizeof(float)));
	GL_BindVBO (0);
	
	glUseProgramObjectARB (g_minimapprogramObj);
	
	for (i = 1; i < r_worldmodel->numedges; i++)
	{
		medge_t *edge = &r_worldmodel->edges[i];
	
		if (!edge->iscorner)
			continue;
	
		if (	r_origin[0]+distance < edge->mins[0] ||
				r_origin[0]-distance > edge->maxs[0] ||
				r_origin[1]+distance < edge->mins[1] ||
				r_origin[1]-distance > edge->maxs[1] ||
				r_origin[2]+256 < edge->mins[2] ||
				r_origin[2]-256 > edge->maxs[2]) continue;
		
		qglDrawArrays (GL_LINES, edge->vbo_start, 2);
	}
	
	R_KillVArrays ();
	
	glUseProgramObjectARB (0);
}

int			numRadarEnts=0;
RadarEnt_t	RadarEnts[MAX_RADAR_ENTS];

void R_DrawRadar(void)
{
	int		i;
	int 	minimap_size = (int) r_minimap_size->value;

	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL)) return;
	if (!r_minimap->integer) return;
	if (!r_newrefdef.areabits) return;

	qglViewport	(	r_newrefdef.x + r_newrefdef.width - minimap_size,
					viddef.height - r_newrefdef.y - r_newrefdef.height, 
					minimap_size, minimap_size);

	qglDisable (GL_DEPTH_TEST);
  	qglMatrixMode (GL_PROJECTION);
	qglPushMatrix ();
	qglLoadIdentity ();
	qglOrtho(0, 1, 1, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity ();
	
	qglStencilMask(255);
	qglClear(GL_STENCIL_BUFFER_BIT);
	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS,4,4);
	qglStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);


	GLSTATE_ENABLE_ALPHATEST;
	qglAlphaFunc(GL_LESS,0.1);
	qglColorMask(0,0,0,0);

	qglColor4f(1,1,1,1);
	if(r_around)
	{
		GL_Bind (r_around->texnum);
		GL_SetupWholeScreen2DVBO (wholescreen_textured);
		R_DrawVarrays (GL_QUADS, 0, 4);
		R_KillVArrays ();
	}

	qglColorMask(1,1,1,1);
	GLSTATE_DISABLE_ALPHATEST;
	qglAlphaFunc(GL_GREATER, 0.666);
	qglStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
	qglStencilFunc(GL_NOTEQUAL,4,4);
	
	if (r_minimap_zoom->value >= 0.1)
		qglScalef (r_minimap_zoom->value, r_minimap_zoom->value, r_minimap_zoom->value);
	
	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	if (r_minimap_style->integer)
		qglOrtho (-1024, 1024, -1024, 1024, -256, 256);
	else
		qglOrtho (-1024, 1024, -512, 1536, -256, 256);
	
	qglMatrixMode(GL_MODELVIEW);
	if (r_minimap_style->integer)
		qglRotatef (90-r_newrefdef.viewangles[1], 0, 0, -1);
	
	GL_EnableTexture (0, false);
	qglBegin (GL_TRIANGLES);
	qglColor4f (1,1,0,0.5);
	qglVertex3f (0,32,0);
	qglVertex3f (24,-32,0);
	qglVertex3f (-24,-32,0);
	qglEnd ();

	qglRotatef (90-r_newrefdef.viewangles[1], 0, 0, 1);
	qglTranslatef (-r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

	qglBegin(GL_QUADS);
	for(i=0;i<numRadarEnts;i++){
		float x=RadarEnts[i].org[0];
		float y=RadarEnts[i].org[1];
		float z=RadarEnts[i].org[2];
		qglColor4fv(RadarEnts[i].color);

		qglVertex3f(x+9, y+9, z);
		qglVertex3f(x+9, y-9, z);
		qglVertex3f(x-9, y-9, z);
		qglVertex3f(x-9, y+9, z);
	}
	qglEnd();

	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);
	GLSTATE_ENABLE_BLEND;
	qglColor3f(1,1,1);

	// draw the actual minimap
	R_DrawRadarEdges ();
	
	GL_EnableTexture (0, true);

	GL_BlendFunction (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	qglPopMatrix();

	qglViewport(0,0,viddef.width,viddef.height);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglDisable(GL_STENCIL_TEST);
	GLSTATE_DISABLE_BLEND;
	qglEnable(GL_DEPTH_TEST);
	qglColor4f(1,1,1,1);

}
