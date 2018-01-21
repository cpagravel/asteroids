/****************************************************************************
 *   $Id:: uart.c 5751 2010-11-30 23:56:11Z usb00423                        $
 *   Project: NXP LPC17xx UART example
 *
 *   Description:
 *     This file contains UART code example which include UART initialization, 
 *     UART interrupt handler, and APIs for UART access.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
#include "lpc17xx.h"
#include "type.h"
#include "uart.h"

//#ifdef __DBG_ITM
volatile int ITM_RxBuffer = ITM_RXBUFFER_EMPTY;  /*  CMSIS Debug Input        */
//#endif

volatile uint32_t UART0Status, UART1Status;
volatile uint8_t UART0TxEmpty = 1, UART1TxEmpty = 1;
volatile uint8_t UART0Buffer[BUFSIZE], UART1Buffer[BUFSIZE];
volatile uint32_t UART0Count = 0, UART1Count = 0;

volatile uint8_t RcvLock0; 
volatile uint8_t SndLock0; 

volatile uint8_t RcvLock1; 
volatile uint8_t SndLock1; 

volatile int i = 0;

void Free(volatile uint8_t *tbl){
	*tbl = 0;
}

uint8_t Lock(volatile uint8_t *tbl){
	// Get the lock status and see if it is already locked
	if (__LDREXW(tbl) == 0) {
		// if not locked, try set lock to 1
		return  (__STREXW(1, tbl) != 0) ;
	} else {
		return(1); // return fail status
	}
}

uint8_t LockRcv(uint8_t portNum){
	if(portNum > 1)
		return 0x1;
	return Lock(portNum == 0? &RcvLock0 : &RcvLock1);
}

uint8_t LockSnd(uint8_t portNum){
	if(portNum > 1)
		return 0x1;
	return Lock(portNum == 0? &SndLock0 : &SndLock1);
}

void FreeRcv(uint8_t portNum){
	if(portNum > 1)
		return;
	Free( portNum == 0? &RcvLock0 : &RcvLock1 );
}

void FreeSnd(uint8_t portNum){
	if(portNum > 1)
		return;
	Free( portNum == 0? &SndLock0 : &SndLock1 );
}


/*****************************************************************************
** Function name:		UART0_IRQHandler
**
** Descriptions:		UART0 interrupt handler
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void UART0_IRQHandler (void) 
{
	uint8_t IIRValue, LSRValue;

	IIRValue = LPC_UART0->IIR;

	IIRValue >>= 1;			/* skip pending bit in IIR */
	IIRValue &= 0x07;			/* check bit 1~3, interrupt identification */

	LSRValue = LPC_UART0->LSR;

	if ( LSRValue & LSR_RDR )	/* Receive Data Ready */	
	{
		/* If no error on RLS, normal ready, save into the data buffer. */
		/* Note: read RBR will clear the interrupt */
		UART0Buffer[UART0Count] = LPC_UART0->RBR;
		UART0Count++;
		if ( UART0Count == BUFSIZE )
		{
		UART0Count = 0;		/* buffer overflow */
		}
	}

	if ( IIRValue == IIR_THRE )	/* THRE, transmit holding register empty */
	{
	/* THRE interrupt */
		LSRValue = LPC_UART0->LSR;		/* Check status in the LSR to see if
									valid data in U0THR or not */
		if ( LSRValue & LSR_THRE ){
			UART0TxEmpty = 1;
		}
		else{
			UART0TxEmpty = 0;
		}
	}

}

/*****************************************************************************
** Function name:		UART1_IRQHandler
**
** Descriptions:		UART1 interrupt handler
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void UART1_IRQHandler (void) 
{

	uint8_t IIRValue, LSRValue;

	IIRValue = LPC_UART1->IIR;

	IIRValue >>= 1;			/* skip pending bit in IIR */
	IIRValue &= 0x07;			/* check bit 1~3, interrupt identification */

	LSRValue = LPC_UART1->LSR;

	if ( LSRValue & LSR_RDR )	/* Receive Data Ready */	
	{
		/* If no error on RLS, normal ready, save into the data buffer. */
		/* Note: read RBR will clear the interrupt */
		UART1Buffer[UART1Count] = LPC_UART1->RBR;
		UART1Count++;
		if ( UART1Count == BUFSIZE ){
		UART0Count = 0;		/* buffer overflow */
		}
	}

	if ( IIRValue == IIR_THRE )	/* THRE, transmit holding register empty */
	{
	/* THRE interrupt */
		LSRValue = LPC_UART1->LSR;		/* Check status in the LSR to see if
									valid data in U0THR or not */
		if ( LSRValue & LSR_THRE ){
			UART1TxEmpty = 1;
		}
		else{
			UART1TxEmpty = 0;
		}
	}

}

