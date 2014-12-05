#include <stdint.h>
#include <strings.h>
#include <stdbool.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>

#include <platform.h>
#include <io.h>
#include "i2c_led.h"

// Address for the IO expander that controlls the LEDs
#define PCA8575PW_7BIT_ADDRESS (0x40 >> 1)

// ROW Table Data
const uint8_t row_data_table[] = {
	0x7e, // 0b01111110
	0x7d, // 0b01111101
	0x7b, // 0b01111011
	0x6f, // 0b01101111
	0x5f, // 0b01011111
};
// Table used to translate from led_data to the byte send to the I2C LEDs
const uint8_t translate_table[] = {
	0x1f, // 0b00011111 [       ] ( 0)
	0x17, // 0b00010111 [1      ] ( 1)
	0x1b, // 0b00011011 [  2    ] ( 2)
	0x13, // 0b00010011 [1,2    ] ( 3)
	0x1d, // 0b00011101 [    3  ] ( 4)
	0x15, // 0b00010101 [1,  3  ] ( 5)
	0x19, // 0b00011001 [  2,3  ] ( 6)
	0x11, // 0b00010001 [1,2,3  ] ( 7)
	0x1e, // 0b00011110 [      4] ( 8)
	0x16, // 0b00010110 [1,    4] ( 9)
	0x1a, // 0b00011010 [  2,  4] (10)
	0x12, // 0b00010010 [1,2,  4] (11)
	0x1c, // 0b00011100 [    3,4] (12)
	0x14, // 0b00010100 [1,  3,4] (13)
	0x18, // 0b00011000 [  2,3,4] (14)
	0x10, // 0b00010000 [1,2,3,4] (15)
};


// Data sent on the next I2C command
enum {
	COL_DATA_OFF,
	ROW_DATA_OFF,
	COL_DATA,
	ROW_DATA,
	I2C_DMA_BUFF_LEN,
};
uint8_t i2c_tx_dma_buff[I2C_DMA_BUFF_LEN];

enum I2C_STATE {
	I2C_idle,
	I2C_running,
	I2C_DMA_done,
	I2C_error,
};
enum I2C_STATE i2c_state = I2C_idle;

// Time (in 100's on microseconds) between the start of each I2C message
#define I2C_TIMER_TIME 33
// Timer handler for starting the I2C message
struct timer_handle_t i2c_timer;

static void start_i2c_message(struct timer_handle_t * UNUSED(cur_timer_handle))
{
	static uint8_t nibble_index = 0;
	uint32_t led_data;
	uint8_t temp_col_data;

	// Disable interrupts for the hardware
	DISABLE_I2C_INTERRUPT();
	if (i2c_state == I2C_error) {
		// Reset the I2C hardware
		i2c_led_reset_hardware();
		// setup the I2C hardware and state
		i2c_led_init();
		return;
	} else if (i2c_state != I2C_idle) {
		// Something went wrong with the I2C state
		// I2C will get reset
		i2c_state = I2C_error;
		// send the stop bit
		i2c_send_stop(I2C_PORT);
		return;
	}
	// Disable the I2C hardware
	i2c_peripheral_disable(I2C_PORT);
	dma_disable_channel(I2C_TX_DMA, I2C_TX_DMA_CHANNEL);

	// ***  Get the data to send  ***
	// increment to the next nibble
	nibble_index = ((nibble_index + 1) % sizeof(row_data_table));
	// get the state of the inputs
	get_led_data(&led_data);
	// get the nibble to be displayed on the LEDs
	temp_col_data = (uint8_t)(led_data >> (nibble_index * 4)) & 0xF;
	// Use the translate_table to convert the data for the I2C LEDs
	// (reverses the bit order so input 1 is on the top and not the bottom)
	i2c_tx_dma_buff[COL_DATA] = translate_table[temp_col_data];
	// Select which ROW will be displayed
	i2c_tx_dma_buff[ROW_DATA] = row_data_table[nibble_index];

	// ***  Setup DMA of the data to send  ***
	// set: Source, Destination, and Amount (DMA channel must be disabled)
	dma_set_peripheral_address(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, (uint32_t)&I2C_DR(I2C_PORT));
	dma_set_memory_address(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, (uint32_t)i2c_tx_dma_buff);
	dma_set_number_of_data(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, (uint16_t)I2C_DMA_BUFF_LEN);

	// ***  Start sending the message  ***
	i2c_state = I2C_running;
	// enable interrupts
	ENABLE_I2C_INTERRUPT();
	// enable I2C hardware
	i2c_peripheral_enable(I2C_PORT);
	// sending the I2C start
	i2c_send_start(I2C_PORT);
}

