/*
===========================================================================
Touch screen controls for iOS / mobile
===========================================================================
*/
#include "client.h"
#include "cl_touch.h"

#define TOUCH_MAX_FINGERS 8
#define TOUCH_BTN_COUNT 6

typedef struct {
	qboolean active;
	float x;
	float y;
} touchFinger_t;

static touchFinger_t touchFingers[TOUCH_MAX_FINGERS];
static cvar_t *in_touch;
static cvar_t *in_touchSensitivity;
static cvar_t *in_touchMoveZone; /* fraction of screen width for move stick (0.0-0.5) */

static struct {
	float x, y, w, h;
	const char *cmd;
	int keyNum; /* cached key binding index via Cmd */
	qboolean held;
} touchButtons[TOUCH_BTN_COUNT];

static int touchMoveFinger = -1;
static int touchLookFinger = -1;
static float touchMoveCenterX;
static float touchMoveCenterY;
static float touchLookLastX;
static float touchLookLastY;

void IN_TouchInit( void )
{
	in_touch = Cvar_Get( "in_touch", "1", CVAR_ARCHIVE );
	in_touchSensitivity = Cvar_Get( "in_touchSensitivity", "1.5", CVAR_ARCHIVE );
	in_touchMoveZone = Cvar_Get( "in_touchMoveZone", "0.35", CVAR_ARCHIVE );

	memset( touchFingers, 0, sizeof( touchFingers ) );
	touchMoveFinger = -1;
	touchLookFinger = -1;

	/* Right side action buttons (normalized coords set in frame) */
	touchButtons[0].cmd = "+attack";
	touchButtons[1].cmd = "+speed";
	touchButtons[2].cmd = "+moveup";
	touchButtons[3].cmd = "+movedown";
	touchButtons[4].cmd = "weapprev";
	touchButtons[5].cmd = "weapnext";
}

void IN_TouchShutdown( void )
{
	int i;
	for ( i = 0; i < TOUCH_BTN_COUNT; i++ )
	{
		if ( touchButtons[i].held )
			Cbuf_AddText( va( "-buttons ; -forward ; -back ; -moveleft ; -moveright ; -moveup ; -movedown ; -speed ; -attack\n" ) );
	}
}

static qboolean IN_TouchHitRect( float px, float py, float x, float y, float w, float h )
{
	return ( px >= x && px <= x + w && py >= y && py <= y + h );
}

void IN_TouchEvent( float x, float y, int finger, qboolean down )
{
	float zone;
	int w, h;
	int i;

	if ( !in_touch || !in_touch->integer || finger < 0 || finger >= TOUCH_MAX_FINGERS )
		return;

	w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 1024;
	h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 768;
	zone = in_touchMoveZone->value;
	if ( zone < 0.1f )
		zone = 0.1f;
	if ( zone > 0.5f )
		zone = 0.5f;

	touchFingers[finger].active = down;
	touchFingers[finger].x = x;
	touchFingers[finger].y = y;

	if ( down )
	{
		/* Move zone: left */
		if ( x < w * zone )
		{
			if ( touchMoveFinger < 0 )
			{
				touchMoveFinger = finger;
				touchMoveCenterX = x;
				touchMoveCenterY = y;
			}
			return;
		}
		/* Look zone: right half excluding button column */
		if ( x > w * 0.55f && x < w * 0.82f )
		{
			if ( touchLookFinger < 0 )
			{
				touchLookFinger = finger;
				touchLookLastX = x;
				touchLookLastY = y;
			}
			return;
		}
		/* On-screen buttons */
		for ( i = 0; i < TOUCH_BTN_COUNT; i++ )
		{
			if ( IN_TouchHitRect( x, y, touchButtons[i].x, touchButtons[i].y,
					touchButtons[i].w, touchButtons[i].h ) )
			{
				touchButtons[i].held = qtrue;
				Cbuf_AddText( va( "%s\n", touchButtons[i].cmd ) );
				return;
			}
		}
	}
	else
	{
		if ( finger == touchMoveFinger )
		{
			Cbuf_AddText( "-forward ; -back ; -moveleft ; -moveright\n" );
			touchMoveFinger = -1;
		}
		if ( finger == touchLookFinger )
			touchLookFinger = -1;
		for ( i = 0; i < TOUCH_BTN_COUNT; i++ )
		{
			if ( touchButtons[i].held )
			{
				if ( touchButtons[i].cmd[0] == '+' )
					Cbuf_AddText( va( "-%s\n", touchButtons[i].cmd + 1 ) );
				touchButtons[i].held = qfalse;
			}
		}
	}
}

void IN_TouchFrame( void )
{
	float dx, dy, sens;
	float zone;
	int w, h;
	int i;

	if ( !in_touch || !in_touch->integer )
		return;

	w = cls.glconfig.vidWidth > 0 ? cls.glconfig.vidWidth : 1024;
	h = cls.glconfig.vidHeight > 0 ? cls.glconfig.vidHeight : 768;
	zone = in_touchMoveZone->value;

	/* Layout buttons */
	for ( i = 0; i < TOUCH_BTN_COUNT; i++ )
	{
		touchButtons[i].w = w * 0.12f;
		touchButtons[i].h = h * 0.12f;
		touchButtons[i].x = w * 0.84f;
		touchButtons[i].y = h * 0.15f + i * ( touchButtons[i].h + 8 );
	}

	sens = in_touchSensitivity->value;

	if ( touchMoveFinger >= 0 && touchFingers[touchMoveFinger].active )
	{
		dx = ( touchFingers[touchMoveFinger].x - touchMoveCenterX ) / ( w * zone );
		dy = ( touchFingers[touchMoveFinger].y - touchMoveCenterY ) / ( h * 0.35f );
		Cbuf_AddText( "-forward ; -back ; -moveleft ; -moveright\n" );
		if ( dy < -0.15f )
			Cbuf_AddText( "+forward\n" );
		else if ( dy > 0.15f )
			Cbuf_AddText( "+back\n" );
		if ( dx < -0.15f )
			Cbuf_AddText( "+moveleft\n" );
		else if ( dx > 0.15f )
			Cbuf_AddText( "+moveright\n" );
	}

	if ( touchLookFinger >= 0 && touchFingers[touchLookFinger].active )
	{
		dx = ( touchFingers[touchLookFinger].x - touchLookLastX ) * sens;
		dy = ( touchFingers[touchLookFinger].y - touchLookLastY ) * sens;
		touchLookLastX = touchFingers[touchLookFinger].x;
		touchLookLastY = touchFingers[touchLookFinger].y;
		if ( dx != 0.0f || dy != 0.0f )
			Com_QueueEvent( 0, SE_MOUSE, (int)dx, (int)dy, 0, NULL );
	}
}
