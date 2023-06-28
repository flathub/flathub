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


/* r_text.c
 *
 * Generic "front" for text rendering.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <qcommon/htable.h>
#include "r_local.h"

#include "r_text.h"
#include "r_ttf.h"



/* Face lookup table. */
static hashtable_t	_FNT_LoadedFaces = NULL;

/* Font lookup table. */
static hashtable_t	_FNT_LoadedFonts = NULL;


/* Color table */
float	FNT_colors[ 8 ][ 4 ] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
};




/**************************************************************************/
/* NULL FONT                                                              */
/**************************************************************************/

/* Null font */
static struct FNT_font_s	_FNT_NullFontStruct;
static FNT_font_t		_FNT_NullFont = NULL;


static void _FNT_NullFont_Destroy( FNT_font_t font )
{
	Com_Error( ERR_FATAL , "FNT_NullFont_Destroy called - this is a bug" );
}


static void _FNT_NullFont_RawPrint(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] )
{
	/* EMPTY */
}


static void _FNT_NullFont_BoundedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	)
{
	/* EMPTY */
}


static void _FNT_NullFont_WrappedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	)
{
	/* EMPTY */
}


static int _FNT_NullFont_PredictSize(
		FNT_font_t	font ,
		const char *	text,
		qboolean color
	)
{
	/* EMPTY */
	return 0;
}



/**************************************************************************/
/* AUTOMATIC FONT MANAGEMENT                                              */
/**************************************************************************/

/* List head for automated fonts */
static struct FNT_auto_s	_FNT_AutoListHead;
static FNT_auto_t		_FNT_AutoList = NULL;

/* Initialise an automatic font structure. */
void FNT_AutoInit( 
		FNT_auto_t	auto_font ,
		const char *	default_face ,
		int		default_size ,
		unsigned int	auto_lines ,
		unsigned int	min_size ,
		unsigned int	max_size )
{
	assert( auto_font != NULL );
	assert( default_face != NULL );
	assert( auto_lines > 8 );
	assert( min_size != 0 );
	assert( max_size > min_size );
	assert( default_size == 0 || default_size > min_size );

	auto_font->next = auto_font->previous = auto_font;

	Q_strncpyz2( auto_font->face , default_face , sizeof( auto_font->face ) );
	auto_font->faceVar = NULL;

	auto_font->size = default_size;
	auto_font->sizeVar = NULL;
	auto_font->lines = auto_lines;
	auto_font->minSize = min_size;
	auto_font->maxSize = max_size;
}


/* Register an automatic font. */
void FNT_AutoRegister(
		FNT_auto_t	auto_font
	)
{
	assert( auto_font != NULL );

	if ( _FNT_AutoList == NULL ) {
		_FNT_AutoListHead.next = _FNT_AutoListHead.previous = _FNT_AutoList = &_FNT_AutoListHead;
	}

	if ( auto_font->previous != auto_font->next || auto_font->previous != auto_font ) {
		return;
	}

	auto_font->previous = _FNT_AutoList->previous;
	auto_font->next = _FNT_AutoList;
	auto_font->next->previous = auto_font->previous->next = auto_font;

	auto_font->font = &_FNT_NullFontStruct;
}


/* Access the font from an automatic font structure. */
FNT_font_t FNT_AutoGet(
		FNT_auto_t	auto_font
	)
{
	int		size;
	FNT_face_t	face;
	qboolean	fvMod , svMod;

	assert( _FNT_AutoList != NULL );
	assert( auto_font->previous != auto_font );

	fvMod = auto_font->faceVar && auto_font->faceVar->modified;
	svMod = auto_font->sizeVar && auto_font->sizeVar->modified;

	// Do we need to load a font?
	if ( auto_font->font != _FNT_NullFont ) {
		if ( ! ( fvMod || svMod ) ) {
			return auto_font->font;
		}
		FNT_ReleaseFont( auto_font->font );
	}

	// Mark other automatic fonts for reload if we have modified variables
	if ( fvMod || svMod ) {
		FNT_auto_t current = auto_font->next;

		while ( current != auto_font ) {
			if ( current != _FNT_AutoList && current->font != _FNT_NullFont
					&& ( ( fvMod && auto_font->faceVar == current->faceVar )
						|| ( svMod && auto_font->sizeVar == current->sizeVar ) ) ) {
				FNT_ReleaseFont( current->font );
				current->font = _FNT_NullFont;
			}
			current = current->next;
		}
	}

	// Load face
	if ( auto_font->faceVar ) {
		face = FNT_GetFace( auto_font->faceVar->string );
		auto_font->faceVar->modified = false;
	} else {
		face = NULL;
	}
	if ( face == NULL ) {
		// fallback font
		face = FNT_GetFace( auto_font->face );
	}

	// Get or compute font size
	if ( auto_font->sizeVar ) {
		size = auto_font->sizeVar->integer;
		auto_font->sizeVar->modified = false;
	} else {
		size = auto_font->size;
	}
	if ( size <= 0 ) {
		size = viddef.height / auto_font->lines;
	}
	if ( size < auto_font->minSize ) {
		size = auto_font->minSize;
	} else if ( size > auto_font->maxSize ) {
		size = auto_font->maxSize;
	}

	return ( auto_font->font = FNT_GetFont( face , size ) );
}



