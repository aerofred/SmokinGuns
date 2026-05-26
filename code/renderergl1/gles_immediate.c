/*
===========================================================================
OpenGL ES 2 immediate-mode + matrix stack emulation for renderergl1
===========================================================================
*/
#include "tr_local.h"

#ifdef USE_GLES

#include "../renderercommon/qgl_gles.h"

void ( *qglLockArraysEXT )( GLint first, GLsizei count ) = NULL;
void ( *qglUnlockArraysEXT )( void ) = NULL;

static void GL_APIENTRY Gles_LockArraysStub( GLint first, GLsizei count )
{
	(void)first;
	(void)count;
}

static void GL_APIENTRY Gles_UnlockArraysStub( void )
{
}

static struct {
	const void *vertex;
	const void *color;
	const void *texcoord;
	GLint vertexSize;
	GLint colorSize;
	GLint texSize;
	GLsizei vertexStride;
	GLsizei colorStride;
	GLsizei texStride;
	qboolean vertexEnabled;
	qboolean colorEnabled;
	qboolean texEnabled;
} glesClient;

#define GLES_MAX_VERTS 4096
#define GLES_MAX_STACK 32

typedef struct {
	GLfloat xyz[3];
	GLfloat st[2];
	GLfloat st2[2];
	GLubyte color[4];
} glesVertex_t;

static glesVertex_t glesBatch[GLES_MAX_VERTS];
static glesVertex_t glesVerts[GLES_MAX_VERTS];
static int glesVertCount;
static GLenum glesPrim;
static qboolean glesInBegin;

static GLfloat glesMatrixStack[GLES_MAX_STACK][16];
static int glesStackDepth;
static GLfloat glesMatrix[16];
static GLfloat glesProjection[16];
static GLfloat glesModelView[16];
static GLenum glesMatrixMode = GL_MODELVIEW;

static GLuint glesProgram;
static GLint glesLocMVP;
static GLint glesLocTex0;
static GLint glesLocTex1;
static GLint glesLocColor;

static int glesActiveTex;
static GLfloat glesCurST[2];
static GLfloat glesCurST2[2];
static GLubyte glesCurColor[4];
static qboolean glesTex2DEnabled;

static const char *glesVertSrc =
	"attribute vec3 aPos;\n"
	"attribute vec2 aTex0;\n"
	"attribute vec2 aTex1;\n"
	"attribute vec4 aColor;\n"
	"uniform mat4 uMVP;\n"
	"varying vec2 vTex0;\n"
	"varying vec2 vTex1;\n"
	"varying vec4 vColor;\n"
	"void main() {\n"
	"  gl_Position = uMVP * vec4(aPos, 1.0);\n"
	"  vTex0 = aTex0;\n"
	"  vTex1 = aTex1;\n"
	"  vColor = aColor;\n"
	"}\n";

static const char *glesFragSrc =
	"precision mediump float;\n"
	"varying vec2 vTex0;\n"
	"varying vec2 vTex1;\n"
	"varying vec4 vColor;\n"
	"uniform sampler2D uTex0;\n"
	"uniform sampler2D uTex1;\n"
	"uniform int uUseTex1;\n"
	"void main() {\n"
	"  vec4 c = vColor;\n"
	"  c *= texture2D(uTex0, vTex0);\n"
	"  if (uUseTex1 != 0) c *= texture2D(uTex1, vTex1);\n"
	"  gl_FragColor = c;\n"
	"}\n";

static GLuint Gles_CompileShader( GLenum type, const char *src )
{
	GLuint sh = glCreateShader( type );
	glShaderSource( sh, 1, &src, NULL );
	glCompileShader( sh );
	return sh;
}

