/*----------------------------------------------------------------------------
 * Name:	Entities.c
 * Purpose: Game Object Functions
 * Note(s): 
 *----------------------------------------------------------------------------*/

#include <RTL.h>
#include <math.h>
#include <stdlib.h>
#include "LPC17xx.H"						
#include "Entities.h"
#include "Graphics.h"
#include "MacroDefs.h"
#include "GLCD.h"
#include "Tasks.h"

Ship_t ship;
Bullet_t * bullets;
Asteroid_t * asteroids;
HUD_t HUD;

OS_MUT mut_ship;
OS_MUT mut_bullets;
OS_MUT mut_asteroids;
OS_MUT mut_physics;

unsigned short AsteroidColour[5] = {0xF808,Orange,Yellow,Blue,Green};

void shipThrusters(unsigned char dir) {
	double fx, fy;
	fy = cos(ship.shipAngle*M_PI/180.0);
	fx = -sin(ship.shipAngle*M_PI/180.0);
	if (dir) {
			ship.forceY = fy*thrustpower;
		  ship.forceX = fx*thrustpower;
	} else {
			ship.forceY = -fy*thrustpower;
		  ship.forceX = -fx*thrustpower;
	}
}

void thrustersOff() {
	ship.forceX = 0;
	ship.forceY = 0;
}

void shipBreaks() {
	ship.velX = ship.velX*0.2;
	ship.velY = ship.velY*0.2;
	ship.accelX = ship.accelX*0.6;
	ship.accelY = ship.accelY*0.6;
}

// ==============================================================================
// 															Create Objects
// ==============================================================================

//create a requested number of asteroid objects

void setAsteroidDifficulty(unsigned short maxHealth) {
	os_mut_wait(mut_asteroids, 0xffff);
		asteroids->maxHealth = maxHealth;
		asteroids->chunkSize = maxHealth/healthSizes;
	os_mut_release(mut_asteroids);
}

void createAsteroids(unsigned char count) {
	unsigned char i;
	float x,y, angle, tempAngle;
	
	os_mut_wait(mut_asteroids, 0xffff);
	for (i = 0; i < count; i ++) {
		asteroids->shapeType[i] = (rand() % 3);
		asteroids->oldPosX[i] = 0;
		asteroids->oldPosY[i] = 0;
		
		//place the asteroids away from the center and along a circle with a radius of 120
		tempAngle = angle + (360*((rand() % 65536)/65536.0));
		y = 120*cos(tempAngle*M_PI/180);
		x = -120*sin(tempAngle*M_PI/180);
		
		asteroids->posX[i] = CENTERX + x;
		asteroids->posY[i] = CENTERY + y;
		
		//give asteroids a random direction vector
		asteroids->velX[i] = (rand() % 65536)/65536.0 *asteroidSpeed;
		asteroids->velY[i] = (rand() % 65536)/65536.0 *asteroidSpeed;	
		
		//set width and height for broad phase collision detection
		asteroids->diameter[i] = asteroidSize*2 + asteroidShapeRadiusVariance*2;
		
		asteroids->health[i] = asteroids->maxHealth;
		asteroids->oldHealth[i] = asteroids->maxHealth;
		
		//set bitVector
		util_BitSet(asteroids->bitVector,i);
	}
	os_mut_release(mut_asteroids);
}

