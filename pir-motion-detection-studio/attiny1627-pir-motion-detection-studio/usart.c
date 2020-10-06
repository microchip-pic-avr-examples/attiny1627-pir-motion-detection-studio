/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <avr/io.h>
#include "clock_config.h"

#define BAUD_RATE  115200


/**************************************usart_0_init**********************************************
    Initializes USART
    Data visualizer communication. PB2:TX, PB3:RX
    Baud Rate : 115200
	PB2:    USART0 TXD:             output
	PB3:    USART0 RXD:             input
**************************************************************************************************/
void USART_init(void)
{
	USART0.BAUD = (F_CPU * 64.0) / (BAUD_RATE * 16.0);
	USART0.CTRLB = USART_TXEN_bm;   
	USART0.DBGCTRL = 1 << USART_DBGRUN_bp; /* Debug Run: enabled */

	PORTB.DIRSET = PIN2_bm ;		/* Set PB2 to output */
}

void usart_putc(uint8_t data)
{
	while(!(USART0.STATUS & USART_DREIF_bm));
	USART0.TXDATAL = data;
}
/* If ADC_SAMPNUM_ACC1024_gc is selected in PIR_OVERSAMPLE_RATE, change from int16_t to int32_t */
void usart_tx(int32_t adc_data, int16_t long_term_average, int16_t short_term_average, int16_t filter_delta)
{
	USART0.STATUS = USART_TXCIE_bm;

	usart_putc(0x3c);				/* Data Visualizer's SOF */
	
	usart_putc(adc_data);
	usart_putc(adc_data>>8);
	usart_putc(adc_data>>16);					
	usart_putc(adc_data>>24);					

	usart_putc(long_term_average);
	usart_putc(long_term_average>>8);

	usart_putc(short_term_average);
	usart_putc(short_term_average>>8);

	usart_putc(filter_delta);
	usart_putc(filter_delta>>8);

	usart_putc(0xc3);			/* Data Visualizer's EOF */
	
	while (!(USART0.STATUS & USART_TXCIE_bm));
}
