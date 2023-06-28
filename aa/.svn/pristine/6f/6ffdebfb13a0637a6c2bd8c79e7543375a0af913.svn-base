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
 * qal_win.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>

#include <al.h>
#include <alc.h>

#include "client/client.h"
#include "client/qal.h"

/*
 * OpenAL Library
 */
const char libopenal_name[] = OPENAL_DRIVER ;
HINSTANCE hinstOpenAL; // HINSTANCE for the OpenAL library
qboolean dlsym_error;

/*
 ==
 get_dlsym()

 get symbol from DLL
 ==
 */
static void get_dlsym( const char* symbol_name, void** symbol_addr )
{
	char *buf = NULL;

	*symbol_addr = GetProcAddress( hinstOpenAL, symbol_name );
	if ( *symbol_addr == NULL )
	{
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		        GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPTSTR ) & buf,
		        0, NULL );
		Com_Printf( "%s\n", buf );
		dlsym_error = true;
	}
}

/*
 ==
 QAL_Init()
 QAL_Shutdown()

 Dynamically link/unlink OpenAL .dll
 ==
 */
qboolean QAL_Init( void )
{
	char *buf = NULL;

	hinstOpenAL = LoadLibrary( libopenal_name );
	if( hinstOpenAL == NULL )
	{
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		        GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPTSTR ) & buf,
		        0, NULL );
		Com_Printf( "%s\n", buf );
		return false;
	}

	dlsym_error = false;
	get_dlsym( "alEnable", (void**) &pqalEnable );
	get_dlsym( "alDisable", (void**) &pqalDisable );
	get_dlsym( "alIsEnabled", (void**) &pqalIsEnabled );
	get_dlsym( "alGetBooleanv", (void**) &pqalGetBooleanv );
	get_dlsym( "alGetIntegerv", (void**) &pqalGetIntegerv );
	get_dlsym( "alGetString", (void**) &pqalGetString );
	get_dlsym( "alGetFloatv", (void**) &pqalGetFloatv );
	get_dlsym( "alGetDoublev", (void**) &pqalGetDoublev );
	get_dlsym( "alGetBoolean", (void**) &pqalGetBoolean );
	get_dlsym( "alGetInteger", (void**) &pqalGetInteger );
	get_dlsym( "alGetFloat", (void**) &pqalGetFloat );
	get_dlsym( "alGetDouble", (void**) &pqalGetDouble );
	get_dlsym( "alGetError", (void**) &pqalGetError );
	get_dlsym( "alIsExtensionPresent", (void**) &pqalIsExtensionPresent );
	get_dlsym( "alGetProcAddress", (void**) &pqalGetProcAddress );
	get_dlsym( "alGetEnumValue", (void**) &pqalGetEnumValue );
	get_dlsym( "alListenerf", (void**) &pqalListenerf );
	get_dlsym( "alListener3f", (void**) &pqalListener3f );
	get_dlsym( "alListenerfv", (void**) &pqalListenerfv );
	get_dlsym( "alListeneri", (void**) &pqalListeneri );
	get_dlsym( "alListener3i", (void**) &pqalListener3i );
	get_dlsym( "alListeneriv", (void**) &pqalListeneriv );
	get_dlsym( "alGetListenerf", (void**) &pqalGetListenerf );
	get_dlsym( "alGetListener3f", (void**) &pqalGetListener3f );
	get_dlsym( "alGetListenerfv", (void**) &pqalGetListenerfv );
	get_dlsym( "alGetListeneri", (void**) &pqalGetListeneri );
	get_dlsym( "alGetListener3i", (void**) &pqalGetListener3i );
	get_dlsym( "alGetListeneriv", (void**) &pqalGetListeneriv );
	get_dlsym( "alGenSources", (void**) &pqalGenSources );
	get_dlsym( "alDeleteSources", (void**) &pqalDeleteSources );
	get_dlsym( "alIsSource", (void**) &pqalIsSource );
	get_dlsym( "alSourcef", (void**) &pqalSourcef );
	get_dlsym( "alSource3f", (void**) &pqalSource3f );
	get_dlsym( "alSourcefv", (void**) &pqalSourcefv );
	get_dlsym( "alSourcei", (void**) &pqalSourcei );
	get_dlsym( "alSource3i", (void**) &pqalSource3i );
	get_dlsym( "alSourceiv", (void**) &pqalSourceiv );
	get_dlsym( "alGetSourcef", (void**) &pqalGetSourcef );
	get_dlsym( "alGetSource3f", (void**) &pqalGetSource3f );
	get_dlsym( "alGetSourcefv", (void**) &pqalGetSourcefv );
	get_dlsym( "alGetSourcei", (void**) &pqalGetSourcei );
	get_dlsym( "alGetSource3i", (void**) &pqalGetSource3i );
	get_dlsym( "alGetSourceiv", (void**) &pqalGetSourceiv );
	get_dlsym( "alSourcePlayv", (void**) &pqalSourcePlayv );
	get_dlsym( "alSourceStopv", (void**) &pqalSourceStopv );
	get_dlsym( "alSourceRewindv", (void**) &pqalSourceRewindv );
	get_dlsym( "alSourcePausev", (void**) &pqalSourcePausev );
	get_dlsym( "alSourcePlay", (void**) &pqalSourcePlay );
	get_dlsym( "alSourceStop", (void**) &pqalSourceStop );
	get_dlsym( "alSourceRewind", (void**) &pqalSourceRewind );
	get_dlsym( "alSourcePause", (void**) &pqalSourcePause );
	get_dlsym( "alSourceQueueBuffers", (void**) &pqalSourceQueueBuffers );
	get_dlsym( "alSourceUnqueueBuffers", (void**) &pqalSourceUnqueueBuffers );
	get_dlsym( "alGenBuffers", (void**) &pqalGenBuffers );
	get_dlsym( "alDeleteBuffers", (void**) &pqalDeleteBuffers );
	get_dlsym( "alIsBuffer", (void**) &pqalIsBuffer );
	get_dlsym( "alBufferData", (void**) &pqalBufferData );
	get_dlsym( "alBufferf", (void**) &pqalBufferf );
	get_dlsym( "alBuffer3f", (void**) &pqalBuffer3f );
	get_dlsym( "alBufferfv", (void**) &pqalBufferfv );
	get_dlsym( "alBufferi", (void**) &pqalBufferi );
	get_dlsym( "alBuffer3i", (void**) &pqalBuffer3i );
	get_dlsym( "alBufferiv", (void**) &pqalBufferiv );
	get_dlsym( "alGetBufferf", (void**) &pqalGetBufferf );
	get_dlsym( "alGetBuffer3f", (void**) &pqalGetBuffer3f );
	get_dlsym( "alGetBufferfv", (void**) &pqalGetBufferfv );
	get_dlsym( "alGetBufferi", (void**) &pqalGetBufferi );
	get_dlsym( "alGetBuffer3i", (void**) &pqalGetBuffer3i );
	get_dlsym( "alGetBufferiv", (void**) &pqalGetBufferiv );
	get_dlsym( "alDopplerFactor", (void**) &pqalDopplerFactor );
	get_dlsym( "alDopplerVelocity", (void**) &pqalDopplerVelocity );
	get_dlsym( "alSpeedOfSound", (void**) &pqalSpeedOfSound );
	get_dlsym( "alDistanceModel", (void**) &pqalDistanceModel );
	get_dlsym( "alcCreateContext", (void**) &pqalcCreateContext );
	get_dlsym( "alcMakeContextCurrent", (void**) &pqalcMakeContextCurrent );
	get_dlsym( "alcProcessContext", (void**) &pqalcProcessContext );
	get_dlsym( "alcSuspendContext", (void**) &pqalcSuspendContext );
	get_dlsym( "alcDestroyContext", (void**) &pqalcDestroyContext );
	get_dlsym( "alcGetCurrentContext", (void**) &pqalcGetCurrentContext );
	get_dlsym( "alcGetContextsDevice", (void**) &pqalcGetContextsDevice );
	get_dlsym( "alcOpenDevice", (void**) &pqalcOpenDevice );
	get_dlsym( "alcCloseDevice", (void**) &pqalcCloseDevice );
	get_dlsym( "alcGetError", (void**) &pqalcGetError );
	get_dlsym( "alcIsExtensionPresent", (void**) &pqalcIsExtensionPresent );
	get_dlsym( "alcGetProcAddress", (void**) &pqalcGetProcAddress );
	get_dlsym( "alcGetEnumValue", (void**) &pqalcGetEnumValue );
	get_dlsym( "alcGetString", (void**) &pqalcGetString );
	get_dlsym( "alcGetIntegerv", (void**) &pqalcGetIntegerv );
	get_dlsym( "alcCaptureOpenDevice", (void**) &pqalcCaptureOpenDevice );
	get_dlsym( "alcCaptureCloseDevice", (void**) &pqalcCaptureCloseDevice );
	get_dlsym( "alcCaptureStart", (void**) &pqalcCaptureStart );
	get_dlsym( "alcCaptureStop", (void**) &pqalcCaptureStop );
	get_dlsym( "alcCaptureSamples", (void**) &pqalcCaptureSamples );

	return !dlsym_error;
}

