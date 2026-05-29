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
	qboolean fireHeld;
	qboolean jumpHeld;
} touchOverlay_t;

/* Configuration tactile (pont moteur ↔ couche iOS) */
typedef struct {
	qboolean enabled;
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
	float jumpY;
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

void IN_TouchGetOverlay( touchOverlay_t *out );

#endif
