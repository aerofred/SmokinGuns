/*
===========================================================================
OpenGL ES 1.x compatibility declarations for the iOS renderer path.
===========================================================================
*/

#ifndef __QGL_ES1_H__
#define __QGL_ES1_H__

#ifdef USE_LOCAL_HEADERS
#	include "SDL_opengles.h"
#else
#	include <OpenGLES/ES1/gl.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef GLsizeiptr GLsizeiptrARB;
typedef GLintptr GLintptrARB;
typedef GLuint GLhandleARB;
typedef GLchar GLcharARB;

#ifndef GL_CLAMP
#define GL_CLAMP GL_CLAMP_TO_EDGE
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE GL_FLOAT
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT GL_BACK
#endif
#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT GL_BACK
#endif
#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_RGB8
#define GL_RGB8 0x8051
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_RGBA4
#define GL_RGBA4 0x8056
#endif
#ifndef GL_RGB5
#define GL_RGB5 0x8057
#endif
#ifndef GL_LUMINANCE8
#define GL_LUMINANCE8 0x8040
#endif
#ifndef GL_LUMINANCE16
#define GL_LUMINANCE16 0x8042
#endif
#ifndef GL_LUMINANCE8_ALPHA8
#define GL_LUMINANCE8_ALPHA8 0x8045
#endif
#ifndef GL_LUMINANCE16_ALPHA16
#define GL_LUMINANCE16_ALPHA16 0x8048
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_RGB4_S3TC
#define GL_RGB4_S3TC 0x83A1
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 GL_TEXTURE0_ARB
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 GL_TEXTURE1_ARB
#endif
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB 0x84C0
#endif
#ifndef GL_TEXTURE1_ARB
#define GL_TEXTURE1_ARB 0x84C1
#endif
#ifndef GL_MAX_TEXTURE_UNITS_ARB
#define GL_MAX_TEXTURE_UNITS_ARB GL_MAX_TEXTURE_UNITS
#endif
#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW 0x0503
#endif
#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 0x0504
#endif

void qglesBegin( GLenum mode );
void qglesEnd( void );
void qglesColor3f( GLfloat r, GLfloat g, GLfloat b );
void qglesColor4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void qglesColor4ubv( const GLubyte *v );
void qglesTexCoord2f( GLfloat s, GLfloat t );
void qglesTexCoord2fv( const GLfloat *v );
void qglesVertex2f( GLfloat x, GLfloat y );
void qglesVertex3f( GLfloat x, GLfloat y, GLfloat z );
void qglesVertex3fv( const GLfloat *v );
void qglesMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t );
void qglesDrawBuffer( GLenum mode );
void qglesReadBuffer( GLenum mode );
void qglesPolygonMode( GLenum face, GLenum mode );
void qglesClearDepth( GLclampf depth );
void qglesDepthRange( GLclampf nearVal, GLclampf farVal );
void qglesLockArrays( GLint first, GLsizei count );
void qglesUnlockArrays( void );
void qglesTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void qglesArrayElement( GLint i );

