#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

// Contains the routines for loading images into buffers
// TODO: maybe move the rest of the image formats here?

static void raw_sample (const byte *texture, int tex_w, int tex_h, int s, int t, vec4_t out)
{
	int i;
	
	if (s >= tex_w)
		s = tex_w-1;
	if (s < 0)
		s = 0;
	if (t >= tex_h)
		t = tex_h-1;
	if (t < 0)
		t = 0;
	
	for (i = 0; i < 4; i++)
		out[i] = (float)texture[(t*tex_w+s)*4+i]/255.0f;
}

// Used for sampling textures on the CPU.
void bilinear_sample (const byte *texture, int tex_w, int tex_h, float u, float v, vec4_t out)
{
	int i;
 	int x, y;
 	float u_ratio, v_ratio, u_opposite, v_opposite;
 	vec4_t sample1, sample2, sample3, sample4;
 	
	u = u * tex_w - 0.5;
	v = v * tex_h - 0.5;
	x = floor(u);
	y = floor(v);
	u_ratio = u - x;
	v_ratio = v - y;
	u_opposite = 1 - u_ratio;
	v_opposite = 1 - v_ratio;
	
#define SAMPLE(x,y,out) raw_sample (texture, tex_w, tex_h, x, y, out)
	SAMPLE (x, y, sample1);
	SAMPLE (x+1, y, sample2);
	SAMPLE (x, y+1, sample3);
	SAMPLE (x+1, y+1, sample4);
#undef SAMPLE
	
	for (i = 0; i < 4; i++)
		out[i] =	(sample1[i] * u_opposite + sample2[i] * u_ratio) * v_opposite +
					(sample3[i] * u_opposite + sample4[i] * u_ratio) * v_ratio;
}

//just a guessed size-- this isn't necessarily raw RGBA data, it's the
//encoded image data.
#define	STATIC_RAWDATA_SIZE	(1024*1024*4+256)
static byte	static_rawdata[STATIC_RAWDATA_SIZE];

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void LoadTGA (const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	size_t	buf_end;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile_TryStatic	(	name, (void **)&buffer, 
										static_rawdata, STATIC_RAWDATA_SIZE);
	if (!buffer)
	{
		return;
	}

	buf_p = buffer;
	buf_end = (size_t)(buffer+length);

#define GET_TGA_BYTE(dest) \
	{\
		if ((size_t)buf_p<buf_end)\
		{\
			(dest) = *buf_p++;\
		}\
		else\
		{\
			/* We don't set *pic to NULL, so if the image is mostly intact,
			 * whatever pixels were already there will still show up.
			 */\
			Com_Printf ("LoadTGA: %s is either truncated or corrupt, please obtain a fresh copy!\n", name);\
			return;\
		}\
	}

	GET_TGA_BYTE (targa_header.id_length);
	GET_TGA_BYTE (targa_header.colormap_type);
	GET_TGA_BYTE (targa_header.image_type);

	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (targa_header.colormap_size);
	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.x_origin = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.y_origin = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.width = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (tmp[0]);
	GET_TGA_BYTE (tmp[1]);
	targa_header.height = LittleShort ( *((short *)tmp) );
	GET_TGA_BYTE (targa_header.pixel_size);
	GET_TGA_BYTE (targa_header.attributes);

	if (targa_header.image_type!=2
		&& targa_header.image_type!=10) {
		//Com_Error (ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");
		if (buffer != static_rawdata)
			FS_FreeFile (buffer);
		return;
	}

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24)) {
		//Com_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
		if (buffer != static_rawdata)
			FS_FreeFile (buffer);
		return;
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	memset (targa_rgba, 0, numPixels*4);
	*pic = targa_rgba;
	
	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte=255;
				GET_TGA_BYTE (blue);
				GET_TGA_BYTE (green);
				GET_TGA_BYTE (red);
				if (targa_header.pixel_size == 32)
				{
					GET_TGA_BYTE (alphabyte);
				}
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alphabyte;
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red=0,green=0,blue=0,alphabyte=255,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				GET_TGA_BYTE (packetHeader);
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					GET_TGA_BYTE (blue);
					GET_TGA_BYTE (green);
					GET_TGA_BYTE (red);
					if (targa_header.pixel_size == 32)
						GET_TGA_BYTE (alphabyte);

					for(j=0;j<packetSize;j++) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						GET_TGA_BYTE (blue);
						GET_TGA_BYTE (green);
						GET_TGA_BYTE (red);
						if (targa_header.pixel_size == 32)
							GET_TGA_BYTE (alphabyte);
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
			}
			breakOut:;
		}
	}

	if (buffer != static_rawdata)
		FS_FreeFile (buffer);
}