void createBullet() {
	unsigned char index;
	unsigned int shipAngle;
	unsigned short shipX, shipY;
	float shipVelX, shipVelY;
	unsigned long bitVector;
	os_mut_wait(mut_bullets, 0xffff);  //protect from collision detection task
		bitVector = ((~(bullets->bitVector)) & bulletMask);
		if (bitVector == 0) {
			os_mut_release(mut_bullets);
			return;
		}
	os_mut_release(mut_bullets);
	//position bullet and take stats from ship

	//The mutexs are prevent from being nested here to prevent deadlock
	os_mut_wait(mut_ship, 0xffff); 
  		shipAngle = ship.shipAngle;
  		shipX = ship.posX;
  		shipY = ship.posY;
  		shipVelX = ship.velX;
  		shipVelY = ship.velY;
  	os_mut_release(mut_ship);	

  	os_mut_wait(mut_bullets, 0xffff);
  		index = util_GetMSB(bitVector);
		//fy = shipsize*cos(angle*M_PI/180)+ship.posY;
		//fx = ship.posX-shipsize*sin(angle*M_PI/180);
		//get direction
		bullets->health[index] = bulletDecayHealth;
		bullets->velX[index] = -shipsize*sin(shipAngle*M_PI/180)*bulletSpeed + shipVelX;
		bullets->velY[index] = shipsize*cos(shipAngle*M_PI/180)*bulletSpeed + shipVelY;
		//put at tip of ship

		bullets->posX[index] = shipX-shipsize*sin(shipAngle*M_PI/180);
		bullets->posY[index] = shipsize*cos(shipAngle*M_PI/180)+shipY;
		
		//bullets->posX[index] = ship.posX-(bullets->dirVectorX[index]);
		//bullets->posY[index] = (bullets->dirVectorY[index])+ship.posY;
		bullets->oldPosX[index] = bullets->posX[index];
		bullets->oldPosY[index] = bullets->posY[index];
		util_BitSet(bullets->bitVector, index);
	os_mut_release(mut_bullets);
}

void createShip() {
	ship.shipAngle = 180;  
	ship.oldShipAngle = 180; 
	ship.posX = 376;
	ship.posY = 672;
	ship.oldPosX = 50;
	ship.oldPosY = 50;
	ship.accelX = 0;
	ship.accelY = 0;
	ship.velX = 0;
	ship.velY = 0;
	ship.forceX = 0;
	ship.forceY = 0;
}

// ==============================================================================
// 															   Physics
// ==============================================================================

void launchBomb() {
	char index;
	unsigned long bitVector;
	HUD.bombs--;
	os_mut_wait(mut_physics, 0xffff);
	os_mut_wait(mut_GLCD, 0xffff);
	bombEffect();
	os_mut_release(mut_GLCD);
	//do animation
	os_mut_wait(mut_asteroids, 0xffff);
	bitVector = asteroids->bitVector;
	//this algorithm just checks whether the bullet is inside the region of the asteroid. Broad phase and then narrow phase.	
	while(bitVector) {
		index = util_GetMSB(bitVector);	
		asteroids->oldHealth[index] = asteroids->health[index];
		asteroids->health[index] -= bombDamage;
		if (asteroids->health[index] < 0) {
			util_BitClear(asteroids->bitVector, index);
			util_BitSet(asteroids->undrawBitVector, index);
			HUD.score += asteroidKillPoints*(1+(HUD.level-1)*0.2);
		}
		
		util_BitClear(bitVector, index);
	}

	os_mut_release(mut_asteroids);
	os_mut_release(mut_physics);	
}

// "left", the x coordinate of its left side,
// "top", the y coordinate of its top side,
// "right", the x coordinate of its right side,
// "bottom", the y coordinate of its bottom side,

// function IntersectRect(r1:Rectangle, r2:Rectangle):Boolean {
//	 return !(r2.left > r1.right
//		 || r2.right < r1.left
//		 || r2.top > r1.bottom
//		 || r2.bottom < r1.top);
// } 	

// bool DoBoxesIntersect(Box a, Box b) {
//   return (abs(a.x - b.x) * 2 < (a.width + b.width)) &&
//		  (abs(a.y - b.y) * 2 < (a.height + b.height));

/*
var circle1 = {radius: 20, x: 5, y: 5};
var circle2 = {radius: 12, x: 10, y: 5};

var dx = circle1.x - circle2.x;
var dy = circle1.y - circle2.y;
var distance = Math.sqrt(dx * dx + dy * dy);

if (distance < circle1.radius + circle2.radius) {
	// collision detected!
}*/

