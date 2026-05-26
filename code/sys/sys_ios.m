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
