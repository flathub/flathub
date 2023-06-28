/*
Copyright (C) 2010-2014 COR Entertainment, LLC

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
// r_particle.c - particle subsystem, renders particles, lightvolumes, lensflares, vegetation, etc

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"


#include "r_lodcalc.h"
// Returns true if a particle is far enough away not to be worth rendering.
static qboolean distancecull_particle (vec3_t origin, float side_length)
{
	vec3_t dist, span;
	
	VectorSubtract (origin, r_origin, dist);
	VectorSet (span, side_length, side_length, side_length);
	
	return 1.5*VectorLength (dist) > LOD_DIST*VectorLength (span);
}

/*
===============
PART_DrawParticles
===============
*/
static float yawOrRoll;
static int compare_particle (const void *_a, const void *_b)
{
	particle_t *a = *(particle_t **)_a;
	particle_t *b = *(particle_t **)_b;
	if (a->image != NULL && b->image == NULL)
		return 1;
	if (a->image == NULL && b->image != NULL)
		return -1;
	if (a->image == NULL && b->image == NULL)
		return 0;
	if (a->image->texnum != b->image->texnum)
		return a->image->texnum-b->image->texnum;
	if (a->blendsrc != b->blendsrc)
		return a->blendsrc-b->blendsrc;
	if (a->blenddst != b->blenddst)
		return a->blenddst-b->blenddst;
	return 0;
}

void PART_AddBillboardToVArray (	const vec3_t origin, const vec3_t up,
									const vec3_t right, float sway,
									qboolean notsideways, float sl, float sh,
									float tl, float th )
{
#define VERTEX(n) (VArray + n*VertexSizes[VERT_SINGLE_TEXTURED])
// Sigh. It turns out that all particles, vegetation sprites, and beam sprites
// are always rendered sideways, but *not* the simple items.
#define TEXVERTEX(n) VERTEX(((n+4-notsideways)%4))
	
	VectorSet (VERTEX(0),
		origin[0] + (up[0] + right[0])*(-0.5),
		origin[1] + (up[1] + right[1])*(-0.5),
		origin[2] + (up[2] + right[2])*(-0.5));
	TEXVERTEX(0)[3] = sh;
	TEXVERTEX(0)[4] = th;

	VectorSet (VERTEX(1),
		VERTEX(0)[0] + up[0] + sway,
		VERTEX(0)[1] + up[1] + sway,
		VERTEX(0)[2] + up[2]);
	TEXVERTEX(1)[3] = sl;
	TEXVERTEX(1)[4] = th;

	VectorSet (VERTEX(2),
		VERTEX(0)[0] + up[0] + right[0] + sway,
		VERTEX(0)[1] + up[1] + right[1] + sway,
		VERTEX(0)[2] + up[2] + right[2]);
	TEXVERTEX(2)[3] = sl;
	TEXVERTEX(2)[4] = tl;

	VectorSet (VERTEX(3),
		VERTEX(0)[0] + right[0],
		VERTEX(0)[1] + right[1],
		VERTEX(0)[2] + right[2]);
	TEXVERTEX(3)[3] = sh;
	TEXVERTEX(3)[4] = tl;
	
	VArray += 4*VertexSizes[VERT_SINGLE_TEXTURED];
	
#undef VERTEX
}

