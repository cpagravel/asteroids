/*----------------------------------------------------------------------------
 * Name:    Grpahics.c
 * Purpose: Graphics related functions
 * Note(s): 
 *----------------------------------------------------------------------------*/
 
#include <math.h>
//#include <stdio.h>
#include <RTL.h>
#include "LPC17xx.H"                         /* LPC17xx definitions           */
#include "Controls.h"
#include "Entities.h"
#include "Graphics.h"
#include "MacroDefs.h"
#include "GLCD.h"

//==============================================================================
//                             SERIAL FUNCTIONS
//==============================================================================

static __inline unsigned char spi_tran (unsigned char byte) {

  LPC_SSP1->DR = byte;
  while (!(LPC_SSP1->SR & RNE));        /* Wait for send to finish            */
  return (LPC_SSP1->DR);
}

static __inline void wr_cmd (unsigned char cmd) {
  LCD_CS(0);
  spi_tran(SPI_START | SPI_WR | SPI_INDEX);   // Write : RS = 0, RW = 0   
  spi_tran(0);
  spi_tran(cmd);
  LCD_CS(1);
}

static __inline void wr_dat_only (unsigned short dat) {

  spi_tran((dat >>   8));                     /* Write D8..D15                */
  spi_tran((dat & 0xFF));                     /* Write D0..D7                 */
}

static __inline void wr_dat_start (void) {
  LCD_CS(0);
  spi_tran(SPI_START | SPI_WR | SPI_DATA);    /* Write : RS = 1, RW = 0       */
}

static __inline void wr_dat_stop (void) {

  LCD_CS(1);
}

static __inline void wr_dat (unsigned short dat) {
  LCD_CS(0);
  spi_tran(SPI_START | SPI_WR | SPI_DATA);    // Write : RS = 1, RW = 0       
  spi_tran((dat >>   8));                     // Write D8..D15                
  spi_tran((dat & 0xFF));                     // Write D0..D7                 
  LCD_CS(1);
}

static __inline void wr_reg (unsigned char reg, unsigned short val) {
  wr_cmd(reg);
  wr_dat(val);
}

void LCD_DrawLine(int x1, int y1, int x2, int y2) {
	//this is optimized by first deciding if more x points or y points need to be drawn. 
	int inflated_slope, temp, x, y, b;
	
	//printf("incoming values are: %d, %d    to %d, %d\n", x1, y1, x2, y2);
	if (((x2-x1)*(x2-x1)) > ((y2-y1)*(y2-y1))) { 
		if (x2 < x1) {
			//switch
			temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}
		//more x points to draw
		inflated_slope = ((y2-y1) << 10)/((x2-x1));
		b = y1-((x1*inflated_slope) >> 10);

		for (x = x1; x < x2; x++) {
			y = (unsigned int)(((x*inflated_slope) >> 10)+b);
			wr_reg(0x20, x);
			wr_reg(0x21, y);
			wr_cmd(0x22);
			wr_dat(_GLCD_Color[TXT_COLOR]);
		}
	} else {
		if (y2 < y1) {
			//switch
			temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}		
		//more y points to draw
		inflated_slope = ((x2-x1) << 10)/((y2-y1));
		b = x1-((y1*inflated_slope) >> 10);
		for (y = y1; y < y2; y++) {
			//set index
			x = (unsigned int)(((y*inflated_slope) >> 10)+b);
			wr_reg(0x20, x);
			wr_reg(0x21, y);
			wr_cmd(0x22);
			wr_dat(_GLCD_Color[TXT_COLOR]);
		}
	}
}

void LCD_drawBlock (unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
  int i,j;

  GLCD_SetWindow(x, y, w, h);
  wr_cmd(0x22);
  wr_dat_start();
  for (i = 0; i < h; i++) {
    for (j = 0; j <= w-1; j++) {
      wr_dat_only(_GLCD_Color[TXT_COLOR]);
    }
  }
  wr_dat_stop();
}

//240x320 dimensions
//create HUD display at top
void setupHUD() {
	GLCD_DisplayString(0, 0, 1, "Score: ");
	//draw lives
}

void drawInHUD() {
	GLCD_SetWindow(0, 0, WIDTH, HUD_HEIGHT);
}

// ==============================================================================
// 															HUD Renders
// ==============================================================================

