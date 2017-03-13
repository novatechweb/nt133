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
	}, {
		.port = GPIOB,
		.pin = GPIO12,
		.enable = true,
	}, {
		.port = GPIOA,
		.pin = GPIO9,
		.enable = true,
	}, {
		.port = GPIOA,
		.pin = GPIO8,
		.enable = true,
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

void get_led_data(uint32_t *led_data)
{
	// read the value of the input pins
	register uint32_t input_state = (uint32_t)get_inputs();
	// read the state of the output ports and shift them into position
	register uint32_t output_state = (uint32_t)get_outputs();
	// store the led data
	*led_data = input_state | (output_state << 4);
}


#if 0
inline uint16_t get_inputs()
{
	return gpio_get(INPUT_PORT, GPIO_ALL);
}
#else //sagazio
inline uint16_t get_inputs()
{
	uint16_t s1, s2, s3, s4;
	//uint16_t ledPort;
	
	s1 = gpio_get(GPIOA, GPIO6 | GPIO7);                 //in1, 2
	s1 = s1 >> 6;
	s1 |= (gpio_get(GPIOB, GPIO0 | GPIO1) << 2);		 //in3, 4

	s2 = (gpio_get(GPIOB,GPIO6 | GPIO7 | GPIO8 | GPIO9) >> 6);  //in5, 6, 7, 8

	s3 = gpio_get(GPIOA, GPIO0 | GPIO1 | GPIO2 | GPIO3);   //in9, 10, 11, 12
	s4 = (gpio_get(GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9) >> 6);   //in13,14, 15, 16


	//ledPort = (s4 << 12) | (s3 << 8) | (s2 << 4) | s1;

    return ((s4 << 12) | (s3 << 8) | (s2 << 4) | s1);
}
#endif


inline uint16_t get_outputs()
{
	return gpio_port_write_value(OUTPUT_PORT,
		(OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 |
		OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
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
		// set the new timeout value, converting milliseconds to 100's of microseconds
		output_timers[output_index].tick_count = msec_time * 10;
		// start the timer
		add_timer(&(output_timers[output_index]));
	}
	return true;
}

usec_time_t get_timer_remaining(uint8_t output_index)
{
	usec_time_t time_remaining = 0;

	if (output_index > NUM_OUTPUTS) {
		// return an invalad number to indicate a failure
		return 0xFFFFFFFF;
	}

	if (timer_started(&(output_timers[output_index]))) {
		cm_disable_interrupts();
		time_remaining = output_timers[output_index].tick_count;
		cm_enable_interrupts();
		// truncate to milliseconds
		time_remaining = time_remaining / 10;
	}

	return time_remaining;
}
