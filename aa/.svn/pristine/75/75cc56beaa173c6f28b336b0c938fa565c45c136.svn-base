/*
Copyright (C) 2009-2014 COR Entertainment

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

#define EXPLOSION 1
#define PAIN 2

extern float v_blend[4];
extern void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out );
void R_DrawBloodEffect (void);

image_t *r_framebuffer;
image_t *r_colorbuffer;
image_t *r_depthbuffer;
image_t *r_distortwave;
image_t *r_droplets;
image_t	*r_blooddroplets;
image_t	*r_blooddroplets_nm;

vec3_t r_explosionOrigin;
int r_drawing_fbeffect;
int	r_fbFxType;
float r_fbeffectTime;

extern  cvar_t	*cl_raindist;

// TODO: fix the way FBOs all over the codebase "leak" at vid_restart
// Note - this has been botched and hacked into a complete mess. It needs a complete overhaul, and 
// should use the Frenzy engine's codebase. 
static GLuint distort_FBO, godray_FBO, ppDepth_FBO;
GLuint pp_FBO;

/*
=================
R_Postprocess_AllocFBOTexture 

Create a 24-bit texture with specified size and attach it to an FBO
=================
*/
image_t *R_Postprocess_AllocFBOTexture (char *name, int width, int height, GLuint *FBO)
{
	byte	*data;
	int		size;
	image_t	*image;
	
	size = width * height * 3;
	data = malloc (size);
	memset (data, 0, size);
	
	// create the texture
	image = GL_FindFreeImage (name, width, height, it_pic);
	GL_Bind (image->texnum);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	qglTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	
	// create up the FBO
	qglGenFramebuffersEXT (1, FBO);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, *FBO);

	// bind the texture to it
	qglFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, image->texnum, 0);
	
	// clean up 
	free (data);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
	
	return image;
}