static void Gles_MatIdentity( GLfloat *m )
{
	int i;
	for ( i = 0; i < 16; i++ )
		m[i] = 0.0f;
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void Gles_MatCopy( const GLfloat *src, GLfloat *dst )
{
	memcpy( dst, src, 16 * sizeof( GLfloat ) );
}

static void Gles_Multiply4x4( const GLfloat *a, const GLfloat *b, GLfloat *out )
{
	int i, j, k;
	for ( i = 0; i < 4; i++ )
		for ( j = 0; j < 4; j++ )
		{
			out[i + j * 4] = 0.0f;
			for ( k = 0; k < 4; k++ )
				out[i + j * 4] += a[i + k * 4] * b[k + j * 4];
		}
}

void GLES_MatrixMode( GLenum mode )
{
	glesMatrixMode = mode;
	if ( mode == GL_PROJECTION )
		Gles_MatCopy( glesProjection, glesMatrix );
	else
		Gles_MatCopy( glesModelView, glesMatrix );
}

void GLES_LoadIdentity( void )
{
	Gles_MatIdentity( glesMatrix );
	if ( glesMatrixMode == GL_PROJECTION )
		Gles_MatIdentity( glesProjection );
	else
		Gles_MatIdentity( glesModelView );
}

void GLES_PushMatrix( void )
{
	if ( glesStackDepth < GLES_MAX_STACK - 1 )
	{
		Gles_MatCopy( glesMatrix, glesMatrixStack[glesStackDepth] );
		glesStackDepth++;
	}
}

void GLES_PopMatrix( void )
{
	if ( glesStackDepth > 0 )
	{
		glesStackDepth--;
		Gles_MatCopy( glesMatrixStack[glesStackDepth], glesMatrix );
	}
}

void GLES_LoadMatrixf( const GLfloat *m )
{
	Gles_MatCopy( m, glesMatrix );
	if ( glesMatrixMode == GL_PROJECTION )
		Gles_MatCopy( m, glesProjection );
	else
		Gles_MatCopy( m, glesModelView );
}

void GLES_MultMatrixf( const GLfloat *m )
{
	GLfloat tmp[16];
	Gles_Multiply4x4( glesMatrix, m, tmp );
	Gles_MatCopy( tmp, glesMatrix );
	if ( glesMatrixMode == GL_PROJECTION )
		Gles_MatCopy( tmp, glesProjection );
	else
		Gles_MatCopy( tmp, glesModelView );
}

void GLES_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
	GLfloat m[16];
	GLfloat len;
	GLfloat c, s, nc;
	GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;

	len = sqrtf( x * x + y * y + z * z );
	if ( len == 0.0f )
		return;

	x /= len;
	y /= len;
	z /= len;

	angle = angle * (float)M_PI / 180.0f;
	c = cosf( angle );
	s = sinf( angle );
	nc = 1.0f - c;
	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = x * y;
	yz = y * z;
	zx = z * x;
	xs = x * s;
	ys = y * s;
	zs = z * s;

	Gles_MatIdentity( m );
	m[0] = xx + c * ( 1.0f - xx );
	m[1] = xy + c * ( -xy ) + zs;
	m[2] = zx + c * ( -zx ) - ys;
	m[4] = xy + c * ( -xy ) - zs;
	m[5] = yy + c * ( 1.0f - yy );
	m[6] = yz + c * ( -yz ) + xs;
	m[8] = zx + c * ( -zx ) + ys;
	m[9] = yz + c * ( -yz ) - xs;
	m[10] = zz + c * ( 1.0f - zz );

	GLES_MultMatrixf( m );
}

void GLES_Ortho( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
	GLfloat zNear, GLfloat zFar )
{
	GLfloat m[16];
	GLfloat rl = right - left;
	GLfloat tb = top - bottom;
	GLfloat fn = zFar - zNear;

	if ( rl == 0.0f || tb == 0.0f || fn == 0.0f )
		return;

	Gles_MatIdentity( m );
	m[0] = 2.0f / rl;
	m[5] = 2.0f / tb;
	m[10] = -2.0f / fn;
	m[12] = -( right + left ) / rl;
	m[13] = -( top + bottom ) / tb;
	m[14] = -( zFar + zNear ) / fn;
	m[15] = 1.0f;

	GLES_MultMatrixf( m );
}

void GLES_Vertex2f( GLfloat x, GLfloat y )
{
	GLES_Vertex3f( x, y, 0.0f );
}

void GLES_Vertex2fv( const GLfloat *v )
{
	GLES_Vertex3f( v[0], v[1], 0.0f );
}

