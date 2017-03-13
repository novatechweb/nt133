#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>

#include "platform/platform.h"
#include "i2c_led/i2c_led.h"
#include "io/io.h"
#include "nt_timer/nt_timer.h"
#include "usb/usb.h"
#include <libopencm3/stm32/f1/timer.h>


int main(int UNUSED(argc), char **UNUSED(argv))
{
    //NEW_CODE: the following volatile declaration is for the purpose of debugging.
	//			please feel free to remove if it's unnecessary.
	volatile uint16_t pin_state;
    volatile uint16_t prev_pin_state;

	//NEW_CODE: Added to support new i/o board
	volatile binary_input_event bin_in_data[PPS_TICKS_BUFFER_DEPTH];
	volatile uint8_t bin_in_index = 0;

	volatile uint32_t local_us125_tick_counter;
	volatile enum pps_status local_pps_quality;
	volatile uint32_t local_seconds_tick_counter;

    
    // Make certain the hardware is back at reset state
    platform_reset_hardware();
	
    // disable interrupts
    cm_disable_interrupts();
	
    // initilize each module in order
    platform_init();
    io_init();
    initialize_timer();
    i2c_led_init();

    pin_state = get_inputs();
    prev_pin_state = pin_state;

    // enable interrupts
    cm_enable_interrupts();

    // Start USB
    usb_reenumerate();

    //gpio_primary_remap(AFIO_MAPR_SWJ_CFG_FULL_SWJ_NO_JNTRST,0x1FFFFF);
    //gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,0x1FFFFF);
    //gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF,0x1FFFFF);

    //NEW_CODE: Added to support new i/o board
    //gpio_clear(GPIOB,GPIO12);	// 80VDC Input Mode
    gpio_set(GPIOB,GPIO12);	// 480VDC Input Mode


    // continualy poll the input lines
    // Loop forever pollong the input lines looking for a change
    while (true) 
    {
        //NEW_CODE: Used for debug only
        //gpio_set(GPIOA,GPIO8);
        
        poll_usb();
		
        // examine the next timer even and determine if a callback function should be executed
        run_timer_event();

		//NEW_CODE: Added to support new i/o board, 
		local_us125_tick_counter = us125_tick_counter;
		local_pps_quality = pps_quality;
		local_seconds_tick_counter = seconds_tick_counter;


        pin_state = get_inputs();
                        
        // check if port changed
        if (prev_pin_state == pin_state) 
            continue;
        
        
        //NEW_CODE: Added to support new i/o board
        // us125_tick_counter represents the number of iterations the 125us interrupt was called
        // multiply the value of tim1_cnt_pps_us x 125us
        // multiply the value of tim1_cnt_pps_ns x 13.88ns
        // both should be placed in event struct for USB extraction

        
        bin_in_data[bin_in_index].micro_sec_counts = local_us125_tick_counter; 
        //bin_in_data[bin_in_index].nano_sec_counts = timer_get_counter(TIM1);
        bin_in_data[bin_in_index].seconds = local_seconds_tick_counter;
        bin_in_data[bin_in_index].quality = local_pps_quality;
        bin_in_data[bin_in_index].pin_state = pin_state;
        
        if (++bin_in_index < PPS_TICKS_BUFFER_DEPTH)
            ;
        else
            bin_in_index = 0;
        
              
        //NEW_CODE: Used for debug only, used to trigger oscilloscope
        //gpio_clear(GPIOA,GPIO8);
        
            
        #if 1  //NEW_CODE: below is the original, released code
        // *** The port is not the same ast previous poll ***
        // Update the USB interrupt endpoint data
        if (usb_set_interrupt_data(pin_state)) 
            prev_pin_state = pin_state;	// store the port state
        #else   //NEW_CODE: below is the modified code to put ts info out to usb
        if (usb_set_interrupt_data(bin_in_data[bin_in_index])) 
            prev_pin_state = pin_state;
        #endif
     }   // end of while (1)

    /* Should never get here */
    return EXIT_SUCCESS;
}

