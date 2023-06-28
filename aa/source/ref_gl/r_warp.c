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
// r_warp.c -- sky and water polygons

// NOTE: Sky rendering is only here due to a historical accident. TODO: find
// a better file for it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

extern	model_t	*loadmodel;

char	skyname[MAX_QPATH];
float	skyrotate;
vec3_t	skyaxis;
image_t	*sky_images[6];

msurface_t	*warpface;

#define	SUBDIVIDE_SIZE	64

void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon (int numverts, float *verts)
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t	*poly;
	float	s, t;
	vec3_t	total;
	float	total_s, total_t;

	if (numverts > 60)
		Com_Error (ERR_DROP, "numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor (m/SUBDIVIDE_SIZE + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Hunk_Alloc (sizeof(glpoly_t) + ((numverts-4)+2) * VERTEXSIZE*sizeof(float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts+2;
	VectorClear (total);
	total_s = 0;
	total_t = 0;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i+1]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd (total, verts, total);

		poly->verts[i+1][3] = s;
		poly->verts[i+1][4] = t;
	}

	VectorScale (total, (1.0/numverts), poly->verts[0]);
	poly->verts[0][3] = total_s/numverts;
	poly->verts[0][4] = total_t/numverts;

	// copy first vertex to last
	memcpy (poly->verts[i+1], poly->verts[1], sizeof(poly->verts[0]));
}