void GLES_Enable( GLenum cap )
{
	if ( cap == GL_TEXTURE_2D )
	{
		glesTex2DEnabled = qtrue;
		return;
	}
	glEnable( cap );
}

void GLES_Disable( GLenum cap )
{
	if ( cap == GL_TEXTURE_2D )
	{
		glesTex2DEnabled = qfalse;
		return;
	}
	glDisable( cap );
}

void GLES_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
	glesMatrix[0] *= x;
	glesMatrix[5] *= y;
	glesMatrix[10] *= z;
}

void GLES_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
	glesMatrix[12] += x;
	glesMatrix[13] += y;
	glesMatrix[14] += z;
}

void GLES_InitImmediate( void )
{
	GLuint vs, fs;

	if ( glesProgram )
		return;

	Gles_MatIdentity( glesMatrix );
	Gles_MatIdentity( glesProjection );
	Gles_MatIdentity( glesModelView );
	glesStackDepth = 0;
	glesCurColor[0] = glesCurColor[1] = glesCurColor[2] = glesCurColor[3] = 255;
	glesTex2DEnabled = qtrue;

	vs = Gles_CompileShader( GL_VERTEX_SHADER, glesVertSrc );
	fs = Gles_CompileShader( GL_FRAGMENT_SHADER, glesFragSrc );
	glesProgram = glCreateProgram();
	glAttachShader( glesProgram, vs );
	glAttachShader( glesProgram, fs );
	glLinkProgram( glesProgram );
	glDeleteShader( vs );
	glDeleteShader( fs );

	glesLocMVP = glGetUniformLocation( glesProgram, "uMVP" );
	glesLocTex0 = glGetUniformLocation( glesProgram, "uTex0" );
	glesLocTex1 = glGetUniformLocation( glesProgram, "uTex1" );
	glesLocColor = glGetUniformLocation( glesProgram, "uColor" );
	glUseProgram( glesProgram );

	qglLockArraysEXT = Gles_LockArraysStub;
	qglUnlockArraysEXT = Gles_UnlockArraysStub;
	Com_Memset( &glesClient, 0, sizeof( glesClient ) );
}

void GLES_ShutdownImmediate( void )
{
	if ( glesProgram )
	{
		glDeleteProgram( glesProgram );
		glesProgram = 0;
	}
}

void GLES_Begin( GLenum mode )
{
	glesPrim = mode;
	glesVertCount = 0;
	glesInBegin = qtrue;
}

void GLES_Color4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
	glesCurColor[0] = (GLubyte)(r * 255.0f);
	glesCurColor[1] = (GLubyte)(g * 255.0f);
	glesCurColor[2] = (GLubyte)(b * 255.0f);
	glesCurColor[3] = (GLubyte)(a * 255.0f);
}

void GLES_Color4ubv( const GLubyte *v )
{
	glesCurColor[0] = v[0];
	glesCurColor[1] = v[1];
	glesCurColor[2] = v[2];
	glesCurColor[3] = v[3];
}

void GLES_Color3f( GLfloat r, GLfloat g, GLfloat b )
{
	GLES_Color4f( r, g, b, 1.0f );
}

void GLES_TexCoord2f( GLfloat s, GLfloat t )
{
	glesCurST[0] = s;
	glesCurST[1] = t;
}

void GLES_TexCoord2fv( const GLfloat *v )
{
	glesCurST[0] = v[0];
	glesCurST[1] = v[1];
}

void GLES_MultiTexCoord2f( GLenum unit, GLfloat s, GLfloat t )
{
	if ( unit == GL_TEXTURE1 || unit == GL_TEXTURE0 + 1 )
	{
		glesCurST2[0] = s;
		glesCurST2[1] = t;
	}
	else
	{
		glesCurST[0] = s;
		glesCurST[1] = t;
	}
}

void GLES_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
	glesVertex_t *v;

	if ( !glesInBegin || glesVertCount >= GLES_MAX_VERTS )
		return;

	v = &glesVerts[glesVertCount++];
	v->xyz[0] = x;
	v->xyz[1] = y;
	v->xyz[2] = z;
	v->st[0] = glesCurST[0];
	v->st[1] = glesCurST[1];
	v->st2[0] = glesCurST2[0];
	v->st2[1] = glesCurST2[1];
	memcpy( v->color, glesCurColor, 4 );
}

