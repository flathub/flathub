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
// r_misc.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

//For screenshots
#if defined HAVE_JPEG_JPEGLIB_H
#include "jpeg/jpeglib.h"
#else
#include "jpeglib.h"
#endif

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles
image_t		*r_smoketexture;	// for smoke, etc...
image_t		*r_fireballtexture;
image_t		*r_firestreamtexture;
image_t		*r_firestream3ptexture;
image_t		*r_explosiontexture;
image_t		*r_explosion1texture;
image_t		*r_explosion2texture;
image_t		*r_explosion3texture;
image_t		*r_explosion4texture;
image_t		*r_explosion5texture;
image_t		*r_explosion6texture;
image_t		*r_explosion7texture;
image_t		*r_bloodtexture;
image_t		*r_blood2texture;
image_t		*r_blood3texture;
image_t		*r_blood4texture;
image_t		*r_blood5texture;
image_t		*r_pufftexture;
image_t		*r_bflashtexture;
image_t		*r_cflashtexture;
image_t		*r_fflashtexture;
image_t		*r_pflashtexture;
image_t		*r_leaderfieldtexture;
image_t		*r_deathfieldtexture;
image_t		*r_deathfieldtexture2;
image_t		*r_shelltexture;
image_t		*r_shelltexture2;
image_t		*r_shellnormal;
image_t		*r_shellnormal2;
image_t		*r_hittexture;
image_t		*r_bubbletexture;
image_t		*r_reflecttexture;
image_t		*r_shottexture;
image_t		*r_sayicontexture;
image_t		*r_flaretexture;
image_t		*r_flagtexture;
image_t		*r_logotexture;
image_t		*r_beamstart;
image_t		*r_beamtexture;
image_t		*r_beam2texture;
image_t		*r_beam3texture;
image_t		*r_dis1texture;
image_t		*r_dis2texture;
image_t		*r_dis3texture;
image_t		*r_bullettexture;
image_t		*r_bulletnormal;
image_t		*r_voltagetexture;
image_t		*r_raintexture;
image_t		*r_leaftexture;
image_t		*r_trashtexture;
image_t		*r_splashtexture;
image_t		*r_splash2texture;
image_t		*r_lavasplashtexture;
image_t		*r_around;
image_t		*r_flare;
image_t		*r_flare1;
image_t		*r_mirrorspec;
image_t		*r_distort;
image_t		*sun_object;
image_t		*sun1_object;
image_t		*sun2_object;
image_t		*r_doubledamage;
image_t		*r_invulnerability;