void PART_DrawParticles( int num_particles, particle_t **particles, const unsigned colortable[768])
{
	particle_t **p1;
	particle_t *p;
	int				i, k;
	vec3_t			up, right, pup, pright, dir;
	float			scale;
	byte			color[4];
	int				oldblendsrc = -1, oldblenddst = -1;
	int				texnum=0, blenddst, blendsrc;
	vec3_t move, delta, v;
	float			sh, th, sl, tl;

	if ( !num_particles )
		return;
	
	GL_SelectTexture (0);

	qglDepthMask (GL_FALSE);	 	// no z buffering
	GLSTATE_ENABLE_BLEND
	GL_TexEnv (GL_MODULATE);

	R_InitVArrays (VERT_SINGLE_TEXTURED);
	
	qglDisable (GL_CULL_FACE);
	
	qsort (particles, num_particles, sizeof (particle_t *), compare_particle);
	
	for ( p1 = particles, i=0; i < num_particles ; i++,p1++)
	{
		p = *p1;

		if( p->type == PARTICLE_CHAINED && p->chain_prev)
		{
			vec3_t span, beam_org;
			VectorSubtract (p->current_origin, p->chain_prev->current_origin, span);
			VectorAdd (p->current_origin, p->chain_prev->current_origin, beam_org);
			VectorScale (beam_org, 0.5, beam_org);
			VectorScale (span, 0.5, span);
			if (R_CullSphere( beam_org, VectorLength (span), 15 ) )
				continue;
		}
		else if (R_CullSphere( p->current_origin, 64, 15 ) )
			continue;

		if(p->type == PARTICLE_NONE) {

			blendsrc = GL_SRC_ALPHA;
			blenddst = GL_ONE;

			*(int *)color = colortable[p->current_color];
			scale = 1;
			VectorCopy (vup, up);
			VectorCopy (vright, right);
		}
		else {
			if (p->image != NULL)
				texnum = p->image->texnum;
			else
				texnum = 0;
			blendsrc = p->blendsrc;
			blenddst = p->blenddst;
			scale = p->current_scale;
			VectorScale (vup, scale, up);
			VectorScale (vright, scale, right);
			*(int *)color = colortable[p->current_color];
		}

		color[3] = p->current_alpha*255;
		
		GL_Bind (texnum);

		if (oldblendsrc != blendsrc || oldblenddst != blenddst)
		{	
			if (oldblendsrc != blendsrc || oldblenddst != blenddst)
				GL_BlendFunction ( blendsrc, blenddst );
			
			oldblendsrc = blendsrc;
			oldblenddst = blenddst;
		}
		
		if(p->type == PARTICLE_RAISEDDECAL) {
			for (k = 0; k < 4; k++)
				color[k] = 255;
		}

		if(p->type == PARTICLE_BEAM) {

			VectorSubtract(p->current_origin, p->angle, move);
			VectorNormalize(move);

			VectorCopy(move, pup);
			VectorSubtract(r_newrefdef.vieworg, p->angle, delta);
			CrossProduct(pup, delta, pright);
			VectorNormalize(pright);

			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);
		}
		else if(p->type == PARTICLE_DECAL || p->type == PARTICLE_RAISEDDECAL) {
			VectorCopy(p->angle, dir);
			AngleVectors(dir, NULL, right, up);
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);

		}
		else if(p->type == PARTICLE_FLAT) {

			VectorCopy(r_newrefdef.viewangles, dir);

			dir[0] = -90;  // and splash particles horizontal by setting it
			AngleVectors(dir, NULL, right, up);

			if(p->current_origin[2] > r_newrefdef.vieworg[2]){  // it's above us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, 5*scale, pup);
			}
			else {  // it's below us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, -5*scale, pup);
			}
		}
		else if(p->type == PARTICLE_FLUTTERWEATHER){
			VectorCopy(p->angle, dir);
			AngleVectors(dir, NULL, right, up);
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else if(p->type == PARTICLE_VERT || p->type == PARTICLE_WEATHER){  // keep it vertical
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			AngleVectors(v, NULL, right, up);
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else if(p->type == PARTICLE_ROTATINGYAW || p->type == PARTICLE_ROTATINGROLL || p->type == PARTICLE_ROTATINGYAWMINUS){  // keep it vertical, and rotate on axis
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			yawOrRoll += r_frametime*50;
			if (yawOrRoll > 360)
				yawOrRoll = 0;
			if(p->type == PARTICLE_ROTATINGYAW)
				v[1] = yawOrRoll;
			else if(p->type == PARTICLE_ROTATINGROLL)
				v[2] = yawOrRoll;
			else
				v[1] = yawOrRoll+180;
			AngleVectors(v, NULL, right, up);
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else {
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
		}
		
		if (p->image != NULL)
		{
			sh = p->image->sh;
			th = p->image->th;
			sl = p->image->sl;
			tl = p->image->tl;
		}
		else
		{
			sh = th = 1;
			sl = tl = 0;
		}
		
		VArray = &VArrayVerts[0];
		
		if (p->type == PARTICLE_CHAINED && p->chain_prev)
		{
			particle_t *pr;
			vec3_t pspan, prev_pspan;
	
			pr = p->chain_prev;
	
			VectorCopy (p->current_pspan, pspan);
			if (pr->type == PARTICLE_CHAINED && pr->chain_prev)
				VectorCopy (pr->current_pspan, prev_pspan);
			else
				VectorCopy (pspan, prev_pspan);
	
#define VERTEX(n) (VArray + n*VertexSizes[VERT_SINGLE_TEXTURED])
			
			VectorSet (VERTEX(0),
				p->current_origin[0] + pspan[0]*(0.5),
				p->current_origin[1] + pspan[1]*(0.5),
				p->current_origin[2] + pspan[2]*(0.5));
			VERTEX(0)[3] = sh;
			VERTEX(0)[4] = th;
			
			VectorSet (VERTEX(1),
				pr->current_origin[0] + prev_pspan[0]*(0.5),
				pr->current_origin[1] + prev_pspan[1]*(0.5),
				pr->current_origin[2] + prev_pspan[2]*(0.5));
			VERTEX(1)[3] = sl;
			VERTEX(1)[4] = th;
			
			VectorSet (VERTEX(2),
				pr->current_origin[0] + prev_pspan[0]*(-0.5),
				pr->current_origin[1] + prev_pspan[1]*(-0.5),
				pr->current_origin[2] + prev_pspan[2]*(-0.5));
			VERTEX(2)[3] = sl;
			VERTEX(2)[4] = tl;
			
			VectorSet (VERTEX(3),
				p->current_origin[0] + pspan[0]*(-0.5),
				p->current_origin[1] + pspan[1]*(-0.5),
				p->current_origin[2] + pspan[2]*(-0.5));
			VERTEX(3)[3] = sh;
			VERTEX(3)[4] = tl;
			
			VArray += 4*VertexSizes[VERT_SINGLE_TEXTURED];
			
#undef VERTEX
			
		}
		else
		{
			PART_AddBillboardToVArray (p->current_origin, pup, pright, 0, false, sl, sh, tl, th);
		}
		
		qglColor4f (color[0]/255.0f, color[1]/255.0f, color[2]/255.0f, color[3]/255.0f);

		R_DrawVarrays(GL_QUADS, 0, 4);
	}	
	


	R_KillVArrays ();
	GL_BlendFunction ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglColor4f( 1,1,1,1 );
	GLSTATE_DISABLE_BLEND
	qglDepthMask( GL_TRUE );	// back to normal Z buffering
	GL_TexEnv (GL_REPLACE);
	qglEnable (GL_CULL_FACE);
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	if ( !r_newrefdef.num_particles )
		return;

	if(map_fog)
		qglDisable(GL_FOG);

	PART_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table);

	if(map_fog)
		qglEnable(GL_FOG);
}

static void R_Surface_Bounds (const msurface_t *surf, vec3_t out_mins, vec3_t out_maxs, vec3_t out_center)
{
	const glpoly_t	*poly;
	const float		*v;
	int				i;
	
	VectorSet(out_mins, 999999, 999999, 999999);
	VectorSet(out_maxs, -999999, -999999, -999999);

	poly = surf->polys;
	for (i=0, v=poly->verts[0] ; i< poly->numverts; i++, v+= VERTEXSIZE)
	{
		if(v[0] > out_maxs[0])   out_maxs[0] = v[0];
		if(v[1] > out_maxs[1])   out_maxs[1] = v[1];
		if(v[2] > out_maxs[2])   out_maxs[2] = v[2];

		if(v[0] < out_mins[0])   out_mins[0] = v[0];
		if(v[1] < out_mins[1])   out_mins[1] = v[1];
		if(v[2] < out_mins[2])   out_mins[2] = v[2];
	}

	out_center[0] = (out_mins[0] + out_maxs[0]) / 2.0;
	out_center[1] = (out_mins[1] + out_maxs[1]) / 2.0;
	out_center[2] = (out_mins[2] + out_maxs[2]) / 2.0;
}

