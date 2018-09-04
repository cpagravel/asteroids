#include <RTL.h>
#include "LPC17xx.H"          /* LPC17xx definitions         */
#include "GLCD.h"
#include "Entities.h"
#include "Graphics.h"
#include "Controls.h"
#include "Tasks.h"


//These are not quint essential tasks. Can tweak this later to get smoother performance
#define ADC_update_delay     2
#define moveShip_check_delay   5
#define joystick_update_delay  3
#define bomb_button_delay    3

//attempt to standardize the frame rate
#define lcd_frame_delay      5

//control how fast objects move with the physics delay
#define physics_delay      1


#undef __SS   //undef to enable start screen. define to disable.

OS_TID t_phys;    
OS_TID t_adc;     
OS_TID t_jst;     
OS_TID t_gunctrl;   
OS_TID t_movectrl;  
OS_TID t_bombctrl;  
OS_TID t_lcd;     
OS_TID t_stage;   
OS_TID t_intro;

OS_MUT mut_GLCD;
OS_MUT mut_levelStart;

OS_SEM sem_reset;

// ==============================================================================
//                                 ADC INPUT
// ==============================================================================
/*NOTE: You need to complete this function*/
__task void adc (void) {
  unsigned short tempAngle;
  while(1)
  {
    ADC_ConversionStart();
    //os_tsk_pass();
    tempAngle = ((4096-ADCValue) * 360) >> 12;
    if (((tempAngle+3) - ship.shipAngle) > 3) { //tolerance
    os_mut_wait(mut_ship, 0xffff);
    ship.shipAngle = tempAngle;
    os_mut_release(mut_ship);
    }
  os_dly_wait(ADC_update_delay);
  }
}

// ==============================================================================
//                                 USER INPUT
// ==============================================================================
// Soul purpose is to read joystick sensor and store in global
__task void joystick (void) {
  for (;;) {
    KBD_Read();
    //os_tsk_pass();
    os_dly_wait(joystick_update_delay);
  }
}

// This is a separate task because it is the best way to ensure the firing rate isn't too fast or slow
__task void shipGun (void) {
  while(1) {
    if (KBD_Left()) {
      shoot();
      while (KBD_Left()) {  //no automatic fire. Force user to press button for bullets
        os_dly_wait(firingDelay);
      }
    } else if(KBD_Right()) {
      shoot();
      while (KBD_Right()) {  //no automatic fire. Force user to press button for bullets
        os_dly_wait(firingDelay);
      }
    }
    os_dly_wait(firingDelay);
  }
}

__task void moveShip (void) {
  //unsigned short tempAngle;
  while(1) {
    if (KBD_Up()) {
      while(KBD_Up()) {  //loop is to maintain smooth thrust
        shipThrusters(1);
        os_tsk_pass();
      }
    } else if (KBD_Down()) {
      while(KBD_Down()) {  //loop is to maintain smooth thrust
        shipThrusters(0);
        os_tsk_pass();
      }
    } 
    thrustersOff();
    os_dly_wait(moveShip_check_delay);
  }
}

__task void bomb (void) {
  os_mut_wait(mut_physics,0xffff);
  os_mut_release(mut_physics);
  while(1) {
    if(HUD.bombs < 1) {
      os_dly_wait(50); 
    } else if (INT0_Get() == 0) {
      launchBomb();
      os_dly_wait(bomb_button_delay);
    }
    os_dly_wait(bomb_button_delay);
  }
}

// ==============================================================================
//                                 DISPLAY
// ==============================================================================
__task void lcd (void) {
  
  while(1) {
    os_mut_wait(mut_GLCD, 0xffff);
    renderShip();
    renderBullets();
    renderAsteroids();
    renderHUD();

    os_mut_release(mut_GLCD);
    //os_tsk_pass();
    os_dly_wait(lcd_frame_delay);
  }
  os_tsk_delete_self ();
}


// ==============================================================================
//                                 PHYSICS
// ==============================================================================
__task void physics (void) {
  
  while(1) {
    os_mut_wait(mut_physics, 0xffff);
    shipPhysics();
    bulletPhysics();
    asteroidPhysics();
    collisionPhysics();
    //os_tsk_pass();
    os_mut_release(mut_physics);
    os_dly_wait(physics_delay);
  }
  os_mut_release(mut_physics);
  os_tsk_delete_self();
}

