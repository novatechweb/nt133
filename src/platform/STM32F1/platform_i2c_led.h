#ifndef __PLATFORM_I2C_LED_H_
#define __PLATFORM_I2C_LED_H_

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>

#define I2C_PORT I2C2
#define I2C_SDA_PIN GPIO_I2C2_SDA
#define I2C_SCL_PIN GPIO_I2C2_SCL

#define I2C_EVENT_ISR i2c2_ev_isr
#define I2C_ERROR_ISR i2c2_er_isr
#define I2C_DMA_ISR dma1_channel4_isr

#define I2C_EV_IRQ NVIC_I2C2_EV_IRQ
#define I2C_ER_IRQ NVIC_I2C2_ER_IRQ
#define I2C_DMA_IRQ NVIC_DMA1_CHANNEL4_IRQ

#define I2C_TX_DMA DMA1
#define I2C_TX_DMA_CHANNEL DMA_CHANNEL4

#define DISABLE_I2C_INTERRUPT() {   \
	cm_disable_interrupts();        \
	nvic_disable_irq(I2C_EV_IRQ);   \
	nvic_disable_irq(I2C_DMA_IRQ);  \
	nvic_disable_irq(I2C_ER_IRQ);   \
	cm_enable_interrupts();         \
}
#define ENABLE_I2C_INTERRUPT() {    \
	cm_disable_interrupts();        \
	nvic_enable_irq(I2C_ER_IRQ);    \
	nvic_enable_irq(I2C_DMA_IRQ);   \
	nvic_enable_irq(I2C_EV_IRQ);    \
	cm_enable_interrupts();         \
}

void i2c_led_reset_hardware(void);
void reset_i2c_pins(void);
void i2c_led_platform_init(void);

#endif /* __PLATFORM_I2C_LED_H_ */