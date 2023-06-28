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
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#if defined WIN32_VARIANT
#  include <windows.h>
#endif

#include <GL/gl.h>
#if defined WIN32_VARIANT
# include "glext.h"
#endif

qboolean QGL_Init( const char *dllname );
void     QGL_Shutdown( void );

#ifndef APIENTRY
#  define APIENTRY
#endif

#define T(x) x // type
#define N(x) ( APIENTRY * qgl ## x ) // name
#define A(x) x; // args

#include "glfuncs.h"


//SEPARATE STENCIL
typedef void (APIENTRY * glStencilFuncSeparatePROC)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
typedef void (APIENTRY * glStencilOpSeparatePROC)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (APIENTRY * glStencilMaskSeparatePROC)(GLenum face, GLuint mask);

//VBO

#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif

extern GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
extern GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
extern GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
extern GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
extern GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
extern void *			(APIENTRY * qglMapBufferARB)(GLenum target, GLenum access);
extern GLboolean		(APIENTRY * qglUnmapBufferARB)(GLenum target);

#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif
//END VBO

#if 0
#if !defined UNIX_VARIANT
// jitwater - fragment programs (pixel shaders)
typedef void (APIENTRY * PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRY * PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRY * PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRY * PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRY * PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRY * PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRY * PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);

//GLSL
typedef unsigned int GLhandleARB;	/* shader object handle */
typedef char GLcharARB;		/* native character */
typedef GLhandleARB (APIENTRY * PFNGLCREATEPROGRAMOBJECTARBPROC) (void);
typedef void (APIENTRY * PFNGLDELETEOBJECTARBPROC) (GLhandleARB obj);
typedef void (APIENTRY * PFNGLUSEPROGRAMOBJECTARBPROC) (GLhandleARB programObj);
typedef GLhandleARB (APIENTRY * PFNGLCREATESHADEROBJECTARBPROC) (GLenum shaderType);
typedef void (APIENTRY * PFNGLSHADERSOURCEARBPROC) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, GLint *length);
typedef void (APIENTRY * PFNGLCOMPILESHADERARBPROC) (GLhandleARB shaderObj);
typedef void (APIENTRY * PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLATTACHOBJECTARBPROC) (GLhandleARB containerObj, GLhandleARB obj);
typedef void (APIENTRY * PFNGLGETINFOLOGARBPROC) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void (APIENTRY * PFNGLLINKPROGRAMARBPROC) (GLhandleARB programObj);
typedef GLint (APIENTRY * PFNGLGETUNIFORMLOCATIONARBPROC) (GLhandleARB programObj, const GLcharARB *name);
typedef void (APIENTRY * PFNGLUNIFORM3FARBPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * PFNGLUNIFORM2FARBPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY * PFNGLUNIFORM1IARBPROC) (GLint location, GLint v0);
typedef void (APIENTRY * PFNGLUNIFORM1FARBPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PFNGLUNIFORMMATRIX3FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, GLfloat [3][3]); 
typedef void (APIENTRY * PFNGLUNIFORMMATRIX3X4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint);
typedef void (APIENTRY * PFNGLBINDATTRIBLOCATIONARBPROC) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
#endif

