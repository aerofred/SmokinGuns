/*
===========================================================================
iOS overlay/layout hooks.
===========================================================================
*/

#ifndef IOS_LAYER_H
#define IOS_LAYER_H

#include "../qcommon/q_shared.h"

typedef struct iosLayout_s
{
	float width;
	float height;
	float safeLeft;
	float safeTop;
	float safeRight;
	float safeBottom;
	float scale;
} iosLayout_t;

void IOS_Layer_Init( void );
void IOS_Layer_Shutdown( void );
void IOS_Layer_Tick( void );
void IOS_Layer_SyncScreen( int width, int height, float scale );
void IOS_Layer_GetLayout( iosLayout_t *layout );
void IOS_Layer_SetGameOverlayVisible( qboolean visible );
void IOS_Layer_OpenTouchSettings( void );
void IOS_Layer_AttachToWindow( void );

#endif