static size_t szr; // just for unused result warnings

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[16][16] =
{
	{0,0,0,0,0,2,6,7,7,5,2,0,0,0,0,0},
    {0,0,0,1,8,20,33,40,39,30,18,7,0,0,0,0},
    {0,0,1,12,35,60,78,87,87,76,56,30,10,0,0,0},
    {0,0,9,37,72,103,125,136,135,122,98,67,31,7,0,0},
    {0,3,25,66,108,142,168,180,179,164,137,102,60,20,2,0},
	{0,9,44,91,137,175,206,220,219,201,170,130,84,38,6,0},
    {0,15,58,109,157,200,233,249,248,229,194,150,101,51,11,0},
    {1,18,66,118,167,212,245,254,254,242,206,159,110,57,14,0},
    {1,18,64,116,165,209,243,254,254,239,203,158,108,56,13,0},
    {0,13,54,104,152,193,225,242,240,221,187,144,97,47,9,0},
    {0,7,39,84,128,165,194,209,207,190,160,122,77,32,5,0},
    {0,2,20,57,97,130,154,167,166,152,125,91,51,15,1,0},
    {0,0,6,27,60,89,111,121,120,108,85,55,23,5,0,0},
    {0,0,0,7,24,46,63,72,72,61,43,20,6,0,0,0},
    {0,0,0,0,4,11,20,26,26,20,10,3,0,0,0,0},
    {0,0,0,0,0,0,2,3,3,2,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[16][16][4];
	char flares[MAX_QPATH];

	//
	// particle texture
	//
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y];
		}
	}

	r_particletexture = R_RegisterParticlePic("particle");
	if (!r_particletexture) {
		r_particletexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_smoketexture = R_RegisterParticlePic("smoke");
	if (!r_smoketexture) {
		r_smoketexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_fireballtexture = R_RegisterParticlePic("fireball");
	if (!r_fireballtexture) {
		r_fireballtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_firestreamtexture = R_RegisterParticlePic("firestream");
	if (!r_firestreamtexture) {
		r_firestreamtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_firestream3ptexture = R_RegisterParticlePic("firestream3p");
	if (!r_firestream3ptexture) {
		r_firestream3ptexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosiontexture = R_RegisterParticlePic("explosion");
	if (!r_explosiontexture) {
		r_explosiontexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosiontexture = R_RegisterParticlePic("explosion");
	if (!r_explosiontexture) {
		r_explosiontexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion1texture = R_RegisterParticlePic("r_explod_1");
	if (!r_explosion1texture) {
		r_explosion1texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion2texture = R_RegisterParticlePic("r_explod_2");
	if (!r_explosion2texture) {
		r_explosion2texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion3texture = R_RegisterParticlePic("r_explod_3");
	if (!r_explosion3texture) {
		r_explosion3texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion4texture = R_RegisterParticlePic("r_explod_4");
	if (!r_explosion4texture) {
		r_explosion4texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion5texture = R_RegisterParticlePic("r_explod_5");
	if (!r_explosion5texture) {
		r_explosion5texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion6texture = R_RegisterParticlePic("r_explod_6");
	if (!r_explosion6texture) {
		r_explosion6texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_explosion7texture = R_RegisterParticlePic("r_explod_7");
	if (!r_explosion7texture) {
		r_explosion7texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_bloodtexture = R_RegisterParticlePic("blood");
	if (!r_bloodtexture) {
		r_bloodtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_blood2texture = R_RegisterParticlePic("blood2");
	if (!r_blood2texture) {
		r_blood2texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_blood3texture = R_RegisterParticlePic("blood3");
	if (!r_blood3texture) {
		r_blood3texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_blood4texture = R_RegisterParticlePic("blood4");
	if (!r_blood4texture) {
		r_blood4texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_blood5texture = R_RegisterParticlePic("blood5");
	if (!r_blood5texture) {
		r_blood5texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_pufftexture = R_RegisterParticlePic("puff");
	if (!r_pufftexture) {
		r_pufftexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_bflashtexture = R_RegisterParticlePic("bflash");
	if (!r_bflashtexture) {
		r_bflashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_cflashtexture = R_RegisterParticlePic("cflash");
	if (!r_cflashtexture) {
		r_cflashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_fflashtexture = R_RegisterParticlePic("fflash");
	if (!r_fflashtexture) {
		r_fflashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_pflashtexture = R_RegisterParticlePic("pflash");
	if (!r_pflashtexture) {
		r_pflashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_leaderfieldtexture = R_RegisterParticlePic("leaderfield");
	if (!r_leaderfieldtexture) {
		r_leaderfieldtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_deathfieldtexture = R_RegisterParticlePic("deathfield");
	if (!r_deathfieldtexture) {
		r_deathfieldtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_deathfieldtexture2 = R_RegisterParticlePic("deathfield2");
	if (!r_deathfieldtexture2) {
		r_deathfieldtexture2 = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_shelltexture = R_RegisterParticlePic("shell");
	if (!r_shelltexture) {
		r_shelltexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_shellnormal = R_RegisterParticlePic("shell_normal");
	if (!r_shellnormal) {
		r_shellnormal = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_shelltexture2 = R_RegisterParticlePic("shell2");
	if (!r_shelltexture2) {
		r_shelltexture2 = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_shellnormal2 = R_RegisterParticlePic("shell2_normal");
	if (!r_shellnormal2) {
		r_shellnormal2 = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_hittexture = R_RegisterParticlePic("aflash");
	if (!r_hittexture) {
		r_hittexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_bubbletexture = R_RegisterParticlePic("bubble");
	if (!r_bubbletexture) {
		r_bubbletexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_reflecttexture = R_RegisterGfxPic("reflect");
	if (!r_reflecttexture) {
		r_reflecttexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_mirrorspec = R_RegisterGfxPic("mirrorspec");
	if (!r_mirrorspec) {
		r_mirrorspec = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_shottexture = R_RegisterParticlePic("dflash");
	if (!r_shottexture) {
		r_shottexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_sayicontexture = R_RegisterParticlePic("sayicon");
	if (!r_sayicontexture) {
		r_sayicontexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_flaretexture = R_RegisterParticlePic("flare");
	if (!r_flaretexture) {
		r_flaretexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_flagtexture = R_RegisterParticlePic("flag");
	if (!r_flagtexture) {
		r_flagtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_logotexture = R_RegisterParticlePic("logo");
	if (!r_logotexture) {
		r_logotexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_beamstart = R_RegisterParticlePic("beamstart");
	if (!r_beamstart) {
		r_beamstart = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_beamtexture = R_RegisterGfxPic("glightning");
	if (!r_beamtexture) {
		r_beamtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_beam2texture = R_RegisterGfxPic("greenline");
	if (!r_beam2texture) {
		r_beam2texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_beam3texture = R_RegisterGfxPic("electrics3d");
	if (!r_beam3texture) {
		r_beam3texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_bullettexture = R_RegisterParticlePic("bullethole");
	if (!r_bullettexture) {
		r_bullettexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_bulletnormal = R_RegisterParticleNormal("bullethole2");
	if (!r_bulletnormal) {
		r_bulletnormal = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_voltagetexture = R_RegisterParticlePic("voltage");
	if (!r_voltagetexture) {
		r_voltagetexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_raintexture = R_RegisterParticlePic("beam");
	if (!r_raintexture) {
		r_raintexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_leaftexture = R_RegisterParticlePic("leaf");
	if (!r_leaftexture) {
		r_leaftexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_trashtexture = R_RegisterParticlePic("trash");
	if (!r_trashtexture) {
		r_trashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_splashtexture = R_RegisterParticlePic("ripples");
	if (!r_splashtexture) {
		r_splashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_splash2texture = R_RegisterParticlePic("watersplash");
	if (!r_splash2texture) {
		r_splash2texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_lavasplashtexture = R_RegisterParticlePic("lavasplash");
	if (!r_lavasplashtexture) {
		r_lavasplashtexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }

	//to do - move more or all of this to their appropriate places
	r_around = GL_FindImage("gfx/radar/around",it_pic);
	if (!r_around) {
		r_around = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_distort = GL_FindImage("gfx/water/distort1.tga", it_pic);
	if (!r_distort) {
		r_distort = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	sun_object = R_RegisterGfxPic("sun");
	if(!sun_object) {
		sun_object = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
	}
	sun1_object = R_RegisterGfxPic("sun1");
	if(!sun1_object) {
		sun1_object = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
	}
	sun2_object = R_RegisterGfxPic("sun2");
	if(!sun2_object) {
		sun2_object = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
	}
	r_dis1texture = R_RegisterParticlePic("disbeam1");
	if (!r_dis1texture) {
		r_dis1texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_dis2texture = R_RegisterParticlePic("disbeam2");
	if (!r_dis2texture) {
		r_dis2texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	r_dis3texture = R_RegisterParticlePic("disbeam3");
	if (!r_dis3texture) {
		r_dis3texture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }

	r_doubledamage = GL_FindImage("pics/p_quad",it_pic);
	if (!r_doubledamage) {
		r_doubledamage = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }

	r_invulnerability = GL_FindImage("pics/p_invulnerability",it_pic);
	if (!r_invulnerability) {
		r_invulnerability = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0;
			data[y][x][2] = 0;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_LoadPic ("***r_notexture***", (byte *)data, 16, 16, it_wall, 32);

	//will eventually add more flaretypes
	Com_sprintf (flares, sizeof(flares), "gfx/flares/flare0.tga");
	r_flare = GL_FindImage(flares, it_pic);
	if (!r_flare) {
		r_flare = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }

	Com_sprintf (flares, sizeof(flares), "gfx/flares/flare1.tga");
	r_flare1 = GL_FindImage(flares, it_pic);
	if (!r_flare1) {
		r_flare1 = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

/*
==================
GL_ScreenShot_JPEG
==================
*/
void GL_ScreenShot_JPEG(void)
{
	struct		jpeg_compress_struct cinfo;
	struct		jpeg_error_mgr jerr;
	byte			*rgbdata;
	JSAMPROW	s[1];
	FILE			*f;
	char			picname[80], checkname[MAX_OSPATH];
	int			i, offset;
	cvar_t		*blocking_cvars[20];

	int			nblocking = 0;

	// Yes, you can set gl_picmip to 40. No, we don't want you to show anyone.
	if (gl_picmip->integer > 1)
		blocking_cvars[nblocking++] = gl_picmip;
	if (gl_fog->integer < 1)
		blocking_cvars[nblocking++] = gl_fog;
	// TODO: check for other avant-garde video settings here.
	
	if (nblocking > 0)
	{
		Com_Printf ("Screenshots are disabled because of your settings for the following cvar");
		if (nblocking > 1)
			Com_Printf ("s");
		Com_Printf (":\n");
		for (i = 0; i < nblocking; i++)
		{
			Com_Printf ("    %s: %s\n", blocking_cvars[i]->name, blocking_cvars[i]->string);
		}
		return;
	}

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/", FS_Gamedir());
	FS_CreatePath( checkname );

	strcpy(picname,"AlienArena_000.jpg");

	for (i=0 ; i<=999 ; i++)
	{
		picname[11] = i/100     + '0';
		picname[12] = (i/10)%10 + '0';
		picname[13] = i%10      + '0';

		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	}

	if (i == 1000) {
		Com_Printf(PRINT_ALL, "GL_ScreenShot_JPEG: Couldn't create a file (You probably have taken to many screenshots!)\n");
		return;
	}

	/* Open the file for Binary Output */
	f = fopen(checkname, "wb");

	if (!f) {
		Com_Printf(PRINT_ALL, "GL_ScreenShot_JPEG: Couldn't create a file\n");
		return;
	}

	/* Allocate room for a copy of the framebuffer */
	rgbdata = malloc(viddef.width * viddef.height * 3);

	if (!rgbdata) {
		fclose(f);
		return;
	}

	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, viddef.width, viddef.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);

	/* Initialise the JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, f);

	/* Setup JPEG Parameters */
	cinfo.image_width = viddef.width;
	cinfo.image_height = viddef.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);

	if ((gl_screenshot_jpeg_quality->integer >= 101) || (gl_screenshot_jpeg_quality->integer <= 0))
		Cvar_Set("gl_screenshot_jpeg_quality", "85");

	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->integer, TRUE);

	/* Start Compression */
	jpeg_start_compress(&cinfo, true);

	/* Feed Scanline data */
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);

	while (cinfo.next_scanline < cinfo.image_height) {
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	/* Finish Compression */
	jpeg_finish_compress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_compress(&cinfo);

	/* Close File */
	fclose(f);

	/* Free Temp Framebuffer */
	free(rgbdata);

	/* Done! */
	Com_Printf ("Wrote %s\n", picname);
}

/*
==================
GL_ScreenShot_TGA
==================
*/
void GL_ScreenShot_TGA (void)
{
	byte		*buffer;
	char		picname[80];
	char		checkname[MAX_OSPATH];
	int		i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/", FS_Gamedir());
	FS_CreatePath( checkname );

	// find a file name to save it to
	strcpy(picname,"AlienArena_000.tga");

	for (i=0 ; i<=999 ; i++)
	{
		picname[11] = i/100     + '0';
		picname[12] = (i/10)%10 + '0';
		picname[13] = i%10      + '0';
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	}

	if (i==1000)
	{
		Com_Printf ("GL_ScreenShot_TGA: Couldn't create file %s (You probably have taken to many screenshots!)\n", picname);
		return;
	}

	buffer = malloc(viddef.width*viddef.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = viddef.width&255;
	buffer[13] = viddef.width>>8;
	buffer[14] = viddef.height&255;
	buffer[15] = viddef.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, viddef.width, viddef.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

	// swap rgb to bgr
	c = 18+viddef.width*viddef.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	szr = fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
	Com_Printf ("Wrote %s\n", picname);
}

/*
==================
GL_ScreenShot_f
==================
*/
void GL_ScreenShot_f (void)
{
	if (strcmp(gl_screenshot_type->string, "jpeg") == 0)
		GL_ScreenShot_JPEG();
	else if (strcmp(gl_screenshot_type->string, "tga") == 0)
		GL_ScreenShot_TGA();
	else {
		Com_Printf("Invalid screenshot type %s; resetting to jpeg.\n", gl_screenshot_type->string);
		Cvar_Set("gl_screenshot_type", "jpeg");
		GL_ScreenShot_JPEG();
	}
}

/**
 * Search for an extension supported by the GL driver.
 *
 * Note that strstr() on the GL_EXTENSIONS string is not robust
 * because an extension name string can be an initial substring of
 * another extension. Also, with OpenGL Version 3,
 * glGetString(GL_EXTENSIONS) is deprecated. See doc for glGetStringi.
 * 
 */
qboolean GL_QueryExtension( const char *extension_name )
{
	size_t name_len;
	const char *strp;
	const char *endp;

	name_len = strlen( extension_name );
	strp = gl_config.extensions_string;
	endp = strp + strlen(strp);
	
	while ( strp < endp )
	{
		size_t n = strcspn( strp, " " ); // length of substring
		if ( name_len == n && !strncmp(extension_name, strp, n) )
			return true; // exit, found
		strp += n + 1; // next substring 
	}

	return false;
}

/**
 * Print the OpenGL driver's GL_EXTENSIONS string
 *
 * Cannot just Com_Printf because the extensions string length can
 * overflow the print buffer.
 *
 */
void GL_PrintExtensions(void)
{
	const char *strp;
	const char *endp;
	char extension_name[256];

	Com_Printf("GL_EXTENSIONS:\n");
	
	strp = gl_config.extensions_string;
	endp = strp + strlen(strp);
	while ( strp < endp )
	{
		size_t n = strcspn( strp, " " ); // length of substring
		if ( n < sizeof(extension_name) )
		{
			strncpy(extension_name, strp, n);
			extension_name[n] = '\0';
			Com_Printf(" %s\n", extension_name);
		}
		strp += n + 1; // next substring
	}
}

/**
 * Target of "gl_strings" console commmand.
 */
void GL_Strings_f( void )
{
	Com_Printf("GL_VENDOR: %s\n", gl_config.vendor_string);
	Com_Printf("GL_RENDERER: %s\n", gl_config.renderer_string);
	Com_Printf("GL_VERSION: %s\n", gl_config.version_string);
	GL_PrintExtensions();
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	GLint MultiSampleEnabled; //1 if MSAA is enabled
	qglGetIntegerv(GL_SAMPLE_BUFFERS, &MultiSampleEnabled);
	gl_state.msaaEnabled = MultiSampleEnabled != 0;
	
	qglClearColor (0, 0, 0, 1.0f);
	qglCullFace(GL_FRONT);
	GL_EnableTexture (0, true);

	qglEnable(GL_ALPHA_TEST);
	gl_state.alpha_test = true;
	qglAlphaFunc(GL_GREATER, 0.666);

	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	gl_state.blend = false;

	qglColor4f (1,1,1,1);

	GL_SelectTexture (0);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	GL_UpdateSwapInterval();

	/* set memory alignment for glReadPixels() and glDrawPixels().
	 * default is 4, screenshots with 1366 vid width fail with that.
	 * setting to 1 should work with any custom vid width.
	 * also affects glTexImage2D() and other commands, so might cause problems.
	 */
	qglPixelStorei(GL_PACK_ALIGNMENT, 1);
	qglPixelStorei(GL_UNPACK_ALIGNMENT, 1);

}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled )
		{
#if defined WIN32_VARIANT
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}