//lens flares
// TODO: flare rendering is actually shockingly slow, and the reason is the 
// overhead of each glDrawArrays call. It MIGHT be worth using a variation on 
// this method: http://stackoverflow.com/a/1039082 and batching up all flares
// of each texture into a single VArray consisting of vertex and vertex color
// data, with a size attribute, one vertex per flare. This would reduce the 
// usage of the PCI bus.

void Mod_AddFlareSurface (msurface_t *surf, int type )
{
	int i, width, height;
	flare_t	*light;
	byte	*buffer;
	byte	*p;
	float	surf_bound;
	vec3_t origin = {0,0,0}, color = {1,1,1}, tmp, rgbSum;
	vec3_t mins, maxs, tmp1;

	if (surf->iflags & ISURF_DRAWTURB)
		return;

	if (surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING|SURF_WARP))
		return;

	if (!(surf->texinfo->flags & (SURF_LIGHT)))
		return;

	if (r_numflares >= MAX_FLARES)
		return;

	light = &r_flares[r_numflares++];

	R_Surface_Bounds (surf, mins, maxs, origin);

	 /*
	=====================================
	calc light surf bounds and flare size
	=====================================
	*/


	VectorSubtract(maxs, mins, tmp1);
	surf_bound = VectorLength(tmp1);
	if (surf_bound <=25) light->size = 10;
	else if (surf_bound <=50)  light->size = 15;
	else if (surf_bound <=100) light->size = 20;
	else if (surf_bound <=150) light->size = 25;
	else if (surf_bound <=200) light->size = 30;
	else if (surf_bound <=250) light->size = 35;


	/*
	===================
	calc texture color
	===================
	*/

	GL_Bind( surf->texinfo->image->texnum );
	width = surf->texinfo->image->upload_width;
	height = surf->texinfo->image->upload_height;

	buffer = malloc(width * height * 3);
	qglGetTexImage (GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	VectorClear(rgbSum);

	for (i=0, p=buffer; i<width*height; i++, p += 3)
	{
		rgbSum[0] += (float)p[0]  * (1.0 / 255);
		rgbSum[1] += (float)p[1]  * (1.0 / 255);
		rgbSum[2] += (float)p[2]  * (1.0 / 255);
	}

	VectorScale(rgbSum, r_lensflare_intens->value / (width *height), color);

	for (i=0; i<3; i++) {
		if (color[i] < 0.5)
			color[i] = color[i] * 0.5;
		else
			color[i] = color[i] * 0.5 + 0.5;
	}
	VectorCopy(color, light->color);

	/*
	==================================
	move flare origin in to map bounds
	==================================
	*/

	if (surf->iflags & ISURF_PLANEBACK)
	VectorNegate(surf->plane->normal, tmp);
	else
	VectorCopy(surf->plane->normal, tmp);

	VectorMA(origin, 2, tmp, origin);
	VectorCopy(origin, light->origin);

	light->style = type;

	light->leafnum = CM_PointLeafnum (light->origin);

	free (buffer);
}

void PART_RenderFlare (flare_t *light)
{
	if(light->style == 0)
		GL_Bind (r_flare->texnum);
	else
		GL_Bind (r_flare1->texnum);
	
	c_flares++;
	
	qglColor4f (light->color[0], light->color[1], light->color[2], light->alpha);

	R_DrawVarrays (GL_QUADS, light->vbo_first_vert, 4);

}

void R_RenderFlares (void)
{
	flare_t	*l;
	int i;
	qboolean visible;
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL) return;
	qglDepthMask (0);
	qglShadeModel (GL_SMOOTH);
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);
	
	GL_SetupLensFlareVBO ();
	glUseProgramObjectARB (g_lensflareprogramObj);
	glUniform3fARB (lensflare_uniforms.up, vup[0], vup[1], vup[2]);
	glUniform3fARB (lensflare_uniforms.right, vright[0], vright[1], vright[2]);

	qglDisable (GL_DEPTH_TEST);
	GL_SelectTexture (0);
	GL_TexEnv (GL_MODULATE);

	l = r_flares;
	for (i = 0; i < r_numflares ; i++, l++)
	{

		// periodically test visibility to ramp alpha
		if(rs_realtime - l->time > 0.02){

			if (!CM_inPVS_leafs (r_origin_leafnum, l->leafnum))
				continue;

			visible = CM_FastTrace (r_origin, l->origin, r_worldmodel->firstnode, MASK_VISIBILILITY);
			
			l->alpha += (visible ? 0.03 : -0.15);  // ramp

			if(l->alpha > 0.5)  // clamp
				l->alpha = 0.5;
			else if(l->alpha < 0)
				l->alpha = 0.0;

			l->time = rs_realtime;
		}

		if (l->alpha > 0)
		{
			PART_RenderFlare (l);
		}
	}
	
	R_KillVArrays ();
	glUseProgramObjectARB (0);
	GL_MTexEnv (0, GL_REPLACE);
	qglEnable (GL_DEPTH_TEST);
	qglColor3f (1,1,1);
	GLSTATE_DISABLE_BLEND
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

void R_ClearFlares(void)
{
	memset(r_flares, 0, sizeof(r_flares));
	r_numflares = 0;
}

/*
================
Sun Object
================
*/

void transform_point(float out[4], const float m[16], const float in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] =
		M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0,
																3) * in[3];
	out[1] =
		M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1,
																3) * in[3];
	out[2] =
		M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2,
																3) * in[3];
	out[3] =
		M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3,
																3) * in[3];
#undef M
}

qboolean gluProject2(float objx, float objy, float objz, const float model[16], const float proj[16], const int viewport[4], float *winx, float *winy)
{
	/* transformation matrix */
	float in[4], out[4], temp;

	/* initialise the matrix and the vector to transform */
	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0;
	transform_point(out, model, in);
	transform_point(in, proj, out);

	/* normalise result to [-1;1] */
	if (in[3] == 0.0)
		return false;

	temp = 1.0 / in[3];
	in[0] *= temp;
	in[1] *= temp;
	in[2] *= temp;

	/* display coordinates */
	*winx = viewport[0] + (1 + in[0]) * (float) viewport[2] * 0.5;
	*winy = viewport[1] + (1 + in[1]) * (float) viewport[3] * 0.5;
	return true;
}