/**************************************************************************/
/* FONT FUNCTIONS                                                         */
/**************************************************************************/

/*
 * Initialise lookup tables for the text rendering front-end.
 */
qboolean FNT_Initialise( )
{
	if ( _FNT_LoadedFaces != NULL )
		return true;

	// Set up Null font if necessary
	if ( _FNT_NullFont == NULL ) {
		_FNT_NullFont = &_FNT_NullFontStruct;
		memset( _FNT_NullFont , 0 , sizeof( struct FNT_font_s ) );
		_FNT_NullFont->Destroy = _FNT_NullFont_Destroy;
		_FNT_NullFont->RawPrint = _FNT_NullFont_RawPrint;
		_FNT_NullFont->BoundedPrint = _FNT_NullFont_BoundedPrint;
		_FNT_NullFont->WrappedPrint = _FNT_NullFont_WrappedPrint;
		_FNT_NullFont->PredictSize = _FNT_NullFont_PredictSize;
	}

	// Create hash tables for both faces and fonts
	_FNT_LoadedFaces = HT_Create( 100 , HT_FLAG_INTABLE ,
			sizeof( struct FNT_face_s ) , 0 , FNT_FACE_NAME_MAX );
	_FNT_LoadedFonts = HT_Create( 400 , HT_FLAG_INTABLE ,
			sizeof( struct FNT_font_s ) , 0 , FNT_FONT_KEY_MAX );

	// Initialise TrueType engine
	if ( ! TTF_Initialise( ) ) {
		HT_Destroy( _FNT_LoadedFonts );
		HT_Destroy( _FNT_LoadedFaces );
		return false;
	}

	return true;
}


/*
 * Function used to destroy fonts
 */
static qboolean _FNT_DestroyAllFonts( void * item , void * extra )
{
	FNT_font_t font = (FNT_font_t) item;
	font->Destroy( font );
	return true;
}


/*
 * Function used to destroy faces
 */
static qboolean _FNT_DestroyAllFaces( void * item , void * extra )
{
	FNT_face_t face = (FNT_face_t) item;
	face->Destroy( face );
	return true;
}


/*
 * Destroy the text drawing front-end
 */
void FNT_Shutdown( )
{
	FNT_auto_t	item;

	if ( _FNT_LoadedFaces == NULL )
		return;

	// Reset all automatic fonts to the Null font if necessary
	if ( _FNT_AutoList != NULL ) {
		item = _FNT_AutoList->next;
		while ( item != _FNT_AutoList ) {
			item->font = _FNT_NullFont;
			item = item->next;
		}
	}

	// Destroy all existing fonts and faces
	HT_Apply( _FNT_LoadedFonts , _FNT_DestroyAllFonts , NULL );
	HT_Destroy( _FNT_LoadedFonts );
	_FNT_LoadedFonts = NULL;
	HT_Apply( _FNT_LoadedFaces , _FNT_DestroyAllFaces , NULL );
	HT_Destroy( _FNT_LoadedFaces );
	_FNT_LoadedFaces = NULL;

	// Shutdown TTF engine
	TTF_Shutdown( );
}



/*
 * Create or access a face.
 */ 
FNT_face_t FNT_GetFace( const char * face_name )
{
	qboolean	created;
	FNT_face_t	face;
	const char *	bnStart;
	const char *	bnEnd;
	const char *	bnPtr;
	size_t		bnSize;
	char		baseName[ FNT_FACE_NAME_MAX + 1 ];

	// Find the argument's file name
	bnStart = bnPtr = face_name;
	while ( *bnPtr ) {
		if ( *bnPtr == '/' || *bnPtr == '\\' )
			bnStart = bnPtr + 1;
		bnPtr ++;
	}
	bnEnd = bnPtr;
	while ( bnPtr > bnStart && *bnPtr != '.' ) {
		bnPtr --;
	}
	if ( *bnPtr == '.' )
		bnEnd = bnPtr;
	bnSize = bnEnd - bnStart;
	if ( bnSize > FNT_FACE_NAME_MAX ) {
		Com_DPrintf( "FNT: face name from '%s' is too long\n" , face_name );
		return NULL;
	} else if ( bnSize == 0 ) {
		Com_DPrintf( "FNT: face name from '%s' is empty\n" , face_name );
		return NULL;
	}
	Q_strncpyz2( baseName , bnStart , bnSize + 1 );

	// Try looking it up first
	face = HT_GetItem( _FNT_LoadedFaces , baseName , &created );
	if ( created ) {
		// Changed from 0 to 1. An initial value of 0 was causing face to be
		// used after being freed.
		face->used = 1;
	} else {
		face->used ++;
		return face;
	}

	// Try loading it as a TrueType font
	if ( TTF_InitFace( face ) )
		return face;

	// Could not load
	Com_Printf( "FNT: could not load face '%s'\n" , baseName );
	HT_DeleteItem( _FNT_LoadedFaces , baseName , NULL );
	return NULL;
}