void GLES_Vertex3fv( const GLfloat *xyz )
{
	GLES_Vertex3f( xyz[0], xyz[1], xyz[2] );
}

static void Gles_DrawIndexed( GLenum mode, int count )
{
	unsigned short *idx;
	int i, n = 0;
	int j;

	idx = (unsigned short *)malloc( count * 3 * sizeof( unsigned short ) );
	if ( !idx )
		return;

	switch ( mode )
	{
	case GL_TRIANGLES:
		for ( i = 0; i + 2 < count; i += 3 )
		{
			idx[n++] = i;
			idx[n++] = i + 1;
			idx[n++] = i + 2;
		}
		break;
	case GL_TRIANGLE_STRIP:
		for ( i = 0; i + 2 < count; i++ )
		{
			if ( i & 1 )
			{
				idx[n++] = i;
				idx[n++] = i + 2;
				idx[n++] = i + 1;
			}
			else
			{
				idx[n++] = i;
				idx[n++] = i + 1;
				idx[n++] = i + 2;
			}
		}
		break;
	case GL_QUADS:
		for ( i = 0; i + 3 < count; i += 4 )
		{
			idx[n++] = i;
			idx[n++] = i + 1;
			idx[n++] = i + 2;
			idx[n++] = i;
			idx[n++] = i + 2;
			idx[n++] = i + 3;
		}
		break;
	case GL_LINES:
		for ( i = 0; i + 1 < count; i += 2 )
		{
			idx[n++] = i;
			idx[n++] = i + 1;
		}
		break;
	case GL_POLYGON:
		if ( count >= 3 )
		{
			for ( j = 1; j < count - 1; j++ )
			{
				idx[n++] = 0;
				idx[n++] = j;
				idx[n++] = j + 1;
			}
		}
		break;
	default:
		free( idx );
		return;
	}

	if ( n > 0 )
		glDrawElements( GL_TRIANGLES, n, GL_UNSIGNED_SHORT, idx );

	free( idx );
}

void GLES_End( void )
{
	GLint locPos, locT0, locT1, locCol;
	GLint useTex1;

	if ( !glesInBegin || glesVertCount < 1 )
	{
		glesInBegin = qfalse;
		return;
	}

	{
		GLfloat mvp[16];
		Gles_Multiply4x4( glesProjection, glesModelView, mvp );
		glUseProgram( glesProgram );
		glUniformMatrix4fv( glesLocMVP, 1, GL_FALSE, mvp );
	}

	locPos = glGetAttribLocation( glesProgram, "aPos" );
	locT0 = glGetAttribLocation( glesProgram, "aTex0" );
	locT1 = glGetAttribLocation( glesProgram, "aTex1" );
	locCol = glGetAttribLocation( glesProgram, "aColor" );
	useTex1 = glGetUniformLocation( glesProgram, "uUseTex1" );

	glUniform1i( glGetUniformLocation( glesProgram, "uTex0" ), 0 );
	glUniform1i( glGetUniformLocation( glesProgram, "uTex1" ), 1 );
	glUniform1i( useTex1, ( glesTex2DEnabled ? 1 : 0 ) );

	glEnableVertexAttribArray( locPos );
	glEnableVertexAttribArray( locT0 );
	glEnableVertexAttribArray( locT1 );
	glEnableVertexAttribArray( locCol );

	glVertexAttribPointer( locPos, 3, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesVerts[0].xyz );
	glVertexAttribPointer( locT0, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesVerts[0].st );
	glVertexAttribPointer( locT1, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesVerts[0].st2 );
	glVertexAttribPointer( locCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( glesVertex_t ), glesVerts[0].color );

	Gles_DrawIndexed( glesPrim, glesVertCount );

	glDisableVertexAttribArray( locPos );
	glDisableVertexAttribArray( locT0 );
	glDisableVertexAttribArray( locT1 );
	glDisableVertexAttribArray( locCol );

	glesInBegin = qfalse;
	glesVertCount = 0;
}

