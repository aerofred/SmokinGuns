#include "ios_loading.h"

#import <Foundation/Foundation.h>

void IOS_Loading_Begin( const char *message )
{
	(void)message;
}

void IOS_Loading_SetProgress( float progress, const char *message )
{
	(void)progress;
	(void)message;
}

void IOS_Loading_PumpUI( void )
{
	@autoreleasepool
	{
		[[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate date]];
	}
}

void IOS_Loading_End( void )
{
}
