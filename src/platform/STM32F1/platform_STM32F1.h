/*  Pin Assignment : STM32F103 RCT6
 *  Pin         Assignment
 *  31  VSS1    Ground
 *  32  VDD1    3.3V
 *  47  VSS2    Ground
 *  48  VDD2    3.3V
 *  63  VSS3    Ground
 *  64  VDD3    3.3V
 *  18  VSS4    Ground
 *  19  VDD4    3.3V
 *  01  VBAT    3.3V
 *  12  VSSa    Ground
 *  13  VDDa    3.3V
 *  
 *  60  BOOT0   Boot0
 *  07  RESET   JTAG/Reset
 *  05  PD0     OscIn
 *  06  PD1     OscOut
 *  54  PD2     Lock_OUT
 *  
 *  14  PA0     IRIG
 *  15  PA1     
 *  16  PA2     
 *  17  PA3     
 *  20  PA4     
 *  21  PA5     
 *  22  PA6     
 *  23  PA7     
 *  41  PA8     USB-ENUM
 *  42  PA9     
 *  43  PA10    
 *  44  PA11    USB D-
 *  45  PA12    USB D+
 *  46  PA13    JTAG
 *  49  PA14    JTAG
 *  50  PA15    JTAG
 *  
 *  26  PB0     
 *  27  PB1     
 *  28  PB2     Boot1/DFU
 *  55  PB3     JTAG
 *  56  PB4     JTAG
 *  57  PB5     
 *  58  PB6     
 *  59  PB7     
 *  61  PB8     
 *  62  PB9     
 *  29  PB10    I2C_SCL
 *  30  PB11    I2C_SDA
 *  33  PB12    OUTPUT1
 *  34  PB13    OUTPUT2
 *  35  PB14    OUTPUT3
 *  36  PB15    OUTPUT4
 *  
 *  08  PC0     INPUT
 *  09  PC1     INPUT
 *  10  PC2     INPUT
 *  11  PC3     INPUT
 *  24  PC4     INPUT
 *  25  PC5     INPUT
 *  37  PC6     INPUT
 *  38  PC7     INPUT
 *  39  PC8     INPUT
 *  40  PC9     INPUT
 *  51  PC10    INPUT
 *  52  PC11    INPUT
 *  53  PC12    INPUT
 *  02  PC13    INPUT
 *  03  PC14    INPUT
 *  04  PC15    INPUT
 */


#ifndef __PLATFORM_STM32F1_H_
#define __PLATFORM_STM32F1_H_

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
//#include <libopencm3/usb/usbd.h>


void platform_reset_hardware(void);
void platform_init(void);
void platform_delay(uint32_t delay);
//void assert_boot_pin(void);


#ifdef INLINE_GPIO
/* Apply soime optimizations that are in the library libopencm3 */
inline void _gpio_set(uint32_t gpioport, uint16_t gpios) {
        GPIO_BSRR(gpioport) = gpios;
}
#define gpio_set _gpio_set

inline void _gpio_clear(uint32_t gpioport, uint16_t gpios) {
        GPIO_BRR(gpioport) = gpios;
}
#define gpio_clear _gpio_clear

inline uint16_t _gpio_get(uint32_t gpioport, uint16_t gpios) {
        return (uint16_t)GPIO_IDR(gpioport) & gpios;
}
#define gpio_get _gpio_get
#endif /* INLINE_GPIO */


#endif /* __PLATFORM_STM32F1_H_ */
