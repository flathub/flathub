/*
Copyright (C) 2010 COR Entertainment, LLC.

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

/* r_ttf.c
 *
 * Displays text using TrueType fonts, loaded through the FreeType
 * library.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "r_local.h"
#include "r_ttf.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef FT_LCD_FILTER_H
#include FT_LCD_FILTER_H
#endif




/*****************************************************************************
 * LOCAL CONSTANTS                                                           *
 *****************************************************************************/

/* Amount of TTF characters to draw, starting from and including space. */
#define TTF_CHARACTERS	96

/*
 * Width of the textures used to store TTF fonts. Should be greater than 64
 * to prevent TTF textures from being uploaded to the scrap buffer.
 */
#define TTF_TEXTURE_WIDTH (1 << 9)



/*****************************************************************************
 * LOCAL TYPE DEFINITIONS                                                    *
 *****************************************************************************/

/* Internal structure for TTF faces */
struct _TTF_face_s
{
	/* TTF file data */
	FT_Byte *	data;

	/* TTF file size */
	unsigned int	size;
};
typedef struct _TTF_face_s * _TTF_face_t;


/* Internal structure for TTF fonts */
struct _TTF_font_s
{
	/* Last flags used to render the font */
	unsigned int		flags;

	/* Image used as a texture */
	image_t *		texture;

	/* Bitmap containing the font's characters */
	unsigned char *		bitmap;

	/* Width of each character */
	int			widths[ TTF_CHARACTERS ];

	/* "Kerning" - specific offset to use for sequences of characters */
	int			kerning[ TTF_CHARACTERS ][ TTF_CHARACTERS ];

	/* Height of the bitmap */
	int			height;

	/* GL list identifier */
	unsigned int		list_id;
};
typedef struct _TTF_font_s * _TTF_font_t;



/*****************************************************************************
 * LOCAL FUNCTIONS ENCAPSULATED BY FONT/FACE STRUCTURES                      *
 *****************************************************************************/

/* Initialise a TTF font */
static qboolean _TTF_GetFont( FNT_font_t font );

/* Destroy a TTF face */
static void _TTF_DestroyFace( FNT_face_t face );

/* Raw printing function */
static void _TTF_RawPrint(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] );

/* Bounded printing function */
static void _TTF_BoundedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	);

/* Wrapped printing function */
static void _TTF_WrappedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	);

/* Wrapped printing function */
static int _TTF_PredictSize(
		FNT_font_t	font ,
		const char *	text,
		qboolean		color
	);


/*****************************************************************************
 * LOCAL VARIABLES                                                           *
 *****************************************************************************/

#if !defined NDEBUG
/*
 * When debugging is not disabled, make sure the library is in the
 * right state before initialising or shutting down.
 */
static qboolean			_TTF_initialised = false;
#endif // !defined NDEBUG

/* The TTF library instance. */
static FT_Library		_TTF_library;

/* CVar to enable/disable sub-pixel rendering. */
static cvar_t *			_TTF_subpixel;

/* CVar that controls autohinting. */
static cvar_t *			_TTF_autohint;


/* Macro that computes a font's flags from the TTF control variables */
#define TTF_COMPUTE_FLAGS() ( \
	( (_TTF_autohint && _TTF_autohint->integer ) ? 1 : 0 ) \
	| ( _TTF_subpixel ? ( 1 + ( _TTF_subpixel->integer << 1 ) ) : 0) << 1 \
)



/*****************************************************************************
 * ENGINE INITIALISATION & SHUTDOWN FUNCTIONS                                *
 *****************************************************************************/

/*
 * Initialise the FreeType library
 */
qboolean TTF_Initialise( )
{
	FT_Error error;

#if !defined NDEBUG
	if ( _TTF_initialised ) {
		Com_DPrintf( "TTF: library already initialised\n" );
		return false;
	}
#endif // !defined NDEBUG

	// Initialise FreeType
	error = FT_Init_FreeType( &_TTF_library );
	if ( error != 0 ) {
		Com_Printf( "TTF: unable to initialise library (error code %d)\n" , error );
		return false;
	}

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	// Access CVar for sub-pixel rendering
	_TTF_subpixel = Cvar_Get( "ttf_subpixel" , "0" , CVAR_ARCHIVE );
	_TTF_subpixel->modified = false;

	// Attempt to initialise sub-pixel rendering
	switch ( _TTF_subpixel->integer ) {
		case 0:
			break;
		case 1:
			FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_NONE );
			break;
		case 2:
			FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_DEFAULT );
			break;
		case 3:
			FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_LIGHT );
			break;
	}
#else // FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	_TTF_subpixel = NULL;
#endif // FT_CONFIG_OPTION_SUBPIXEL_RENDERING

	// Access CVar for autohinting
	_TTF_autohint = Cvar_Get( "ttf_autohint" , "0" , CVAR_ARCHIVE );
	_TTF_autohint->modified = false;

