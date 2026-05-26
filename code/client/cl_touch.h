#ifndef CL_TOUCH_H
#define CL_TOUCH_H

void IN_TouchInit( void );
void IN_TouchShutdown( void );
void IN_TouchFrame( void );
void IN_TouchEvent( float x, float y, int finger, qboolean down );

#endif
