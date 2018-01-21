/*----------------------------------------------------------------------------
 * Name:    Graphics.h
 * Purpose: Graphics realted definitions
 * Note(s): 
 *--------------------------------------------------------------------------*/

#ifndef _Graphics_H
#define _Graphics_H

#define M_PI 3.14159265358979323846264338327
#define HUD_HEIGHT 		30

#define CENTERX 376
#define CENTERY 672
#define LEFTBOUND 256
#define RIGHTBOUND 496
#define TOPBOUND 512
#define BOTTOMBOUND 832
#define SCREENWIDTH 240
#define SCREENHEIGHT 320

#define GameOverLines 12

extern OS_MUT mut_GLCD; 		/* Mutex to controll GLCD access     */

extern void LCD_DrawLine(int x1, int y1, int x2, int y2);
extern __task void startScreen(void);
extern void renderAsteroids(void);
extern void renderBullets(void);
extern void renderShip(void);
extern void renderHUD(void);

extern void bombEffect(void);

extern void setupHUD(void);
extern __task void GameOver(void);
#endif