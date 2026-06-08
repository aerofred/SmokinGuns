#include "client.h"
#include "cl_touch.h"
#include "../ios/ios_layer.h"

#define TOUCH_MAX_FINGERS 10
#define TOUCH_KEY_BASE 240

typedef struct
{
	qboolean active;
	long long id;
	float x;
	float y;
	float startX;
	float startY;
	touchZone_t zone;
} touchFinger_t;

static touchFinger_t fingers[TOUCH_MAX_FINGERS];
static touchMode_t touchMode = TOUCH_MODE_COMBAT;
static int touchWidth = 1;
static int touchHeight = 1;
static float touchScale = 1.0f;
static qboolean touchFireDown;
static qboolean touchAltFireDown;
static qboolean touchJumpDown;
static qboolean touchCrouchDown;
static qboolean touchUseDown;
static qboolean touchReloadDown;
static qboolean touchOpenDown;
static qboolean touchBuyDown;
static qboolean touchUIMouseDown;
static int touchUIPendingLeftUpFrames;
static long long touchUIPointerFinger = -1;
static long long touchUIRightFinger = -1;
static qboolean touchUIRightDown;
static float touchUILastPx = -1.0f;
static float touchUILastPy = -1.0f;
static float touchUICursorX = 320.0f;
static float touchUICursorY = 240.0f;
static qboolean touchUITapCandidate;
static float touchUIStartPx = -1.0f;
static float touchUIStartPy = -1.0f;

static cvar_t *in_touch;
static cvar_t *in_touchMoveSensitivity;
static cvar_t *in_touchSensitivity;
static cvar_t *in_touchUISensitivity;
static cvar_t *in_touchDeadzone;
static cvar_t *in_touchStickSize;
static cvar_t *in_touchBtnSize;
static cvar_t *in_touchMoveX;
static cvar_t *in_touchMoveY;
static cvar_t *in_touchLookX;
static cvar_t *in_touchLookY;
static cvar_t *in_touchFireX;
static cvar_t *in_touchFireY;
static cvar_t *in_touchAltFireX;
static cvar_t *in_touchAltFireY;
static cvar_t *in_touchJumpX;
static cvar_t *in_touchJumpY;
static cvar_t *in_touchCrouchX;
static cvar_t *in_touchCrouchY;
static cvar_t *in_touchUseX;
static cvar_t *in_touchUseY;
static cvar_t *in_touchReloadX;
static cvar_t *in_touchReloadY;
static cvar_t *in_touchOpenX;
static cvar_t *in_touchOpenY;
static cvar_t *in_touchBuyX;
static cvar_t *in_touchBuyY;
static cvar_t *in_touchWeaponsX;
static cvar_t *in_touchWeaponsY;
static cvar_t *in_touchMenuX;
static cvar_t *in_touchMenuY;
static cvar_t *in_touchDebug;
static cvar_t *in_touchOpacity;

static void IN_TouchUIReset( void );

static void Touch_GetSafePixels( float *left, float *top, float *right, float *bottom )
{
	iosLayout_t layout;
	float scale;

	IOS_Layer_GetLayout( &layout );
	scale = layout.scale > 0.0f ? layout.scale : touchScale;
	if( scale <= 0.0f )
		scale = 1.0f;

	if( left )
		*left = layout.safeLeft * scale;
	if( top )
		*top = layout.safeTop * scale;
	if( right )
		*right = layout.safeRight * scale;
	if( bottom )
		*bottom = layout.safeBottom * scale;
}

static void Touch_Key( int key, qboolean down )
{
	Com_QueueEvent( 0, SE_KEY, key, down, 0, NULL );
}

static touchFinger_t *Touch_FindFinger( long long id )
{
	int i;

	for( i = 0; i < TOUCH_MAX_FINGERS; i++ )
	{
		if( fingers[i].active && fingers[i].id == id )
			return &fingers[i];
	}

	return NULL;
}