/* By default, the PCLKSELx value is zero, thus, the PCLK for
	all the peripherals is 1/4 of the SystemFrequency. */
uint32_t getFrequency(uint32_t clk_slct){

	uint32_t pclk;

	switch ( (LPC_SC->PCLKSEL0 >> clk_slct) & 0x03 )
	{
		case 0x00:
		default:
		pclk = SystemCoreClock/4;
		break;
		case 0x01:
		pclk = SystemCoreClock;
		break; 
		case 0x02:
		pclk = SystemCoreClock/2;
		break; 
		case 0x03:
		pclk = SystemCoreClock/8;
		break;
	}

	return pclk;
}

/*****************************************************************************
** Function name:		UARTInit
**
** Descriptions:		Initialize UART port, setup pin select,
**						clock, parity, stop bits, FIFO, etc.
**
** parameters:			portNum(0 or 1) and UART baudrate
** Returned value:		true or false, return false only if the 
**						interrupt handler can't be installed to the 
**						VIC table
** 
*****************************************************************************/
uint32_t UARTInit( uint32_t PortNum, uint32_t baudrate )
{
	uint32_t Fdiv;
	uint32_t  pclk;

	if ( PortNum == 0 )
	{
		LPC_PINCON->PINSEL0 &= ~0x000000F0;
		LPC_PINCON->PINSEL0 |= 0x00000050;  /* RxD0 is P0.3 and TxD0 is P0.2 */

		/* Bit 6~7 is for UART0 */
		pclk = getFrequency(6);

		LPC_UART0->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit, The access to Divisor latches is enabled. */

		Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
		LPC_UART0->DLM = Fdiv / 256;					
		LPC_UART0->DLL = Fdiv % 256;

		LPC_UART0->LCR = 0x03;		/* DLAB = 0 */
		LPC_UART0->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	 	NVIC_EnableIRQ(UART0_IRQn);

		//LPC_UART0->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART0 interrupt */
		//LPC_UART0->IER =  IER_THRE ;//| IER_RLS;			/* Disable RBR */

		FreeRcv(0);
		FreeSnd(0);
		return (TRUE);
	}
	else if ( PortNum == 1 )
	{
		LPC_PINCON->PINSEL4 &= ~0x0000000F;
		LPC_PINCON->PINSEL4 |= 0x0000000A;	/* Enable RxD1 P2.1, TxD1 P2.0 */

	/* By default, the PCLKSELx value is zero, thus, the PCLK for
	all the peripherals is 1/4 of the SystemFrequency. */
	/* Bit 8,9 are for UART1 */
		pclk = getFrequency(8);

		LPC_UART1->LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */

		Fdiv = ( pclk / 16 ) / baudrate ;	/*baud rate */
		LPC_UART1->DLM = Fdiv / 256;					
		LPC_UART1->DLL = Fdiv % 256;

		LPC_UART1->LCR = 0x03;		/* DLAB = 0 */
		LPC_UART1->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

	 	NVIC_EnableIRQ(UART1_IRQn);

		//LPC_UART1->IER = IER_RBR | IER_THRE | IER_RLS;	/* Enable UART1 interrupt */

		FreeRcv(1);
		FreeSnd(1);

		return (TRUE);
	}
	return( FALSE ); 
}

/*****************************************************************************
** Function name:		UARTSend
**
** Descriptions:		Send a block of data to the UART 0 port based
**						on the data length
**
** parameters:			portNum, buffer pointer, and data length
** Returned value:		None
** 
*****************************************************************************/

