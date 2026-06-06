#ifndef IOS_LOADING_H
#define IOS_LOADING_H

void IOS_Loading_Begin( const char *message );
void IOS_Loading_SetProgress( float progress, const char *message );
void IOS_Loading_PumpUI( void );
void IOS_Loading_End( void );

#endif