static touchFinger_t *Touch_AllocFinger( long long id )
{
	int i;

	for( i = 0; i < TOUCH_MAX_FINGERS; i++ )
	{
		if( !fingers[i].active )
		{
			Com_Memset( &fingers[i], 0, sizeof( fingers[i] ) );
			fingers[i].active = qtrue;
			fingers[i].id = id;
			return &fingers[i];
		}
	}

	return NULL;
}

static float Touch_EdgeX( cvar_t *cv )
{
	float v = cv->value;
	float safeLeft, safeRight;

	Touch_GetSafePixels( &safeLeft, NULL, &safeRight, NULL );
	if( v < 0.0f )
		return touchWidth - safeRight + v * touchWidth;
	return safeLeft + v * touchWidth;
}

static float Touch_EdgeY( cvar_t *cv )
{
	float v = cv->value;
	float safeTop, safeBottom;

	Touch_GetSafePixels( NULL, &safeTop, NULL, &safeBottom );
	if( v < 0.0f )
		return touchHeight - safeBottom + v * touchHeight;
	return safeTop + v * touchHeight;
}

static qboolean Touch_PointNear( float x, float y, float cx, float cy, float radius )
{
	float dx = x - cx;
	float dy = y - cy;
	return dx * dx + dy * dy <= radius * radius;
}

static float Touch_ControlBase( void )
{
	return touchWidth < touchHeight ? (float)touchWidth : (float)touchHeight;
}

static touchZone_t Touch_Classify( float x, float y )
{
	float base = Touch_ControlBase();
	float stick = in_touchStickSize->value * base;
	float button = in_touchBtnSize->value * base;

	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchMoveX ), Touch_EdgeY( in_touchMoveY ), stick ) )
		return TOUCH_ZONE_MOVE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchFireX ), Touch_EdgeY( in_touchFireY ), button ) )
		return TOUCH_ZONE_FIRE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchAltFireX ), Touch_EdgeY( in_touchAltFireY ), button ) )
		return TOUCH_ZONE_ALT_FIRE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchJumpX ), Touch_EdgeY( in_touchJumpY ), button ) )
		return TOUCH_ZONE_JUMP;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchCrouchX ), Touch_EdgeY( in_touchCrouchY ), button ) )
		return TOUCH_ZONE_CROUCH;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchUseX ), Touch_EdgeY( in_touchUseY ), button ) )
		return TOUCH_ZONE_USE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchReloadX ), Touch_EdgeY( in_touchReloadY ), button ) )
		return TOUCH_ZONE_RELOAD;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchOpenX ), Touch_EdgeY( in_touchOpenY ), button ) )
		return TOUCH_ZONE_OPEN;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchBuyX ), Touch_EdgeY( in_touchBuyY ), button ) )
		return TOUCH_ZONE_BUY;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchWeaponsX ), Touch_EdgeY( in_touchWeaponsY ), button ) )
		return TOUCH_ZONE_WEAPONS;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchMenuX ), Touch_EdgeY( in_touchMenuY ), button ) )
		return TOUCH_ZONE_MENU;
	if( x > touchWidth * 0.45f )
		return TOUCH_ZONE_LOOK;
	return TOUCH_ZONE_NONE;
}

static void Touch_SetHeldCommand( qboolean *state, const char *downCommand, const char *upCommand, qboolean down )
{
	if( *state == down )
		return;
	*state = down;
	if( down ? ( downCommand && downCommand[0] ) : ( upCommand && upCommand[0] ) )
	{
		Cbuf_AddText( down ? downCommand : upCommand );
		Cbuf_AddText( "\n" );
	}
}

