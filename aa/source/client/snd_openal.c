/*
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
 * snd_openal.c
 *
 * Reference:
 *   OpenAL 1.1 Specification and Reference (June 2005)
 *   OpenAL Programmer's Guide : OpenAL Version 1.1 (June 2007)
 *   OpenAL Deployment Guide (PC Windows) Version 0.16 (June 2007)
 *	 http://www.openal.org (at creativelabs.com)
 *	 http://kcat.strangesoft.net/openal.html  (OpenAL Soft)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if defined HAVE_MALLOC_H
#include <malloc.h>
#elif defined HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif
#include <string.h>

#if defined HAVE_AL_H
#include <al.h>
#include <alc.h>
#elif defined HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined HAVE_OPENAL_AL_H
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "client.h"
#include "qal.h"

/*
 * Sound CVARS
 */
#define MAX_SRC 128
#define MAX_SRC_DEFAULT 96
#define MIN_SRC_DEFAULT 64
/*--- previous system ---*/
// These 2 are defined in cl_main.c
//cvar_t *background_music; //     enable/disable music
//cvar_t *background_music_vol; // music volume setting
cvar_t *s_initsound; //   in cfg or on command line, to disable sound
cvar_t *s_volume; //      global volume setting
/*--- OpenAL additions ---*/
cvar_t *s_doppler; //     0=doppler off, non-zero Doppler Factor
cvar_t *s_maxsources; //  upper limit on generated Sources
cvar_t *s_minsources; //  lower limit on generated Sources
cvar_t *s_device; //      name of device to use
/*--- debug ---*/
cvar_t *snd_developer; // for debug printf's

/*
 * Sound System State Control
 */
qboolean sound_system_enable;
qboolean s_registering; // loading pre-cache sound effects in "batch" mode
int s_registration_sequence; // id for current "batch"

/*
 * Sound FX & OpenAL Buffer
 */
typedef struct sfx_s
{
	void *backlink; //           link to the sfx_link node
	qboolean silent; //          for when sound effect file is missing
	qboolean bg_music; //        this is background music
	int registration_sequence; // used for culling stale sfx's from pre-cache
	int byte_width; //           1=8-bit, 2=16-bit
	int channels; //             1=mono, 2=stereo,
	int samplerate; //           samples-per-sec, aka Hz
	size_t byte_count; //        size of the sound data
	void *filebfr; //            intermediate buffer for file reading
	void *pcmbfr; //             where PCM data is in filebfr
	qboolean buffered; //        data has been read into OpenAL buffer
	int oalFormat; //            file format code from OpenAL
	ALuint oalBuffer; //         index into collection of OpenAL buffers
	qboolean aliased; //         the file loaded is an alias for the truename
	char name[MAX_QPATH]; //     relative file path
	char truename[MAX_QPATH]; // in case 'name' is an 'alias'
} sfx_t;

typedef struct sfxlink_s // node for the sfx_t lists
{
	struct sfxlink_s *prev;
	struct sfxlink_s *next;
	sfx_t *sfx;
} sfxlink_t;

#define MAX_SFX 256
sfxlink_t *sfx_datahead;
sfxlink_t *sfx_freehead;
sfxlink_t sfx_link[MAX_SFX];
sfx_t sfx_data[MAX_SFX];
int actual_sfx_count;

/*
 * OpenAL Sources and related data
 */
typedef enum loop_mark_e
{
	lpmk_nonloop, lpmk_start, lpmk_continue, lpmk_stop
} loop_mark_t;

typedef enum velocity_state_e
{
	velst_nodoppler, velst_update, velst_init
} velocity_state_t;

typedef struct src_s
{
	void *backlink; //        link to the src_link node
	int start_timer; //       for delayed start
	int stop_timer; //        for delayed forced stop
	int entnum; //            the entity identifier, index into cl_entities[]
	int entchannel; //        the "channel", AUTO, WEAP, etc.
	qboolean fixed_origin; // non-moving point implied by non-NULL origin arg
	int attn_class; //        attenuation classification ATTN_NORM, etc.
	qboolean looping; //      for AL_LOOPING Sources
	loop_mark_t loop_mark; // for finding "automatic" looping sounds
	int entity_index; //      for CL_GetEntitySoundOrigin()
	int sound_field; //       entity_state_t .sound
	velocity_state_t velocity_state; //velocity tracking for doppler
	sfx_t *sfx; //            the associated sound effect
	ALuint oalSource; //      the OpenAL Source name/id
} src_t;

typedef struct srclink_s // node for the src_t lists
{
	struct srclink_s *prev;
	struct srclink_s *next;
	src_t *src;
} srclink_t;

srclink_t *src_datahead;
srclink_t *src_freehead;
srclink_t src_link[MAX_SRC];
src_t src_data[MAX_SRC];
int actual_src_count; //      number of sources generated
int source_counter; //        current number of sources in use
int max_sources_used; //      for tracking max number of sources used
int source_failed_counter; // for counting source alloc failures
int max_sources_failed;

// single dedicated background sound, stereo enabled
// "Your only Source for Alien Arena Music"
// on/off switch is cvar:  background_music
// volume control is cvar: background_music_volume
extern cvar_t *background_music_vol; // missing in header file
struct music_s
{
	qboolean playing;
	sfx_t *sfx;
	ALuint oalSource;
} music;

/*
 * Defaults for OpenAL Sources, Listener
 */
