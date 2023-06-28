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
 * qal.c
 *
 * OpenAL functions
 *
 * All OpenAL 1.1 functions are declared here, but may not currently be used.
 *
 * Note: functions wrap the function pointers for ease of development.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined WIN32_VARIANT
#  include <windows.h>
#endif

#if defined HAVE_AL_H
#include <al.h>
#include <alc.h>
#elif defined HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined HAVE_OPENAL_AL_H
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#error OpenAL header includes not defined.
#endif

#include "client.h"
#include "qal.h"

/*
 * OpenAL symbol addresses
 */
LPALENABLE pqalEnable;
LPALDISABLE pqalDisable;
LPALISENABLED pqalIsEnabled;
LPALGETBOOLEANV pqalGetBooleanv;
LPALGETINTEGERV pqalGetIntegerv;
LPALGETSTRING pqalGetString;
LPALGETFLOATV pqalGetFloatv;
LPALGETDOUBLEV pqalGetDoublev;
LPALGETBOOLEAN pqalGetBoolean;
LPALGETINTEGER pqalGetInteger;
LPALGETFLOAT pqalGetFloat;
LPALGETDOUBLE pqalGetDouble;
LPALGETERROR pqalGetError;
LPALISEXTENSIONPRESENT pqalIsExtensionPresent;
LPALGETPROCADDRESS pqalGetProcAddress;
LPALGETENUMVALUE pqalGetEnumValue;
LPALLISTENERF pqalListenerf;
LPALLISTENER3F pqalListener3f;
LPALLISTENERFV pqalListenerfv;
LPALLISTENERI pqalListeneri;
LPALLISTENER3I pqalListener3i;
LPALLISTENERIV pqalListeneriv;
LPALGETLISTENERF pqalGetListenerf;
LPALGETLISTENER3F pqalGetListener3f;
LPALGETLISTENERFV pqalGetListenerfv;
LPALGETLISTENERI pqalGetListeneri;
LPALGETLISTENER3I pqalGetListener3i;
LPALGETLISTENERIV pqalGetListeneriv;
LPALGENSOURCES pqalGenSources;
LPALDELETESOURCES pqalDeleteSources;
LPALISSOURCE pqalIsSource;
LPALSOURCEF pqalSourcef;
LPALSOURCE3F pqalSource3f;
LPALSOURCEFV pqalSourcefv;
LPALSOURCEI pqalSourcei;
LPALSOURCE3I pqalSource3i;
LPALSOURCEIV pqalSourceiv;
LPALGETSOURCEF pqalGetSourcef;
LPALGETSOURCE3F pqalGetSource3f;
LPALGETSOURCEFV pqalGetSourcefv;
LPALGETSOURCEI pqalGetSourcei;
LPALGETSOURCE3I pqalGetSource3i;
LPALGETSOURCEIV pqalGetSourceiv;
LPALSOURCEPLAYV pqalSourcePlayv;
LPALSOURCESTOPV pqalSourceStopv;
LPALSOURCEREWINDV pqalSourceRewindv;
LPALSOURCEPAUSEV pqalSourcePausev;
LPALSOURCEPLAY pqalSourcePlay;
LPALSOURCESTOP pqalSourceStop;
LPALSOURCEREWIND pqalSourceRewind;
LPALSOURCEPAUSE pqalSourcePause;
LPALSOURCEQUEUEBUFFERS pqalSourceQueueBuffers;
LPALSOURCEUNQUEUEBUFFERS pqalSourceUnqueueBuffers;
LPALGENBUFFERS pqalGenBuffers;
LPALDELETEBUFFERS pqalDeleteBuffers;
LPALISBUFFER pqalIsBuffer;
LPALBUFFERDATA pqalBufferData;
LPALBUFFERF pqalBufferf;
LPALBUFFER3F pqalBuffer3f;
LPALBUFFERFV pqalBufferfv;
LPALBUFFERI pqalBufferi;
LPALBUFFER3I pqalBuffer3i;
LPALBUFFERIV pqalBufferiv;
LPALGETBUFFERF pqalGetBufferf;
LPALGETBUFFER3F pqalGetBuffer3f;
LPALGETBUFFERFV pqalGetBufferfv;
LPALGETBUFFERI pqalGetBufferi;
LPALGETBUFFER3I pqalGetBuffer3i;
LPALGETBUFFERIV pqalGetBufferiv;
LPALDOPPLERFACTOR pqalDopplerFactor;
LPALDOPPLERVELOCITY pqalDopplerVelocity;
LPALSPEEDOFSOUND pqalSpeedOfSound;
LPALDISTANCEMODEL pqalDistanceModel;

