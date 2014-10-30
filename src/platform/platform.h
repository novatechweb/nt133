#ifndef __PLATFORM_H_
#define __PLATFORM_H_

#ifdef STM32F1
  #include "platform/STM32F1/platform_STM32F1.h"
  #include "platform/STM32F1/platform_io.h"
  #include "platform/STM32F1/platform_i2c_led.h"
  #include "platform/STM32F1/platform_usb.h"
#endif

/*** Interrupt priorities ***/
/* Interrupt priorities.  Low numbers are high priority.
 * ((1 << 7) + ?) are assigned to the system interrupts.
 */
#define IRQ_PRI_ER_I2C      (0 << 4)
#define IRQ_PRI_SYSTICK     (1 << 4)
#define IRQ_PRI_I2C         (3 << 4)
#define IRQ_PRI_USB         (4 << 4)

#define NOVATECH_VENDOR_ID  0x2AEB
// board #: 750-133
#define USB_PRODUCT_ID 133
#define USB_BCD_VERSION_NUM ((VERSION_MAJOR << 8) | VERSION_MINOR)

#endif /* __PLATFORM_H_ */