#if !defined NDEBUG
	Com_Printf( "...initialised TTF engine\n" );
	_TTF_initialised = true;
#endif // !defined NDEBUG
	return true;
}


/*
 * Shutdown the FreeType library
 */
void TTF_Shutdown( )
{
#if !defined NDEBUG
	if ( ! _TTF_initialised ) {
		Com_DPrintf( "TTF: library is not initialised\n" );
		return;
	}
	Com_Printf( "...shutting down TTF engine\n" );
	_TTF_initialised = false;
#endif // !defined NDEBUG
	FT_Done_FreeType( _TTF_library );
}




/*****************************************************************************
 * FONT FRONT-END INTERFACE FUNCTION                                         *
 *****************************************************************************/

qboolean TTF_InitFace( FNT_face_t face )
{
	char		fileName[ FNT_FACE_NAME_MAX + 11 ];
	FT_Byte *	fileData;
	unsigned int	dataSize;
	FT_Face		ftFace;
	FT_Error	error;
	_TTF_face_t	internal;
	qboolean	ok;

	Com_sprintf( fileName , sizeof( fileName ) , "fonts/%s.ttf" , face->name );

	// Attempt to load the font's file
	dataSize = FS_LoadFile( fileName , (void **)&fileData );
	if ( !fileData ) {
		return false;
	}

	// Load TTF face information
	error = FT_New_Memory_Face( _TTF_library , fileData , dataSize , 0 , &ftFace );
	if ( error != 0 ) {
		Com_Printf( "TTF: failed to load font from '%s' (error code %d)\n" , fileName , error );
		FS_FreeFile( fileData );
		return false;
	}

	// Check font information
	if ( (ftFace->face_flags & FT_FACE_FLAG_SCALABLE) && (ftFace->face_flags & FT_FACE_FLAG_HORIZONTAL) ) {
		ok = true;
	} else {
		Com_Printf( "TTF: font loaded from '%s' is not scalable\n" , fileName );
		ok = false;
	}
	FT_Done_Face( ftFace );

	if ( ok ) {
		// Initialise face structure's internal data and function pointers
		internal = (_TTF_face_t) Z_Malloc( sizeof( struct _TTF_face_s ) );
		internal->data = fileData;
		internal->size = dataSize;
		face->internal = internal;
		face->GetFont = _TTF_GetFont;
		face->Destroy = _TTF_DestroyFace;
	} else {
		FS_FreeFile( fileData );
	}

	return ok;
}



/*****************************************************************************
 * FONT MANAGEMENT FUNCTIONS                                                 *
 *****************************************************************************/

/* Face destruction function */
static void _TTF_DestroyFace( FNT_face_t face )
{
	_TTF_face_t	faceInt = (_TTF_face_t) face->internal;
	FS_FreeFile( faceInt->data );
	Z_Free( faceInt );
}