LPALCCREATECONTEXT pqalcCreateContext;
LPALCMAKECONTEXTCURRENT pqalcMakeContextCurrent;
LPALCPROCESSCONTEXT pqalcProcessContext;
LPALCSUSPENDCONTEXT pqalcSuspendContext;
LPALCDESTROYCONTEXT pqalcDestroyContext;
LPALCGETCURRENTCONTEXT pqalcGetCurrentContext;
LPALCGETCONTEXTSDEVICE pqalcGetContextsDevice;
LPALCOPENDEVICE pqalcOpenDevice;
LPALCCLOSEDEVICE pqalcCloseDevice;
LPALCGETERROR pqalcGetError;
LPALCISEXTENSIONPRESENT pqalcIsExtensionPresent;
LPALCGETPROCADDRESS pqalcGetProcAddress;
LPALCGETENUMVALUE pqalcGetEnumValue;
LPALCGETSTRING pqalcGetString;
LPALCGETINTEGERV pqalcGetIntegerv;
LPALCCAPTUREOPENDEVICE pqalcCaptureOpenDevice;
LPALCCAPTURECLOSEDEVICE pqalcCaptureCloseDevice;
LPALCCAPTURESTART pqalcCaptureStart;
LPALCCAPTURESTOP pqalcCaptureStop;
LPALCCAPTURESAMPLES pqalcCaptureSamples;


/*
 * Renderer State management
 */
void qalEnable( ALenum capability )
{
	( *pqalEnable )( capability );
}

void qalDisable( ALenum capability )
{
	( *pqalDisable )( capability );
}

ALboolean qalIsEnabled( ALenum capability )
{
	return ( *pqalIsEnabled )( capability );
}

/* State retrieval
 */
const ALchar* qalGetString( ALenum param )
{
	return ( *pqalGetString )( param );
}

void qalGetBooleanv( ALenum param, ALboolean* data )
{
	( *pqalGetBooleanv )( param, data );
}

void qalGetIntegerv( ALenum param, ALint* data )
{
	( *pqalGetIntegerv )( param, data );
}

void qalGetFloatv( ALenum param, ALfloat* data )
{
	( *pqalGetFloatv )( param, data );
}

void qalGetDoublev( ALenum param, ALdouble* data )
{
	( *pqalGetDoublev )( param, data );
}

ALboolean qalGetBoolean( ALenum param )
{
	return ( *pqalGetBoolean )( param );
}

ALint qalGetInteger( ALenum param )
{
	return ( *pqalGetInteger )( param );
}

ALfloat qalGetFloat( ALenum param )
{
	return ( *pqalGetFloat )( param );
}

ALdouble qalGetDouble( ALenum param )
{
	return ( *pqalGetDouble )( param );
}

/*
 * Error support.
 * Obtain the most recent error generated in the AL state machine.
 */
ALenum qalGetError( void )
{
	return ( *pqalGetError )();
}

/*
 * Extension support.
 * Query for the presence of an extension, and obtain any appropriate
 * function pointers and enum values.
 */
ALboolean qalIsExtensionPresent( const ALchar* extname )
{
	return ( *pqalIsExtensionPresent )( extname );
}

