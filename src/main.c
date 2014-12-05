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

int main(int UNUSED(argc), char **UNUSED(argv))
{
	uint16_t pin_state = get_inputs();
	uint16_t prev_pin_state = pin_state;

	// Make certain the hardware is back at reset state
	platform_reset_hardware();
	
	// disable interrupts
	cm_disable_interrupts();
	
	// initilize each module in order
	platform_init();
	io_init();
	i2c_led_init();
	initialize_timer();

	// enable interrupts
	cm_enable_interrupts();

	// Start USB
	usb_reenumerate();

	// continualy poll the input lines
	// Loop forever pollong the input lines looking for a change
	while (true) {
		poll_usb();
		// examine the next timer even and determine if a callback function should be executed
		run_timer_event();
		// read the value of the input pins
		pin_state = get_inputs();
		// check if port changed
		if (prev_pin_state == pin_state) {
			continue;
		}
		// *** The port is not the same as previous poll ***
		// Update the USB interrupt endpoint data
		if (usb_set_interrupt_data(pin_state)) {
			// store the port state
			prev_pin_state = pin_state;
		}
	}

	/* Should never get here */
	return EXIT_SUCCESS;
}