/*
line intersection Ref wikipedia
(x1-x2)*(y3-y4) - (y1-y2)*(x3-x4) == 0
check that (y2-y1)/(x2-x1) != (y4-y3)/(x4-x3)



*/
// }

// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
// intersect the intersection point may be stored in the floats i_x and i_y.

//Line in polygon test credit: W Randolph Franklin, Prof, Rensselaer Polytechnic Institute @ https://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
int pnpoly(int nvert, float * vertx, float * verty, float testx, float testy)
{
  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
	if ( ((verty[i]>testy) != (verty[j]>testy)) &&
	 (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
	   c = !c;
  }
  return c;
}

//credit: Gavin @ http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
char get_line_intersection(int p0_x, int p0_y, int p1_x, int p1_y, 
	int p2_x, int p2_y, int p3_x, int p3_y)
{
	float s1_x, s1_y, s2_x, s2_y;
		float s, t;
	s1_x = p1_x - p0_x;	 s1_y = p1_y - p0_y;
	s2_x = p3_x - p2_x;	 s2_y = p3_y - p2_y;

	s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
		return 1;
	}

	return 0; // No collision
}

void collisionPhysics() {
	unsigned char index, indexBullet, i, j, healthLevel;
	unsigned long asterBitVector, bulletBitVector;
	unsigned int tempCalc; 
	unsigned short asterx[2], astery[2], shipx[3], shipy[3];
	float ux, uy, dist;
	float asterVx[shapeResolution], asterVy[shapeResolution];
	
	//detect collion between asteroid and ship
	os_mut_wait(mut_asteroids, 0xffff);
	os_mut_wait(mut_ship, 0xffff);
	
	//get points of ship for narrow phase detection
	shipx[0] = ship.posX+shipsize*cos((300+ship.shipAngle)*M_PI/180);
	shipy[0] = ship.posY+shipsize*sin((300+ship.shipAngle)*M_PI/180);
	shipx[1] = ship.posX-shipsize*sin(ship.shipAngle*M_PI/180);
	shipy[1] = shipsize*cos(ship.shipAngle*M_PI/180)+ship.posY;
	shipx[2] = ship.posX+shipsize*cos((240+ship.shipAngle)*M_PI/180);
	shipy[2] = ship.posY+shipsize*sin((240+ship.shipAngle)*M_PI/180);
	asterBitVector = asteroids->bitVector;
	
	// while(0) {
	while(asterBitVector) {
		index = util_GetMSB(asterBitVector);
		healthLevel = (asteroids->health[index]-1)/asteroids->chunkSize;
		tempCalc = (2*shipsize + asteroids->diameter[index])*(2*shipsize + asteroids->diameter[index]);
		//broad phase	simple bounding boxes  Credit: ZorbaTHut @ http://gamedev.stackexchange.com/a/587
		if (((ship.posX-asteroids->posX[index])*(ship.posX-asteroids->posX[index])*4) < (tempCalc) && \
			((ship.posY-asteroids->posY[index])*(ship.posY-asteroids->posY[index])*4) < (tempCalc)) {
				//narrow phase  --  loop through all points in the asteroid and see if any of them intersect with the ship
				for(j = 0; j < 3; j++) {				//for each point of the triangle
					for(i = 0; i < shapeResolution; i++) {
						//This is to clean it up. It's really ugly otherwise
						asterx[0] = asteroids->posX[index] - asteroids->shapes[asteroids->shapeType[index]][i][healthLevel][0];
						astery[0] = asteroids->posY[index] + asteroids->shapes[asteroids->shapeType[index]][i][healthLevel][1];
						asterx[1] = asteroids->posX[index] - asteroids->shapes[asteroids->shapeType[index]][(i+1) % shapeResolution][healthLevel][0];
						astery[1] = asteroids->posY[index] + asteroids->shapes[asteroids->shapeType[index]][(i+1) % shapeResolution][healthLevel][1];
						if (get_line_intersection(asterx[0], astery[0], asterx[1], astery[1], shipx[j], shipy[j], shipx[(j+1)%3], shipy[(j+1)%3])) {
							HUD.lives -= 1;
							HUD.reset = 1; //signal for stage reset
							os_mut_release(mut_ship);
							os_mut_release(mut_asteroids);
							if (HUD.lives < 0) {
								os_mut_release(mut_physics);
								// os_sem_wait(&sem_reset, 0xffff); //wait for stage task to reset level
							}
							os_sem_wait(&sem_reset, 0xffff); //wait for stage task to reset level
							return; //stage monitor will check for this
						}
					}
				}
		}
		//mark as checked
		util_BitClear(asterBitVector, index);
	}
	os_mut_release(mut_ship);
	//don't release asteroids mutex just yet. Need to check collision with bullets
	
 	os_mut_wait(mut_bullets, 0xffff);
  	// bulletBitVector = bullets->bitVector;
	asterBitVector = asteroids->bitVector;
	
	//this algorithm just checks whether the bullet is inside the region of the asteroid. Broad phase and then narrow phase.
	while(asterBitVector) {

		index = util_GetMSB(asterBitVector);	
		healthLevel = (asteroids->health[index]-1)/asteroids->chunkSize;
		bulletBitVector = bullets->bitVector;
		while(bulletBitVector) {
			indexBullet = util_GetMSB(bulletBitVector);
			tempCalc = (2*bulletSize + asteroids->diameter[index])*(2*bulletSize + asteroids->diameter[index]);
			//broad phase -- bounding boxes
			if (((asteroids->posX[index]-bullets->posX[indexBullet])*(asteroids->posX[index]-bullets->posX[indexBullet])*4) < (tempCalc) && \
				((asteroids->posY[index]-bullets->posY[indexBullet])*(asteroids->posY[index]-bullets->posY[indexBullet])*4) < (tempCalc)) {
				//narrow phase -- point in convex polygon inclusion test
				//build vertex array
				for(i = 0; i < shapeResolution; i++) {
					asterVx[i] = asteroids->posX[index] - asteroids->shapes[asteroids->shapeType[index]][i][healthLevel][0];
					asterVy[i] = asteroids->posY[index] + asteroids->shapes[asteroids->shapeType[index]][i][healthLevel][1];
				}	
				tempCalc = pnpoly(shapeResolution, (float *)(&asterVx), (float *)(&asterVy), (float)(bullets->posX[indexBullet]), (float)(bullets->posY[indexBullet]));
				if (tempCalc != 0) {
				//bullet collision. Award points. Decrease health. Impart momentum. 
					HUD.score += bulletCollisionPoints*(1+(HUD.level-1)*0.2);
					util_BitClear(bullets->bitVector, indexBullet); //destroy bullet
					util_BitSet(bullets->undrawBitVector, indexBullet); //mark for undrawing

					asteroids->health[index] -= bulletDamage;
					//TODO verify correctness of mapping diameter to health
						// k = health/asteroids->chunkSize - 1;
						// tempRadius = asteroidSize - (k * healthToRadius)
					j = (asteroids->health[index]-1)/asteroids->chunkSize;
					asteroids->diameter[index] = (asteroidSize - (j * healthToRadius) + asteroidShapeRadiusVariance)*2;
					if (asteroids->health[index] < 0) {
						util_BitClear(asteroids->bitVector, index);
						util_BitSet(asteroids->undrawBitVector, index);
						HUD.score += asteroidKillPoints*(1+(HUD.level-1)*0.2);
					} else {
		            	dist = sqrt((bullets->posX[indexBullet] - asteroids->posX[index])*(bullets->posX[indexBullet] - asteroids->posX[index]) + \
						(bullets->posY[indexBullet] - asteroids->posY[index])*(bullets->posY[indexBullet] - asteroids->posY[index]));
						//unit vector for direction to impart
						ux = (asteroids->posX[index]-bullets->posX[indexBullet])/dist;
						uy = (asteroids->posY[index]-bullets->posY[indexBullet])/dist;

						//TODO Impart momentum more realistically
						asteroids->velX[index] += ux*bulletMass;
						asteroids->velY[index] += uy*bulletMass;
					}

				}
			}
			util_BitClear(bulletBitVector, indexBullet);
		}
		util_BitClear(asterBitVector, index);
	}
	os_mut_release(mut_bullets);
	os_mut_release(mut_asteroids);
}