void renderHUD() {
	//TODO update the GLCD_SetWindow function calls so that drawing never happens on top the HUD
	unsigned char i;
	unsigned short tmp;
	unsigned char asciiCode[10];
	unsigned char scoreStr[10];
	unsigned char levelStr[5];
	sprintf(levelStr, "%d", HUD.level);
	sprintf(scoreStr, "%d", HUD.score);
	GLCD_SetTextColor(White);
	GLCD_DisplayString(0, 0, 1, "Scr: ");
	GLCD_DisplayString(0, 5, 1, scoreStr);
	GLCD_DisplayString(0, 10, 1, "Lv ");
	GLCD_DisplayString(0, 13, 1, levelStr);

	//display Lives
	for (i = 0; i < 5; i ++) {
		if (i < HUD.lives) {
			GLCD_DisplayChar(1, 0+i, 1, 0x90);  
		} else {
			GLCD_DisplayString(1, 0+i, 1, " ");
		}
	}
	// os_mut_wait(mut_ship,0xffff);
	// tmp = ship.shipAngle;
	// os_mut_release(mut_ship);
	// sprintf(asciiCode, "%x", tmp);
	// GLCD_DisplayString(3, 8, 0, asciiCode); 
	// GLCD_DisplayChar(2, 8, 0, tmp);  	
	//display bombs
	for (i = 0; i < HUD.bombs; i++) {
		GLCD_DisplayChar(1, 14-i, 1, 0x80+1);
	}
	GLCD_WindowMax(); 
}

// ==============================================================================
// 															Asteroid Renders
// ==============================================================================

//TODO select a colour based on asteroid health
void drawAsteroid(unsigned short x, unsigned short y, unsigned short type, unsigned char sz) {
	unsigned char i;
	//determine index of radius size
 	for(i = 0; i < shapeResolution; i++) {
 		LCD_DrawLine(x-asteroids->shapes[type][i][sz][0],y+asteroids->shapes[type][i][sz][1], \
					 x-asteroids->shapes[type][(i+1) % shapeResolution][sz][0],y+asteroids->shapes[type][(i+1) % shapeResolution][sz][1]);
 	}
}

void renderAsteroids() {
	//this is a bit ugly, but it's fine as long as its standardized
	unsigned char index, szNew, szOld;
	unsigned long bitVector, undrawBitVector;
	float posX, posY, oldPosX, oldPosY, shape;
	os_mut_wait(mut_asteroids, 0xffff);
		bitVector = asteroids->bitVector;
		undrawBitVector = asteroids->undrawBitVector;
	os_mut_release(mut_asteroids);
	//asteroids to be undrawn
	while(undrawBitVector) {
		index = util_GetMSB(undrawBitVector);
		
		os_mut_wait(mut_asteroids, 0xffff);
			oldPosX = asteroids->oldPosX[index];
			oldPosY = asteroids->oldPosY[index];
			shape = asteroids->shapeType[index];
			szOld = (asteroids->oldHealth[index]-1)/asteroids->chunkSize;
			util_BitClear(asteroids->undrawBitVector, index); //mark as comleted for the undraw task vector
		os_mut_release(mut_asteroids);	
		
		//blacken out old spot
		GLCD_SetTextColor(Black);
		drawAsteroid(oldPosX, oldPosY, shape, szOld);
		
		util_BitClear(undrawBitVector, index);
	}

	while(bitVector) {
		index = util_GetMSB(bitVector);
		
		os_mut_wait(mut_asteroids, 0xffff);
			oldPosX = asteroids->oldPosX[index];
			oldPosY = asteroids->oldPosY[index];
			shape = asteroids->shapeType[index];
			posX = asteroids->posX[index];
			posY = asteroids->posY[index];
			//update old position this might look weird to update the oldposition before rendering it, saves some mutex access though
			szNew = (asteroids->health[index]-1)/asteroids->chunkSize;
			szOld = (asteroids->oldHealth[index]-1)/asteroids->chunkSize;

			//never forget
			asteroids->oldPosX[index] = asteroids->posX[index];
			asteroids->oldPosY[index] = asteroids->posY[index];
			asteroids->oldHealth[index] = asteroids->health[index];
		os_mut_release(mut_asteroids);
		
		//blacken out old spot
		GLCD_SetTextColor(Black);
		drawAsteroid(oldPosX, oldPosY, shape, szOld);

		//draw new
		GLCD_SetTextColor(AsteroidColour[szNew]);
		drawAsteroid(posX, posY, shape, szNew);
		
		GLCD_SetTextColor(White);
		//mark as drawn
		util_BitClear(bitVector, index);
	}
}

// ==============================================================================
// 															Bullet Renders
// ==============================================================================

//draw a bullet. Optimized for performance
void drawBullet(unsigned short x, unsigned short y) {
  unsigned char i;
  GLCD_SetWindow(x, y, bulletSize, bulletSize);
  wr_cmd(0x22);
  wr_dat_start();
	for (i = 0; i < 16;i++) {
		wr_dat_only(Magenta);
	}
  wr_dat_stop();	
}