/*
================
R_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.

TODO: use GPU tesselation!
================
*/
void R_SubdivideSurface (msurface_t *fa, int firstedge, int numedges)
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<numedges ; i++)
	{
		lindex = loadmodel->surfedges[firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (numverts, verts[0]);
}

//=========================================================



/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void R_RenderWaterPolys (msurface_t *fa)
{
	rscript_t	*rs_shader;
	int			texnum;
	float		scaleX, scaleY;
	
	rs_shader = NULL;
	if (r_shaders->integer)
		rs_shader = fa->texinfo->image->script;
	
	scaleX = scaleY = 1.0f;
	texnum = 0;
	
	if(rs_shader && rs_shader->stage) 
	{
		//for now, just map a reflection texture
		rs_stage_t *stage = rs_shader->stage;
		
		texnum = stage->texture->texnum; //pass this to renderwaterpolys
		
		if(stage->scale.scaleX != 0.0f && stage->scale.scaleY != 0.0f) 
		{
			scaleX = stage->scale.scaleX;
			scaleY = stage->scale.scaleY;
		}
	}

	if(fa->texinfo->has_normalmap)
	{
		if (SurfaceIsAlphaMasked (fa))
			GLSTATE_ENABLE_ALPHATEST

		glUseProgramObjectARB( g_waterprogramObj );

		GL_MBind (0, fa->texinfo->image->texnum);
		glUniform1iARB( g_location_baseTexture, 0);

		GL_MBind (1, fa->texinfo->normalMap->texnum);
		glUniform1iARB( g_location_normTexture, 1);

		if (texnum)
		{
			GL_MBind (2, texnum);
			glUniform1iARB( g_location_refTexture, 2);
		}
		else
		{
			glUniform1iARB( g_location_refTexture, 0);
		}

		if(fa->texinfo->flags &(SURF_TRANS33|SURF_TRANS66))
			glUniform1fARB( g_location_trans, 0.5);
		else
			glUniform1fARB( g_location_trans, 0.75);
		
		if(texnum)
			glUniform1iARB( g_location_reflect, 1);
		else
			glUniform1iARB( g_location_reflect, 0);

		//send these to the shader program	
		glUniform3fARB( g_location_lightPos, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);
		glUniform1iARB( g_location_fogamount, map_fog);
		glUniform1fARB( g_location_time, rs_realtime);
		
		BSP_AddSurfToVBOAccum (fa);
		BSP_FlushVBOAccum ();

		glUseProgramObjectARB( 0 );

		if (SurfaceIsAlphaMasked (fa))
			GLSTATE_DISABLE_ALPHATEST
	}
	else
	{
		// Be sure to check the warptest map when you change this code
		float scroll = 0.0f;
		
		if (fa->texinfo->flags & SURF_FLOWING)
			scroll = -((r_newrefdef.time / 2.0f) - (int)(r_newrefdef.time / 2.0f));
		
		GL_MBind (0, fa->texinfo->image->texnum);
		
		GL_SelectTexture (0);
		qglMatrixMode (GL_TEXTURE);
		qglPushMatrix ();
		qglLoadIdentity ();
		qglTranslatef (scroll, 0, 0);
		qglScalef (1.0f/64.0f, 1.0f/64.f, 1);
		
		glUseProgramObjectARB (g_warpprogramObj);
		glUniform1fARB (warp_uniforms.time, r_newrefdef.time);
		glUniform1iARB (warp_uniforms.warpvert, !(fa->texinfo->flags & SURF_FLOWING));
		
		// Already subdivided
		BSP_AddSurfToVBOAccum (fa);
		
		if (!texnum || (SurfaceIsTranslucent(fa) && !SurfaceIsAlphaMasked (fa)))
		{
			glUniform1iARB (warp_uniforms.envmap, 0);
			BSP_DrawVBOAccum ();
		}
		
		if (texnum)
		{
			GL_MBind (0, texnum);
			glUniform1iARB (warp_uniforms.envmap, 1);
			qglLoadIdentity ();
			qglScalef (scaleX/512.0f, scaleY/512.0f, 1);
			qglTranslatef (128.0f*scaleX - r_newrefdef.vieworg[0], r_newrefdef.vieworg[1] + 128.0f*scaleY, 0);
			BSP_DrawVBOAccum ();
		}
		
		BSP_ClearVBOAccum ();
		
		glUseProgramObjectARB (0);

		GL_SelectTexture (0);
		qglPopMatrix ();
		qglMatrixMode (GL_MODELVIEW);
	}
}

// SKY RENDERING

// The Quake 2 code we started with did a bunch of elaborite math to minimize
// the amount of skybox that got drawn. However, modern graphics hardware
// (i.e. 2001 and newer) has early Z rejection, making this unnecessary. We
// now draw the whole skybox all the time, which in testing has been shown to
// have no appreciable performance hit.

/*
==============
R_DrawSkyBox
==============
*/
static int	skytexorder[6] = {0,2,1,3,4,5};

static int skyboxside;
static void Sky_DrawQuad_Callback (void)
{
	R_DrawVarrays (GL_QUADS, 4 * skyboxside, 4);
}

void R_DrawSkyBox (void)
{
	int		i;
	rscript_t *rs = NULL;
	
	GL_SetupSkyboxVBO ();

	qglPushMatrix ();
	qglTranslatef (r_origin[0], r_origin[1], r_origin[2]);

	if (skyrotate != 0)
	{
		qglRotatef (r_newrefdef.time * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);
	}
	else
	{
		// Set angle based on fog script
		if (r_skyangleX != 0)
			qglRotatef (r_skyangleX, 1, 0, 0);
		if (r_skyangleY != 0)
			qglRotatef (r_skyangleY, 0, 1, 0);
		if (r_skyangleZ != 0)
			qglRotatef (r_skyangleZ, 0, 0, 1);
	}
	
	// Scale the fog distances up so fog looks like it used to before 
	// SKYDIST was increased. 
	R_SetupFog (((float)SKYDIST)/2300.0);

	GL_SelectTexture (0);

	for (i=0 ; i<6 ; i++)
	{
		GL_Bind (sky_images[skytexorder[i]]->texnum);

		skyboxside = i;
		Sky_DrawQuad_Callback ();
	}
	
	qglPopMatrix ();

	if(r_shaders->integer) { //just cloud layers for now, we can expand this

		qglPushMatrix (); //rotate the clouds
		qglTranslatef (r_origin[0], r_origin[1], r_origin[2]);
		qglRotatef (rs_realtime * 20, 0, 1, 0);
		
		qglDepthMask( GL_FALSE );	 	// no z buffering
		
		for (i=0 ; i<6 ; i++)
		{
			rs=(rscript_t *)sky_images[skytexorder[i]]->script;
			
			if (rs == NULL)
				continue;
			
			skyboxside = i;
			RS_Draw (rs, NULL, 0, vec3_origin, vec3_origin, false, rs_lightmap_off, false, false, Sky_DrawQuad_Callback);
		}
		
		// restore the original blend mode
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
		GL_BlendFunction ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		qglColor4f( 1,1,1,1 );
		qglDepthMask( GL_TRUE );	// back to normal Z buffering
		GL_TexEnv( GL_REPLACE );
		qglPopMatrix ();
	}
	
	R_KillVArrays ();
	
	R_SetupFog (1);
}


/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
void R_SetSky (char *name, float rotate, vec3_t axis)
{
	int		i;
	char	pathname[MAX_QPATH];

	strncpy (skyname, name, sizeof(skyname)-1);
	skyrotate = rotate;
	VectorCopy (axis, skyaxis);

	for (i=0 ; i<6 ; i++)
	{
		if (gl_skymip->integer)
			gl_picmip->integer += gl_skymip->integer;

		Com_sprintf (pathname, sizeof(pathname), "env/%s%s.jpg", skyname, suf[i]);

		sky_images[i] = GL_FindImage (pathname, it_sky);
		if (!sky_images[i])
			sky_images[i] = r_notexture;
		else 
		{
			//valid sky, load shader
			strcpy(pathname,sky_images[i]->name);
			pathname[strlen(pathname)-4]=0;
			if(sky_images[i]->script)
				RS_ReadyScript(sky_images[i]->script);
		}
		
		// get rid of nasty visible seams in the skybox (better method than 
		// screwing around with the texcoords)
		GL_SelectTexture (0);
		GL_Bind (sky_images[i]->texnum);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

		//set only if not set by fog script
		if(r_sunX == 0.0 && r_sunY == 0.0 && r_sunZ == 0.0)
		{
			if (strstr(pathname, "hspace")) {
				VectorSet(sun_origin, -5000, -100000, 115000);
				spacebox = true;
			}
			else {
				VectorSet(sun_origin, 140000, -80000, 45000);
				spacebox = false;
			}
		}
		else
		{
			VectorSet(sun_origin, r_sunX, r_sunY, r_sunZ);
			spacebox = false; //we will change this to sun vs star
		}

		if(r_sunIntens == 0.0)
			r_sunIntens = 1.0;

		if (gl_skymip->integer)
			gl_picmip->integer -= gl_skymip->integer;
	}
}