void asteroidPhysics() {
	unsigned char index;
	unsigned long bitVector;

	os_mut_wait(mut_asteroids, 0xffff); {
		bitVector = asteroids->bitVector;
		while(bitVector) {
			index = util_GetMSB(bitVector);
			
			asteroids->posX[index] += deltaTime * asteroids->velX[index];
			asteroids->posY[index] += deltaTime * asteroids->velY[index];
			
			applyXBounds(asteroids->posX[index]);
			applyYBounds(asteroids->posY[index]);
			//mark as drawn
			util_BitClear(bitVector, index);
		}
	} os_mut_release(mut_asteroids);
}

void bulletPhysics() {
	unsigned char index;
	unsigned long bitVector;

	os_mut_wait(mut_bullets, 0xffff); {
		bitVector = bullets->bitVector;
		while(bitVector) {
			index = util_GetMSB(bitVector);
			
			bullets->health[index] -= 1;
			if (bullets->health[index] < 1) {
				//bullet has decayed. Unset bitVector
				util_BitClear(bullets->bitVector, index);
				util_BitSet(bullets->undrawBitVector, index);
			} else {
				bullets->posX[index] += deltaTime * bullets->velX[index];
				bullets->posY[index] += deltaTime * bullets->velY[index];
				
				applyXBounds(bullets->posX[index]);
				applyYBounds(bullets->posY[index]);				
			}
			//mark as drawn
			util_BitClear(bitVector, index);
		}
	} os_mut_release(mut_bullets);
	
}