//Framebuffer objects
typedef GLboolean (APIENTRY * PFNGLISRENDERBUFFEREXTPROC) (GLuint renderbuffer);
typedef void (APIENTRY * PFNGLBINDRENDERBUFFEREXTPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY * PFNGLDELETERENDERBUFFERSEXTPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRY * PFNGLGENRENDERBUFFERSEXTPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY * PFNGLRENDERBUFFERSTORAGEEXTPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef GLboolean (APIENTRY * PFNGLISFRAMEBUFFEREXTPROC) (GLuint framebuffer);
typedef void (APIENTRY * PFNGLBINDFRAMEBUFFEREXTPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY * PFNGLDELETEFRAMEBUFFERSEXTPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY * PFNGLGENFRAMEBUFFERSEXTPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRY * PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) (GLenum target);
typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
typedef void (APIENTRY * PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY * PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGENERATEMIPMAPEXTPROC) (GLenum target);
typedef void (APIENTRY * PFNGLBLITFRAMEBUFFEREXTPROC) (GLint  srcX0, GLint  srcY0, GLint  srcX1, GLint  srcY1, GLint  dstX0, 
			  GLint  dstY0, GLint  dstX1, GLint  dstY1, GLbitfield  mask, GLenum  filter);   


#endif

#if defined WIN32_VARIANT

extern  int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
extern  int   ( WINAPI * qwglDescribePixelFormat) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
extern  int   ( WINAPI * qwglGetPixelFormat)(HDC);
extern  BOOL  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
extern  BOOL  ( WINAPI * qwglSwapBuffers)(HDC);

extern BOOL  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC ( WINAPI * qwglCreateContext)(HDC);
extern HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
extern BOOL  ( WINAPI * qwglDeleteContext)(HGLRC);
extern HGLRC ( WINAPI * qwglGetCurrentContext)(VOID);
extern HDC   ( WINAPI * qwglGetCurrentDC)(VOID);
extern PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
extern BOOL  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern BOOL  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
extern BOOL  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

extern BOOL  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

extern BOOL ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,
                                            LPLAYERPLANEDESCRIPTOR);
extern int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,
                                                CONST COLORREF *);
extern int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,
                                                COLORREF *);
extern BOOL ( WINAPI * qwglRealizeLayerPalette)(HDC, int, BOOL);
extern BOOL ( WINAPI * qwglSwapLayerBuffers)(HDC, UINT);

extern BOOL ( WINAPI * qwglSwapIntervalEXT)( int interval );

extern BOOL ( WINAPI * qwglGetDeviceGammaRampEXT ) ( unsigned char *pRed, unsigned char *pGreen, unsigned char *pBlue );
extern BOOL ( WINAPI * qwglSetDeviceGammaRampEXT ) ( const unsigned char *pRed, const unsigned char *pGreen, const unsigned char *pBlue );

#endif

#if defined UNIX_VARIANT

#include <GL/glx.h>

// local function in dll
extern void * (*qwglGetProcAddress) (const char*);

/*
//FX Mesa Functions
extern fxMesaContext (*qfxMesaCreateContext)(GLuint win, GrScreenResolution_t, GrScreenRefresh_t, const GLint attribList[]);
extern fxMesaContext (*qfxMesaCreateBestContext)(GLuint win, GLint width, GLint height, const GLint attribList[]);
extern void (*qfxMesaDestroyContext)(fxMesaContext ctx);
extern void (*qfxMesaMakeCurrent)(fxMesaContext ctx);
extern fxMesaContext (*qfxMesaGetCurrentContext)(void);
extern void (*qfxMesaSwapBuffers)(void);
*/

