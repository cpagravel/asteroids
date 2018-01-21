/*----------------------------------------------------------------------------
 * Name:    Entities.h
 * Purpose: Game Object Definitions
 * Note(s): 
 *--------------------------------------------------------------------------*/
 
#ifndef _Entities_H
#define _Entities_H

#define shoot()  	createBullet()

#define lineIntersect(x1,y1,x2,y2,x3,y3,x4,y4)  ((((x1)-(x2))*((y3)-(y4)) - ((y1)-(y2))*((x3)-(x4))) == 0)
#define lineTest(x1,y1,x2,y2,x3,y3,x4,y4)  (((x1)-(x2))*((y3)-(y4)) - ((y1)-(y2))*((x3)-(x4)))

#define applyLowBoundX(x)  (x) = ((x) < 256) ? ((x) + 240) : (x)
#define applyHighBoundX(x)  (x) = ((x) > 496) ? ((x) - 240) : (x)
#define applyLowBoundY(y)  (y) = ((y) < 512) ? ((y) + 320) : (y)
#define applyHighBoundY(y)  (y) = ((y) > 832) ? ((y) - 320) : (y)

#define applyXBounds(x) applyLowBoundX(x); applyHighBoundX(x)
#define applyYBounds(y) applyLowBoundY(y); applyHighBoundY(y)

#define thrustpower 35
#define shipmass 150
#define shipsize 20
#define shipdrag 2
#define deltaTime 1
#define stopThresh 0.001
#define velocityDecay 0.98
#define accelDecay 0.98

typedef struct Ship {
	unsigned int shipAngle;
	unsigned int oldShipAngle;
	float posX;
	float posY;
	float oldPosX;
	float oldPosY;
	float forceX;
	float forceY;
	float accelX;
	float accelY;
	float velX;
	float velY;
} Ship_t;

#define bombDamage 1000
#define startingLives 3
#define startingBombs 2
#define asteroidKillPoints 30
#define bulletCollisionPoints 4

typedef struct HUDdisplay {
	unsigned short level; //keep track of level
	unsigned int score;
	signed short lives;
	unsigned short bombs;
	unsigned char reset;
} HUD_t;

#define maxBullets 18
#define bulletSize 2
#define bulletMass 0.5  //imparted into asteroids
#define bulletMask ((1 << maxBullets) - 1)
#define bulletSpeed 0.1
#define bulletDamage 100
#define firingDelay 5
#define bulletDecayHealth 200

typedef struct Bullets {
	unsigned long bitVector;
	unsigned long undrawBitVector;
	float oldPosX[maxBullets];
	float oldPosY[maxBullets];
	float posX[maxBullets];
	float posY[maxBullets];
	float velX[maxBullets];
	float velY[maxBullets];
	float health[maxBullets];
} Bullet_t;

#define maxAsteroids 3
#define asteroidMask ((1 << maxAsteroids) - 1)
#define asteroidSize 40
#define asteroidMinSize 15
#define asteroidSpeed 0.2

#define numShapeTypes 3
#define shapeResolution 8
#define healthSizes 5    	//number of radius sizes which correspond to health
#define healthToRadius 2    //relationship between a health level and radius change

//used for randomly constructing asteroid shapes
#define asteroidShapeAngleVariance 20
#define asteroidShapeRadiusVariance 10

typedef struct Asteroids {
	unsigned char count;
	unsigned long bitVector;
	unsigned long undrawBitVector;
	unsigned char shapeType[maxAsteroids];
	unsigned char diameter[maxAsteroids]; 		 //for collision detection	
	signed short health[maxAsteroids];
	signed short oldHealth[maxAsteroids];
	signed short maxHealth; 					 //influences difficulty
	float chunkSize;       				//size of health chunk. Calculated once every level.
	float oldPosX[maxAsteroids];
	float oldPosY[maxAsteroids];
	float posX[maxAsteroids];
	float posY[maxAsteroids];
	float velX[maxAsteroids];
	float velY[maxAsteroids];
	
	signed short shapes[numShapeTypes][shapeResolution][healthSizes][2]; //3 shapes; each with 8 points; each with 2 rel coords.
} Asteroid_t;

extern Ship_t ship;
extern Asteroid_t * asteroids;
extern Bullet_t * bullets;
extern HUD_t HUD;

extern unsigned short AsteroidColour[5];

extern OS_MUT mut_ship; 
extern OS_MUT mut_bullets; 
extern OS_MUT mut_asteroids; 
extern OS_MUT mut_physics;
extern OS_MUT mut_levelStart;

extern void launchBomb(void);

extern void setAsteroidDifficulty(unsigned short maxHealth);

extern void createBullet(void);
extern void createAsteroids(unsigned char count);

extern void collisionPhysics(void);
extern void asteroidPhysics(void);
extern void bulletPhysics(void);
extern void shipPhysics(void);
extern void shipThrusters(unsigned char dir);
extern void shipBreaks(void);
extern void thrustersOff(void);
extern void resetEntities(void);
extern void initializeEntities(void);


#endif // Entities_H
 