/* The actual loader function */
static qboolean _TTF_LoadFont( FNT_font_t font )
{
	_TTF_face_t		faceInt = (_TTF_face_t) font->face->internal;
	_TTF_font_t		fontInt = (_TTF_font_t) font->internal;
	FT_Error		error;
	FT_Face			face;

	unsigned int		render_mode;
	unsigned int		glyphs[ TTF_CHARACTERS ];
	int			max_ascent , max_descent;
	unsigned int		x , y;
	unsigned int		n_lines;
	unsigned int		i;
	unsigned int		texture_height;
	
	// Load TTF face information
	error = FT_New_Memory_Face( _TTF_library , faceInt->data , faceInt->size , 0 , &face );
	if ( error != 0 ) {
		Com_Printf( "TTF: failed to load font face '%s' (error code %d)\n" , font->face->name , error );
		return false;
	}

	// Sets the font's size
	error = FT_Set_Pixel_Sizes( face , font->size , 0 );
	if ( error != 0 ) {
		Com_Printf( "TTF: could not set size %lu to font '%s' (error code %d)\n" , font->size , font->face->name , error );
		goto _TTF_load_err_0;
	}

	// Set rendering mode
	render_mode = FT_LOAD_RENDER;
	if ( _TTF_autohint->integer )
		render_mode |=  FT_LOAD_FORCE_AUTOHINT;
	if ( _TTF_subpixel && _TTF_subpixel->integer ) {
		render_mode |= FT_LOAD_TARGET_LCD;
	} else {
		render_mode |= FT_LOAD_TARGET_NORMAL;
	}

	// Get size information
	x = TTF_TEXTURE_WIDTH;
	n_lines = 1;
	max_ascent = max_descent = 0;
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		int temp;

		// Load the character
		glyphs[ i ] = FT_Get_Char_Index( face , i + ' ' );
		error = FT_Load_Glyph( face , glyphs[ i ] , render_mode );
		if ( error != 0 ) {
			Com_Printf( "TTF: could not load character '%c' from font '%s' (error code %d)\n" , i + ' ' , font->face->name , error );
			goto _TTF_load_err_1;
		}

		// Get horizontal advance
		fontInt->widths[ i ] = face->glyph->metrics.horiAdvance >> 6;
		if ( ( face->glyph->metrics.horiAdvance & 0x3f ) != 0 ) {
			fontInt->widths[ i ] ++;
		}

		// Now get the "real" width and check if the line is full
		temp = face->glyph->bitmap.width;
		if ( _TTF_subpixel && _TTF_subpixel->integer ) {
			temp /= 3;
		}

		if ( temp > x ) {
			n_lines ++;
			x = TTF_TEXTURE_WIDTH;
		}
		x -= temp;

		// Update max ascent / descent
		if ( face->glyph->bitmap_top > max_ascent )
			max_ascent = face->glyph->bitmap_top;
		temp = 1 + face->glyph->bitmap.rows - face->glyph->bitmap_top;
		if ( temp > max_descent )
			max_descent = temp;
	}
	
	font->height = fontInt->height = max_ascent + max_descent;
	
	// FIXME HACK: some fonts (like freemono) disappear if used as the menu
	// font if we don't do this, because the menu code ends up calculating 
	// negative margins.
	if (font->height < font->size)
		font->height = font->size;

	font->width = 0;
	// Get kerning information
	if ( FT_HAS_KERNING( face ) ) {
		for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
			int j;
			for ( j = 0 ; j < TTF_CHARACTERS ; j ++ ) {
				FT_Vector kvec;
				FT_Get_Kerning( face , glyphs[ i ] , glyphs[ j ] , FT_KERNING_DEFAULT , &kvec );
				fontInt->kerning[ i ][ j ] = kvec.x >> 6;
				if (fontInt->kerning[i][j] + fontInt->widths[i] > font->width)
					font->width = fontInt->kerning[i][j] + fontInt->widths[i];
				if (fontInt->kerning[i][j] + fontInt->widths[j] > font->width)
					font->width = fontInt->kerning[i][j] + fontInt->widths[j];
			}
		}
	} else {
		font->width = font->size;
		memset( fontInt->kerning , 0 , sizeof( fontInt->kerning ) );
	}
	
	// Compute texture height
	texture_height = ( max_ascent + max_descent) * n_lines;
	for ( i = 16 ; i <= 4096 ; i <<= 1 ) {
		if ( i > texture_height ) {
			texture_height = i;
			break;
		}
	}

	// Allocate texture
	fontInt->bitmap = Z_Malloc( TTF_TEXTURE_WIDTH * texture_height * 4 );
	if ( fontInt->bitmap == NULL )
		goto _TTF_load_err_0;
	memset( fontInt->bitmap , 0 , TTF_TEXTURE_WIDTH * texture_height * 4 );

	// Initialise GL lists
	//
	// XXX: make sure there's no pending error first; this should not be
	// needed, but since the rest of the GL code doesn't check for errors
	// that often...
	qglGetError( );
	fontInt->list_id = qglGenLists( TTF_CHARACTERS );
	if ( qglGetError( ) != GL_NO_ERROR ) {
		Com_Printf( "TTF: could not create OpenGL lists\n" );
		goto _TTF_load_err_1;
	}

	// Draw glyphs on texture
	x = 0;
	y = max_ascent;
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		unsigned int rWidth , tx , ty;
		unsigned char * bptr , * fptr;
		float x1 , x2 , y1 , y2;

		// Render the character
		error = FT_Load_Glyph( face , glyphs[ i ] , render_mode );
		if ( error != 0 ) {
			Com_Printf( "TTF: could not load character '%c' from font '%s' (error code %d)\n" , i + ' ' , font->face->name , error );
			goto _TTF_load_err_2;
		}

		// Compute actual width
		rWidth = face->glyph->bitmap.width;
		if ( _TTF_subpixel && _TTF_subpixel->integer ) {
			rWidth /= 3;
		}

		// Wrap current line if needed
		if ( rWidth > TTF_TEXTURE_WIDTH - x ) {
			x = 0;
			y += fontInt->height;
		}

		// Compute texture coordinates
		x1 = (float)x / (float)TTF_TEXTURE_WIDTH;
		x2 = (float)( x + fontInt->widths[ i ] ) / (float)TTF_TEXTURE_WIDTH;
		y1 = (float)( y - max_ascent ) / (float)texture_height;
		y2 = y1 + (float)( fontInt->height ) / (float)texture_height;

		// Create GL display list
		qglNewList( fontInt->list_id + i , GL_COMPILE );
		qglBegin( GL_QUADS );
		qglTexCoord2i( x , y - max_ascent );
		qglVertex2i( 0 , 0 );
		qglTexCoord2i( x + rWidth , y - max_ascent );
		qglVertex2i( rWidth , 0 );
		qglTexCoord2i( x + rWidth , y + max_descent );
		qglVertex2i( rWidth , fontInt->height );
		qglTexCoord2i( x , y + max_descent );
		qglVertex2i( 0 , fontInt->height );
		qglEnd( );
		qglEndList( );

		// Copy glyph image to bitmap
		bptr = fontInt->bitmap + ( x + ( y - face->glyph->bitmap_top ) * TTF_TEXTURE_WIDTH ) * 4;
		fptr = face->glyph->bitmap.buffer;
		for ( ty = 0 ; ty < face->glyph->bitmap.rows ; ty ++ ) {
			unsigned char * rbptr = bptr;

			if ( _TTF_subpixel && _TTF_subpixel->integer ) {
				for ( tx = 0 ; tx < face->glyph->bitmap.width / 3 ; tx ++ , rbptr += 4 , fptr += 3 ) {
					rbptr[0] = fptr[0];
					rbptr[1] = fptr[1];
					rbptr[2] = fptr[2];
					rbptr[3] = fptr[0] / 3 + fptr[1] / 3 + fptr[2] / 3;
				}
			} else {
				for ( tx = 0 ; tx < face->glyph->bitmap.width ; tx ++ , rbptr += 4 , fptr ++ ) {
					rbptr[0] = 255;
					rbptr[1] = 255;
					rbptr[2] = 255;
					rbptr[3] = *fptr;
				}
			}

			bptr += TTF_TEXTURE_WIDTH * 4;
			fptr += face->glyph->bitmap.pitch - face->glyph->bitmap.width;
		}

		// Update current X
		x += rWidth;
	}

	// Upload the texture
	// FIXME: if GL_LoadPic fails, we just wasted some memory
	fontInt->texture = GL_LoadPic( va( "***TTF*%s***" , font->lookup ) ,
				fontInt->bitmap , TTF_TEXTURE_WIDTH , texture_height ,
				it_pic , 32 );

	// Set flags
	fontInt->flags = TTF_COMPUTE_FLAGS( );

	FT_Done_Face( face );
	return true;

