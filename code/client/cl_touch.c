/*
===========================================================================
Touch iOS : souris virtuelle (menus) + joysticks / boutons en jeu
===========================================================================
*/
#include "client.h"
#include "cl_touch.h"
#include "keycodes.h"
#include "../renderercommon/tr_common.h"
#if IOS
#include "../ios/ios_layer.h"
#endif

#define TOUCH_MAX_FINGERS 8

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
static cvar_t *in_touchRightX;
static cvar_t *in_touchFireY;
static cvar_t *in_touchWeaponsX;
static cvar_t *in_touchWeaponsY;
static cvar_t *in_touchActionsX;
static cvar_t *in_touchActionsY;
static cvar_t *in_touchEscX;
static cvar_t *in_touchEscY;
static cvar_t *in_touchDebug;

static float moveStickX;
static float moveStickY;
static qboolean moveStickActive;

static float lookStickX;
static float lookStickY;
static qboolean lookStickActive;

static qboolean fireHeld;
static qboolean jumpHeld;
static qboolean aimHeld;
static qboolean runHeld;
static qboolean useHeld;
static qboolean eHeld;

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

static float IN_TouchPlaceInRange( float norm, float size, float inset )
{
	if ( size <= inset * 2.0f )
		return size * 0.5f;
	return inset + ( size - inset * 2.0f ) * IN_TouchClamp01( norm );
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

static void IN_TouchQueueKey( int key, qboolean down )
{
	Com_QueueEvent( 0, SE_KEY, key, down, 0, NULL );
}

static void IN_TouchKeyTap( int key )
{
	IN_TouchQueueKey( key, qtrue );
	IN_TouchQueueKey( key, qfalse );
}

static void IN_TouchReleaseAim( void )
{
	if ( aimHeld )
	{
		aimHeld = qfalse;
		IN_TouchQueueKey( K_MOUSE2, qfalse );
	}
}

static void IN_TouchReleaseRun( void )
{
	if ( runHeld )
	{
		runHeld = qfalse;
		IN_TouchQueueKey( K_SHIFT, qfalse );
	}
}

static void IN_TouchReleaseUse( void )
{
	if ( useHeld )
	{
		useHeld = qfalse;
		IN_TouchQueueKey( 'f', qfalse );
	}
}

static void IN_TouchReleaseE( void )
{
	if ( eHeld )
	{
		eHeld = qfalse;
		IN_TouchQueueKey( 'e', qfalse );
	}
}

static void IN_TouchReleaseAllKeys( void )
{
	IN_TouchReleaseAim();
	IN_TouchReleaseRun();
	IN_TouchReleaseUse();
	IN_TouchReleaseE();
}

static void IN_TouchLayoutMetrics( int *w, int *h, float *minDim );

static void IN_TouchUIMouse( float x, float y, qboolean down, qboolean move )
{
	float sens;
	float scaleX, scaleY;
	float minDim;
	float dpx, dpy;
	int dx, dy;
	int w, h;

	IN_TouchLayoutMetrics( &w, &h, &minDim );
	(void)minDim;
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
	in_touchMoveSensitivity = Cvar_Get( "in_touchMoveSensitivity", "1.0", CVAR_ARCHIVE );
	in_touchSensitivity = Cvar_Get( "in_touchSensitivity", "1.0", CVAR_ARCHIVE );
	in_touchUISensitivity = Cvar_Get( "in_touchUISensitivity", "3.5", CVAR_ARCHIVE );
	in_touchDeadzone = Cvar_Get( "in_touchDeadzone", "0.18", CVAR_ARCHIVE );
	in_touchStickSize = Cvar_Get( "in_touchStickSize", "0.13", CVAR_ARCHIVE );
	in_touchBtnSize = Cvar_Get( "in_touchBtnSize", "0.11", CVAR_ARCHIVE );
	in_touchMoveX = Cvar_Get( "in_touchMoveX", "0", CVAR_ARCHIVE );
	in_touchMoveY = Cvar_Get( "in_touchMoveY", "0.82", CVAR_ARCHIVE );
	in_touchLookX = Cvar_Get( "in_touchLookX", "0.82", CVAR_ARCHIVE );
	in_touchLookY = Cvar_Get( "in_touchLookY", "0.34", CVAR_ARCHIVE );
	in_touchRightX = Cvar_Get( "in_touchRightX", "0.92", CVAR_ARCHIVE );
	in_touchFireY = Cvar_Get( "in_touchFireY", "0.82", CVAR_ARCHIVE );
	in_touchWeaponsX = Cvar_Get( "in_touchWeaponsX", "0.52", CVAR_ARCHIVE );
	in_touchWeaponsY = Cvar_Get( "in_touchWeaponsY", "0.03", CVAR_ARCHIVE );
	in_touchActionsX = Cvar_Get( "in_touchActionsX", "0.05", CVAR_ARCHIVE );
	in_touchActionsY = Cvar_Get( "in_touchActionsY", "0.15", CVAR_ARCHIVE );
	in_touchEscX = Cvar_Get( "in_touchEscX", "0.92", CVAR_ARCHIVE );
	in_touchEscY = Cvar_Get( "in_touchEscY", "0.03", CVAR_ARCHIVE );
	in_touchDebug = Cvar_Get( "in_touchDebug", "0", CVAR_ARCHIVE );
}

void IN_TouchApplyDefaults( void )
{
	Cvar_Set( "in_touch", "1" );
	Cvar_Set( "in_touchMoveSensitivity", "1.0" );
	Cvar_Set( "in_touchSensitivity", "1.0" );
	Cvar_Set( "in_touchUISensitivity", "3.5" );
	Cvar_Set( "in_touchDeadzone", "0.18" );
	Cvar_Set( "in_touchStickSize", "0.13" );
	Cvar_Set( "in_touchBtnSize", "0.11" );
	Cvar_Set( "in_touchMoveX", "0" );
	Cvar_Set( "in_touchMoveY", "0.82" );
	Cvar_Set( "in_touchLookX", "0.82" );
	Cvar_Set( "in_touchLookY", "0.34" );
	Cvar_Set( "in_touchRightX", "0.92" );
	Cvar_Set( "in_touchFireY", "0.82" );
	Cvar_Set( "in_touchWeaponsX", "0.52" );
	Cvar_Set( "in_touchWeaponsY", "0.03" );
	Cvar_Set( "in_touchActionsX", "0.05" );
	Cvar_Set( "in_touchActionsY", "0.15" );
	Cvar_Set( "in_touchEscX", "0.92" );
	Cvar_Set( "in_touchEscY", "0.03" );
	Cvar_Set( "r_gamma", "1" );
	Cvar_Set( "r_intensity", "1" );
	Cvar_Set( "r_overBrightBits", "1" );
}

void IN_TouchReadConfig( touchConfig_t *cfg )
{
	if ( !cfg )
		return;

	IN_TouchRegisterCvars();

	cfg->enabled = ( in_touch && in_touch->integer ) ? qtrue : qfalse;
	cfg->moveSensitivity = in_touchMoveSensitivity ? in_touchMoveSensitivity->value : 1.0f;
	cfg->lookSensitivity = in_touchSensitivity ? in_touchSensitivity->value : 1.0f;
	cfg->uiSensitivity = in_touchUISensitivity ? in_touchUISensitivity->value : 3.5f;
	cfg->deadzone = in_touchDeadzone ? in_touchDeadzone->value : 0.18f;
	cfg->stickSize = in_touchStickSize ? in_touchStickSize->value : 0.13f;
	cfg->btnSize = in_touchBtnSize ? in_touchBtnSize->value : 0.11f;
	cfg->moveX = in_touchMoveX ? in_touchMoveX->value : 0.0f;
	cfg->moveY = in_touchMoveY ? in_touchMoveY->value : 0.82f;
	cfg->lookX = in_touchLookX ? in_touchLookX->value : 0.82f;
	cfg->lookY = in_touchLookY ? in_touchLookY->value : 0.34f;
	cfg->rightX = in_touchRightX ? in_touchRightX->value : 0.92f;
	cfg->fireY = in_touchFireY ? in_touchFireY->value : 0.82f;
	cfg->weaponsX = in_touchWeaponsX ? in_touchWeaponsX->value : 0.52f;
	cfg->weaponsY = in_touchWeaponsY ? in_touchWeaponsY->value : 0.03f;
	cfg->actionsX = in_touchActionsX ? in_touchActionsX->value : 0.05f;
	cfg->actionsY = in_touchActionsY ? in_touchActionsY->value : 0.15f;
	cfg->escX = in_touchEscX ? in_touchEscX->value : 0.92f;
	cfg->escY = in_touchEscY ? in_touchEscY->value : 0.03f;
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
	Cvar_SetValue( "in_touchMoveSensitivity", cfg->moveSensitivity );
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
	Cvar_SetValue( "in_touchWeaponsX", cfg->weaponsX );
	Cvar_SetValue( "in_touchWeaponsY", cfg->weaponsY );
	Cvar_SetValue( "in_touchActionsX", cfg->actionsX );
	Cvar_SetValue( "in_touchActionsY", cfg->actionsY );
	Cvar_SetValue( "in_touchEscX", cfg->escX );
	Cvar_SetValue( "in_touchEscY", cfg->escY );

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
	aimHeld = runHeld = useHeld = eHeld = qfalse;
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
	IN_TouchReleaseAllKeys();
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

void IN_TouchButton( touchButton_t btn, qboolean down )
{
	if ( IN_TouchUIMode() )
		return;

	switch ( btn )
	{
	case TOUCH_BTN_FIRE:
		IN_TouchSetFire( down );
		return;
	case TOUCH_BTN_JUMP:
		IN_TouchSetJump( down );
		return;
	case TOUCH_BTN_AIM:
		if ( down && !aimHeld )
		{
			aimHeld = qtrue;
			IN_TouchQueueKey( K_MOUSE2, qtrue );
		}
		else if ( !down && aimHeld )
			IN_TouchReleaseAim();
		return;
	case TOUCH_BTN_RUN:
		if ( down && !runHeld )
		{
			runHeld = qtrue;
			IN_TouchQueueKey( K_SHIFT, qtrue );
		}
		else if ( !down && runHeld )
			IN_TouchReleaseRun();
		return;
	case TOUCH_BTN_USE:
		if ( down && !useHeld )
		{
			useHeld = qtrue;
			IN_TouchQueueKey( 'f', qtrue );
		}
		else if ( !down && useHeld )
			IN_TouchReleaseUse();
		return;
	case TOUCH_BTN_WEAPON1:
		if ( down )
			IN_TouchKeyTap( '1' );
		return;
	case TOUCH_BTN_WEAPON2:
		if ( down )
			IN_TouchKeyTap( '2' );
		return;
	case TOUCH_BTN_WEAPON3:
		if ( down )
			IN_TouchKeyTap( '3' );
		return;
	case TOUCH_BTN_WEAPON4:
		if ( down )
			IN_TouchKeyTap( '4' );
		return;
	case TOUCH_BTN_DROP:
		if ( down )
			IN_TouchKeyTap( 'l' );
		return;
	case TOUCH_BTN_RELOAD:
		if ( down )
			IN_TouchKeyTap( 'r' );
		return;
	case TOUCH_BTN_BUY:
		if ( down )
			IN_TouchKeyTap( 'b' );
		return;
	case TOUCH_BTN_E:
		if ( down && !eHeld )
		{
			eHeld = qtrue;
			IN_TouchQueueKey( 'e', qtrue );
		}
		else if ( !down && eHeld )
			IN_TouchReleaseE();
		return;
	case TOUCH_BTN_ESC:
		if ( down )
			IN_TouchKeyTap( K_ESCAPE );
		return;
	default:
		return;
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

	vw = 0;
	vh = 0;
#if IOS
	IOS_Layer_GetLayoutSize( &vw, &vh );
#endif
	if ( vw < 1 || vh < 1 )
		GLimp_GetWindowSize( &vw, &vh );
#if IOS
	/* Ne jamais retomber sur glConfig (pixels viewport) pour le tactile */
	if ( vw < 1 )
		vw = 390;
	if ( vh < 1 )
		vh = 844;
#else
	if ( vw < 1 )
		vw = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 1024;
	if ( vh < 1 )
		vh = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 768;
#endif
	*w = vw;
	*h = vh;
	*minDim = (float)( vw < vh ? vw : vh );
}

static qboolean IN_TouchConsoleActiveInternal( void )
{
	return ( Key_GetCatcher() & ( KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) ? qtrue : qfalse;
}

qboolean IN_TouchConsoleActive( void )
{
	return IN_TouchConsoleActiveInternal();
}

void IN_TouchToggleConsole( void )
{
	Con_ToggleConsole_f();
}

void IN_TouchGetOverlayForSize( touchOverlay_t *o, int screenW, int screenH )
{
	int w, h;
	float minDim, r, btn, rx, mx, lx;
	float areaW, areaH;

	if ( !o )
		return;

	memset( o, 0, sizeof( *o ) );
	if ( !in_touch || !in_touch->integer || IN_TouchUIMode() || IN_TouchConsoleActiveInternal() )
		return;

	w = screenW;
	h = screenH;
	if ( w < 1 || h < 1 )
		IN_TouchLayoutMetrics( &w, &h, &minDim );

	/* Plein écran physique (points UIKit), pas le viewport letterboxé */
	areaW = (float)w;
	areaH = (float)h;
	minDim = areaW < areaH ? areaW : areaH;

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

	rx = IN_TouchClamp01( in_touchRightX ? in_touchRightX->value : 0.92f );
	mx = IN_TouchClamp01( in_touchMoveX ? in_touchMoveX->value : 0.0f );
	lx = IN_TouchClamp01( in_touchLookX ? in_touchLookX->value : 0.82f );

	o->visible = qtrue;
	o->moveR = r;
	o->lookR = r * 0.92f;

	o->moveCx = IN_TouchPlaceInRange( mx, areaW, r );
	o->lookCx = IN_TouchPlaceInRange( lx, areaW, o->lookR );

	{
		float gridCell, gridGap, gridW;
		float gridCx, gridLeft, bottomRowCy, topRowCy;

		gridCell = btn;
		gridGap = 6.0f;
		gridW = gridCell * 2.0f + gridGap;

		o->fireW = o->fireH = gridCell;
		o->jumpW = o->jumpH = gridCell;
		o->aimW = o->aimH = gridCell;
		o->runW = o->runH = gridCell;

		gridCx = IN_TouchPlaceInRange( rx, areaW, gridW * 0.5f );
		gridLeft = gridCx - gridW * 0.5f;

		bottomRowCy = IN_TouchPlaceInRange( in_touchFireY ? in_touchFireY->value : 0.82f,
			areaH, gridCell * 0.5f + gridGap * 0.5f );
		topRowCy = bottomRowCy - gridCell - gridGap;

		o->aimX = gridLeft;
		o->aimY = topRowCy - gridCell * 0.5f;
		o->runX = gridLeft + gridCell + gridGap;
		o->runY = topRowCy - gridCell * 0.5f;

		o->jumpX = gridLeft;
		o->jumpY = bottomRowCy - gridCell * 0.5f;
		o->fireX = gridLeft + gridCell + gridGap;
		o->fireY = bottomRowCy - gridCell * 0.5f;
	}

	o->w1W = o->w1H = btn * 0.72f;
	o->w2W = o->w2H = btn * 0.72f;
	o->w3W = o->w3H = btn * 0.72f;
	o->w4W = o->w4H = btn * 0.72f;
	o->dropW = o->dropH = btn * 0.72f;
	o->reloadW = o->reloadH = btn * 0.72f;
	o->useW = o->useH = btn * 0.72f;
	o->buyW = o->buyH = btn * 0.72f;
	o->eW = o->eH = btn * 0.72f;
	o->escW = o->escH = btn * 0.72f;

	o->cfgW = o->cfgH = 44.0f;
	o->cfgX = 8.0f;
	o->cfgY = 8.0f;
	o->kbdW = o->kbdH = 44.0f;
	o->kbdX = 8.0f;
	o->kbdY = 58.0f;

	o->moveCy = IN_TouchPlaceInRange( in_touchMoveY ? in_touchMoveY->value : 0.82f, areaH, r );
	o->moveKnobX = o->moveCx + moveStickX * r;
	o->moveKnobY = o->moveCy + moveStickY * r;

	o->lookCy = IN_TouchPlaceInRange( in_touchLookY ? in_touchLookY->value : 0.34f, areaH, o->lookR );
	o->lookKnobX = o->lookCx + lookStickX * o->lookR;
	o->lookKnobY = o->lookCy + lookStickY * o->lookR;

	{
		float wx, wy, gap, rowW, rowCx, rowTop;

		wx = IN_TouchClamp01( in_touchWeaponsX ? in_touchWeaponsX->value : 0.52f );
		wy = IN_TouchClamp01( in_touchWeaponsY ? in_touchWeaponsY->value : 0.03f );
		gap = o->w1W * 1.15f;
		rowW = gap * 3.0f + o->w1W;
		rowCx = IN_TouchPlaceInRange( wx, areaW, rowW * 0.5f );
		rowTop = IN_TouchPlaceInRange( wy, areaH, o->w1H * 0.5f ) - o->w1H * 0.5f;

		o->w1X = rowCx - rowW * 0.5f;
		o->w1Y = rowTop;
		o->w2X = o->w1X + gap;
		o->w2Y = rowTop;
		o->w3X = o->w1X + gap * 2.0f;
		o->w3Y = rowTop;
		o->w4X = o->w1X + gap * 3.0f;
		o->w4Y = rowTop;
	}

	{
		float ax, ay, ex, ey;

		ax = IN_TouchClamp01( in_touchActionsX ? in_touchActionsX->value : 0.05f );
		ay = IN_TouchClamp01( in_touchActionsY ? in_touchActionsY->value : 0.15f );
		ex = IN_TouchClamp01( in_touchEscX ? in_touchEscX->value : 0.92f );
		ey = IN_TouchClamp01( in_touchEscY ? in_touchEscY->value : 0.03f );

		o->dropX = IN_TouchPlaceInRange( ax, areaW, o->dropW * 0.5f ) - o->dropW * 0.5f;
		o->dropY = IN_TouchPlaceInRange( ay, areaH, o->dropH * 0.5f ) - o->dropH * 0.5f;
		o->reloadX = o->dropX + o->dropW + 8.0f;
		o->reloadY = o->dropY;
		o->eX = o->reloadX + o->reloadW + 8.0f;
		o->eY = o->dropY;
		o->useX = o->dropX;
		o->useY = o->dropY + o->useH + 8.0f;
		o->buyX = o->useX + o->buyW + 8.0f;
		o->buyY = o->useY;

		o->escX = IN_TouchPlaceInRange( ex, areaW, o->escW * 0.5f ) - o->escW * 0.5f;
		o->escY = IN_TouchPlaceInRange( ey, areaH, o->escH * 0.5f ) - o->escH * 0.5f;
	}

	o->fireHeld = fireHeld;
	o->jumpHeld = jumpHeld;
	o->aimHeld = aimHeld;
	o->runHeld = runHeld;
	o->useHeld = useHeld;
	o->eHeld = eHeld;
}

void IN_TouchGetOverlay( touchOverlay_t *o )
{
	int w, h;
	float minDim;

	if ( !o )
		return;

	IN_TouchLayoutMetrics( &w, &h, &minDim );
	IN_TouchGetOverlayForSize( o, w, h );
}

void IN_TouchFrame( void )
{
	float moveSens, lookSens, dz, minDim;
	float mx, my;
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
		moveSens = in_touchMoveSensitivity ? in_touchMoveSensitivity->value : 1.0f;
		if ( moveSens < 0.25f )
			moveSens = 0.25f;
		if ( moveSens > 4.0f )
			moveSens = 4.0f;

		mx = moveStickX * moveSens;
		my = moveStickY * moveSens;
		if ( mx > 1.0f )
			mx = 1.0f;
		else if ( mx < -1.0f )
			mx = -1.0f;
		if ( my > 1.0f )
			my = 1.0f;
		else if ( my < -1.0f )
			my = -1.0f;

		if ( my < -dz )
			Cbuf_AddText( "+forward\n" );
		else if ( my > dz )
			Cbuf_AddText( "+back\n" );

		if ( mx < -dz )
			Cbuf_AddText( "+left\n" );
		else if ( mx > dz )
			Cbuf_AddText( "+right\n" );
	}

	if ( lookStickActive )
	{
		IN_TouchLayoutMetrics( &w, &h, &minDim );
		(void)minDim;
		lookSens = in_touchSensitivity ? in_touchSensitivity->value : 1.0f;
		dx = (int)( lookStickX * lookSens * (float)w * 0.03f );
		dy = (int)( lookStickY * lookSens * (float)h * 0.03f );
		if ( dx != 0 || dy != 0 )
			Com_QueueEvent( 0, SE_MOUSE, dx, dy, 0, NULL );
	}
}