void shipPhysics() {
	double dragX, dragY;
	ship.posX += deltaTime * ship.velX;
	ship.posY += deltaTime * ship.velY;
	
	applyXBounds(ship.posX);
	applyYBounds(ship.posY);
	
	ship.velX += deltaTime * ship.accelX;
	ship.velY += deltaTime * ship.accelY;
	
	if (ship.velX*ship.velX < stopThresh) 
		ship.velX = 0;
	
	if (ship.velY*ship.velY < stopThresh)
		ship.velY = 0;
	
	ship.velX = ship.velX*velocityDecay;
	ship.velY = ship.velY*velocityDecay;
	
	//drag and thrusters
	dragX = shipdrag * ship.velX * ship.velX * (1-2*(ship.velX < 0));
	dragY = shipdrag * ship.velY * ship.velY * (1-2*(ship.velY < 0));
	
	//prevent oscillation due to numerical integration method
	if ((dragX*dragX)/shipmass > (ship.velX*ship.velX))
		dragX = ship.velX*shipmass;
	if ((dragY*dragY)/shipmass > (ship.velY*ship.velY))
		dragY = ship.velY*shipmass;
	
	ship.accelX = (ship.forceX-dragX)/shipmass;//(ship.forceX - 0.5*ship.velX*ship.velX)/shipmass;
	ship.accelY = (ship.forceY-dragY)/shipmass;//(ship.forceY - 0.5*ship.velY*ship.velY)/shipmass;
	
	//decay
	ship.accelX = ship.accelX*accelDecay;
	ship.accelY = ship.accelY*accelDecay;
	
	//printf("velx: %f  vely: %f  ax: %f  ay: %f  fx: %f  fy: %f\n", ship.velX, ship.velY, ship.accelX, ship.accelY, ship.forceX, ship.forceY);
	//printf("velx: %f  vely: %f  ax: %f  ay: %f  dragX: %f  dragY: %f\n", ship.velX, ship.velY, ship.accelX, ship.accelY, dragX, dragY);
}