_TTF_load_err_2:
	qglDeleteLists( fontInt->list_id , TTF_CHARACTERS );
_TTF_load_err_1:
	free( fontInt->bitmap );
_TTF_load_err_0:
	FT_Done_Face( face );
	return false;
}


/*
 * Check CVar changes, modify FreeType settings if needed.
 */
static void _TTF_CheckCVars( )
{
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	if ( _TTF_subpixel->modified ) {
		switch ( _TTF_subpixel->integer ) {
			case 0:
				break;
			case 1:
				FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_NONE );
				break;
			case 2:
				FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_DEFAULT );
				break;
			case 3:
				FT_Library_SetLcdFilter( _TTF_library , FT_LCD_FILTER_LIGHT );
				break;
		}
		_TTF_subpixel->modified = false;
	}
#endif // FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	_TTF_autohint->modified = false;
}


/*
 * Destroys a font's internal data
 */
static void _TTF_DestroyData( _TTF_font_t font )
{
	GL_FreeImage( font->texture );
	qglDeleteLists( font->list_id , TTF_CHARACTERS );
	Z_Free( font->bitmap );
}


/*
 * Destroys a font's data structure
 */
static void _TTF_Destroy( FNT_font_t font )
{
	_TTF_font_t	fInternal = (_TTF_font_t) font->internal;
	_TTF_DestroyData( fInternal );
	Z_Free( fInternal );
}


/*
 * Check a font's flags and re-render it if that is necessary.
 */
static qboolean _TTF_CheckFlags( FNT_font_t font )
{
	_TTF_font_t	fInternal = (_TTF_font_t) font->internal;
	unsigned int	flags = TTF_COMPUTE_FLAGS( );
	_TTF_CheckCVars( );
	if ( flags != fInternal->flags ) {
		_TTF_DestroyData( fInternal );
		return _TTF_LoadFont( font );
	}
	return true;
}


static qboolean _TTF_GetFont( FNT_font_t font )
{
	// Allocate internal memory and load font
	font->internal = Z_Malloc( sizeof( struct _TTF_font_s ) );
	_TTF_CheckCVars( );
	if ( ! _TTF_LoadFont( font ) ) {
		// Failed - free resources
		Z_Free( font->internal );
		font->internal = NULL;
		return false;
	}

	font->RawPrint = _TTF_RawPrint;
	font->BoundedPrint = _TTF_BoundedPrint;
	font->WrappedPrint = _TTF_WrappedPrint;
	font->PredictSize = _TTF_PredictSize;
	font->Destroy = _TTF_Destroy;
	return true;
}




/*****************************************************************************
 * PRINTING FUNCTIONS: INTERNALS                                             *
 *****************************************************************************/

/*
 * Prepares the environment before drawing a string.
 */
