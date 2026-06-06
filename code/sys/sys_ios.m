/*
===========================================================================
Smokin' Guns iOS system layer
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

char *Sys_DefaultHomePath( void )
{
	static char homePath[MAX_OSPATH];
	const char *p;

	if( homePath[0] )
		return homePath;

	homePath[0] = '\0';

	p = [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory
		inDomains:NSUserDomainMask] firstObject].path.UTF8String;
	if( p && p[0] )
	{
		Com_sprintf( homePath, sizeof( homePath ), "%s%c", p, PATH_SEP );
		if( com_homepath && com_homepath->string[0] )
			Q_strcat( homePath, sizeof( homePath ), com_homepath->string );
		else
			Q_strcat( homePath, sizeof( homePath ), HOMEPATH_NAME_MACOSX );
	}

	return homePath;
}

char *Sys_DefaultAppPath( void )
{
	static char appPath[MAX_OSPATH];
	NSString *resourcePath;

	if( appPath[0] )
		return appPath;

	resourcePath = [[NSBundle mainBundle] resourcePath];
	if( resourcePath )
		Q_strncpyz( appPath, resourcePath.UTF8String, sizeof( appPath ) );
	else
		appPath[0] = '\0';

	return appPath;
}

void Sys_ShowLANAddresses( void )
{
	struct ifaddrs *ifap, *ifa;
	char buf[64];

	if ( getifaddrs( &ifap ) != 0 )
		return;

	Com_Printf( "=== Adresses LAN (pour host / join) ===\n" );
	for ( ifa = ifap; ifa; ifa = ifa->ifa_next )
	{
		if ( !ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET )
			continue;
		if ( ifa->ifa_flags & IFF_LOOPBACK )
			continue;
		if ( !( ifa->ifa_flags & IFF_UP ) )
			continue;
		if ( !inet_ntop( AF_INET, &( (struct sockaddr_in *)ifa->ifa_addr )->sin_addr, buf, sizeof( buf ) ) )
			continue;
		Com_Printf( "  connect %s:27960\n", buf );
	}
	freeifaddrs( ifap );
}

static int iosViewportX = 0;
static int iosViewportY = 0;
static int iosViewportWidth = 0;
static int iosViewportHeight = 0;
static float iosViewportXScale = 1.0f;
static float iosViewportYScale = 1.0f;
static float iosViewportXBias = 0.0f;
static float iosViewportYBias = 0.0f;
static int iosModeWidth = 0;
static int iosModeHeight = 0;
static int iosVidWidth = 0;
static int iosVidHeight = 0;

void Sys_SetViewportMode( int modeWidth, int modeHeight ) {
	iosModeWidth = modeWidth;
	iosModeHeight = modeHeight;
}

void Sys_UpdateViewport4x3( int vidWidth, int vidHeight ) {
	int availWidth;
	int availHeight;
	int viewWidth;
	int viewHeight;
	int refW;
	int refH;

	if ( vidWidth <= 0 || vidHeight <= 0 ) {
		return;
	}

	availWidth = vidWidth;
	availHeight = vidHeight;

	refW = iosModeWidth;
	refH = iosModeHeight;

	/* r_mode -2 (natif) : occuper tout l'écran */
	if ( refW <= 0 || refH <= 0 ) {
		viewWidth = availWidth;
		viewHeight = availHeight;
		refW = vidWidth;
		refH = vidHeight;
	} else {
		/* Pour les modes fixes, remplir toute la hauteur et garder le ratio. */
		viewHeight = availHeight;
		viewWidth = ( availHeight * refW ) / refH;
	}

	viewWidth &= ~1;
	viewHeight &= ~1;

	iosViewportWidth = viewWidth;
	iosViewportHeight = viewHeight;
	iosViewportX = ( vidWidth - viewWidth ) / 2;
	iosViewportY = ( vidHeight - viewHeight ) / 2;

	iosViewportXScale = (float)viewWidth / (float)refW;
	iosViewportYScale = (float)viewHeight / (float)refH;
	iosViewportXBias = (float)iosViewportX;
	iosViewportYBias = (float)iosViewportY;
	iosVidWidth = vidWidth;
	iosVidHeight = vidHeight;
}

void Sys_GetViewport4x3( int *x, int *y, int *width, int *height ) {
	if ( x ) {
		*x = iosViewportX;
	}
	if ( y ) {
		*y = iosViewportY;
	}
	if ( width ) {
		*width = iosViewportWidth;
	}
	if ( height ) {
		*height = iosViewportHeight;
	}
}

void Sys_GetViewport640Mapping( float *xscale, float *yscale, float *xbias, float *ybias ) {
	if ( xscale ) {
		*xscale = iosViewportXScale;
	}
	if ( yscale ) {
		*yscale = iosViewportYScale;
	}
	if ( xbias ) {
		*xbias = iosViewportXBias;
	}
	if ( ybias ) {
		*ybias = iosViewportYBias;
	}
}

void Sys_GetViewportPoints( int screenW, int screenH, int *x, int *y, int *width, int *height ) {
	float sx;
	float sy;
	int vx;
	int vy;
	int vw;
	int vh;

	Sys_GetViewport4x3( &vx, &vy, &vw, &vh );

	if ( iosVidWidth <= 0 || iosVidHeight <= 0 || screenW <= 0 || screenH <= 0 ) {
		if ( x ) {
			*x = 0;
		}
		if ( y ) {
			*y = 0;
		}
		if ( width ) {
			*width = screenW;
		}
		if ( height ) {
			*height = screenH;
		}
		return;
	}

	sx = (float)screenW / (float)iosVidWidth;
	sy = (float)screenH / (float)iosVidHeight;

	if ( x ) {
		*x = (int)( vx * sx + 0.5f );
	}
	if ( y ) {
		*y = (int)( vy * sy + 0.5f );
	}
	if ( width ) {
		*width = (int)( vw * sx + 0.5f );
	}
	if ( height ) {
		*height = (int)( vh * sy + 0.5f );
	}
}
