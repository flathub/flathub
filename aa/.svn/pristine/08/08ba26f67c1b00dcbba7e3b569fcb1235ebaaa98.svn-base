/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2014 COR Entertainment, LLC.

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

// r_draw.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

extern cvar_t *con_font;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
}

/*
=============
R_RegisterPic
=============
*/
static image_t *R_RegisterPicOfType (	const char *name, const char *dirname,
										imagetype_t type )
{
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
		Com_sprintf (fullname, sizeof(fullname), "%s/%s.pcx", dirname, name);
	else
		strcpy (fullname, &name[1]);
	
	return GL_FindImage (fullname, type);
}

image_t	*R_RegisterPic (const char *name)
{
	return R_RegisterPicOfType (name, "pics", it_pic);
}

image_t	*R_RegisterParticlePic (const char *name)
{
	return R_RegisterPicOfType (name, "particles", it_particle);
}

image_t	*R_RegisterParticleNormal (const char *name)
{
	return R_RegisterPicOfType (name, "particles", it_pic);
}

image_t	*R_RegisterGfxPic (const char *name)
{
	return R_RegisterPicOfType (name, "gfx", it_pic);
}

static image_t	*R_RegisterPlayerIcon (const char *name)
{
	return R_RegisterPicOfType (name, "players", it_pic);
}