static void _TTF_PrepareToDraw( image_t * texture )
{
	// Save current context
	qglPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT );
	qglMatrixMode( GL_MODELVIEW );
	qglPushMatrix( );
	qglMatrixMode( GL_TEXTURE );
	qglPushMatrix( );

	// Prepare texture
	GL_Bind( texture->texnum );
	GL_TexEnv( GL_MODULATE );
	qglLoadIdentity();
	qglScaled( 1.0 / texture->width , 1.0 / texture->height , 1 );
	qglMatrixMode( GL_MODELVIEW );

	// Set blending function. NOTE: we do NOT use GLSTATE_ENABLE/DISABLE
	// macros or GL_BlendFunction because we're using glPushAttrib/glPopAttrib
	// to handle that state instead.
	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglBlendFunc (GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
}


/*
 * Restores the environment as if was before _TTF_PrepareToDraw was called
 */
static void _TTF_RestoreEnvironment( )
{
	qglMatrixMode( GL_TEXTURE );
	qglPopMatrix( );
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix( );
	qglPopAttrib( );
}


/*
 * Internal raw printing function; assumes the GL environment is properly set up.
 */
static void _TTF_RawPrintInternal( 
	_TTF_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y )
{
	const unsigned char *	ptr = ( const unsigned char *) text;
	int			ptrInc = r2l ? -1 : 1;
	int			previous = -1;
	
	if (r2l)
		ptr += text_length-1;

	while ( text_length ) {
		unsigned int	i = ( *ptr & 0x7F ) - ' ';

		if ( i < TTF_CHARACTERS ) {
			float	tx;
			if ( previous == -1 ) {
				tx = x;
				if ( r2l ) {
					tx -= font->widths[ i ] + font->kerning[ i ][ 0 ];
				}
			} else if ( r2l ) {
				tx = - ( font->widths[ i ] + font->kerning[ i ][ previous ] );
			} else {
				tx = font->widths[ previous ] + font->kerning[ previous ][ i ];
			}

			qglTranslatef( tx , y , 0 );
			qglCallList( font->list_id + i );

			y = 0;
			previous = i;
		}

		ptr += ptrInc;
		text_length --;
	}

}


/*
 * "Fix" an array of wrapped unit by changing characters to TTF font indexes,
 * computing widths and kernings, and removing non-printable characters.
 */
static void _TTF_FixWrappedUnit(
		const _TTF_font_t	font ,
		_FNT_render_info_t	renderInfo ,
		unsigned int *		riLength )
{
	unsigned int	riSkipped , riIndex;

	// Remove non-printable characters and set width and kerning information
	for ( riSkipped = riIndex = 0 ; riIndex < *riLength ; riIndex ++ ) {
		int value = ( renderInfo[ riIndex ].toDraw & 0x7f ) - ' ';

		// Check for characters to skip
		if ( value < 0 || value >= TTF_CHARACTERS ) {
			riSkipped ++;
			continue;
		}

		// If characters were skipped, copy color
		if ( riSkipped != 0 ) {
			renderInfo[ riIndex - riSkipped ].color =  renderInfo[ riIndex ].color;
		}

		// Set modified data for character
		renderInfo[ riIndex - riSkipped ].toDraw = value;
		renderInfo[ riIndex - riSkipped ].width = font->widths[ value ];
		if ( riIndex - riSkipped == 0 ) {
			renderInfo[ riIndex - riSkipped ].kerning = 0;
		} else {
			renderInfo[ riIndex - riSkipped ].kerning = font->kerning[ renderInfo[ riIndex - riSkipped - 1 ].toDraw ][ value ];
		}
	}
	*riLength -= riSkipped;
}



/*
 * Function that prints a string until either the string ends, a newline is
 * found or a boundary is reached.
 */
static void _TTF_PrintUntilEOL(
		_TTF_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		const float *	color ,
		unsigned int *	width ,
		int *		read ,
		const float **	curColor
	)
{
	const char *	ptr		= text;
	unsigned int	cWidth		= 0;
	qboolean	expectColor	= false;
	qboolean	colorChanged	= true;
	int		previous	= -1;
	int		txTotal		= 0;

	while ( *ptr ) {
		int		current , charWidth , tx;

		// Handle color codes
		if ( ! _FNT_HandleColor( &ptr , cmode , color , &expectColor , &colorChanged , curColor ) ) {
			continue;
		}

		// Found EOL
		current = ( *ptr & 0x7f );
		ptr ++;
		if ( current == '\n' ) {
			break;
		}

		current -= ' ';
		if ( current < 0 || current >= TTF_CHARACTERS ) {
			continue;
		}

		// Other character; draw it and update width
		charWidth = font->widths[ current ];
		tx = 0;
		if ( previous != -1 ) {
			tx = font->kerning[ previous ][ current ];
			charWidth += tx;
			tx += font->widths[ previous ];
		}

		if ( *width && cWidth + charWidth > *width ) {
			break;
		}

		if ( colorChanged ) {
			qglColor4fv( *curColor );
			colorChanged = false;
		}
		if ( tx ) {
			txTotal += tx;
			qglTranslatef( tx , 0 , 0 );
		}
		qglCallList( font->list_id + current );
		cWidth += charWidth;
		previous = current;
	}

	*read = ptr - text;
	*width = cWidth;
}