const ALfloat zero_position[3] =
{ 0.0f, 0.0f, 0.0f };
const ALfloat zero_velocity[3] =
{ 0.0f, 0.0f, 0.0f };
const ALfloat default_orientation[6] =
{ 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
const ALfloat zero_direction[3] =
{ 0.0f, 0.0f, 0.0f };

/*
 * Settings for OpenAL context
 *
 * The Exponent Distance Clamped Model:
 *  distance = max(distance, AL_REFERENCE_DISTANCE)
 *  distance = min(distance, AL_MAX_DISTANCE)
 *  gain = (distance / AL_REFERENCE_DISTANCE) ^ (- AL_ROLLOFF_FACTOR)
 *
 * Doppler:
 *  speed_of_sound : lower for more doppler effect
 *  doppler_factor : higher for more doppler effect
 *
 * Velocities are in QuakeMapUnits-per-millisecond
 * Just FYI:
 *  if 8 QuakeMapUnits == 1 foot, 9/8 feet == 1.125 ft
 *  1.125 feet-per-millisec == 1125 feet-per-second
 *  Speed of Sound is 1125 feet-per-second, 343 meters-per-second
 *
 *  Doppler defaults to OFF, because lower performance systems
 *    might have problems
 */
const ALenum distance_model = AL_EXPONENT_DISTANCE_CLAMPED;
const ALfloat default_doppler_factor = (ALfloat)0.0f;
const ALfloat maximum_doppler_factor = (ALfloat)5.0f;
const ALfloat speed_of_sound = (ALfloat)9.0f;
const ALfloat velocity_max_valid = (ALfloat)( 3.0f * 3.0f );
ALfloat doppler_factor;

const struct source_default_s
{
	// Source properties
	ALfloat pitch;
	ALfloat gain;
	ALfloat looping_gain;
	ALfloat max_distance;
	ALfloat rolloff_factor;
	ALfloat reference_distance;
	ALfloat idle_max_distance;
	ALfloat idle_rolloff_factor;
	ALfloat idle_reference_distance;
	ALfloat static_max_distance;
	ALfloat static_rolloff_factor;
	ALfloat static_reference_distance;
	ALfloat looping_max_distance;
	ALfloat looping_rolloff_factor;
	ALfloat looping_reference_distance;
	ALfloat min_gain;
	ALfloat max_gain;
	ALfloat cone_outer_gain;
	ALfloat cone_inner_angle;
	ALfloat cone_outer_angle;
	// int stop_timer_delay;
} source_default =
{
		(ALfloat)1.0f, //    AL_PITCH
        (ALfloat)1.0f, //    AL_GAIN
        (ALfloat)0.7f, //    AL_GAIN for auto loop sounds
        // ATTN_NORM:
        (ALfloat)3200.0f, // AL_MAX_DISTANCE (ATTN_NORM)
        (ALfloat)1.5f, //    AL_ROLLOFF_FACTOR (ATTN_NORM)
        (ALfloat)320.0f, //  AL_REFERENCE_DISTANCE (ATTN_NORM)
        // ATTN_IDLE:
        (ALfloat)2800.0f, // AL_MAX_DISTANCE (ATTN_IDLE)
        (ALfloat)2.0f, //    AL_ROLLOFF_FACTOR (ATTN_IDLE)
        (ALfloat)280.0f, //  AL_REFERENCE_DISTANCE (ATTN_IDLE)
        // ATTN_STATIC:
        (ALfloat)1600.0f, // AL_MAX_DISTANCE (ATTN_STATIC)
        (ALfloat)2.2f, //    AL_ROLLOFF_FACTOR (ATTN_STATIC)
        (ALfloat)160.0f, //  AL_REFERENCE_DISTANCE (ATTN_STATIC)
        // "Auto Looping"
        (ALfloat)1600.0f, // AL_MAX_DISTANCE (Looping)
        (ALfloat)1.8f, //    AL_ROLLOFF_FACTOR (Looping)
        (ALfloat)160.0f, //  AL_REFERENCE_DISTANCE (Looping)
        //
        (ALfloat)0.0f, //    AL_MIN_GAIN
        (ALfloat)1.0f, //    AL_MAX_GAIN
        (ALfloat)0.0f, //    AL_CONE_OUTER_GAIN
        (ALfloat)360.0f, //  AL_CONE_INNER_ANGLE
        (ALfloat)360.0f //   AL_CONE_OUTER_ANGLE
 };

/*
 * Settings for Distance Culling
 *
 * - OpenAL does not cull by distance, just scales gain. Its up to
 *   the application to cull.
 * - Today's theory is that the cull distance should take into account
 *   longest line-of-sight paths in maps, so visible events are heard.
 *   And, then, depend mostly on the distance attenuation model to
 *   keep things reasonable.
 */
const ALfloat norm_sq_cull_distance = (ALfloat)( 3296.0f * 3296.0f );
const ALfloat idle_sq_cull_distance = (ALfloat)( 2864.0f * 2864.0f );
const ALfloat static_sq_cull_distance = (ALfloat)( 1664.0f * 1664.0f );
const ALfloat looping_sq_cull_distance = (ALfloat)( 1664.0f * 1664.0f );
const ALfloat looping_cull_hysteresis = (ALfloat)( 64.0f * 64.0f );

/*
 * OpenAL Device
 */
#define MAX_OAL_DEVICES 16
const ALCchar* oalDeviceList; // pointer to OpenAL's nul-separated device list
int oalDeviceCount; // number of valid devices
ALCchar* pDeviceNames[MAX_OAL_DEVICES]; // pointers to valid devices in list

/*
 * Other settings
 */
const int stop_timer_msecs = 750; // for src_t stop_timer field

/* alBufferData( bfrid, AL_MONO16, pcmNil, sizeof(pcmNil), 44100 );
 *  should allow OpenAL to free pcm data
 */
unsigned short pcmNil[] =
{ 0 };

/*
==
 Snd_DPrintf()

  debug printf when "snd_developer" cvar is set
  does not print messages where level > snd_developer cvar value
  use 1 for "normal" developer error/status messages,
   >1 for higher rate event monitoring, for example.

  Note: the usual Com_DPrintf() sometimes floods the console output
==
*/
void Snd_DPrintf( int level, char *fmt, ... )
{
	va_list		argptr;
	char		msg[256];

	if( !snd_developer || !snd_developer->value || level > snd_developer->value )
		return;

	va_start( argptr, fmt );
	vsnprintf(msg, sizeof(msg), fmt, argptr );
	va_end (argptr);

	Com_Printf ("%s", msg);
}

/*
==
 calErrorCheckAL()
==
*/
ALenum calErrorCheckAL( int line_no )
{
	ALenum error_code;

	error_code = qalGetError();

	switch( error_code )
	{
	case AL_NO_ERROR:
		break;
	case AL_INVALID_NAME:
		Snd_DPrintf( 1,"OpenAL Error: AL_INVALID_NAME (%i)\n", line_no );
		break;
	case AL_INVALID_ENUM:
		Snd_DPrintf( 1, "OpenAL Error: AL_INVALID_ENUM (%i)\n", line_no );
		break;
	case AL_INVALID_VALUE:
		Snd_DPrintf( 1, "OpenAL Error: AL_INVALID_VALUE (%i)\n", line_no );
		break;
	case AL_INVALID_OPERATION:
		Snd_DPrintf( 1, "OpenAL Error: AL_INVALID_OPERATION (%i)\n", line_no );
		break;
	case AL_OUT_OF_MEMORY:
		Snd_DPrintf( 1, "OpenAL Error: AL_OUT_OF_MEMORY (%i)\n", line_no );
		break;
	default:
		Snd_DPrintf( 1, "OpenAL Error: AL Code = %i (%#X) (%i)\n", error_code,
		        error_code, line_no );
		break;
	}

	return error_code;
}

/*
==
 calErrorCheckALC()
==
*/
ALCenum calErrorCheckALC( ALCdevice *device, int line_no )
{
	ALCenum error_code;

	error_code = qalcGetError( device );

	switch( error_code )
	{
	case ALC_NO_ERROR:
		break;
	case ALC_INVALID_DEVICE:
		Snd_DPrintf( 1, "OpenALC Error: ALC_INVALID_DEVICE (%i)\n", line_no );
		break;
	case ALC_INVALID_CONTEXT:
		Snd_DPrintf( 1, "OpenALC Error: ALC_INVALID_CONTEXT (%i)\n", line_no );
		break;
	case ALC_INVALID_ENUM:
		Snd_DPrintf( 1, "OpenALC Error: ALC_INVALID_ENUM (%i)\n", line_no );
		break;
	case ALC_INVALID_VALUE:
		Snd_DPrintf( 1, "OpenALC Error: ALC_INVALID_VALUE (%i)\n", line_no );
		break;
	case ALC_OUT_OF_MEMORY:
		Snd_DPrintf( 1, "OpenALC Error: ALC_OUT_OF_MEMORY (%i)\n", line_no );
		break;
	default:
		Snd_DPrintf( 1, "OpenALC Error: ALC Code = %i (%#X) (%i)\n", error_code,
		        error_code, line_no );
		break;
	}

	return error_code;
}

/*
 ==
 calSourcesInitialize()
 calSourceAlloc()
 calSourceFree()

 generate and list manage the collection of OpenAL Sources and related info
 ==
 */
void calSourcesInitialize( ALint device_sources )
{
	int ix;
	int last;
	ALuint gensrc[MAX_SRC];
	ALint sources_n;
	char cvarset[16];

	// init Source lists to empty
	src_datahead = NULL;
	src_freehead = NULL;

	source_counter = 0; // gather statistics about Source use
	max_sources_used = 0;
	source_failed_counter = 0;
	max_sources_failed = 0;

	// generate a collection of OpenAL Sources
	// may be limited by what the current Device supports
	// otherwise use the s_maxsources cvar
	if( s_maxsources->integer > MAX_SRC || s_maxsources->integer <= 0 )
	{ // set to default
		Com_sprintf(cvarset, sizeof(cvarset),"%i", MAX_SRC );
		Cvar_ForceSet( "s_maxsources", cvarset );
	}
	sources_n = s_maxsources->integer; // cvar sets limit on sources
	if( device_sources > 0 && sources_n > device_sources )
	{ // less one seems to work better
		sources_n = device_sources;
		if( sources_n > 1 )
			--sources_n;
	}

	qalGetError();
	qalGenSources( sources_n, gensrc ); // generate the collection of Sources
	if( calErrorCheckAL( __LINE__ ) != AL_NO_ERROR )
	{
		Com_Printf("Failed to generate %i OpenAL Sources. "
				" Sound effects disabled\n", sources_n );
		return;
	}

	// init free Sources list
	src_freehead = src_link;
	last = 0;
	for( ix = 0; ix < sources_n; ix++ )
	{
		src_link[ix].next = &src_link[ix + 1];
		src_link[ix].prev = &src_link[ix - 1];
		src_link[ix].src = &src_data[ix];

		src_data[ix].backlink = (void*) &src_link[ix];
		src_data[ix].start_timer = 0;
		src_data[ix].stop_timer = 0;
		src_data[ix].entnum = 0;
		src_data[ix].entchannel = CHAN_AUTO;
		src_data[ix].fixed_origin = false;
		src_data[ix].attn_class = ATTN_NONE;
		src_data[ix].looping = 0;
		src_data[ix].loop_mark = lpmk_nonloop;
		src_data[ix].entity_index = 0;
		src_data[ix].sound_field = 0;
		src_data[ix].velocity_state = velst_nodoppler;
		src_data[ix].sfx = NULL;
		src_data[ix].oalSource = gensrc[ix];
		last = ix;

		// Set Source properties to distance settings and defaults
		qalSourcef( src_data[ix].oalSource, AL_GAIN, (ALfloat)0.0f );
		qalSourcei( src_data[ix].oalSource, AL_LOOPING, AL_FALSE );
		qalSourcef( src_data[ix].oalSource, AL_REFERENCE_DISTANCE,
		        source_default.reference_distance );
		qalSourcef( src_data[ix].oalSource, AL_MAX_DISTANCE,
		        source_default.max_distance );
		qalSourcef( src_data[ix].oalSource, AL_ROLLOFF_FACTOR,
		        source_default.rolloff_factor );
		qalSourcef( src_data[ix].oalSource, AL_PITCH, source_default.pitch );
		qalSourcef( src_data[ix].oalSource, AL_MIN_GAIN,
		        source_default.min_gain );
		qalSourcef( src_data[ix].oalSource, AL_MAX_GAIN,
		        source_default.max_gain );
		qalSourcefv( src_data[ix].oalSource, AL_VELOCITY, zero_velocity );
		qalSourcefv( src_data[ix].oalSource, AL_DIRECTION, zero_direction );
		qalSourcef( src_data[ix].oalSource, AL_CONE_OUTER_GAIN,
		        source_default.cone_outer_gain );
		qalSourcef( src_data[ix].oalSource, AL_CONE_INNER_ANGLE,
		        source_default.cone_inner_angle );
		qalSourcef( src_data[ix].oalSource, AL_CONE_OUTER_ANGLE,
		        source_default.cone_outer_angle );
	}
	src_link[0].prev = NULL;
	src_link[last].next = NULL;
	actual_src_count = last + 1;
}

src_t *calSourceAlloc( void )
{
	src_t *src = NULL;
	srclink_t *srclink;

	if( src_freehead )
	{ // at least one avail on freelist
		srclink = src_freehead;
		if( srclink->next == NULL )
		{ // the free list is now empty
			src_freehead = NULL;
		}
		else
		{ // unlink from freelist
			src_freehead = srclink->next;
			src_freehead->prev = NULL;
		}
		if( src_datahead )
		{ // link to head of active Sources list
			srclink->next = src_datahead;
			srclink->prev = NULL;
			src_datahead->prev = srclink;
			src_datahead = srclink;
		}
		else
		{ // first entry on active Sources list
			srclink->next = NULL;
			srclink->prev = NULL;
			src_datahead = srclink;
		}
		// get the return src, backlink to link record
		src = srclink->src;

		if( ++source_counter > max_sources_used )
		{ // update maximum used statistic
			max_sources_used = source_counter;
		}
		source_failed_counter = 0;
	}
	else
	{
		if( ++source_failed_counter > max_sources_failed )
		{
			max_sources_failed = source_failed_counter;
		}
		//Snd_DPrintf( 3, "[Source Allocation Failed]\n" );
	}

	return src;
}

void calSourceFree( src_t *src )
{
	srclink_t *srclink;
	srclink_t *srclink_prev;
	srclink_t *srclink_next;

	--source_counter; // statistics update

	// unlink from active list
	srclink = src->backlink;
	srclink_prev = srclink->prev;
	srclink_next = srclink->next;
	if( srclink_next == NULL )
	{ // tail entry on active list
		if( srclink_prev == NULL )
		{ // active list is now empty
			src_datahead = NULL;
		}
		else
		{
			srclink_prev->next = NULL;
		}
	}
	else if( srclink_prev == NULL )
	{ // first entry on active list
		srclink_next->prev = NULL;
		src_datahead = srclink_next;
	}
	else
	{ // inbetween
		srclink_prev->next = srclink_next;
		srclink_next->prev = srclink_prev;
	}
	// link to freelist
	if( src_freehead )
	{ // link to head of non-empty free list
		srclink->next = src_freehead;
		srclink->prev = NULL;
		src_freehead->prev = srclink;
		src_freehead = srclink;
	}
	else
	{ // freelist was empty
		srclink->next = NULL;
		srclink->prev = NULL;
		src_freehead = srclink;
	}

}

/*
 ==
 calSoundFXInitialize()
 calSoundFXAlloc()
 calSoundFXFree()

 generate and manage OpenAL Buffers and associated Sound Effects data
 ==
 */
void calSoundFXInitialize( void )
{
	int ix;
	int last;
	ALuint genbfr[MAX_SFX];
	ALint buffers_n;

	// generate a collection of OpenAL Buffers
	buffers_n = MAX_SFX;
	qalGetError();
	for( ;; )
	{
		qalGenBuffers( buffers_n, genbfr );// generate the collection of Buffers
		if( qalGetError() == AL_NO_ERROR )
		{
			break;
		}
		// If failed, OpenAL creates no Buffers. Try for a lower number.
		buffers_n -= 2;
		if( buffers_n < 128 ) // somewhat arbitrary
		{ // give up
			return;
		}
	}

	// init active SoundFX list to empty
	sfx_datahead = NULL;

	// init free SoundFX list
	sfx_freehead = sfx_link;
	last = 0;
	for( ix = 0; ix < buffers_n; ix++ )
	{
		sfx_link[ix].next = &sfx_link[ix + 1];
		sfx_link[ix].prev = &sfx_link[ix - 1];
		sfx_link[ix].sfx = &sfx_data[ix];

		sfx_data[ix].backlink = (void*) &sfx_link[ix];
		sfx_data[ix].silent = false;
		sfx_data[ix].bg_music = false;
		sfx_data[ix].registration_sequence = 0;
		sfx_data[ix].byte_width = 0;
		sfx_data[ix].channels = 0;
		sfx_data[ix].samplerate = 0;
		sfx_data[ix].byte_count = 0;
		sfx_data[ix].pcmbfr = NULL;
		sfx_data[ix].buffered = false;
		sfx_data[ix].oalFormat = 0;
		sfx_data[ix].oalBuffer = genbfr[ix];
		sfx_data[ix].aliased = false;
		memset( sfx_data[ix].name, '\0', MAX_QPATH );
		memset( sfx_data[ix].truename, '\0', MAX_QPATH );
		last = ix;
	}
	sfx_link[0].prev = NULL;
	sfx_link[last].next = NULL;
	actual_sfx_count = last + 1;
}

sfx_t *calSoundFXAlloc( void )
{
	sfx_t *sfx = NULL;
	sfxlink_t *sfxlink;

	if( sfx_freehead )
	{ // at least one avail on freelist
		sfxlink = sfx_freehead;
		if( sfxlink->next == NULL )
		{ // the free list is now empty
			sfx_freehead = NULL;
		}
		else
		{ // unlink from freelist
			sfx_freehead = sfxlink->next;
			sfx_freehead->prev = NULL;
		}
		if( sfx_datahead )
		{ // link to head of active Sources list
			sfxlink->next = sfx_datahead;
			sfxlink->prev = NULL;
			sfx_datahead->prev = sfxlink;
			sfx_datahead = sfxlink;
		}
		else
		{ // first entry on active Sources list
			sfxlink->next = NULL;
			sfxlink->prev = NULL;
			sfx_datahead = sfxlink;
		}
		// get the return sfx, backlink to link record
		sfx = sfxlink->sfx;
	}
	return sfx;
}

void calSoundFXFree( sfx_t *sfx )
{
	sfxlink_t *sfxlink;
	sfxlink_t *sfxlink_prev;
	sfxlink_t *sfxlink_next;

	// unlink from active list
	sfxlink = sfx->backlink;
	sfxlink_prev = sfxlink->prev;
	sfxlink_next = sfxlink->next;
	if( sfxlink_next == NULL )
	{ // tail entry on active list
		if( sfxlink_prev == NULL )
		{ // active list is now empty
			sfx_datahead = NULL;
		}
		else
		{
			sfxlink_prev->next = NULL;
		}
	}
	else if( sfxlink_prev == NULL )
	{ // first entry on active list
		sfxlink_next->prev = NULL;
		sfx_datahead = sfxlink_next;
	}
	else
	{ // inbetween
		sfxlink_prev->next = sfxlink_next;
		sfxlink_next->prev = sfxlink_prev;
	}
	// link to freelist
	if( sfx_freehead )
	{ // link to head of non-empty free list
		sfxlink->next = sfx_freehead;
		sfxlink->prev = NULL;
		sfx_freehead->prev = sfxlink;
		sfx_freehead = sfxlink;
	}
	else
	{ // freelist was empty
		sfxlink->next = NULL;
		sfxlink->prev = NULL;
		sfx_freehead = sfxlink;
	}

	// the oalBuffer is not deleted, but this should free the memory
	qalBufferData( sfx->oalBuffer, AL_FORMAT_MONO16, pcmNil, sizeof( pcmNil ),
	        44100 );
}

/*
 ==
 calALFormat()

 Standard OpenAL buffers may contain:
 unsigned 8-bit, mono
 unsigned 8-bit, stereo
 signed 16-bit, mono
 signed 16-bit, stereo

 unsigned 8-bit is 0 to 255
 signed 16-bit is -32768 to 32767
 stereo data is interleaved, left channel first
 ==
 */
ALuint calALFormat( int byte_width, int channels )
{
	ALuint al_format = 0;

	switch( byte_width )
	{
	case 1:
		switch( channels )
		{
		case 1:
			al_format = AL_FORMAT_MONO8;
			break;
		case 2:
			al_format = AL_FORMAT_STEREO8;
			break;
		default:
			break;
		}
		break;

	case 2:
		switch( channels )
		{
		case 1:
			al_format = AL_FORMAT_MONO16;
			break;
		case 2:
			al_format = AL_FORMAT_STEREO16;
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return al_format;
}

/*
 ==
 calDistanceFromListenerSq()

 calculate the squared distance from the Listener to the Source.

 returns the SQUARED distance

 Note: OpenAL does not cull by distance (see documentation for the why)
 ==
 */
ALfloat calDistanceFromListenerSq( ALfloat *listener_position,
        ALfloat *source_position )
{
	ALfloat dvector[3];
	ALfloat distance;

	dvector[0] = listener_position[0] - source_position[0];
	dvector[1] = listener_position[1] - source_position[1];
	dvector[2] = listener_position[2] - source_position[2];
	distance = dvector[0] * dvector[0] + dvector[1] * dvector[1] + dvector[2]
	        * dvector[2];

	return distance;
}

/*
 ==
 calNewSrc()
 calNewLocalSrc()
 calNewSrcAutoLoop()

 create a new instance of a sound. special cases for "automatic" looping sounds
 and local (non-3D) sources.
 ==
 */
src_t *calNewSrc( int entnum, int entchannel, sfx_t *sfx, vec3_t origin,
        float gain, float fvol, float attenuation, float timeofs )
{
	src_t *src = NULL;
	src_t *src2;
	srclink_t *srclink;
	ALfloat position[3];
	vec_t distance_squared;
	vec3_t delta;
	ALfloat max_distance = 0.0f;
	ALfloat rolloff_factor = 0.0f;
	ALfloat reference_distance = 0.0f;
	ALint state;
	ALint attn_class;

	attn_class = (int)( attenuation + 0.5f );
	if( attn_class < ATTN_NONE || attn_class > ATTN_STATIC )
	{
		Snd_DPrintf( 1, "Program Error: Invalid Attenuation (%i)\n", __LINE__);
		return NULL;
	}

	src = calSourceAlloc();
	if( src == NULL )
	{
		return NULL;
	}

	src->sfx = sfx;
	src->start_timer = (int)( timeofs * 1000.0f ); // to int msecs
	src->stop_timer = 0;
	src->entnum = entnum;
	src->entchannel = entchannel;
	src->fixed_origin = ( origin != NULL );
	src->attn_class = attn_class;
	src->looping = false;
	src->loop_mark = lpmk_nonloop;
	src->entity_index = entnum; // might be 0. fixed_origin overrides.
	src->sound_field = 0;
	src->velocity_state = velst_nodoppler; // might change below

	if( !src->fixed_origin && ( ( src->entchannel == CHAN_WEAPON
	        && src->attn_class == ATTN_NORM ) || ( src->start_timer != 0
	        && src->entchannel == CHAN_AUTO && src->attn_class == ATTN_NORM )))
	{ // setup for updating velocity for doppler.
		src->velocity_state = velst_init;
	}

	if( src->entnum != 0 && src->entchannel != 0 )
	{ // Theory: Stop anything playing with the same entity and non-zero channel
		/*
		 * it seems unnecessary to stop same channel sounds in most cases
		 *
		 * Some special hacks here.
		 * - moving things (plats, doors) require the "closing/landing" sound
		 *  to stop the "traveling" sound.
		 * - other things, "sproing", for instance, can lose their sound.
		 * The solution here is to use a timer to delay before stopping the
		 * sound. It appears to work.
		 * (Possible that "sproing" or what stops it is on the wrong entchannel)
		 */
		for( srclink = src_datahead; srclink != NULL; srclink = srclink->next )
		{
			src2 = srclink->src;
			if( src2 == src || src2->start_timer != 0 || src->start_timer != 0
			        || src2->attn_class != src->attn_class )
			{ // don't stop self plus some other overriding conditions
				continue;
			}
			if( ( ( src->entnum == src2->entnum ) ) && ( src->entchannel
			        == src2->entchannel ) )
			{ // found one, set timer to stop this sound
				qalGetSourcei( src2->oalSource, AL_SOURCE_STATE, &state );
				if( state == AL_PLAYING )
				{
					src2->stop_timer = stop_timer_msecs;
				}
			}
		}
	}

	/*
	 * Determine positioning settings
	 */
	if( src->attn_class == ATTN_NONE )
	{ // like local. announcements.
		qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
		qalSourcefv( src->oalSource, AL_POSITION, zero_position );
	}
	else if( src->entnum == cl.playernum + 1 )
	{ // associated with listener entity
		if( origin == NULL )
		{ // same as Listener
			qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
			qalSourcefv( src->oalSource, AL_POSITION, zero_position );
		}
		else
		{ // just close to Listener
			qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
			qalGetListenerfv( AL_POSITION, position );
			VectorSubtract( position, origin, position );
			qalSourcefv( src->oalSource, AL_POSITION, position );
		}
	}
	else if( src->entnum != 0 )
	{ // some other entity
		qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_FALSE );
		if( origin == NULL )
		{ // position from entity
			CL_GetEntitySoundOrigin( src->entity_index, position );
			qalSourcefv( src->oalSource, AL_POSITION, position );
		}
		else
		{ // position from function arg
			qalSourcefv( src->oalSource, AL_POSITION, origin );
		}
	}
	else
	{ // no entity
		if( origin != NULL )
		{ // position from function arg
			//
			// SPECIAL CASE: if close to Listener, make it RELATIVE
			//  otherwise, alien disruptor sound gets lost
			//
			qalGetListenerfv( AL_POSITION, position );
			VectorSubtract( origin, position, delta );
			distance_squared = ( delta[0] * delta[0] + delta[1] * delta[1]
			        + delta[2] * delta[2] );
			if( distance_squared < ( 128.0f * 128.0f ) ) // just a guess
			{ // Probably Listener local
				qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
				qalSourcefv( src->oalSource, AL_POSITION, zero_position );
			}
			else
			{ // regular
				qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_FALSE );
				qalSourcefv( src->oalSource, AL_POSITION, origin );
			}
		}
		else
		{ // no entnum, no origin, must be local
			qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
			qalSourcefv( src->oalSource, AL_POSITION, zero_position );
		}
	}

	/*
	 * Set up distance model parameters
	 * (Note: for "local" sounds, does not really matter)
	 */
	switch( src->attn_class )
	{ // distance attenuation variants.
	case ATTN_NONE: // "full volume the entire level"
		reference_distance =  0.0f;
		max_distance = 0.0f;
		rolloff_factor = 0.0f;
		break;

	case ATTN_NORM:  // normal distance attenuation
		reference_distance = source_default.reference_distance;
		max_distance = source_default.max_distance;
		rolloff_factor = source_default.rolloff_factor;
		break;

	case ATTN_IDLE: // "idle" (meaning?) distance attenuation
		reference_distance = source_default.idle_reference_distance;
		max_distance = source_default.idle_max_distance;
		rolloff_factor = source_default.idle_rolloff_factor;
		break;

	case ATTN_STATIC: // "diminish very rapidly with distance"
		reference_distance = source_default.static_reference_distance;
		max_distance = source_default.static_max_distance;
		rolloff_factor = source_default.static_rolloff_factor;
		break;

	default: // should be impossible, checked above
		Sys_Error("In snd_openal.c:calNewSrc()");
		break;
	}

	// set properties
	qalSourcef( src->oalSource, AL_GAIN, gain );
	qalSourcei( src->oalSource, AL_LOOPING, AL_FALSE );
	qalSourcef( src->oalSource, AL_PITCH, source_default.pitch );
	qalSourcef( src->oalSource, AL_MAX_DISTANCE, max_distance );
	qalSourcef( src->oalSource, AL_ROLLOFF_FACTOR, rolloff_factor );
	qalSourcef( src->oalSource, AL_REFERENCE_DISTANCE, reference_distance );
	qalSourcefv( src->oalSource, AL_VELOCITY, zero_velocity );
	qalSourcef( src->oalSource, AL_MIN_GAIN, source_default.min_gain );
	qalSourcef( src->oalSource, AL_MAX_GAIN, source_default.max_gain );
	qalSourcefv( src->oalSource, AL_DIRECTION, zero_direction );
	// when direction vector is the zero vector, these disabled
	//qalSourcef( src->oalSource, AL_CONE_OUTER_GAIN, source_default.cone_outer_gain );
	//qalSourcef( src->oalSource, AL_CONE_INNER_ANGLE, source_default.cone_inner_angle );
	//qalSourcef( src->oalSource, AL_CONE_OUTER_ANGLE, source_default.cone_outer_angle );

	// attach the Buffer
	qalSourcei( src->oalSource, AL_BUFFER, src->sfx->oalBuffer );
	calErrorCheckAL( __LINE__ );

	return src;
}

src_t *calNewLocalSrc( sfx_t *sfx, float gain )
{
	src_t *src;

	src = calSourceAlloc();
	if( src == NULL )
	{
		return NULL;
	}

	src->sfx = sfx;
	src->start_timer = 0;
	src->stop_timer = 0;
	src->entnum = 0;
	src->entchannel = 0;
	src->fixed_origin = false;
	src->attn_class = 0;
	src->looping = false;
	src->loop_mark = lpmk_nonloop;
	src->entity_index = 0;
	src->sound_field = 0;
	src->velocity_state = velst_nodoppler;

	// Set as a local Source with the requested gain (usually 1.0)
	qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
	qalSourcefv( src->oalSource, AL_POSITION, zero_position );
	qalSourcef( src->oalSource, AL_GAIN, gain );
	qalSourcei( src->oalSource, AL_LOOPING, AL_FALSE );
	qalSourcefv( src->oalSource, AL_VELOCITY, zero_velocity );

	// Make sure Source properties are at defaults
	qalSourcef( src->oalSource, AL_PITCH, source_default.pitch );
	qalSourcef( src->oalSource, AL_MAX_DISTANCE, source_default.max_distance );
	qalSourcef( src->oalSource, AL_ROLLOFF_FACTOR,
	        source_default.rolloff_factor );
	qalSourcef( src->oalSource, AL_REFERENCE_DISTANCE,
	        source_default.reference_distance );
	qalSourcef( src->oalSource, AL_MIN_GAIN, source_default.min_gain );
	qalSourcef( src->oalSource, AL_MAX_GAIN, source_default.max_gain );
	qalSourcefv( src->oalSource, AL_DIRECTION, zero_direction );
	// when direction vector is the zero vector, these disabled
	//qalSourcef( src->oalSource, AL_CONE_OUTER_GAIN, source_default.cone_outer_gain );
	//qalSourcef( src->oalSource, AL_CONE_INNER_ANGLE, source_default.cone_inner_angle );
	//qalSourcef( src->oalSource, AL_CONE_OUTER_ANGLE, source_default.cone_outer_angle );

	// Attach the Buffer
	qalSourcei( src->oalSource, AL_BUFFER, src->sfx->oalBuffer );

	calErrorCheckAL( __LINE__ );
	return src;
}

src_t *calNewSrcAutoLoop( int entity_index, int sound_field )
{
	sfx_t *sfx;
	src_t *src;
	ALfloat source_position[3];

	sfx = cl.sound_precache[sound_field]; // get SFX from the precache.

	if( sfx == NULL )
	{ // note: sound_precache sfx pointers are determined by S_RegisterSound()
		//Snd_DPrintf( 3, "[looping sound not registered (%i)", __LINE__ );
		return NULL;
	}

	if( sfx->bg_music || !Q_strncasecmp( sfx->name, "music", 5 ))
	{ //  background music from speakers is obsolete
		//Snd_DPrintf( 3, "[background music looping sound (%i)", __LINE__ );
		return NULL;
	}

	src = calSourceAlloc(); // get Src from free list
	if( src == NULL )
	{
		return NULL;
	}

	src->start_timer = 0;
	src->stop_timer = 0;
	src->entnum = entity_index;
	src->entchannel = CHAN_AUTO;
	src->fixed_origin = false;
	src->attn_class = ATTN_NONE; // Does not apply
	src->looping = true;
	src->loop_mark = lpmk_start;
	src->entity_index = entity_index;
	src->sound_field = sound_field;
	src->velocity_state = velst_init;
	src->sfx = sfx;

	qalSourcei( src->oalSource, AL_LOOPING, AL_TRUE );
	if( src->entnum == cl.playernum + 1 )
	{ // associated with listener entity, for instance, "smartgun hum"
		src->velocity_state = velst_nodoppler; // of course
		qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
		qalSourcefv( src->oalSource, AL_POSITION, zero_position );
	}
	else
	{ //  set position from entity information
		CL_GetEntitySoundOrigin( entity_index, source_position );
		qalSourcei( src->oalSource, AL_SOURCE_RELATIVE, AL_FALSE );
		qalSourcefv( src->oalSource, AL_POSITION, source_position );
	}

	qalSourcef( src->oalSource, AL_GAIN, source_default.looping_gain );
	qalSourcef( src->oalSource, AL_REFERENCE_DISTANCE,
	        source_default.looping_reference_distance );
	qalSourcef( src->oalSource, AL_MAX_DISTANCE,
	        source_default.looping_max_distance );
	qalSourcef( src->oalSource, AL_ROLLOFF_FACTOR,
	        source_default.looping_rolloff_factor );
	qalSourcefv( src->oalSource, AL_VELOCITY, zero_velocity );

	// Make sure Source properties are at defaults
	qalSourcef( src->oalSource, AL_PITCH, source_default.pitch );
	qalSourcef( src->oalSource, AL_MIN_GAIN, source_default.min_gain );
	qalSourcef( src->oalSource, AL_MAX_GAIN, source_default.max_gain );
	qalSourcefv( src->oalSource, AL_DIRECTION, zero_direction );
	// when direction vector is the zero vector, these disabled
	//qalSourcef( src->oalSource, AL_CONE_OUTER_GAIN, source_default.cone_outer_gain );
	//qalSourcef( src->oalSource, AL_CONE_INNER_ANGLE, source_default.cone_inner_angle );
	//qalSourcef( src->oalSource, AL_CONE_OUTER_ANGLE, source_default.cone_outer_angle );

	// attach Buffer
	qalSourcei( src->oalSource, AL_BUFFER, src->sfx->oalBuffer );

	return src;
}

/*
 ==
 calLoadSound()

 Load an initialized/registered sound effect into an OpenAL Buffer
 ==
 */
void calLoadSound( sfx_t *sfx )
{
	char *name;
	char namebuffer[MAX_OSPATH];
	qboolean success;

	if( sfx->buffered || sfx->silent )
	{ // already done for this one
		return;
	}

	if( sfx->name[0] == '*' )
	{ // Model specific sounds are loaded in S_StartSound() when entity is known
		// use this SFX for the default
		// set it up as an aliased SFX
		Com_sprintf(sfx->truename, MAX_QPATH, "player/male/%s",&(sfx->name[1]));
		sfx->aliased = true;
	}

	// figure out full filepath
	name = sfx->aliased ? sfx->truename : sfx->name;

	if( name[0] == '#' )
	{ // not in sound subdirectory, normally, #player/blah,
		strncpy( namebuffer, &name[1], MAX_OSPATH ); // strip off '#'
	}
	else
	{ // standard sound effect location
		Com_sprintf( namebuffer, sizeof( namebuffer ), "sound/%s", name );
	}

	// read the sound file PCM data into an intermediate buffer
	// enable Ogg Vorbis substition for background music files
	success = S_LoadSound( namebuffer, sfx->bg_music,
			&sfx->filebfr, &sfx->pcmbfr, &sfx->byte_width,
			&sfx->channels, &sfx->samplerate, &sfx->byte_count );

	if( !success )
	{
		sfx->silent = true; // prevent repetitive searching for missing/bad file
	}

	if( sfx->silent )
	{ // unable to get sound effect data
		Snd_DPrintf( 1, "Sound File %s could not be read.\n", ( sfx->aliased
		        ? sfx->truename : sfx->name ) );
		return;
	}

	// transfer PCM data to OpenAL Buffer
	sfx->oalFormat = calALFormat( sfx->byte_width, sfx->channels );
	if( sfx->oalFormat )
	{
		qalGetError();
		qalBufferData( sfx->oalBuffer, sfx->oalFormat, sfx->pcmbfr,
		        (ALsizei)( sfx->byte_count ), (ALsizei)( sfx->samplerate ) );
		if( calErrorCheckAL( __LINE__ ) != AL_NO_ERROR )
		{ // failed to transfer
			sfx->silent = true;
			Snd_DPrintf( 1, "Sound File %s failed to load into OpenAL buffer\n",
			        ( sfx->aliased ? sfx->truename : sfx->name ) );
		}
		else
		{ // success
			sfx->buffered = true;
		}
	}
	else
	{ // not a supported file format
		sfx->silent = true;
		Snd_DPrintf( 1, "Sound File %s's format is not supported.\n",
		        ( sfx->aliased ? sfx->truename : sfx->name ) );
	}

	// done with file buffer
	FS_FreeFile( sfx->filebfr );
	sfx->filebfr = NULL;
	sfx->pcmbfr = NULL;
}

/*
 ==
 calContextInfo()

 info about sound system configuration
 ==
 */
void calContextInfo( ALCdevice *pDevice )
{
	ALCint alc_frequency;
	ALCint alc_mono_sources;
	ALCint alc_stereo_sources;
	ALCint alc_refresh;
	ALCint alc_sync;
	int ix;

	calErrorCheckALC( pDevice, __LINE__ );

	Com_Printf( "OpenAL Information:\n" );
	Com_Printf( "Active Device: %s\n", qalcGetString( pDevice, ALC_DEVICE_SPECIFIER ));
	Com_Printf( "  Vendor:     %s\n", qalGetString( AL_VENDOR ) );
	Com_Printf( "  Version:    %s\n", qalGetString( AL_VERSION ) );
	Com_Printf( "  Renderer:   %s\n", qalGetString( AL_RENDERER ) );
	qalcGetIntegerv( pDevice, ALC_FREQUENCY, 1, &alc_frequency );
	Com_Printf( "  ALC Frequency: %i\n", alc_frequency );
	Com_Printf( "  Generated Buffer Count: %i\n", actual_sfx_count );
	// Source statistics
	qalcGetIntegerv( pDevice, ALC_MONO_SOURCES, 1, &alc_mono_sources );
	Com_Printf( "  ALC Mono Sources: %i\n", alc_mono_sources );
	qalcGetIntegerv( pDevice, ALC_STEREO_SOURCES, 1, &alc_stereo_sources );
	Com_Printf( "  ALC Stereo Sources: %i\n", alc_stereo_sources );
	Com_Printf( "  Generated Source Count: %i\n", actual_src_count );
	if( max_sources_used > 0 )
	{
		Com_Printf( "  Maximum Sources Used: %i\n", max_sources_used );
		if( max_sources_failed > 0 )
		{
			Com_Printf( "  Maximum Sources Requested: %i\n",
					max_sources_used + max_sources_failed );
		}
	}
	Snd_DPrintf( 1, "  AL Extensions: %s\n", qalGetString( AL_EXTENSIONS ) );
	Snd_DPrintf( 1, "  ALC Extensions: %s\n", qalcGetString( pDevice,ALC_EXTENSIONS ) );
	qalcGetIntegerv( pDevice, ALC_REFRESH, 1, &alc_refresh );
	Snd_DPrintf( 1, "  ALC Refresh: %i\n", alc_refresh);
	qalcGetIntegerv( pDevice, ALC_SYNC, 1, &alc_sync );
	Snd_DPrintf( 1, "  ALC Sync: %i\n", alc_sync );
	Com_Printf( "Available Devices:\n" );
	for( ix = 0; ix < oalDeviceCount; ++ix )
	{
		Com_Printf("  %s\n", pDeviceNames[ix] );
	}
	Snd_DPrintf( 1, "Speed of Sound: %f\n", speed_of_sound );
	Snd_DPrintf( 1, "Doppler Factor: %f\n", doppler_factor );

	calErrorCheckALC( pDevice, __LINE__ );
}


/*
 ==
 S_UpdateDopplerFactor()

  for update from menu, according to cvar setting,
 ==
 */
void S_UpdateDopplerFactor( void )
{

	doppler_factor = s_doppler->value;
	if( doppler_factor < 0.0f )
	{
		doppler_factor = 0.0f;
		Cvar_Set( "s_doppler", "0" );
	}
	else if( doppler_factor > maximum_doppler_factor )
	{
		doppler_factor = maximum_doppler_factor;
		Cvar_SetValue( "s_doppler", maximum_doppler_factor );
	}
	if ( sound_system_enable )
	{
		qalDopplerFactor( doppler_factor );
	}
	s_doppler->modified = false;

}

/*
 ==

 calEnumerateDevices()

  Builds a list of devices supporting OpenAL 1.1 or greater
  An empty list probably indicates OpenAL 1.0 devices only

  Notes:
   The OpenAL device list is a null-separated, double-null terminated
    list of device name strings
   snd_restart command should rebuild list because devices can change

 ==
 */
void calEnumerateDevices( void )
{
	ALCchar* pDeviceName;
	ALCdevice *pDevice;
	ALCint major_version;
	ALCint minor_version;
	int ix;
	int len;
	int pos;
	qboolean success;

	ix = 0;
	pos = 0;
	if( qalcIsExtensionPresent( NULL, "ALC_ENUMERATION_EXT" ) )
	{
		oalDeviceList = qalcGetString( NULL, ALC_DEVICE_SPECIFIER );
		pDeviceName = (ALCchar*)oalDeviceList;
		while( pDeviceName && (len = strlen( pDeviceName )) )
		{
			pDevice = qalcOpenDevice( pDeviceName );
			calErrorCheckALC( pDevice, __LINE__ );  // clear any device error
			if( pDevice )
			{ // the device can be opened
				qalcGetIntegerv( pDevice, ALC_MAJOR_VERSION, 1, &major_version );
				qalcGetIntegerv( pDevice, ALC_MINOR_VERSION, 1, &minor_version );
				if( (major_version == 1 && minor_version >= 1)
						|| major_version > 1 )
				{ // device supports OpenAL 1.1 or greater
					if( ix >= MAX_OAL_DEVICES )
					{
						Snd_DPrintf(1, "Program Error: Valid devices  > %i\n",
								MAX_OAL_DEVICES );
					}
					else
					{
						pDeviceNames[ix++] = pDeviceName;
						Snd_DPrintf( 1, "Adding Valid Device %s\n", pDeviceName );
					}
				}
				calErrorCheckALC( pDevice, __LINE__ ); // clear any device error
				success = qalcCloseDevice( pDevice );
				if( !success )
				{
					Snd_DPrintf( 1, "Close failed on %s\n", pDeviceName );
				}
			}
			else
			{
				Snd_DPrintf(1, "Open failed on %s\n", pDeviceName );
			}
			pos += len + 1;
			pDeviceName = (ALCchar*)&oalDeviceList[ pos ];  // next device string
		}
	}
	oalDeviceCount = ix; // store count of available devices in list
}

/*
 ==

 calSelectDevice()

 Theory is:
   Always accept the device specified by the user set cvar (provided it opens)
   Otherwise, use the default device.
   For the special Windows XP case, override the generic hardware default if it
     has less than the minimum specified in the s_minsources cvar.
==
 */
ALCdevice *calSelectDevice( void )
{
	ALCdevice *pDevice = NULL;
	ALCdevice *pDevice2 = NULL;
	ALCchar *pDeviceName;
	ALCint alc_mono_sources;
	ALenum result;
	int len;
	int pos;

	// selected device (note: case insensitive compare)
	if( oalDeviceCount > 0 && s_device
			&& Q_strcasecmp(s_device->string, "Default" ) )
	{ // cvar set and it is not 'Default'
		// alcOpenDevice needs case sensitive name from device list
		pDeviceName = (ALCchar*)oalDeviceList;
		pos = 0;
		while( (len = strlen( pDeviceName )) > 0 )
		{
			if( !Q_strcasecmp( s_device->string, pDeviceName ) )
			{ // found cvar device selection in list
				pDevice = qalcOpenDevice( pDeviceName );
				if( pDevice != NULL )
				{
					if( calErrorCheckALC( pDevice, __LINE__ ) != ALC_NO_ERROR )
					{
						qalcCloseDevice( pDevice );
						Com_Printf("Error opening sound device: %s\n",
								pDeviceName );
						pDevice = NULL;
					}
				}
				else
				{
					Com_Printf( "Failed to open sound device: %s\n",
							pDeviceName );
				}
				break;  // exit loop
			}
			pos += len + 1;
			pDeviceName = (ALCchar*)&oalDeviceList[ pos ]; // next device name
		}
		if( len == 0 )
		{ // no match to cvar found in list
			Com_Printf( "Device %s not in available device list\n"
					"  (check that cvar: s_device is correct.)\n",
					s_device->string );
		}
	}

	// default device
	if (pDevice == NULL)
	{ // open the Default device
		pDevice = qalcOpenDevice(NULL);
		if( pDevice != NULL )
		{ // handle Windows XP "Generic Hardware" case
			result = calErrorCheckALC( pDevice,  __LINE__ );
			pDeviceName = (ALCchar*)qalcGetString( pDevice, ALC_DEVICE_SPECIFIER );
			if( !Q_strcasecmp( (char*)pDeviceName, "Generic Hardware") )
			{
				qalcGetIntegerv(pDevice, ALC_MONO_SOURCES, 1, &alc_mono_sources);
				if ( alc_mono_sources < s_minsources->integer )
				{ // supported sources are below limit
					pDevice2 = qalcOpenDevice( "Generic Software" );
					if( pDevice2 != NULL )
					{ // use Generic Software device instead
						qalcCloseDevice( pDevice );
						pDevice = pDevice2;
					}
				}
			}
		}
	}

	return pDevice;
}

/*
 ==

 S_Init()

 - cold-start initialization
 - also target of snd_restart command

 ==
 */
// some forward declarations
void sndCvarInit( void );
void sndCmdInit( void );
void sndCmdRemove( void );

void S_Init( void )
{
	ALCdevice *pDevice;
	ALCchar *pDeviceName;
	ALCcontext *pContext;
	qboolean success;
	ALCenum result;
	ALCint alc_mono_sources;

	sound_system_enable = false;

	Com_Printf( "\n------- sound initialization -------\n" );

	// init global device state
	oalDeviceCount = 0;

	/*
	 * Setup CVARS
	 */
	sndCvarInit(); // setup s_* cvars

	doppler_factor = s_doppler->value;
	if( doppler_factor < 0.0f )
		doppler_factor = 0.0f;
	else if( doppler_factor > maximum_doppler_factor )
		doppler_factor = maximum_doppler_factor;

	if( s_initsound->value <= 0 )
	{ // command line disable of sound system
		Com_Printf( "!Sound Disabled!\n" );
		return;
	}

	// Link to OpenAL library
	success = QAL_Init();
	if( !success )
	{
		Com_Printf("Sound failed: Unable to start OpenAL.\n"
			        "Game will continue without sound.\n" );
		return;
	}

	// Enumerate Devices
	calEnumerateDevices();
	if( oalDeviceCount == 0 )
	{
		Com_Printf("Sound failed: OpenAL 1.1 or greater required\n"
				"Game will continue without sound.\n");
		return;
	}

	// Select Device
	pDevice = calSelectDevice();
	if( pDevice == NULL )
	{
		Com_Printf("Sound failed: Unable to open sound device\n"
			        "Game will continue without sound.\n" );
		return;
	}

	pContext = qalcCreateContext( pDevice, NULL ); // 2. create context
	if ( pContext == NULL )
	{
		qalcCloseDevice( pDevice );
		pDevice = NULL;
		Com_Printf(
				"Sound failed: Possible OpenAL or device configuration problem\n"
					"Game will continue without sound.\n" );
		return;
	}
	qalcMakeContextCurrent( pContext ); // 3. make context current
	result = calErrorCheckALC( pDevice, __LINE__ );

	// warn about number of Sources
	qalcGetIntegerv( pDevice, ALC_MONO_SOURCES, 1, &alc_mono_sources );
	if ( alc_mono_sources < s_minsources->integer )
	{
		pDeviceName = (ALCchar*)qalcGetString( pDevice, ALC_DEVICE_SPECIFIER );
		Com_Printf( "Warning: \"%s\" supports %i OpenAL Sources.\n"
			"  Minimum Sources (cvar: s_minsources) set to %i.\n"
			"  Some sound effects may not be heard\n",
			pDeviceName, alc_mono_sources, s_minsources->integer );
	}

	// Setup 3D Audio State
	qalDistanceModel( distance_model );
	qalDopplerFactor( doppler_factor );
	qalSpeedOfSound( speed_of_sound );

	// Initialize Listener
	if ( s_volume->value > 0  )
		qalListenerf( AL_GAIN, (ALfloat)0.1f ); // start quiet, need some for menu clicks
	else
		qalListenerf( AL_GAIN, (ALfloat)0.0f ); // sound is off
	qalListenerfv( AL_POSITION, zero_position );
	qalListenerfv( AL_VELOCITY, zero_velocity );
	qalListenerfv( AL_ORIENTATION, default_orientation );

	qalcGetIntegerv( pDevice, ALC_MONO_SOURCES, 1, &alc_mono_sources );
	result = calErrorCheckALC( pDevice, __LINE__ );
	if( result != ALC_NO_ERROR || alc_mono_sources <= 0 )
	{
		Com_Printf("OpenAL Device error: Number of Mono Sources\n");
		alc_mono_sources = 0;
	}
	else
	{ // generate OpenAL sound effects Sources.
		calSourcesInitialize( alc_mono_sources );
	}
	calSoundFXInitialize(); // generate OpenAL Buffers
	sndCmdInit(); // setup snd* console commands
	calContextInfo( pDevice ); // display OpenAL technical info

	// Initialize background music dedicated source record
	// other intialization done in S_StartMusic
	music.playing = false;
	music.sfx = NULL;
	music.oalSource = 0;

	sound_system_enable = true;

	Com_Printf( "------------------------------------\n" );
}

/*
 ==
 S_StopAllSounds()

 - quiet all sounds. Called on program state changes.
 - also target of console command: stopsound
 ==
 */
void S_StopAllSounds( void )
{
	src_t *src;
	srclink_t *srclink;

	if( !QAL_Loaded() || !sound_system_enable )
		return;

	srclink = src_datahead;
	while( srclink != NULL  )
	{
		src = srclink->src;
		srclink=srclink->next;
		qalSourceStop( src->oalSource ); // ok from any state per spec
		qalSourcei( src->oalSource, AL_BUFFER, AL_NONE ); // disconnect Buffer
		calSourceFree( src );
	}
	if ( music.oalSource && qalIsSource( music.oalSource ) )
	{ // special case for music
		qalSourceStop( music.oalSource );
		qalSourcei( music.oalSource, AL_BUFFER, AL_NONE ); // disconnect Buffer
		// special for music, only one Source using this Buffer
		// NOTE: not freeing sfx means 2 music files are resident which uses
		//  more memory, but means the Ogg Vorbis decode does not have to
		//  be done everytime the menu is accessed.
		// calSoundFXFree( music.sfx );
		music.sfx = NULL;
		music.playing = false;
	}
}

/*
 ==
 S_Shutdown()

 Complete shutdown of sound system for program exit or sound system restart
 ==
 */
void S_Shutdown( void )
{
	ALCdevice *pDevice;
	ALCcontext *pContext;
	ALCboolean result;
	int ix;
	int count;
	ALuint delsrc[MAX_SRC];
	ALuint delbfr[MAX_SFX];

	if( !QAL_Loaded() )
	{
		return;
	}

	S_StopAllSounds(); // quiets all sources, disconnects Sources from Buffers

	qalGetError();

	// Special case for music. delete the Source only, here.
	if ( music.oalSource && qalIsSource( music.oalSource ) )
	{
		qalDeleteSources( 1, &music.oalSource );
	}

	// Delete all other Sources and all Buffers
	count = 0;
	for( ix = 0; ix < MAX_SRC; ix++ )
	{
		if( qalIsSource( src_data[ix].oalSource ) )
		{
			delsrc[count++ ] = src_data[ix].oalSource;
		}
	}
	qalDeleteSources( count, delsrc );
	count = 0;
	for( ix = 0; ix < MAX_SFX; ix++ )
	{
		if( qalIsBuffer( sfx_data[ix].oalBuffer ) )
		{
			delbfr[count++ ] = sfx_data[ix].oalBuffer;
		}
	}
	qalDeleteBuffers( count, delbfr );
	calErrorCheckAL( __LINE__ );


	// Empty the src and sfx data, so its all clean if restarting
	src_datahead = NULL;
	src_freehead = NULL;
	sfx_datahead = NULL;
	sfx_freehead = NULL;
	actual_src_count = 0;
	actual_sfx_count = 0;
	memset( src_link, 0, sizeof( src_link ) );
	memset( src_data, 0, sizeof( src_link ) );
	memset( sfx_link, 0, sizeof( sfx_link ) );
	memset( sfx_data, 0, sizeof( sfx_link ) );

	// Close OpenAL Context
	pContext = qalcGetCurrentContext();
	pDevice = qalcGetContextsDevice( pContext );
	calErrorCheckALC( pDevice, __LINE__ );
	result = qalcMakeContextCurrent( NULL );
	if( result )
	{
		qalcDestroyContext( pContext );
	}
	else
	{
		Snd_DPrintf( 1, "qalcMakeContextCurrent(NULL) failed\n" );
	}
	// Close OpenAL Device
	result = qalcCloseDevice( pDevice );
	if( !result )
	{
		Snd_DPrintf( 1, "qalcCloseDevice failed\n" );
	}

	QAL_Shutdown(); // Unlink OpenAL dynamic/shared library
	sndCmdRemove(); // remove certain console commands

	sound_system_enable = false;
}

/*
 ==
 S_FindName()

 Lookup a sound effects record.
 If it does not exist and 'create' requested, create a new one

 ==
 */
sfx_t *S_FindName( const char *name, qboolean create )
{
	sfxlink_t *sfxlink;
	sfx_t *sfx = NULL;

	for( sfxlink = sfx_datahead; sfxlink != NULL; sfxlink = sfxlink->next )
	{
		sfx = sfxlink->sfx;
		if( strncmp( sfx->name, name, MAX_QPATH ) == 0 )
		{
			return sfx;
		}
	}
	if( create )
	{
		sfx = calSoundFXAlloc(); // from free list
		if( sfx != NULL )
		{ // set registration seq, file name. clear or default others
			sfx->silent = false;
			sfx->bg_music = false;
			sfx->registration_sequence = s_registration_sequence;
			sfx->byte_width = 0;
			sfx->channels = 0;
			sfx->samplerate = 0;
			sfx->byte_count = 0;
			sfx->pcmbfr = NULL;
			sfx->buffered = false;
			sfx->oalFormat = 0;
			sfx->aliased = false;
			memset( sfx->name, '\0', MAX_QPATH );
			strncpy( sfx->name, name, MAX_QPATH - 1 );
			memset( sfx->truename, '\0', MAX_QPATH );
		}
	}
	return sfx;
}

/*
 ==
 S_BeginRegistration

 Start registration of sound effects.
 Preloads commonly used and looping sounds
 ==
 */
void S_BeginRegistration( void )
{
	if( sound_system_enable )
	{
		s_registration_sequence++ ;
		s_registering = true;
	}
}

/*
 ==
 calFreeOldSndFX()

 free the SFX's not in the current registration sequence
 ==
 */
void calFreeOldSndFX( void )
{
	sfxlink_t *sfxlink;
	sfx_t *sfx;

	sfxlink = sfx_datahead;
	while( sfxlink )
	{
		sfx = sfxlink->sfx;
		sfxlink = sfxlink->next; // must advance pointer *before* list modify
		if( sfx->registration_sequence != s_registration_sequence )
		{
			calSoundFXFree( sfx );
		}
	}
}

/*
 ==
 S_RegisterSound()

 if pre-caching, add a sound effect to the collection, but don't load it yet
 otherwise, register and load the sound effect

 the returned sfx_t pointer is just a "handle" to other parts of the system
 ==
 */
sfx_t *S_RegisterSound( char *name )
{
	sfx_t *sfx = NULL;

	if( sound_system_enable )
	{
		sfx = S_FindName( name, true );
		if( sfx == NULL )
		{ // out of SFX's, free up some, try again.
			// this may free an SFX that will be registered later in the
			// sequence -- the downside of only having as many SFXs as
			// there are OpenAL Buffers.
			calFreeOldSndFX();
			sfx = S_FindName( name, true );
		}
		if( sfx != NULL )
		{ // if it already existed, needs to be put in current sequence
			sfx->registration_sequence = s_registration_sequence;
			if( !s_registering )
			{ // not doing a pre-cache sequence, load the data
				calLoadSound( sfx );
			}
		}
		else
		{
			Snd_DPrintf( 1, "S_RegisterSound(): Failed to register %s\n", name );
		}
	}

	return sfx;
}

/*
 ==
 S_EndRegistration()

 Finish cache processing. Does actual loading of sound data.
 ==
 */
void S_EndRegistration( void )
{
	sfxlink_t *sfxlink;

	if( sound_system_enable )
	{
		// free now unused SFX's from previous registration sequence
		calFreeOldSndFX();
		// load up this registration sequence
		for( sfxlink = sfx_datahead; sfxlink != NULL; sfxlink = sfxlink->next )
		{
			calLoadSound( sfxlink->sfx );
		}
		s_registering = false; // done with this registration sequence
	}
}

/*
 ==
 calModelFilename

 extract the model name from configstrings, and construct the filename
 format of model/skin names in configstrings is:
 <playername>\<model>/<skin>

 ==
 */
void calModelFilename( entity_state_t *ent, const char *basename,
        char* qfilepath )
{
	int cfgstr_ix;
	char *ptr;
	char model[MAX_QPATH];

	cfgstr_ix = CS_PLAYERSKINS + ent->number - 1;
	ptr = cl.configstrings[cfgstr_ix];
	model[0] = '\0';
	if( *ptr )
	{ // non-empty string with player\model/skin
		ptr = strchr( ptr, '\\' );
		if( ptr )
		{
			++ptr;
			strncpy( model, ptr, MAX_QPATH );
			ptr = strchr( model, '/' );
			if( ptr )
				*ptr = '\0';
		}
	}
	if( model[0] == '\0' )
	{
		*qfilepath = '\0'; // return empty string
	}
	else
	{ // return path to model specific sound
		Com_sprintf( qfilepath, MAX_QPATH, "#players/%s/%s", model, basename );
	}
}

/*
 ==
 S_RegisterSoundsForPlayer()

 Register the model-specific sounds (pain, jumping, etc.) that should be
 present for each player model.
 
 This function is called from the model loading code. It relies on the fact
 that the server precaches sounds before models, so the generic (fallback)
 version of each model-specific sound is already in the precache list at this
 point.
 ==
 */
void S_RegisterSoundsForPlayer (char *playername)
{
	int		i;
	char	soundpath[MAX_QPATH];
	
	return; // FIXME: figure out why Windows doesn't like this.
	
	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		if (cl.configstrings[CS_SOUNDS+i][0] != '*')
			continue;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Com_sprintf (soundpath, MAX_QPATH, "#players/%s/%s", playername, &cl.configstrings[CS_SOUNDS+i][1]);
		S_RegisterSound (soundpath);
	}
}