void R_InitSun()
{
	draw_sun = false;

	if (!r_drawsun->integer || r_nosun)
		return;

	if (spacebox)
		sun_size = 0.1f;
	else
		sun_size = 0.2f;

	if (R_CullOrigin(sun_origin))
		return;

	draw_sun = true;

	gluProject2(sun_origin[0], sun_origin[1], sun_origin[2], r_world_matrix, r_project_matrix, (int *) r_viewport, &sun_x, &sun_y);
	sun_y = r_newrefdef.height - sun_y;
}


void PART_GetSunFlareBounds (float offset, float radius, vec2_t out_mins, vec2_t out_maxs)
{
	vec_t new_x, new_y;

	if (offset) {
		new_x = offset * (r_newrefdef.width / 2 - sun_x) + sun_x;
		new_y = offset * (r_newrefdef.height / 2 - sun_y) + sun_y;
	} else {
		new_x = sun_x;
		new_y = sun_y;
	}

	out_mins[0] = new_x - radius;
	out_mins[1] = new_y - radius;
	out_maxs[0] = new_x + radius;
	out_maxs[1] = new_y + radius;
}


void PART_RenderSunFlare(image_t * tex, float offset, float radius, float depth, float r,
					  float g, float b, float alpha)
{
	vec2_t mins, maxs;
	float diameter = 2.0*radius;

	qglColor4f (r, g, b, alpha);
	GL_MBind (0, tex->texnum);

	PART_GetSunFlareBounds (offset, radius, mins, maxs);
	
	if (r_test->integer && false)
	{
		// TODO: add an alpha channel to gfx/sun.jpg (will require converting
		// to TGA) because otherwise this code makes no difference.
		
		mins[0] += diameter * (float)tex->crop_left / (float)tex->upload_width;
		mins[1] += diameter * (float)tex->crop_top / (float)tex->upload_height;
		maxs[0] = mins[0] + diameter * (float)tex->crop_width / (float)tex->upload_width;
		maxs[1] = mins[1] + diameter * (float)tex->crop_height / (float)tex->upload_height;
		
		qglBegin (GL_QUADS);
		qglTexCoord2f (tex->crop_sl, tex->crop_tl);
		qglVertex3f (mins[0], mins[1], depth);
		qglTexCoord2f (tex->crop_sh, tex->crop_tl);
		qglVertex3f (maxs[0], mins[1], depth);
		qglTexCoord2f (tex->crop_sh, tex->crop_th);
		qglVertex3f (maxs[0], maxs[1], depth);
		qglTexCoord2f (tex->crop_sl, tex->crop_th);
		qglVertex3f (mins[0], maxs[1], depth);
		qglEnd ();
	}
	else
	{
		qglBegin (GL_QUADS);
		qglTexCoord2f (0, 0);
		qglVertex3f (mins[0], mins[1], depth);
		qglTexCoord2f (1, 0);
		qglVertex3f (maxs[0], mins[1], depth);
		qglTexCoord2f (1, 1);
		qglVertex3f (maxs[0], maxs[1], depth);
		qglTexCoord2f (0, 1);
		qglVertex3f (mins[0], maxs[1], depth);
		qglEnd ();
	}
}

float sun_alpha = 0;
void R_RenderSun()
{
	static float l;
	static float sun_vistest_time = 0;
	static float sun_ramp_time = 0;
	float size;

	if (!draw_sun)
		return;

	if (r_nosun)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// The sun is only visible if a single pixel at sun_x, sun_y is not
	// covered by anything. Just like lens flares, we ramp the opacity up and
	// down linearly to smooth out transitions. Since glReadPixels is
	// expensive, we only test visibility every SUN_VIS_TEST_PERIOD seconds,
	// but to keep the opacity animation smooth, we do that every frame.

	#define SUN_VIS_TEST_PERIOD 0.1
	#define SUN_ALPHA_RAMP_PER_SECOND 7.5
	#define SUN_ALPHA_RAMP_PER_FRAME (SUN_ALPHA_RAMP_PER_SECOND*(rs_realtime-sun_ramp_time))

	// periodically test visibility to ramp alpha
	if(rs_realtime - sun_vistest_time > SUN_VIS_TEST_PERIOD)
	{
		qglReadPixels (	sun_x, r_newrefdef.height - sun_y, 1, 1,
						GL_DEPTH_COMPONENT, GL_FLOAT, &l);
		sun_vistest_time = rs_realtime;
	}

	// ramp opacity up or down each frame
	sun_alpha += (l == 1.0 ? 1.0 : -1.0)*SUN_ALPHA_RAMP_PER_FRAME;
	sun_ramp_time = rs_realtime;

	if(sun_alpha > 1.0)  // clamp
		sun_alpha = 1.0;
	else if(sun_alpha < 0)
		sun_alpha = 0.0;

	if (sun_alpha > 0)
	{
		// set 2d
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglOrtho(0, r_newrefdef.width, r_newrefdef.height, 0, -99999,
				99999);
		qglMatrixMode(GL_MODELVIEW);
		qglPushMatrix();
		qglLoadIdentity();
		GLSTATE_ENABLE_BLEND
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);
		GL_MTexEnv (0, GL_MODULATE);
		qglDepthRange (0, 0.3);
		qglDepthMask (GL_FALSE);

		size = r_newrefdef.width * sun_size;
		PART_RenderSunFlare(sun_object, 0, size, 0, .75, .75, .75, sun_alpha);
		if (r_drawsun->value == 2) {
			PART_RenderSunFlare(sun2_object, -0.9, size * 0.07, 0, 0.1, 0.1, 0, sun_alpha);
			PART_RenderSunFlare(sun2_object, -0.7, size * 0.15, 0, 0, 0, 0.1, sun_alpha);
			PART_RenderSunFlare(sun2_object, -0.5, size * 0.085, 0, 0.1, 0, 0, sun_alpha);
			PART_RenderSunFlare(sun1_object, 0.3, size * 0.25, 0, 0.1, 0.1, 0.1, sun_alpha);
			PART_RenderSunFlare(sun2_object, 0.5, size * 0.05, 0, 0.1, 0, 0, sun_alpha);
			PART_RenderSunFlare(sun2_object, 0.64, size * 0.05, 0, 0, 0.1, 0, sun_alpha);
			PART_RenderSunFlare(sun2_object, 0.7, size * 0.25, 0, 0.1, 0.1, 0, sun_alpha);
			PART_RenderSunFlare(sun1_object, 0.85, size * 0.5, 0, 0.1, 0.1, 0.1, sun_alpha);
			PART_RenderSunFlare(sun2_object, 1.1, size * 0.125, 0, 0.1, 0, 0, sun_alpha);
			PART_RenderSunFlare(sun2_object, 1.25, size * 0.08, 0, 0.1, 0.1, 0, sun_alpha);
		}

		qglDepthMask (GL_TRUE);
		qglDepthRange(0, 1);
		qglColor4f(1, 1, 1, 1);
		GLSTATE_DISABLE_BLEND
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// set 3d
		qglPopMatrix();
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
	}
}

