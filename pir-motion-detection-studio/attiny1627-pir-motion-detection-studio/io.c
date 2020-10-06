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

/**************************************IO_init*****************************************************
    Initializes IO pin directions.

    PB7:	Tiny1627 NANO LED0:		output									// ATtiny1627 Curiosity Nano
    PB4:    PIR sensor signal:      digital input buffer disabled
    PA5:    PIR sensor signal:      digital input buffer disabled
	
	All other pins set as input with pull-up on to reduce power consumption

**************************************************************************************************/
void IO_init(void)
{

PORTA.PIN0CTRL = PORT_PULLUPEN_bm;
PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
PORTA.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc ; /* PA5 analog */
PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

PORTB.PIN0CTRL = PORT_PULLUPEN_bm;
PORTB.PIN1CTRL = PORT_PULLUPEN_bm;
PORTB.PIN2CTRL = PORT_PULLUPEN_bm;
PORTB.PIN3CTRL = PORT_PULLUPEN_bm;
PORTB.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc ; /* PB4 analog */
PORTB.PIN5CTRL = PORT_PULLUPEN_bm;
PORTB.PIN6CTRL = PORT_PULLUPEN_bm;

PORTC.PIN0CTRL = PORT_PULLUPEN_bm;
PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
PORTC.PIN2CTRL = PORT_PULLUPEN_bm;
PORTC.PIN3CTRL = PORT_PULLUPEN_bm;
PORTC.PIN4CTRL = PORT_PULLUPEN_bm;
PORTC.PIN5CTRL = PORT_PULLUPEN_bm;


PORTB.DIRSET = PIN7_bm ;	/* Set pin direction to output */
PORTB.OUTSET = PIN7_bm;


}
