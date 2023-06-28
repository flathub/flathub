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
#ifndef __R_TEXT_H
#define __R_TEXT_H

/* Forward-declare the face structure and its pointer type */
struct FNT_face_s;
typedef struct FNT_face_s * FNT_face_t;

/* Forward-declare the font structure and its pointer type */
struct FNT_font_s;
typedef struct FNT_font_s * FNT_font_t;

/* Forward-declare the text window structure and its pointer type */
struct FNT_window_s;
typedef struct FNT_window_s * FNT_window_t;



/**************************************************************************/
/* CONSTANTS                                                              */
/**************************************************************************/

/* Maximal length of a font file's base name */
#define FNT_FACE_NAME_MAX		48

/* Maximal length of a font key ("font name!font size") */
#define FNT_FONT_KEY_MAX		( FNT_FACE_NAME_MAX + 11 )

/* Color modes */
#define FNT_CMODE_NONE			0	// Ignore color codes
#define FNT_CMODE_QUAKE			1	// Use Quake color codes
#define FNT_CMODE_QUAKE_SRS		2	// Use Quake color codes, but
						// reset to default color when
						// space is found
#define FNT_CMODE_TWO			3	// Use two colors, with bit 7
						// of the string's characters
						// to indicate the second one;
						// Quake color codes will be
						// stripped out.

/* Text alignment */
#define FNT_ALIGN_LEFT			0
#define FNT_ALIGN_RIGHT			1
#define FNT_ALIGN_CENTER		2

/* Color table for Quake colors */
extern float FNT_colors[ 8 ][ 4 ];



/**************************************************************************/
/* FUNCTION TYPES FOR FACES AND FONTS                                     */
/**************************************************************************/


/*
 * Face destruction function
 *
 * Parameters:
 *	face		the face to destroy
 */
typedef void (* FNT_DestroyFace_funct)( FNT_face_t face );


/*
 * Font loader function - used by faces when initialising a font.
 *
 * Parameters:
 *	font		a font structure where everything except the
 *			internal structure pointer has been initialised
 *
 * Returns:
 *	true on success, false on failure
 */
typedef qboolean (* FNT_GetFont_funct )( FNT_font_t font );


/*
 * Raw printing function.
 *
 * Used to draw some text at a specific location using a fixed colour.
 *
 * Parameters:
 *	font		the font to draw text with
 *	text		pointer to the string to draw
 *	text_length	length of the string to draw
 *	r2l		false for left-to-right, true for right-to-left
 *	x , y		coordinates to start from
 *	color		(R,G,B,A) colour to apply when drawing
 *
 * Notes:
 *	1/ All control or non-ASCII characters will be skipped.
 *	2/ No checks will be made regarding the string's end, only the
 *		specified length will be used.
 *	3/ In right-to-left mode, the string will be read backwards.
 */
typedef void (* FNT_RawPrint_funct )(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] );


/*
 * Bounded printing function
 *
 * Draws a string in a region; does not perform wrapping, everything that is
 * out of the specified box is ignored.
 *
 * Parameters:
 *	font		the font to use
 *	text		the text to draw
 *	cmode		the color coding to use
 *	align		the text's alignment
 *	box		the text's window
 *	color		the color (or colors) to use
 *
 * Notes:
 *	1/ The initial window must include a valid width.
 *	2/ In "two colors" mode, the "color" parameter should point to an
 *		array of 8 floats; in other modes, only 4 floats are needed.
 *	3/ The window's width and height will be set to the actual space used
 *		by the text.
 */
typedef void (* FNT_BoundedPrint_funct)(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	);


/*
 * Printing function that supports word-wrapping.
 *
 * Draws a string in a region and performs wrapping as appropriate. If the
 * region's height is not set to 0, stop printing when it is reached.
 *
 * Parameters:
 *	font		the font to use
 *	text		the text to draw
 *	cmode		the color coding to use
 *	align		the text's alignment
 *	indent		the size of the indent when a line is wrapped
 *	box		the text's window
 *	color		the color (or colors) to use
 *
 * Notes:
 *	1/ The initial window must include a valid width.
 *	2/ In "two colors" mode, the "color" parameter should point to an
 *		array of 8 floats; in other modes, only 4 floats are needed.
 *	3/ The window's width and height will be set to the actual space used
 *		by the text.
 */
typedef void (* FNT_WrappedPrint_funct)(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	);


/*
 * Font destruction function
 *
 * Parameters:
 *	font		the font to destroy
 */
typedef void (* FNT_DestroyFont_funct)( FNT_font_t font );


/*
 * Size prediction function
 *
 * Determine how long the given text would be if drawn in the given font.
 * Assumes no wrapping or boundaries-- it could well return a number greater
 * than the horizontal resolution of the display.
 *
 * Parameters:
 *	font		the font to use
 *	text		the text to use
 *  color       enable ^color code filtering
 *
 * Returns:
 *  The width in pixels required to render the text in the font
 */