void* qalGetProcAddress( const ALchar* fname )
{
	return ( *pqalGetProcAddress )( fname );
}

ALenum qalGetEnumValue( const ALchar* ename )
{
	return ( *pqalGetEnumValue )( ename );
}

/*
 * LISTENER
 * Listener represents the location and orientation of the
 * 'user' in 3D-space.
 *
 * Properties include: -
 *
 * Gain         AL_GAIN         ALfloat
 * Position     AL_POSITION     ALfloat[3]
 * Velocity     AL_VELOCITY     ALfloat[3]
 * Orientation  AL_ORIENTATION  ALfloat[6] (Forward then Up vectors)
 */

/*
 * Set Listener parameters
 */
void qalListenerf( ALenum param, ALfloat value )
{
	( *pqalListenerf )( param, value );
}

void qalListener3f( ALenum param, ALfloat value1, ALfloat value2,
        ALfloat value3 )
{
	( *pqalListener3f )( param, value1, value2, value3 );
}

void qalListenerfv( ALenum param, const ALfloat* values )
{
	( *pqalListenerfv )( param, values );
}

void qalListeneri( ALenum param, ALint value )
{
	( *pqalListeneri )( param, value );
}

void qalListener3i( ALenum param, ALint value1, ALint value2, ALint value3 )
{
	( *pqalListener3i )( param, value1, value2, value3 );
}

void qalListeneriv( ALenum param, const ALint* values )
{
	( *pqalListeneriv )( param, values );
}

/*
 * Get Listener parameters
 */
void qalGetListenerf( ALenum param, ALfloat* value )
{
	( *pqalGetListenerf )( param, value );
}

void qalGetListener3f( ALenum param, ALfloat *value1, ALfloat *value2,
        ALfloat *value3 )
{
	( *pqalGetListener3f )( param, value1, value2, value3 );
}

void qalGetListenerfv( ALenum param, ALfloat* values )
{
	( *pqalGetListenerfv )( param, values );
}

void qalGetListeneri( ALenum param, ALint* value )
{
	( *pqalGetListeneri )( param, value );
}

void qalGetListener3i( ALenum param, ALint *value1, ALint *value2,
        ALint *value3 )
{
	( *pqalGetListener3i )( param, value1, value2, value3 );
}

void qalGetListeneriv( ALenum param, ALint* values )
{
	( *pqalGetListeneriv )( param, values );
}

/**
 * SOURCE
 * Sources represent individual sound objects in 3D-space.
 * Sources take the PCM data provided in the specified Buffer,
 * apply Source-specific modifications, and then
 * submit them to be mixed according to spatial arrangement etc.
 *
 * Properties include: -
 *
 * Gain                              AL_GAIN                 ALfloat
 * Min Gain                          AL_MIN_GAIN             ALfloat
 * Max Gain                          AL_MAX_GAIN             ALfloat
 * Position                          AL_POSITION             ALfloat[3]
 * Velocity                          AL_VELOCITY             ALfloat[3]
 * Direction                         AL_DIRECTION            ALfloat[3]
 * Head Relative Mode                AL_SOURCE_RELATIVE      ALint (AL_TRUE,AL_FALSE)
 * Reference Distance                AL_REFERENCE_DISTANCE   ALfloat
 * Max Distance                      AL_MAX_DISTANCE         ALfloat
 * RollOff Factor                    AL_ROLLOFF_FACTOR       ALfloat
 * Inner Angle                       AL_CONE_INNER_ANGLE     ALint or ALfloat
 * Outer Angle                       AL_CONE_OUTER_ANGLE     ALint or ALfloat
 * Cone Outer Gain                   AL_CONE_OUTER_GAIN      ALint or ALfloat
 * Pitch                             AL_PITCH                ALfloat
 * Looping                           AL_LOOPING              ALint (AL_TRUE,AL_FALSE)
 * MS Offset                         AL_MSEC_OFFSET          ALint or ALfloat
 * Byte Offset                       AL_BYTE_OFFSET          ALint or ALfloat
 * Sample Offset                     AL_SAMPLE_OFFSET        ALint or ALfloat
 * Attached Buffer                   AL_BUFFER               ALint
 * State (Query only)                AL_SOURCE_STATE         ALint
 * Buffers Queued (Query only)       AL_BUFFERS_QUEUED       ALint
 * Buffers Processed (Query only)    AL_BUFFERS_PROCESSED    ALint
 */

