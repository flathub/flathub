/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// r_bloom.c: 2D lighting post process effect

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

/*
==============================================================================

						LIGHT BLOOMS

==============================================================================
*/

static int		BLOOM_WIDTH, BLOOM_HEIGHT, BLOOM_NUM_MIPMAPS;

cvar_t		*r_bloom_alpha;
cvar_t		*r_bloom_diamond_size;
cvar_t		*r_bloom_intensity;
cvar_t		*r_bloom_darken;
cvar_t		*r_bloom_sample_scaledown;
cvar_t		*r_bloom_autoexposure;

image_t	*r_bloomscratchtexture;
image_t	*r_bloomscratchtexture2;
image_t	*r_bloomeffecttexture;
image_t	*r_bloomcoloraveragingtexture;
static GLuint bloomscratchFBO, bloomscratch2FBO, bloomeffectFBO, bloomcoloraveragingFBO;
static qboolean coloraverage_valid;
static GLuint bloom_fullsize_downsampling_rbo_FBO;
static GLuint bloom_fullsize_downsampling_RBO;
static GLuint bloom_coloraveraging_download_PBO;

static int		screen_texture_width, screen_texture_height;

//current refdef size:
static int	curView_x;
static int	curView_y;
static int	curView_width;
static int	curView_height;

/*
=================
R_Bloom_AllocRBO

Create a 24-bit RBO with specified size and attach it to an FBO
=================
*/
static void R_Bloom_AllocRBO (int width, int height, GLuint *RBO, GLuint *FBO)
{
	// create the RBO
	qglGenRenderbuffersEXT (1, RBO);
	qglBindRenderbufferEXT (GL_RENDERBUFFER_EXT, *RBO);
    qglRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_RGB, width, height);
    qglBindRenderbufferEXT (GL_RENDERBUFFER_EXT, 0);
    
    // create up the FBO
	qglGenFramebuffersEXT (1, FBO);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, *FBO);
	
	// bind the RBO to it
	qglFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, *RBO);
	
	//clean up
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}



/*
=================
R_Bloom_InitEffectTexture
=================
*/
static void R_Bloom_InitEffectTexture (void)
{
	BLOOM_WIDTH = vid.width / r_bloom_sample_scaledown->value;
	BLOOM_HEIGHT = vid.height / r_bloom_sample_scaledown->value;
	// we use mipmapping to calculate a 1-pixel average color for auto-exposure
	BLOOM_NUM_MIPMAPS = floor (Q_log2 (BLOOM_WIDTH > BLOOM_HEIGHT ? BLOOM_WIDTH : BLOOM_HEIGHT));

	r_bloomeffecttexture = R_Postprocess_AllocFBOTexture ("***r_bloomeffecttexture***", BLOOM_WIDTH, BLOOM_HEIGHT, &bloomeffectFBO);

	r_bloomcoloraveragingtexture = R_Postprocess_AllocFBOTexture ("***r_bloomcoloraveragingtexture***", BLOOM_WIDTH, BLOOM_HEIGHT, &bloomcoloraveragingFBO);
	GL_Bind (r_bloomcoloraveragingtexture->texnum);
	coloraverage_valid = false;
	qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1000);
	GL_Bind (0);

	qglGenBuffersARB (1, &bloom_coloraveraging_download_PBO);
	qglBindBufferARB (GL_PIXEL_PACK_BUFFER_ARB, bloom_coloraveraging_download_PBO);
	qglBufferDataARB (GL_PIXEL_PACK_BUFFER_ARB, 3, NULL, GL_STREAM_READ);
	qglBindBufferARB (GL_PIXEL_PACK_BUFFER_ARB, 0);
}

/*
=================
R_Bloom_InitTextures
=================
*/
void checkFBOExtensions (void);
static void R_Bloom_InitTextures (void)
{
	if (!gl_state.fbo || !gl_state.hasFBOblit)
	{
		Com_Printf ("FBO Failed, disabling bloom.\n");
		Cvar_SetValue ("r_bloom", 0);
		return;
	}
	
	GL_SelectTexture (0);
	
	qglGetError ();
	
	//find closer power of 2 to screen size
	for (screen_texture_width = 1;screen_texture_width < vid.width;screen_texture_width *= 2);
	for (screen_texture_height = 1;screen_texture_height < vid.height;screen_texture_height *= 2);

	//validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture();

	//init the "scratch" texture
	r_bloomscratchtexture = R_Postprocess_AllocFBOTexture ("***r_bloomscratchtexture***", BLOOM_WIDTH, BLOOM_HEIGHT, &bloomscratchFBO);
	r_bloomscratchtexture2 = R_Postprocess_AllocFBOTexture ("***r_bloomscratchtexture2***", BLOOM_WIDTH, BLOOM_HEIGHT, &bloomscratch2FBO);
	
	//init the screen-size RBO
	R_Bloom_AllocRBO (vid.width, vid.height, &bloom_fullsize_downsampling_RBO, &bloom_fullsize_downsampling_rbo_FBO);
}

