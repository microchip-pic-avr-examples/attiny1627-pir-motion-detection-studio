/**
 * \file main.c
 *
 * \brief Main source file.
 *
 (c) 2020 Microchip Technology Inc. and its subsidiaries.
    Subject to your compliance with these terms, you may use this software and
    any derivatives exclusively with Microchip products. It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */
 
#include "clock_config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdlib.h>

#include "io.h"
#include "rtc.h"
#include "usart.h"

void adc_init();
void EVENT_SYSTEM_0_init();
void warm_up_and_filter_creation();
void check_for_movement();
void update_average_filters();

#define F_CPU_IN_SLEEP 5000000	
#define ADC_TIMEBASE_VALUE ((uint8_t)ceil(F_CPU_IN_SLEEP * 0.000001) << ADC_TIMEBASE0_bp)

/**********************************************************************************************
Application Sampling, Filtering and Detection Parameters
***********************************************************************************************/
#define PIR_OVERSAMPLE_RATE				ADC_SAMPNUM_ACC16_gc				/* Valid options are NONE, 2, 4, 8, 16, 32, 64, 128, 256, 512 and 1024 */
#define PIR_SAMPLE_RATE_PER_SECOND		EVSYS_CHANNEL1_RTC_PIT_DIV256_gc	/* Valid options are 64(16x), 128(8x), 256(4x) and 512(2x) */
#define PIR_PGA_GAIN					ADC_GAIN_16X_gc						/* Valid options are 1X, 2X, 4X, 8X and 16X */

#define PIR_DETECTION_THRESHOLD 60		/* Detection threshold */
#define PIR_WARMUP_TIME_MS 0			/* PIR warm-up time in milliseconds */
#define PIR_LONG_TERM_FILTER_RANGE 16	/* Length of long term filter */
#define PIR_SHORT_TERM_FILTER_RANGE 4   /* Length of short term filter */

//#define PIR_DEBUG_MESSAGES 		/* Comment out this line for lower power consumption, comment in for debug messages to be sent over USART */

/**********************************************************************************************/
/* If ADC_SAMPNUM_ACC1024_gc is selected in PIR_OVERSAMPLE_RATE, the below variables must be changed to int32_t */
volatile int32_t adc_result = 0;
volatile int16_t filter_delta = 0;
volatile int16_t long_term_average = 0;
volatile int16_t short_term_average = 0;
volatile int32_t accumulated_long_term_average = 0;
volatile int16_t accumulated_short_term_average = 0;

/**********************************************************************************************
ADC initialization
***********************************************************************************************/
void ADC_0_init()
{
	ADC0.CTRLB = ADC_PRESC_DIV2_gc;			/* System clock divided by 2 */
	ADC0.CTRLF = PIR_OVERSAMPLE_RATE;		/* Samples accumulation */
	ADC0.CTRLC = ADC_REFSEL_1024MV_gc		/* Internal 1.024V Reference */
	             | ADC_TIMEBASE_VALUE;		/* timebase value */

				
	ADC0.CTRLE = 0x12;						/* Sample Duration */

	ADC0.DBGCTRL = ADC_DBGRUN_bm;			/* Debug run: enabled */
	ADC0.COMMAND = ADC_DIFF_bm				/* Differential ADC Conversion enabled */
				 | ADC_MODE_BURST_gc;		/* Burst Accumulation */
				 
	ADC0.INTCTRL = ADC_RESRDY_bm;			/* Result Ready Interrupt enabled */

	ADC0.MUXNEG = ADC_VIA_PGA_gc			/* Via PGA */
				| ADC_MUXNEG_AIN5_gc;		/* ADC input pin 5, PIR sensor */
				
	ADC0.MUXPOS = ADC_VIA_PGA_gc			/* Via PGA */
				| ADC_MUXPOS_AIN9_gc;		/* ADC input pin 9, PIR sensor */
				
	ADC0.PGACTRL = ADC_PGAEN_bm				/* PGA Enabled */
				 | PIR_PGA_GAIN				/* Gain */
				 | ADC_ADCPGASAMPDUR_15CLK_gc/* PGA Sample Duration */
				 | ADC_PGABIASSEL_1_2X_gc;  /* PGA bias set to 1/2 */
				  
	ADC0.CTRLA = ADC_ENABLE_bm				/* Enable ADC */
			   | ADC_RUNSTDBY_bm;			/* Enable ADC to run in Standby Sleep mode */
			    
	ADC0.INTFLAGS = ADC_RESRDY_bm;			/* Clear ADC Result Ready Interrupt Flag */
}