/* Create Source objects */
void qalGenSources( ALsizei n, ALuint* sources )
{
	( *pqalGenSources )( n, sources );
}

/* Delete Source objects */
void qalDeleteSources( ALsizei n, const ALuint* sources )
{
	( *pqalDeleteSources )( n, sources );
}

/* Verify a handle is a valid Source */
ALboolean qalIsSource( ALuint sid )
{
	return ( *pqalIsSource )( sid );
}

/*
 * Set Source parameters
 */
void qalSourcef( ALuint sid, ALenum param, ALfloat value )
{
	( *pqalSourcef )( sid, param, value );
}

void qalSource3f( ALuint sid, ALenum param, ALfloat value1, ALfloat value2,
        ALfloat value3 )
{
	( *pqalSource3f )( sid, param, value1, value2, value3 );
}

void qalSourcefv( ALuint sid, ALenum param, const ALfloat* values )
{
	( *pqalSourcefv )( sid, param, values );
}

void qalSourcei( ALuint sid, ALenum param, ALint value )
{
	( *pqalSourcei )( sid, param, value );
}

void qalSource3i( ALuint sid, ALenum param, ALint value1, ALint value2,
        ALint value3 )
{
	( *pqalSource3i )( sid, param, value1, value2, value3 );
}

void qalSourceiv( ALuint sid, ALenum param, const ALint* values )
{
	( *pqalSourceiv )( sid, param, values );
}

/*
 * Get Source parameters
 */
void qalGetSourcef( ALuint sid, ALenum param, ALfloat* value )
{
	( *pqalGetSourcef )( sid, param, value );
}

void qalGetSource3f( ALuint sid, ALenum param, ALfloat* value1,
        ALfloat* value2, ALfloat* value3 )
{
	( *pqalGetSource3f )( sid, param, value1, value2, value3 );
}

void qalGetSourcefv( ALuint sid, ALenum param, ALfloat* values )
{
	( *pqalGetSourcefv )( sid, param, values );
}

void qalGetSourcei( ALuint sid, ALenum param, ALint* value )
{
	( *pqalGetSourcei )( sid, param, value );
}

void qalGetSource3i( ALuint sid, ALenum param, ALint* value1, ALint* value2,
        ALint* value3 )
{
	( *pqalGetSource3i )( sid, param, value1, value2, value3 );
}

void qalGetSourceiv( ALuint sid, ALenum param, ALint* values )
{
	( *pqalGetSourceiv )( sid, param, values );
}

/*
 * Source vector based playback calls
 */
/* Play, replay, or resume (if paused) a list of Sources */
void qalSourcePlayv( ALsizei ns, const ALuint *sids )
{
	( *pqalSourcePlayv )( ns, sids );
}

/* Stop a list of Sources */
void qalSourceStopv( ALsizei ns, const ALuint *sids )
{
	( *pqalSourceStopv )( ns, sids );
}

/* Rewind a list of Sources */
void qalSourceRewindv( ALsizei ns, const ALuint *sids )
{
	( *pqalSourceRewindv )( ns, sids );
}

/* Pause a list of Sources */
void qalSourcePausev( ALsizei ns, const ALuint *sids )
{
	( *pqalSourcePausev )( ns, sids );
}

/*
 * Source based playback calls
 */
