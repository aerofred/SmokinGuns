#include <SDL.h>
#include <SDL_opengles.h>

#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"
#include "../client/cl_touch.h"

static SDL_Window *window;
static SDL_GLContext context;

cvar_t *r_allowSoftwareGL;
cvar_t *r_allowResize;
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();
	if( context )
		SDL_GL_DeleteContext( context );
	context = NULL;
	if( window )
		SDL_DestroyWindow( window );
	window = NULL;
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

void GLimp_Minimize( void )
{
}

void GLimp_LogComment( char *comment )
{
	(void)comment;
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	(void)red;
	(void)green;
	(void)blue;
}

static qboolean GLimp_CreateWindow( void )
{
	int width = 0;
	int height = 0;
	int drawableW = 0;
	int drawableH = 0;
	float aspect;

	if( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
	{
		ri.Printf( PRINT_ALL, "SDL_InitSubSystem(video) failed: %s\n", SDL_GetError() );
		return qfalse;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	window = SDL_CreateWindow( CLIENT_WINDOW_TITLE, 0, 0, 0, 0,
		SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI );
	if( !window )
	{
		ri.Printf( PRINT_ALL, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
		return qfalse;
	}

	context = SDL_GL_CreateContext( window );
	if( !context )
	{
		ri.Printf( PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
		return qfalse;
	}

	SDL_GL_MakeCurrent( window, context );
	SDL_GL_SetSwapInterval( r_swapInterval ? r_swapInterval->integer : 1 );
	SDL_GetWindowSize( window, &width, &height );
	SDL_GL_GetDrawableSize( window, &drawableW, &drawableH );
	if( drawableW <= 0 || drawableH <= 0 )
	{
		drawableW = width > 0 ? width : 1280;
		drawableH = height > 0 ? height : 720;
	}

	aspect = (float)drawableW / (float)drawableH;
	displayAspect = aspect;
	glConfig.vidWidth = drawableW;
	glConfig.vidHeight = drawableH;
	glConfig.windowAspect = aspect;
	glConfig.isFullscreen = qtrue;
	glConfig.colorBits = 32;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	ri.Cvar_Set( "r_mode", "-1" );
	ri.Cvar_Set( "r_customwidth", va( "%d", drawableW ) );
	ri.Cvar_Set( "r_customheight", va( "%d", drawableH ) );
	IN_TouchSyncLayout( drawableW, drawableH, width > 0 ? (float)drawableW / (float)width : 1.0f );

	ri.Printf( PRINT_ALL, "Initializing OpenGL ES 1.1 display (SDL2) %dx%d\n", drawableW, drawableH );
	return qtrue;
}

static void GLimp_InitExtensions( void )
{
	qglActiveTextureARB = glActiveTexture;
	qglClientActiveTextureARB = glClientActiveTexture;
	qglMultiTexCoord2fARB = qglesMultiTexCoord2f;
	qglLockArraysEXT = qglesLockArrays;
	qglUnlockArraysEXT = qglesUnlockArrays;
	glConfig.textureCompression = TC_NONE;
	glConfig.textureEnvAddAvailable = qtrue;
	glConfig.numTextureUnits = 1;
	qglGetIntegerv( GL_MAX_TEXTURE_UNITS, &glConfig.numTextureUnits );
	if( glConfig.numTextureUnits < 1 )
		glConfig.numTextureUnits = 1;
}

void GLimp_Init( void )
{
	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "SDL2-iOS", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );

	ri.Sys_GLimpInit();

	if( !GLimp_CreateWindow() )
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL ES subsystem" );

	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = qfalse;
	Q_strncpyz( glConfig.vendor_string, (char *)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, (char *)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *)qglGetString( GL_EXTENSIONS ), sizeof( glConfig.extensions_string ) );

	GLimp_InitExtensions();
	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );
	ri.IN_Init();
}

void GLimp_EndFrame( void )
{
	SDL_GL_SwapWindow( window );
}
