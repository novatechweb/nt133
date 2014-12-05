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


/* Unused pins */
#define UNUSED_PORTA_PIN1 GPIO1
#define UNUSED_PORTA_PIN2 GPIO2
#define UNUSED_PORTA_PIN3 GPIO3
#define UNUSED_PORTA_PIN4 GPIO4
#define UNUSED_PORTA_PIN5 GPIO5
#define UNUSED_PORTA_PIN6 GPIO6
#define UNUSED_PORTA_PIN7 GPIO7
#define UNUSED_PORTA_PIN8 GPIO9
#define UNUSED_PORTA_PIN9 GPIO10
#define UNUSED_PORTB_PIN1 GPIO0
#define UNUSED_PORTB_PIN2 GPIO1
#define UNUSED_PORTB_PIN3 GPIO5
#define UNUSED_PORTB_PIN4 GPIO6
#define UNUSED_PORTB_PIN5 GPIO7
#define UNUSED_PORTB_PIN6 GPIO8
#define UNUSED_PORTB_PIN7 GPIO9

#define DEBUG_PORT_A GPIOA
#define DEBUG_PA1    GPIO1
#define DEBUG_PA2    GPIO2
#define DEBUG_PA3    GPIO3
#define DEBUG_PA4    GPIO4
#define DEBUG_PA5    GPIO5
#define DEBUG_PA6    GPIO6
#define DEBUG_PA7    GPIO7
#define DEBUG_PA9    GPIO9
#define DEBUG_PA10   GPIO10

#ifdef DEBUG
 #define DBG_TOGGLE_PA1()   { gpio_toggle(DEBUG_PORT_A, DEBUG_PA1); }
 #define DBG_TOGGLE_PA2()   { gpio_toggle(DEBUG_PORT_A, DEBUG_PA2); }
 #define DBG_TOGGLE_PA3()   { gpio_toggle(DEBUG_PORT_A, DEBUG_PA3); }
 #define DBG_TOGGLE_PA4()   { gpio_toggle(DEBUG_PORT_A, DEBUG_PA4); }
 #define DBG_USB_ISR()      { gpio_toggle(DEBUG_PORT_A, DEBUG_PA5); }
 #define DBG_I2C_ERR_ISR()  { gpio_toggle(DEBUG_PORT_A, DEBUG_PA6); }
 #define DBG_I2C_DMA_ISR()  { gpio_toggle(DEBUG_PORT_A, DEBUG_PA7); }
 #define DBG_I2C_EVT_ISR()  { gpio_toggle(DEBUG_PORT_A, DEBUG_PA9); }
 #define DBG_TIMER_ISR()    { gpio_toggle(DEBUG_PORT_A, DEBUG_PA10); }
#else
 #define DBG_TOGGLE_PA1()   { ; }
 #define DBG_TOGGLE_PA2()   { ; }
 #define DBG_TOGGLE_PA3()   { ; }
 #define DBG_TOGGLE_PA4()   { ; }
 #define DBG_USB_ISR()      { ; }
 #define DBG_I2C_ERR_ISR()  { ; }
 #define DBG_I2C_DMA_ISR()  { ; }
 #define DBG_I2C_EVT_ISR()  { ; }
 #define DBG_TIMER_ISR()    { ; }
#endif


void platform_reset_hardware(void);
void platform_init(void);
void platform_delay(uint32_t delay);


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

inline uint16_t _gpio_port_write_value(uint32_t gpioport, uint16_t gpios) {
        return (uint16_t)GPIO_ODR(gpioport) & gpios;
}
#define gpio_port_write_value _gpio_port_write_value


#endif /* __PLATFORM_STM32F1_H_ */
