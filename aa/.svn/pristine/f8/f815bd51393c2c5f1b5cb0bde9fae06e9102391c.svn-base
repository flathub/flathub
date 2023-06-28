/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2009 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/*
 * snd_file.c
 *
 * .wav and .ogg (vorbis) file support for OpenAL version
 *
 * Reference:
 *   Vorbisfile API Reference (vorbisfile version 1.2.0 - 20070723)
 *   from: http://www.xiph.org/vorbis/doc/vorbisfile/index.html
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vorbis/vorbisfile.h>

#include "client.h"

typedef struct
{
	int rate;
	int width;
	int channels;
//	int loopstart;
	int samples;
	int dataofs; // chunk starts this many bytes from file start
} pcminfo_t;

/* return 0 for little endian, 1 for big endian */
int  bigendian( void )
{
	long one = 1;
	return !(*((char*)(&one)));
}

/*
 ==
 .WAV file parsing
 ==
 */

byte *data_p;
byte *iff_end;
byte *last_chunk;
byte *iff_data;
int iff_chunk_len;

short GetLittleShort( void )
{
	short val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	data_p += 2;
	return val;
}

int GetLittleLong( void )
{
	int val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	val = val + ( *( data_p + 2 ) << 16 );
	val = val + ( *( data_p + 3 ) << 24 );
	data_p += 4;
	return val;
}

