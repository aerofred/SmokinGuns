#ifndef IOS_LAYER_H
#define IOS_LAYER_H

#include "../qcommon/q_shared.h"

struct SDL_Window;

/*
 * Couche UI iOS (UIKit) au-dessus du moteur SDL / Quake.
 * - Affichage et saisie : overlay de jeu
 * - Réglages tactiles : interface native (pas de menu Quake ni console)
 */

void IOS_Layer_Init( struct SDL_Window *window );
void IOS_Layer_Shutdown( void );
void IOS_Layer_Tick( void );

/* Affiche la fenêtre overlay au-dessus de SDL (après l’écran de chargement) */
void IOS_Layer_AttachToWindow( void );

/* Présente le panneau de réglages tactiles (UIKit) */
void IOS_Layer_OpenTouchSettings( void );

/* Masque l’overlay jeu (menus Quake, réglages iOS ouverts) */
void IOS_Layer_SetGameOverlayVisible( qboolean visible );

#endif
