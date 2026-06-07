/*
===========================================================================
OpenGL ES 2 mappings for Smokin' Guns iOS
===========================================================================
*/
#ifndef QGL_GLES_H
#define QGL_GLES_H

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP *
#endif

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include "gl_legacy_const.h"

/* ES2 reuses GL_REPLACE for stencil; renderer expects desktop texture-env value. */
#ifdef GL_REPLACE
#undef GL_REPLACE
#endif
#define GL_REPLACE GL_TEXENV_REPLACE

/* Fixed-function enums (no-op on ES2; shaders handle blending) */
#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_DECAL
#define GL_DECAL 0x2103
#endif
#ifndef GL_ADD
#define GL_ADD 0x0104
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_CLIP_PLANE0
#define GL_CLIP_PLANE0 0x3000
#endif
/* Desktop texture-env GL_REPLACE (0x2101); ES2 defines GL_REPLACE for stencil (0x1E01). */
#ifndef GL_TEXENV_REPLACE
#define GL_TEXENV_REPLACE 0x2101
#endif

/* Immediate mode (desktop glBegin) — implemented in gles_immediate.c */
void GLES_Begin( GLenum mode );
void GLES_End( void );
void GLES_Color4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void GLES_Color4ubv( const GLubyte *v );
void GLES_Color3f( GLfloat r, GLfloat g, GLfloat b );
void GLES_TexCoord2f( GLfloat s, GLfloat t );
void GLES_TexCoord2fv( const GLfloat *v );
void GLES_MultiTexCoord2f( GLenum unit, GLfloat s, GLfloat t );
void GLES_Vertex2f( GLfloat x, GLfloat y );
void GLES_Vertex2fv( const GLfloat *v );
void GLES_Vertex3f( GLfloat x, GLfloat y, GLfloat z );
void GLES_Vertex3fv( const GLfloat *v );
void GLES_Ortho( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
	GLfloat zNear, GLfloat zFar );
void GLES_Enable( GLenum cap );
void GLES_Disable( GLenum cap );
void GLES_ClientActiveTexture( GLenum texture );
void GLES_TexEnvf( GLenum target, GLenum pname, GLfloat param );
void GLES_PushMatrix( void );
void GLES_PopMatrix( void );
void GLES_LoadMatrixf( const GLfloat *m );
void GLES_Translatef( GLfloat x, GLfloat y, GLfloat z );
void GLES_InitImmediate( void );
void GLES_ShutdownImmediate( void );
void GLES_ColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void GLES_TexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void GLES_VertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer );
void GLES_NormalPointer( GLenum type, GLsizei stride, const void *pointer );
void GLES_EnableClientState( GLenum array );
void GLES_DisableClientState( GLenum array );
void GLES_DrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices );
void GLES_DrawArrays( GLenum mode, GLint first, GLsizei count );
void GLES_ArrayElement( GLint i );

#define qglBegin GLES_Begin
#define qglEnd GLES_End
#define qglColor4f GLES_Color4f
#define qglColor4ubv GLES_Color4ubv
#define qglColor3f GLES_Color3f
#define qglTexCoord2f GLES_TexCoord2f
#define qglTexCoord2fv GLES_TexCoord2fv
#define qglVertex2f GLES_Vertex2f
#define qglVertex2fv GLES_Vertex2fv
#define qglVertex3f GLES_Vertex3f
#define qglVertex3fv GLES_Vertex3fv
#define qglPushMatrix GLES_PushMatrix
#define qglPopMatrix GLES_PopMatrix
#define qglLoadMatrixf GLES_LoadMatrixf
#define qglTranslatef GLES_Translatef

/* ARB multitexture → GLES2 */
#define qglActiveTextureARB glActiveTexture
#define qglClientActiveTextureARB GLES_ClientActiveTexture
#define qglMultiTexCoord2fARB GLES_MultiTexCoord2f

/* Checked as function pointers in tr_init / tr_shade (NULL = unavailable on ES2). */
extern void ( *qglLockArraysEXT )( GLint first, GLsizei count );
extern void ( *qglUnlockArraysEXT )( void );

