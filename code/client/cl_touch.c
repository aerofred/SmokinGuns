/*
===========================================================================
Touch iOS : souris virtuelle (menus) + joysticks / boutons en jeu
===========================================================================
*/
#include "client.h"
#include "cl_touch.h"
#include "../renderercommon/tr_common.h"

#define TOUCH_MAX_FINGERS 8

static cvar_t *in_touch;
static cvar_t *in_touchSensitivity;
static cvar_t *in_touchUISensitivity;
static cvar_t *in_touchDeadzone;
static cvar_t *in_touchStickSize;
static cvar_t *in_touchBtnSize;
static cvar_t *in_touchMoveX;
static cvar_t *in_touchMoveY;
static cvar_t *in_touchLookX;
static cvar_t *in_touchLookY;
static cvar_t *in_touchRightX;
static cvar_t *in_touchFireY;
static cvar_t *in_touchJumpY;
static cvar_t *in_touchDebug;

static float moveStickX;
static float moveStickY;
static qboolean moveStickActive;

static float lookStickX;
static float lookStickY;
static qboolean lookStickActive;

static qboolean fireHeld;
static qboolean jumpHeld;

static float touchUILastPx = -1.0f;
static float touchUILastPy = -1.0f;
static qboolean touchUIMouseDown = qfalse;
static qboolean touchUIActive = qfalse;

static float IN_TouchClamp01( float v )
{
	if ( v < 0.0f )
		return 0.0f;
	if ( v > 1.0f )
		return 1.0f;
	return v;
}

static float IN_TouchGetDeadzone( void )
{
	float dz;

	dz = in_touchDeadzone ? in_touchDeadzone->value : 0.18f;
	if ( dz < 0.05f )
		dz = 0.05f;
	if ( dz > 0.45f )
		dz = 0.45f;
	return dz;
}

static qboolean IN_TouchUIMode( void )
{
	if ( Key_GetCatcher() & KEYCATCH_UI )
		return qtrue;
	if ( clc.state == CA_DISCONNECTED )
		return qtrue;
	return qfalse;
}

qboolean IN_TouchInUIMode( void )
{
	return IN_TouchUIMode();
}

static void IN_TouchReleaseMove( void )
{
	Cbuf_AddText( "-forward ; -back ; -left ; -right\n" );
}

static void IN_TouchReleaseFire( void )
{
	if ( fireHeld )
	{
		fireHeld = qfalse;
		Cbuf_AddText( "-attack\n" );
	}
}

static void IN_TouchReleaseJump( void )
{
	if ( jumpHeld )
	{
		jumpHeld = qfalse;
		Cbuf_AddText( "-moveup\n" );
	}
}

static void IN_TouchUIMouse( float x, float y, qboolean down, qboolean move )
{
	float sens;
	float scaleX, scaleY;
	float dpx, dpy;
	int dx, dy;
	int w, h;

	GLimp_GetWindowSize( &w, &h );
	if ( w < 1 )
		w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 640;
	if ( h < 1 )
		h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 480;
	scaleX = 640.0f / (float)w;
	scaleY = 480.0f / (float)h;
	sens = in_touchUISensitivity ? in_touchUISensitivity->value : 3.0f;
	if ( sens < 0.25f )
		sens = 0.25f;

	if ( !down )
	{
		if ( touchUIMouseDown )
		{
			touchUIMouseDown = qfalse;
			Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
		}
		touchUILastPx = -1.0f;
		touchUILastPy = -1.0f;
		return;
	}

	if ( touchUILastPx < 0.0f )
	{
		touchUILastPx = x;
		touchUILastPy = y;
		if ( !touchUIMouseDown )
		{
			touchUIMouseDown = qtrue;
			Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qtrue, 0, NULL );
		}
		if ( !move )
			return;
	}

	if ( !move )
		return;

	dpx = ( x - touchUILastPx ) * sens;
	dpy = ( y - touchUILastPy ) * sens;
	touchUILastPx = x;
	touchUILastPy = y;

	dx = (int)( dpx * scaleX );
	dy = (int)( dpy * scaleY );

	if ( dx == 0 && dy == 0 )
		return;

	Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );

	if ( in_touchDebug && in_touchDebug->integer )
		Com_Printf( "touch UI: d=%d,%d\n", dx, dy );
}

