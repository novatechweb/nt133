#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <libopencm3/cm3/cortex.h>

#include "platform/platform.h"
#include "io/io.h"
#include "i2c_led/i2c_led.h"
#include "usb/usb.h"

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	// Make certain the hardware is back at reset state
	platform_reset_hardware();
	
	// disable interrupts
	cm_disable_interrupts();
	
	// initilize each module in order
	platform_init();
	io_init();
	i2c_led_init();

	// enable interrupts
	cm_enable_interrupts();

	// Start USB
	usb_reenumerate();

	// continualy poll the input lines
	io_poll();

	/* Should never get here */
	return EXIT_SUCCESS;
}
