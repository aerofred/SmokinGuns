/*
===========================================================================
SDL2 + OpenGL ES 2 video driver for iOS
===========================================================================
*/
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"

#ifdef USE_GLES
#include "../renderercommon/qgl_gles.h"
#endif

static SDL_Window *sdlWindow = NULL;
static SDL_GLContext glContext = NULL;

cvar_t *r_allowSoftwareGL;
cvar_t *r_allowResize;
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

typedef enum {
	RSERR_OK,
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	RSERR_UNKNOWN
} rserr_t;

static rserr_t GLimp_SetMode( int mode, qboolean fullscreen, qboolean noborder )
{
	int width, height;
	Uint32 flags;
	const char *glstring;

	(void)noborder;
	ri.Printf( PRINT_ALL, "Initializing OpenGL ES display (SDL2)\n" );

	if ( mode == -2 && sdlWindow )
	{
		SDL_GL_GetDrawableSize( sdlWindow, &width, &height );
	}
	else if ( !R_GetModeInfo( &width, &height, &displayAspect, mode ) )
	{
		width = 1024;
		height = 768;
		displayAspect = 4.0f / 3.0f;
	}

	glConfig.vidWidth = width;
	glConfig.vidHeight = height;
	glConfig.windowAspect = (float)width / (float)height;
	glConfig.isFullscreen = fullscreen ? qtrue : qfalse;

	flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if ( fullscreen )
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if ( sdlWindow )
	{
		SDL_SetWindowSize( sdlWindow, width, height );
	}
	else
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

		sdlWindow = SDL_CreateWindow( CLIENT_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, width, height, flags );
		if ( !sdlWindow )
		{
			ri.Printf( PRINT_ALL, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
			return RSERR_INVALID_MODE;
		}

		glContext = SDL_GL_CreateContext( sdlWindow );
		if ( !glContext )
		{
			ri.Printf( PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
			return RSERR_INVALID_MODE;
		}

		SDL_GL_SetSwapInterval( 1 );

		SDL_GL_GetDrawableSize( sdlWindow, &width, &height );
		glConfig.vidWidth = width;
		glConfig.vidHeight = height;
		glConfig.windowAspect = (float)width / (float)height;
	}

	SDL_GL_MakeCurrent( sdlWindow, glContext );

	glConfig.colorBits = 24;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	glstring = (const char *)qglGetString( GL_RENDERER );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring ? glstring : "unknown" );

	return RSERR_OK;
}

static void GLimp_InitExtensions( void )
{
#ifdef USE_GLES
	GLES_InitImmediate();
#endif

	glConfig.textureCompression = TC_NONE;
	glConfig.textureEnvAddAvailable = qtrue;
	glConfig.numTextureUnits = 2;

	Q_strncpyz( glConfig.extensions_string,
		"GL_ARB_multitexture GL_EXT_texture_env_add GL_EXT_texture_filter_anisotropic",
		sizeof( glConfig.extensions_string ) );
}

void GLimp_Init( void )
{
	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = ri.Cvar_Get( "r_sdlDriver", "ios", CVAR_ROM );
	r_allowResize = ri.Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = ri.Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) < 0 )
			ri.Error( ERR_FATAL, "SDL_Init failed: %s", SDL_GetError() );
		ri.Cvar_Set( "r_sdlDriver", "SDL2" );
	}

	if ( GLimp_SetMode( r_mode->integer, r_fullscreen->integer, r_noborder->integer ) != RSERR_OK )
		ri.Error( ERR_FATAL, "GLimp_Init: could not set video mode" );

	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = qfalse;

	Q_strncpyz( glConfig.vendor_string, (char *)qglGetString( GL_VENDOR ),
		sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *)qglGetString( GL_RENDERER ),
		sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, (char *)qglGetString( GL_VERSION ),
		sizeof( glConfig.version_string ) );

	GLimp_InitExtensions();
	ri.IN_Init();
}

void GLimp_Shutdown( void )
{
#ifdef USE_GLES
	GLES_ShutdownImmediate();
#endif
	if ( glContext )
	{
		SDL_GL_DeleteContext( glContext );
		glContext = NULL;
	}
	if ( sdlWindow )
	{
		SDL_DestroyWindow( sdlWindow );
		sdlWindow = NULL;
	}
}

void GLimp_EndFrame( void )
{
	if ( sdlWindow && glContext )
		SDL_GL_SwapWindow( sdlWindow );
}

void GLimp_Minimize( void ) {}

void GLimp_LogComment( char *comment )
{
	(void)comment;
}

void GLimp_FrontEndSleep( void ) {}
int GLimp_SpawnWorkerThread( void (*function)( void ) ) { (void)function; return 0; }
void GLimp_WakeBackEnd( void *data ) { (void)data; }
void GLimp_WakeBackEndPost( void ) {}
void GLimp_DeactivateContext( void )
{
	if ( sdlWindow && glContext )
		SDL_GL_MakeCurrent( sdlWindow, NULL );
}
void GLimp_ActivateContext( void )
{
	if ( sdlWindow && glContext )
		SDL_GL_MakeCurrent( sdlWindow, glContext );
}
void GLimp_DestroyContext( void )
{
	GLimp_Shutdown();
}

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	(void)red;
	(void)green;
	(void)blue;
}
