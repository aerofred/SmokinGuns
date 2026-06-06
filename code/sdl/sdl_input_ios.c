/*
===========================================================================
SDL2 input for iOS — souris virtuelle + touch
===========================================================================
*/
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include "../client/client.h"
#include "../client/cl_touch.h"
#include "../ios/ios_layer.h"
#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"

static qboolean mouseActive = qfalse;
static cvar_t *in_mouse = NULL;

static float mouseAbsX = -1.0f;
static float mouseAbsY = -1.0f;

static void IN_ToPixels( float *x, float *y )
{
	int w, h;

	GLimp_GetWindowSize( &w, &h );
	if ( w < 1 )
		w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 640;
	if ( h < 1 )
		h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 480;

	if ( *x >= 0.0f && *x <= 1.0f && *y >= 0.0f && *y <= 1.0f )
	{
		*x *= (float)w;
		*y *= (float)h;
	}
}

static void IN_ScaleMouseDelta( int xrel, int yrel, int *dx, int *dy )
{
	int w, h;

	GLimp_GetWindowSize( &w, &h );
	if ( w < 1 )
		w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 640;
	if ( h < 1 )
		h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 480;

	if ( Key_GetCatcher() & KEYCATCH_UI || clc.state == CA_DISCONNECTED )
	{
		*dx = (int)( xrel * 640.0f / (float)w );
		*dy = (int)( yrel * 480.0f / (float)h );
	}
	else
	{
		*dx = (int)( xrel * 0.65f );
		*dy = (int)( yrel * 0.65f );
	}
}

static void IN_PointerAt( float x, float y, qboolean down, qboolean move )
{
	IN_ToPixels( &x, &y );
	IN_TouchPointer( x, y, down, move );
}

static void IN_PointerFromMouse( SDL_Event *e, qboolean down, qboolean move )
{
	float x, y;
	qboolean isSynth;

	if ( !e )
		return;

	if ( e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP )
	{
		x = (float)e->button.x;
		y = (float)e->button.y;
		isSynth = ( e->button.which == SDL_TOUCH_MOUSEID );
	}
	else
	{
		x = (float)e->motion.x;
		y = (float)e->motion.y;
		isSynth = ( e->motion.which == SDL_TOUCH_MOUSEID );
	}

	if ( isSynth || mouseAbsX < 0.0f ||
		Key_GetCatcher() & KEYCATCH_UI || clc.state == CA_DISCONNECTED )
	{
		IN_PointerAt( x, y, down, move );
		mouseAbsX = x;
		mouseAbsY = y;
		return;
	}

	/* Souris desktop / trackpad */
	if ( move && mouseActive && in_mouse->integer )
	{
		int dx, dy;
		IN_ScaleMouseDelta( e->motion.xrel, e->motion.yrel, &dx, &dy );
		if ( dx != 0 || dy != 0 )
			Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );
	}
}

void IN_Init( void )
{
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );

	/* Doigts bruts pour menus (souris virtuelle) ; overlay UIKit pour le jeu */
	SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
	SDL_SetHint( SDL_HINT_MOUSE_TOUCH_EVENTS, "0" );

	IN_TouchInit();

	if ( !SDL_WasInit( SDL_INIT_GAMECONTROLLER ) )
		SDL_Init( SDL_INIT_GAMECONTROLLER );

	mouseActive = qtrue;
	mouseAbsX = -1.0f;
	mouseAbsY = -1.0f;

	Com_Printf( "iOS input: overlay controls + finger UI mouse\n" );
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

	switch ( keysym->sym )
	{
	case SDLK_ESCAPE: key = K_ESCAPE; break;
	case SDLK_RETURN: key = K_ENTER; break;
	case SDLK_TAB: key = K_TAB; break;
	case SDLK_SPACE: key = K_SPACE; break;
	case SDLK_UP: key = K_UPARROW; break;
	case SDLK_DOWN: key = K_DOWNARROW; break;
	case SDLK_LEFT: key = K_LEFTARROW; break;
	case SDLK_RIGHT: key = K_RIGHTARROW; break;
	case SDLK_BACKSPACE: key = K_BACKSPACE; break;
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

static void IN_ProcessMouseButton( SDL_Event *e )
{
	int b;
	qboolean down;

	if ( !e )
		return;

	down = ( e->type == SDL_MOUSEBUTTONDOWN ) ? qtrue : qfalse;

	if ( e->button.which == SDL_TOUCH_MOUSEID ||
		Key_GetCatcher() & KEYCATCH_UI ||
		clc.state == CA_DISCONNECTED )
	{
		IN_PointerFromMouse( e, down, qfalse );
		return;
	}

	switch ( e->button.button )
	{
	case SDL_BUTTON_LEFT:   b = K_MOUSE1; break;
	case SDL_BUTTON_RIGHT:  b = K_MOUSE2; break;
	case SDL_BUTTON_MIDDLE: b = K_MOUSE3; break;
	default:
		return;
	}

	Com_QueueEvent( 0, SE_KEY, b, down, 0, NULL );
}

void IN_Frame( void )
{
	SDL_Event e;
	int w, h;

	IN_TouchFrame();
	IOS_Layer_Tick();

	if ( !( Key_GetCatcher() & ( KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) )
		SDL_StopTextInput();

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
		case SDL_TEXTINPUT:
		{
			char *c = e.text.text;

			while ( c && *c )
			{
				unsigned char ch = (unsigned char)*c++;

				if ( ch >= 32 && ch < 127 )
					Com_QueueEvent( 0, SE_CHAR, (int)ch, 0, 0, NULL );
			}
			break;
		}
		case SDL_MOUSEMOTION:
			if ( e.motion.which == SDL_TOUCH_MOUSEID ||
				Key_GetCatcher() & KEYCATCH_UI ||
				clc.state == CA_DISCONNECTED )
			{
				IN_PointerFromMouse( &e, qtrue, qtrue );
			}
			else if ( mouseActive && in_mouse->integer )
			{
				int dx, dy;
				IN_ScaleMouseDelta( e.motion.xrel, e.motion.yrel, &dx, &dy );
				if ( dx != 0 || dy != 0 )
					Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			IN_ProcessMouseButton( &e );
			break;
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			GLimp_GetWindowSize( &w, &h );
			if ( w < 1 )
				w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 640;
			if ( h < 1 )
				h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 480;
			if ( e.type == SDL_FINGERDOWN )
				IN_TouchFinger( (int)e.tfinger.fingerId, e.tfinger.x * w, e.tfinger.y * h, qtrue, qfalse );
			else if ( e.type == SDL_FINGERUP )
				IN_TouchFinger( (int)e.tfinger.fingerId, e.tfinger.x * w, e.tfinger.y * h, qfalse, qfalse );
			else
				IN_TouchFinger( (int)e.tfinger.fingerId, e.tfinger.x * w, e.tfinger.y * h, qtrue, qtrue );
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