/**********************************************************************************************
Event System initialization
***********************************************************************************************/

void EVENT_SYSTEM_0_init()
{
	PORTMUX.EVSYSROUTEA |= PORTMUX_EVOUTB_bm;			/* Select PB7 as event out pin */
	
	EVSYS.CHANNEL1 = PIR_SAMPLE_RATE_PER_SECOND;		/* Connects Event System to PIT, used for triggering the ADC conversion */
	EVSYS.CHANNEL2 = EVSYS_CHANNEL2_RTC_PIT_DIV1024_gc; /* Connects Event System to PIT, used for flashing LED 0 at 1 Hz during warm-up */
	EVSYS.CHANNEL3 = EVSYS_CHANNEL3_RTC_PIT_DIV256_gc;	/* Connects Event System to PIT, used for flashing LED 0 at 4 Hz when movement is detected */

	EVSYS.USERADC0START = EVSYS_USER_CHANNEL1_gc;		/* Connect ADC to event channel 1 */
}
/**********************************************************************************************
Initialize peripherals.
***********************************************************************************************/
int main(void)
{
	/* Initial clock set to 5MHz.																							*/
	/* If number of samples accumulated by the ADC is less than 16, CLKCTRL_PDIV_2X_gc in MCLKCTRLB and ADC0.CTRLB = ADC_PRESC_DIV4_gc;	*/
	/*  should be used to lower average power consumption																	*/						
	ccp_write_io((void *)&(CLKCTRL.MCLKCTRLB), CLKCTRL_PDIV_4X_gc | 1 << CLKCTRL_PEN_bp ); /* Divide main clock by 4x*/
	
	IO_init();						/* Initialize IO pins */
	EVENT_SYSTEM_0_init();			/* Initialize Event System */
	RTC_init();						/* Initialize PIT */
	#ifdef PIR_DEBUG_MESSAGES
	USART_init();					/* Initialize USART if SEND_SERIAL_DATA is defined */
	#endif
	
	ADC_0_init();					/* Initialize the ADC */

	SLPCTRL.CTRLA = SLPCTRL_SMODE_STDBY_gc; /* Select standby sleep mode */

	sei();							/* Enable global interrupt */

	warm_up_and_filter_creation();	/* Run warm-up and collect ADC data to create filters */
	
	while (1) 
	{
		/* Reduce main clock speed to lower peripheral clock tree power consumption when ADC is running in sleep mode */
		/* Only effective if ADC is configured to accumulate more that 8 samples.			  			 			  */ 
		/* If number of samples accumulated is less than 16, the line below should be removed						  */
		ccp_write_io((void *)&(CLKCTRL.MCLKCTRLB), CLKCTRL_PDIV_4X_gc | 1 << CLKCTRL_PEN_bp ); /* Divide main clock by 4x */

		sleep_mode();				/* Enter sleep mode */

		/* Increase main clock to run the code faster in active mode, this will reduce the average power consumption */
		/* If number of samples accumulated is less than 16, the line below should be removed						*/
		ccp_write_io((void *)&(CLKCTRL.MCLKCTRLB), CLKCTRL_PDIV_2X_gc | 1 << CLKCTRL_PEN_bp ); /* Divide main clock by 2x */
	
		update_average_filters();	/* Update filters with new measurement */
		check_for_movement();		/* Check if movement has happened by examining delta between filters */

		#ifdef PIR_DEBUG_MESSAGES	/* Send data to data visualizer if SEND_SERIAL_DATA is defined */
		usart_tx(adc_result, long_term_average, short_term_average, filter_delta);
		#endif
	}
}

