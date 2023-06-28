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
#ifndef QAL_H_
#define QAL_H_

/*
 * function pointers for library loaders
 */
extern LPALENABLE pqalEnable;
extern LPALDISABLE pqalDisable;
extern LPALISENABLED pqalIsEnabled;
extern LPALGETBOOLEANV pqalGetBooleanv;
extern LPALGETINTEGERV pqalGetIntegerv;
extern LPALGETSTRING pqalGetString;
extern LPALGETFLOATV pqalGetFloatv;
extern LPALGETDOUBLEV pqalGetDoublev;
extern LPALGETBOOLEAN pqalGetBoolean;
extern LPALGETINTEGER pqalGetInteger;
extern LPALGETFLOAT pqalGetFloat;
extern LPALGETDOUBLE pqalGetDouble;
extern LPALGETERROR pqalGetError;
extern LPALISEXTENSIONPRESENT pqalIsExtensionPresent;
extern LPALGETPROCADDRESS pqalGetProcAddress;
extern LPALGETENUMVALUE pqalGetEnumValue;
extern LPALLISTENERF pqalListenerf;
extern LPALLISTENER3F pqalListener3f;
extern LPALLISTENERFV pqalListenerfv;
extern LPALLISTENERI pqalListeneri;
extern LPALLISTENER3I pqalListener3i;
extern LPALLISTENERIV pqalListeneriv;
extern LPALGETLISTENERF pqalGetListenerf;
extern LPALGETLISTENER3F pqalGetListener3f;
extern LPALGETLISTENERFV pqalGetListenerfv;
extern LPALGETLISTENERI pqalGetListeneri;
extern LPALGETLISTENER3I pqalGetListener3i;
extern LPALGETLISTENERIV pqalGetListeneriv;
extern LPALGENSOURCES pqalGenSources;
extern LPALDELETESOURCES pqalDeleteSources;
extern LPALISSOURCE pqalIsSource;
extern LPALSOURCEF pqalSourcef;
extern LPALSOURCE3F pqalSource3f;
extern LPALSOURCEFV pqalSourcefv;
extern LPALSOURCEI pqalSourcei;
extern LPALSOURCE3I pqalSource3i;
extern LPALSOURCEIV pqalSourceiv;
extern LPALGETSOURCEF pqalGetSourcef;
extern LPALGETSOURCE3F pqalGetSource3f;
extern LPALGETSOURCEFV pqalGetSourcefv;
extern LPALGETSOURCEI pqalGetSourcei;
extern LPALGETSOURCE3I pqalGetSource3i;
extern LPALGETSOURCEIV pqalGetSourceiv;
extern LPALSOURCEPLAYV pqalSourcePlayv;
extern LPALSOURCESTOPV pqalSourceStopv;
extern LPALSOURCEREWINDV pqalSourceRewindv;
extern LPALSOURCEPAUSEV pqalSourcePausev;
extern LPALSOURCEPLAY pqalSourcePlay;
extern LPALSOURCESTOP pqalSourceStop;
extern LPALSOURCEREWIND pqalSourceRewind;
extern LPALSOURCEPAUSE pqalSourcePause;
extern LPALSOURCEQUEUEBUFFERS pqalSourceQueueBuffers;
extern LPALSOURCEUNQUEUEBUFFERS pqalSourceUnqueueBuffers;
extern LPALGENBUFFERS pqalGenBuffers;
extern LPALDELETEBUFFERS pqalDeleteBuffers;
extern LPALISBUFFER pqalIsBuffer;
extern LPALBUFFERDATA pqalBufferData;
extern LPALBUFFERF pqalBufferf;
extern LPALBUFFER3F pqalBuffer3f;
extern LPALBUFFERFV pqalBufferfv;
extern LPALBUFFERI pqalBufferi;
extern LPALBUFFER3I pqalBuffer3i;
extern LPALBUFFERIV pqalBufferiv;
extern LPALGETBUFFERF pqalGetBufferf;
extern LPALGETBUFFER3F pqalGetBuffer3f;
extern LPALGETBUFFERFV pqalGetBufferfv;
extern LPALGETBUFFERI pqalGetBufferi;
extern LPALGETBUFFER3I pqalGetBuffer3i;
extern LPALGETBUFFERIV pqalGetBufferiv;
extern LPALDOPPLERFACTOR pqalDopplerFactor;
extern LPALDOPPLERVELOCITY pqalDopplerVelocity;
extern LPALSPEEDOFSOUND pqalSpeedOfSound;
extern LPALDISTANCEMODEL pqalDistanceModel;