static void Gles_ApplyMVP( void )
{
	GLfloat mvp[16];

	Gles_Multiply4x4( glesProjection, glesModelView, mvp );
	glUseProgram( glesProgram );
	glUniformMatrix4fv( glesLocMVP, 1, GL_FALSE, mvp );
}

static void Gles_FetchVertex( int idx, glesVertex_t *v )
{
	const char *base;
	const float *f;
	const GLubyte *c;

	if ( glesClient.vertexEnabled && glesClient.vertex )
	{
		base = (const char *)glesClient.vertex + idx * glesClient.vertexStride;
		f = (const float *)base;
		v->xyz[0] = f[0];
		v->xyz[1] = f[1];
		v->xyz[2] = f[2];
	}
	else
	{
		v->xyz[0] = v->xyz[1] = v->xyz[2] = 0.0f;
	}

	if ( glesClient.colorEnabled && glesClient.color )
	{
		if ( glesClient.colorStride )
			base = (const char *)glesClient.color + idx * glesClient.colorStride;
		else
			base = (const char *)glesClient.color + idx * glesClient.colorSize;
		c = (const GLubyte *)base;
		v->color[0] = c[0];
		v->color[1] = c[1];
		v->color[2] = c[2];
		v->color[3] = c[3];
	}
	else
	{
		v->color[0] = v->color[1] = v->color[2] = v->color[3] = 255;
	}

	if ( glesClient.texEnabled && glesClient.texcoord )
	{
		base = (const char *)glesClient.texcoord + idx * glesClient.texStride;
		f = (const float *)base;
		v->st[0] = f[0];
		v->st[1] = f[1];
	}
	else
	{
		v->st[0] = v->st[1] = 0.0f;
	}

	v->st2[0] = glesCurST2[0];
	v->st2[1] = glesCurST2[1];
}

static void Gles_DrawBatchIndexed( GLsizei count, GLenum indexType, const void *indices, GLenum mode )
{
	GLint locPos, locT0, locT1, locCol;
	GLint useTex1;
	GLsizei i;
	GLushort shortIdx[GLES_MAX_VERTS];
	int verts = 0;

	if ( !glesProgram || count <= 0 )
		return;

	for ( i = 0; i < count && verts < GLES_MAX_VERTS; i++ )
	{
		int idx;

		if ( indexType == GL_UNSIGNED_INT )
			idx = (int)( (const unsigned int *)indices )[i];
		else if ( indexType == GL_UNSIGNED_SHORT )
			idx = (int)( (const unsigned short *)indices )[i];
		else
			idx = (int)( (const unsigned char *)indices )[i];

		Gles_FetchVertex( idx, &glesBatch[verts] );
		shortIdx[verts] = (GLushort)verts;
		verts++;
	}

	if ( verts < 1 )
		return;

	Gles_ApplyMVP();

	locPos = glGetAttribLocation( glesProgram, "aPos" );
	locT0 = glGetAttribLocation( glesProgram, "aTex0" );
	locT1 = glGetAttribLocation( glesProgram, "aTex1" );
	locCol = glGetAttribLocation( glesProgram, "aColor" );
	useTex1 = glGetUniformLocation( glesProgram, "uUseTex1" );

	glUniform1i( glGetUniformLocation( glesProgram, "uTex0" ), 0 );
	glUniform1i( glGetUniformLocation( glesProgram, "uTex1" ), 1 );
	glUniform1i( useTex1, 0 );

	glEnableVertexAttribArray( locPos );
	glEnableVertexAttribArray( locT0 );
	glEnableVertexAttribArray( locT1 );
	glEnableVertexAttribArray( locCol );

	glVertexAttribPointer( locPos, 3, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].xyz );
	glVertexAttribPointer( locT0, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].st );
	glVertexAttribPointer( locT1, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].st2 );
	glVertexAttribPointer( locCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( glesVertex_t ), glesBatch[0].color );

	glDrawElements( mode, verts, GL_UNSIGNED_SHORT, shortIdx );

	glDisableVertexAttribArray( locPos );
	glDisableVertexAttribArray( locT0 );
	glDisableVertexAttribArray( locT1 );
	glDisableVertexAttribArray( locCol );
}