typedef int (* FNT_PredictSize_funct)(
		FNT_font_t	font ,
		const char *	text,
		qboolean        color
	);



/**************************************************************************/
/* STRUCTURES FOR FACES AND FONTS                                         */
/**************************************************************************/

/*
 * This structure corresponds to a "face" - a type of font, with no
 * specifics such as size.
 */
struct FNT_face_s
{
	/* The face's name */
	char			name[ FNT_FACE_NAME_MAX + 1 ];

	/* The function that allows fonts to be initialised */
	FNT_GetFont_funct	GetFont;

	/* Destructor */
	FNT_DestroyFace_funct	Destroy;

	/* Amount of fonts using this face */
	unsigned int		used;

	/* Internal data used by the actual font renderer */
	void *			internal;
};


/*
 * This structure corresponds to a generic font.
 */
struct FNT_font_s
{
	/* The font's look-up key */
	char			lookup[ FNT_FONT_KEY_MAX + 1 ];

	/* The font's face */
	FNT_face_t		face;

	/* The font's height in pixels */
	unsigned int		size;
	/* total line height in pixels, including spacing */
	unsigned int		height;
	/* theoretical maximum character width in pixels, given current size */
	unsigned int		width; 

	/* Printing functions */
	FNT_RawPrint_funct	RawPrint;
	FNT_BoundedPrint_funct	BoundedPrint;
	FNT_WrappedPrint_funct	WrappedPrint;
	FNT_PredictSize_funct	PredictSize;

	/* Destructor */
	FNT_DestroyFont_funct	Destroy;

	/* Amount of "unfreed" GetFont's for this font */
	unsigned int		used;

	/* Internal data used by the actual font renderer */
	void *			internal;
};


/*
 * This structure is used when drawing text, it carries coordinates/region size
 * between the caller and the text drawing function.
 */
struct FNT_window_s
{
	int			x;
	int			y;
	unsigned int		width;
	unsigned int		height;
};


/*
 * This structure is used by wrapped printing functions, which use
 * _FNT_NextWrappedUnit() to transform a string into an array of
 * render information.
 */
struct _FNT_render_info_s
{
	int			toDraw;
	const float *		color;
	int			width;
	int			kerning;
};
typedef struct _FNT_render_info_s * _FNT_render_info_t;


/*
 * This structure is used for registration of auto-loaded fonts
 */
struct FNT_auto_s
{
	/* List pointers */
	struct FNT_auto_s *	previous;
	struct FNT_auto_s *	next;

	/* Face to use / default face to use if CVar is set but there is
	 * no such face. */
	char			face[ FNT_FACE_NAME_MAX + 1 ];

	/* CVar that replaces the default face, or NULL if the default
	 * is always used. */
	cvar_t *		faceVar;

	/* Default font size. May be <= 0 if the size is to be computed
	 * from the screen's size. */
	int			size;

	/* CVar that replaces the default size, or NULL if the default
	 * is always used. */
	cvar_t *		sizeVar;

	/* When automatic size computation is used, this indicates the
	 * amount of lines of text on the screen. */
	unsigned int		lines;

	/* The minimal size that can be selected, either automatically
	 * or through the variable. */
	unsigned int		minSize;

	/* The maximal size that can be selected, either automatically
	 * or through the variable. */
	unsigned int		maxSize;

	/* The current font used. May be NULL if unregistered, or may be
	 * the Null Font. */
	FNT_font_t		font;
};
typedef struct FNT_auto_s * FNT_auto_t;



/**************************************************************************/
/* FONT FUNCTIONS                                                         */
/**************************************************************************/


/*
 * Initialise the text drawing front-end.
 *
 * Returns:
 *	true on success, false on failure.
 */
qboolean FNT_Initialise( );


/*
 * Destroy the text drawing front-end.
 */
void FNT_Shutdown( );


/*
 * Create or access a face.
 *
 * Parameters:
 *	face_name	name of the face's definition file
 *
 * Returns:
 *	a pointer to the face's structure, or NULL if the face could not
 *	be loaded.
 *
 * Notes:
 *	face_name corresponds to a file name; its path and extension are
 *	ignored: faces should be in the "fonts" directory, and they should
 *	be either TrueType fonts or bitmap fonts. If both files exist, the
 *	TrueType font will be used.
 */
FNT_face_t FNT_GetFace( const char * face_name );


/*
 * Create or access a font.
 *
 * Parameters:
 *	face		the face to use to create the font
 *	size		the font's desired height in pixels
 *
 * Returns:
 *	a pointer to the font's structure, or NULL if the font could not
 *	be initialised.
 */
FNT_font_t FNT_GetFont( FNT_face_t face , unsigned int size );