/**********************************************************************************************
Delay to allow PIR sensor to "warm-up" to "get use to" the IR levels applied on the sensor.
Start ADC and create short and long term average filters with initial ADC measurements.
Flash LED 0 at 1 Hz during entire period.
***********************************************************************************************/
void warm_up_and_filter_creation()
{
	uint8_t i = 0;

	EVSYS.USEREVSYSEVOUTB = EVSYS_USER_CHANNEL2_gc; /* Flash LED0 at 1 Hz to indicate warm-up */

	/* Put warm-up delay/code here */
	_delay_ms(PIR_WARMUP_TIME_MS);
	/* End of warm-up delay/code */

	ADC0.COMMAND |= ADC_START_EVENT_TRIGGER_gc;		/* Enable ADC to be triggered by Event*/

	while(i < PIR_LONG_TERM_FILTER_RANGE)
	{
		sleep_mode();								/* Enter Standby sleep mode and wait for ADC to complete*/
		accumulated_long_term_average = accumulated_long_term_average + adc_result; 
	
		if (i < PIR_SHORT_TERM_FILTER_RANGE)
		{
			accumulated_short_term_average = accumulated_short_term_average + adc_result;
		}
		i++;
	}

	EVSYS.USEREVSYSEVOUTB = 0; /* Warm up and average filter creation complete, stop flashing LED0 */
}

/**********************************************************************************************
Check if delta between average filters are bigger than DETECTION_LIMIT.
When there are no movement, the delta is close to zero.
When movement happens, short term filter will react faster than long term filter and delta will increase.
***********************************************************************************************/
void check_for_movement()
{
		if (abs(filter_delta) > PIR_DETECTION_THRESHOLD )  
		{
			EVSYS.USEREVSYSEVOUTB = EVSYS_USER_CHANNEL3_gc; /* Movement detected, flash LED0 */
		}
		else
		{
			EVSYS.USEREVSYSEVOUTB = 0; /* No movement, stop flashing LED0 */
		}
}

/**********************************************************************************************
Update filters, calculate short term average, long term average and delta between filters.
Idea is to update the filters on a regular basis swapping out 1/x from the long term filter
and 1/y from the short term filter each time the ADC has a new measurement. Using this method
the filters will average out noise from the ADC measurements ans improve S/N ratio.
When the output of the PIR is stable, the filter values will be close to each other.
When movement happens the short term filter will respond to change quickly and a delta between
the filters will be created and can be used to determine if movement has occurred or not.
***********************************************************************************************/
void update_average_filters()
{
	accumulated_long_term_average -= accumulated_long_term_average / PIR_LONG_TERM_FILTER_RANGE; /* Subtract 1/x from accumulated filter value */
	accumulated_short_term_average -= accumulated_short_term_average / PIR_SHORT_TERM_FILTER_RANGE; /* Subtract 1/y from accumulated filter value */

	accumulated_long_term_average += adc_result;  /* Add new ADC measurement (1/x) to accumulated filter value */
	accumulated_short_term_average += adc_result; /* Add new ADC measurement (1/y) to accumulated filter value */

	long_term_average = accumulated_long_term_average / PIR_LONG_TERM_FILTER_RANGE; /* Divide the accumulated_long_term_average on X to create long_term_average */
	short_term_average = accumulated_short_term_average	/ PIR_SHORT_TERM_FILTER_RANGE; /* Divide the accumulated_long_term_average on Y to create long_term_average */

	filter_delta = long_term_average - short_term_average; /* Find delta between filters */
}
/**********************************************************************************************
ADC result ready interrupt routine.
Read ADC result and return
***********************************************************************************************/
ISR(ADC0_RESRDY_vect)
{
	adc_result = ADC0.RESULT;			/* Read ADC result register */
	ADC0.INTFLAGS = ADC_RESRDY_bm;		/* Clear ADC result ready interrupt flag */
}
