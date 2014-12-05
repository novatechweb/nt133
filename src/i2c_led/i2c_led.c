#include <stdint.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/i2c.h>

#include <platform.h>
#include <io.h>
#include "i2c_led.h"

#define PCA8574D_7BIT_ADDRESS (0x40 >> 1)

// The upper 4 bits (LED row) for each of the 4 bytes of the value sent to the LEDs
uint8_t i2c_row_values[] = {0xE0, 0xD0, 0xB0, 0x70};
// Data sent on the next I2C command
uint8_t i2c_data = 0;

static void device_PCA8574D_init(uint32_t i2c)
{
	// make certain interrupts are disabled
	nvic_disable_irq(I2C_EV_IRQ);
	nvic_disable_irq(I2C_ER_IRQ);
	// set the initial state of the I2C devices on the buss (using polling)
	i2c_send_start(i2c);
	while (!((I2C_SR1(i2c) & I2C_SR1_SB) & (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));
	i2c_send_7bit_address(i2c, PCA8574D_7BIT_ADDRESS, I2C_WRITE);
	while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
	I2C_SR2(i2c);
	i2c_send_data(i2c, 0xFF);
	while (!(I2C_SR1(i2c) & (I2C_SR1_BTF | I2C_SR1_TxE)));
	i2c_send_stop(i2c);
	while (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY)) {I2C_SR1(i2c);}
	// enable IRQ
	nvic_enable_irq(I2C_ER_IRQ);
	nvic_enable_irq(I2C_EV_IRQ);
}

void i2c_led_init(void)
{
	i2c_led_platform_init();
	// configure I2C device (PCA8574D)
	device_PCA8574D_init(I2C_PORT);
}


void SYSTICK_IRQ(void)
{
	static uint8_t row_index = 0;
	uint16_t led_status;
	uint8_t i2c_value;

	i2c_peripheral_disable(I2C2);
	// disable I2C event interrupt
	nvic_disable_irq(I2C_EV_IRQ);
	nvic_disable_irq(I2C_ER_IRQ);

	get_led_state(&led_status);
	// invert the bits to get the value for the I2C port expander
	led_status = ~led_status;
	// Select the COLUMN (least significant nibble) from the LED status
	i2c_value = 0x0F & (led_status >> (row_index * 4));
	// Select the ROW (most significant nibble) for the i2c_value
	i2c_value |= i2c_row_values[row_index];
	// increment row_index to the next ROW
	row_index = (row_index + 1) % 4;
	// send the values to I2C LEDs
	i2c_data = i2c_value;

	// enable I2C event interrupt
	nvic_enable_irq(I2C_ER_IRQ);
	nvic_enable_irq(I2C_EV_IRQ);
	// enable I2C
	i2c_peripheral_enable(I2C2);
	// start sending the I2C message
	i2c_send_start(I2C2);
}

void I2C_EVENT_IRQ(void)
{
	uint32_t reg32_sr1;
	uint8_t data;

	// read Status Register
	reg32_sr1 = I2C_SR1(I2C2);
	if (!reg32_sr1) {
		// There are two extra interrupts occuring that should not be
 		return;
	}

	if (reg32_sr1 & I2C_SR1_SB) {
		// SB=1: Start bit. Send the destination address.
		i2c_send_7bit_address(I2C2, PCA8574D_7BIT_ADDRESS, I2C_WRITE);
	} else if (reg32_sr1 & I2C_SR1_ADDR) {
		// read the second status register to clear ADDR=1
		I2C_SR2(I2C2);
		// ADDR=1: Address sent
		// get the data to send
		systick_interrupt_disable();
		data = i2c_data;
		systick_interrupt_enable();
		// Sending the data.
		i2c_send_data(I2C2, data);
		// Set STOP condition.
		i2c_send_stop(I2C2);
		nvic_disable_irq(I2C_ER_IRQ);
		nvic_disable_irq(I2C_EV_IRQ);
	}
}

void I2C_ERROR_IRQ(void)
{
	reset_i2c_pins();
	device_PCA8574D_init(I2C_PORT);
}
