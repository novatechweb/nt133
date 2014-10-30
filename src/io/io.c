#include "io.h"

#include <stdint.h>
#include <strings.h>

#include <platform.h>
#include <usb.h>

#include <libopencm3/stm32/gpio.h>

void io_init(void)
{
	io_platform_init();
	// disable voltage to output relays
	gpio_clear(LOCKOUT_PORT, LOCKOUT_PIN);
	// turn off all outputs
	gpio_port_write(OUTPUT_PORT, 0x00);
}

void io_poll(void)
{
	register uint16_t port_c = gpio_get(GPIOC, GPIO_ALL);
	register uint16_t prev_port_c = port_c;
	// Loop forever pollong the input lines looking for a change
	while (1) {
		// read the port
		port_c = gpio_get(GPIOC, GPIO_ALL);
		// check if port changed
		if (prev_port_c == port_c) {
			continue;
		}
		// *** The port is not the same as previous poll ***
		// Update the USB interrupt endpoint data
		usb_set_interrupt_data(port_c);
		// store the port state
		prev_port_c = port_c;
	}
}

void get_led_state(uint16_t *led_state)
{
	uint16_t led_status;
	uint16_t port_B;
	uint16_t port_D;

	// read the value of the input pins
	led_status = gpio_get(GPIOC, GPIO_ALL);
	// read the state of the output ports
	port_B = GPIO_ODR(OUTPUT_PORT);
	port_D = GPIO_ODR(LOCKOUT_PORT);
	// get only the bits we want to display on the LEDs
	if ((port_D & LOCKOUT_PIN) == LOCKOUT_PIN) {
		// the outputs are enabled, display them
		led_status = ((led_status & 0x0FFF) | (port_B & (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4)));
	}
	*led_state = led_status;
}

void get_inputs(uint8_t *buff)
{
	register uint16_t port_c = gpio_get(GPIOC, GPIO_ALL);
	buff[0] = (uint8_t)port_c >> 8;
	buff[1] = (uint8_t)port_c & 0xFF;
}

void set_outputs(bool output_enabled, uint8_t output_state)
{
	uint16_t outputs = output_state << OUTPUT_PORT_BITSHIFT;
	if (output_enabled) {
		// clear the output port pins
		gpio_clear(OUTPUT_PORT, (~outputs & (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4)));
		// set the output port pins
		gpio_set(OUTPUT_PORT, ( outputs & (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4)));
		// enable realys
		gpio_set(LOCKOUT_PORT, LOCKOUT_PIN);
	} else {
		// disable relays
		gpio_clear(LOCKOUT_PORT, LOCKOUT_PIN);
		// turn off outputs
		gpio_clear(OUTPUT_PORT, (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 |
			OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
	}
}