static void Touch_StopZone( touchZone_t zone )
{
	if( zone == TOUCH_ZONE_FIRE )
		Touch_SetHeldCommand( &touchFireDown, "+attack", "-attack", qfalse );
	else if( zone == TOUCH_ZONE_ALT_FIRE )
		Touch_SetHeldCommand( &touchAltFireDown, "+button6", "-button6", qfalse );
	else if( zone == TOUCH_ZONE_JUMP )
		Touch_SetHeldCommand( &touchJumpDown, "+moveup", "-moveup", qfalse );
	else if( zone == TOUCH_ZONE_CROUCH )
		Touch_SetHeldCommand( &touchCrouchDown, "+button3", "-button3", qfalse );
	else if( zone == TOUCH_ZONE_USE )
		Touch_SetHeldCommand( &touchUseDown, "+button2", "-button2", qfalse );
	else if( zone == TOUCH_ZONE_RELOAD )
		Touch_SetHeldCommand( &touchReloadDown, "+button5", "-button5", qfalse );
	else if( zone == TOUCH_ZONE_OPEN )
		Touch_SetHeldCommand( &touchOpenDown, "+button7", "-button7", qfalse );
	else if( zone == TOUCH_ZONE_BUY )
		Touch_SetHeldCommand( &touchBuyDown, "wq_buy", "", qfalse );
}

static void Touch_TapCommand( const char *command )
{
	Cbuf_AddText( command );
	Cbuf_AddText( "\n" );
}

void IN_TouchApplyDefaults( void )
{
	Cvar_Set( "r_mode", "-2" );
	Cvar_Set( "r_fullscreen", "1" );
	Cvar_Set( "in_touchUISensitivity", "1.1" );
	Cvar_Set( "in_touchStickSize", "0.14" );
	Cvar_Set( "in_touchBtnSize", "0.075" );
	Cvar_Set( "in_touchFireX", "-0.11" );
	Cvar_Set( "in_touchFireY", "-0.17" );
	Cvar_Set( "in_touchAltFireX", "-0.23" );
	Cvar_Set( "in_touchAltFireY", "-0.30" );
	Cvar_Set( "in_touchJumpX", "-0.24" );
	Cvar_Set( "in_touchJumpY", "-0.11" );
	Cvar_Set( "in_touchCrouchX", "-0.36" );
	Cvar_Set( "in_touchCrouchY", "-0.11" );
	Cvar_Set( "in_touchUseX", "-0.19" );
	Cvar_Set( "in_touchUseY", "0.35" );
	Cvar_Set( "in_touchReloadX", "-0.08" );
	Cvar_Set( "in_touchReloadY", "0.33" );
	Cvar_Set( "in_touchOpenX", "-0.30" );
	Cvar_Set( "in_touchOpenY", "0.33" );
	Cvar_Set( "in_touchBuyX", "-0.19" );
	Cvar_Set( "in_touchBuyY", "0.47" );
	Cvar_Set( "in_touchWeaponsX", "-0.08" );
	Cvar_Set( "in_touchWeaponsY", "0.17" );
}