void I2C_EVENT_ISR(void)
{
	uint32_t reg32_sr1;

	DBG_I2C_EVT_ISR();
	DISABLE_I2C_INTERRUPT();

	// read Status Register
	reg32_sr1 = I2C_SR1(I2C_PORT);

	// ***  check the I2C interrupt error flags  ***
	if (reg32_sr1 & I2C_SR1_ADDR) {
		// read the second status register to clear I2C_SR1_ADDR
		I2C_SR2(I2C_PORT);
	}
	if ((reg32_sr1 & I2C_SR1_BERR) ||
		(reg32_sr1 & I2C_SR1_ARLO) ||
		(reg32_sr1 & I2C_SR1_AF))
	{
		// Bus error || Arbitration lost || Acknowledge failure
		i2c_state = I2C_error;
		// send the stop bit
		i2c_send_stop(I2C_PORT);
		// skip enabiling the interrupts
		goto finish;
	} else if (reg32_sr1 & I2C_SR1_BTF) {
		// Byte transfer finished
		i2c_state = I2C_idle;
		// send the stop bit
		i2c_send_stop(I2C_PORT);
		// skip enabiling the interrupts
		goto finish;
	} else if (reg32_sr1 & I2C_SR1_SB) {
		// Send the destination address.
		i2c_send_7bit_address(I2C_PORT, PCA8575PW_7BIT_ADDRESS, I2C_WRITE);
		// enable uart TX DMA channel
		dma_enable_channel(I2C_TX_DMA, I2C_TX_DMA_CHANNEL);
	}
	ENABLE_I2C_INTERRUPT();
 finish:
	DBG_I2C_EVT_ISR();
}

void I2C_DMA_ISR(void)
{
	DBG_I2C_DMA_ISR();
	DISABLE_I2C_INTERRUPT();
	if (dma_get_interrupt_flag(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_TCIF)) {
		// the transmit buffer is empty, clear the interupt flag
		dma_clear_interrupt_flags(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_TCIF);
		// finished with the DMA hardware
		dma_disable_channel(I2C_TX_DMA, I2C_TX_DMA_CHANNEL);
		i2c_state = I2C_DMA_done;
	}
	if (dma_get_interrupt_flag(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_TEIF)) {
		// Transfer Error Interrupt Flag, clear the flag
		dma_clear_interrupt_flags(I2C_TX_DMA, I2C_TX_DMA_CHANNEL, DMA_TEIF);
		// send the stop bit
		i2c_send_stop(I2C_PORT);
		i2c_state = I2C_error;
		// skip enabiling the interrupts
	} else {
		ENABLE_I2C_INTERRUPT();
	}
	DBG_I2C_DMA_ISR();
}

void I2C_ERROR_ISR(void)
{
	DBG_I2C_ERR_ISR();
	DISABLE_I2C_INTERRUPT();
	i2c_state = I2C_error;
	// send the stop bit
	i2c_send_stop(I2C_PORT);
	// skip enabiling the interrupts
	DBG_I2C_ERR_ISR();
}

void i2c_led_init(void)
{
	i2c_led_platform_init();
	i2c_state = I2C_idle;
	// initilize I2C TX DMA buffer
	i2c_tx_dma_buff[COL_DATA_OFF] = 0x1F;
	i2c_tx_dma_buff[ROW_DATA_OFF] = 0xFF;
	i2c_tx_dma_buff[COL_DATA] = translate_table[0];
	i2c_tx_dma_buff[ROW_DATA] = row_data_table[0];
	// Initilize I2C timer
	remove_timer(&i2c_timer);
	INIT_TIMER_HANDLE(i2c_timer, I2C_TIMER_TIME, true, start_i2c_message, NULL);
	add_timer(&i2c_timer);
}