/*
 ==

 S_StartSound()

 ==
 */
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfx_t *arg_sfx,
        float fvol, float attenuation, float timeofs )
{
	src_t *src;
	entity_state_t *ent;
	sfx_t *model_sfx;
	sfx_t *sfx;
	char qfilename[MAX_QPATH];
	ALfloat listener_position[3];
	ALfloat source_position[3];
	ALfloat dist_sq = 0.0f;
	ALfloat sq_cull_distance;
	int attn_class;

	if( !sound_system_enable || arg_sfx == NULL )
	{
/*
		if( arg_sfx == NULL )
		{
			Snd_DPrintf( 3, "[StartSound: SFX is NULL]\n" );
		}
*/
		return;
	}

	if( arg_sfx->bg_music
			|| !Q_strncasecmp( arg_sfx->name, "music", 5 ) )
	{ // background music from speakers is obsolete
		return;
	}
	
	// cull sound by distance
	attn_class = (int)(attenuation + 0.5f);
	if( attn_class != ATTN_NONE )
	{
		switch( attn_class )
		{
		case ATTN_NORM:
			sq_cull_distance = norm_sq_cull_distance;
			break;
		case ATTN_IDLE:
			sq_cull_distance = idle_sq_cull_distance;
			break;
		case ATTN_STATIC:
			sq_cull_distance = static_sq_cull_distance;
			break;
		default:
			Snd_DPrintf( 1, "Program Error: ATTN_ value (%i)\n", __LINE__);
			sq_cull_distance = 0.0f;
			break;
		}
		qalGetListenerfv( AL_POSITION, listener_position );
		if( origin != NULL )
		{
			dist_sq = calDistanceFromListenerSq( listener_position, origin );
			if( dist_sq > sq_cull_distance)
			{
				//Snd_DPrintf( 3, "[Culled-1: %s]\n", arg_sfx->name);
				return;
			}
		}
		else if( entnum != 0 )
		{
			CL_GetEntitySoundOrigin( entnum, source_position );
			dist_sq = calDistanceFromListenerSq( listener_position, source_position );
			if( dist_sq > sq_cull_distance)
			{
				//Snd_DPrintf( 3, "[Culled-2: %s]\n", arg_sfx->name);
				return;
			}
		}
	}

	model_sfx = NULL;
	sfx = arg_sfx;
	if( sfx->name[0] == '*' )
	{ // model specific sound
		// get the model specific filename
		ent = &cl_entities[entnum].current;
		calModelFilename( ent, &( sfx->name[1] ), qfilename );
		if( qfilename[0] != '\0' )
		{ // there is a model specific name, register it
			model_sfx = S_RegisterSound( qfilename );
			if( model_sfx != NULL )
			{ // use it, otherwise the default (aliased) SFX is used
				sfx = model_sfx;
			}
		}
	}
	calLoadSound( sfx );
	if( !sfx->buffered || sfx->silent )
	{ // not available
		return;
	}

	src = calNewSrc( entnum, entchannel, sfx, origin, 1.0, fvol, attenuation,
	        timeofs );
	if( src == NULL )
	{ // no src available
		return;
	}

	if( src->start_timer < 5 )
	{
		src->start_timer = 0;
		qalSourcePlay( src->oalSource ); // !make some noise!
	}

/*
	if( origin != NULL )
	{
		Snd_DPrintf( 3,
			"[StartSound: CHAN=%i, ATTN=%i, timer=%i, origin=(%f,%f,%f) %s]\n",
			src->entchannel, src->attn_class, src->start_timer,
			origin[0], origin[1], origin[2], src->sfx->name );
	}
	else
	{
		Snd_DPrintf( 3, "[StartSound: CHAN=%i, ATTN=%i, timer=%i, %s]\n",
		src->entchannel, src->attn_class, src->start_timer,
		src->sfx->name );
	}
*/
}