void IN_TouchInit( void )
{
	in_touch = Cvar_Get( "in_touch", "1", CVAR_ARCHIVE );
	in_touchMoveSensitivity = Cvar_Get( "in_touchMoveSensitivity", "1.0", CVAR_ARCHIVE );
	in_touchSensitivity = Cvar_Get( "in_touchSensitivity", "2.2", CVAR_ARCHIVE );
	in_touchUISensitivity = Cvar_Get( "in_touchUISensitivity", "1.6", CVAR_ARCHIVE );
	in_touchDeadzone = Cvar_Get( "in_touchDeadzone", "0.035", CVAR_ARCHIVE );
	in_touchStickSize = Cvar_Get( "in_touchStickSize", "0.14", CVAR_ARCHIVE );
	in_touchBtnSize = Cvar_Get( "in_touchBtnSize", "0.075", CVAR_ARCHIVE );
	in_touchMoveX = Cvar_Get( "in_touchMoveX", "0.13", CVAR_ARCHIVE );
	in_touchMoveY = Cvar_Get( "in_touchMoveY", "-0.18", CVAR_ARCHIVE );
	in_touchLookX = Cvar_Get( "in_touchLookX", "-0.32", CVAR_ARCHIVE );
	in_touchLookY = Cvar_Get( "in_touchLookY", "-0.18", CVAR_ARCHIVE );
	in_touchFireX = Cvar_Get( "in_touchFireX", "-0.11", CVAR_ARCHIVE );
	in_touchFireY = Cvar_Get( "in_touchFireY", "-0.17", CVAR_ARCHIVE );
	in_touchAltFireX = Cvar_Get( "in_touchAltFireX", "-0.23", CVAR_ARCHIVE );
	in_touchAltFireY = Cvar_Get( "in_touchAltFireY", "-0.30", CVAR_ARCHIVE );
	in_touchJumpX = Cvar_Get( "in_touchJumpX", "-0.24", CVAR_ARCHIVE );
	in_touchJumpY = Cvar_Get( "in_touchJumpY", "-0.11", CVAR_ARCHIVE );
	in_touchCrouchX = Cvar_Get( "in_touchCrouchX", "-0.36", CVAR_ARCHIVE );
	in_touchCrouchY = Cvar_Get( "in_touchCrouchY", "-0.11", CVAR_ARCHIVE );
	in_touchUseX = Cvar_Get( "in_touchUseX", "-0.19", CVAR_ARCHIVE );
	in_touchUseY = Cvar_Get( "in_touchUseY", "0.35", CVAR_ARCHIVE );
	in_touchReloadX = Cvar_Get( "in_touchReloadX", "-0.08", CVAR_ARCHIVE );
	in_touchReloadY = Cvar_Get( "in_touchReloadY", "0.33", CVAR_ARCHIVE );
	in_touchOpenX = Cvar_Get( "in_touchOpenX", "-0.30", CVAR_ARCHIVE );
	in_touchOpenY = Cvar_Get( "in_touchOpenY", "0.33", CVAR_ARCHIVE );
	in_touchBuyX = Cvar_Get( "in_touchBuyX", "-0.19", CVAR_ARCHIVE );
	in_touchBuyY = Cvar_Get( "in_touchBuyY", "0.47", CVAR_ARCHIVE );
	in_touchWeaponsX = Cvar_Get( "in_touchWeaponsX", "-0.08", CVAR_ARCHIVE );
	in_touchWeaponsY = Cvar_Get( "in_touchWeaponsY", "0.17", CVAR_ARCHIVE );
	in_touchMenuX = Cvar_Get( "in_touchMenuX", "0.08", CVAR_ARCHIVE );
	in_touchMenuY = Cvar_Get( "in_touchMenuY", "0.12", CVAR_ARCHIVE );
	in_touchDebug = Cvar_Get( "in_touchDebug", "0", CVAR_ARCHIVE );
	in_touchOpacity = Cvar_Get( "in_touchOpacity", "0.34", CVAR_ARCHIVE );
	IN_TouchApplyDefaults();
}

void IN_TouchShutdown( void )
{
	int i;

	for( i = 0; i < TOUCH_MAX_FINGERS; i++ )
	{
		if( fingers[i].active )
			Touch_StopZone( fingers[i].zone );
	}
	Com_Memset( fingers, 0, sizeof( fingers ) );
	Touch_SetHeldCommand( &touchFireDown, "+attack", "-attack", qfalse );
	Touch_SetHeldCommand( &touchAltFireDown, "+button6", "-button6", qfalse );
	Touch_SetHeldCommand( &touchJumpDown, "+moveup", "-moveup", qfalse );
	Touch_SetHeldCommand( &touchCrouchDown, "+button3", "-button3", qfalse );
	Touch_SetHeldCommand( &touchUseDown, "+button2", "-button2", qfalse );
	Touch_SetHeldCommand( &touchReloadDown, "+button5", "-button5", qfalse );
	Touch_SetHeldCommand( &touchOpenDown, "+button7", "-button7", qfalse );
	Touch_SetHeldCommand( &touchBuyDown, "wq_buy", "", qfalse );
	IN_TouchUIReset();
}

