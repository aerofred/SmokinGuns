#include "ios_layer.h"

#import <UIKit/UIKit.h>

static iosLayout_t iosLayout = { 0, 0, 0, 0, 0, 0, 1 };
static qboolean iosOverlayVisible = qtrue;

static void IOS_UpdateSafeArea( void )
{
	UIWindow *window = UIApplication.sharedApplication.keyWindow;
	if( window && @available(iOS 11.0, *) )
	{
		UIEdgeInsets insets = window.safeAreaInsets;
		iosLayout.safeLeft = (float)insets.left;
		iosLayout.safeTop = (float)insets.top;
		iosLayout.safeRight = (float)insets.right;
		iosLayout.safeBottom = (float)insets.bottom;
	}
}

void IOS_Layer_Init( void )
{
	UIScreen *screen = UIScreen.mainScreen;
	CGSize size = screen.bounds.size;
	iosLayout.width = (float)size.width;
	iosLayout.height = (float)size.height;
	iosLayout.scale = (float)screen.scale;
	IOS_UpdateSafeArea();
}

void IOS_Layer_Shutdown( void )
{
}

void IOS_Layer_Tick( void )
{
	IOS_UpdateSafeArea();
}

void IOS_Layer_SyncScreen( int width, int height, float scale )
{
	iosLayout.width = width > 0 ? (float)width / ( scale > 0 ? scale : 1.0f ) : iosLayout.width;
	iosLayout.height = height > 0 ? (float)height / ( scale > 0 ? scale : 1.0f ) : iosLayout.height;
	iosLayout.scale = scale > 0 ? scale : iosLayout.scale;
	IOS_UpdateSafeArea();
}

void IOS_Layer_GetLayout( iosLayout_t *layout )
{
	if( layout )
		*layout = iosLayout;
}

void IOS_Layer_SetGameOverlayVisible( qboolean visible )
{
	iosOverlayVisible = visible;
	(void)iosOverlayVisible;
}

void IOS_Layer_OpenTouchSettings( void )
{
	// The first playable port keeps settings as cvars; this hook is for a later UIKit editor.
}

void IOS_Layer_AttachToWindow( void )
{
	IOS_UpdateSafeArea();
}