/*
 ==
 S_StartMusic()

 Note: background music has it own dedicated src-like struct, but gets
   its sfx from the general pool. (Memo to self: Remember This.)
 ==
 */
void S_StartMusic( char *qfilename )
{
	ALfloat gain;

	if( !sound_system_enable )
	{
		return;
	}

	if( music.playing )
	{
		if ( background_music->value
				&& !Q_strcasecmp( qfilename, music.sfx->name) )
		{ // music is enabled and this song already playing.
			return;
		}
	}

	if ( music.oalSource && qalIsSource( music.oalSource ) )
	{ // stop the music
		qalSourceStop( music.oalSource ); // ok from any state, per spec
		qalSourcei( music.oalSource, AL_BUFFER, AL_NONE ); // disconnect Buffer
		// special for music, only one Source using this Buffer
		// NOTE: not freeing sfx means 2 music files are resident, map music
		// and menu music. This uses more memory, but means the Ogg Vorbis
		// decode does not have to  be done everytime the menu is accessed.
		// calSoundFXFree( music.sfx );
		music.sfx = NULL;
	}
	music.playing = false;

	if( background_music->value == 0 )
	{ // disabled
		return;
	}

	music.sfx = S_FindName( qfilename, true ); // get a new sfx from pool
	if( music.sfx != NULL )
	{
		music.sfx->bg_music = true; // special case sfx marker
		calLoadSound( music.sfx ); // load the music file into a Buffer
	}
	else
	{
		Snd_DPrintf( 1, "No SFX for music file %s\n", qfilename );
	}

	if ( music.sfx->silent )
	{
		Snd_DPrintf( 1, "Music file %s failed load\n", qfilename);
		return;
	}

    // music file is loaded, attach to a source and play it
	if( music.oalSource == 0 )
	{ // haven't generated the dedicated Source yet, so do that first
		qalGenSources( 1, &( music.oalSource ) );
	}

	if( qalIsSource( music.oalSource ) )
	{ // got a Source
		// make sure Source is set to defaults
		qalSourcef( music.oalSource, AL_REFERENCE_DISTANCE,source_default.reference_distance );
		qalSourcef( music.oalSource, AL_MAX_DISTANCE, source_default.max_distance );
		qalSourcef( music.oalSource, AL_ROLLOFF_FACTOR, source_default.rolloff_factor );
		qalSourcef( music.oalSource, AL_PITCH, source_default.pitch );
		qalSourcef( music.oalSource, AL_MIN_GAIN, source_default.min_gain );
		qalSourcef( music.oalSource, AL_MAX_GAIN, source_default.max_gain );
		qalSourcefv( music.oalSource, AL_DIRECTION, zero_direction );
		qalSourcef( music.oalSource, AL_CONE_OUTER_GAIN, source_default.cone_outer_gain );
		qalSourcef( music.oalSource, AL_CONE_INNER_ANGLE, source_default.cone_inner_angle );
		qalSourcef( music.oalSource, AL_CONE_OUTER_ANGLE, source_default.cone_outer_angle );

		// set gain
		gain = (ALfloat)( background_music_vol->value );
		if( gain > 0.2f )
		{
			gain = 0.2f; // start softly, will go up in S_Update()
		}
		qalSourcef( music.oalSource, AL_GAIN, gain );

		// set for looping and "local"
		qalSourcei( music.oalSource, AL_LOOPING, AL_TRUE );
		qalSourcei( music.oalSource, AL_SOURCE_RELATIVE, AL_TRUE );
		qalSourcefv( music.oalSource, AL_POSITION, zero_position );
		qalSourcefv( music.oalSource, AL_VELOCITY, zero_velocity );

		// attach to buffer
		qalSourcei( music.oalSource, AL_BUFFER, music.sfx->oalBuffer );

		// and start playing
		qalSourcePlay( music.oalSource );
		music.playing = true;
	}
	else
	{ // failed to obtain a Source
		Com_Printf( "Sound Error: Disabling background music.\n" );
		Cvar_Set( "background_music", "0" );
	}

}