/*
 * Access an actual font
 */
FNT_font_t FNT_GetFont( FNT_face_t face , unsigned int size )
{
	char		fontId[ FNT_FONT_KEY_MAX + 1 ];
	FNT_font_t	font;
	qboolean	created;

	if ( face == NULL ) {
		return _FNT_NullFont;
	}

	// Try looking it up
	Com_sprintf( fontId , sizeof( fontId ) , "%s|%lu" , face->name , size );
	font = HT_GetItem( _FNT_LoadedFonts , fontId , &created );

	if ( created ) {
		// Not found - initialise it.
		font->used = 1;
		font->face = face;
		font->size = size;
		if ( ! face->GetFont( font ) ) {
			HT_DeleteItem( _FNT_LoadedFonts , fontId , NULL );
			font = _FNT_NullFont;
		}
	} else {
		font->used ++;
	}

	return font;
}


/*
 * Release a font
 */
void FNT_ReleaseFont( FNT_font_t font )
{
	FNT_face_t face;

	if ( font == NULL || font == _FNT_NullFont ) {
		return;
	}

	font->used --;
	if ( font->used ) {
		return;
	}

	face = font->face;
	font->Destroy( font );
	HT_DeleteItem( _FNT_LoadedFonts , font->lookup , NULL );
	face->used --;
	if ( face->used ) {
		return;
	}

	face->Destroy( face );
	HT_DeleteItem( _FNT_LoadedFaces , face->name , NULL );
}



/**************************************************************************/
/* HELPER FUNCTIONS                                                       */
/**************************************************************************/

/*
 * Function that handles color codes in strings.
 */
qboolean _FNT_HandleColor(
		const char **	pptr ,
		unsigned int	cmode ,
		const float *	color ,
		qboolean *	expectColor ,
		qboolean *	colorChanged ,
		const float **	curColor
	)
{
	const char *	ptr;
	char		current;

	if ( cmode == FNT_CMODE_NONE ) {
		return true;
	}

	ptr = *pptr;
	current = *ptr;
	if ( cmode == FNT_CMODE_TWO ) {
		// Two-color mode
		if ( ( current & 0x80 ) == 0 && *curColor != color ) {
			*curColor = color;
			*colorChanged = true;
		} else if ( ( current & 0x80 ) != 0 && *curColor == color ) {
			*curColor = &( color[ 4 ] );
			*colorChanged = true;
		}
	}

	// Quake color codes
	current &= 0x7f;
	if ( *expectColor ) {
		if ( current == '\n' ) {
			(*pptr) --;
			return false;
		}

		*expectColor = false;
		if ( current != '^' ) {
			if ( cmode != FNT_CMODE_TWO ) {
				*curColor = FNT_colors[ ( current - '0' ) & 7 ];
				*colorChanged = true;
			}
			(*pptr) ++;
			return false;
		}
	} else if ( *ptr == '^' ) {
		*expectColor = true;
		(*pptr) ++;
		if ( ! **pptr ) {
			(*pptr) --;
		}
		return false;
	} else if ( cmode == FNT_CMODE_QUAKE_SRS && *ptr == ' ' ) {
		*curColor = color;
		*colorChanged = true;
	}

	return true;
}



/*
 * Find the next wrapped unit (i.e. line or rest of the string until EOL).
 */
qboolean _FNT_NextWrappedUnit(
		const char **		pptr ,
		_FNT_render_info_t	renderInfo ,
		unsigned int *		unitLength ,
		unsigned int		cmode ,
		const float *		color )
{
	const float *		curColor = color;
	qboolean		expectColor = false;
	qboolean		colorChanged;
	char			previous = ' ';
	int			index = 0;

	while ( **pptr && **pptr != '\n' ) {
		char		curChar;

		// Handle color codes
		if ( ! _FNT_HandleColor( pptr , cmode , color , &expectColor , &colorChanged , &curColor ) ) {
			continue;
		}

		// Skip extra spaces
		curChar = *(*pptr) ++;

		// Add character to info array
		renderInfo[ index ].toDraw = curChar;
		renderInfo[ index ].color = curColor;
		index ++;

		previous = curChar;
	}

	if ( **pptr ) {
		(*pptr) ++;
	}
	*unitLength = index;
	return ( **pptr != 0 );
}
