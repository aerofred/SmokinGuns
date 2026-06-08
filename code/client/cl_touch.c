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
static qboolean touchJumpDown;
static qboolean touchUseDown;
static qboolean touchUIMouseDown;
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
static cvar_t *in_touchWeaponsX;
static cvar_t *in_touchWeaponsY;
static cvar_t *in_touchMenuX;
static cvar_t *in_touchMenuY;
static cvar_t *in_touchDebug;
static cvar_t *in_touchOpacity;

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

static touchZone_t Touch_Classify( float x, float y )
{
	float stick = in_touchStickSize->value * touchWidth;
	float button = in_touchBtnSize->value * touchWidth;

	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchMoveX ), Touch_EdgeY( in_touchMoveY ), stick ) )
		return TOUCH_ZONE_MOVE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchFireX ), Touch_EdgeY( in_touchFireY ), button ) )
		return TOUCH_ZONE_FIRE;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchWeaponsX ), Touch_EdgeY( in_touchWeaponsY ), button ) )
		return TOUCH_ZONE_WEAPONS;
	if( Touch_PointNear( x, y, Touch_EdgeX( in_touchMenuX ), Touch_EdgeY( in_touchMenuY ), button ) )
		return TOUCH_ZONE_MENU;
	if( x > touchWidth * 0.45f )
		return TOUCH_ZONE_LOOK;
	return TOUCH_ZONE_NONE;
}

static void Touch_SetHeld( qboolean *state, int key, qboolean down )
{
	if( *state == down )
		return;
	*state = down;
	Touch_Key( key, down );
}

static void Touch_StopZone( touchZone_t zone )
{
	if( zone == TOUCH_ZONE_FIRE )
		Touch_SetHeld( &touchFireDown, K_MOUSE1, qfalse );
	else if( zone == TOUCH_ZONE_JUMP )
		Touch_SetHeld( &touchJumpDown, K_SPACE, qfalse );
	else if( zone == TOUCH_ZONE_USE )
		Touch_SetHeld( &touchUseDown, 'f', qfalse );
}

static void Touch_CommandRadial( float x, float y )
{
	float cx = Touch_EdgeX( in_touchWeaponsX );
	float cy = Touch_EdgeY( in_touchWeaponsY );
	float dx = x - cx;
	float dy = y - cy;

	if( fabsf( dx ) > fabsf( dy ) )
	{
		if( dx > 0 )
			Cbuf_AddText( "weapprev\n" );
		else
			Cbuf_AddText( "weapnext\n" );
	}
	else
	{
		if( dy < 0 )
			Cbuf_AddText( "weapon 1\n" );
		else
			Cbuf_AddText( "weapon 2\n" );
	}
}

void IN_TouchApplyDefaults( void )
{
	Cvar_Set( "r_mode", "-2" );
	Cvar_Set( "r_fullscreen", "1" );
	Cvar_Set( "in_touchUISensitivity", "1.1" );
}

void IN_TouchInit( void )
{
	in_touch = Cvar_Get( "in_touch", "1", CVAR_ARCHIVE );
	in_touchMoveSensitivity = Cvar_Get( "in_touchMoveSensitivity", "1.0", CVAR_ARCHIVE );
	in_touchSensitivity = Cvar_Get( "in_touchSensitivity", "2.2", CVAR_ARCHIVE );
	in_touchUISensitivity = Cvar_Get( "in_touchUISensitivity", "1.6", CVAR_ARCHIVE );
	in_touchDeadzone = Cvar_Get( "in_touchDeadzone", "0.035", CVAR_ARCHIVE );
	in_touchStickSize = Cvar_Get( "in_touchStickSize", "0.16", CVAR_ARCHIVE );
	in_touchBtnSize = Cvar_Get( "in_touchBtnSize", "0.09", CVAR_ARCHIVE );
	in_touchMoveX = Cvar_Get( "in_touchMoveX", "0.13", CVAR_ARCHIVE );
	in_touchMoveY = Cvar_Get( "in_touchMoveY", "-0.18", CVAR_ARCHIVE );
	in_touchLookX = Cvar_Get( "in_touchLookX", "-0.32", CVAR_ARCHIVE );
	in_touchLookY = Cvar_Get( "in_touchLookY", "-0.18", CVAR_ARCHIVE );
	in_touchFireX = Cvar_Get( "in_touchFireX", "-0.11", CVAR_ARCHIVE );
	in_touchFireY = Cvar_Get( "in_touchFireY", "-0.17", CVAR_ARCHIVE );
	in_touchWeaponsX = Cvar_Get( "in_touchWeaponsX", "-0.10", CVAR_ARCHIVE );
	in_touchWeaponsY = Cvar_Get( "in_touchWeaponsY", "0.20", CVAR_ARCHIVE );
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
	Touch_SetHeld( &touchFireDown, K_MOUSE1, qfalse );
	Touch_SetHeld( &touchJumpDown, K_SPACE, qfalse );
	Touch_SetHeld( &touchUseDown, 'f', qfalse );
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
			Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
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
		IN_TouchUIMouse( x, y, down, motion );
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
				Touch_SetHeld( &touchFireDown, K_MOUSE1, qtrue );
			else if( finger->zone == TOUCH_ZONE_MENU )
				Cbuf_AddText( "togglemenu\n" );
			else if( finger->zone == TOUCH_ZONE_WEAPONS )
				touchMode = TOUCH_MODE_RADIAL;
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
			Touch_CommandRadial( x, y );

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
	float mx, my, fx, fy, wx, wy, ux, uy;

	if( !in_touch || !in_touch->integer )
	{
		IOS_Layer_UpdateTouchControls( qfalse, 0.0f, touchMode,
			0, 0, 0, 0, 0, 0, qfalse, 0, 0, 0, 0, 0, 0 );
		return;
	}
	if( IN_TouchInUIMode() || clc.state != CA_ACTIVE )
	{
		IOS_Layer_UpdateTouchControls( qfalse, 0.0f, touchMode,
			0, 0, 0, 0, 0, 0, qfalse, 0, 0, 0, 0, 0, 0 );
		return;
	}

	stick = in_touchStickSize->value * touchWidth;
	button = in_touchBtnSize->value * touchWidth;
	if( stick < 72.0f )
		stick = 72.0f;
	if( button < 48.0f )
		button = 48.0f;

	mx = Touch_EdgeX( in_touchMoveX );
	my = Touch_EdgeY( in_touchMoveY );
	fx = Touch_EdgeX( in_touchFireX );
	fy = Touch_EdgeY( in_touchFireY );
	wx = Touch_EdgeX( in_touchWeaponsX );
	wy = Touch_EdgeY( in_touchWeaponsY );
	ux = Touch_EdgeX( in_touchMenuX );
	uy = Touch_EdgeY( in_touchMenuY );

	IOS_Layer_UpdateTouchControls( qtrue, Touch_Opacity(), touchMode,
		mx, my, stick,
		fx, fy, button, touchFireDown,
		wx, wy, button,
		ux, uy, button * 0.78f );
}