void QAL_Shutdown( void )
{
	if ( hinstOpenAL != NULL )
	{
		FreeLibrary( hinstOpenAL );
		hinstOpenAL = NULL;
	}

	pqalEnable = NULL;
	pqalDisable = NULL;
	pqalIsEnabled = NULL;
	pqalGetBooleanv = NULL;
	pqalGetIntegerv = NULL;
	pqalGetString = NULL;
	pqalGetFloatv = NULL;
	pqalGetDoublev = NULL;
	pqalGetBoolean = NULL;
	pqalGetInteger = NULL;
	pqalGetFloat = NULL;
	pqalGetDouble = NULL;
	pqalGetError = NULL;
	pqalIsExtensionPresent = NULL;
	pqalGetProcAddress = NULL;
	pqalGetEnumValue = NULL;
	pqalListenerf = NULL;
	pqalListener3f = NULL;
	pqalListenerfv = NULL;
	pqalListeneri = NULL;
	pqalListener3i = NULL;
	pqalListeneriv = NULL;
	pqalGetListenerf = NULL;
	pqalGetListener3f = NULL;
	pqalGetListenerfv = NULL;
	pqalGetListeneri = NULL;
	pqalGetListener3i = NULL;
	pqalGetListeneriv = NULL;
	pqalGenSources = NULL;
	pqalDeleteSources = NULL;
	pqalIsSource = NULL;
	pqalSourcef = NULL;
	pqalSource3f = NULL;
	pqalSourcefv = NULL;
	pqalSourcei = NULL;
	pqalSource3i = NULL;
	pqalSourceiv = NULL;
	pqalGetSourcef = NULL;
	pqalGetSource3f = NULL;
	pqalGetSourcefv = NULL;
	pqalGetSourcei = NULL;
	pqalGetSource3i = NULL;
	pqalGetSourceiv = NULL;
	pqalSourcePlayv = NULL;
	pqalSourceStopv = NULL;
	pqalSourceRewindv = NULL;
	pqalSourcePausev = NULL;
	pqalSourcePlay = NULL;
	pqalSourceStop = NULL;
	pqalSourceRewind = NULL;
	pqalSourcePause = NULL;
	pqalSourceQueueBuffers = NULL;
	pqalSourceUnqueueBuffers = NULL;
	pqalGenBuffers = NULL;
	pqalDeleteBuffers = NULL;
	pqalIsBuffer = NULL;
	pqalBufferData = NULL;
	pqalBufferf = NULL;
	pqalBuffer3f = NULL;
	pqalBufferfv = NULL;
	pqalBufferi = NULL;
	pqalBuffer3i = NULL;
	pqalBufferiv = NULL;
	pqalGetBufferf = NULL;
	pqalGetBuffer3f = NULL;
	pqalGetBufferfv = NULL;
	pqalGetBufferi = NULL;
	pqalGetBuffer3i = NULL;
	pqalGetBufferiv = NULL;
	pqalDopplerFactor = NULL;
	pqalDopplerVelocity = NULL;
	pqalSpeedOfSound = NULL;
	pqalDistanceModel = NULL;
	pqalcCreateContext = NULL;
	pqalcMakeContextCurrent = NULL;
	pqalcProcessContext = NULL;
	pqalcSuspendContext = NULL;
	pqalcDestroyContext = NULL;
	pqalcGetCurrentContext = NULL;
	pqalcGetContextsDevice = NULL;
	pqalcOpenDevice = NULL;
	pqalcCloseDevice = NULL;
	pqalcGetError = NULL;
	pqalcIsExtensionPresent = NULL;
	pqalcGetProcAddress = NULL;
	pqalcGetEnumValue = NULL;
	pqalcGetString = NULL;
	pqalcGetIntegerv = NULL;
	pqalcCaptureOpenDevice = NULL;
	pqalcCaptureCloseDevice = NULL;
	pqalcCaptureStart = NULL;
	pqalcCaptureStop = NULL;
	pqalcCaptureSamples = NULL;


}

/*
 ==
 QAL_Loaded()

 check for OpenAL DLL load
 ==
*/
qboolean QAL_Loaded( void )
{
	return ( hinstOpenAL != NULL && !dlsym_error );
}