/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{

	r_bloom = Cvar_Get ("r_bloom", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_bloom_alpha = Cvar_Get ("r_bloom_alpha", "0.2", CVAR_ARCHIVE|CVARDOC_FLOAT);
	r_bloom_diamond_size = Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE|CVARDOC_INT);
	r_bloom_intensity = Cvar_Get ("r_bloom_intensity", "0.75", CVAR_ARCHIVE|CVARDOC_FLOAT);
	r_bloom_darken = Cvar_Get ("r_bloom_darken", "8", CVAR_ARCHIVE|CVARDOC_FLOAT);
	r_bloom_sample_scaledown = Cvar_Get ("r_bloom_sample_scaledown", "2", CVAR_ARCHIVE|CVARDOC_FLOAT);
	r_bloom_autoexposure = Cvar_Get ("r_bloom_autoexposure", "1", CVAR_ARCHIVE|CVARDOC_BOOL);

	BLOOM_WIDTH = BLOOM_HEIGHT = 0;
	if( !r_bloom->integer )
		return;

	R_Bloom_InitTextures ();
}


/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect (void)
{
	GL_SelectTexture (0);
	GL_Bind (r_bloomeffecttexture->texnum);
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_ONE, GL_ONE);
	qglColor4f(r_bloom_alpha->value, r_bloom_alpha->value, r_bloom_alpha->value, 1.0f);
	GL_TexEnv(GL_MODULATE);
	
	qglBegin(GL_QUADS);
	qglTexCoord2f(	0,							1.0	);
	qglVertex2f(	curView_x,					curView_y	);
	qglTexCoord2f(	0,							0	);
	qglVertex2f(	curView_x,					curView_y + curView_height	);
	qglTexCoord2f(	1.0,						0	);
	qglVertex2f(	curView_x + curView_width,	curView_y + curView_height	);
	qglTexCoord2f(	1.0,						1.0	);
	qglVertex2f(	curView_x + curView_width,	curView_y	);
	qglEnd();

	GLSTATE_DISABLE_BLEND
}


