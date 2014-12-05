#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#include "platform_i2c_led.h"
#include "../platform.h"

enum I2C_FREQ {
	I2C_400KHz,
	I2C_100KHz,
	I2C_53KHz,
	I2C_50KHz,
};

void i2c_led_reset_hardware(void)
{
	i2c_reset(I2C_PORT);
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

static void setup_i2c_port(enum I2C_FREQ I2C_speed)
{
	// Disable I2C if it happens to be enabled
	i2c_peripheral_disable(I2C_PORT);
	dma_channel_reset(I2C_TX_DMA, I2C_TX_DMA_CHANNEL);
	i2c_disable_interrupt(I2C_PORT, (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN));
	DISABLE_I2C_INTERRUPT();
	reset_i2c_pins();

	// set: Source, Destination, and Amount (DMA channel must be disabled)
	dma_set_peripheral_address(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, (uint32_t)&I2C_DR(I2C_PORT));
	dma_set_memory_address(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, 0);
	dma_set_number_of_data(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, 0);
	// set the DMA Configuration (DMA_CCRx)
	//			 (BIT 14) mem2mem_mode disabled
	dma_set_priority(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_CCR_PL_HIGH); // (BIT 12:13)
	dma_set_memory_size(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_CCR_MSIZE_8BIT); // (BIT 10:11)
	dma_set_peripheral_size(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_CCR_PSIZE_8BIT); // (BIT 8:9)
	dma_enable_memory_increment_mode(I2C_TX_DMA, I2C_TX_DMA_CHANNEL); // (BIT 7)
	dma_disable_peripheral_increment_mode(I2C_TX_DMA, I2C_TX_DMA_CHANNEL); // (BIT 6)
	//			 (BIT 5) Circular mode is disabled
	dma_set_read_from_memory(I2C_TX_DMA, I2C_TX_DMA_CHANNEL);	// (BIT 4)
	dma_enable_transfer_error_interrupt(I2C_TX_DMA, I2C_TX_DMA_CHANNEL); // (BIT 3)
	dma_disable_half_transfer_interrupt(I2C_TX_DMA, I2C_TX_DMA_CHANNEL); // (BIT 2)
	dma_enable_transfer_complete_interrupt(I2C_TX_DMA, I2C_TX_DMA_CHANNEL); // (BIT 1)

	// This is the slave address when not transmitting data
	i2c_set_own_7bit_slave_address(I2C_PORT, 0x32);
	// do not respond to the specified slave address
	i2c_disable_ack(I2C_PORT);
	// Use DMA to send I2C data
	i2c_enable_dma(I2C_PORT);
	// set which interrupts I2C uses
	i2c_enable_interrupt(I2C_PORT, (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN));
	// APB1 is running at 36MHz = T(PCLK1) = 1/36000000 sec.
	i2c_set_clock_frequency(I2C_PORT, I2C_CR2_FREQ_36MHZ);
	// Set up the hardware for the particular speed
	switch (I2C_speed) {
		// Values found on Internet for the I2C standard
		//   STANDARD : SCL max rise time = 1000ns = 1000/1000000000 sec
		//       FAST : SCL max rise time =  300ns =  300/1000000000 sec
		// 
		// DATASHEET Function:
		//   TRISE = (T(MAX_SCL_RISE) / T(PCLK1)) + 1
		// 
		// DATASHEET Functions:
		//   STANDARD :      
		//     T(high) =      CCR * T(PCLK1)
		//     T(low)  =      CCR * T(PCLK1)
		//   FAST (DUTY=I2C_CCR_DUTY_DIV2)
		//     T(high) =      CCR * T(PCLK1)
		//     T(low)  =  2 * CCR * T(PCLK1)
		//   FAST (DUTY=I2C_CCR_DUTY_16_DIV_9)   [To reach 400KHz]
		//     T(high) =  9 * CCR * T(PCLK1)
		//     T(low)  = 16 * CCR * T(PCLK1)
		// 
		// I2C PERIOD:
		//   STANDARD
		//     PERIOD = T(high) + T(low) = (2 * CCR * T(PCLK1))
		//   FAST (DUTY=I2C_CCR_DUTY_DIV2)
		//     PERIOD = T(high) + T(low) = (3 * CCR * T(PCLK1))
		//   FAST (DUTY=I2C_CCR_DUTY_16_DIV_9)
		//     PERIOD = T(high) + T(low) = (25 * CCR * T(PCLK1))
		case I2C_400KHz:
			// I2C PERIOD: 400KHz = 400000Hz = 1/400000 sec.
			i2c_set_fast_mode(I2C_PORT);
			// I2C_CCR_DUTY_DIV2 or I2C_CCR_DUTY_16_DIV_9
			i2c_set_dutycycle(I2C_PORT, I2C_CCR_DUTY_16_DIV_9);
			// CCR = PERIOD / (25 * T(PCLK1))
			// CCR = (1/400000) / (25/36000000) = 18/5 = 3.6
			// CCR = 4 => I2C PERIOD = 360kHz
			// CCR = 3 => I2C PERIOD = 480kHz
			i2c_set_ccr(I2C_PORT, 4);	// Only fast mode can have a value less than 0x04
			// TRISE = ( (300/1000000000) / (1/36000000) ) + 1 = 59/5 = 11.8
			// TRISE = 12 => SCL max rise time ~= 305.555ns
			// TRISE = 11 => SCL max rise time ~= 277.777ns
			i2c_set_trise(I2C_PORT, 11);
			break;
		case I2C_100KHz:
			// I2C PERIOD: 100KHz = 100000Hz = 1/100000 sec.
			i2c_set_standard_mode(I2C_PORT);
			// I2C_CCR_DUTY_DIV2 or I2C_CCR_DUTY_16_DIV_9
			i2c_set_dutycycle(I2C_PORT, I2C_CCR_DUTY_DIV2);
			// CCR = PERIOD / (2 * T(PCLK1))
			// CCR = (1/100000) / (2/36000000) = 180
			i2c_set_ccr(I2C_PORT, 180);
			// TRISE = ( (1000/1000000000) / (1/36000000) ) + 1 = 37
			i2c_set_trise(I2C_PORT, 37);
			break;
		case I2C_53KHz:
			// ~= 52.91kHz is the slowest I could get to work
			// CCR value of 341 works but not 342 or higher
		case I2C_50KHz:
		default:
			// I2C PERIOD: 50KHz = 50000Hz = 1/50000 sec.
			i2c_set_standard_mode(I2C_PORT);
			// I2C_CCR_DUTY_DIV2 or I2C_CCR_DUTY_16_DIV_9
			i2c_set_dutycycle(I2C_PORT, I2C_CCR_DUTY_DIV2);
			// CCR = PERIOD / (2 * T(PCLK1))
			// CCR = (1/50000) / (2/36000000) = 360
			//   (341 works but not 342 or higher)
			i2c_set_ccr(I2C_PORT, 341);
			// TRISE = ( (1000/1000000000) / (1/36000000) ) + 1 = 37
			i2c_set_trise(I2C_PORT, 37);
			break;
	}
	i2c_peripheral_enable(I2C_PORT);

	// set the priorities for the interrupts
	nvic_set_priority(I2C_EV_IRQ, IRQ_PRI_I2C);
	nvic_set_priority(I2C_ER_IRQ, IRQ_PRI_ER_I2C);
	nvic_set_priority(I2C_DMA_IRQ, IRQ_PRI_DMA_I2C);
}

void i2c_led_platform_init(void)
{
	// Enable I2C2
	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_DMA1);

	// Setup I2C port
	setup_i2c_port(I2C_400KHz);
}
