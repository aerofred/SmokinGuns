#ifndef CL_TOUCH_H
#define CL_TOUCH_H

#include "../qcommon/q_shared.h"

typedef struct {
	qboolean visible;
	float moveCx, moveCy, moveR;
	float moveKnobX, moveKnobY;
	float lookCx, lookCy, lookR;
	float lookKnobX, lookKnobY;
	float fireX, fireY, fireW, fireH;
	float jumpX, jumpY, jumpW, jumpH;
	float aimX, aimY, aimW, aimH;
	float runX, runY, runW, runH;
	float w1X, w1Y, w1W, w1H;
	float w2X, w2Y, w2W, w2H;
	float w3X, w3Y, w3W, w3H;
	float w4X, w4Y, w4W, w4H;
	float dropX, dropY, dropW, dropH;
	float reloadX, reloadY, reloadW, reloadH;
	float useX, useY, useW, useH;
	float buyX, buyY, buyW, buyH;
	float eX, eY, eW, eH;
	float escX, escY, escW, escH;
	float cfgX, cfgY, cfgW, cfgH;
	float kbdX, kbdY, kbdW, kbdH;
	qboolean fireHeld;
	qboolean jumpHeld;
	qboolean aimHeld;
	qboolean runHeld;
	qboolean useHeld;
	qboolean eHeld;
} touchOverlay_t;

typedef enum {
	TOUCH_BTN_FIRE = 0,
	TOUCH_BTN_JUMP,
	TOUCH_BTN_AIM,
	TOUCH_BTN_RUN,
	TOUCH_BTN_WEAPON1,
	TOUCH_BTN_WEAPON2,
	TOUCH_BTN_WEAPON3,
	TOUCH_BTN_WEAPON4,
	TOUCH_BTN_DROP,
	TOUCH_BTN_RELOAD,
	TOUCH_BTN_USE,
	TOUCH_BTN_BUY,
	TOUCH_BTN_E,
	TOUCH_BTN_ESC
} touchButton_t;

/* Configuration tactile (pont moteur ↔ couche iOS) */
typedef struct {
	qboolean enabled;
	float moveSensitivity;
	float lookSensitivity;
	float uiSensitivity;
	float deadzone;
	float stickSize;
	float btnSize;
	float moveX;
	float moveY;
	float lookX;
	float lookY;
	float rightX;
	float fireY;
	float weaponsX;
	float weaponsY;
	float actionsX;
	float actionsY;
	float escX;
	float escY;
	/* Affichage (iOS) — r_gamma, r_intensity, r_overBrightBits */
	float gamma;
	float intensity;
	float overBrightBits;
} touchConfig_t;

void IN_TouchInit( void );
void IN_TouchShutdown( void );
void IN_TouchFrame( void );

void IN_TouchReadConfig( touchConfig_t *cfg );
void IN_TouchWriteConfig( const touchConfig_t *cfg );
void IN_TouchApplyDefaults( void );

qboolean IN_TouchInUIMode( void );

/* Entrée menu Quake (souris virtuelle au doigt) */
void IN_TouchPointer( float x, float y, qboolean down, qboolean move );
void IN_TouchFinger( int fingerId, float x, float y, qboolean down, qboolean move );

/* Entrée jeu depuis la couche UIKit */
void IN_TouchSetMoveStick( float nx, float ny, qboolean active );
void IN_TouchSetLookStick( float nx, float ny, qboolean active );
void IN_TouchSetFire( qboolean down );
void IN_TouchSetJump( qboolean down );
void IN_TouchButton( touchButton_t btn, qboolean down );

void IN_TouchGetOverlay( touchOverlay_t *out );
/* screenW/H : bords physiques UIKit en points (pas le viewport Quake) */
void IN_TouchGetOverlayForSize( touchOverlay_t *out, int screenW, int screenH );

void IN_TouchToggleConsole( void );
qboolean IN_TouchConsoleActive( void );

#endif