//Vegetation

void Mod_AddVegetation (vec3_t origin, vec3_t normal, image_t *tex, vec3_t color, float size, char name[MAX_OSPATH], int type)
{
	grass_t  *grass;
	image_t *gl;
	vec3_t binormal, tangent;

	if (r_numgrasses >= MAX_GRASSES)
		return;

	if(size == 0.0)
		size = 1.0f;

	AngleVectors(normal, NULL, tangent, binormal);
	VectorNormalize(tangent);
	VectorNormalize(binormal);

	VectorMA(origin, -32*frand(), tangent, origin);
	VectorMA(origin, 2, normal, origin);

	grass = &r_grasses[r_numgrasses++];
	VectorCopy(origin, grass->origin);

	gl = R_RegisterGfxPic (name);
	if (gl)
		grass->texsize = gl->height;
	else
		grass->texsize = 64; //sane default
	
	grass->tex = tex;
	VectorCopy(color, grass->color);
	grass->size = size;
	strcpy(grass->name, name);
	grass->type = type;
	
	grass->leafnum = CM_PointLeafnum (grass->origin);

	if(grass->type == 1)
		r_hasleaves = true;
}

void Mod_AddVegetationSurface (msurface_t *surf, image_t *tex, vec3_t color, float size, char name[MAX_QPATH], int type)
{
	glpoly_t *poly;
	vec3_t normal, origin;

	poly = surf->polys;
	
	if (surf->iflags & ISURF_PLANEBACK)
		VectorNegate(surf->plane->normal, normal);
	else
		VectorCopy(surf->plane->normal, normal);

	VectorCopy (poly->verts[0], origin);
	
	Mod_AddVegetation (origin, normal, tex, color, size, name, type);
}

// Mark any vegetation sprites that can cast shadows in sunlight, and get 
// static light levels.
void R_FinalizeGrass(void)
{
	vec3_t origin, orig2;
	grass_t *grass;
	int i;
	
	grass = r_grasses;
	
	for (i=0; i<r_numgrasses; i++, grass++) 
	{
		// get static light once at load time
		VectorCopy(grass->origin, origin);
		if (grass->type != 1)
			origin[2] += (grass->texsize/32) * grass->size;
		// XXX: HACK!
		grass->static_light_expires = R_StaticLightPoint (origin, grass->static_light);
	
		//cull for pathline to sunlight
		VectorCopy (grass->origin, orig2);
		orig2[2] += (grass->texsize/32) * grass->size;
		grass->sunVisible = CM_FastTrace (r_sunLight->origin, orig2, r_worldmodel->firstnode, MASK_VISIBILILITY);
		
		if (grass->type == 0)
		{
			//only deal with leaves, grass shadows look kind of bad
			grass->sunVisible = false;
		}
	}
}

//rendering
int compare_grass (void const *_a, void const *_b)
{
	vec3_t dist;
	int distA, distB;

	grass_t a = *(grass_t *)_a;
	grass_t b = *(grass_t *)_b;	

	VectorSubtract(a.origin, r_origin, dist);
	distA = VectorLength(dist);
	VectorSubtract(b.origin, r_origin, dist);
	distB = VectorLength(dist);

	if (distA > distB)
		return -1;
	else if (distA < distB)
		return 1;

	return 0;
}

static int g_lastGSort = 0;
void R_DrawVegetationSurface ( void )
{
	int		i;
	grass_t *grass;
	float   scale;
	vec3_t	origin;
	qboolean visible;
	float	lightLevel[3];

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	//sort the grasses from furthest to closest(we can safely do this every 3/4 second
	//instead of every frame since our POV shouldn't change that drastically enough
	//to create artifacts.  Grasses must be sorted to prevent major artifacting.
	if(g_lastGSort < Sys_Milliseconds() - 750)
	{
		qsort( r_grasses, r_numgrasses, sizeof( r_grasses[0] ), compare_grass );
		g_lastGSort = Sys_Milliseconds();
	}
	
	GL_SelectTexture (0);
	
	qglDepthMask( GL_FALSE );
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	GL_TexEnv (GL_MODULATE);
	qglDisable (GL_CULL_FACE);
	
	GL_SetupVegetationVBO ();
	glUseProgramObjectARB (g_vegetationprogramObj);
	glUniform1fARB (vegetation_uniforms.rsTime, rs_realtime);
	glUniform3fARB (vegetation_uniforms.up, vup[0], vup[1], vup[2]);
	glUniform3fARB (vegetation_uniforms.right, vright[0], vright[1], vright[2]);
	
	for (grass = r_grasses, i = 0; i < r_numgrasses; i++, grass++)
	{
		scale = 10.0*grass->size;

		VectorCopy(grass->origin, origin);		

		if (distancecull_particle (origin, scale))
			continue;
		
		if (grass->type == 1) // foliage
		{
			grass->alpha = 1.0;
			visible = true; //leaves tend to use much larger images, culling results in undesired effects
		}
		else if (grass->type == 2) // shrubbery
		{
			grass->alpha = 1.0;
			visible = true; //leaves tend to use much larger images, culling results in undesired effects
			
			// adjust vertical position, scaled
			origin[2] += (grass->texsize/32.0) * grass->size/(grass->texsize/128.0);
		}
		else // grass
		{
			// adjust vertical position, scaled
			origin[2] += (grass->texsize/32.0) * grass->size/(grass->texsize/128.0);

			// periodically test visibility to ramp alpha
			if(rs_realtime - grass->time > 0.02){

				if (grass->type == 1 && !CM_inPVS_leafs (r_origin_leafnum, grass->leafnum))
					continue;

				visible = CM_FastTrace (r_origin, grass->origin, r_worldmodel->firstnode, MASK_VISIBILILITY);
			
				grass->alpha += (visible ? 0.03 : -0.15);  // ramp

				if(grass->alpha > 1.0)  // clamp
					grass->alpha = 1.0;
				else if(grass->alpha < 0)
					grass->alpha = 0.0;

				grass->time = rs_realtime;
			}
			if (grass->alpha > 0.0)
				visible = true;
			else
				visible = false;
		}
		
		if(visible)
		{
			c_grasses++;
			
			GL_Bind(grass->tex->texnum);
			
			if(gl_dynamic->integer)
				R_DynamicLightPoint (origin, lightLevel);
			else
				VectorClear (lightLevel);
			if (grass->static_light_expires)
			    R_StaticLightPoint (origin, grass->static_light);
			VectorAdd (grass->static_light, lightLevel, lightLevel);
			
			VectorScale(lightLevel, 2.0, lightLevel);
			qglColor4f( grass->color[0]*(lightLevel[0]+0.1),grass->color[1]*(lightLevel[1]+0.1),grass->color[2]*(lightLevel[2]+0.1), grass->alpha );
			
			R_DrawVarrays (GL_QUADS, grass->vbo_first_vert, grass->vbo_num_verts);
		}
	}
	
	glUseProgramObjectARB (0);
	
	R_KillVArrays ();

	qglColor4f (1,1,1,1);
	GLSTATE_DISABLE_BLEND
	qglDepthMask (GL_TRUE);
	GL_TexEnv (GL_REPLACE);
	qglEnable (GL_CULL_FACE);
}