/*
 * Release a font
 *
 * This function needs to be called when a font is no longer needed by some
 * part of the program.
 *
 * It will decrease the font's use count and, if necessary, destroy it.
 * If the face is no longer in use, it will be freed as well.
 *
 * Parameters:
 *	font		the font to release
 */
void FNT_ReleaseFont( FNT_font_t font );


/*
 * Print a raw string. See comment on FNT_RawPrint_funct for more information.
 */
#define FNT_RawPrint(font,text,text_length,r2l,x,y,color) \
	( font->RawPrint( font,text,text_length,r2l,x,y,color ) )


/*
 * Print a string bounded by a box. See comment on FNT_BoundedPrint_funct for
 * more information.
 */
#define FNT_BoundedPrint(font,text,cmode,align,box,color) \
	( font->BoundedPrint( font , text , cmode , align , box , color ) )


/*
 * Print a string bounded by a box using word wrapping. See comment on
 * FNT_WrappedPrint_funct for more information.
 */
#define FNT_WrappedPrint(font,text,cmode,align,indent,box,color) \
	( font->WrappedPrint( font , text , cmode , align , indent , box , color ) )


/*
 * Determine how much space is required to print the string. See comment on
 * FNT_PredictSize_funct for more information.
 */
#define FNT_PredictSize(font,text,color) \
	( font->PredictSize( font , text , color ) )



/**************************************************************************/
/* AUTOMATIC FONT MANAGEMENT                                              */
/**************************************************************************/

/*
 * Initialise an automatic font structure.
 *
 * Parameters:
 *	auto_font	automatic font structure to initialise
 *	default_face	name of the default font face
 *	default_size	default size of the font
 *	auto_lines	amount of lines of text on the screen in automatic
 *			size mode
 *	min_size	minimal font size
 *	max_size	maximal font size
 */
void FNT_AutoInit( 
		FNT_auto_t	auto_font ,
		const char *	default_face ,
		int		default_size ,
		unsigned int	auto_lines ,
		unsigned int	min_size ,
		unsigned int	max_size
	);


/*
 * Register an automatic font.
 *
 * Parameters:
 *	auto_font	automatic font structure to register
 *
 * Notes:
 *	This function will either ignore the structure or crash, depending
 *	on whatever's in memory, if the structure hasn't been initialised.
 */
void FNT_AutoRegister(
		FNT_auto_t	auto_font
	);


/*
 * Access the font from an automatic font structure.
 *
 * If the automatic font uses CVars and one of the variables has been
 * modified, or if the game went through a video restart, the font
 * will be reloaded automatically.
 *
 * Parameters:
 *	auto_font	automatic font structure to access
 *
 * Returns:
 *	The font described by the automatic font structure.
 *
 * Notes:
 *	This function will probably crash (or at least yield undefined
 *	results) if the automatic font structure hasn't been registered.
 */
FNT_font_t FNT_AutoGet(
		FNT_auto_t	auto_font
	);


/**************************************************************************/
/* HELPER FUNCTIONS                                                       */
/**************************************************************************/

/*
 * Handle color codes in strings.
 *
 * This function finds color codes, allowing them to be skipped when rendering
 * strings, while setting the current color vector accordingly.
 *
 * Parameters:
 *	pptr		pointer to the text pointer, will be updated
 *			depending on what is found
 *	cmode		color mode
 *	color		default color vector(s) passed to the rendering
 *			function
 *	expectColor	pointer to a boolean which is used to keep track
 *			of Quake escape characters
 *	colorChanged	pointer to a boolean which will be set to true
 *			if the color has been changed
 *	curColor	pointer to a color vector pointer, will be updated
 *			to the new color if needed
 *
 * Returns:
 *	true if the next character is to be rendered, false if it must be
 *	skipped
 */
qboolean _FNT_HandleColor(
		const char **	pptr ,
		unsigned int	cmode ,
		const float *	color ,
		qboolean *	expectColor ,
		qboolean *	colorChanged ,
		const float **	curColor
	);

/*
 * Find the next wrapped unit (i.e. line or rest of the string until EOL).
 *
 * Parameters:
 *	pptr		pointer to the text pointer, will be updated to the
 *			character that follows the wrapped unit (i.e. after
 *			\n or \0)
 *	renderInfo	start of the rendering information array
 *	unitLength	pointer to an integer which will be updated to the
 *			amount of items in the rendering information array
 *	cmode		color mode
 *	color		default color vector(s)
 *
 * Returns:
 *	false if the end of the string has been reached, false otherwise
 */
qboolean _FNT_NextWrappedUnit(
		const char **		pptr ,
		_FNT_render_info_t	renderInfo ,
		unsigned int *		unitLength ,
		unsigned int		cmode ,
		const float *		color
	);



#endif // __R_TEXT_H