/*
==
 S_StartMenuMusic()
 S_StartMapMusic()

 Note: added to get better control of background music. These functions expect
 S_StartMusic() to do  checks for music enabled, etc.
 =
 */
void S_StartMenuMusic( void )
{
	S_StartMusic( "music/menumusic.wav" );
}

extern char map_music[260]; // declared and set in r_model.c

void S_StartMapMusic( void )
{
	S_StartMusic( map_music );
}

/*
 ==
 S_StartLocalSound()

 for sounds that are not position dependent

 always client-side origination ?

 ==
 */
void S_StartLocalSound( const char *qfilename )
{
	sfx_t *sfx;
	src_t *src;

	if( !sound_system_enable )
	{
		return;
	}

	// Start the new local sound
	sfx = S_FindName( qfilename, true );
	if( sfx != NULL )
	{
		calLoadSound( sfx );
	}
	if( sfx != NULL && !sfx->silent )
	{
		src = calNewLocalSrc( sfx, source_default.gain );
		if( src )
		{
			qalSourcePlay( src->oalSource );
		}
	}
}

/*
 ==
 calVelocityVector()

 calculate and validate the velocity vector

 deltaT : time difference in msecs, the frame time
 ==
 **/
typedef ALfloat ALvector[3];
typedef ALfloat ALpoint[3];