//clear bullet. Optimized
void clearBullet(unsigned short x, unsigned short y) {
  unsigned char i;
  GLCD_SetWindow(x, y, bulletSize, bulletSize);
  wr_cmd(0x22);
  wr_dat_start();
	for (i=0; i < 16;i++) {
		wr_dat_only(_GLCD_Color[BG_COLOR]);
	}
	wr_dat_stop();		
}

//pretty simple function. Few dots to draw. Use bit ops as much as possible
void renderBullets() {
	unsigned char index;
	unsigned long bitVector;
	
	//shift vector and check. lock mut while doing this
	os_mut_wait(mut_bullets, 0xffff); {
		bitVector = bullets->undrawBitVector;
		
		//bullets to be undrawn
		while(bitVector) {

			index = util_GetMSB(bitVector);
			//blacken out old spot
			// clearBullet(bullets->oldPosX[index], bullets->oldPosY[index]);
			// GLCD_SetTextColor(White);
		    // GLCD_DrawChar(bullets->oldPosX[index]-3, bullets->oldPosY[index]-4,  6,  8, (unsigned char *)&Font_6x8_h  [80]);
			GLCD_SetTextColor(Black);
			clearBullet(bullets->oldPosX[index], bullets->oldPosY[index]);
			util_BitClear(bitVector, index);
		}

		bullets->undrawBitVector = bitVector;
		bitVector = bullets->bitVector;
		while(bitVector) {
			index = util_GetMSB(bitVector);
			
			//blacken out old spot
		  	// c -= 32;
		  	// switch (fi) {
		    // case 0:  /* Font 6 x 8 */
		    // GLCD_DrawChar(bullets->oldPosX[index]-3, bullets->oldPosY[index]-4,  6,  8, (unsigned char *)&Font_6x8_h  [80]);
		    // GLCD_DrawChar(bullets->oldPosX[index], bullets->oldPosY[index],  6,  8, (unsigned char *)&Font_6x8_h  [c * 8]);

			// GLCD_DisplayChar(bullets->oldPosX[index], bullets->oldPosY[index], 0, 0x2a);  
			// GLCD_DrawChar(bullets->posX[index]-3, bullets->posY[index]-4,  6,  8, (unsigned char *)&Font_6x8_h  [80]);
			GLCD_SetTextColor(Black);
			clearBullet(bullets->oldPosX[index], bullets->oldPosY[index]);
			
			//draw new
			GLCD_SetTextColor(Magenta);
			drawBullet(bullets->posX[index], bullets->posY[index]);
			
			//update old position
			bullets->oldPosX[index] = bullets->posX[index];
			bullets->oldPosY[index] = bullets->posY[index];
			
			GLCD_SetTextColor(White);
			//mark as drawn
			util_BitClear(bitVector, index);
		}
	} os_mut_release(mut_bullets);
	GLCD_WindowMax();
}

// ==============================================================================
// 															Ship Renders
// ==============================================================================

void drawShip(unsigned int angle, unsigned int posX, unsigned int posY) {
	unsigned int fx, fy, lx, ly, rx, ry;

	fy = shipsize*cos(angle*M_PI/180)+posY;
	fx = posX-shipsize*sin(angle*M_PI/180);
	
	lx = posX+shipsize*cos((240+angle)*M_PI/180);
	ly = posY+shipsize*sin((240+angle)*M_PI/180);
	
	rx = posX+shipsize*cos((300+angle)*M_PI/180);
	ry = posY+shipsize*sin((300+angle)*M_PI/180);
	
	//printf("fx: %d  fy: %d   lx: %d   ly: %d   rx: %d   ry: %d\n", fx, fy, lx, ly, rx, ry);
	LCD_DrawLine(fx,fy,lx,ly);
	LCD_DrawLine(lx,ly,rx,ry);
	LCD_DrawLine(rx,ry,fx,fy);
}

void renderShip() {
	unsigned char i;
	unsigned int ang, oldAng;
	unsigned short x, y, oldX, oldY;
	//printf("ang: %d  oldang: %d  posx: %f  oldposx: %f   posy: %f  oldposy: %f\n", ship.shipAngle, ship.oldShipAngle, ship.posX, ship.oldPosX, ship.posY, ship.oldPosY);
	os_mut_wait(mut_ship, 0xffff);
		if ((ship.shipAngle == ship.oldShipAngle) && ((int)(ship.posX) == (int)(ship.oldPosX)) && ((int)(ship.posY) == (int)(ship.oldPosY))) { //no change. Do nothing 
			os_mut_release(mut_ship);
			return;
		}
		ang = ship.shipAngle;
		x = ship.posX;
		y = ship.posY;
		oldAng = ship.oldShipAngle;
		oldX = ship.oldPosX;
		oldY = ship.oldPosY;
		ship.oldShipAngle = ang;
		ship.oldPosX = x;
		ship.oldPosY = y;
	os_mut_release(mut_ship);

	//clear ship
	GLCD_SetTextColor(Black);
	drawShip(oldAng, oldX, oldY);

	//draw ship
	GLCD_SetTextColor(White);
	drawShip(ang, x, y);

}