extern LPALCCREATECONTEXT pqalcCreateContext;
extern LPALCMAKECONTEXTCURRENT pqalcMakeContextCurrent;
extern LPALCPROCESSCONTEXT pqalcProcessContext;
extern LPALCSUSPENDCONTEXT pqalcSuspendContext;
extern LPALCDESTROYCONTEXT pqalcDestroyContext;
extern LPALCGETCURRENTCONTEXT pqalcGetCurrentContext;
extern LPALCGETCONTEXTSDEVICE pqalcGetContextsDevice;
extern LPALCOPENDEVICE pqalcOpenDevice;
extern LPALCCLOSEDEVICE pqalcCloseDevice;
extern LPALCGETERROR pqalcGetError;
extern LPALCISEXTENSIONPRESENT pqalcIsExtensionPresent;
extern LPALCGETPROCADDRESS pqalcGetProcAddress;
extern LPALCGETENUMVALUE pqalcGetEnumValue;
extern LPALCGETSTRING pqalcGetString;
extern LPALCGETINTEGERV pqalcGetIntegerv;
extern LPALCCAPTUREOPENDEVICE pqalcCaptureOpenDevice;
extern LPALCCAPTURECLOSEDEVICE pqalcCaptureCloseDevice;
extern LPALCCAPTURESTART pqalcCaptureStart;
extern LPALCCAPTURESTOP pqalcCaptureStop;
extern LPALCCAPTURESAMPLES pqalcCaptureSamples;

/*
 * Note: using functions that wrap the function pointers for easier development
 */
extern void qalEnable( ALenum capability );
extern void qalDisable( ALenum capability );
extern ALboolean qalIsEnabled( ALenum capability );
extern const ALchar *qalGetString( ALenum param );
extern void qalGetBooleanv( ALenum param, ALboolean *data );
extern void qalGetIntegerv( ALenum param, ALint *data );
extern void qalGetFloatv( ALenum param, ALfloat *data );
extern void qalGetDoublev( ALenum param, ALdouble *data );
extern ALboolean qalGetBoolean( ALenum param );
extern ALint qalGetInteger( ALenum param );
extern ALfloat qalGetFloat( ALenum param );
extern ALdouble qalGetDouble( ALenum param );
extern ALenum qalGetError( void );
extern void qalClearError( void );
extern ALboolean qalIsExtensionPresent( const ALchar *extname );
extern void *qalGetProcAddress( const ALchar *fname );
extern ALenum qalGetEnumValue( const ALchar *ename );
extern void qalListenerf( ALenum param, ALfloat value );
extern void qalListener3f( ALenum param, ALfloat value1,
        ALfloat value2, ALfloat value3 );
extern void qalListenerfv( ALenum param, const ALfloat *values );
extern void qalListeneri( ALenum param, ALint value );
extern void qalListener3i( ALenum param, ALint value1, ALint value2,
        ALint value3 );
extern void qalListeneriv( ALenum param, const ALint *values );
extern void qalGetListenerf( ALenum param, ALfloat *value );
extern void qalGetListener3f( ALenum param, ALfloat *value1,
        ALfloat *value2, ALfloat *value3 );
extern void qalGetListenerfv( ALenum param, ALfloat *values );
extern void qalGetListeneri( ALenum param, ALint *value );
extern void qalGetListener3i( ALenum param, ALint *value1,
        ALint *value2, ALint *value3 );
extern void qalGetListeneriv( ALenum param, ALint *values );
extern void qalGenSources( ALsizei n, ALuint *sources );
extern void qalDeleteSources( ALsizei n, const ALuint *sources );
extern ALboolean qalIsSource( ALuint sid );
extern void qalSourcef( ALuint sid, ALenum param, ALfloat value );
extern void qalSource3f( ALuint sid, ALenum param, ALfloat value1,
        ALfloat value2, ALfloat value3 );
extern void qalSourcefv( ALuint sid, ALenum param,
        const ALfloat *values );
extern void qalSourcei( ALuint sid, ALenum param, ALint value );
extern void qalSource3i( ALuint sid, ALenum param, ALint value1,
        ALint value2, ALint value3 );
extern void qalSourceiv( ALuint sid, ALenum param,
        const ALint *values );
extern void qalGetSourcef( ALuint sid, ALenum param, ALfloat *value );
extern void qalGetSource3f( ALuint sid, ALenum param,
        ALfloat *value1, ALfloat *value2, ALfloat *value3 );
extern void qalGetSourcefv( ALuint sid, ALenum param,
        ALfloat *values );