qboolean calVelocityVector( int deltaT, ALpoint in_previous,
        ALpoint in_current, ALvector out_velocity )
{
	qboolean valid;
	ALfloat mag_sq;
	ALfloat dT;

	dT = (ALfloat)(float)deltaT;
	out_velocity[0] = ( in_current[0] - in_previous[0] ) / dT;
	out_velocity[1] = ( in_current[1] - in_previous[1] ) / dT;
	out_velocity[2] = ( in_current[2] - in_previous[2] ) / dT;
	// note: so velocity is in QuakeUnits-per-millisecond

	// spawning, transporter jumps, and probably other stuff will cause
	// wildly inaccurate velocities. use squared values; no need for sqrt()
	mag_sq = out_velocity[0] * out_velocity[0] + out_velocity[1]
	        * out_velocity[1] + out_velocity[2] * out_velocity[2];
	valid = ( mag_sq <= velocity_max_valid );

	return valid;
}

/*
 ==
 calUpdateLoopSounds()

 manage the "automatic" looping sounds
 these are triggered by the entity_state_t .sound field
 the .number field and the .sound field identify the looping object

 ==
 */
void calUpdateLoopSounds( int deltaT, ALpoint listener_position )
{
	int num;
	int ix;
	entity_state_t *ent;
	src_t *src;
	srclink_t *srclink;
	int new_entity_index;
	int new_sound_field;
	qboolean src_marked;
	ALvector velocity;
	ALpoint old_position;
	ALpoint new_position;
	ALint state;
	ALfloat dist_sq;
	ALpoint source_position;
	qboolean valid_velocity;

	if( cl_paused->value || cls.state != ca_active || !cl.sound_prepped )
	{ // not in a game playing state
		return;
	}

	for( srclink = src_datahead; srclink; srclink = srclink->next )
	{ // first phase, mark all running looping sounds for stopping
		src = srclink->src;
		if( src->looping )
		{
			src->loop_mark = lpmk_stop; // may be overridden below
		}
	}

	for( ix = 0; ix < cl.frame.num_entities; ix++ )
	{ // second phase, update with continuation mark, or new one with start mark

		// get the data that implicitly implies a looping sound effect
		num = ( cl.frame.parse_entities + ix ) & ( MAX_PARSE_ENTITIES - 1 );
		ent = &cl_parse_entities[num];
		new_sound_field = ent->sound;

		if( new_sound_field == 0 )
		{ // nothing in .sound field, no looping
			continue;
		};
		new_entity_index = ent->number;

		src_marked = false;
		for( srclink = src_datahead; srclink; srclink = srclink->next )
		{ // second marking phase, mark for continuing the loop
			src = srclink->src;
			if( src->entity_index == new_entity_index && src->sound_field
			        == new_sound_field )
			{ // still exists, keep on looping
				src->loop_mark = lpmk_continue;
				src_marked = true;
			}
		}

		if( !src_marked )
		{ // this must be a new entity w/ .sound field, add a new looping src
			// but watch out for any obsolete speakers, music/*.wav
			CL_GetEntitySoundOrigin( new_entity_index, source_position );
			dist_sq = calDistanceFromListenerSq( listener_position,
			        source_position );
			if( dist_sq < looping_sq_cull_distance )
			{ // within cull distance
				src = calNewSrcAutoLoop( new_entity_index, new_sound_field );
			}
		}
	}

	// third phase, perform the actions according to the marks
	srclink = src_datahead;
	while( srclink != NULL )
	{
		src = srclink->src;
		srclink = srclink->next; // before list modification
		switch( src->loop_mark )
		{
		case lpmk_nonloop:
			break;

		case lpmk_start:
			qalSourcePlay( src->oalSource );
			//Snd_DPrintf( 7, "[lpmk_start %s]\n", src->sfx->name );
			break;

		case lpmk_continue:
			qalGetSourcei( src->oalSource, AL_SOURCE_RELATIVE, &state );
			if( state == AL_FALSE )
			{ // not Listener relative, so update the position and velocity
				qalGetSourcefv( src->oalSource, AL_POSITION, old_position );
				CL_GetEntitySoundOrigin( src->entity_index, new_position );
				qalSourcefv( src->oalSource, AL_POSITION, new_position );

				dist_sq = calDistanceFromListenerSq( listener_position,
				        new_position );
				if( dist_sq > (looping_sq_cull_distance + looping_cull_hysteresis ))
				{ // beyond cull distance w/ some hysteresis
					// might re-start if it comes inside of cull distance
					//Snd_DPrintf( 7, "[lpmk_continue culled %s]\n", src->sfx->name);
					qalSourceStop( src->oalSource );
					qalSourcei( src->oalSource, AL_BUFFER, AL_NONE );
					calSourceFree( src );
					break;
				}

				switch( src->velocity_state )
				{
				case velst_nodoppler:
					break;

				case velst_update:
					valid_velocity = calVelocityVector( deltaT, old_position,
					        new_position, velocity );
					if( valid_velocity )
					{
						qalSourcefv( src->oalSource, AL_VELOCITY, velocity );
					}
					else
					{
						src->velocity_state = velst_init;
						qalSourcefv( src->oalSource, AL_VELOCITY, zero_velocity );
					}
					break;

				case velst_init:
					valid_velocity = calVelocityVector( deltaT, old_position,
					        new_position, velocity );
					if( valid_velocity )
					{
						src->velocity_state = velst_update;
					}
					break;
				}
			}
			//Snd_DPrintf( 7, "[lpmk_continue %s]\n", src->sfx->name );
			break;

		case lpmk_stop:
			//Snd_DPrintf( 7, "[lpmk_stop %s]\n", src->sfx->name );
			qalSourceStop( src->oalSource );
			qalSourcei( src->oalSource, AL_BUFFER, AL_NONE );
			calSourceFree( src );
			break;

		default:
			break;
		}
	}
}