void IN_TouchSyncLayout( int width, int height, float scale )
{
	iosLayout_t layout;
	int screenWidth;
	int screenHeight;

	touchWidth = width > 0 ? width : 1;
	touchHeight = height > 0 ? height : 1;
	touchScale = scale > 0 ? scale : 1.0f;
	IOS_Layer_SyncScreen( width, height, touchScale );
	IOS_Layer_GetLayout( &layout );
	screenWidth = (int)( layout.width * layout.scale );
	screenHeight = (int)( layout.height * layout.scale );
	if( screenWidth > touchWidth )
		touchWidth = screenWidth;
	if( screenHeight > touchHeight )
		touchHeight = screenHeight;
}

qboolean IN_TouchInUIMode( void )
{
	if( Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CGAME ) )
		return qtrue;
	if( clc.state == CA_DISCONNECTED )
		return qtrue;
	return qfalse;
}

static void IN_TouchUIMouse( float x, float y, qboolean down, qboolean move )
{
	float tapDx, tapDy;
	float tapThreshold;
	int dx, dy;

	if( !down )
	{
		if( touchUIMouseDown )
		{
			touchUIMouseDown = qfalse;
			Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
		}
		else if( touchUITapCandidate )
		{
			Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qtrue, 0, NULL );
			touchUIPendingLeftUpFrames = 2;
		}
		touchUITapCandidate = qfalse;
		touchUIStartPx = -1.0f;
		touchUIStartPy = -1.0f;
		touchUILastPx = -1.0f;
		touchUILastPy = -1.0f;
		return;
	}

	if( touchUILastPx < 0.0f )
	{
		touchUIStartPx = x;
		touchUIStartPy = y;
		touchUILastPx = x;
		touchUILastPy = y;
		touchUITapCandidate = qtrue;
		return;
	}
	else if( move )
	{
		tapDx = x - touchUIStartPx;
		tapDy = y - touchUIStartPy;
		tapThreshold = 18.0f * ( touchScale > 0.0f ? touchScale : 1.0f );
		if( tapDx * tapDx + tapDy * tapDy > tapThreshold * tapThreshold )
			touchUITapCandidate = qfalse;
	}

	dx = (int)( ( x - touchUILastPx ) * in_touchUISensitivity->value );
	dy = (int)( ( y - touchUILastPy ) * in_touchUISensitivity->value );
	touchUILastPx = x;
	touchUILastPy = y;
	if( dx == 0 && dy == 0 )
		return;

	touchUICursorX += dx;
	touchUICursorY += dy;
	if( touchUICursorX < 0.0f )
		touchUICursorX = 0.0f;
	else if( touchUICursorX > 640.0f )
		touchUICursorX = 640.0f;
	if( touchUICursorY < 0.0f )
		touchUICursorY = 0.0f;
	else if( touchUICursorY > 480.0f )
		touchUICursorY = 480.0f;

	Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );
}

static void IN_TouchUIRightClick( qboolean down )
{
	if( touchUIRightDown == down )
		return;
	touchUIRightDown = down;
	Com_QueueEvent( 0, SE_KEY, K_MOUSE2, down, 0, NULL );
}

static void IN_TouchUIReset( void )
{
	if( touchUIPendingLeftUpFrames > 0 )
	{
		touchUIPendingLeftUpFrames = 0;
		Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
	}
	if( touchUIMouseDown )
	{
		touchUIMouseDown = qfalse;
		Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
	}
	IN_TouchUIRightClick( qfalse );
	touchUIPointerFinger = -1;
	touchUIRightFinger = -1;
	touchUITapCandidate = qfalse;
	touchUIStartPx = -1.0f;
	touchUIStartPy = -1.0f;
	touchUILastPx = -1.0f;
	touchUILastPy = -1.0f;
}

