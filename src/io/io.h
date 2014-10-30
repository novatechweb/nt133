#ifndef __IO_H_
#define __IO_H_

#include <stdint.h>
#include <stdbool.h>

void io_init(void);
void io_poll(void);
void get_led_state(uint16_t *led_state);
void get_inputs(uint8_t *buff);
void set_outputs(bool output_enabled, uint8_t output_state);

#endif /* __IO_H_ */