void GLES_EnableClientState( GLenum array )
{
	switch ( array )
	{
	case GL_VERTEX_ARRAY: glesClient.vertexEnabled = qtrue; break;
	case GL_COLOR_ARRAY: glesClient.colorEnabled = qtrue; break;
	case GL_TEXTURE_COORD_ARRAY: glesClient.texEnabled = qtrue; break;
	default: break;
	}
}

void GLES_DisableClientState( GLenum array )
{
	switch ( array )
	{
	case GL_VERTEX_ARRAY: glesClient.vertexEnabled = qfalse; break;
	case GL_COLOR_ARRAY: glesClient.colorEnabled = qfalse; break;
	case GL_TEXTURE_COORD_ARRAY: glesClient.texEnabled = qfalse; break;
	default: break;
	}
}

void GLES_VertexPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
	(void)type;
	glesClient.vertex = pointer;
	glesClient.vertexSize = size;
	glesClient.vertexStride = stride ? stride : (GLsizei)( size * (GLint)sizeof( float ) );
}

void GLES_ColorPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
	(void)type;
	glesClient.color = pointer;
	glesClient.colorSize = size;
	glesClient.colorStride = stride ? stride : size;
}

void GLES_TexCoordPointer( GLint size, GLenum type, GLsizei stride, const void *pointer )
{
	(void)type;
	glesClient.texcoord = pointer;
	glesClient.texSize = size;
	glesClient.texStride = stride ? stride : (GLsizei)( size * (GLint)sizeof( float ) );
}

void GLES_NormalPointer( GLenum type, GLsizei stride, const void *pointer )
{
	(void)type;
	(void)stride;
	(void)pointer;
}

void GLES_DrawElements( GLenum mode, GLsizei count, GLenum type, const void *indices )
{
	Gles_DrawBatchIndexed( count, type, indices, mode );
}

void GLES_DrawArrays( GLenum mode, GLint first, GLsizei count )
{
	GLushort shortIdx[GLES_MAX_VERTS];
	GLint locPos, locT0, locT1, locCol;
	GLint useTex1;
	GLsizei i;

	if ( !glesProgram || count <= 0 )
		return;
	if ( count > GLES_MAX_VERTS )
		count = GLES_MAX_VERTS;

	for ( i = 0; i < count; i++ )
	{
		Gles_FetchVertex( first + i, &glesBatch[i] );
		shortIdx[i] = (GLushort)i;
	}

	Gles_ApplyMVP();

	locPos = glGetAttribLocation( glesProgram, "aPos" );
	locT0 = glGetAttribLocation( glesProgram, "aTex0" );
	locT1 = glGetAttribLocation( glesProgram, "aTex1" );
	locCol = glGetAttribLocation( glesProgram, "aColor" );
	useTex1 = glGetUniformLocation( glesProgram, "uUseTex1" );

	glUniform1i( glGetUniformLocation( glesProgram, "uTex0" ), 0 );
	glUniform1i( glGetUniformLocation( glesProgram, "uTex1" ), 1 );
	glUniform1i( useTex1, 0 );

	glEnableVertexAttribArray( locPos );
	glEnableVertexAttribArray( locT0 );
	glEnableVertexAttribArray( locT1 );
	glEnableVertexAttribArray( locCol );

	glVertexAttribPointer( locPos, 3, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].xyz );
	glVertexAttribPointer( locT0, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].st );
	glVertexAttribPointer( locT1, 2, GL_FLOAT, GL_FALSE, sizeof( glesVertex_t ), &glesBatch[0].st2 );
	glVertexAttribPointer( locCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( glesVertex_t ), glesBatch[0].color );

	glDrawElements( mode, count, GL_UNSIGNED_SHORT, shortIdx );

	glDisableVertexAttribArray( locPos );
	glDisableVertexAttribArray( locT0 );
	glDisableVertexAttribArray( locT1 );
	glDisableVertexAttribArray( locCol );
}

void GLES_ArrayElement( GLint i )
{
	GLushort idx = (GLushort)i;
	Gles_DrawBatchIndexed( 1, GL_UNSIGNED_SHORT, &idx, GL_TRIANGLES );
}

#endif /* USE_GLES */