void FindNextChunk( char *name )
{
	while( 1 )
	{
		data_p = last_chunk;

		if( data_p >= iff_end )
		{ // didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if( iff_chunk_len < 0 )
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		if( !strncmp( (const char *)data_p, name, 4 ) )
			return;
	}
}

void FindChunk( char *name )
{
	last_chunk = iff_data;
	FindNextChunk( name );
}

/*
 ============
 ReadWavFile
 ============
 */
qboolean ReadWavFile( const char *name, byte *wav, int wavlength,
        pcminfo_t *info )
{
	int format;
	int samples;

	memset( info, 0, sizeof(pcminfo_t) );

	if( !wav )
		return false;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk( "RIFF" );
	if( !( data_p && !strncmp( (const char *)data_p + 8, "WAVE", 4 ) ) )
	{
		Com_DPrintf( "%s: Missing RIFF/WAVE chunks\n", name );
		return false;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;

	FindChunk( "fmt " );
	if( !data_p )
	{
		Com_DPrintf( "%s: Missing fmt chunk\n", name );
		return false;
	}

	data_p += 8;
	format = GetLittleShort();
	if( format != 1 ) // format 1 is PCM data,
	{
		Com_DPrintf( "%s: audio format %i not supported\n", name, format );
		return false;
	}

	info->channels = GetLittleShort();
	info->rate = GetLittleLong();
	data_p += 4 + 2;

	info->width = GetLittleShort();
	if( info->width != 8 && info->width != 16 )
	{
		Com_DPrintf( "%s: %i bits-per-sample not supported\n", name, info->width );
		return false;
	}
	info->width /= 8;

	// "cue " chunk handling removed, no need for it with OpenAL

	// find data chunk
	FindChunk( "data" );
	if( !data_p )
	{
		Com_DPrintf( "%s: Missing data chunk\n", name );
		return false;
	}
	data_p += 4;

	samples = GetLittleLong() / info->width;
	info->samples = samples;
	info->dataofs = data_p - wav;

	if ( (info->width == 2) && bigendian() )
	{ /* for big-endian byte swap little-endian 16-bit samples */
		short *psample = (short*)data_p;
		while ( samples-- )
		{
			*psample++ = GetLittleShort(); /* note: increments data_p */
		}
	}

	return true;
}

/*
 * Ogg/Vorbis Buffer I/O
 */
/*
typedef struct {
	  size_t (*read_func)  (void *ptr, size_t size, size_t nmemb, void *datasource);
	  int    (*seek_func)  (void *datasource, ogg_int64_t offset, int whence);
	  int    (*close_func) (void *datasource);
	  long   (*tell_func)  (void *datasource);
	} ov_callbacks;
*/

typedef struct ovbfr_s
{
	byte *pdata;
	size_t offset;
	size_t length;
} ovbfr_t;

size_t ovbfr_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	size_t bytes_remaining;
	size_t byte_count;
	size_t nmemb_read;
	ovbfr_t *pbfr = (ovbfr_t *)datasource;

	byte_count = size * nmemb;
	bytes_remaining = pbfr->length - pbfr->offset;
	nmemb_read = nmemb;
	if ( bytes_remaining < byte_count )
	{
		nmemb_read = bytes_remaining / size ;
		byte_count = size * nmemb_read;
	}
	memcpy( ptr, pbfr->pdata + pbfr->offset , byte_count );
	pbfr->offset += byte_count;

	return nmemb_read;
}

int ovbfr_seek(void *datasource, ogg_int64_t offset, int whence)
{
	ovbfr_t *pbfr = (ovbfr_t *)datasource;

	switch( whence )
	{
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += pbfr->offset;
		break;
	case SEEK_END:
		offset += pbfr->length;
		break;
	default:
		return -1;
		break;
	}

	if( offset < 0 || offset > pbfr->length )
		return -1;
	pbfr->offset = offset;
	return 0;
}

/* No need for a 'fclose' function
 int ovbfr_close(void *datasource)
{
	ovbfr_t *pbfr = (ovbfr_t *)datasource;

	return 0;
}
*/

long ovbfr_tell( void *datasource )
{
	ovbfr_t *pbfr = (ovbfr_t *)datasource;

	return pbfr->offset;
}

// function pointers for Ogg Vorbis file-like io
ov_callbacks ovbfr_callbacks = { ovbfr_read, ovbfr_seek, NULL, ovbfr_tell };


/*
 ============
 ReadVorbisFile
 ============
 */
qboolean ReadVorbisFile( const char *name, byte *pdata, int filelength,
        pcminfo_t *info, byte **pcmbfr )
{
	int result;
	ogg_int64_t samples;
	vorbis_info *vi;
	OggVorbis_File vf;
	ovbfr_t ovbfr;
	size_t pcmbfr_size;
	char *pcmbfr_pdata;
	char *pdest;
	size_t bytes_remaining;
	long read_result;
	int bitstream;
	int bigendianp;

	Com_DPrintf("ReadVorbisFile: %s\n", name );

	memset( info, 0, sizeof(pcminfo_t) ); // clear return data
	*pcmbfr = NULL;

	info->width = 2; // always 16-bit

	ovbfr.pdata = pdata; // open the Ogg Vorbis file i/o buffer
	ovbfr.offset = 0;
	ovbfr.length = filelength;

	result = ov_open_callbacks( &ovbfr, &vf, NULL, 0, ovbfr_callbacks );
	Com_DPrintf("..result: %i\n", result);

	vi = ov_info( &vf, -1 );
	info->channels = vi->channels;
	info->rate = vi->rate;
	Com_DPrintf("..channels: %i, rate: %li\n", vi->channels, vi->rate );

	samples = ov_pcm_total( &vf, -1 );
	info->samples = samples;
	Com_DPrintf("..samples: %li\n", samples );

	bytes_remaining = pcmbfr_size = info->samples * info->channels * info->width;
	if ( pcmbfr_size > 0 )
	{
		pdest = pcmbfr_pdata = (char *)Z_Malloc( pcmbfr_size );
		if ( pdest == NULL )
		{ // unable to allocate buffer for decoded pcm data
			Com_DPrintf("Memory allocation error: decode buffer for %s.\n", name );
			result = ov_clear( &vf ); // close Ogg Vorbis read/decode
			return false;
		}
	}
	else
	{
		Com_DPrintf("File size or format error: %s\n", name );
		result = ov_clear( &vf ); // close Ogg Vorbis read/decode
		return false;
	}

	bigendianp = bigendian();
	do
	{ // decode Ogg Vorbis data to host endian, 16-bit, signed PCM data
		read_result = ov_read( &vf, pdest, bytes_remaining, bigendianp, 2, 1, &bitstream );
		if ( read_result > 0 )
		{
			bytes_remaining -= (size_t)read_result;
			pdest += read_result;
		}
		else
		{
			switch( read_result )
			{
			case 0: // premature end of file
				Com_DPrintf("ov_read(): file size error, %s \n", name );
				break;
			case OV_HOLE: // interruption in data
				Com_DPrintf("ov_read(): OV_HOLE error, %s \n", name );
				break;
			case OV_EBADLINK: // invalid stream section, or corrupt data
				Com_DPrintf("ov_read(): OV_EBADLINK error, %s \n", name );
				break;
			default:
				Com_DPrintf("ov_read(): invalid return value, %s\n", name );
				break;
			}
			Z_Free( pcmbfr_pdata );
			result = ov_clear( &vf ); // close Ogg Vorbis read/decode
			return false;
		}
	}
	while ( bytes_remaining > 0 );

	result = ov_clear( &vf ); // close Ogg Vorbis read/decode
	*pcmbfr = (byte *)pcmbfr_pdata; // return the decoded buffer
	return true;
}

/*
 ==
 S_LoadSound()

 creates a buffer containing PCM data, 8- or 16-bit, mono or stereo.
 stereo samples are left-channel first

 OUTPUTS:
 readbfr : the whole file, caller must FS_FileFree() this
 pcmdata : start of PCM data, for transfer to OpenAL buffer
 channels : 1=mono, 2=stereo
 bytewidth : 1=8-bit, 2=16-bit
 samplerate : samples-per-second (aka, frequency)
 byte_count : number of bytes of PCM data
 ==
 */
qboolean S_LoadSound( char *filename, // in
		qboolean ogg_substitute, // in
        void** filebfr, // out
        void** pcmbfr, // out
        int *bytewidth, // out
        int *channels, // out
        int *samplerate, // out
        size_t *byte_count // out
)
{
	byte *data; // for buffer allocated by FS_LoadFile
	byte *decoded_data; // for buffer allocated by ReadVorbisFile
	int size;
	pcminfo_t info;
	qboolean success;
	char *pfile_ext;

	// clear return values
	*filebfr = NULL;
	*pcmbfr = NULL;
	*channels = 0;
	*bytewidth = 0;
	*samplerate = 0;
	*byte_count = 0;

	if( ogg_substitute )
	{ 	// .wav => .ogg if .ogg exists
		if ( !Q_strncasecmp(
				(pfile_ext = &filename[ strlen(filename)-4 ]),
				".wav", 4) )
		{ // look for .ogg alternative, by mangling the filename
			strncpy( pfile_ext, ".ogg", 4 );
			if ( !FS_FileExists( filename ) )
			{ // no .ogg alternative, unmangle
				strncpy( pfile_ext, ".wav", 4 );
			}
		}
	}

	// load sound file from game directories
	size = FS_LoadFile( (char*)filename, (void **) &data );
	if( !data )
	{
		Com_DPrintf( "Could not load %s\n", filename );
		return false;
	}

	if ( !Q_strncasecmp( &filename[ strlen(filename)-4 ], ".ogg", 4) )
	{
		success = ReadVorbisFile( filename, data, size, &info, &decoded_data );
		FS_FreeFile( data ); // free the file image buffer
		if ( !success )
		{
			Com_DPrintf("Could not decode %s\n", filename );
			return false;
		}
		// return values
		*filebfr = decoded_data;
		*pcmbfr = decoded_data;
		// return values
		*channels = info.channels;
		*bytewidth = info.width;
		*samplerate = info.rate;
		*byte_count = info.samples * info.channels * info.width;
	}
	else
	{
		success = ReadWavFile( filename, data, size, &info );
		if( !success )
		{
			FS_FreeFile( data ); // discard sound data
			return false;
		}
		// return values
		*filebfr = data;
		*pcmbfr = data + info.dataofs;
		// return values
		*channels = info.channels;
		*bytewidth = info.width;
		*samplerate = info.rate;
		*byte_count = info.samples * info.width;
	}

	return true;
}