static void IN_TouchRegisterCvars( void )
{
	in_touch = Cvar_Get( "in_touch", "1", CVAR_ARCHIVE );
	in_touchSensitivity = Cvar_Get( "in_touchSensitivity", "1.0", CVAR_ARCHIVE );
	in_touchUISensitivity = Cvar_Get( "in_touchUISensitivity", "3.5", CVAR_ARCHIVE );
	in_touchDeadzone = Cvar_Get( "in_touchDeadzone", "0.18", CVAR_ARCHIVE );
	in_touchStickSize = Cvar_Get( "in_touchStickSize", "0.13", CVAR_ARCHIVE );
	in_touchBtnSize = Cvar_Get( "in_touchBtnSize", "0.11", CVAR_ARCHIVE );
	in_touchMoveX = Cvar_Get( "in_touchMoveX", "0.14", CVAR_ARCHIVE );
	in_touchMoveY = Cvar_Get( "in_touchMoveY", "0.78", CVAR_ARCHIVE );
	in_touchLookX = Cvar_Get( "in_touchLookX", "0.86", CVAR_ARCHIVE );
	in_touchLookY = Cvar_Get( "in_touchLookY", "0.34", CVAR_ARCHIVE );
	in_touchRightX = Cvar_Get( "in_touchRightX", "0.90", CVAR_ARCHIVE );
	in_touchFireY = Cvar_Get( "in_touchFireY", "0.78", CVAR_ARCHIVE );
	in_touchJumpY = Cvar_Get( "in_touchJumpY", "0.58", CVAR_ARCHIVE );
	in_touchDebug = Cvar_Get( "in_touchDebug", "0", CVAR_ARCHIVE );
}

void IN_TouchApplyDefaults( void )
{
	Cvar_Set( "in_touch", "1" );
	Cvar_Set( "in_touchSensitivity", "1.0" );
	Cvar_Set( "in_touchUISensitivity", "3.5" );
	Cvar_Set( "in_touchDeadzone", "0.18" );
	Cvar_Set( "in_touchStickSize", "0.13" );
	Cvar_Set( "in_touchBtnSize", "0.11" );
	Cvar_Set( "in_touchMoveX", "0.14" );
	Cvar_Set( "in_touchMoveY", "0.78" );
	Cvar_Set( "in_touchLookX", "0.86" );
	Cvar_Set( "in_touchLookY", "0.34" );
	Cvar_Set( "in_touchRightX", "0.90" );
	Cvar_Set( "in_touchFireY", "0.78" );
	Cvar_Set( "in_touchJumpY", "0.58" );
	Cvar_Set( "r_gamma", "1.35" );
	Cvar_Set( "r_intensity", "1.35" );
	Cvar_Set( "r_overBrightBits", "1" );
}

void IN_TouchReadConfig( touchConfig_t *cfg )
{
	if ( !cfg )
		return;

	IN_TouchRegisterCvars();

	cfg->enabled = ( in_touch && in_touch->integer ) ? qtrue : qfalse;
	cfg->lookSensitivity = in_touchSensitivity ? in_touchSensitivity->value : 1.0f;
	cfg->uiSensitivity = in_touchUISensitivity ? in_touchUISensitivity->value : 3.5f;
	cfg->deadzone = in_touchDeadzone ? in_touchDeadzone->value : 0.18f;
	cfg->stickSize = in_touchStickSize ? in_touchStickSize->value : 0.13f;
	cfg->btnSize = in_touchBtnSize ? in_touchBtnSize->value : 0.11f;
	cfg->moveX = in_touchMoveX ? in_touchMoveX->value : 0.14f;
	cfg->moveY = in_touchMoveY ? in_touchMoveY->value : 0.78f;
	cfg->lookX = in_touchLookX ? in_touchLookX->value : 0.86f;
	cfg->lookY = in_touchLookY ? in_touchLookY->value : 0.34f;
	cfg->rightX = in_touchRightX ? in_touchRightX->value : 0.90f;
	cfg->fireY = in_touchFireY ? in_touchFireY->value : 0.78f;
	cfg->jumpY = in_touchJumpY ? in_touchJumpY->value : 0.58f;
	cfg->gamma = Cvar_VariableValue( "r_gamma" );
	if ( cfg->gamma < 0.5f )
		cfg->gamma = 0.5f;
	cfg->intensity = Cvar_VariableValue( "r_intensity" );
	if ( cfg->intensity < 1.0f )
		cfg->intensity = 1.0f;
	cfg->overBrightBits = Cvar_VariableValue( "r_overBrightBits" );
	if ( cfg->overBrightBits < 0.0f )
		cfg->overBrightBits = 0.0f;
}