/*
 * Internal function for bounded printing when alignment is left.
 * Assumes the GL environment is ready.
 */
static void _TTF_BoundedPrintLeft( 
		_TTF_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		FNT_window_t	box ,
		const float *	color
	)
{
	const char *	ptr		= text;
	unsigned int	cHeight		= 0;
	unsigned int	maxWidth	= 0;

	qglTranslatef( box->x , box->y , 0 );
	while ( *ptr && ( box->height == 0 || cHeight + font->height < box->height ) ) {
		const float *	curColor	= color;
		unsigned int	lineWidth	= box->width;
		int		read;

		// Print current line
		qglPushMatrix( );
		qglTranslatef( 0 , (float) cHeight , 0 );
		_TTF_PrintUntilEOL( font , ptr , cmode , color , &lineWidth , &read , &curColor );
		qglPopMatrix( );

		// Update height and maximal width
		cHeight += font->height;
		if ( lineWidth > maxWidth ) {
			maxWidth = lineWidth;
		}

		// Find end of line
		ptr += read - 1;
		while ( *ptr ) {
			char c = *ptr;
			ptr ++;
			if ( c == '\n' ) {
				break;
			}
		}
	}

	box->width = maxWidth;
	box->height = cHeight;
}


/*
 * Draw a set of characters from a render information array.
 */
static void _TTF_DrawFromInfo(
		const _TTF_font_t			font ,
		const struct _FNT_render_info_s *	start ,
		unsigned int				length ,
		int					lineWidth ,
		int					boxX ,
		unsigned int				boxW ,
		unsigned int				y ,
		unsigned int				align ,
		unsigned int				indent )
{
	const float *		curColor = NULL;
	int			x = 0;
	int			cx;

	// Do not draw stuff outside of the screen
	if ( y + font->height < 0 || y >= viddef.height ) {
		return;
	}

	// Determine starting location
	switch ( align ) {
		case FNT_ALIGN_LEFT:
			x = boxX + indent;
			break;
		case FNT_ALIGN_RIGHT:
			x = boxX + boxW - indent - lineWidth;
			break;
		case FNT_ALIGN_CENTER:
			x = boxX + ( boxW - lineWidth ) / 2;
			break;
	}

	// Do not draw stuff outside of the screen, again
	if ( x + lineWidth < 0 || x >= viddef.width ) {
		return;
	}

	// Display each character
	qglPushMatrix( );
	cx = 0;
	while ( length ) {
		if ( !y ) {
			x += start->kerning;
		}
		qglTranslatef( (float) x , (float) y , 0 );
		cx += x;

		if ( cx >= viddef.width ) {
			break;
		}

		if ( cx + start->width > 0 ) {
			if ( curColor != start->color ) {
				qglColor4fv( curColor = start->color );
			}
			qglCallList( font->list_id + start->toDraw );
		}

		x = start->width;
		y = 0;

		length --;
		start ++;
	}
	qglPopMatrix( );
}



// Used by BoundedPrintInternal and WrappedPrintInternal: most of the time a
// static buffer can be used rather than a dynamically allocated one.
#define HUGE_STR_CUTOFF 4096
static struct _FNT_render_info_s static_renderInfo[HUGE_STR_CUTOFF];

/*
 * Internal function for bounded printing.
 * Assumes the GL environment is ready.
 */
