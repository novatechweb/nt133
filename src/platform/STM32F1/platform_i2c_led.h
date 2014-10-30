#ifndef __PLATFORM_I2C_LED_H_
#define __PLATFORM_I2C_LED_H_

#include <stdint.h>

#define I2C_PORT I2C2
#define I2C_SDA_PIN GPIO_I2C2_SDA
#define I2C_SCL_PIN GPIO_I2C2_SCL

#define SYSTICK_IRQ   sys_tick_handler
#define I2C_EVENT_IRQ i2c2_ev_isr
#define I2C_ERROR_IRQ i2c2_er_isr

#define I2C_EV_IRQ NVIC_I2C2_EV_IRQ
#define I2C_ER_IRQ NVIC_I2C2_ER_IRQ

void i2c_led_reset_hardware(void);
void reset_i2c_pins(void);
void i2c_led_platform_init(void);

#endif /* __PLATFORM_I2C_LED_H_ */