/*
=================
R_Bloom_DoGaussian
=================
*/
static void R_Bloom_DoGaussian (void)
{
	int			i;
	static float intensity;
	int maxscale, exp;
	
	//set up sample size workspace
	qglViewport (0, 0, BLOOM_WIDTH, BLOOM_HEIGHT);
	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -10, 100);
	qglMatrixMode (GL_MODELVIEW);

	GL_SetupWholeScreen2DVBO (wholescreen_fliptextured);

	// auto-exposure-- adjust the light bloom intensity based on how dark or
	// light the scene is. We use a PBO and give it a whole frame to compute the
	// color average and transfer it into main memory in the background. The
	// extra delay from using last frame's average is harmless.
	if (r_bloom_autoexposure->integer)
	{
		const float brightness_bias = 0.8f, brightness_reverse_bias = 0.2f;
		float brightness = 1.0f; // reasonable default

		qglBindBufferARB (GL_PIXEL_PACK_BUFFER_ARB, bloom_coloraveraging_download_PBO);

		// get last frame's color average
		if (coloraverage_valid) // will only be false just after a vid_restart
		{
			unsigned char *pixel;
			pixel = (unsigned char *)qglMapBufferARB (GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
			if (pixel != NULL)
			{
				brightness = (float)(pixel[0] + pixel[1] + pixel[2]) / (256.0f * 3.0f);
				qglUnmapBufferARB (GL_PIXEL_PACK_BUFFER_ARB);
			}
		}

		// start computing this frame's color average so we can use it next frame
		qglBindFramebufferEXT (GL_READ_FRAMEBUFFER_EXT, bloomeffectFBO);
		qglBindFramebufferEXT (GL_DRAW_FRAMEBUFFER_EXT, bloomcoloraveragingFBO);
		qglBlitFramebufferEXT (0, 0, BLOOM_WIDTH, BLOOM_HEIGHT,
		                       0, 0, BLOOM_WIDTH, BLOOM_HEIGHT,
							   GL_COLOR_BUFFER_BIT, GL_NEAREST);
		GL_MBind (0, r_bloomcoloraveragingtexture->texnum);
		qglGenerateMipmapEXT (GL_TEXTURE_2D);
		qglGetTexImage (GL_TEXTURE_2D, BLOOM_NUM_MIPMAPS, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		coloraverage_valid = true; // any time after this, we could read from the rbo
		qglBindBufferARB (GL_PIXEL_PACK_BUFFER_ARB, 0);

		// Bias upward so that the brighness can occasionally go *above* 1.0
		// in bright areas. That helps kill the bloom completely in bright
		// scenes while leaving some in dark dark scenes.
		brightness += brightness_bias;
		// since the bias will tend to reduce the bloom even in dark
		// scenes, we re-exaggerate the darkness with exponentiation.
		brightness = powf (brightness, 4.0f);
		brightness -= brightness_reverse_bias;

		//apply the color scaling
		GL_MBind (0, r_bloomeffecttexture->texnum);
		qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratchFBO);
		glUseProgramObjectARB (g_colorscaleprogramObj);
		glUniform1iARB (colorscale_uniforms.source, 0);
		glUniform3fARB (colorscale_uniforms.scale, 1.0f/brightness, 1.0f/brightness, 1.0f/brightness);
		R_DrawVarrays (GL_QUADS, 0, 4);
		qglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, BLOOM_WIDTH, BLOOM_HEIGHT);
	}

	GL_MBind (0, r_bloomeffecttexture->texnum);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratchFBO);

	//darkening passes
	if (r_bloom_darken->value)
	{
		exp = r_bloom_darken->value + 1.0f;
		glUseProgramObjectARB (g_colorexpprogramObj);
		glUniform1iARB (colorexp_uniforms.source, 0);
		glUniform4fARB (colorexp_uniforms.exponent, exp, exp, exp, 1.0f);
		R_DrawVarrays (GL_QUADS, 0, 4);
		qglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, BLOOM_WIDTH, BLOOM_HEIGHT);
	}

	glUseProgramObjectARB (g_kawaseprogramObj);
	glUniform1iARB (kawase_uniforms.source, 0);
	GLSTATE_DISABLE_BLEND

	// compute bloom effect scale and round to nearest odd integer
	maxscale = r_bloom_diamond_size->integer * BLOOM_HEIGHT / 384;
	if (maxscale % 2 == 0)
		maxscale++;
	if (maxscale < 3)
		maxscale = 3;

	// Apply Kawase filters of increasing size until the desired filter size
	// is reached. i needs to stay odd, so it must be incremented by an even
	// number each time. Choosing a higher increment value reduces the quality
	// of the blur but improves performance.
	for (i = 3; i <= maxscale; i += 2)
	{
		int j;
		image_t *swap_image;
		GLuint swap_fbo;
		float scale = (float)i / 2.0f - 1.0f;

		// Increasing the repetitions here increases the strength of the blur
		// but hurts performance.
		for (j = 0; j < 2; j++)
		{
			GL_MBind (0, r_bloomscratchtexture->texnum);
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratch2FBO);
			glUniform2fARB (kawase_uniforms.scale, scale / BLOOM_WIDTH, scale / BLOOM_HEIGHT);
			R_DrawVarrays (GL_QUADS, 0, 4);

			swap_image = r_bloomscratchtexture2;
			r_bloomscratchtexture2 = r_bloomscratchtexture;
			r_bloomscratchtexture = swap_image;
			swap_fbo = bloomscratch2FBO;
			bloomscratch2FBO = bloomscratchFBO;
			bloomscratchFBO = swap_fbo;
		}
	}

	glUseProgramObjectARB (g_colorscaleprogramObj);
	glUniform1iARB (colorscale_uniforms.source, 0);
	intensity = 6.0 * r_bloom_intensity->value;
	glUniform3fARB (colorscale_uniforms.scale, intensity, intensity, intensity);
	
	GL_BlendFunction (GL_ONE, GL_ONE);
	GLSTATE_ENABLE_BLEND
	
	GL_MBind (0, r_bloomscratchtexture->texnum);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomeffectFBO);
	R_DrawVarrays (GL_QUADS, 0, 4);
	
	glUseProgramObjectARB (0);
	R_KillVArrays ();
	
	//restore full screen workspace
	qglViewport( 0, 0, viddef.width, viddef.height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, BLOOM_WIDTH, BLOOM_HEIGHT);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

