#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include <libopencm3/stm32/gpio.h>

#include <platform.h>
#include <nt_timer.h>
#include "io.h"

struct output_t {
	const uint32_t port;
	const uint16_t pin;
	bool enable;
};

struct output_t output[] = {
	{
		.port = OUTPUT_PORT,
		.pin = OUTPUT_PORT_PIN1,
		.enable = false,
	}, {
		.port = OUTPUT_PORT,
		.pin = OUTPUT_PORT_PIN2,
		.enable = false,
	}, {
		.port = OUTPUT_PORT,
		.pin = OUTPUT_PORT_PIN3,
		.enable = false,
	}, {
		.port = OUTPUT_PORT,
		.pin = OUTPUT_PORT_PIN4,
		.enable = false,
	}
};
#define NUM_OUTPUTS (sizeof(output)/sizeof(struct output_t))

struct timer_handle_t output_timers[NUM_OUTPUTS];

static void output_timeout(struct timer_handle_t *cur_timer_handle)
{
	struct output_t *timer_data = cur_timer_handle->timer_data;
	if (timer_data->enable) {
		gpio_set(timer_data->port, timer_data->pin);
	} else {
		gpio_clear(timer_data->port, timer_data->pin);
	}
}

void io_init(void)
{
	uint8_t output_index;

	io_platform_init();
	// turn off all outputs
	disable_relays();
	// initilize output_timers
	for (output_index=0; output_index<(sizeof(output_timers)/sizeof(struct timer_handle_t)); output_index++) {
		INIT_TIMER_HANDLE(output_timers[output_index], 0, false, output_timeout, &(output[output_index]));
	}
}

void get_led_state(uint16_t *led_state)
{
	register uint16_t pin_state;
	uint16_t port_B;
	uint16_t port_D;

	// read the value of the input pins
	pin_state = get_inputs();
	// read the state of the output ports
	port_B = GPIO_ODR(OUTPUT_PORT);
	port_D = GPIO_ODR(LOCKOUT_PORT);
	// get only the bits we want to display on the LEDs
	if ((port_D & LOCKOUT_PIN) == LOCKOUT_PIN) {
		// the outputs are enabled, display them
		pin_state = ((pin_state & 0x0FFF) | (port_B & (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4)));
	}
	*led_state = pin_state;
}

inline uint16_t get_inputs()
{
	return gpio_get(INPUT_PORT, GPIO_ALL);
}

inline uint16_t relays_enabled()
{
	return ((GPIO_ODR(LOCKOUT_PORT) & LOCKOUT_PIN) == LOCKOUT_PIN);
}

inline void enable_relays()
{
	// cake certain the relays are not enabled
	gpio_clear(OUTPUT_PORT, (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
	// enable realys
	gpio_set(LOCKOUT_PORT, LOCKOUT_PIN);
}

inline void disable_relays()
{
	uint8_t output_index;
	// disable relays
	gpio_clear(LOCKOUT_PORT, LOCKOUT_PIN);
	// turn off outputs
	gpio_clear(OUTPUT_PORT, (OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
	// Stop any of the output timers that may be running
	for (output_index=0; output_index<(sizeof(output_timers)/sizeof(struct timer_handle_t)); output_index++) {
		remove_timer(&(output_timers[output_index]));
	}
}

bool set_output(uint8_t output_index, bool enable, usec_time_t msec_time)
{
	if (output_index > NUM_OUTPUTS) {
		return false;
	}
	// set the state of the output port
	if (enable) {
		gpio_set(output[output_index].port, output[output_index].pin);
	} else {
		gpio_clear(output[output_index].port, output[output_index].pin);
	}
	// remove the timer if it is running
	remove_timer(&(output_timers[output_index]));
	output[output_index].enable = !enable;
	// resetup the timer if we have a time
	if (msec_time != 0) {
		// set the new timeout value
		output_timers[output_index].tick_count = msec_time * 10;
		// start the timer
		add_timer(&(output_timers[output_index]));
	}
	return true;
}
