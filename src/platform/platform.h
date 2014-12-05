#ifndef __PLATFORM_H_
#define __PLATFORM_H_

/*** Interrupt priorities ***/
/* Interrupt priorities.  Low numbers are high priority.
 * ((1 << LEVEL) + SUBLEVEL) are assigned to the system interrupts.
 * values to use for LEVEL and SUBLEVEL:
 *      7 >=  LEVEL   >= 4
 *     15 >= SUBLEVEL >= 0
 */
#define IRQ_PRI_TIMER       ((1 << 7) + 1)
#define IRQ_PRI_I2C         ((1 << 6) + 3)
#define IRQ_PRI_DMA_I2C     ((1 << 6) + 2)
#define IRQ_PRI_ER_I2C      ((1 << 6) + 1)
#define IRQ_PRI_USB         ((1 << 4) + 1)

#define USB_1_0_STANDARD 0x0100
#define USB_1_1_STANDARD 0x0101
#define USB_2_0_STANDARD 0x0200
#define EP_ADDR_IN  0x80
#define EP_ADDR_OUT 0x00

#define NOVATECH_VENDOR_ID  0x2AEB
// board #: 750-133
#define USB_PRODUCT_ID 133
#define USB_BCD_VERSION_NUM ((VERSION_MAJOR << 8) | VERSION_MINOR)

#define MAX_TIMERS 5

#ifdef STM32F1
  #include "platform/STM32F1/platform_STM32F1.h"
  #include "platform/STM32F1/platform_io.h"
  #include "platform/STM32F1/platform_i2c_led.h"
  #include "platform/STM32F1/platform_usb.h"
  #include "platform/STM32F1/platform_nt_timer.h"
#endif

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#endif /* __PLATFORM_H_ */
