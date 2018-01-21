#ifndef _Controls_H
#define _Controls_H


extern void LED_on(unsigned char led);
extern void LED_off(unsigned char led);
extern __task void led (void);
extern __task void keyread(void);
extern __task void adc (void);
extern __task void joystick (void); 

extern void initializeControls(void);


#endif // Controls_H
 
// ==========================================================================================================================================
// =========================================================================  ADC  ==========================================================
// ==========================================================================================================================================
 
#ifndef __ADC_H
#define __ADC_H

#define SBIT_BURST      16u
#define SBIT_START      24u
#define SBIT_PDN        21u
#define SBIT_EDGE       27u 
#define SBIT_DONE       31u
#define SBIT_RESULT     4u
#define SBIT_CLCKDIV    8u

#define  util_GetBitMask(bit)          ((uint32_t)1<<(bit))
#define  util_BitSet(x,bit)            ((x) |=  util_GetBitMask(bit))
#define  util_GetBitStatus(x,bit)      (((x)&(util_GetBitMask(bit)))!=0u)
 
 extern unsigned int ADCStat;
 extern unsigned int ADCValue;
 
 extern void ADC_Init (void);
 extern void ADC_IRQHandler( void );
 extern void ADC_ConversionStart (void );

#endif

// ==========================================================================================================================================
// =========================================================================  KBD  ==========================================================
// ==========================================================================================================================================

#ifndef __KBD_H
#define __KBD_H

#define KBD_SELECT      0x01               
#define KBD_LEFT        0x08               
#define KBD_UP          0x10               
#define KBD_RIGHT       0x20               
#define KBD_DOWN        0x40
#define KBD_MASK        0x79  

#define KBD_Right()   !(KBD_val & KBD_RIGHT)
#define KBD_Down()    !(KBD_val & KBD_DOWN)
#define KBD_Up()      !(KBD_val & KBD_UP)
#define KBD_Left()    !(KBD_val & KBD_LEFT)

extern uint32_t KBD_val;             

extern void          KBD_Init  (void);
extern void 				 KBD_Read  (void);
extern uint32_t      KBD_Get   (void);
extern uint32_t      INT0_Get  (void);


#endif

// ==========================================================================================================================================
// =========================================================================  LED  ==========================================================
// ==========================================================================================================================================
 
#ifndef __LED_H
#define __LED_H

/* LED Definitions */
#define LED_NUM     8                        /* Number of user LEDs          */

extern void LED_Init(void);
extern void LED_On  (unsigned int num);
extern void LED_Off (unsigned int num);
extern void LED_Out (unsigned int value);
extern void LED_Toggle (unsigned int value); /*Added by AG for MTE 241 */

#endif


 