/*
=============
Draw_PicExists
=============
*/
qboolean Draw_PicExists (const char *pic)
{
	return R_RegisterPic (pic) != NULL;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, const char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

#define DIV254BY255 (0.9960784313725490196078431372549f)
#define DEG2RAD (0.0174532925f)

/*
=============
Draw_AlphaStretchPic
- Note: If tiling is true, the texture wrapping flags are adjusted to prevent
        gaps from appearing if the texture is tiled with itself or with other
        textures. This adjustment is permanent, although it would be easy to
        change the code to undo it after rendering.
=============
*/
enum draw_tiling_s
{
	draw_without_tiling,
	draw_with_tiling
};
static void Draw_DrawQuad_Callback (void)
{
	R_DrawVarrays(GL_QUADS, 0, 4);
}
static void Draw_AlphaStretchImage (float x, float y, float w, float h, const image_t *gl, float alphaval, enum draw_tiling_s tiling)
{
	rscript_t *rs;
	char shortname[MAX_QPATH];
	float xscale, yscale;
	float cropped_x, cropped_y, cropped_w, cropped_h;
	
	COM_StripExtension ( gl->name, shortname );
	
	//if we draw the red team icon, we are on red team
	if(!strcmp(shortname, "pics/i_team1"))
		r_teamColor = 1;
	else if(!strcmp(shortname, "pics/i_team2"))
		r_teamColor = 2;
	
	rs = gl->script;

	R_InitQuadVarrays();
	
	GL_SelectTexture (0);
	
	if (!rs)
	{
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_ENABLE_BLEND
		GL_TexEnv( GL_MODULATE );
		
		xscale = (float)w/(float)gl->upload_width;
		yscale = (float)h/(float)gl->upload_height;
		
		cropped_x = x + xscale*(float)gl->crop_left;
		cropped_y = y + yscale*(float)gl->crop_top;
	
		cropped_w = xscale*(float)gl->crop_width; 
		cropped_h = yscale*(float)gl->crop_height;
		
		VA_SetElem2(vert_array[0], cropped_x,			cropped_y);
		VA_SetElem2(vert_array[1], cropped_x+cropped_w, cropped_y);
		VA_SetElem2(vert_array[2], cropped_x+cropped_w, cropped_y+cropped_h);
		VA_SetElem2(vert_array[3], cropped_x,			cropped_y+cropped_h);
	
		qglColor4f(1,1,1, alphaval);

		GL_Bind (gl->texnum);
		if (tiling == draw_with_tiling)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}
		VA_SetElem2(tex_array[0], gl->crop_sl, gl->crop_tl);
		VA_SetElem2(tex_array[1], gl->crop_sh, gl->crop_tl);
		VA_SetElem2(tex_array[2], gl->crop_sh, gl->crop_th);
		VA_SetElem2(tex_array[3], gl->crop_sl, gl->crop_th);

		R_DrawVarrays (GL_QUADS, 0, 4);
		GLSTATE_DISABLE_BLEND
		GLSTATE_ENABLE_ALPHATEST
		R_KillVArrays();
	}
	else
	{
		VA_SetElem2(vert_array[0], x,	y);
		VA_SetElem2(vert_array[1], x+w, y);
		VA_SetElem2(vert_array[2], x+w, y+h);
		VA_SetElem2(vert_array[3], x,	y+h);
		
		// We don't use sl, sh, tl, and th here because that opens up a whole
		// can of worms of the texcoords for each RScript stage being
		// different. As a result, the current policy is just don't draw any
		// textures of type it_particle using RScripts.
		VA_SetElem2(tex_array[0], 0, 0);
		VA_SetElem2(tex_array[1], 1, 0);
		VA_SetElem2(tex_array[2], 1, 1);
		VA_SetElem2(tex_array[3], 0, 1);
	
		RS_ReadyScript(rs);

		// FIXME: HACK to work around the other compatibility HACK in RS_Draw
		GL_BlendFunction (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		RS_Draw (rs, NULL, 0, vec3_origin, vec3_origin, false, rs_lightmap_off, false, false, Draw_DrawQuad_Callback);

		qglColor4f(1,1,1,1);
		GLSTATE_ENABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		R_KillVArrays();
	}
}

static void Draw_AlphaAngledStretchImage (float x, float y, float w, float h, const image_t *gl, float alphaval, qboolean isNum)
{
	char shortname[MAX_QPATH];
	float xscale, yscale;
	float cropped_x, cropped_y, cropped_w, cropped_h;
	float topRStretch = 0.0, botRStretch = 0.0, topLStretch = 0.0, botLStretch = 0.0;
	
	COM_StripExtension ( gl->name, shortname );

	R_InitQuadVarrays();
	
	GL_SelectTexture (0);
	
	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_ENABLE_BLEND
	GL_TexEnv( GL_MODULATE );
		
	xscale = (float)w/(float)gl->upload_width;
	yscale = (float)h/(float)gl->upload_height;
		
	cropped_x = x + xscale*(float)gl->crop_left;
	cropped_y = y + yscale*(float)gl->crop_top;
	
	cropped_w = xscale*(float)gl->crop_width; 
	cropped_h = yscale*(float)gl->crop_height;

	//bottom left quadrant
	if(cropped_x < r_newrefdef.width/2.0 && cropped_y > r_newrefdef.height/2.0)
	{
		topRStretch = cropped_w/15.0;
		botRStretch = cropped_w/10.0;

		topLStretch = botLStretch = 0.0;

		if(isNum)
		{
			//adjust position 
			cropped_y += ((r_newrefdef.height/10.0)*(r_newrefdef.width/(x - r_newrefdef.width)) + (r_newrefdef.height/11.0));
		}
	}

	//top right quadrant
	if(cropped_x > r_newrefdef.width/2.0 && cropped_y < r_newrefdef.height/2.0)
	{
		topLStretch = -cropped_w/10.0;
		botLStretch = -cropped_w/15.0;

		topRStretch = botRStretch = 0.0;

		if(isNum)
		{
			//adjust position 
			cropped_y -= ((r_newrefdef.height/10.0)*((x - r_newrefdef.width)/r_newrefdef.width));
		}
	}

	//bottom right quadrant
	if(cropped_x > r_newrefdef.width/1.5 && cropped_y > r_newrefdef.height/2.0)
	{
		topLStretch = cropped_w/15.0;
		botLStretch = cropped_w/10.0;

		topRStretch = botRStretch = 0.0;

		if(isNum)
		{
			//adjust position 
			cropped_y += ((r_newrefdef.height/10.0)*((x - r_newrefdef.width)/r_newrefdef.width));
		}
	}
		
	VA_SetElem2(vert_array[0], cropped_x,			cropped_y - topLStretch);
	VA_SetElem2(vert_array[1], cropped_x+cropped_w, cropped_y - topRStretch);
	VA_SetElem2(vert_array[2], cropped_x+cropped_w, cropped_y+cropped_h - botRStretch);
	VA_SetElem2(vert_array[3], cropped_x,			cropped_y+cropped_h - botLStretch);
	
	qglColor4f(1,1,1, alphaval);

	//set color of hud by team
	if(r_teamColor == 1) {
		if(!strcmp(shortname, "pics/i_health") || !strcmp(shortname, "pics/i_ammo"))
			qglColor4f(.7, .2, .2, alphaval);
	}
	else if(r_teamColor == 2) {
		if(!strcmp(shortname, "pics/i_health") || !strcmp(shortname, "pics/i_ammo"))
			qglColor4f(.1, .4, .6, alphaval);
	}

	GL_Bind (gl->texnum);
	
	VA_SetElem2(tex_array[0], gl->crop_sl, gl->crop_tl);
	VA_SetElem2(tex_array[1], gl->crop_sh, gl->crop_tl);
	VA_SetElem2(tex_array[2], gl->crop_sh, gl->crop_th);
	VA_SetElem2(tex_array[3], gl->crop_sl, gl->crop_th);

	R_DrawVarrays (GL_QUADS, 0, 4);
	GLSTATE_DISABLE_BLEND
	GLSTATE_ENABLE_ALPHATEST
	R_KillVArrays();
}

void Draw_AlphaStretchTilingPic (float x, float y, float w, float h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}
	
	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_with_tiling);
}

/*
=============
Draw_AlphaStretchPic
=============
*/
void Draw_AlphaStretchPic (float x, float y, float w, float h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}
	
	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_without_tiling);
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (float x, float y, float w, float h, const char *pic)
{
	Draw_AlphaStretchPic (x, y, w, h, pic, DIV254BY255);
}

/*
=============
Draw_ScaledPic
=============
*/
void Draw_ScaledPic (float x, float y, float scale, const char *pic)
{
	image_t *gl;
	float w, h;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}

	w = (float)gl->width*scale;
	h = (float)gl->height*scale;
	
	Draw_AlphaStretchImage (x, y, w, h, gl, DIV254BY255, draw_without_tiling);
}