// ==============================================================================
//                                 LEVEL SETUP
// ==============================================================================

__task void stageMonitor(void) {
  unsigned char asteroidCount;
  HUD.level = 1;
  os_mut_wait(mut_levelStart, 0xffff); //mutex is set from start screen
  while(1) {  
    //set difficulty of level
    asteroidCount = HUD.level*2;
    if (asteroidCount > maxAsteroids)
      asteroidCount = maxAsteroids;
    setAsteroidDifficulty(100 + HUD.level*20);
  
    os_mut_wait(mut_asteroids, 0xffff);
    createAsteroids(asteroidCount);
    os_mut_release(mut_asteroids);
    
    //place ship
    os_mut_wait(mut_ship, 0xffff);
    ship.shipAngle = 180;
    ship.posX = 376;
    ship.posY = 672;
    os_mut_release(mut_ship);
  
    //while there are asteroids remaining
    while((asteroids->bitVector != 0) && (HUD.lives >= 0) && (HUD.reset == 0)) {
      os_dly_wait(50);
    }
    if (HUD.lives < 0) {
      //game over
      os_tsk_create (GameOver, 254);
    } else if (HUD.reset == 1) {
      //reset entities. Maintain HUD data
      resetEntities();

      //clear screen
      os_mut_wait(mut_GLCD, 0xffff);
      GLCD_Clear(White);
      os_dly_wait(2);
      GLCD_Clear(Black);
      os_dly_wait(20);
      os_mut_wait(mut_ship, 0xffff);
      ship.oldShipAngle = ship.shipAngle + 1; //force ship redraw
      os_mut_release(mut_ship);
      os_mut_release(mut_GLCD);

      HUD.reset = 0;
      os_sem_send(&sem_reset);
    } else {
      os_mut_wait(mut_GLCD, 0xffff);
      GLCD_Clear(Black);
      os_dly_wait(2);
      os_mut_wait(mut_ship, 0xffff);
      ship.oldShipAngle = ship.shipAngle + 1; //force ship redraw
      os_mut_release(mut_ship);
      os_mut_release(mut_GLCD);
      HUD.level++; //not bothering with MUTEX. Unlikely to be a problem
    }
  }
  os_tsk_delete_self();
}



// ==============================================================================
//                                 INITIALIZE TASKS
// ==============================================================================
__task void init (void) {

  os_mut_init(mut_ship);
  os_mut_init(mut_bullets);
  os_mut_init(mut_asteroids);
  os_mut_init(mut_physics);
  os_mut_init(mut_GLCD);
  os_mut_init(mut_levelStart);

  os_sem_init(&sem_reset, 0);

  t_jst     = os_tsk_create (joystick, 1); // Updates global variable for joystick sensor
  t_gunctrl   = os_tsk_create (shipGun, 1);  // Controls rate of fire for gun
  t_adc     = os_tsk_create (adc, 1);     // Updates global variable for ADC 
  t_movectrl  = os_tsk_create (moveShip, 1); // Monitors joystick value for movement. Monitors ADC value for rotation.
  t_bombctrl  = os_tsk_create (bomb, 1);   // Monitors joystick value for movement. Monitors ADC value for rotation.
  t_lcd     = os_tsk_create (lcd, 3);    // Display task. Does all rendering   
  
  //level setup
  t_stage     = os_tsk_create (stageMonitor, 0);  // Performs physics on all objects. Includes collision detection

  #ifndef __SS
  #define __SS
  t_intro     = os_tsk_create (startScreen, 254); //startup screen intro
  #endif

  t_phys     = os_tsk_create (physics, 3);  // Performs physics on all objects. Includes collision detection

  os_tsk_delete_self ();
}

void initializeTasks() {
  os_sys_init(init);            /* Initialize RTX and start init */
}



/*
The stack stk must be aligned at an 8-byte boundary and must be declared as an array of type U64 (unsigned long long).
The default stack size is defined in rtx_config.c.
*/

/*
OS_TID os_tsk_create_user_ex (
  void (*task)(void *),  // Task to create 
  U8  priority,      // Task priority (1-254) 
  void* stk,         // Pointer to the task's stack 
  U16   size,        // Size of stack in bytes 
  void* argv );      // Argument to the task 
*/
