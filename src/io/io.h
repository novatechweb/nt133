#ifndef __IO_H_
#define __IO_H_

#include <stdint.h>
#include <stdbool.h>

#include <nt_timer.h>

void io_init(void);
void get_led_state(uint16_t *led_state);
uint16_t get_inputs();
uint16_t relays_enabled();
void enable_relays();
void disable_relays();
bool set_output(uint8_t output_index, bool enable, usec_time_t msec_time);
usec_time_t get_timer_remaining(uint8_t output_index);

#endif /* __IO_H_ */