// Be sure to set up your GLSL program and uniforms before calling this! Make
// sure r_framebuffer is already bound to a TMU, and tell this function which
// TMU it is.
static void Distort_RenderQuad (int x, int y, int w, int h)
{
	int xl = x, yt = y, xr = x + w, yb = y + h;
	
	//set up full screen workspace
	qglMatrixMode (GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -10, 100);
	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();
	
	GLSTATE_DISABLE_BLEND
	qglDisable (GL_DEPTH_TEST);
	
	GL_MBind (0, r_framebuffer->texnum);
	
	// Ie need to grab the frame buffer. If we're not working with the whole
	// framebuffer, copy a margin to avoid artifacts
	qglBindFramebufferEXT (GL_DRAW_FRAMEBUFFER_EXT, distort_FBO);
	if (xl > 0) xl--;
	if (xr < vid.width) xr++;
	if (yt > 0) yt--;
	if (yb < vid.height) yb++;
	qglBlitFramebufferEXT (xl, yt, xr, yb, xl, yt, xr, yb, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	qglBindFramebufferEXT (GL_DRAW_FRAMEBUFFER_EXT, 0);
	
	qglViewport (x, y, w, h);
	
	// Set the texture matrix so the GLSL shader knows only to sample part
	// of the FB texture
	GL_SelectTexture (0);
	qglMatrixMode (GL_TEXTURE);
	qglPushMatrix ();
	qglTranslatef ((float)x/(float)vid.width, (float)y/(float)vid.height, 0);
	qglScalef ((float)w/(float)vid.width, (float)h/(float)vid.height, 1);
	
	// FIXME: textures captured from the framebuffer need to be rendered
	// upside down for some reason
	GL_SetupWholeScreen2DVBO (wholescreen_fliptextured);
	R_DrawVarrays (GL_QUADS, 0, 4);
	
	qglPopMatrix ();
	qglMatrixMode (GL_MODELVIEW);
	R_KillVArrays ();
	R_SetupViewport ();
	GLSTATE_ENABLE_BLEND
	qglEnable (GL_DEPTH_TEST);
}

void R_GLSLDistortion(void)
{
	vec3_t	vec;
	float	dot, r_fbeffectLen;
	vec3_t	forward, mins, maxs;
	trace_t r_trace;
	
	if(r_fbFxType == EXPLOSION) 
	{
		//is it in our view?
		AngleVectors (r_newrefdef.viewangles, forward, NULL, NULL);

		VectorSubtract (r_explosionOrigin, r_newrefdef.vieworg, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			r_drawing_fbeffect = false;

		//is anything blocking it from view?
		VectorSet(mins, 0, 0, 0);
		VectorSet(maxs,	0, 0, 0);

		r_trace = CM_BoxTrace(r_origin, r_explosionOrigin, maxs, mins, r_worldmodel->firstnode, MASK_VISIBILILITY);
		if (r_trace.fraction != 1.0)
			r_drawing_fbeffect = false;
	}

	//if not doing stuff, return
	if(!r_drawing_fbeffect)
		return;

	if(r_fbFxType == EXPLOSION)
	{
		vec3_t ul_3d, lr_3d;
		vec2_t ul_2d, lr_2d;
		float scale, intensity;
		
		r_fbeffectLen = 0.2f;
		
		//create a distortion wave effect at point of explosion
		glUseProgramObjectARB( g_fbprogramObj );

		glUniform1iARB (distort_uniforms.framebuffTex, 0);

		if(r_distortwave)
			GL_MBind (1, r_distortwave->texnum);
		glUniform1iARB (distort_uniforms.distortTex, 1);

		// get positions of bounds of warp area
		scale = 8.0f + (rs_realtime - r_fbeffectTime) * 78.0f / r_fbeffectLen;
		VectorMA (r_explosionOrigin, -scale, vright, ul_3d);
		VectorMA (ul_3d, scale, vup, ul_3d);

		R_TransformVectorToScreen(&r_newrefdef, ul_3d, ul_2d);
		VectorMA (r_explosionOrigin, scale, vright, lr_3d);
		VectorMA (lr_3d, -scale, vup, lr_3d);
		R_TransformVectorToScreen(&r_newrefdef, lr_3d, lr_2d);
		
		intensity = sinf (M_PI * (rs_realtime - r_fbeffectTime) / r_fbeffectLen);
		glUniform1fARB (distort_uniforms.intensity, intensity);
		
		// Note that r_framebuffer is on TMU 1 this time
		Distort_RenderQuad (ul_2d[0], lr_2d[1], lr_2d[0] - ul_2d[0], ul_2d[1] - lr_2d[1]);
		
		glUseProgramObjectARB (0);
		GL_MBind (1, 0);
	}
	else
	{
		r_fbeffectLen = 0.2f;
		R_DrawBloodEffect();
		
		//do a radial blur
		glUseProgramObjectARB( g_rblurprogramObj );

		glUniform1iARB( g_location_rsource, 0);

		glUniform3fARB( g_location_rparams, 1.0, 0.5, 0.5);

		Distort_RenderQuad (0, 0, vid.width, vid.height);

		glUseProgramObjectARB( 0 );
	}

	if(rs_realtime > r_fbeffectTime + r_fbeffectLen) 
		r_drawing_fbeffect = false; //done effect

	return;
}

void R_GLSLWaterDroplets(void)
{
	trace_t tr;
	vec3_t end;
	static float r_drTime;

	if(!(r_weather == 1) || !cl_raindist->integer)
		return;

	VectorCopy(r_newrefdef.vieworg, end);
	end[2] += 8192;

	// trace up looking for sky
	tr = CM_BoxTrace(r_newrefdef.vieworg, end, vec3_origin, vec3_origin, 0, MASK_SHOT);

	if((tr.surface->flags & SURF_SKY))
	{
		r_drTime = rs_realtime;
	}

	if(rs_realtime - r_drTime > 0.5)
		return; //been out of the rain long enough for effect to dry up
	
	//draw water droplets - set up GLSL program and uniforms
	glUseProgramObjectARB( g_dropletsprogramObj ); //this program will have two or three of the normalmap scrolling over the buffer

	glUniform1iARB( g_location_drSource, 0);

	GL_MBind (1, r_droplets->texnum);
	glUniform1iARB( g_location_drTex, 1);

	glUniform1fARB( g_location_drTime, rs_realtime);
	
	Distort_RenderQuad (0, 0, vid.width, vid.height);

	glUseProgramObjectARB( 0 );

	return;
}

void R_GLSLDOF(void)
{	
	glUseProgramObjectARB(g_DOFprogramObj); 

	glUniform1iARB(g_location_dofSource, 0);

	GL_MBind(1, r_depthbuffer->texnum);
	glUniform1iARB(g_location_dofDepth, 1);

	GL_MBind(0, r_colorbuffer->texnum);
	glUniform1iARB(g_location_dofSource, 0);

	//render quad 
	GLSTATE_DISABLE_BLEND
	qglDisable(GL_DEPTH_TEST);

	GL_SetupWholeScreen2DVBO(wholescreen_fliptextured);
	R_DrawVarrays(GL_QUADS, 0, 4);
	R_KillVArrays();

	qglPopMatrix();
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	GLSTATE_ENABLE_BLEND
	qglEnable(GL_DEPTH_TEST);

	glUseProgramObjectARB(0);

	return;
}

/*
=================
R_GenPPFrameBuffer
=================
*/

void R_GenPPFrameBuffer(void)
{
	// Create color buffer
	//qglDeleteFramebuffersEXT(1, &pp_FBO);
	qglGenFramebuffersEXT(1, &pp_FBO);
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, pp_FBO);

	// Create depth buffer
	//qglDeleteRenderbuffersEXT(1, &ppDepth_FBO);
	qglGenRenderbuffersEXT(1, &ppDepth_FBO);
	qglBindRenderbufferEXT(GL_RENDERBUFFER, ppDepth_FBO);
	qglRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, vid.width, vid.height);
	qglBindRenderbufferEXT(GL_RENDERBUFFER, 0);

	// Create color texture
	qglGenTextures(1, &pp_FBO);
	qglBindTexture(GL_TEXTURE_2D, r_colorbuffer->texnum);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, vid.width, vid.height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Create depth texture
	qglGenTextures(1, &ppDepth_FBO);
	qglBindTexture(GL_TEXTURE_2D, r_depthbuffer->texnum);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, vid.width, vid.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	// Attach textures to buffers
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r_colorbuffer->texnum, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_depthbuffer->texnum, 0);

	if (qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("framebuffer failed!\n");
}