//GLX Functions
extern XVisualInfo * (*qglXChooseVisual)( Display *dpy, int screen, int *attribList );
extern GLXContext (*qglXCreateContext)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
extern void (*qglXDestroyContext)( Display *dpy, GLXContext ctx );
extern Bool (*qglXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void (*qglXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, GLuint mask );
extern void (*qglXSwapBuffers)( Display *dpy, GLXDrawable drawable );

#endif // linux

/*
** extension constants
*/
#define GL_POINT_SIZE_MIN_EXT				0x8126
#define GL_POINT_SIZE_MAX_EXT				0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT			0x8129

#ifdef __sgi
#define GL_SHARED_TEXTURE_PALETTE_EXT		GL_TEXTURE_COLOR_TABLE_SGI
#else
#define GL_SHARED_TEXTURE_PALETTE_EXT		0x81FB
#endif

#define GL_TEXTURE0_SGIS					0x835E
#define GL_TEXTURE1_SGIS					0x835F
#define GL_TEXTURE0_ARB						0x84C0
#define GL_TEXTURE1_ARB						0x84C1

#ifdef GL_TEXTURE0
#undef GL_TEXTURE0
#endif
#ifdef GL_TEXTURE1
#undef GL_TEXTURE1
#endif
#ifdef GL_TEXTURE2
#undef GL_TEXTURE2
#endif
#ifdef GL_TEXTURE3
#undef GL_TEXTURE3
#endif
#ifdef GL_TEXTURE4
#undef GL_TEXTURE4
#endif
#ifdef GL_TEXTURE5
#undef GL_TEXTURE5
#endif
#ifdef GL_TEXTURE6
#undef GL_TEXTURE6
#endif
#ifdef GL_TEXTURE7
#undef GL_TEXTURE7
#endif

extern int GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7;

#define  GL_FRAGMENT_PROGRAM_ARB			0x8804 // jitwater
#define  GL_PROGRAM_FORMAT_ASCII_ARB		0x8875 // jitwater
#define GL_VERTEX_SHADER_ARB				0x8B31
#define GL_FRAGMENT_SHADER_ARB				0x8B30

#define GL_OBJECT_DELETE_STATUS_ARB			0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB		0x8B81
#define GL_OBJECT_LINK_STATUS_ARB			0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB		0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB		0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB		0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB		0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB	0x8B88

// Vic - begin
#ifndef GL_EXT_texture_env_combine
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE3_RGB_EXT                0x8583
#define GL_SOURCE4_RGB_EXT                0x8584
#define GL_SOURCE5_RGB_EXT                0x8585
#define GL_SOURCE6_RGB_EXT                0x8586
#define GL_SOURCE7_RGB_EXT                0x8587
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_SOURCE3_ALPHA_EXT              0x858B
#define GL_SOURCE4_ALPHA_EXT              0x858C
#define GL_SOURCE5_ALPHA_EXT              0x858D
#define GL_SOURCE6_ALPHA_EXT              0x858E
#define GL_SOURCE7_ALPHA_EXT              0x858F
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND3_RGB_EXT               0x8593
#define GL_OPERAND4_RGB_EXT               0x8594
#define GL_OPERAND5_RGB_EXT               0x8595
#define GL_OPERAND6_RGB_EXT               0x8596
#define GL_OPERAND7_RGB_EXT               0x8597
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#define GL_OPERAND3_ALPHA_EXT             0x859B
#define GL_OPERAND4_ALPHA_EXT             0x859C
#define GL_OPERAND5_ALPHA_EXT             0x859D
#define GL_OPERAND6_ALPHA_EXT             0x859E
#define GL_OPERAND7_ALPHA_EXT             0x859F
#endif
// Vic - end

//add glsl program extensions
extern	PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB;	
extern	PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB;
extern	PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB;
extern	PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB;
extern  PFNGLSHADERSOURCEARBPROC			glShaderSourceARB;		
extern	PFNGLCOMPILESHADERPROC				glCompileShader;
extern	PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB;
extern	PFNGLATTACHOBJECTARBPROC			glAttachObjectARB;
extern	PFNGLGETINFOLOGARBPROC				glGetInfoLogARB;
extern	PFNGLLINKPROGRAMARBPROC				glLinkProgramARB;
extern	PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB;
extern	PFNGLUNIFORM4IARBPROC				glUniform4iARB;
extern	PFNGLUNIFORM4FARBPROC				glUniform4fARB;
extern	PFNGLUNIFORM3FARBPROC				glUniform3fARB;
extern	PFNGLUNIFORM2FARBPROC				glUniform2fARB;
extern	PFNGLUNIFORM1IARBPROC				glUniform1iARB;
extern  PFNGLUNIFORM1FARBPROC				glUniform1fARB;
extern	PFNGLUNIFORM4IVARBPROC				glUniform4ivARB;
extern	PFNGLUNIFORM4FVARBPROC				glUniform4fvARB;
extern	PFNGLUNIFORM3FVARBPROC				glUniform3fvARB;
extern	PFNGLUNIFORM2FVARBPROC				glUniform2fvARB;
extern	PFNGLUNIFORM1IVARBPROC				glUniform1ivARB;
extern  PFNGLUNIFORM1FVARBPROC				glUniform1fvARB;
extern  PFNGLUNIFORMMATRIX3FVARBPROC		glUniformMatrix3fvARB;
#ifndef PFNGLUNIFORMMATRIX3X4FVARBPROC //FIXME HACK to get it compiling on Linux
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3X4FVARBPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
#endif
extern  PFNGLUNIFORMMATRIX3X4FVARBPROC		glUniformMatrix3x4fvARB;
extern	PFNGLVERTEXATTRIBPOINTERARBPROC		glVertexAttribPointerARB;
extern	PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
extern	PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern	PFNGLBINDATTRIBLOCATIONARBPROC		qglBindAttribLocationARB;

//framebuffer objects
#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object

#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT				0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT						0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT							0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT							0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT			0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT			0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT			0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT	0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT							0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT			0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT	0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT	0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT			0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT				0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT			0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT			0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT						0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT						0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT							0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT							0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT							0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT							0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT							0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT							0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT							0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT							0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT							0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT							0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT							0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT							0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT							0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT							0x8CED
#define GL_COLOR_ATTACHMENT14_EXT							0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT							0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT								0x8D00
#define GL_STENCIL_ATTACHMENT_EXT							0x8D20
#define GL_FRAMEBUFFER_EXT									0x8D40
#define GL_RENDERBUFFER_EXT									0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT							0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT							0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT					0x8D44
#define GL_STENCIL_INDEX1_EXT								0x8D46
#define GL_STENCIL_INDEX4_EXT								0x8D47
#define GL_STENCIL_INDEX8_EXT								0x8D48
#define GL_STENCIL_INDEX16_EXT								0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT						0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT						0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT						0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT						0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT						0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT					0x8D55
#endif

extern          GLboolean(APIENTRY * qglIsRenderbufferEXT) (GLuint renderbuffer);
extern void     (APIENTRY * qglBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
extern void     (APIENTRY * qglDeleteRenderbuffersEXT) (GLsizei n, const GLuint * renderbuffers);
extern void     (APIENTRY * qglGenRenderbuffersEXT) (GLsizei n, GLuint * renderbuffers);
extern void     (APIENTRY * qglRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void     (APIENTRY * qglGetRenderbufferParameterivEXT) (GLenum target, GLenum pname, GLint * params);
extern          GLboolean(APIENTRY * qglIsFramebufferEXT) (GLuint framebuffer);
extern void     (APIENTRY * qglBindFramebufferEXT) (GLenum target, GLuint framebuffer);
extern void     (APIENTRY * qglDeleteFramebuffersEXT) (GLsizei n, const GLuint * framebuffers);
extern void     (APIENTRY * qglGenFramebuffersEXT) (GLsizei n, GLuint * framebuffers);
extern          GLenum(APIENTRY * qglCheckFramebufferStatusEXT) (GLenum target);
extern void     (APIENTRY * qglFramebufferTexture1DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
extern void     (APIENTRY * qglFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
extern void     (APIENTRY * qglFramebufferTexture3DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level, GLint zoffset);
extern void     (APIENTRY * qglFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget,
															GLuint renderbuffer);
extern void     (APIENTRY * qglGetFramebufferAttachmentParameterivEXT) (GLenum target, GLenum attachment, GLenum pname,
																		GLint * params);
extern void     (APIENTRY * qglGenerateMipmapEXT) (GLenum target);

// GL_EXT_packed_depth_stencil
#ifndef GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT              0x84F9
#define GL_UNSIGNED_INT_24_8_EXT          0x84FA
#define GL_DEPTH24_STENCIL8_EXT           0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT       0x88F1
#endif

// GL_EXT_framebuffer_blit
#ifndef GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   GL_FRAMEBUFFER_BINDING_EXT
#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA
#endif

extern void		(APIENTRY * qglBlitFramebufferEXT) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

#endif /* __QGL_H__ */