void IN_TouchWriteConfig( const touchConfig_t *cfg )
{
	if ( !cfg )
		return;

	IN_TouchRegisterCvars();

	Cvar_SetValue( "in_touch", cfg->enabled ? 1.0f : 0.0f );
	Cvar_SetValue( "in_touchSensitivity", cfg->lookSensitivity );
	Cvar_SetValue( "in_touchUISensitivity", cfg->uiSensitivity );
	Cvar_SetValue( "in_touchDeadzone", cfg->deadzone );
	Cvar_SetValue( "in_touchStickSize", cfg->stickSize );
	Cvar_SetValue( "in_touchBtnSize", cfg->btnSize );
	Cvar_SetValue( "in_touchMoveX", cfg->moveX );
	Cvar_SetValue( "in_touchMoveY", cfg->moveY );
	Cvar_SetValue( "in_touchLookX", cfg->lookX );
	Cvar_SetValue( "in_touchLookY", cfg->lookY );
	Cvar_SetValue( "in_touchRightX", cfg->rightX );
	Cvar_SetValue( "in_touchFireY", cfg->fireY );
	Cvar_SetValue( "in_touchJumpY", cfg->jumpY );

	Cvar_SetValue( "r_gamma", cfg->gamma );
	Cvar_SetValue( "r_intensity", cfg->intensity );
	Cvar_SetValue( "r_overBrightBits", cfg->overBrightBits );
}

void IN_TouchInit( void )
{
	IN_TouchRegisterCvars();

	moveStickX = moveStickY = 0.0f;
	lookStickX = lookStickY = 0.0f;
	moveStickActive = lookStickActive = qfalse;
	fireHeld = jumpHeld = qfalse;
	touchUILastPx = -1.0f;
	touchUILastPy = -1.0f;
	touchUIMouseDown = qfalse;
	touchUIActive = qfalse;
}

void IN_TouchShutdown( void )
{
	if ( touchUIMouseDown )
		Com_QueueEvent( 0, SE_KEY, K_MOUSE1, qfalse, 0, NULL );
	touchUIMouseDown = qfalse;
	IN_TouchReleaseMove();
	IN_TouchReleaseFire();
	IN_TouchReleaseJump();
	moveStickActive = qfalse;
	lookStickActive = qfalse;
}

void IN_TouchSetMoveStick( float nx, float ny, qboolean active )
{
	if ( IN_TouchUIMode() )
		return;
	moveStickActive = active;
	if ( active )
	{
		moveStickX = nx > 1.0f ? 1.0f : ( nx < -1.0f ? -1.0f : nx );
		moveStickY = ny > 1.0f ? 1.0f : ( ny < -1.0f ? -1.0f : ny );
	}
	else
	{
		moveStickX = moveStickY = 0.0f;
	}
}

void IN_TouchSetLookStick( float nx, float ny, qboolean active )
{
	if ( IN_TouchUIMode() )
		return;
	lookStickActive = active;
	if ( active )
	{
		lookStickX = nx > 1.0f ? 1.0f : ( nx < -1.0f ? -1.0f : nx );
		lookStickY = ny > 1.0f ? 1.0f : ( ny < -1.0f ? -1.0f : ny );
	}
	else
	{
		lookStickX = lookStickY = 0.0f;
	}
}

void IN_TouchSetFire( qboolean down )
{
	if ( IN_TouchUIMode() )
		return;
	if ( down && !fireHeld )
	{
		fireHeld = qtrue;
		Cbuf_AddText( "+attack\n" );
	}
	else if ( !down && fireHeld )
	{
		IN_TouchReleaseFire();
	}
}

void IN_TouchSetJump( qboolean down )
{
	if ( IN_TouchUIMode() )
		return;
	if ( down && !jumpHeld )
	{
		jumpHeld = qtrue;
		Cbuf_AddText( "+moveup\n" );
	}
	else if ( !down && jumpHeld )
	{
		IN_TouchReleaseJump();
	}
}

void IN_TouchPointer( float x, float y, qboolean down, qboolean move )
{
	if ( !in_touch || !in_touch->integer )
		return;

	if ( IN_TouchUIMode() )
	{
		touchUIActive = qtrue;
		IN_TouchUIMouse( x, y, down, move );
		return;
	}

	if ( touchUIActive )
	{
		if ( touchUIMouseDown )
			IN_TouchUIMouse( x, y, qfalse, qfalse );
		touchUIActive = qfalse;
	}
}