void UARTSend( uint32_t portNum, uint8_t *BufferPtr, uint32_t Length )
{
	LPC_UART_TypeDef *LPC_UART;
	volatile unsigned char *UARTTxEmpty;				//ASK Douglas
	uint8_t *localBufferPtr;
	uint32_t localLength;

	if((portNum >> 1 ) != 0)
		return;

	localLength = Length;
	localBufferPtr = BufferPtr;
	UARTTxEmpty = (portNum == 0 ? &UART0TxEmpty : &UART1TxEmpty);
	LPC_UART = (portNum == 0 ? (LPC_UART_TypeDef *)LPC_UART0 : (LPC_UART_TypeDef *)LPC_UART1 );

	//Enable interupt
	LPC_UART->IER |=  IER_THRE;

	while( LockSnd(portNum));

	while ( localLength != 0 ){
		/* THRE status, contain valid data */
		while ( !(*UARTTxEmpty & 0x01) );
		LPC_UART->THR = *localBufferPtr;
		*UARTTxEmpty = 0;	/* not empty in the THR until it shifts out */
		localBufferPtr++;
		localLength--;
	}

	FreeSnd(portNum);

	//Reanble other interpts
	LPC_UART->IER &= ~IER_THRE;

	return;
}

void UARTSendChar( uint32_t portNum, uint8_t character)
{
	#ifdef __RTGT_UART
		LPC_UART_TypeDef *LPC_UART;
		LPC_UART = (portNum == 0 ? (LPC_UART_TypeDef *)LPC_UART0 : (LPC_UART_TypeDef *)LPC_UART1 );
		while (!(LPC_UART->LSR & 0x20));
		LPC_UART->THR = character;
	#else
		ITM_SendChar(character);
	#endif

}


/*****************************************************************************
** Function name:		UARTRecieve
**
** Descriptions:		Recieve a block of data to the UART 0-1 port based
**						on the data length
**
** parameters:			portNum, buffer pointer, and data length
** Returned value:		integer showing status
** 
*****************************************************************************/
uint32_t UARTRecieve( uint32_t portNum, uint8_t *BufferPtr, uint32_t Length )
{

	LPC_UART_TypeDef *LPC_UART;
	volatile uint32_t *UARTCount;				//ASK Douglas
	volatile uint8_t *UARTBuffer;
	uint8_t *rcvdBufferPtr;
	uint32_t rcvd_len, i;

	if((portNum >> 1 ) != 0)
		return 0;

	rcvd_len = 0x0;
	rcvdBufferPtr = BufferPtr;
	UARTCount = (portNum == 0 ? &UART0Count : &UART1Count);
	UARTBuffer = (portNum == 0 ? UART0Buffer : UART1Buffer);
	LPC_UART = (portNum == 0 ? (LPC_UART_TypeDef *)LPC_UART0 : (LPC_UART_TypeDef *)LPC_UART1 );

	*UARTCount = 0x0;

	//Enable interupt
	LPC_UART->IER |=  IER_RBR;

	//busy waiting

	while( *UARTCount == 0 );


	//This part has to be put in the critical section
	while(LockRcv(portNum));

	//Reanble other interpts
	LPC_UART->IER &= ~IER_RBR;			/* Disable RBR */


	for(i = 0; i < *UARTCount; ++i)
		rcvdBufferPtr[rcvd_len++] = UARTBuffer[i];

	*UARTCount = 0;

	FreeRcv(portNum);

	return rcvd_len;
}

uint8_t UARTReceiveChar( uint32_t portNum)
{
	#ifdef __RTGT_UART
		/*uint8_t ret[1];
		if (UARTRecieve(portNum, ret, 1) == 1)
			return ret[0];
		return 0x0;	*/
		LPC_UART_TypeDef *LPC_UART;
		LPC_UART = (portNum == 0 ? (LPC_UART_TypeDef *)LPC_UART0 : (LPC_UART_TypeDef *)LPC_UART1 );
		while (!(LPC_UART->LSR & 0x01));
		return (LPC_UART->RBR);
	#else
		while (ITM_CheckChar() != 1) __NOP();
		return (ITM_ReceiveChar());
	#endif
}

/******************************************************************************
**                            End Of File
******************************************************************************/