// ==============================================================================
// 															Startup Screen
// ==============================================================================

//this func is for making a nice little graphic
__task void startScreen() {
	unsigned int i, j;
	os_mut_wait(mut_GLCD, 0xffff);
	os_mut_wait(mut_physics, 0xffff);
	os_mut_wait(mut_ship, 0xffff);
	if (INT0_Get() != 0) {   //skip intro by holding key                    
		ship.posX = 376;
		ship.posY = 672;
		ship.shipAngle = 180;
		
		//set a ship force
		ship.forceX = 0;
		ship.forceY = 0;
		
		//spin ship
		for(i = 0; i < 360/20; i++) {
			ship.shipAngle = (ship.shipAngle+20) % 360;
			drawShip(ship.shipAngle, ship.posX, ship.posY);
			createBullet();
			os_dly_wait(10);
		
		}
		//set bullet health to large so does not decay
		for (j = 0; j < maxBullets; j++) {
			bullets->health[j] = 100000;
		}
		
		//change background colour so redraw does not clear bullets
		_GLCD_Color[BG_COLOR] = Magenta;
		//fire in all directions
		os_dly_wait(40);
		for (i = 0; i < 200; i++) {
			renderBullets();
			bulletPhysics();
		}
		
		//fix changes
		//clear bullet bitVector
		bullets->bitVector = 0;
		_GLCD_Color[BG_COLOR] = Black;
		
		ship.forceX = 0;
		ship.forceY = 0;
		
		GLCD_DisplayString(1,1,1,"-------------");
		GLCD_DisplayString(2,1,1,"| ASTEROIDS |");
		GLCD_DisplayString(3,1,1,"-------------");
		GLCD_DisplayString(5,0,1,"By Chris Gravel");
		
		os_dly_wait(50);

		GLCD_DisplayString(8,0,1,"  PRESS START  ");
		
		while (1) {                                 /* endless loop                */
			if (INT0_Get() == 0) {                    /* if key pressed              */
				break;											
			}
			//do not let another thread interrupt
		}
		GLCD_Clear(Black);                        /* Clear the GLCD                */
	
		//put ship back to start position
		ship.shipAngle = 180;
		ship.posX = 376;
		ship.posY = 672;
	}
	
	os_mut_release(mut_ship);
	os_mut_release(mut_physics);
	os_mut_release(mut_GLCD);
	os_tsk_delete_self();

}

void bombEffect(){
	char i;
	for(i = 0; i < 2; i++) {
		GLCD_Clear(Red);
		os_dly_wait(2);
		GLCD_Clear(Black);
		os_dly_wait(2);
	}
}

__task void GameOver(){
	char i,j;
	char score[10];
	float increY, increX;
	HUD.reset = 0;
	os_mut_wait(mut_GLCD, 0xffff);
	os_mut_wait(mut_physics, 0xffff);
	sprintf(score, "%d", HUD.score);
	//draw horizontal rows
	increY = SCREENHEIGHT/GameOverLines;
	increX = SCREENWIDTH/GameOverLines;
	for (j = 0; j < 5; j++) {
		for (i = 0; i < GameOverLines; i++) {
			LCD_DrawLine(256, 512+i*increY+j, 496, 512+i*increY+j);
			os_dly_wait(3);
		}
	}
	for (j = 0; j < 5; j++) {
		for (i = 0; i < GameOverLines; i++) {
			LCD_DrawLine(256+i*increX+j, 512, 496+i*increX+j, 832);
			os_dly_wait(3);
		}
	}
	GLCD_DisplayString(1,1,1,"-------------");
	GLCD_DisplayString(2,1,1,"| GAME OVER |");
	GLCD_DisplayString(3,1,1,"-------------");
	GLCD_DisplayString(5,1,1,"Final Score:");
	GLCD_DisplayString(6,1,1,score);

	while(1) {
		os_dly_wait(10);
	}

	os_mut_release(mut_physics);
	os_mut_release(mut_GLCD);
	os_tsk_delete_self();
}