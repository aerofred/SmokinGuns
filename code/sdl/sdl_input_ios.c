/*
===========================================================================
SDL2 input for iOS (keyboard, game controller, touch)
===========================================================================
*/
#include <SDL.h>
#include <stdlib.h>

#include "../client/client.h"
#include "../client/cl_touch.h"
#include "../sys/sys_local.h"

static qboolean mouseActive = qfalse;
static cvar_t *in_mouse = NULL;
static cvar_t *in_nograb = NULL;

void IN_Init( void )
{
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "1", CVAR_ARCHIVE );

	IN_TouchInit();

	if ( !SDL_WasInit( SDL_INIT_GAMECONTROLLER ) )
		SDL_Init( SDL_INIT_GAMECONTROLLER );

	Com_Printf( "iOS input initialized (SDL2 + touch)\n" );
}

void IN_Shutdown( void )
{
	IN_TouchShutdown();
}

void IN_Restart( void )
{
	IN_Shutdown();
	IN_Init();
}

static void IN_ProcessKey( SDL_Keysym *keysym, qboolean down )
{
	int key = 0;

	if ( !keysym )
		return;

	/* Minimal SDL2 -> Q3 key map */
	switch ( keysym->sym )
	{
	case SDLK_ESCAPE: key = K_ESCAPE; break;
	case SDLK_RETURN: key = K_ENTER; break;
	case SDLK_TAB: key = K_TAB; break;
	case SDLK_SPACE: key = K_SPACE; break;
	case SDLK_LSHIFT: case SDLK_RSHIFT: key = K_SHIFT; break;
	case SDLK_LCTRL: case SDLK_RCTRL: key = K_CTRL; break;
	case SDLK_LALT: case SDLK_RALT: key = K_ALT; break;
	case SDLK_UP: key = K_UPARROW; break;
	case SDLK_DOWN: key = K_DOWNARROW; break;
	case SDLK_LEFT: key = K_LEFTARROW; break;
	case SDLK_RIGHT: key = K_RIGHTARROW; break;
	case SDLK_F1: key = K_F1; break;
	case SDLK_F2: key = K_F2; break;
	case SDLK_F3: key = K_F3; break;
	case SDLK_F4: key = K_F4; break;
	case SDLK_F5: key = K_F5; break;
	default:
		if ( keysym->sym >= SDLK_a && keysym->sym <= SDLK_z )
			key = 'a' + ( keysym->sym - SDLK_a );
		else if ( keysym->sym >= SDLK_0 && keysym->sym <= SDLK_9 )
			key = '0' + ( keysym->sym - SDLK_0 );
		break;
	}

	if ( key )
		Com_QueueEvent( 0, SE_KEY, key, down, 0, NULL );
}

void IN_Frame( void )
{
	SDL_Event e;

	IN_TouchFrame();

	while ( SDL_PollEvent( &e ) )
	{
		switch ( e.type )
		{
		case SDL_QUIT:
			Cbuf_AddText( "quit\n" );
			break;
		case SDL_KEYDOWN:
			IN_ProcessKey( &e.key.keysym, qtrue );
			break;
		case SDL_KEYUP:
			IN_ProcessKey( &e.key.keysym, qfalse );
			break;
		case SDL_MOUSEMOTION:
			if ( mouseActive && in_mouse->integer )
				Com_QueueEvent( 0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			/* optional */
			break;
		case SDL_FINGERDOWN:
			IN_TouchEvent( e.tfinger.x * cls.glconfig.vidWidth,
				e.tfinger.y * cls.glconfig.vidHeight,
				e.tfinger.fingerId, qtrue );
			break;
		case SDL_FINGERUP:
			IN_TouchEvent( e.tfinger.x * cls.glconfig.vidWidth,
				e.tfinger.y * cls.glconfig.vidHeight,
				e.tfinger.fingerId, qfalse );
			break;
		case SDL_FINGERMOTION:
			IN_TouchEvent( e.tfinger.x * cls.glconfig.vidWidth,
				e.tfinger.y * cls.glconfig.vidHeight,
				e.tfinger.fingerId, qtrue );
			break;
		case SDL_CONTROLLERAXISMOTION:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			break;
		default:
			break;
		}
	}
}

void IN_Activate( qboolean active )
{
	mouseActive = active;
}

void IN_SetMainCursor( void ) {}