static void _TTF_BoundedPrintInternal( 
		_TTF_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	)
{
	qboolean				mustQuit	= false;
	const char *			ptr			= text;
	unsigned int			cHeight		= 0;
	unsigned int			nHeight		= 0;
	unsigned int			maxWidth	= 0;
	qboolean				huge;
	_FNT_render_info_t		renderInfo;
	
	huge = strlen (text) >= HUGE_STR_CUTOFF;
	
	if (huge)
	{
		renderInfo = Z_Malloc (strlen (text) * sizeof(*renderInfo));
	}
	else
	{
		memset (static_renderInfo, 0, sizeof(static_renderInfo));
		renderInfo = static_renderInfo;
	}

	while ( ! mustQuit && ( box->height == 0 || cHeight + font->height < box->height ) ) {
		unsigned int	riLength , riIndex;
		int		lWidth , sWidth , eWidth;
		int		sIndex , eIndex;

		mustQuit = ! _FNT_NextWrappedUnit( &ptr , renderInfo , &riLength , cmode , color );
		_TTF_FixWrappedUnit( font , renderInfo , &riLength );

		// Skip empty lines
		if ( riLength == 0 ) {
			nHeight += font->height;
			continue;
		}
		cHeight += nHeight;
		nHeight = 0;

		// Compute total width
		lWidth = 0;
		for ( riIndex = 0 ; riIndex < riLength ; riIndex ++ ) {
			lWidth += renderInfo[ riIndex ].width + renderInfo[ riIndex ].kerning;
		}

		// Compute string widths between which characters should be printed
		if ( lWidth <= box->width ) {
			sWidth = 0;
			eWidth = lWidth;
			sIndex = 0;
			eIndex = riLength - 1;
		} else {
			sWidth = lWidth - box->width;
			if ( align == FNT_ALIGN_CENTER ) {
				sWidth /= 2;
			}
			eWidth = sWidth + box->width;

			// Now we need to find the actual width of whatever it is we're going to print
			lWidth = 0;
			sIndex = eIndex = -1;
			for ( riIndex = 0 ; riIndex < riLength ; riIndex ++ ) {
				int nWidth = renderInfo[ riIndex ].width + renderInfo[ riIndex ].kerning;

				if ( lWidth >= sWidth && sIndex == -1 ) {
					sIndex = riIndex;
					sWidth = lWidth;
				}
				if ( lWidth + nWidth > eWidth && eIndex == -1 ) {
					eIndex = riIndex;
					eWidth = lWidth;
					break;
				}
				lWidth += nWidth;
			}
			if ( sIndex == -1 ) {
				continue;
			}
			if ( eIndex == -1 ) {
				eIndex = riLength - 1;
				eWidth = lWidth;
			}
		}

		lWidth = eWidth - sWidth;
		if ( lWidth > maxWidth ) {
			maxWidth = lWidth;
		}
		
		_TTF_DrawFromInfo( font , renderInfo + sIndex , 1 + eIndex - sIndex , lWidth ,
			box->x , box->width , box->y + cHeight , align , 0 );
		cHeight += font->height;
	}

	box->width = maxWidth;
	box->height = cHeight;
	
	if (huge)
		Z_Free (renderInfo);
}


/*
 * Draws a line of text from a render information array while performing wrapping.
 */
static void _TTF_WrapFromInfo(
		const _TTF_font_t		font ,
		const _FNT_render_info_t	renderInfo ,
		unsigned int			length ,
		unsigned int			align ,
		unsigned int			indent ,
		const FNT_window_t		box ,
		unsigned int *			curHeight ,
		unsigned int *			maxWidth )
{
	unsigned int	curIndex	= 0;
	qboolean	lineStarted	= false;
	unsigned int	nLines		= 0;
	unsigned int	mlWidth		= box->width;

	int		lWidth;
	int		wWidth;
	unsigned int	lsIndex;
	unsigned int	wsIndex;

	while ( curIndex < length ) {
		const struct _FNT_render_info_s *	current;
		int					cWidth;
		current = &( renderInfo[ curIndex ++ ] );

		// Handle start of lines
		if ( ! lineStarted ) {
			lineStarted = true;
			lWidth = wWidth = 0;
			wsIndex = lsIndex = curIndex - 1;

			nLines ++;
			if ( nLines == 2 ) {
				mlWidth -= indent;
			}

			if ( current->toDraw == 0 ) {
				wsIndex ++;
				continue;
			}
		}

		// Compute character width
		cWidth = current->width;
		if ( lWidth != 0 ) {
			cWidth += current->kerning;
		}

		if ( lWidth + cWidth > mlWidth ) {
			// We need to draw
			if ( lWidth == wWidth ) {
				// That word fills the whole line
				_TTF_DrawFromInfo( font , renderInfo + lsIndex , curIndex - ( 1 + lsIndex ) , lWidth ,
					box->x , box->width , box->y + *curHeight , align , ( nLines == 1 ) ? 0 : indent );
				curIndex --;
			} else {
				// Draw from line start to word start
				lWidth -= wWidth;
				_TTF_DrawFromInfo( font , renderInfo + lsIndex , wsIndex - lsIndex , lWidth ,
					box->x , box->width , box->y + *curHeight , align , ( nLines == 1 ) ? 0 : indent );
				curIndex = wsIndex;
			}
			if ( lWidth > *maxWidth ) {
				*maxWidth = lWidth;
			}

			*curHeight += font->height;
			lineStarted = false;
		} else {
			// Not drawing yet
			lWidth += cWidth;
			if ( current->toDraw == 0 ) {
				wsIndex = curIndex;
				wWidth = 0;
			} else {
				wWidth += cWidth;
			}
		}
	}

	// Draw rest of the line if required
	if ( lineStarted && lWidth ) {
		_TTF_DrawFromInfo( font , renderInfo + lsIndex , curIndex - lsIndex , lWidth ,
			box->x , box->width , box->y + *curHeight , align , ( nLines == 1 ) ? 0 : indent );
		*curHeight += font->height;
		if ( lWidth > *maxWidth ) {
			*maxWidth = lWidth;
		}
	}
}