#define glBegin qglesBegin
#define glEnd qglesEnd
#define glColor3f qglesColor3f
#define glColor4f qglesColor4f
#define glColor4ubv qglesColor4ubv
#define glTexCoord2f qglesTexCoord2f
#define glTexCoord2fv qglesTexCoord2fv
#define glVertex2f qglesVertex2f
#define glVertex3f qglesVertex3f
#define glVertex3fv qglesVertex3fv
#define glMultiTexCoord2fARB qglesMultiTexCoord2f
#define glActiveTextureARB glActiveTexture
#define glClientActiveTextureARB glClientActiveTexture
#define glDrawBuffer qglesDrawBuffer
#define glReadBuffer qglesReadBuffer
#define glPolygonMode qglesPolygonMode
#define glOrtho glOrthof
#define glClearDepth qglesClearDepth
#define glDepthRange qglesDepthRange
#define glLockArraysEXT qglesLockArrays
#define glUnlockArraysEXT qglesUnlockArrays
#define glTexImage2D qglesTexImage2D
#define glArrayElement qglesArrayElement
#define glTexImage1D(...) ((void)0)
#define glCopyTexImage1D(...) ((void)0)
#define glCopyTexSubImage1D(...) ((void)0)
#define glGetTexImage(...) ((void)0)
#define glGetTexLevelParameterfv(...) ((void)0)
#define glGetTexLevelParameteriv(...) ((void)0)
#define glCallList(...) ((void)0)
#define glCallLists(...) ((void)0)
#define glNewList(...) ((void)0)
#define glEndList(...) ((void)0)
#define glGenLists(...) (0)
#define glDeleteLists(...) ((void)0)
#define glListBase(...) ((void)0)
#define glIsList(...) (GL_FALSE)
#define glInitNames(...) ((void)0)
#define glLoadName(...) ((void)0)
#define glPushName(...) ((void)0)
#define glPopName(...) ((void)0)
#define glFeedbackBuffer(...) ((void)0)
#define glSelectBuffer(...) ((void)0)
#define glRenderMode(...) (0)
#define glPassThrough(...) ((void)0)
#define glPixelZoom(...) ((void)0)
#define glDrawPixels(...) ((void)0)
#define glCopyPixels(...) ((void)0)
#define glBitmap(...) ((void)0)
#define glAccum(...) ((void)0)
#define glClearAccum(...) ((void)0)
#define glIndexMask(...) ((void)0)
#define glIndexPointer(...) ((void)0)
#define glIndexd(...) ((void)0)
#define glIndexdv(...) ((void)0)
#define glIndexf(...) ((void)0)
#define glIndexfv(...) ((void)0)
#define glIndexi(...) ((void)0)
#define glIndexiv(...) ((void)0)
#define glIndexs(...) ((void)0)
#define glIndexsv(...) ((void)0)
#define glIndexub(...) ((void)0)
#define glIndexubv(...) ((void)0)
#define glAreTexturesResident(...) (GL_FALSE)
#define glEdgeFlag(...) ((void)0)
#define glEdgeFlagPointer(...) ((void)0)
#define glEdgeFlagv(...) ((void)0)
#define glClipPlane(...) ((void)0)
#define glGetClipPlane(...) ((void)0)
#define glGetMapdv(...) ((void)0)
#define glGetMapfv(...) ((void)0)
#define glGetMapiv(...) ((void)0)
#define glMap1d(...) ((void)0)
#define glMap1f(...) ((void)0)
#define glMap2d(...) ((void)0)
#define glMap2f(...) ((void)0)
#define glMapGrid1d(...) ((void)0)
#define glMapGrid1f(...) ((void)0)
#define glMapGrid2d(...) ((void)0)
#define glMapGrid2f(...) ((void)0)
#define glEvalCoord1d(...) ((void)0)
#define glEvalCoord1dv(...) ((void)0)
#define glEvalCoord1f(...) ((void)0)
#define glEvalCoord1fv(...) ((void)0)
#define glEvalCoord2d(...) ((void)0)
#define glEvalCoord2dv(...) ((void)0)
#define glEvalCoord2f(...) ((void)0)
#define glEvalCoord2fv(...) ((void)0)
#define glEvalMesh1(...) ((void)0)
#define glEvalMesh2(...) ((void)0)
#define glEvalPoint1(...) ((void)0)
#define glEvalPoint2(...) ((void)0)
#define glColor3b(r,g,b) qglesColor3f((r)/127.0f,(g)/127.0f,(b)/127.0f)
#define glColor3bv(v) qglesColor3f((v)[0]/127.0f,(v)[1]/127.0f,(v)[2]/127.0f)
#define glColor3d(r,g,b) qglesColor3f((GLfloat)(r),(GLfloat)(g),(GLfloat)(b))
#define glColor3dv(v) qglesColor3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glColor3fv(v) qglesColor3f((v)[0],(v)[1],(v)[2])
#define glColor3i(r,g,b) qglesColor3f((r)/2147483647.0f,(g)/2147483647.0f,(b)/2147483647.0f)
#define glColor3iv(v) qglesColor3f((v)[0]/2147483647.0f,(v)[1]/2147483647.0f,(v)[2]/2147483647.0f)
#define glColor3s(r,g,b) qglesColor3f((r)/32767.0f,(g)/32767.0f,(b)/32767.0f)
#define glColor3sv(v) qglesColor3f((v)[0]/32767.0f,(v)[1]/32767.0f,(v)[2]/32767.0f)
#define glColor3ub(r,g,b) qglesColor3f((r)/255.0f,(g)/255.0f,(b)/255.0f)
#define glColor3ubv(v) qglesColor3f((v)[0]/255.0f,(v)[1]/255.0f,(v)[2]/255.0f)
#define glColor3ui(r,g,b) qglesColor3f((r)/4294967295.0f,(g)/4294967295.0f,(b)/4294967295.0f)
#define glColor3uiv(v) qglesColor3f((v)[0]/4294967295.0f,(v)[1]/4294967295.0f,(v)[2]/4294967295.0f)
#define glColor3us(r,g,b) qglesColor3f((r)/65535.0f,(g)/65535.0f,(b)/65535.0f)
#define glColor3usv(v) qglesColor3f((v)[0]/65535.0f,(v)[1]/65535.0f,(v)[2]/65535.0f)
#define glColor4b(r,g,b,a) qglesColor4f((r)/127.0f,(g)/127.0f,(b)/127.0f,(a)/127.0f)
#define glColor4bv(v) qglesColor4f((v)[0]/127.0f,(v)[1]/127.0f,(v)[2]/127.0f,(v)[3]/127.0f)
#define glColor4d(r,g,b,a) qglesColor4f((GLfloat)(r),(GLfloat)(g),(GLfloat)(b),(GLfloat)(a))
#define glColor4dv(v) qglesColor4f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2],(GLfloat)(v)[3])
#define glColor4fv(v) qglesColor4f((v)[0],(v)[1],(v)[2],(v)[3])
#define glColor4i(r,g,b,a) qglesColor4f((r)/2147483647.0f,(g)/2147483647.0f,(b)/2147483647.0f,(a)/2147483647.0f)
#define glColor4iv(v) qglesColor4f((v)[0]/2147483647.0f,(v)[1]/2147483647.0f,(v)[2]/2147483647.0f,(v)[3]/2147483647.0f)
#define glColor4s(r,g,b,a) qglesColor4f((r)/32767.0f,(g)/32767.0f,(b)/32767.0f,(a)/32767.0f)
#define glColor4sv(v) qglesColor4f((v)[0]/32767.0f,(v)[1]/32767.0f,(v)[2]/32767.0f,(v)[3]/32767.0f)
#define glColor4ub(r,g,b,a) qglesColor4f((r)/255.0f,(g)/255.0f,(b)/255.0f,(a)/255.0f)
#define glColor4ui(r,g,b,a) qglesColor4f((r)/4294967295.0f,(g)/4294967295.0f,(b)/4294967295.0f,(a)/4294967295.0f)
#define glColor4uiv(v) qglesColor4f((v)[0]/4294967295.0f,(v)[1]/4294967295.0f,(v)[2]/4294967295.0f,(v)[3]/4294967295.0f)
#define glColor4us(r,g,b,a) qglesColor4f((r)/65535.0f,(g)/65535.0f,(b)/65535.0f,(a)/65535.0f)
#define glColor4usv(v) qglesColor4f((v)[0]/65535.0f,(v)[1]/65535.0f,(v)[2]/65535.0f,(v)[3]/65535.0f)
#define glTexCoord1d(s) qglesTexCoord2f((GLfloat)(s),0)
#define glTexCoord1dv(v) qglesTexCoord2f((GLfloat)(v)[0],0)
#define glTexCoord1f(s) qglesTexCoord2f((s),0)
#define glTexCoord1fv(v) qglesTexCoord2f((v)[0],0)
#define glTexCoord1i(s) qglesTexCoord2f((GLfloat)(s),0)
#define glTexCoord1iv(v) qglesTexCoord2f((GLfloat)(v)[0],0)
#define glTexCoord1s(s) qglesTexCoord2f((GLfloat)(s),0)
#define glTexCoord1sv(v) qglesTexCoord2f((GLfloat)(v)[0],0)
#define glTexCoord2d(s,t) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord2dv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord2i(s,t) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord2iv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord2s(s,t) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord2sv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord3d(s,t,r) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord3dv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord3f(s,t,r) qglesTexCoord2f((s),(t))
#define glTexCoord3fv(v) qglesTexCoord2f((v)[0],(v)[1])
#define glTexCoord3i(s,t,r) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord3iv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord3s(s,t,r) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord3sv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord4d(s,t,r,q) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord4dv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord4f(s,t,r,q) qglesTexCoord2f((s),(t))
#define glTexCoord4fv(v) qglesTexCoord2f((v)[0],(v)[1])
#define glTexCoord4i(s,t,r,q) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord4iv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glTexCoord4s(s,t,r,q) qglesTexCoord2f((GLfloat)(s),(GLfloat)(t))
#define glTexCoord4sv(v) qglesTexCoord2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glVertex2d(x,y) qglesVertex2f((GLfloat)(x),(GLfloat)(y))
#define glVertex2dv(v) qglesVertex2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glVertex2fv(v) qglesVertex2f((v)[0],(v)[1])
#define glVertex2i(x,y) qglesVertex2f((GLfloat)(x),(GLfloat)(y))
#define glVertex2iv(v) qglesVertex2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glVertex2s(x,y) qglesVertex2f((GLfloat)(x),(GLfloat)(y))
#define glVertex2sv(v) qglesVertex2f((GLfloat)(v)[0],(GLfloat)(v)[1])
#define glVertex3d(x,y,z) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex3dv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glVertex3i(x,y,z) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex3iv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glVertex3s(x,y,z) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex3sv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glVertex4d(x,y,z,w) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex4dv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glVertex4f(x,y,z,w) qglesVertex3f((x),(y),(z))
#define glVertex4fv(v) qglesVertex3f((v)[0],(v)[1],(v)[2])
#define glVertex4i(x,y,z,w) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex4iv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])
#define glVertex4s(x,y,z,w) qglesVertex3f((GLfloat)(x),(GLfloat)(y),(GLfloat)(z))
#define glVertex4sv(v) qglesVertex3f((GLfloat)(v)[0],(GLfloat)(v)[1],(GLfloat)(v)[2])

#endif