qboolean IN_TouchConsoleActive( void )
{
	return ( Key_GetCatcher() & KEYCATCH_CONSOLE ) != 0;
}

void IN_TouchToggleConsole( void )
{
	Touch_Key( K_CONSOLE, qtrue );
	Touch_Key( K_CONSOLE, qfalse );
}

void IN_TouchMouse( int x, int y, qboolean down, qboolean motion )
{
	if( motion )
		Com_QueueEvent( 0, SE_MOUSE, x, y, 0, NULL );
	if( down != qfalse )
		Touch_Key( K_MOUSE1, qtrue );
	else if( !motion )
		Touch_Key( K_MOUSE1, qfalse );
}

void IN_TouchFinger( long long fingerId, float nx, float ny, qboolean down, qboolean motion )
{
	touchFinger_t *finger;
	float x = nx * touchWidth;
	float y = ny * touchHeight;

	if( !in_touch || !in_touch->integer )
		return;

	if( IN_TouchInUIMode() )
	{
		if( down )
		{
			if( touchUIPointerFinger < 0 || touchUIPointerFinger == fingerId )
			{
				touchUIPointerFinger = fingerId;
				IN_TouchUIMouse( x, y, qtrue, motion );
			}
			else
			{
				touchUITapCandidate = qfalse;
				if( touchUIRightFinger < 0 || touchUIRightFinger == fingerId )
				{
					touchUIRightFinger = fingerId;
					IN_TouchUIRightClick( qtrue );
				}
			}
		}
		else if( touchUIRightFinger == fingerId )
		{
			IN_TouchUIRightClick( qfalse );
			touchUIRightFinger = -1;
		}
		else if( touchUIPointerFinger == fingerId )
		{
			IN_TouchUIMouse( x, y, qfalse, motion );
			touchUIPointerFinger = -1;
		}
		return;
	}

	if( down )
	{
		finger = Touch_FindFinger( fingerId );
		if( !finger )
		{
			finger = Touch_AllocFinger( fingerId );
			if( !finger )
				return;
			finger->startX = x;
			finger->startY = y;
			finger->zone = Touch_Classify( x, y );

			if( finger->zone == TOUCH_ZONE_FIRE )
				Touch_SetHeldCommand( &touchFireDown, "+attack", "-attack", qtrue );
			else if( finger->zone == TOUCH_ZONE_ALT_FIRE )
				Touch_SetHeldCommand( &touchAltFireDown, "+button6", "-button6", qtrue );
			else if( finger->zone == TOUCH_ZONE_JUMP )
				Touch_SetHeldCommand( &touchJumpDown, "+moveup", "-moveup", qtrue );
			else if( finger->zone == TOUCH_ZONE_CROUCH )
				Touch_SetHeldCommand( &touchCrouchDown, "+button3", "-button3", qtrue );
			else if( finger->zone == TOUCH_ZONE_USE )
				Touch_SetHeldCommand( &touchUseDown, "+button2", "-button2", qtrue );
			else if( finger->zone == TOUCH_ZONE_RELOAD )
				Touch_SetHeldCommand( &touchReloadDown, "+button5", "-button5", qtrue );
			else if( finger->zone == TOUCH_ZONE_OPEN )
				Touch_SetHeldCommand( &touchOpenDown, "+button7", "-button7", qtrue );
			else if( finger->zone == TOUCH_ZONE_BUY )
				Touch_SetHeldCommand( &touchBuyDown, "wq_buy", "", qtrue );
			else if( finger->zone == TOUCH_ZONE_MENU )
				Cbuf_AddText( "togglemenu\n" );
		}

		if( !finger )
			return;

		if( motion && finger->zone == TOUCH_ZONE_LOOK )
		{
			float dx = ( x - finger->x ) * in_touchSensitivity->value;
			float dy = ( y - finger->y ) * in_touchSensitivity->value;
			Com_QueueEvent( 0, SE_MOUSE, (int)dx, (int)dy, 0, NULL );
		}
		else if( motion && finger->zone == TOUCH_ZONE_MOVE )
		{
			float dx = x - finger->startX;
			float dy = y - finger->startY;
			float dead = in_touchDeadzone->value * touchWidth;
			Touch_Key( K_RIGHTARROW, dx > dead );
			Touch_Key( K_LEFTARROW, dx < -dead );
			Touch_Key( K_DOWNARROW, dy > dead );
			Touch_Key( K_UPARROW, dy < -dead );
		}

		finger->x = x;
		finger->y = y;
	}
	else
	{
		finger = Touch_FindFinger( fingerId );
		if( !finger )
			return;

		if( finger->zone == TOUCH_ZONE_WEAPONS )
			Touch_TapCommand( "weapnext" );

		Touch_StopZone( finger->zone );
		if( finger->zone == TOUCH_ZONE_MOVE )
		{
			Touch_Key( K_RIGHTARROW, qfalse );
			Touch_Key( K_LEFTARROW, qfalse );
			Touch_Key( K_DOWNARROW, qfalse );
			Touch_Key( K_UPARROW, qfalse );
		}

		finger->active = qfalse;
		touchMode = TOUCH_MODE_COMBAT;
	}
}