/*
 ==
 S_Update()

 Called every frame to update sound system state.
 ==
 */
void S_Update( vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up )
{
	src_t *src;
	srclink_t *srclink;
	ALint state;
	ALfloat gain;
	ALfloat lgain;
	ALfloat dgain;
	static int prev_systime = 0;
	int difftime;
	ALfloat old_lposition[3];
	ALfloat lorientation[6];
	ALfloat lvelocity[3];
	ALfloat new_origin[3];
	ALfloat old_origin[3];
	ALfloat velocity[3];
	qboolean valid_velocity;

	if( !sound_system_enable )
	{
		prev_systime = 0;
		return;
	}
	if( prev_systime == 0 )
	{ // first time
		prev_systime = Sys_Milliseconds();
		return;
	}
	difftime = Sys_Milliseconds() - prev_systime;
	prev_systime += difftime;

	qalGetError();

	// update Listener position, orientation and velocity
	qalGetListenerfv( AL_POSITION, old_lposition );
	lorientation[0] = v_forward[0];
	lorientation[1] = v_forward[1];
	lorientation[2] = v_forward[2];
	lorientation[3] = v_up[0];
	lorientation[4] = v_up[1];
	lorientation[5] = v_up[2];
	qalListenerfv( AL_POSITION, origin );
	qalListenerfv( AL_ORIENTATION, lorientation );
	valid_velocity = calVelocityVector( difftime, old_lposition, origin,
	        lvelocity );
	qalListenerfv( AL_VELOCITY, ( valid_velocity ? lvelocity : zero_velocity ) );

	// update listener gain, aka global volume
	gain = (ALfloat)s_volume->value;
	gain = ( gain > 1.0f ? 1.0f : ( gain < 0.0f ? 0.0f : gain ) ); // clamp
	qalGetListenerf( AL_GAIN, &lgain );
	dgain = gain - lgain;
	if( dgain < (ALfloat) -0.05 )
	{ // turn gain down,
		qalListenerf( AL_GAIN, gain );
	}
	else if( dgain > 0.05 )
	{ // turn gain up, incrementally
		lgain += (ALfloat)0.005f;
		qalListenerf( AL_GAIN, lgain );
	}

	// do "automatic" looping sounds
	calUpdateLoopSounds( difftime, origin );

	// do other sound updates
	srclink = src_datahead;
	while( srclink != NULL )
	{
		src = srclink->src;
		srclink = srclink->next; // advance pointer before link list is updated
		if( src->looping )
		{ // handled above
			continue;
		}

		// delayed start sound effect
		if( src->start_timer > 0 )
		{ // delayed start
			src->start_timer -= difftime;
			if( src->start_timer <= 5 )
			{
				src->start_timer = 0;
				if( src->entity_index != 0 && !src->fixed_origin )
				{
					qalGetSourcei( src->oalSource, AL_SOURCE_RELATIVE, &state );
					if( state == AL_FALSE )
					{ // not Listener relative, so update the position
						CL_GetEntitySoundOrigin( src->entity_index, new_origin );
						qalSourcefv( src->oalSource, AL_POSITION, new_origin );
					}
				}
				qalSourcePlay( src->oalSource );
				//Snd_DPrintf( 3, "[Delayed start %s]\n", src->sfx->name);
			}
			continue;
		}

		// update sounds according to current state
		qalGetSourcei( src->oalSource, AL_SOURCE_STATE, &state );
		switch( state )
		{
		case AL_PLAYING:
			if( src->stop_timer )
			{ // stop initiated by SFX with same entnum/CHAN_*/ATTN_*
				src->stop_timer -= difftime;
				if( src->stop_timer < 5 )
				{
					//Snd_DPrintf( 3, "[Timed stop %s]\n", src->sfx->name);
					src->stop_timer = 0;
					qalSourceStop( src->oalSource );
					qalSourcei( src->oalSource, AL_BUFFER, AL_NONE );
					calSourceFree( src );
					break;
				}
			}
			if( src->fixed_origin )
			{ // special case, leave position as is
				break;
			}
			qalGetSourcei( src->oalSource, AL_SOURCE_RELATIVE, &state );
			if( state == AL_TRUE )
			{ // 'travels' with listener
				break;
			}
			if( src->entity_index != 0 )
			{ // there is an entity. update position and velocity

				qalGetSourcefv( src->oalSource, AL_POSITION, old_origin );
				CL_GetEntitySoundOrigin( src->entity_index, new_origin );
				qalSourcefv( src->oalSource, AL_POSITION, new_origin );

				switch( src->velocity_state )
				{
				case velst_nodoppler:
					break;

				case velst_update:
					valid_velocity = calVelocityVector( difftime, old_origin,
					        new_origin, velocity );
					if( valid_velocity )
					{
						qalSourcefv( src->oalSource, AL_VELOCITY, velocity );
					}
					else
					{ // re-initialize
						src->velocity_state = velst_init;
						qalSourcefv( src->oalSource, AL_VELOCITY, zero_velocity );
					}
					break;

				case velst_init:
					valid_velocity = calVelocityVector( difftime, old_origin,
					        new_origin, velocity );
					if( valid_velocity )
					{
						src->velocity_state = velst_update;
					}
					break;

				default:
					break;
				}
			}
			break;

		case AL_STOPPED: // has finished playing, free the src
			//Snd_DPrintf( 3, "[Done playing %s]\n", src->sfx->name);
			qalSourcei( src->oalSource, AL_BUFFER, AL_NONE );
			calSourceFree( src );
			break;

		case AL_INITIAL:
		case AL_PAUSED:
		default:
			break;
		}
	}

	if( music.playing )
	{ // update per background cvars: enable and volume
		if( background_music->modified )
		{ // either on->off or off->on
			if(	background_music->value == 0 )
			{ // here, turning down probably better than fully shutting down
				qalSourcef( music.oalSource, AL_GAIN, (ALfloat)0.0f );
			}
		}
		if ( background_music->value )
		{
			gain = background_music_vol->value;
			gain = gain < 0.0 ? 0.0 : (gain > 1.0 ? 1.0 : gain ); //clamp
			qalGetSourcef( music.oalSource, AL_GAIN, &lgain );
			dgain = gain - lgain; // difference between setting and current
			if( dgain < (ALfloat)(-0.05f) )
			{ // turn gain down to setting
				qalSourcef( music.oalSource, AL_GAIN, gain );
			}
			else if( dgain > (ALfloat)0.05f )
			{ // turn gain up, incrementally
				lgain += (ALfloat)0.005f;
				qalSourcef( music.oalSource, AL_GAIN, lgain );
			}
			background_music->modified = false;
			background_music_vol->modified = false;
		}
	}

	if ( s_doppler->modified )
	{ // for console Doppler factor changes
		S_UpdateDopplerFactor();
	}

	calErrorCheckAL( __LINE__ );
}