void R_ClearGrasses(void)
{
	memset(r_grasses, 0, sizeof(r_grasses));
	r_numgrasses = 0;
	r_hasleaves = 0;
}

//Light beams/volumes

void Mod_AddBeamSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type, float xang, float yang,
	qboolean rotating)
{
	beam_t  *beam;
	image_t *gl;
	vec3_t mins, maxs;
	vec3_t origin = {0,0,0}, binormal, tangent, tmp;

	if (r_numbeams >= MAX_BEAMS)
			return;

	if(size == 0.0)
		size = 1.0f;

	R_Surface_Bounds (surf, mins, maxs, origin);

	AngleVectors(surf->plane->normal, NULL, tangent, binormal);
	VectorNormalize(tangent);
	VectorNormalize(binormal);

	if (surf->iflags & ISURF_PLANEBACK)
		VectorNegate(surf->plane->normal, tmp);
	else
		VectorCopy(surf->plane->normal, tmp);

	VectorMA(origin, 2, tmp, origin);

	beam = &r_beams[r_numbeams++];
	VectorCopy(origin, beam->origin);

	gl = R_RegisterGfxPic (name);
	if (gl)
		beam->texsize = gl->height;
	else
		beam->texsize = 64; //sane default

	beam->texnum = texnum;
	VectorCopy(color, beam->color);
	beam->size = size;
	strcpy(beam->name, name);
	beam->type = type;
	beam->xang = xang;
	beam->yang = yang;
	beam->rotating = rotating;
	beam->leafnum = 0;
	
	if (!beam->rotating)
	{
		vec3_t start, end;
		double maxang;
		float movdir;
		
		if(fabs(beam->xang) > fabs(beam->yang))
			maxang = beam->xang;
		else
			maxang = beam->yang;
			
		if(maxang >= 0.0)
			movdir = 1.0;
		else
			movdir = 0;
		
		VectorCopy(beam->origin, start);
		if(!beam->type)
			start[2] -= (2.5 - (10.0*maxang))*beam->size*movdir;
		else
			start[2] += (2.5 - (10.0*maxang))*beam->size*movdir;
		
		VectorCopy(start, end);
		if(!beam->type)
			end[2] -= (2.5 + pow(fabs(maxang),2)*10)*beam->size;
		else
			end[2] += (2.5 + pow(fabs(maxang),2)*10)*beam->size;

		end[0] += (pow(fabs(maxang*10), 3)*beam->xang)*beam->size; //angle in rads
		end[1] += (pow(fabs(maxang*10), 3)*beam->yang)*beam->size;
		
		beam->leafnum = CM_PointLeafnum (start);
		beam->leafnum2 = CM_PointLeafnum (end);
		if (beam->leafnum == beam->leafnum2)
			beam->leafnum2 = 0;
	}
}

//Rendering