void Draw_AngledScaledPic (float x, float y, float scale, const char *pic, qboolean isNum)
{
	image_t *gl;
	float w, h;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}

	w = (float)gl->width*scale;
	h = (float)gl->height*scale;
	
	Draw_AlphaAngledStretchImage (x, y, w, h, gl, DIV254BY255, isNum);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (float x, float y, const char *pic)
{
	Draw_ScaledPic (x, y, 1.0, pic);
}

/*
=============
Draw_AlphaStretchPlayerIcon
=============
*/
void Draw_AlphaStretchPlayerIcon (int x, int y, int w, int h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPlayerIcon (pic);
	if (!gl)
	{
		return;
	}

	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_without_tiling);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (float x, float y, float w, float h, const float rgba[])
{
	GL_EnableTexture (0, false);
	GLSTATE_ENABLE_BLEND;
	GLSTATE_DISABLE_ALPHATEST;
	qglColor4fv (rgba);
	
	qglBegin (GL_QUADS);
		qglVertex2f (x,y);
		qglVertex2f (x+w, y);
		qglVertex2f (x+w, y+h);
		qglVertex2f (x, y+h);
	qglEnd ();
	
	GLSTATE_DISABLE_BLEND;
	qglColor3f (1,1,1);
	GL_EnableTexture (0, true);
}
//=============================================================================

/*
** Fills a box of pixels with a single color, and with cut corners at the specified corners.
** Parameter corners: flags RCF_TOPLEFT, RCF_TOPRIGHT, RCF_BOTTOMLEFT, RCF_BOTTOMRIGHT, RCF_ALL
*/
void Draw_Fill_CutCorners (float x, float y, float w, float h, const float rgba[], float radius, int corners)
{
	GL_EnableTexture (0, false);
	GLSTATE_ENABLE_BLEND;
	GLSTATE_DISABLE_ALPHATEST;
	qglColor4fv (rgba);
	
	qglBegin (GL_QUADS);		
		qglVertex2f (x + radius, y);
		qglVertex2f (x + w - radius, y);			
		qglVertex2f (x + w - radius, y + radius);
		qglVertex2f (x + radius, y + radius);

		qglVertex2f (x, y + radius);
		qglVertex2f (x + w, y + radius);			
		qglVertex2f (x + w, y + h - radius);
		qglVertex2f (x, y + h - radius);

		qglVertex2f (x + radius, y + h);
		qglVertex2f (x + w - radius, y + h);			
		qglVertex2f (x + w - radius, y + h - radius);
		qglVertex2f (x + radius, y + h - radius);

		if (!(corners & RCF_TOPLEFT))
		{
			qglVertex2f (x, y);
			qglVertex2f (x + radius, y);			
			qglVertex2f (x + radius, y + radius);
			qglVertex2f (x, y + radius);				
		}
		if (!(corners & RCF_TOPRIGHT)) 
		{
			qglVertex2f (x + w - radius, y);
			qglVertex2f (x + w, y);			
			qglVertex2f (x + w, y + radius);
			qglVertex2f (x + w - radius, y + radius);	
		}
		if (!(corners & RCF_BOTTOMLEFT)) 
		{
			qglVertex2f (x, y + h - radius);
			qglVertex2f (x + radius, y + h - radius);			
			qglVertex2f (x + radius, y + h);
			qglVertex2f (x, y + h);	
		}
		if (!(corners & RCF_BOTTOMRIGHT))
		{
			qglVertex2f (x + w - radius, y + h - radius);
			qglVertex2f (x + w, y + h - radius);			
			qglVertex2f (x + w, y + h);
			qglVertex2f (x + w - radius, y + h);	
		}
	qglEnd ();
	
	qglBegin(GL_TRIANGLES);
	if (corners & RCF_TOPLEFT) 
	{
		qglVertex2f (x + radius, y);
		qglVertex2f (x + radius, y + radius);
		qglVertex2f (x, y + radius);
	}

	if (corners & RCF_TOPRIGHT) 
	{
		qglVertex2f (x + w - radius, y);
		qglVertex2f (x + w, y + radius);
		qglVertex2f (x + w - radius, y + radius);
	}

	if (corners & RCF_BOTTOMLEFT)
	{
		qglVertex2f (x, y + h - radius);
		qglVertex2f (x + radius, y + h - radius);
		qglVertex2f (x + radius, y + h);
	}

	if (corners & RCF_BOTTOMRIGHT)
	{
		qglVertex2f (x + w - radius, y + h - radius);
		qglVertex2f (x + w, y + h - radius);
		qglVertex2f (x + w - radius, y + h);
	}
	qglEnd();

	GLSTATE_DISABLE_BLEND;
	qglColor3f (1,1,1);
	GL_EnableTexture (0, true);
}

/*
================
RGBA - This really should be a macro, but MSVC doesn't support C99.
*/

float *RGBA (float r, float g, float b, float a)
{
	static float ret[4];
	
	ret[0] = r;
	ret[1] = g;
	ret[2] = b;
	ret[3] = a;
	
	return ret;
}
