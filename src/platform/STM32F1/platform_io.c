#include <stdint.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include "platform_io.h"

void io_reset_hardware(void)
{
	rcc_peripheral_reset(&RCC_APB2RSTR, RCC_APB2RSTR_IOPARST |
		RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_IOPCRST | RCC_APB2RSTR_IOPDRST);
	rcc_peripheral_clear_reset(&RCC_APB2RSTR,RCC_APB2RSTR_IOPARST |
		RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_IOPCRST | RCC_APB2RSTR_IOPDRST);
}

void io_platform_init(void)
{
	// OUTPUT PINS
	gpio_set_mode(OUTPUT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
		(OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
	gpio_set_mode(LOCKOUT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LOCKOUT_PIN);
	// INPUT PINS
	gpio_set_mode(INPUT_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_ALL);
}