void R_DrawBeamSurface ( void )
{
	int		i, j;
	beam_t *beam;
	double   scale, maxang;
	vec3_t	start, end, mins, maxs, angle, right, up, delta;
	qboolean visible;
	trace_t r_trace;
	vec3_t	absmins, absmaxs;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	beam = r_beams;

	VectorSet(mins, 32, 32, 64);
	VectorSet(maxs, -32, -32, -64);
	
	GL_SelectTexture (0);

	R_InitVArrays (VERT_SINGLE_TEXTURED);
	qglDepthMask (GL_FALSE);
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);
	GL_TexEnv (GL_MODULATE);

	for (i=0; i<r_numbeams; i++, beam++) {
		float movdir;
		
		if (	beam->leafnum && beam->leafnum2 && 
				!CM_inPVS_leafs (r_origin_leafnum, beam->leafnum) && 
				!CM_inPVS_leafs (r_origin_leafnum, beam->leafnum2))
			continue;
		if (	beam->leafnum && !beam->leafnum2 && 
				!CM_inPVS_leafs (r_origin_leafnum, beam->leafnum))
			continue;
		
		scale = 10.0*beam->size;

		if(fabs(beam->xang) > fabs(beam->yang))
			maxang = beam->xang;
		else
			maxang = beam->yang;

		if(maxang >= 0.0)
			movdir = 1.0;
		else
			movdir = 0;
		
		//to do - this is rather hacky, and really only works for 0 degrees and 45 degree angles(0.25 rads).  
		//will revisit this as needed, for now it works for what I need it for.
		VectorCopy(beam->origin, start);
		if(!beam->type)
			start[2] -= (2.5 - (10.0*maxang))*beam->size*movdir;
		else
			start[2] += (2.5 - (10.0*maxang))*beam->size*movdir;
		
		VectorCopy(start, end);
		if(!beam->type)
			end[2] -= (2.5 + pow(fabs(maxang),2)*10)*beam->size;
		else
			end[2] += (2.5 + pow(fabs(maxang),2)*10)*beam->size;

		if(beam->rotating)
		{
			end[0] += sin(rs_realtime)*(pow(fabs(maxang*10), 3)*beam->xang)*beam->size; //angle in rads
			end[1] += cos(rs_realtime)*(pow(fabs(maxang*10), 3)*beam->yang)*beam->size;
		}
		else
		{
			end[0] += (pow(fabs(maxang*10), 3)*beam->xang)*beam->size; //angle in rads
			end[1] += (pow(fabs(maxang*10), 3)*beam->yang)*beam->size;
		}

		VectorSubtract(end, start, up);
		if(!beam->type)
			VectorScale(up, -beam->size, up);
		else
			VectorScale(up, beam->size, up);

		VectorAdd(start, up, angle);
		
		VectorNormalize(up);

		VectorSubtract(r_newrefdef.vieworg, angle, delta);
		CrossProduct(up, delta, right);
		VectorNormalize(right);

		VectorScale(right, scale, right);
		VectorScale(up, scale, up);
		
		VectorCopy (end, absmins);
		VectorCopy (end, absmaxs);
		
		for (j = 0; j < 3; j++)
		{
			if (right[j] < 0.0)
			{
				absmins[j] += right[j]*0.5;
				absmaxs[j] -= right[j]*0.5;
			}
			else
			{
				absmins[j] -= right[j]*0.5;
				absmaxs[j] += right[j]*0.5;
			}
			
			if (up[j] < 0.0)
			{
				absmins[j] += up[j]*0.5;
				absmaxs[j] -= up[j]*0.5;
			}
			else
			{
				absmins[j] -= up[j]*0.5;
				absmaxs[j] += up[j]*0.5;
			}
		}
		
		if (R_CullBox (absmins, absmaxs))
			continue;
		
		// TODO: establish if we should even be using CM_BoxTrace at all--
		// performance-wise it's very bad to use it a lot.
		r_trace = CM_BoxTrace(r_origin, beam->origin, mins, maxs, r_worldmodel->firstnode, MASK_VISIBILILITY);
		visible = r_trace.fraction == 1.0;
		
		if(visible) {
			//render polygon
			qglColor4f( beam->color[0],beam->color[1],beam->color[2], 1 );

			GL_Bind(beam->texnum);
			
			VArray = &VArrayVerts[0];
			PART_AddBillboardToVArray (end, up, right, 0, false, 0, 1, 0, 1);

			R_DrawVarrays(GL_QUADS, 0, 4);

			c_beams++;
		}
	}

	R_KillVArrays ();

	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglColor4f (1,1,1,1 );
	GLSTATE_DISABLE_BLEND
	qglDepthMask (GL_TRUE);
	GL_TexEnv (GL_REPLACE);
}

void R_ClearBeams(void)
{
	memset(r_beams, 0, sizeof(r_beams));
	r_numbeams = 0;
}

//Simple items

//simple item images
image_t		*s_item0;
image_t		*s_item1;
image_t		*s_item2;
image_t		*s_item3;
image_t		*s_item4;
image_t		*s_item5;
image_t		*s_item6;
image_t		*s_item7;
image_t		*s_item8;
image_t		*s_item9;
image_t		*s_item10;
image_t		*s_item11;
image_t		*s_item12;
image_t		*s_item13;
image_t		*s_item14;
image_t		*s_item17;
image_t		*s_item18;
image_t		*s_item19;
image_t		*s_item20;
image_t		*s_item21;
image_t		*s_item22;
image_t		*s_item23;
image_t		*s_item24;