extern void qalGetSourcei( ALuint sid, ALenum param, ALint *value );
extern void qalGetSource3i( ALuint sid, ALenum param, ALint *value1,
        ALint *value2, ALint *value3 );
extern void qalGetSourceiv( ALuint sid, ALenum param, ALint *values );
extern void qalSourcePlayv( ALsizei ns, const ALuint *sids );
extern void qalSourceStopv( ALsizei ns, const ALuint *sids );
extern void qalSourceRewindv( ALsizei ns, const ALuint *sids );
extern void qalSourcePausev( ALsizei ns, const ALuint *sids );
extern void qalSourcePlay( ALuint sid );
extern void qalSourceStop( ALuint sid );
extern void qalSourceRewind( ALuint sid );
extern void qalSourcePause( ALuint sid );
extern void qalSourceQueueBuffers( ALuint sid, ALsizei numEntries,
        const ALuint *bids );
extern void qalSourceUnqueueBuffers( ALuint sid, ALsizei numEntries,
        ALuint *bids );
extern void qalGenBuffers( ALsizei n, ALuint *buffers );
extern void qalDeleteBuffers( ALsizei n, const ALuint *buffers );
extern ALboolean qalIsBuffer( ALuint bid );
extern void qalBufferData( ALuint bid, ALenum format,
        const ALvoid *data, ALsizei size, ALsizei freq );
extern void qalBufferf( ALuint bid, ALenum param, ALfloat value );
extern void qalBuffer3f( ALuint bid, ALenum param, ALfloat value1,
        ALfloat value2, ALfloat value3 );
extern void qalBufferfv( ALuint bid, ALenum param,
        const ALfloat *values );
extern void qalBufferi( ALuint bid, ALenum param, ALint value );
extern void qalBuffer3i( ALuint bid, ALenum param, ALint value1,
        ALint value2, ALint value3 );
extern void qalBufferiv( ALuint bid, ALenum param,
        const ALint *values );
extern void qalGetBufferf( ALuint bid, ALenum param, ALfloat *value );
extern void qalGetBuffer3f( ALuint bid, ALenum param,
        ALfloat *value1, ALfloat *value2, ALfloat *value3 );
extern void qalGetBufferfv( ALuint bid, ALenum param,
        ALfloat *values );
extern void qalGetBufferi( ALuint bid, ALenum param, ALint *value );
extern void qalGetBuffer3i( ALuint bid, ALenum param, ALint *value1,
        ALint *value2, ALint *value3 );
extern void qalGetBufferiv( ALuint bid, ALenum param, ALint *values );
extern void qalDopplerFactor( ALfloat value );
extern void qalDopplerVelocity( ALfloat value );
extern void qalSpeedOfSound( ALfloat value );
extern void qalDistanceModel( ALenum distanceModel );
extern ALCcontext
        *qalcCreateContext( ALCdevice *device, const ALCint *attrlist );
extern ALCboolean qalcMakeContextCurrent( ALCcontext *context );
extern void qalcProcessContext( ALCcontext *context );
extern void qalcSuspendContext( ALCcontext *context );
extern void qalcDestroyContext( ALCcontext *context );
extern ALCcontext *qalcGetCurrentContext( void );
extern ALCdevice *qalcGetContextsDevice( ALCcontext *context );
extern ALCdevice *qalcOpenDevice( const ALCchar *devicename );
extern ALCboolean qalcCloseDevice( ALCdevice *device );
extern ALCenum qalcGetError( ALCdevice *device );
extern ALCboolean qalcIsExtensionPresent( ALCdevice *device,
        const ALCchar *extname );
extern void *qalcGetProcAddress( ALCdevice *device, const ALCchar *funcname );
extern ALCenum qalcGetEnumValue( ALCdevice *device,
        const ALCchar *enumname );
extern const ALCchar *qalcGetString( ALCdevice *device, ALCenum param );
extern void qalcGetIntegerv( ALCdevice *device, ALCenum param,
        ALCsizei size, ALCint *data );
extern ALCdevice *qalcCaptureOpenDevice( const ALCchar *devicename,
        ALCuint frequency, ALCenum format, ALCsizei buffersize );
extern ALCboolean qalcCaptureCloseDevice( ALCdevice *device );
extern void qalcCaptureStart( ALCdevice *device );
extern void qalcCaptureStop( ALCdevice *device );
extern void qalcCaptureSamples( ALCdevice *device, ALCvoid *buffer,
        ALCsizei samples );

/*
 * System dependent dynamic/shared lib managment
 */
extern qboolean QAL_Init( void );
extern void QAL_Shutdown( void );
extern qboolean QAL_Loaded( void );

#endif /* QAL_H_ */

