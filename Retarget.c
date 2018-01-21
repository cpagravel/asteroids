/*----------------------------------------------------------------------------
* Name:    Retarget.c
* Purpose: 'Retarget' layer for target-dependent low level functions
* Note(s):
*----------------------------------------------------------------------------
* This file is part of the uVision/ARM development tools.
* This software may only be used under the terms of a valid, current,
* end user licence from KEIL for a compatible version of KEIL software
* development tools. Nothing else gives you the right to use this software.
*
* This software is supplied "AS IS" without warranties of any kind.
*
* Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
*----------------------------------------------------------------------------*/


#include <stdio.h>
#include <rt_misc.h>

#define __RTGT_UART

#ifdef __RTGT_GLCD
	#include "GLCD_Scroll.h"
#endif

#ifdef __RTGT_UART 
	#include "uart.h"
	#define PORT_NUM 0
	#define BAUD_RATE 9600
#endif

#if !defined( __RTGT_GLCD ) && !defined(__RTGT_UART)
	#include "uart.h"
	
	#define PORT_NUM 10 //The printf window in the simulator will get the stream
	#define __DBG_ITM
#endif

//#pragma import(__use_no_semihosting_swi)

#ifdef __RTGT_GLCD
//A switch varaible to see if the init is called.
volatile uint8_t glcd_init_called = 0;
#endif

#ifdef __RTGT_UART
//A switch varaible to see if the init is called.
volatile uint8_t uart_init_called = 0;
#endif

/*----------------------------------------------------------------------------
Write character to Serial Port
*----------------------------------------------------------------------------*/
int sendchar( int c ) {

	#ifdef __RTGT_GLCD
	//call init_scroll if it is not called
	//Warning, this is not a thread safe code
	if ( glcd_init_called == 0 ) {
		glcd_init_called = 1;
		ScrollInit();
	}
	#endif

	#ifdef __RTGT_UART

	//call init_scroll if it is not called
	//Warning, this is not a thread safe code
	if ( uart_init_called == 0 ) {
		uart_init_called = 1;
		UARTInit(PORT_NUM, BAUD_RATE);
	}

	#endif
	
	if ( c == '\r' || c == '\n' ) {
		#if defined( __RTGT_UART ) || defined( __DBG_ITM )
			UARTSendChar( PORT_NUM, 0x0D );
			UARTSendChar( PORT_NUM, 0x0A );
		#endif

		#ifdef __RTGT_GLCD
			CharAppend('\n');
		#endif
	} else {
		#if defined(__RTGT_UART) || defined(__DBG_ITM)
			UARTSendChar(PORT_NUM, c);
		#endif
		#ifdef __RTGT_GLCD
			CharAppend(c);
		#endif
	}

	return c;
}


/*----------------------------------------------------------------------------
Read character from Serial Port   (blocking read)
*----------------------------------------------------------------------------*/
int getkey( void ) {

	#ifdef __RTGT_UART
	//call UARTInit if it is not called
	//Warning, this is not a thread safe code

	if ( uart_init_called == 0 ) {
		uart_init_called = 1;
		UARTInit(PORT_NUM, BAUD_RATE);
	}
	#endif
	
	#if defined( __RTGT_UART ) || defined( __DBG_ITM )
		return UARTReceiveChar( PORT_NUM );
	#else
		return -1;
	#endif
}


struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;


int fputc( int ch, FILE *f ) {

	return (sendchar(ch));
}


int fgetc( FILE *f ) {
	int ch = getkey();

	sendchar( ch );

	return ch;
}


int ferror( FILE *f ) {

	/* Your implementation of ferror */
	return EOF;
}


void _ttywrch( int ch ) {

	sendchar(ch);
}


void _sys_exit( int return_code ) {

label:  goto label;  /* endless loop */
}