// ==============================================================================
// 															Initialize
// ==============================================================================

void initializeHUD() {
	HUD.reset = 0;
	HUD.level = 0;
	HUD.score = 0;
	HUD.lives = startingLives;
	HUD.bombs = startingBombs;
}

void initializeBullets() {
	unsigned char i;
	
	//initialize memory to 0
	bullets->bitVector = 0;
	for (i = 0; i < maxBullets; i ++) {
		bullets->oldPosX[i] = 0;
		bullets->oldPosY[i] = 0;
		bullets->posX[i] = 0;
		bullets->posY[i] = 0;
		bullets->velX[i] = 0;
		bullets->velY[i] = 0;
	}
}

void initializeAsteroids() {
	unsigned char i, j, k;
	float x,y, angle, tempRadius, tempAngle;

	//initialize memory to 0
	asteroids->count = 0;
	asteroids->bitVector = 0;
	for (i = 0; i < maxAsteroids; i ++) {
		asteroids->shapeType[i] = (rand() % 3);//randr(0,numShapeTypes);
		asteroids->oldPosX[i] = 0;
		asteroids->oldPosY[i] = 0;
		asteroids->posX[i] = ((rand() % 256)/256.0)*240+256;
		asteroids->posY[i] = ((rand() % 256)/256.0)*320+512;
		asteroids->velX[i] = 0;
		asteroids->velY[i] = 0;	
		asteroids->diameter[i] = asteroidSize*2;
	}


	//TODO generate 5 radius sizes for each.
	//[numShapeTypes][shapeResolution][2]
	//randomly generate 3 shapes
	for(i = 0; i < numShapeTypes; i++) {
		angle = 0;
		for(j = 0; j < shapeResolution; j++) {
			// create points around a circle by randomly varying radius and angle. Use a threshold for the variance of each
			//tempRadius = asteroidSize - (k * healthToRadius) + (asteroidShapeRadiusVariance*((rand() % 65536)/65536.0));
			tempRadius = asteroidSize + (asteroidShapeRadiusVariance*((rand() % 65536)/65536.0));
			tempAngle = angle + (asteroidShapeAngleVariance * ((rand() % 65536)/65536.0));
			angle += 360/(shapeResolution+1);
			for(k = 0; k < healthSizes; k++) {
				tempRadius -= (k * healthToRadius);
				y = tempRadius*cos(tempAngle*M_PI/180);
				x = -tempRadius*sin(tempAngle*M_PI/180);
				asteroids->shapes[i][j][healthSizes-k-1][0] = x;
				asteroids->shapes[i][j][healthSizes-k-1][1] = y;
			}
		}				
	}
}

void initializeEntities() {
	//allocate memory in here
	bullets = malloc(sizeof(Bullet_t));
	asteroids = malloc(sizeof(Asteroid_t));
// 	os_mut_wait(mut_ship, 0xffff);
// 	os_mut_wait(mut_bullets, 0xffff);
// 	os_mut_wait(mut_asteroids, 0xffff);
	createShip();
	initializeBullets();
	initializeAsteroids();
	initializeHUD();
// 	os_mut_release(mut_asteroids);
// 	os_mut_release(mut_bullets);
// 	os_mut_release(mut_ship);
}

//called for resetting the game
void resetEntities() {
	os_mut_wait(mut_ship, 0xffff);
	os_mut_wait(mut_bullets, 0xffff);
	os_mut_wait(mut_asteroids, 0xffff);	
	createShip();
	initializeBullets();
	initializeAsteroids();
	os_mut_release(mut_asteroids);
	os_mut_release(mut_bullets);
	os_mut_release(mut_ship);
}