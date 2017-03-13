#ifndef __IO_H_
#define __IO_H_

#include <stdint.h>
#include <stdbool.h>

#include <nt_timer.h>

void io_init(void);
void get_led_data(uint32_t *led_data);
uint16_t get_inputs();
uint16_t get_outputs();
uint16_t relays_enabled();
void enable_relays();
void disable_relays();
bool set_output(uint8_t output_index, bool enable, usec_time_t msec_time);
usec_time_t get_timer_remaining(uint8_t output_index);

#define RNGN0	4	//NEW_CODE: , index for binary input range control 

#endif /* __IO_H_ */