void IN_TouchFrame( void )
{
	if( !in_touch || !in_touch->integer )
		return;
	if( touchUIPendingLeftUpFrames > 0 && --touchUIPendingLeftUpFrames == 0 )
		Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
	if( !IN_TouchInUIMode() && ( touchUIPointerFinger >= 0 || touchUIRightFinger >= 0 || touchUIRightDown ) )
		IN_TouchUIReset();
	IOS_Layer_Tick();
	(void)in_touchMoveSensitivity;
	(void)in_touchLookX;
	(void)in_touchLookY;
	(void)in_touchDebug;
}

static float Touch_To640X( float x )
{
	return x * 640.0f / (float)( touchWidth > 0 ? touchWidth : 1 );
}

static float Touch_To480Y( float y )
{
	return y * 480.0f / (float)( touchHeight > 0 ? touchHeight : 1 );
}

static void Touch_DrawRectPx( float x, float y, float w, float h, const float *color )
{
	re.SetColor( color );
	re.DrawStretchPic( x, y, w, h, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );
}

static void Touch_DrawBorderPx( float cx, float cy, float radius, float thickness, const float *color )
{
	float x = cx - radius;
	float y = cy - radius;
	float size = radius * 2.0f;

	Touch_DrawRectPx( x, y, size, thickness, color );
	Touch_DrawRectPx( x, y + size - thickness, size, thickness, color );
	Touch_DrawRectPx( x, y, thickness, size, color );
	Touch_DrawRectPx( x + size - thickness, y, thickness, size, color );
}

static void Touch_DrawLabelPx( float cx, float cy, const char *label, const float *color )
{
	int x = (int)Touch_To640X( cx ) - (int)strlen( label ) * 4;
	int y = (int)Touch_To480Y( cy ) - 4;

	SCR_DrawSmallStringExt( x, y, label, (float *)color, qtrue, qtrue );
}

static touchFinger_t *Touch_ActiveFingerForZone( touchZone_t zone )
{
	int i;

	for( i = 0; i < TOUCH_MAX_FINGERS; i++ )
	{
		if( fingers[i].active && fingers[i].zone == zone )
			return &fingers[i];
	}

	return NULL;
}

static float Touch_Opacity( void )
{
	float alpha = in_touchOpacity ? in_touchOpacity->value : 0.34f;

	if( alpha < 0.05f )
		alpha = 0.05f;
	if( alpha > 0.85f )
		alpha = 0.85f;
	return alpha;
}