/* Standard GLES2 entry points */
#define qglAccum( a, b ) ((void)(a),(void)(b))
#define qglAlphaFunc( a, b ) ((void)(a),(void)(b))
#define qglAreTexturesResident( a, b ) (0)
#define qglArrayElement GLES_ArrayElement
#define qglBindTexture glBindTexture
#define qglBitmap( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglBlendFunc glBlendFunc
#define qglCallList( a ) ((void)(a))
#define qglCallLists( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglClear glClear
#define qglClearAccum( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglClearColor glClearColor
#define qglClearDepth glClearDepthf
#define qglClearIndex( a ) ((void)(a))
#define qglClearStencil glClearStencil
#define qglClipPlane( a, b ) ((void)(a),(void)(b))
#define qglColorMask glColorMask
#define qglColorMaterial( a, b ) ((void)(a),(void)(b))
#define qglColorPointer GLES_ColorPointer
#define qglCopyPixels( a, b, c, d, e ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e))
#define qglCopyTexImage1D( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglCopyTexImage2D glCopyTexImage2D
#define qglCopyTexSubImage1D( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglCopyTexSubImage2D glCopyTexSubImage2D
#define qglCullFace glCullFace
#define qglDeleteLists( a, b ) ((void)(a),(void)(b))
#define qglDeleteTextures glDeleteTextures
#define qglDepthFunc glDepthFunc
#define qglDepthMask glDepthMask
#define qglDepthRange glDepthRangef
#define qglDisable GLES_Disable
#define qglDisableClientState GLES_DisableClientState
#define qglDrawArrays GLES_DrawArrays
#define qglDrawBuffer( a ) ((void)(a))
#define qglDrawElements GLES_DrawElements
#define qglDrawPixels( a, b, c, d, e ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e))
#define qglEdgeFlag( a ) ((void)(a))
#define qglEnable GLES_Enable
#define qglEnableClientState GLES_EnableClientState
#define qglFinish glFinish
#define qglFlush glFlush
#define qglFogf( a, b ) ((void)(a),(void)(b))
#define qglFogfv( a, b ) ((void)(a),(void)(b))
#define qglFogi( a, b ) ((void)(a),(void)(b))
#define qglFogiv( a, b ) ((void)(a),(void)(b))
#define qglFrontFace glFrontFace
#define qglFrustum( a, b, c, d, e, f ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f))
#define qglGenTextures glGenTextures
#define qglGetBooleanv glGetBooleanv
#define qglGetDoublev( a, b ) ((void)(a),(void)(b))
#define qglGetFloatv glGetFloatv
#define qglGetIntegerv glGetIntegerv
#define qglGetError glGetError
#define qglGetString glGetString
#define qglGetTexEnviv( a, b, c ) glGetTexParameteriv( GL_TEXTURE_2D, b, c )
#define qglGetTexLevelParameteriv glGetTexLevelParameteriv
#define qglHint glHint
#define qglIndexMask( a ) ((void)(a))
#define qglIndexPointer( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglInitNames() ((void)0)
#define qglIsEnabled glIsEnabled
#define qglIsList( a ) (0)
#define qglIsTexture glIsTexture
#define qglLightModelf( a, b ) ((void)(a),(void)(b))
#define qglLightModelfv( a, b ) ((void)(a),(void)(b))
#define qglLightModeli( a, b ) ((void)(a),(void)(b))
#define qglLightModeliv( a, b ) ((void)(a),(void)(b))
#define qglLightf( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglLightfv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglLineStipple( a, b ) ((void)(a),(void)(b))
#define qglLineWidth glLineWidth
#define qglListBase( a ) ((void)(a))
#define qglLoadIdentity() GLES_LoadIdentity()
#define qglLoadMatrixd( a ) ((void)(a))
#define qglMatrixMode GLES_MatrixMode
#define qglMultMatrixf GLES_MultMatrixf
#define qglRotatef GLES_Rotatef
#define qglScalef GLES_Scalef
#define qglLogicOp( a ) ((void)(a))
#define qglMap1d( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglMap1f( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglMap2d( a, b, c, d, e, f, g, h, i, j, k ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g),(void)(h),(void)(i),(void)(j),(void)(k))
#define qglMap2f( a, b, c, d, e, f, g, h, i, j, k ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g),(void)(h),(void)(i),(void)(j),(void)(k))
#define qglMapGrid1d( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMapGrid1f( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMapGrid2d( a, b, c, d, e, f ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f))
#define qglMapGrid2f( a, b, c, d, e, f ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f))
#define qglMaterialf( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMaterialfv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMateriali( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMaterialiv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglMultMatrixd( a ) ((void)(a))
#define qglNewList( a, b ) ((void)(a),(void)(b))
#define qglNormal3f( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglNormal3fv( a ) ((void)(a))
#define qglNormalPointer GLES_NormalPointer
#define qglOrtho GLES_Ortho
#define qglPassThrough( a ) ((void)(a))
#define qglPixelMapfv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglPixelStorei glPixelStorei
#define qglPixelTransferf( a, b ) ((void)(a),(void)(b))
#define qglPixelZoom( a, b ) ((void)(a),(void)(b))
#define qglPointSize glPointSize
#define qglPolygonMode( a, b ) ((void)(a),(void)(b))
#define qglPolygonOffset glPolygonOffset
#define qglPolygonStipple( a ) ((void)(a))
#define qglPopAttrib() ((void)0)
#define qglPopName() ((void)0)
#define qglPrioritizeTextures( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglPushAttrib( a ) ((void)(a))
#define qglPushName( a ) ((void)(a))
#define qglRasterPos2d( a, b ) ((void)(a),(void)(b))
#define qglRasterPos2f( a, b ) ((void)(a),(void)(b))
#define qglRasterPos2i( a, b ) ((void)(a),(void)(b))
#define qglRasterPos2s( a, b ) ((void)(a),(void)(b))
#define qglRasterPos3d( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglRasterPos3f( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglRasterPos3fv( a ) ((void)(a))
#define qglRasterPos3i( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglReadBuffer( a ) ((void)(a))
#define qglReadPixels glReadPixels
#define qglRectd( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglRectf( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglRecti( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglRects( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglRotated( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglScaled( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglScissor glScissor
#define qglSelectBuffer( a, b ) ((void)(a),(void)(b))
#define qglShadeModel( a ) ((void)(a))
#define qglStencilFunc glStencilFunc
#define qglStencilMask glStencilMask
#define qglStencilOp glStencilOp
#define qglTexCoord1d( a ) ((void)(a))
#define qglTexCoord1f( a ) ((void)(a))
#define qglTexCoord1i( a ) ((void)(a))
#define qglTexCoord2d( a, b ) ((void)(a),(void)(b))
#define qglTexCoord2i( a, b ) ((void)(a),(void)(b))
#define qglTexCoord2s( a, b ) ((void)(a),(void)(b))
#define qglTexCoord3d( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexCoord3f( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexCoord3fv( a ) ((void)(a))
#define qglTexCoord3i( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexCoord4d( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglTexCoord4f( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglTexCoord4fv( a ) ((void)(a))
#define qglTexCoordPointer GLES_TexCoordPointer
#define qglTexEnvf GLES_TexEnvf
#define qglTexEnvfv( a, b, c ) GLES_TexEnvf( a, b, (c) ? (c)[0] : 0.0f )
#define qglTexEnvi( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexEnviv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexGend( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexGenf( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexGenfv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexGeni( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexGeniv( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglTexImage1D( a, b, c, d, e, f, g, h ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g),(void)(h))
#define qglTexImage2D glTexImage2D
#define qglTexParameterf glTexParameterf
#define qglTexParameterfv( a, b, c ) glTexParameterfv( a, b, c )
#define qglTexParameteri glTexParameteri
#define qglTexParameteriv( a, b, c ) glTexParameteriv( a, b, c )
#define qglTexSubImage1D( a, b, c, d, e, f, g ) ((void)(a),(void)(b),(void)(c),(void)(d),(void)(e),(void)(f),(void)(g))
#define qglTexSubImage2D glTexSubImage2D
#define qglTranslated( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglVertex2d( a, b ) ((void)(a),(void)(b))
#define qglVertex2i( a, b ) ((void)(a),(void)(b))
#define qglVertex2s( a, b ) ((void)(a),(void)(b))
#define qglVertex3d( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglVertex3i( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglVertex3s( a, b, c ) ((void)(a),(void)(b),(void)(c))
#define qglVertex4d( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglVertex4f( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglVertex4fv( a ) ((void)(a))
#define qglVertex4i( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglVertex4s( a, b, c, d ) ((void)(a),(void)(b),(void)(c),(void)(d))
#define qglVertexPointer GLES_VertexPointer
#define qglViewport glViewport

void GLES_LoadIdentity( void );
void GLES_MatrixMode( GLenum mode );
void GLES_MultMatrixf( const GLfloat *m );
void GLES_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
void GLES_Scalef( GLfloat x, GLfloat y, GLfloat z );

#endif /* QGL_GLES_H */