void IN_TouchFinger( int fingerId, float x, float y, qboolean down, qboolean move )
{
	(void)fingerId;

	if ( !in_touch || !in_touch->integer )
		return;

	if ( !IN_TouchUIMode() )
		return;

	IN_TouchPointer( x, y, down, move );
}

static void IN_TouchLayoutMetrics( int *w, int *h, float *minDim )
{
	int vw, vh;

	GLimp_GetWindowSize( &vw, &vh );
	if ( vw < 1 )
		vw = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 1024;
	if ( vh < 1 )
		vh = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 768;
	*w = vw;
	*h = vh;
	*minDim = (float)( vw < vh ? vw : vh );
}

void IN_TouchGetOverlay( touchOverlay_t *o )
{
	int w, h;
	float minDim, r, btn, rx;

	if ( !o )
		return;

	memset( o, 0, sizeof( *o ) );
	if ( !in_touch || !in_touch->integer || IN_TouchUIMode() )
		return;

	IN_TouchLayoutMetrics( &w, &h, &minDim );

	r = minDim * ( in_touchStickSize ? in_touchStickSize->value : 0.13f );
	if ( r < minDim * 0.06f )
		r = minDim * 0.06f;
	if ( r > minDim * 0.24f )
		r = minDim * 0.24f;

	btn = minDim * ( in_touchBtnSize ? in_touchBtnSize->value : 0.11f );
	if ( btn < 44.0f )
		btn = 44.0f;
	if ( btn > minDim * 0.20f )
		btn = minDim * 0.20f;

	rx = IN_TouchClamp01( in_touchRightX ? in_touchRightX->value : 0.90f );

	o->visible = qtrue;
	o->moveR = r;
	o->moveCx = w * IN_TouchClamp01( in_touchMoveX ? in_touchMoveX->value : 0.14f );
	o->moveCy = h * IN_TouchClamp01( in_touchMoveY ? in_touchMoveY->value : 0.78f );
	o->moveKnobX = o->moveCx + moveStickX * r;
	o->moveKnobY = o->moveCy + moveStickY * r;

	o->lookR = r * 0.92f;
	o->lookCx = w * IN_TouchClamp01( in_touchLookX ? in_touchLookX->value : rx );
	o->lookCy = h * IN_TouchClamp01( in_touchLookY ? in_touchLookY->value : 0.48f );
	o->lookKnobX = o->lookCx + lookStickX * o->lookR;
	o->lookKnobY = o->lookCy + lookStickY * o->lookR;

	o->fireW = o->fireH = btn;
	o->fireX = w * rx - btn * 0.5f;
	o->fireY = h * IN_TouchClamp01( in_touchFireY ? in_touchFireY->value : 0.82f ) - btn * 0.5f;

	o->jumpW = o->jumpH = btn * 0.88f;
	o->jumpX = w * rx - o->jumpW * 0.5f;
	o->jumpY = h * IN_TouchClamp01( in_touchJumpY ? in_touchJumpY->value : 0.66f ) - o->jumpW * 0.5f;

	o->fireHeld = fireHeld;
	o->jumpHeld = jumpHeld;
}

void IN_TouchFrame( void )
{
	float sens, dz;
	int dx, dy;
	int w, h;

	if ( !in_touch || !in_touch->integer )
		return;

	if ( IN_TouchUIMode() )
		return;

	dz = IN_TouchGetDeadzone();

	IN_TouchReleaseMove();
	if ( moveStickActive )
	{
		if ( moveStickY < -dz )
			Cbuf_AddText( "+forward\n" );
		else if ( moveStickY > dz )
			Cbuf_AddText( "+back\n" );

		if ( moveStickX < -dz )
			Cbuf_AddText( "+left\n" );
		else if ( moveStickX > dz )
			Cbuf_AddText( "+right\n" );
	}

	if ( lookStickActive )
	{
		GLimp_GetWindowSize( &w, &h );
		if ( w < 1 )
			w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 1024;
		if ( h < 1 )
			h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 768;
		sens = in_touchSensitivity ? in_touchSensitivity->value : 1.0f;
		dx = (int)( lookStickX * sens * (float)w * 0.03f );
		dy = (int)( lookStickY * sens * (float)h * 0.03f );
		if ( dx != 0 || dy != 0 )
			Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );
	}
}