/*
 ==
 S_Play()

 target of console command: playsounds <file>[.wav] [<file>[.wav]] ...
 plays all simultaneously, possibly by design, in order to test mixing.

 changed command from 'play' to 'playsound' to prevent confusion
 ==
 */
void S_Play( void )
{
	int ix;
	char filename[MAX_QPATH];

	Com_Printf( "playsound starting..." );
	for( ix = 1; ix < Cmd_Argc(); ix++ )
	{
		if( !strrchr( Cmd_Argv( ix ), '.' ) )
		{
			strncpy( filename, Cmd_Argv( ix ), sizeof( filename ) - 5 );
			strcat( filename, ".wav" );
		}
		else
		{
			strncpy( filename, Cmd_Argv( ix ), sizeof( filename ) - 1 );
		}
		S_StartLocalSound( filename );
	}
	Com_Printf( " use stopsound command to stop.\n" );
}

/*
 ==
 S_SoundInfo_f()

 target of console command: soundinfo
 ==
 */
void S_SoundInfo_f( void )
{
	ALCdevice *pDevice;
	ALCcontext *pContext;

	// get current device in current context
	pContext = qalcGetCurrentContext();
	pDevice = qalcGetContextsDevice( pContext );

	// dump some info
	calContextInfo( pDevice );

}

/*
 ==
 S_SoundList()

 target of console command: soundlist
 ==
 */
void S_SoundList( void )
{
	sfxlink_t *sfxlink;
	sfx_t *sfx = NULL;
	size_t total = 0;


	for( sfxlink = sfx_datahead; sfxlink != NULL; sfxlink = sfxlink->next )
	{
		sfx = sfxlink->sfx;
		if ( !sfx->silent && sfx->registration_sequence )
		{
			if ( sfx->buffered )
			{
				if ( sfx->name[0] == '#')
				{
					Com_Printf("(%2db) %10i : %s\n", sfx->byte_width*8,
							sfx->byte_count, &sfx->name[1]);
				}
				else
				{
					Com_Printf("(%2db) %10i : sound/%s\n", sfx->byte_width*8,
							sfx->byte_count,
							(sfx->aliased ? sfx->truename : sfx->name ) );
				}
				total += sfx->byte_count;
			}
		}
	}
	Com_Printf ("Total resident: %i\n", total);

}

/*
 ==
 S_Activate()

 Windows only. gain window focus/lose window focus
 ==
 */
void S_Activate( qboolean active )
{
	/* TODO: may not be anything to do here. */
	Snd_DPrintf( 1, "S_Activate() is a stub.\n" );
}

/**
 ==
 sndCmdInit()
 sndCmdRemove()

 sound related console commands
 ==
 **/
void sndCmdInit( void )
{
	Cmd_AddCommand( "playsound", S_Play );
	Cmd_AddCommand( "stopsound", S_StopAllSounds );
	Cmd_AddCommand( "soundlist", S_SoundList );
	Cmd_AddCommand( "soundinfo", S_SoundInfo_f );
}

void sndCmdRemove( void )
{
	Cmd_RemoveCommand( "playsound" );
	Cmd_RemoveCommand( "stopsound" );
	Cmd_RemoveCommand( "soundlist" );
	Cmd_RemoveCommand( "soundinfo" );
}

/*
 ==
 sndCvarInit()

 sound related console variables for OpenAL

 The following cvar's are not implemented in this version
 s_khz, s_loadas8bit, s_mixahead, s_show, s_testsound, s_primary

 ==
 */
void sndCvarInit( void )
{
	char cvarset[16];

	s_initsound = Cvar_Get( "s_initsound", "1", 0 );
	s_volume = Cvar_Get( "s_volume", "0.1", CVAR_ARCHIVE );
	// OpenAL CVARs
	s_doppler = Cvar_Get( "s_doppler", "0.0", CVAR_ARCHIVE );
	Com_sprintf(cvarset, sizeof(cvarset),"%i", MAX_SRC_DEFAULT );
	s_maxsources = Cvar_Get( "s_maxsources", cvarset , CVAR_ARCHIVE );
	Com_sprintf(cvarset, sizeof(cvarset),"%i", MIN_SRC_DEFAULT );
	s_minsources = Cvar_Get( "s_minsources", cvarset, CVAR_ARCHIVE );
	s_device = Cvar_Get( "s_device", "Default", CVAR_ARCHIVE );
	// for debug
	snd_developer = Cvar_Get( "snd_developer", "0", 0);

}

