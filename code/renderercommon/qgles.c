/*
===========================================================================
Small immediate-mode bridge for the iOS OpenGL ES 1.x renderer path.
===========================================================================
*/

#include "qgl.h"

#ifdef IOS

#undef glTexImage2D

#define QGLES_MAX_IMMEDIATE_VERTS 4096

typedef struct
{
	GLfloat xyz[3];
	GLfloat st[2];
	GLubyte color[4];
} qglesVertex_t;

static qglesVertex_t qglesVerts[QGLES_MAX_IMMEDIATE_VERTS];
static int qglesNumVerts;
static GLenum qglesMode;
static GLfloat qglesST[2];
static GLubyte qglesColor[4] = { 255, 255, 255, 255 };

static void qglesEmitVertex( GLfloat x, GLfloat y, GLfloat z )
{
	qglesVertex_t *v;

	if( qglesNumVerts >= QGLES_MAX_IMMEDIATE_VERTS )
		return;

	v = &qglesVerts[qglesNumVerts++];
	v->xyz[0] = x;
	v->xyz[1] = y;
	v->xyz[2] = z;
	v->st[0] = qglesST[0];
	v->st[1] = qglesST[1];
	v->color[0] = qglesColor[0];
	v->color[1] = qglesColor[1];
	v->color[2] = qglesColor[2];
	v->color[3] = qglesColor[3];
}

static void qglesDrawRange( GLenum mode, int first, int count )
{
	if( count <= 0 )
		return;

	glVertexPointer( 3, GL_FLOAT, sizeof( qglesVertex_t ), qglesVerts[first].xyz );
	glTexCoordPointer( 2, GL_FLOAT, sizeof( qglesVertex_t ), qglesVerts[first].st );
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( qglesVertex_t ), qglesVerts[first].color );
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
	glDrawArrays( mode, 0, count );
}

void qglesBegin( GLenum mode )
{
	qglesMode = mode;
	qglesNumVerts = 0;
}

void qglesEnd( void )
{
	int i;

	if( qglesMode == GL_QUADS )
	{
		for( i = 0; i + 3 < qglesNumVerts; i += 4 )
		{
			GLubyte indices[6] = { 0, 1, 2, 0, 2, 3 };

			glVertexPointer( 3, GL_FLOAT, sizeof( qglesVertex_t ), qglesVerts[i].xyz );
			glTexCoordPointer( 2, GL_FLOAT, sizeof( qglesVertex_t ), qglesVerts[i].st );
			glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( qglesVertex_t ), qglesVerts[i].color );
			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glEnableClientState( GL_COLOR_ARRAY );
			glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices );
		}
	}
	else if( qglesMode == GL_POLYGON )
	{
		qglesDrawRange( GL_TRIANGLE_FAN, 0, qglesNumVerts );
	}
	else
	{
		qglesDrawRange( qglesMode, 0, qglesNumVerts );
	}

	qglesNumVerts = 0;
}

void qglesColor3f( GLfloat r, GLfloat g, GLfloat b )
{
	qglesColor[0] = (GLubyte)( r * 255.0f );
	qglesColor[1] = (GLubyte)( g * 255.0f );
	qglesColor[2] = (GLubyte)( b * 255.0f );
	qglesColor[3] = 255;
	glColor4ub( qglesColor[0], qglesColor[1], qglesColor[2], qglesColor[3] );
}

void qglesColor4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
	qglesColor[0] = (GLubyte)( r * 255.0f );
	qglesColor[1] = (GLubyte)( g * 255.0f );
	qglesColor[2] = (GLubyte)( b * 255.0f );
	qglesColor[3] = (GLubyte)( a * 255.0f );
	glColor4ub( qglesColor[0], qglesColor[1], qglesColor[2], qglesColor[3] );
}

void qglesColor4ubv( const GLubyte *v )
{
	qglesColor[0] = v[0];
	qglesColor[1] = v[1];
	qglesColor[2] = v[2];
	qglesColor[3] = v[3];
	glColor4ubv( v );
}

void qglesTexCoord2f( GLfloat s, GLfloat t )
{
	qglesST[0] = s;
	qglesST[1] = t;
}

void qglesTexCoord2fv( const GLfloat *v )
{
	qglesTexCoord2f( v[0], v[1] );
}

void qglesVertex2f( GLfloat x, GLfloat y )
{
	qglesEmitVertex( x, y, 0.0f );
}

void qglesVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
	qglesEmitVertex( x, y, z );
}

void qglesVertex3fv( const GLfloat *v )
{
	qglesVertex3f( v[0], v[1], v[2] );
}

void qglesMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t )
{
	glClientActiveTexture( target );
	qglesTexCoord2f( s, t );
}

void qglesDrawBuffer( GLenum mode )
{
	(void)mode;
}

void qglesReadBuffer( GLenum mode )
{
	(void)mode;
}

void qglesPolygonMode( GLenum face, GLenum mode )
{
	(void)face;
	(void)mode;
}

void qglesClearDepth( GLclampf depth )
{
	glClearDepthf( depth );
}

void qglesDepthRange( GLclampf nearVal, GLclampf farVal )
{
	glDepthRangef( nearVal, farVal );
}

void qglesLockArrays( GLint first, GLsizei count )
{
	(void)first;
	(void)count;
}

void qglesUnlockArrays( void )
{
}

static GLint qglesNormalizeInternalFormat( GLint internalformat )
{
	switch( internalformat )
	{
		case GL_RGB8:
		case GL_RGB5:
		case GL_RGB4_S3TC:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return GL_RGB;
		case GL_RGBA8:
		case GL_RGBA4:
			return GL_RGBA;
		case GL_LUMINANCE8:
		case GL_LUMINANCE16:
			return GL_LUMINANCE;
		case GL_LUMINANCE8_ALPHA8:
		case GL_LUMINANCE16_ALPHA16:
			return GL_LUMINANCE_ALPHA;
		default:
			return internalformat;
	}
}

void qglesTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
	glTexImage2D( target, level, qglesNormalizeInternalFormat( internalformat ), width, height, border, format, type, pixels );
}

void qglesArrayElement( GLint i )
{
	(void)i;
}

#endif