/*
 * Wrapped printing implementation ; assumes OpenGL settings are in place.
 */
static void _TTF_WrappedPrintInternal(
		const _TTF_font_t	font ,
		const char *		text ,
		unsigned int		cmode ,
		unsigned int		align ,
		unsigned int		indent ,
		FNT_window_t		box ,
		const float *		color
	)
{
	qboolean				mustQuit	= false;
	const char *			curText		= text;
	unsigned int			curHeight	= 0;
	unsigned int			nextHeight	= 0;
	unsigned int			maxWidth	= 0;
	qboolean				huge;
	_FNT_render_info_t		renderInfo;
	
	huge = strlen (text) >= HUGE_STR_CUTOFF;
	
	if (huge)
	{
		renderInfo = Z_Malloc (strlen (text) * sizeof(*renderInfo));
	}
	else
	{
		memset (static_renderInfo, 0, sizeof(static_renderInfo));
		renderInfo = static_renderInfo;
	}

	assert( box->width > indent );
	while ( !mustQuit && ( box->height == 0 || curHeight + font->height < box->height ) ) {
		unsigned int			riLength;

		mustQuit = ! _FNT_NextWrappedUnit( &curText , renderInfo , &riLength , cmode , color );
		_TTF_FixWrappedUnit( font , renderInfo , &riLength );

		// Skip empty lines
		if ( riLength == 0 ) {
			nextHeight += font->height;
			continue;
		}

		// Print line
		curHeight += nextHeight;
		nextHeight = 0;
		_TTF_WrapFromInfo( font , renderInfo , riLength , align , indent , box , &curHeight , &maxWidth );
	}

	// Update box
	box->height = curHeight;
	box->width = maxWidth;
	if (huge)
	Z_Free (renderInfo);
}



/*****************************************************************************
 * PRINTING FUNCTIONS AVAILABLE THROUGH FRONT-END                            *
 *****************************************************************************/

/*
 * Raw printing function for external use
 */
static void _TTF_RawPrint(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] )
{
	_TTF_font_t	fInternal = (_TTF_font_t) font->internal;

	if ( ! _TTF_CheckFlags( font ) )
		return;

	_TTF_PrepareToDraw( fInternal->texture );
	qglColor4fv( color );
	_TTF_RawPrintInternal( fInternal , text , text_length , r2l , x , y );
	_TTF_RestoreEnvironment( );
}


/*
 * Bounded printing function
 */
static void _TTF_BoundedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	)
{
	_TTF_font_t	fInternal = (_TTF_font_t) font->internal;

	if ( ! _TTF_CheckFlags( font ) )
		return;

	_TTF_PrepareToDraw( fInternal->texture );
	if ( align == FNT_ALIGN_LEFT || box->width == 0 ) {
		_TTF_BoundedPrintLeft( fInternal , text , cmode , box , color );
	} else {
		_TTF_BoundedPrintInternal( fInternal , text , cmode , align , box , color );
	}
	_TTF_RestoreEnvironment( );
}


/*
 * Wrapped printing function
 */
static void _TTF_WrappedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	)
{
	_TTF_font_t	fInternal = (_TTF_font_t) font->internal;
	assert( box->width > 0 );

	if ( ! _TTF_CheckFlags( font ) )
		return;

	_TTF_PrepareToDraw( fInternal->texture );
	_TTF_WrappedPrintInternal( fInternal , text , cmode , align , indent , box , color );
	_TTF_RestoreEnvironment( );
}


/*
 * Size prediction function
 */
static int _TTF_PredictSize(
		FNT_font_t	font ,
		const char *	text,
		qboolean		color
	)
{
	_TTF_font_t				fInternal;
	float					tx;
	int						ret;
	unsigned int			previous;
	const unsigned char *	ptr;
	
	fInternal = (_TTF_font_t) font->internal;
	ptr = ( const unsigned char *) text;
	
	if (*ptr == '\0')
		return 0;

	if (*ptr == '^' && color)
	{
		if ( *(ptr + 1) == '\0' )
			return 1; /* "^" only */
		if ( *(ptr + 2) == '\0' )
			return 0; /* a color escape only */
		ptr += 2;
	}
	
	previous = ( *ptr & 0x7F ) - ' ';
	
	tx = fInternal->widths[ previous ];
	
	while ( *(++ptr) ) {
		unsigned int	i = ( *ptr & 0x7F ) - ' ';
		
		if (*ptr == '^' && color)
		{
			ptr++;
			continue;
		}

		if ( i < TTF_CHARACTERS ) {
			tx += fInternal->kerning[ previous ][ i ] + fInternal->widths[ i ];
			previous = i;
		}
	}
	
	// always round up the pixel count
	ret = (int)tx;
	if (ret < tx)
		ret++;
	
	return ret;
}
