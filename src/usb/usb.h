#ifndef __CDCACM_H_
#define __CDCACM_H_

#include <stdint.h>

#include <libopencm3/usb/usbd.h>

//NEW_CODE:
#if 1
uint16_t usb_set_interrupt_data(uint16_t pin_state);
#else
uint16_t usb_set_interrupt_data(uint16_t pin_state, uint32_t micro_sec, uint32_t nano_sec, uint8_t quality);
#endif

void usb_reenumerate(void);
void poll_usb(void);

#endif /* __CDCACM_H_ */