extern int gl_filter_max;
void R_SI_InitTextures( void )
{
	byte	nullpic[16][16][4];
	int x, y;

	//
	// blank texture
	//
	for (x = 0 ; x < 16 ; x++)
	{
		for (y = 0 ; y < 16 ; y++)
		{
			nullpic[y][x][0] = 255;
			nullpic[y][x][1] = 255;
			nullpic[y][x][2] = 255;
			nullpic[y][x][3] = 255;
		}
	}

#define R_SI_InitTexture(itemnum,imgname) \
	/* load the texture */ \
	s_item ## itemnum = GL_FindImage ("pics/" #imgname ".tga,", it_pic);\
	/* load a blank texture if it isn't found */ \
	if (!s_item ## itemnum) \
		s_item ## itemnum = GL_LoadPic ("***s_item" #itemnum "***", (byte *)nullpic, 16, 16, it_pic, 32); \
	/* Disable more than one mipmap level. FIXME: improve mushy mipmap 
	 * detection so this isn't necessary.
	 */ \
	GL_Bind(s_item ## itemnum ->texnum); \
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);

	R_SI_InitTexture (0,  w_chaingun);
	R_SI_InitTexture (1,  w_beamgun);
	R_SI_InitTexture (2,  w_flamethrower);
	R_SI_InitTexture (3,  w_rlauncher);
	R_SI_InitTexture (4,  w_smartgun);
	R_SI_InitTexture (5,  p_haste);
	R_SI_InitTexture (6,  p_invulnerability);
	R_SI_InitTexture (7,  p_quad);
	R_SI_InitTexture (8,  p_sproing);
	R_SI_InitTexture (9,  p_adrenaline);
	R_SI_InitTexture (10, p_shard);
	R_SI_InitTexture (11, p_jacket);
	R_SI_InitTexture (12, p_combat);
	R_SI_InitTexture (13, p_body);
	R_SI_InitTexture (14, p_health);
	R_SI_InitTexture (17, i_beamgun);
	R_SI_InitTexture (18, i_vaporizer);
	R_SI_InitTexture (19, i_disruptor);
	R_SI_InitTexture (20, i_flamethrower);
	R_SI_InitTexture (21, i_smartgun);
	R_SI_InitTexture (22, i_chaingun);
	R_SI_InitTexture (23, i_rocketlauncher);
	R_SI_InitTexture (24, i_minderaser);
}

//rendering
extern cvar_t *cl_simpleitems;
void R_DrawSimpleItems ( void )
{
	int		i;
	float   scale;
	vec3_t	origin, angle, right, up;
	
	if (!cl_simpleitems->integer)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	GL_SelectTexture (0);
	
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction ( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
	GL_Bind (0);
	qglTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );
	qglTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	GL_TexEnv (GL_MODULATE);

	qglDepthMask( GL_TRUE );
	GLSTATE_ENABLE_ALPHATEST
	
	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for (i = 0 ; i < r_newrefdef.num_entities; i++)
	{
	    model_t *mod = r_newrefdef.entities[i].model;
		if (!mod || !mod->simple_texnum)
			continue;
		
		VArray = &VArrayVerts[0];

		VectorCopy (r_newrefdef.entities[i].origin, origin);

		//PVS checking is not necessary; the server already does that for
		//entities.
		if (CM_FastTrace (r_origin, origin, r_worldmodel->firstnode, MASK_VISIBILILITY))
		{
			GL_Bind (mod->simple_texnum);
			switch (mod->simple_color)
			{
				case simplecolor_white:
					qglColor4f( 1, 1, 1, 1 ); //uses texture for color unless specified
					break;
				case simplecolor_green:
					qglColor4f( 0, 1, 0, 1 ); 
					break;
				case simplecolor_blue:
					qglColor4f( 0, .3, 1, 1 ); 
					break;
				case simplecolor_purple:
					qglColor4f( 1, 0, 1, 1 ); 
					break;
			}
			if (	(mod->simple_texnum == s_item17->texnum) ||
					(mod->simple_texnum == s_item18->texnum) ||
					(mod->simple_texnum == s_item19->texnum) ||
					(mod->simple_texnum == s_item20->texnum) ||
					(mod->simple_texnum == s_item21->texnum) ||
					(mod->simple_texnum == s_item22->texnum) ||
					(mod->simple_texnum == s_item23->texnum) ||
					(mod->simple_texnum == s_item24->texnum))
			{
				scale = 40.0;
			}
			else
			{
				scale = 20.0;
			}
					
			VectorCopy(r_newrefdef.viewangles, angle);
			
			angle[0] = 0;  // keep vertical by removing pitch

			AngleVectors(angle, NULL, right, up);
			VectorScale(right, scale, right);
			VectorScale(up, scale, up);			

			PART_AddBillboardToVArray (origin, up, right, 0, true, 0, 1, 0, 1);

			R_DrawVarrays(GL_QUADS, 0, 4);
		}
	}
	
	R_KillVArrays ();

	GL_BlendFunction ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST
	GL_TexEnv (GL_REPLACE);
}

void R_SetSimpleTexnum (model_t *loadmodel, const char *pathname)
{
	char	shortname[MAX_OSPATH];

	loadmodel->simple_texnum = 0;
	loadmodel->simple_color = simplecolor_white;
	COM_StripExtension(pathname, shortname);
	if (!Q_strcasecmp (shortname, "models/items/ammo/bullets/medium/tris"))
	{
		loadmodel->simple_texnum = s_item0->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/ammo/cells/medium/tris"))
	{
		loadmodel->simple_texnum = s_item1->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/ammo/grenades/medium/tris"))
	{
		loadmodel->simple_texnum = s_item2->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/ammo/rockets/medium/tris"))
	{
		loadmodel->simple_texnum = s_item3->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/ammo/shells/medium/tris"))
	{
		loadmodel->simple_texnum = s_item4->texnum;
	}
	//powerups
	else if (!Q_strcasecmp (shortname, "models/items/haste/tris"))
	{
		loadmodel->simple_texnum = s_item5->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/invulner/tris"))
	{
		loadmodel->simple_texnum = s_item6->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/quaddama/tris"))
	{
		loadmodel->simple_texnum = s_item7->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/sproing/tris"))
	{
		loadmodel->simple_texnum = s_item8->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/adrenaline/tris"))
	{
		loadmodel->simple_texnum = s_item9->texnum;
	}
	//armor
	else if (!Q_strcasecmp (shortname, "models/items/armor/shard/tris"))
	{
		loadmodel->simple_texnum = s_item10->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/armor/jacket/tris"))
	{
		loadmodel->simple_texnum = s_item11->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/armor/combat/tris"))
	{
		loadmodel->simple_texnum = s_item12->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/items/armor/body/tris"))
	{
		loadmodel->simple_texnum = s_item13->texnum;
	}
	//health
	else if (!Q_strcasecmp (shortname, "models/items/healing/small/tris"))
	{
		loadmodel->simple_texnum = s_item14->texnum;
		loadmodel->simple_color = simplecolor_green;
	}
	else if (!Q_strcasecmp (shortname, "models/items/healing/medium/tris"))
	{
		loadmodel->simple_texnum = s_item14->texnum;
		loadmodel->simple_color = simplecolor_blue;
	}
	else if (!Q_strcasecmp (shortname, "models/items/healing/large/tris"))
	{
		loadmodel->simple_texnum = s_item14->texnum;
		loadmodel->simple_color = simplecolor_purple;
	}
	//weapons
	else if (!Q_strcasecmp (shortname, "models/weapons/g_beamgun/tris"))
	{
		loadmodel->simple_texnum = s_item17->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_vaporizer/tris"))
	{
		loadmodel->simple_texnum = s_item18->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_disruptor/tris"))
	{
		loadmodel->simple_texnum = s_item19->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_flamethrower/tris"))
	{
		loadmodel->simple_texnum = s_item20->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_smartgun/tris"))
	{
		loadmodel->simple_texnum = s_item21->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_chaingun/tris"))
	{
		loadmodel->simple_texnum = s_item22->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_rocket/tris"))
	{
		loadmodel->simple_texnum = s_item23->texnum;
	}
	else if (!Q_strcasecmp (shortname, "models/weapons/g_minderaser/tris"))
	{
		loadmodel->simple_texnum = s_item24->texnum;
	}
}
