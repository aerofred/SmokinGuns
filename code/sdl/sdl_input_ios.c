#include <SDL.h>

#include "../client/client.h"
#include "../client/cl_touch.h"
#include "../ios/ios_layer.h"
#include "../ios/ios_gamepad.h"
#include "../sdl/sdl_input_ios_gamepad.h"
#include "../sys/sys_local.h"

static qboolean inputInited = qfalse;

static void IN_IosRegisterCommands( void );

static int IN_TranslateControllerButton( int button )
{
	switch( button )
	{
		case SDL_CONTROLLER_BUTTON_A: return K_SPACE;
		case SDL_CONTROLLER_BUTTON_B: return 'f';
		case SDL_CONTROLLER_BUTTON_X: return 'r';
		case SDL_CONTROLLER_BUTTON_Y: return K_ESCAPE;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return K_MOUSE2;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return K_MOUSE1;
		case SDL_CONTROLLER_BUTTON_DPAD_UP: return K_UPARROW;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return K_DOWNARROW;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return K_LEFTARROW;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return K_RIGHTARROW;
		default: return 0;
	}
}

static void IN_ProcessEvent( SDL_Event *event )
{
	switch( event->type )
	{
		case SDL_FINGERDOWN:
			IN_TouchFinger( event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, qtrue, qfalse );
			break;
		case SDL_FINGERMOTION:
			IN_TouchFinger( event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, qtrue, qtrue );
			break;
		case SDL_FINGERUP:
			IN_TouchFinger( event->tfinger.fingerId, event->tfinger.x, event->tfinger.y, qfalse, qfalse );
			break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if( IN_TouchInUIMode() )
				break;
			if( event->type == SDL_MOUSEMOTION )
				Com_QueueEvent( 0, SE_MOUSE, event->motion.xrel, event->motion.yrel, 0, NULL );
			else
				Com_QueueEvent( 0, SE_KEY, event->button.button == SDL_BUTTON_RIGHT ? K_MOUSE2 : K_MOUSE1,
					event->button.state == SDL_PRESSED, 0, NULL );
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		{
			int key = IN_TranslateControllerButton( event->cbutton.button );
			if( key )
				Com_QueueEvent( 0, SE_KEY, key, event->type == SDL_CONTROLLERBUTTONDOWN, 0, NULL );
			break;
		}
		case SDL_CONTROLLERDEVICEADDED:
			IN_IosRefreshJoystick( qfalse );
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			IN_IosCloseJoystick();
			break;
		case SDL_APP_WILLENTERBACKGROUND:
		case SDL_APP_DIDENTERBACKGROUND:
			IOS_Layer_SetActive( qfalse );
			Cvar_Set( "com_minimized", "1" );
			Cvar_Set( "com_unfocused", "1" );
			Cvar_Set( "s_muted", "1" );
			IOS_Gamepad_PauseForOverlay();
			break;
		case SDL_APP_WILLENTERFOREGROUND:
		case SDL_APP_DIDENTERFOREGROUND:
			IOS_Layer_SetActive( qtrue );
			Cvar_Set( "com_minimized", "0" );
			Cvar_Set( "com_unfocused", "0" );
			Cvar_Set( "s_muted", "0" );
			break;
		case SDL_QUIT:
			Cbuf_AddText( "quit\n" );
			break;
	}
}

void IN_Frame( void )
{
	SDL_Event event;

	while( SDL_PollEvent( &event ) )
		IN_ProcessEvent( &event );

	IN_IosGamepadFrame();
	IN_TouchFrame();
}

void IN_InitKeyLockStates( void )
{
}

void IN_Init( void )
{
	if( inputInited )
		return;

	SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
	SDL_SetHint( SDL_HINT_MOUSE_TOUCH_EVENTS, "0" );
	SDL_InitSubSystem( SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK );
	IN_TouchInit();
	IN_IosGamepadInit();
	IN_IosRegisterCommands();
	inputInited = qtrue;
}

void IN_Shutdown( void )
{
	if( !inputInited )
		return;
	IN_IosGamepadShutdown();
	IN_TouchShutdown();
	SDL_QuitSubSystem( SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK );
	inputInited = qfalse;
}

void IN_Restart( void )
{
	IN_Shutdown();
	IN_Init();
}

static void IOS_GamepadSettings_f( void )
{
	IOS_Gamepad_PresentSettings();
}

static void IN_IosRegisterCommands( void )
{
	Cmd_AddCommand( "gamepad_config", IOS_GamepadSettings_f );
}