/*
=================
R_Bloom_FullsizeRBOUpdate

This updates the full size RBO from the screen. The whole thing is guaranteed
to be updated 60 times a second, but it does it a 1/4 of the screen at a time
if the framerate is high enough. It does it in horizontal slices instead of
quadrants because that way the GPU doesn't have to skip over part of each row
of pixels. Tearing isn't an issue because it'll just be blurred to mush
anyway.
=================
*/
static void R_Bloom_FullsizeRBOUpdate (void)
{
	static int	cur_section = 0;
	static int	last_time = 0;
	int			i, num_sections, cur_time;
	int			y;
	
	cur_time = Sys_Milliseconds();
	num_sections = (cur_time-last_time+2)/4;
	if (num_sections > 4) num_sections = 4;
	if (num_sections == 0) return;
	
	for (i = 0; i < num_sections; i++)
	{
		y = cur_section*(viddef.height/4);
		qglBlitFramebufferEXT(0, y, viddef.width, y+viddef.height/4, 0, y, viddef.width, y+viddef.height/4,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		cur_section = (cur_section + 1) % 4;
	}
	last_time = cur_time;
}

/*
=================
R_Bloom_DownsampleView

Creates a downscaled, blurred version of the screen, leaving it in the
"scratch" and "effect" textures (identical in both.) This function is name is
a bit confusing, because "downsampling" means two things here:
 1) Creating a scaled-down version of an image
 2) Converting a multisampled image to a non-multisampled image the same size,
	which is necessary if MSAA is enabled in the graphics driver settings
The function name uses meaning 1.
=================
*/
static void R_Bloom_DownsampleView (void)
{
	GLSTATE_DISABLE_BLEND
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	
	GL_SelectTexture (0);
	
	if (gl_state.msaaEnabled)
	{
		// If MSAA is enabled, the FBO blitting needs an extra step.
		// Copy onto full-screen sized RBO first, to go from the multisample
		// format of the screen to a non-multisampled format.
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloom_fullsize_downsampling_rbo_FBO);
		R_Bloom_FullsizeRBOUpdate ();
		// Set the downsampled RBO as the read framebuffer, then run the rest
		// of the code as normal.
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, bloom_fullsize_downsampling_rbo_FBO);
	}

	// downsample
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomeffectFBO);
	qglBlitFramebufferEXT(0, 0, viddef.width, viddef.height, 0, 0, BLOOM_WIDTH, BLOOM_HEIGHT,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
	
	// Blit the finished downsampled texture onto a second FBO. We end up with
	// with two copies, which DoGaussian will take advantage of.
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomscratchFBO);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, bloomeffectFBO);
	qglBlitFramebufferEXT(0, 0, BLOOM_WIDTH, BLOOM_HEIGHT, 0, 0, BLOOM_WIDTH, BLOOM_HEIGHT,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
}

/*
=================
R_BloomBlend
=================
*/
void R_BloomBlend (refdef_t *fd)
{

	if( !(fd->rdflags & RDF_BLOOM) || !r_bloom->integer )
		return;

	if( !BLOOM_WIDTH )
		R_Bloom_InitTextures();
	
	// previous function can set this if there's no FBO
	if (!r_bloom->integer)
		return;

	if (vid.width < BLOOM_WIDTH || vid.height < BLOOM_HEIGHT)
		return;
	
	GL_SelectTexture (0);

	//set up full screen workspace
	qglViewport( 0, 0, viddef.width, viddef.height );
	qglDisable( GL_DEPTH_TEST );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	qglDisable(GL_CULL_FACE);

	GLSTATE_DISABLE_BLEND
	GL_EnableTexture (0, true);

	qglColor4f( 1, 1, 1, 1 );

	//set up current sizes
	// TODO: get rid of these nasty globals
	curView_x = fd->x;
	curView_y = fd->y;
	curView_width = fd->width;
	curView_height = fd->height;

	//create the bloom image
	R_Bloom_DownsampleView();
	R_Bloom_DoGaussian();

	R_Bloom_DrawEffect();

	qglColor3f (1,1,1);
	GLSTATE_DISABLE_BLEND
	GL_EnableTexture (0, true);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

