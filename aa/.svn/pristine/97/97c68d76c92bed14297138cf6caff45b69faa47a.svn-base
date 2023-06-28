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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// for jpeglib
#define HAVE_PROTOTYPES

#include "r_local.h"

#if defined HAVE_JPEG_JPEGLIB_H
#include "jpeg/jpeglib.h"
#else
#include "jpeglib.h"
#endif

static char deptex_names[deptex_num][32];

image_t		gltextures[MAX_GLTEXTURES];
image_t		*r_mirrortexture;
int			numgltextures;

extern cvar_t	*cl_hudimage1; //custom huds
extern cvar_t	*cl_hudimage2;
extern cvar_t	*cl_hudimage3;
extern viddef_t vid;

unsigned	d_8to24table[256];

qboolean GL_Upload8 (byte *data, int width, int height, int picmip, qboolean filter);
qboolean GL_Upload32 (byte *data, int width, int height, int picmip, qboolean filter, qboolean modulate, qboolean force_standard_mipmap);

int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int		gl_filter_max = GL_LINEAR;

void R_InitImageSubsystem(void)
{
	int			max_aniso;

	if ( GL_QueryExtension("GL_ARB_multitexture") )
	{
		Com_Printf ("...using GL_ARB_multitexture\n" );
		qglMTexCoord2fARB = ( void * ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
		qglMTexCoord3fARB = ( void * ) qwglGetProcAddress( "glMultiTexCoord3fARB" );
		qglMultiTexCoord3fvARB = (void*)qwglGetProcAddress("glMultiTexCoord3fvARB");
		qglActiveTextureARB = ( void * ) qwglGetProcAddress( "glActiveTextureARB" );
		qglClientActiveTextureARB = ( void * ) qwglGetProcAddress( "glClientActiveTextureARB" );
		GL_TEXTURE0 = GL_TEXTURE0_ARB;
		GL_TEXTURE1 = GL_TEXTURE1_ARB;
		GL_TEXTURE2 = GL_TEXTURE2_ARB;
		GL_TEXTURE3 = GL_TEXTURE3_ARB;
		GL_TEXTURE4 = GL_TEXTURE4_ARB;
		GL_TEXTURE5 = GL_TEXTURE5_ARB;
		GL_TEXTURE6 = GL_TEXTURE6_ARB;
		GL_TEXTURE7 = GL_TEXTURE7_ARB;
	}
	else
	{
		Com_Error (ERR_FATAL, "...GL_ARB_multitexture not found\n" );
	}
	
	if ( GL_QueryExtension("GL_ARB_texture_env_combine") )
	{
		Com_Printf( "...using GL_ARB_texture_env_combine\n" );
	}
	else
	{
		Com_Error (ERR_FATAL, "...GL_ARB_texture_env_combine not found\n" );
	}

	if ( GL_QueryExtension("GL_EXT_texture_filter_anisotropic") )
	{
		qglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

		r_ext_max_anisotropy = Cvar_Get("r_ext_max_anisotropy", "1", CVAR_ARCHIVE );
		Cvar_SetValue("r_ext_max_anisotropy", max_aniso);

		r_anisotropic = Cvar_Get("r_anisotropic", "1", CVAR_ARCHIVE);
		if (r_anisotropic->integer >= r_ext_max_anisotropy->integer)
			Cvar_SetValue("r_anisotropic", r_ext_max_anisotropy->integer);
		if (r_anisotropic->integer <= 0)
			Cvar_SetValue("r_anisotropic", 1);
		
		r_alphamasked_anisotropic = Cvar_Get("r_alphamasked_anisotropic", "1", CVAR_ARCHIVE);
		if (r_alphamasked_anisotropic->integer >= r_ext_max_anisotropy->integer)
			Cvar_SetValue("r_alphamasked_anisotropic", r_ext_max_anisotropy->integer);
		if (r_alphamasked_anisotropic->integer <= 0)
			Cvar_SetValue("r_alphamasked_anisotropic", 1);

		if (r_anisotropic->integer == 1 && r_alphamasked_anisotropic->integer == 1)
			Com_Printf("...ignoring GL_EXT_texture_filter_anisotropic\n");
		else
			Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
	}
	else
	{
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");
		r_anisotropic = Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
		r_alphamasked_anisotropic = Cvar_Get("r_alphamasked_anisotropic", "0", CVAR_ARCHIVE);
		r_ext_max_anisotropy = Cvar_Get("r_ext_max_anisotropy", "0", CVAR_ARCHIVE);
	}

	GL_InvalidateTextureState ();
}

void GL_BlendFunction (GLenum sfactor, GLenum dfactor)
{
	if (sfactor != gl_state.bFunc1 || dfactor != gl_state.bFunc2)
	{
		gl_state.bFunc1 = sfactor;
		gl_state.bFunc2 = dfactor;

		qglBlendFunc (sfactor, dfactor);
	}
}

#define GetTextureFromNum(num) (num+GL_TEXTURE0_ARB)

void GL_EnableTexture (int target, qboolean enable)
{
	if (gl_state.enabledtmus[target] != enable)
	{
		gl_state.enabledtmus[target] = enable;
	
		GL_SelectTexture (target);
	
		if (enable)
			qglEnable (GL_TEXTURE_2D);
		else
			qglDisable (GL_TEXTURE_2D);
	}
	
	// Since this function may or may not have changed the current texture
	// unit with GL_SelectTexture, the code that follows should not count on
	// any particular texture unit being selected.
	gl_state.currenttmu_defined = false;
}

static inline void GL_ForceSelectTexture (void)
{
	if (gl_state.tmuswitch_done)
		return;
	
	gl_state.tmuswitch_done = true;
	qglActiveTextureARB( GetTextureFromNum (gl_state.currenttmu) );
}

void GL_SelectTexture (int target)
{
	gl_state.currenttmu_defined = true;
	
	if (target == gl_state.currenttmu)
		return;

	// this should be a crash bug, as it depends entirely on our code and not
	// the GL environment.	
	assert (target < MAX_TMUS);

	gl_state.currenttmu = target;
	gl_state.tmuswitch_done = false;
	
	// TODO: figure out if I can remove this part:
	GL_ForceSelectTexture ();
}

void GL_TexEnv (GLenum mode)
{
	static qboolean firstrun = true;
	
	// Fun fact: setting each byte to an 8-bit -1 works even if it's actually
	// an array of regular-size integers, just like 0.
	if (firstrun)
	{
		firstrun = false;
		memset (gl_state.currenttexturemodes, -1, sizeof(gl_state.currenttexturemodes));
	}

	// TODO: eliminate every current warning, then change to regular
	// Com_Printf to prevent new ones from going unnoticed.
	if (!gl_state.currenttmu_defined)
		Com_DPrintf ("Warning: GL_TexEnv with undefined TMU!\n");
	
	GL_ForceSelectTexture();
	
	if (mode != gl_state.currenttexturemodes[gl_state.currenttmu])
	{
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		gl_state.currenttexturemodes[gl_state.currenttmu] = mode;
	}
}

void GL_MTexEnv (int target, GLenum mode)
{
	if (gl_state.currenttexturemodes[target] != mode)
	{
		GL_SelectTexture (target);
		GL_TexEnv (mode);
	}
	
	// Since this function may or may not have changed the current texture
	// unit with GL_SelectTexture, the code that follows should not count on
	// any particular texture unit being selected.
	gl_state.currenttmu_defined = false;
}

void GL_Bind (int texnum)
{
	// TODO: eliminate every current warning, then change to regular
	// Com_Printf to prevent new ones from going unnoticed.
	if (!gl_state.currenttmu_defined)
		Com_DPrintf ("Warning: GL_Bind with undefined TMU!\n");

	if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;
	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	GL_ForceSelectTexture();
	qglBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind (int target, int texnum)
{
	if (gl_state.currenttextures[target] != texnum)
	{
		GL_SelectTexture (target);
		GL_Bind (texnum);
	}
	
	// Since this function may or may not have changed the current texture
	// unit with GL_SelectTexture, the code that follows should not count on
	// any particular texture unit being selected.
	gl_state.currenttmu_defined = false;
}

// If you need to use the OpenGL texture calls manually for some reason, use
// this function right after so the wrapper functions know that the current
// state changed when they weren't watching.
void GL_InvalidateTextureState (void)
{
	gl_state.tmuswitch_done = false;
	GL_ForceSelectTexture ();
	gl_state.currenttmu_defined = false;
	memset (gl_state.currenttexturemodes, -1, sizeof(gl_state.currenttexturemodes));
	memset (gl_state.currenttextures, -1, sizeof(gl_state.currenttextures));
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_strcasecmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		Com_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	GL_SelectTexture (0);
	
	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_particle && glt->type != it_sky )
		{
			GL_Bind (glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_strcasecmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		Com_Printf ("bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_strcasecmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		Com_Printf ("bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void	GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	Com_Printf ("------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			Com_Printf ("M");
			break;
		case it_sprite:
			Com_Printf ("S");
			break;
		case it_wall:
			Com_Printf ("W");
			break;
		case it_pic:
			Com_Printf ("P");
			break;
		case it_particle:
			Com_Printf ("A");
			break;
		default:
			Com_Printf (" ");
			break;
		}

		Com_Printf (" %3i %3i %s: %s\n",
			image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	Com_Printf ("Total texel count (not counting mipmaps): %i\n", texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		1024
#define	BLOCK_HEIGHT	512

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
/*	Sys_Error ("Scrap_AllocBlock: full");*/
}

static void Scrap_Upload (int texnum)
{
	GL_SelectTexture (0);

	GL_Bind (texnum + TEXNUM_SCRAPS);
	GL_Upload32 (scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, 0, false, false, true);
}

//just a guessed size-- this isn't necessarily raw RGBA data, it's the
//encoded image data.
#define	STATIC_RAWDATA_SIZE	(1024*1024*4+256)
static byte	static_rawdata[STATIC_RAWDATA_SIZE];

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = FS_LoadFile_TryStatic (	filename, (void **)&raw, static_rawdata, 
									STATIC_RAWDATA_SIZE);
	if (!raw)
	{
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 1024
		|| pcx->ymax >= 512)
	{
		Com_Printf ("Bad pcx file %s\n", filename);
		if ((byte *)pcx != static_rawdata)
			FS_FreeFile (pcx);
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		Com_DPrintf ("PCX file %s was malformed", filename);
		free (*pic);
		*pic = NULL;
	}

	if ((byte *)pcx != static_rawdata)
		FS_FreeFile (pcx);
}
/*
=================================================================

JPEG LOADING

By Robert 'Heffo' Heffernan

=================================================================
*/

static JOCTET eoi_buffer[2] = {(JOCTET)0xFF, (JOCTET)JPEG_EOI};
static qboolean crjpg_corrupted;

void crjpg_null(j_decompress_ptr cinfo)
{
}

int crjpg_fill_input_buffer(j_decompress_ptr cinfo)
{
    Com_Printf("Premature end of JPEG data\n");
    cinfo->src->next_input_byte = eoi_buffer;
    cinfo->src->bytes_in_buffer = 2;
    crjpg_corrupted = true;
    return 1;
}

void crjpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{

    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

    if (cinfo->src->bytes_in_buffer < 0)
    {
		Com_Printf("Premature end of JPEG data\n");
		cinfo->src->next_input_byte = eoi_buffer;
    	cinfo->src->bytes_in_buffer = 2;
    	crjpg_corrupted = true;
    }
}

void crjpg_mem_src(j_decompress_ptr cinfo, byte *mem, int len)
{
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = crjpg_null;
    cinfo->src->fill_input_buffer = crjpg_fill_input_buffer;
    cinfo->src->skip_input_data = crjpg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = crjpg_null;
    cinfo->src->bytes_in_buffer = len;
    cinfo->src->next_input_byte = mem;
}

#define DSTATE_START	200	/* after create_decompress */
#define DSTATE_INHEADER	201	/* reading header markers, no SOS yet */

/*
==============
LoadJPG
==============
*/
#define	STATIC_SCANLINE_SIZE	(1024*3)
byte	static_scanline[STATIC_SCANLINE_SIZE];
void LoadJPG (char *filename, byte **pic, int *width, int *height)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rawdata, *rgbadata, *scanline, *p, *q;
	int								rawsize, i;

	crjpg_corrupted = false;
	// Load JPEG file into memory
	rawsize = FS_LoadFile_TryStatic	(	filename, (void **)&rawdata,
										static_rawdata, STATIC_RAWDATA_SIZE);
	if (!rawdata)
	{
		return;
	}

	// Knightmare- check for bad data
	if (	rawdata[6] != 'J'
		||	rawdata[7] != 'F'
		||	rawdata[8] != 'I'
		||	rawdata[9] != 'F') {
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Initialise libJpeg Object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Feed JPEG memory into the libJpeg Object
	crjpg_mem_src(&cinfo, rawdata, rawsize);

	// Process JPEG header
	jpeg_read_header(&cinfo, true); // bombs out here

	// Start Decompression
	jpeg_start_decompress(&cinfo);

	// Check Color Components
	if(cinfo.output_components != 3)
	{
		jpeg_destroy_decompress(&cinfo);
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Allocate Memory for decompressed image
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if(!rgbadata)
	{
		jpeg_destroy_decompress(&cinfo);
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Pass sizes to output
	*width = cinfo.output_width; *height = cinfo.output_height;

	// Allocate Scanline buffer
	if (cinfo.output_width * 3 < STATIC_SCANLINE_SIZE)
	{
		scanline = &static_scanline[0];
	}
	else
	{
		scanline = malloc(cinfo.output_width * 3);
		if(!scanline)
		{
			free(rgbadata);
			jpeg_destroy_decompress(&cinfo);
			if (rawdata != static_rawdata)
				FS_FreeFile(rawdata);
			return;
		}
	}

	// Read Scanlines, and expand from RGB to RGBA
	q = rgbadata;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		p = scanline;
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		for(i=0; i<cinfo.output_width; i++)
		{
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p+=3; q+=4;
		}
	}

	// Free the scanline buffer
	if (scanline != &static_scanline[0])
		free(scanline);

	// Finish Decompression
	jpeg_finish_decompress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_decompress(&cinfo);

	// Free the raw data now that it's done being processed
	if (rawdata != static_rawdata)
    	FS_FreeFile(rawdata);
    
    if (crjpg_corrupted)
    	Com_Printf ("JPEG file %s is likely corrupted, please obtain a fresh copy.\n", filename);

	// Return the 'rgbadata'
	*pic = rgbadata;
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================


/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (byte *in, int inwidth, int inheight, byte *out, int outwidth, int outheight)
{
	int		i, j;
	byte	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth*4)
	{
		inrow = in + 4*inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + 4*inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			*(out+4*j+0) = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			*(out+4*j+1) = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			*(out+4*j+2) = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			*(out+4*j+3) = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
R_FilterTexture

Applies brightness and contrast to the specified image while optionally computing
the image's average color.  Also handles image inversion and monochrome.  This is
all munged into one function to reduce loops on level load.
*/
void R_FilterTexture(byte *in, int width, int height, qboolean modulate)
{
	int count;
	float fcb;
	int ix;
	static byte lut[256];
	static int first_time = 1;
	static float lut_gamma = 0.0f;
	static float lut_contrast = 0.0f;

	if ( lut_gamma != vid_gamma->value || lut_contrast != vid_contrast->value || first_time ) {
		first_time = 0;
		lut_gamma = vid_gamma->value;
		lut_contrast = vid_contrast->value;
		// build lookup table
		for ( ix = 0; ix < sizeof(lut); ix++ ) {
			fcb = (float)ix / 255.0f;
			fcb *= lut_gamma;
			if (fcb < 0.0f )
				fcb = 0.0f;
			fcb -= 0.5f;
			fcb *= lut_contrast;
			fcb += 0.5f;
			fcb *= 255.0f;
			if ( fcb >= 255.0f )
				lut[ix] = 255 ;
			else if (fcb <= 0.0f )
				lut[ix] = 0;
			else
				lut[ix] = (byte)fcb;
		}
	}

	// apply to image
	count = width * height;
	while ( count-- ) {
		int i;
		for (i = 0; i < 3; i++)
		{
			if (modulate)
			{
				int c = (int)(gl_modulate->value*(*in));
				if (c > 255)
					c = 255;
				*in = c;
			}
			else
			{
				*in = lut[*in];
			}
			++in;
		}
		++in;
	}

}


#define ALPHATEST_THRESHOLD 169
// Uses the canonical mipmap generation algorithm, but unlike OpenGL's built-
// in mipmapping, it this function knows when to quit-- that is, when the 
// mipmaps have gotten too "mushy." Mushy mipmaps usually aren't a problem, 
// but they're a huge one when dealing with alphatest surfaces.
int GL_RecursiveGenerateAlphaMipmaps (byte *data, int width, int height, int depth)
{
	int			ret;
	int			x, y;
	int			newwidth, newheight;
	int			upper_left_alpha;
	qboolean	uniform = true;
	byte		*mipmap, *scan, *scan2, *out;
	
	if (width == 1 && height == 1)
		return depth;
	
	newwidth = width;
	if (newwidth > 1)
		newwidth /= 2;
	newheight = height;
	if (newheight > 1)
		newheight /= 2;
	
	mipmap = malloc (newwidth*newheight*4); 
	
	for (y = 0; (y+1) < height; y += 2)
	{
		for (x = 0; (x+1) < width; x += 2)
		{
			scan = data + (y*width+x)*4;
			scan2 = data + ((y+1)*width+x)*4;
			out = mipmap + ((y/2)*(width/2)+(x/2))*4;
			out[0] = (scan[0]+scan[4]+scan2[0]+scan2[4])/4;
			out[1] = (scan[1]+scan[5]+scan2[1]+scan2[5])/4;
			out[2] = (scan[2]+scan[6]+scan2[2]+scan2[6])/4;
			out[3] = (scan[3]+scan[7]+scan2[3]+scan2[7])/4;
			if (x == 0 && y == 0)
				upper_left_alpha = out[3];
			else if ((upper_left_alpha > ALPHATEST_THRESHOLD) != (out[3] > ALPHATEST_THRESHOLD))
				uniform = false;
		}
	}
	
	if (uniform)
	{
		free (mipmap);
		return depth;
	}
	
	ret = GL_RecursiveGenerateAlphaMipmaps (mipmap, newwidth, newheight, depth+1);
	
	qglTexImage2D (GL_TEXTURE_2D, depth+1, gl_tex_alpha_format, newwidth, newheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipmap);
	
	free (mipmap);
	return ret;
}

int GL_GenerateAlphaMipmaps (byte *data, int width, int height)
{
	return GL_RecursiveGenerateAlphaMipmaps (data, width, height, 0);
}


int		upload_width, upload_height;
int		crop_left, crop_right, crop_top, crop_bottom;
qboolean	uploaded_paletted;


qboolean GL_Upload32 (byte *data, int width, int height, int picmip, qboolean filter, qboolean modulate, qboolean force_standard_mipmap)
{
	int		samples;
	byte 	*scaled;
	int		scaled_width, scaled_height;
	int		i, c;
	byte		*scan;
	int comp;

	if(filter)
		R_FilterTexture(data, width, height, modulate);

	uploaded_paletted = false;    // scan the texture for any non-255 alpha
	c = width*height;
	scan = data + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}
	comp = (samples == gl_solid_format) ? gl_tex_solid_format : gl_tex_alpha_format;

	{
		int max_size;
		int i;
		qglGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);
		scaled_width = width;
		scaled_height = height;
		
		// let people sample down the world textures for speed
		for (i = 0; i < picmip; i++)
		{
			if (scaled_width <= 1 || scaled_height <= 1)
				break;
			scaled_width >>= 1;
			scaled_height >>= 1;
		}
		if (scaled_width > max_size)
			scaled_width = max_size;
		if (scaled_height > max_size)
			scaled_height = max_size;
	}

	if (scaled_width != width || scaled_height != height) {
		scaled=malloc((scaled_width * scaled_height) * 4);
		GL_ResampleTexture(data,width,height,scaled,scaled_width,scaled_height);
	} else {
		scaled=data;
	}

	if (samples != gl_alpha_format || force_standard_mipmap)
		qglTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	
	crop_left = crop_right = crop_top = crop_bottom = 0;
	if (samples == gl_alpha_format)
	{
		int			x, y;
		qboolean 	found;
		
		scan = (byte*)scaled+3;
		
		for (y = 0; y < scaled_height; y++, crop_top++)
		{
			found = false;
			for (x = 0; x < scaled_width; x++)
			{
				scan = ((byte*)scaled + (y*scaled_width+x)*4+3);
				if (*scan != 0)
					found = true;
			}
			if (found)
				break;
		}
		
		crop_left = scaled_width;
		crop_right = scaled_width;
		
		for (; y < scaled_height; y++)
		{
			for (x = 0; x < scaled_width; x++)
			{
				scan = ((byte*)scaled + (y*scaled_width+x)*4+3);
				if (*scan != 0)
				{
					if (x < crop_left)
						crop_left = x;
					if (scaled_width-x-1 < crop_right)
						crop_right = scaled_width-x-1;
					crop_bottom = scaled_height-y-1;
				}
			}
		}
	}
	
	if (samples == gl_alpha_format && !force_standard_mipmap)
	{
		int generated_mipmaps = 
			GL_GenerateAlphaMipmaps (scaled, scaled_width, scaled_height);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, generated_mipmaps);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_alphamasked_anisotropic->integer);
	}
	else
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->integer);

	if (scaled_width != width || scaled_height != height)
		free(scaled);

	upload_width = scaled_width;
	upload_height = scaled_height;
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	return (samples == gl_alpha_format);
}

/*
===============
GL_Upload8

Returns has_alpha
===============
*/
qboolean GL_Upload8 (byte *data, int width, int height, int picmip, qboolean filter)
{
	unsigned	trans[512*256]; // contains 32 bit RGBA color
	int			i, s;
	int			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		Com_Error (ERR_DROP, "GL_Upload8: too large");
	
	for (i=0 ; i<s ; i++)
    {
		p = data[i];
        trans[i] = d_8to24table[p];

        if (p == 255)
        {   // transparent, so scan around for another color
            // to avoid alpha fringes
            // FIXME: do a full flood fill so mips work...
            if (i > width && data[i-width] != 255)
                p = data[i-width];
            else if (i < s-width && data[i+width] != 255)
                p = data[i+width];
            else if (i > 0 && data[i-1] != 255)
                p = data[i-1];
            else if (i < s-1 && data[i+1] != 255)
                p = data[i+1];
            else
                p = 0;
            // copy rgb components
            ((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
            ((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
            ((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
        }
    }
    return GL_Upload32 ((byte*)trans, width, height, picmip, filter, false, false);
}

/*
================
GL_ReservePic

Find a free image_t
================
*/
image_t *GL_FindFreeImage (const char *name, int width, int height, imagetype_t type)
{
	image_t		*image;
	int			i;
	
	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			Com_Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		Com_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);

	strcpy (image->name, name);

	if ( name[0] != '*' )
	{ // not a special name, remove the path part
		strcpy( image->bare_name, COM_SkipPath( name ) );
	}

	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;
	
	image->texnum = TEXNUM_IMAGES + (image - gltextures);
	
	return image;
}

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (const char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;
	int			picmip;

	image = GL_FindFreeImage (name, width, height, type);

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);
	
	if (type == it_particle || type == it_pic)
		picmip = 0;
	else
		picmip = gl_picmip->integer;

	// load little particles into the scrap
	if (type == it_particle && bits != 8
		&& image->width <= 128 && image->height <= 128)
	{
		int		x, y;
		int		i, j, k, l;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		
		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++)
			    for (l = 0; l < 4; l++, k++)
				    scrap_texels[texnum][((y+i)*BLOCK_WIDTH + x + j)*4+l] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum; // overwrite old texnum
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (double)(x+0.5)/(double)BLOCK_WIDTH;
		image->sh = (double)(x+image->width-0.5)/(double)BLOCK_WIDTH;
		image->tl = (double)(y+0.5)/(double)BLOCK_HEIGHT;
		image->th = (double)(y+image->height-0.5)/(double)BLOCK_HEIGHT;
		
		// Send updated scrap to OpenGL. Wasteful to do it over and over like
		// this, but we don't care.
		Scrap_Upload (texnum);
	}
	else
	{
nonscrap:
		image->scrap = false;
		GL_SelectTexture (0);
		GL_Bind (image->texnum);
		if (bits == 8) {
			image->has_alpha = GL_Upload8 (pic, width, height, picmip, type <= it_wall);
		} else {
			image->has_alpha = GL_Upload32 (pic, width, height, picmip, type <= it_wall, type == it_lightmap, type >= it_bump);
		}

		if (type == it_pic)
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		
		image->paletted = uploaded_paletted;

		// size in pixels after power of 2 and scales
		image->upload_width = upload_width;
		image->upload_height = upload_height;
		
		// vertex offset to apply when cropping
		image->crop_left = crop_left;
		image->crop_top = crop_top;
		
		// size in pixels after cropping
		image->crop_width = upload_width-crop_left-crop_right;
		image->crop_height = upload_height-crop_top-crop_bottom;
		
		// texcoords to use when not cropping
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
		
		// texcoords to use when cropping
		image->crop_sl = 0 + (float)crop_left/(float)upload_width;
		image->crop_sh = 1 - (float)crop_right/(float)upload_width;
		image->crop_tl = 0 + (float)crop_top/(float)upload_height;
		image->crop_th = 1 - (float)crop_bottom/(float)upload_height;
	}

	COMPUTE_HASH_KEY( image->hash_key, name, i );
	return image;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = GL_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	FS_FreeFile ((void *)mt);

	return image;
}


/*
===============
GL_GetImage

Finds an image, do not attempt to load it
===============
*/
image_t *GL_GetImage( const char * name )
{
	image_t *	image = NULL;
	unsigned int	hash_key;
	int		i;

	COMPUTE_HASH_KEY( hash_key, name, i );
	for ( i = 0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (hash_key == image->hash_key && !strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	return NULL;
}


/*
===============
GL_FreeImage

Frees an image slot and deletes the texture
===============
*/
void GL_FreeImage( image_t * image )
{
	if ( ! image->texnum )
		return;
	qglDeleteTextures (1, (unsigned *)&image->texnum );
	if ( gl_state.currenttextures[gl_state.currenttmu] == image->texnum ) {
		gl_state.currenttextures[ gl_state.currenttmu ] = 0;
	}
	memset (image, 0, sizeof(*image));
}


/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (const char *name, imagetype_t type)
{
	image_t		*image = NULL;
	int		len;
	byte		*pic, *palette;
	int		width, height;
	char		shortname[MAX_QPATH];

	if (!name)
		goto ret_image;	//	Com_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len < 5)
		goto ret_image;	//	Com_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	//if HUD, then we want to load the one according to what it is set to.
	if(!strcmp(name, "pics/i_health.pcx"))
		return GL_FindImage (cl_hudimage1->string, type);
	if(!strcmp(name, "pics/i_score.pcx"))
		return GL_FindImage (cl_hudimage2->string, type);
	if(!strcmp(name, "pics/i_ammo.pcx"))
		return GL_FindImage (cl_hudimage3->string, type);

	// look for it
	image = GL_GetImage( name );
	if ( image != NULL )
		return image;

	// strip off .pcx, .tga, etc...
	COM_StripExtension ( name, shortname );

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;

	// Try to load the image with different file extensions, in the following
	// order of decreasing preference: TGA, JPEG, PCX, and WAL.
	
	LoadTGA (va("%s.tga", shortname), &pic, &width, &height);
	if (pic)
	{
		image = GL_LoadPic (name, pic, width, height, type, 32);
		goto done;
	}
	
	LoadJPG (va("%s.jpg", shortname), &pic, &width, &height);
	if (pic)
	{
		image = GL_LoadPic (name, pic, width, height, type, 32);
		goto done;
	}
	
	// TGA and JPEG are the only file types used for heightmaps and
	// normalmaps, so if we haven't found it yet, it isn't there, and we can
	// save ourselves a file lookup.
	if (type == it_bump)
		goto done;
	
	LoadPCX (va("%s.pcx", shortname), &pic, &palette, &width, &height);
	if (pic)
	{
		image = GL_LoadPic (name, pic, width, height, type, 8);
		goto done;
	}

	if (type == it_wall)
		image = GL_LoadWal (va("%s.wal", shortname));

done:
	if (pic)
		free(pic);
	if (palette)
		free(palette);
	
	if (image != NULL)
		image->script = RS_FindScript (shortname);

ret_image:
	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free r_notexture
	r_notexture->registration_sequence = registration_sequence;
	
	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic || image->type == it_sprite || image->type == it_particle)
			continue;		// don't free pics or particles
		// free it
		qglDeleteTextures (1, (unsigned *)&image->texnum );
		memset (image, 0, sizeof(*image));
	}
}

/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
	{
		Com_Error (ERR_FATAL,
				"Couldn't load pics/colormap.pcx\n"
				"The game executable cannot find its data files. This means\n"
				"the game is likely not installed correctly."
				// TODO: automatically generate a list of paths where the data
				// files should be
#if defined UNIX_VARIANT
				" If you\ninstalled the game from the repository of a Linux\n"
				"distribution, please report this to ther package\n"
				"maintainers and have them check the README for packaging\n"
				"instructions."
#endif //UNIX_VARIANT
			);
	}

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];

		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}


	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	free (pic);
	free (pal);

	return 0;
}

// TODO: have r_newrefdef available before calling these, put r_mirroretxture
// in that struct, base it on the viewport specified in that struct. (Needed
// for splitscreen.)
void R_InitMirrorTextures( void )
{
	byte	*data;
	int		size;
	int		size_oneside;

	//init the partial screen texture
	size_oneside = ceil((512.0f/1080.0f)*(float)vid.height);
	size = size_oneside * size_oneside * 4;
	data = malloc( size );
	memset( data, 255, size );
	r_mirrortexture = GL_LoadPic( "***r_mirrortexture***", (byte *)data, size_oneside, size_oneside, it_pic, 32 );
	free ( data );
}

void R_InitDepthTextures( void )
{
	byte	*data;
	int		size, buffersize;
	int 	bigsize, littlesize;
	int		i;

	//init the framebuffer textures
	bigsize = max (vid.width, vid.height) * 2 * r_shadowmapscale->value;
	littlesize = min (vid.width, vid.height) * r_shadowmapscale->value;
	bigsize = littlesize;
	buffersize = bigsize * bigsize * 4;

	data = malloc (buffersize);
	memset (data, 255, buffersize);

	for (i = 0; i < deptex_num; i++)
	{
		Com_sprintf (deptex_names[i], sizeof (deptex_names[i]), "***r_depthtexture%d***", i);
		if (i == deptex_sunstatic)
			size = bigsize;
		else
			size = littlesize;
		r_depthtextures[i] = GL_LoadPic (deptex_names[i], (byte *)data, size, size, it_pic, 32);
	}

	free ( data );
}


/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{

	registration_sequence = 1;

	gl_state.inverse_intensity = 1;

	Draw_GetPalette ();

	R_InitMirrorTextures();//MIRRORS
	R_InitDepthTextures();//DEPTH(SHADOWMAPS)
	R_FB_InitTextures();//FULLSCREEN EFFECTS
	R_SI_InitTextures();//SIMPLE ITEMS
	R_InitBloomTextures();//BLOOMS
	R_Decals_InitFBO (); // DECALS
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		// free it
		qglDeleteTextures (1, (unsigned *)&image->texnum);
		memset (image, 0, sizeof(*image));
	}
	
	memset (scrap_allocated, 0, sizeof(scrap_allocated));
	memset (scrap_texels, 0, sizeof(scrap_texels));
}

