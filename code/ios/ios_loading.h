#ifndef IOS_LOADING_H
#define IOS_LOADING_H

#include "../qcommon/q_shared.h"

void IOS_Loading_Begin( void );
void IOS_Loading_SetProgress( float progress, const char *status );
void IOS_Loading_PumpUI( void );
void IOS_Loading_End( void );

#endif
