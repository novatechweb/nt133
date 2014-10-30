#ifndef __CDCACM_H_
#define __CDCACM_H_

#include <stdint.h>

#include <io.h>

void usb_set_interrupt_data(uint16_t pin_state);
void usb_reenumerate(void);

#endif /* __CDCACM_H_ */