/* Play, replay, or resume a Source */
void qalSourcePlay( ALuint sid )
{
	( *pqalSourcePlay )( sid );
}

/* Stop a Source */
void qalSourceStop( ALuint sid )
{
	( *pqalSourceStop )( sid );
}

/* Rewind a Source (set playback postiton to beginning) */
void qalSourceRewind( ALuint sid )
{
	( *pqalSourceRewind )( sid );
}

/* Pause a Source */
void qalSourcePause( ALuint sid )
{
	( *pqalSourcePause )( sid );
}

/*
 * Source Queuing
 */
void qalSourceQueueBuffers( ALuint sid, ALsizei numEntries, const ALuint *bids )
{
	( *pqalSourceQueueBuffers )( sid, numEntries, bids );
}
void qalSourceUnqueueBuffers( ALuint sid, ALsizei numEntries, ALuint *bids )
{
	( *pqalSourceUnqueueBuffers )( sid, numEntries, bids );
}

/**
 * BUFFER
 * Buffer objects are storage space for sample data.
 * Buffers are referred to by Sources. One Buffer can be used
 * by multiple Sources.
 *
 * Properties include: -
 *
 * Frequency (Query only)    AL_FREQUENCY      ALint
 * Size (Query only)         AL_SIZE           ALint
 * Bits (Query only)         AL_BITS           ALint
 * Channels (Query only)     AL_CHANNELS       ALint
 */

/* Create Buffer objects */
void qalGenBuffers( ALsizei n, ALuint* buffers )
{
	( *pqalGenBuffers )( n, buffers );
}

/* Delete Buffer objects */
void qalDeleteBuffers( ALsizei n, const ALuint* buffers )
{
	( *pqalDeleteBuffers )( n, buffers );
}

/* Verify a handle is a valid Buffer */
ALboolean qalIsBuffer( ALuint bid )
{
	return ( *pqalIsBuffer )( bid );
}

/* Specify the data to be copied into a buffer */
void qalBufferData( ALuint bid, ALenum format, const ALvoid* data,
        ALsizei size, ALsizei freq )
{
	( *pqalBufferData )( bid, format, data, size, freq );
}

/*
 * Set Buffer parameters
 */
void qalBufferf( ALuint bid, ALenum param, ALfloat value )
{
	( *pqalBufferf )( bid, param, value );
}

void qalBuffer3f( ALuint bid, ALenum param, ALfloat value1, ALfloat value2,
        ALfloat value3 )
{
	( *pqalBuffer3f )( bid, param, value1, value2, value3 );
}

void qalBufferfv( ALuint bid, ALenum param, const ALfloat* values )
{
	( *pqalBufferfv )( bid, param, values );
}

void qalBufferi( ALuint bid, ALenum param, ALint value )
{
	( *pqalBufferi )( bid, param, value );
}

void qalBuffer3i( ALuint bid, ALenum param, ALint value1, ALint value2,
        ALint value3 )
{
	( *pqalBuffer3i )( bid, param, value1, value2, value3 );
}

void qalBufferiv( ALuint bid, ALenum param, const ALint* values )
{
	( *pqalBufferiv )( bid, param, values );
}

/*
 * Get Buffer parameters
 */
void qalGetBufferf( ALuint bid, ALenum param, ALfloat* value )
{
	( *pqalGetBufferf )( bid, param, value );
}

void qalGetBuffer3f( ALuint bid, ALenum param, ALfloat* value1,
        ALfloat* value2, ALfloat* value3 )
{
	( *pqalGetBuffer3f )( bid, param, value1, value2, value3 );
}

void qalGetBufferfv( ALuint bid, ALenum param, ALfloat* values )
{
	( *pqalGetBufferfv )( bid, param, values );
}

void qalGetBufferi( ALuint bid, ALenum param, ALint* value )
{
	( *pqalGetBufferi )( bid, param, value );
}

