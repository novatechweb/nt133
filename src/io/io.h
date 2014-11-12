#ifndef __IO_H_
#define __IO_H_

#include <stdint.h>
#include <stdbool.h>
#include <platform.h>

void io_init(void);
void io_poll(void);
void get_led_state(uint16_t *led_state);
void set_outputs(bool output_enabled, uint8_t output_state);

inline uint16_t get_inputs()
{
	return gpio_get(INPUT_PORT, GPIO_ALL);
}

#endif /* __IO_H_ */
