
#include <RTL.h>
//#include <stdio.h>
#include "LPC17xx.H"                    /* LPC17xx definitions               */
#include "GLCD.h"
#include "Entities.h"
#include "Graphics.h"
#include "Controls.h"

// ==========================================================================================================================================
// =========================================================================  ADC  ==========================================================
// ==========================================================================================================================================

unsigned int ADCStat = 0;
unsigned int ADCValue = 0;

/*----------------------------------------------------------------------------
  initialize ADC Pins
 *----------------------------------------------------------------------------*/
void ADC_Init (void) {
	
  LPC_SC->PCONP     |= (1 << 12);            /* enable power to GPIO & IOCON  */

	//See table 81, setting GPIO Port 0.25 to AD0.2 mode
  LPC_PINCON->PINSEL1 &= ~( 0x3 << 18 ); //clear bits
	LPC_PINCON->PINSEL1 |=  ( 0x1 << 18 ); //set bits
	
	LPC_ADC->ADCR = ( 1 <<  2 ) | // Select the second channel
									( 4 <<  8 ) | // ADC clock is 25MHz/(4+1)
									( 0 << 24 ) | // Do not start the conversion yet
									( 1 << 21 );  // Enable ADC

	LPC_ADC->ADINTEN = ( 1 <<  8);  //Enable interrupts for all ADC channels	
	
	LPC_ADC->ADCR  = (LPC_ADC->ADCR  & 0xFFFFFF00) | (0x01 << 2 );		// set ADC0[2] for sampling
}


/*----------------------------------------------------------------------------
  ADC Interrupt Handler Routine
 *----------------------------------------------------------------------------*/
void ADC_IRQHandler( void ) 
{
	// Read ADC Status clears the interrupt condition
	ADCStat = LPC_ADC->ADSTAT;
	ADCValue = (LPC_ADC->ADGDR >> 4) & 0xFFF;
}

/*----------------------------------------------------------------------------
  Start ADC conversion 
 *----------------------------------------------------------------------------*/
/*NOTE: You need to write this funciton if using the ADC */
void ADC_ConversionStart (void )
{
	util_BitSet(LPC_ADC->ADCR, SBIT_START);			//Start ADC conversion	
}

// ==========================================================================================================================================
// =========================================================================  KBD  ==========================================================
// ==========================================================================================================================================


uint32_t KBD_val  = 0;             
uint32_t INT0_val = 0;             

/*----------------------------------------------------------------------------
  initialize LED Pins
 *----------------------------------------------------------------------------*/
void KBD_Init (void) {

  LPC_PINCON->PINSEL4 &= ~(3<<20);           /* P2.10 is GPIO (INT0)          */
  LPC_GPIO2->FIODIR   &= ~(1<<10);           /* P2.10 is input                */

  LPC_PINCON->PINSEL3 &= ~((3<< 8)|(3<<14)|(3<<16)|(3<<18)|(3<<20)); /* P1.20, P1.23..26 is GPIO (Joystick) */
  LPC_GPIO1->FIODIR   &= ~((1<<20)|(1<<23)|(1<<24)|(1<<25)|(1<<26)); /* P1.20, P1.23..26 is input           */
}


/*----------------------------------------------------------------------------
  Get Joystick value
 *----------------------------------------------------------------------------*/
uint32_t KBD_Get  (void) {
  uint32_t kbd_val;

  kbd_val = (LPC_GPIO1->FIOPIN >> 20) & KBD_MASK;
  return (kbd_val);
}

void KBD_Read (void) {
		KBD_val = KBD_Get();
}

/*----------------------------------------------------------------------------
  Get INT0 value
 *----------------------------------------------------------------------------*/
uint32_t INT0_Get  (void) {
  uint32_t int0_val;

  int0_val = (LPC_GPIO2->FIOPIN >> 10) & 0x01;
  return (int0_val);
}


// ==========================================================================================================================================
// =========================================================================  LED  ==========================================================
// ==========================================================================================================================================

const unsigned long led_mask[] = { 1UL<<28, 1UL<<29, 1UL<<31, 1UL<< 2,
                                   1UL<< 3, 1UL<< 4, 1UL<< 5, 1UL<< 6 };


/*----------------------------------------------------------------------------
  initialize LED Pins
 *----------------------------------------------------------------------------*/
void LED_Init (void) {
	
  LPC_SC->PCONP     |= (1 << 15);            /* enable power to GPIO & IOCON  */

  LPC_GPIO1->FIODIR |= 0xB0000000;           /* LEDs on PORT1 are output      */
  LPC_GPIO2->FIODIR |= 0x0000007C;           /* LEDs on PORT2 are output      */
	
}

/*----------------------------------------------------------------------------
  Function that turns on requested LED
 *----------------------------------------------------------------------------*/
void LED_On (unsigned int num) {
  if (num < 3) LPC_GPIO1->FIOPIN |=  led_mask[num];  //Notice that LEDs are on two ports
  else         LPC_GPIO2->FIOPIN |=  led_mask[num];  //Need to handle each on separately
}

/*----------------------------------------------------------------------------
  Function that turns off requested LED
 *----------------------------------------------------------------------------*/
void LED_Off (unsigned int num) {

  if (num < 3) LPC_GPIO1->FIOPIN &= ~led_mask[num];
  else         LPC_GPIO2->FIOPIN &= ~led_mask[num];
}

/*----------------------------------------------------------------------------
  Function that outputs value to LEDs
 *----------------------------------------------------------------------------*/
void LED_Out(unsigned int value) {
  int i;

  for (i = 0; i < LED_NUM; i++) {
    if (value & (1<<i)) {
      LED_On (i);
    } else {
      LED_Off(i);
    }
  }
}

/*----------------------------------------------------------------------------
  Function that toggles state of requested LED
 *----------------------------------------------------------------------------*/
void LED_Toggle (unsigned int num) {

  if (num < 3) LPC_GPIO1->FIOPIN ^= led_mask[num];
  else         LPC_GPIO2->FIOPIN ^= led_mask[num];
}

// ==========================================================================================================================================
// =========================================================================  OTHERS  =======================================================
// ==========================================================================================================================================




void initializeControls() {
	NVIC_EnableIRQ( ADC_IRQn ); 							/* Enable ADC interrupt handler  */
  LED_Init ();                              /* Initialize the LEDs           */
  GLCD_Init();                              /* Initialize the GLCD           */
	GLCD_Clear(Black);                        /* Clear the GLCD                */
	LCD_Init ();															/* Configure LCD Settings        */
	KBD_Init ();                              /* initialize Push Button        */
	ADC_Init ();															/* initialize the ADC            */
}