#ifndef __CDCACM_H_
#define __CDCACM_H_

#include <stdint.h>

#include <libopencm3/usb/usbd.h>

uint16_t usb_set_interrupt_data(uint16_t pin_state);
void usb_reenumerate(void);
void poll_usb(void);

#endif /* __CDCACM_H_ */