/*
=================
R_FB_InitTextures
=================
*/

void R_FB_InitTextures( void )
{
	byte	*data;
	int		size;
	GLenum FBOstatus;

	//init the various FBO textures
	size = vid.width * vid.height * 4;
	if (size < 16 * 16 * 4) // nullpic min size
		size = 16 * 16 * 4;
	data = malloc (size);

	r_framebuffer = R_Postprocess_AllocFBOTexture ("***r_framebuffer***", vid.width, vid.height, &distort_FBO);
	
	memset (data, 255, size);
	r_colorbuffer = GL_LoadPic ("***r_colorbuffer***", data, vid.width, vid.height, it_pic, 3);	
	r_depthbuffer = GL_LoadPic ("***r_depthbuffer***", data, vid.width, vid.height, it_pic, 3);	

	//FBO for capturing stencil volumes
	qglBindTexture(GL_TEXTURE_2D, r_colorbuffer->texnum);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);

	qglGenFramebuffersEXT(1, &godray_FBO);
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, godray_FBO);	

	// attach a texture to FBO color attachement point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, r_colorbuffer->texnum, 0);

	// attach a renderbuffer to depth attachment point
	qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, godray_FBO);

	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use secondary FBO\n");

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,vid.width,vid.height); 

	// Initialize frame values.
	// This only makes a difference if the viewport is less than the screen
	// size, like when the netgraph is on-- otherwise it's redundant with 
	// later glClear calls.
	qglClearColor (1, 1, 1, 1);
	qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	qglClearColor (0, 0, 0, 1.0f);

	// back to previous screen coordinates
	R_SetupViewport ();

	//R_GenPPFrameBuffer();

	// switch back to window-system-provided framebuffer
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	//init the distortion textures
	r_distortwave = GL_FindImage("gfx/distortwave.jpg", it_pic);
	if (!r_distortwave) 
		r_distortwave = GL_LoadPic ("***r_distortwave***", data, 16, 16, it_pic, 32);
	r_droplets = GL_FindImage("gfx/droplets.jpg", it_pic);
	if (!r_droplets) 
		r_droplets = GL_LoadPic ("***r_droplets***", data, 16, 16, it_pic, 32);

	//init gore/blood textures
	r_blooddroplets = GL_FindImage("gfx/blooddrops.jpg", it_pic);
	if (!r_blooddroplets) 
		r_blooddroplets = GL_LoadPic ("***r_blooddroplets***", data, 16, 16, it_pic, 32);
	r_blooddroplets_nm = GL_FindImage("gfx/blooddrops_nm.jpg", it_pic);
	if (!r_blooddroplets_nm) 
		r_blooddroplets_nm = GL_LoadPic ("***r_blooddroplets_nm***", data, 16, 16, it_pic, 32);
	
	free (data);
}