static void Touch_DrawButton( float cx, float cy, float radius, const char *label, qboolean active, const float *rgb )
{
	vec4_t fill;
	vec4_t border;
	float alpha = Touch_Opacity();

	fill[0] = rgb[0];
	fill[1] = rgb[1];
	fill[2] = rgb[2];
	fill[3] = active ? alpha + 0.22f : alpha;
	if( fill[3] > 0.92f )
		fill[3] = 0.92f;

	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = active ? 0.72f : 0.42f;

	Touch_DrawRectPx( cx - radius * 0.58f, cy - radius * 0.58f, radius * 1.16f, radius * 1.16f, fill );
	Touch_DrawBorderPx( cx, cy, radius, radius * 0.10f, border );
	Touch_DrawLabelPx( cx, cy, label, border );
}

void IN_TouchDraw( void )
{
	float stick;
	float button;
	float mx, my, fx, fy, ax, ay, jx, jy, cx, cy, ex, ey, rx, ry, ox, oy, bx, by, wx, wy, ux, uy;

	if( !in_touch || !in_touch->integer )
	{
		IOS_Layer_UpdateTouchControls( qfalse, 0.0f, touchMode,
			0, 0, 0, 0, 0, 0, qfalse,
			0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0,
			qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse,
			0, 0, 0, 0, 0, 0 );
		return;
	}
	if( IN_TouchInUIMode() || clc.state != CA_ACTIVE )
	{
		IOS_Layer_UpdateTouchControls( qfalse, 0.0f, touchMode,
			0, 0, 0, 0, 0, 0, qfalse,
			0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0,
			qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse, 0, 0, 0, qfalse,
			0, 0, 0, 0, 0, 0 );
		return;
	}

	stick = in_touchStickSize->value * Touch_ControlBase();
	button = in_touchBtnSize->value * Touch_ControlBase();
	if( stick < 72.0f )
		stick = 72.0f;
	if( button < 48.0f )
		button = 48.0f;

	mx = Touch_EdgeX( in_touchMoveX );
	my = Touch_EdgeY( in_touchMoveY );
	fx = Touch_EdgeX( in_touchFireX );
	fy = Touch_EdgeY( in_touchFireY );
	ax = Touch_EdgeX( in_touchAltFireX );
	ay = Touch_EdgeY( in_touchAltFireY );
	jx = Touch_EdgeX( in_touchJumpX );
	jy = Touch_EdgeY( in_touchJumpY );
	cx = Touch_EdgeX( in_touchCrouchX );
	cy = Touch_EdgeY( in_touchCrouchY );
	ex = Touch_EdgeX( in_touchUseX );
	ey = Touch_EdgeY( in_touchUseY );
	rx = Touch_EdgeX( in_touchReloadX );
	ry = Touch_EdgeY( in_touchReloadY );
	ox = Touch_EdgeX( in_touchOpenX );
	oy = Touch_EdgeY( in_touchOpenY );
	bx = Touch_EdgeX( in_touchBuyX );
	by = Touch_EdgeY( in_touchBuyY );
	wx = Touch_EdgeX( in_touchWeaponsX );
	wy = Touch_EdgeY( in_touchWeaponsY );
	ux = Touch_EdgeX( in_touchMenuX );
	uy = Touch_EdgeY( in_touchMenuY );

	IOS_Layer_UpdateTouchControls( qtrue, Touch_Opacity(), touchMode,
		mx, my, stick,
		fx, fy, button, touchFireDown,
		ax, ay, button, touchAltFireDown,
		jx, jy, button, touchJumpDown,
		cx, cy, button, touchCrouchDown,
		ex, ey, button, touchUseDown,
		rx, ry, button, touchReloadDown,
		ox, oy, button, touchOpenDown,
		bx, by, button, touchBuyDown,
		wx, wy, button,
		ux, uy, button * 0.78f );
}
