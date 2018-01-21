#include <RTL.h>
#include <stdio.h>
#include "LPC17xx.H"                    /* LPC17xx definitions               */
#include "GLCD.h"
#include "Entities.h"
#include "Graphics.h"
#include "Controls.h"
#include "Tasks.h"

#define __FI        1                   /* Font index 16x24                  */


/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/
int main (void) {
	printf("STARTED\n");
	initializeEntities();
	initializeControls();
	initializeTasks();
}