void qalGetBuffer3i( ALuint bid, ALenum param, ALint* value1, ALint* value2,
        ALint* value3 )
{
	( *pqalGetBuffer3i )( bid, param, value1, value2, value3 );
}

void qalGetBufferiv( ALuint bid, ALenum param, ALint* values )
{
	( *pqalGetBufferiv )( bid, param, values );
}

/*
 * Global Parameters
 */
void qalDopplerFactor( ALfloat value )
{
	( *pqalDopplerFactor )( value );
}

void qalDopplerVelocity( ALfloat value )
{
	( *pqalDopplerVelocity )( value );
}

void qalSpeedOfSound( ALfloat value )
{
	( *pqalSpeedOfSound )( value );
}

void qalDistanceModel( ALenum distanceModel )
{
	( *pqalDistanceModel )( distanceModel );
}

/*
 * Context Management
 */
ALCcontext* qalcCreateContext( ALCdevice *device, const ALCint* attrlist )
{
	return ( *pqalcCreateContext )( device, attrlist );
}

ALCboolean qalcMakeContextCurrent( ALCcontext *context )
{
	return ( *pqalcMakeContextCurrent )( context );

}

void qalcProcessContext( ALCcontext *context )
{
	( *pqalcProcessContext )( context );

}

void qalcSuspendContext( ALCcontext *context )
{
	( *pqalcSuspendContext )( context );

}

void qalcDestroyContext( ALCcontext *context )
{
	( *pqalcDestroyContext )( context );

}

ALCcontext* qalcGetCurrentContext( void )
{
	return ( *pqalcGetCurrentContext )();
}

ALCdevice* qalcGetContextsDevice( ALCcontext *context )
{
	return ( *pqalcGetContextsDevice )( context );

}

/*
 * Device Management
 */
ALCdevice* qalcOpenDevice( const ALCchar *devicename )
{
	return ( *pqalcOpenDevice )( devicename );
}

ALCboolean qalcCloseDevice( ALCdevice *device )
{
	return ( *pqalcCloseDevice )( device );
}

/*
 * Error support.
 * Obtain the most recent Context error
 */
ALCenum qalcGetError( ALCdevice *device )
{
	return ( *pqalcGetError )( device );
}

/*
 * Extension support.
 * Query for the presence of an extension, and obtain any appropriate
 * function pointers and enum values.
 */
ALCboolean qalcIsExtensionPresent( ALCdevice *device, const ALCchar *extname )
{
	return ( *pqalcIsExtensionPresent )( device, extname );
}

void* qalcGetProcAddress( ALCdevice *device, const ALCchar *funcname )
{
	return ( *pqalcGetProcAddress )( device, funcname );
}

ALCenum qalcGetEnumValue( ALCdevice *device, const ALCchar *enumname )
{
	return ( *pqalcGetEnumValue )( device, enumname );
}

/*
 * Query functions
 */
const ALCchar* qalcGetString( ALCdevice *device, ALCenum param )
{
	return ( *pqalcGetString )( device, param );
}

void qalcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size,
        ALCint *data )
{
	( *pqalcGetIntegerv )( device, param, size, data );
}

/*
 * Capture functions
 */
ALCdevice* qalcCaptureOpenDevice( const ALCchar *devicename, ALCuint frequency,
        ALCenum format, ALCsizei buffersize )
{
	return ( *pqalcCaptureOpenDevice )( devicename, frequency, format,
	        buffersize );
}

ALCboolean qalcCaptureCloseDevice( ALCdevice *device )
{
	return ( *pqalcCaptureCloseDevice )( device );
}

void qalcCaptureStart( ALCdevice *device )
{
	( *pqalcCaptureStart )( device );
}

void qalcCaptureStop( ALCdevice *device )
{
	( *pqalcCaptureStop )( device );
}

void qalcCaptureSamples( ALCdevice *device, ALCvoid *buffer, ALCsizei samples )
{
	( *pqalcCaptureSamples )( device, buffer, samples );
}