void R_DrawBloodEffect (void)
{	
	image_t *gl = NULL;
	
	gl = R_RegisterPic ("blood_ring");
	
	if (!gl)
		return;

	GLSTATE_ENABLE_BLEND

	GL_MBind (0, gl->texnum);
		
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho(0, 1, 1, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();
	
	GL_SetupWholeScreen2DVBO (wholescreen_textured);
	R_DrawVarrays (GL_QUADS, 0, 4);
	R_KillVArrays ();

	GLSTATE_DISABLE_BLEND
}

extern void PART_RenderSunFlare(image_t * tex, float offset, float size, float depth, float r,
                      float g, float b, float alpha);
void PART_GetSunFlareBounds (float offset, float radius, vec2_t out_mins, vec2_t out_maxs);
extern void R_DrawVegetationCasters( qboolean forShadows );
extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern void SM_RecursiveWorldNode (mnode_t *node, int clipflags);
void R_DrawShadowMapWorld (qboolean forEnt, vec3_t origin)
{
	int i;

	if (!r_drawworld->integer)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;
	
	{
		qglEnableClientState (GL_VERTEX_ARRAY);
		
		SM_RecursiveWorldNode (r_worldmodel->nodes, 15);
		
		// Flush the VBO accumulator now because each brush model will mess
		// with the modelview matrix when rendering its own surfaces.
		BSP_FlushVBOAccum ();

		//draw brush models(not for ent shadow, for now)
		for (i=0 ; i<r_newrefdef.num_entities ; i++)
		{
			currententity = &r_newrefdef.entities[i];
			if (currententity->flags & RF_TRANSLUCENT)
				continue;	// transluscent

			currentmodel = currententity->model;

			if (!currentmodel)
			{
				continue;
			}
			if( currentmodel->type == mod_brush)
				BSP_DrawTexturelessBrushModel (currententity);
			else
				continue;
		}
		
		R_KillVArrays();
	}
}

void R_GLSLGodRays(void)
{
	float size, screenaspect;
	vec2_t fxScreenPos;
	vec3_t origin = {0, 0, 0};

	if(!r_godrays->integer || !r_drawsun->integer)
		return;

	 if (!draw_sun || sun_alpha <= 0)
		return;

	//switch to fbo
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, godray_FBO); //need color buffer

	qglDisable( GL_DEPTH_TEST );
	qglDepthMask (1);

	qglClear ( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	//render sun object center
	qglMatrixMode(GL_PROJECTION);
    qglPushMatrix();
    qglLoadIdentity();
    qglOrtho(0, r_newrefdef.width, r_newrefdef.height, 0, -99999, 99999);
    qglMatrixMode(GL_MODELVIEW);
    qglPushMatrix();
    qglLoadIdentity();

	size = r_newrefdef.width * sun_size/4.0;
    PART_RenderSunFlare(sun2_object, 0, size, -99999, 1.0, 1.0, 1.0, 0.5);
    
	qglPopMatrix();
    qglMatrixMode(GL_PROJECTION);
    qglPopMatrix();
	qglLoadIdentity();

	//render occuders simple, textureless
	//need to set up proper matrix for this view!
	screenaspect = (float)r_newrefdef.width/(float)r_newrefdef.height;    

	if(r_newrefdef.fov_y < 90)
		MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  128000);
	else
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4 * 74 / r_newrefdef.fov_y, 15000); 

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	qglRotatef (-90, 1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

    qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
	qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
	qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
	qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

	qglCullFace(GL_FRONT);
	if (gl_cull->integer)
		qglEnable(GL_CULL_FACE);

	R_DrawShadowMapWorld(false, origin); //could tweak this to only draw surfaces that are in the sun?
	R_DrawVegetationCasters(false);
	
	qglMatrixMode(GL_PROJECTION);
    qglPushMatrix();
    qglLoadIdentity();
    qglOrtho(0, 1, 1, 0, -99999, 99999);
    qglMatrixMode(GL_MODELVIEW);
    qglPushMatrix();
    qglLoadIdentity();	

	//glsl the fbo with effect

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 

	glUseProgramObjectARB( g_godraysprogramObj );

	GL_MBind (0, r_colorbuffer->texnum);

	glUniform1iARB( g_location_sunTex, 0);

	R_TransformVectorToScreen(&r_newrefdef, sun_origin, fxScreenPos);

	fxScreenPos[0] /= viddef.width; 
	fxScreenPos[1] /= viddef.height;

	glUniform2fARB( g_location_lightPositionOnScreen, fxScreenPos[0], fxScreenPos[1]);

	glUniform1fARB( g_location_godrayScreenAspect, screenaspect);
    glUniform1fARB( g_location_sunRadius, sun_size*r_godray_intensity->value);
    
	//render quad 
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE);
	
	GL_SetupWholeScreen2DVBO (wholescreen_fliptextured);
	R_DrawVarrays (GL_QUADS, 0, 4);
	R_KillVArrays ();

	GLSTATE_DISABLE_BLEND
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgramObjectARB( 0 );
	
	qglPopMatrix();
    qglMatrixMode(GL_PROJECTION);
    qglPopMatrix();
    qglMatrixMode(GL_MODELVIEW);	
}

void R_GLSLPostProcess(void)
{
	//R_GLSLDOF();
	R_GLSLGodRays();
	R_GLSLWaterDroplets();
	R_GLSLDistortion();
	GL_SelectTexture (0); // FIXME: make unnecessary
}
