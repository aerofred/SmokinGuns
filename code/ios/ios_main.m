/*
===========================================================================
Smokin' Guns iOS application entry (SDL2 UIKit run loop)
===========================================================================
*/

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_main.h>

extern int Sys_AppMain( int argc, char **argv );

int main( int argc, char *argv[] )
{
	return SDL_UIKitRunApp( argc, argv, Sys_AppMain );
}
