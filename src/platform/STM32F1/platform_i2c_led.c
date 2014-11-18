#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/nvic.h>

#include "platform_i2c_led.h"
#include "../platform.h"

void i2c_led_reset_hardware(void)
{
	i2c_reset(I2C_PORT);
	systick_clear();
}

void reset_i2c_pins(void)
{
	// reseting I2C pins [errata_sheet CD00197763.pdf section 2.14.7]
	i2c_peripheral_disable(I2C_PORT);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, I2C_SCL_PIN | I2C_SDA_PIN);
	gpio_set(GPIOB, I2C_SCL_PIN | I2C_SDA_PIN);
	while (!gpio_get(GPIOB, I2C_SCL_PIN | I2C_SDA_PIN));
	gpio_clear(GPIOB, I2C_SCL_PIN | I2C_SDA_PIN);
	while (gpio_get(GPIOB, I2C_SCL_PIN | I2C_SDA_PIN));
	gpio_set(GPIOB, I2C_SDA_PIN);
	while (!gpio_get(GPIOB, I2C_SDA_PIN));
	gpio_clear(GPIOB, I2C_SDA_PIN);
	while (gpio_get(GPIOB, I2C_SDA_PIN));
	gpio_set(GPIOB, I2C_SCL_PIN);
	while (!gpio_get(GPIOB, I2C_SCL_PIN));
	gpio_clear(GPIOB, I2C_SCL_PIN);
	while (gpio_get(GPIOB, I2C_SCL_PIN));
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, I2C_SCL_PIN | I2C_SDA_PIN);
	I2C_CR1(I2C_PORT) |= I2C_CR1_SWRST;
	I2C_CR1(I2C_PORT) &= ~I2C_CR1_SWRST;
	i2c_peripheral_enable(I2C_PORT);
}

static void setup_i2c_port(uint32_t i2c)
{
	// make certain interrupts are disabled
	nvic_disable_irq(I2C_EV_IRQ);
	nvic_disable_irq(I2C_ER_IRQ);

	reset_i2c_pins();

	i2c_peripheral_disable(i2c);
	// APB1 is running at 36MHz.
	i2c_set_clock_frequency(i2c, I2C_CR2_FREQ_36MHZ);
	// 400KHz - I2C Fast Mode
	i2c_set_fast_mode(i2c);
	// I2C_CCR_DUTY_DIV2 or I2C_CCR_DUTY_16_DIV_9
	i2c_set_dutycycle(i2c, I2C_CCR_DUTY_DIV2);
	// * T(PCLK1) = APB low-speed clock
	// * SMBUS or Standard mode
	// * -   T(high) = CCR * T(PLLK1)
	// * -   T(low) = CCR * T(PLLK1)
	// * Fast mode
	// *   DUTY = 0:
	// *   -   T(high) = CCR * T(PLLK1)
	// *   -   T(low) = 2 * CCR * T(PLLK1)
	// *   DUTY = 1:
	// *   -   T(high) = 9 * CCR * T(PLLK1)
	// *   -   T(low) = 16 * CCR * T(PLLK1)
	// * 
	// * 400kHz SCL Freq
	// * 400kHz = 1/400000 = 0.0000025 = 2.5 us = 2500ns
	// * (Fast mode, DUTY = 0)
	// *   T(high) + T(low) = 2500ns ==> CCR * T(PLLK1) + 2 * CCR * T(PLLK1) = 2500ns ==> CCR = 2500ns / 3 * T(PLLK1)
	// *   T(high) = 2500ns - T(low) = 2500ns - 2 * CCR * T(PLLK1) = 2500ns - 2 * (2500ns / 3 * T(PLLK1)) * T(PLLK1)
	// *     T(high) = (25s/10000000) - 2 * ((25s/10000000) / 3 * T(PLLK1)) * T(PLLK1) = (1s/400000) - (1s/600000) * T(PLLK1) * T(PLLK1)
	// * Given:
	// *   FREQ = 0x24 = 36MHz
	// *   T(PCLK1) = 1/36MHz = 1s/36000000
	// * Therefore:
	// *   T(high) = CCR * T(PLLK1)
	// *   CCR = T(high) / T(PLLK1) = ((1s/400000) - (1s/600000) * T(PLLK1) * T(PLLK1)) / T(PLLK1)
	// *   CCR = (3 - 2 * T(PLLK1) * T(PLLK1)) / (1200000 * T(PLLK1))
	// *   CCR = (3 - 2 * (1s/36000000) * (1s/36000000)) / (1200000 * (1s/36000000))
	// *   CCR = 1822499999999999 / 20250000000000 ~= 89.99999999999996 ~= 90 = 0x5A
	i2c_set_ccr(i2c, 0x5A);
	// * (Fast mode)
	// *   Max. SCL rise time = 300ns = 3/10000000s  = T(MAX_SCL_RISE)
	// * Given:
	// *     FREQ = 0x24 = 36MHz
	// *     T(PCLK1) = 1/36MHz = 1s/36000000 ~= 27.777777777777777ns
	// * Therefore:
	// *     TRISE = (T(MAX_SCL_RISE) / T(PCLK1)) + 1 = ((3/10000000) / (1/36000000)) + 1 = 54/5 + 1 = 59/5 = 11.8 ~= 12 = 0x0C
	i2c_set_trise(i2c, 0x0C);
	// This is our slave address - needed only if we want to receive from other masters.
	i2c_set_own_7bit_slave_address(i2c, 0x32);
	// set which interrupts we are using
	i2c_enable_interrupt(i2c, (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN));
	// set the priorities for the interrupts
	nvic_set_priority(I2C_EV_IRQ, IRQ_PRI_I2C);
	nvic_set_priority(I2C_ER_IRQ, IRQ_PRI_ER_I2C);
	// enable I2C
	i2c_peripheral_enable(i2c);
	// enable IRQ
	nvic_enable_irq(I2C_ER_IRQ);
	nvic_enable_irq(I2C_EV_IRQ);
}

static void setup_systick(void)
{
	systick_set_frequency(450, 72000000);
	
	// Set priority of systic
	// NOTE: I do not know how this priority pertains to all the others (It was copied out of BlackMagic code))
	SCB_SHPR(11) &= ~(15 << 4);
	SCB_SHPR(11) |= IRQ_PRI_SYSTICK;
	
	// Start the systick
	systick_interrupt_enable();
	systick_counter_enable();
}

void i2c_led_platform_init(void)
{
	// Enable I2C2
	rcc_periph_clock_enable(RCC_I2C2);

	// Setup I2C port
	setup_i2c_port(I2C_PORT);
	// Setup systic hardware
	setup_systick();
}
