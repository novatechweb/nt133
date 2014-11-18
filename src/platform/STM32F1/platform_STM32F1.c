#include <stdint.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>

#include "platform_STM32F1.h"
#include "platform_i2c_led.h"
#include "platform_io.h"
#include "platform_nt_timer.h"
#include "platform_usb.h"

void platform_reset_hardware(void)
{
	rcc_peripheral_reset(&RCC_APB2RSTR,
		RCC_APB2RSTR_IOPDRST | RCC_APB2RSTR_IOPCRST |
		RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_IOPARST |
		RCC_APB2RSTR_AFIORST);
	rcc_peripheral_clear_reset(&RCC_APB2RSTR,
		RCC_APB2RSTR_IOPDRST | RCC_APB2RSTR_IOPCRST |
		RCC_APB2RSTR_IOPBRST | RCC_APB2RSTR_IOPARST |
		RCC_APB2RSTR_AFIORST);
	io_reset_hardware();
	usb_reset_hardware();
	i2c_led_reset_hardware();
	nt_timer_reset_hardware();
}

void platform_init(void)
{
	// Setup oscillator
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

 #ifdef BOOTLOADER
	// Relocate interrupt vector table here
	SCB_VTOR = 0x2000;
 #endif

	// Enable clocks for all GPIO ports and their Alternate function.
	rcc_peripheral_enable_clock(&RCC_APB2ENR,
	    RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
	    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
	    RCC_APB2ENR_AFIOEN);

	// Set all port pins to input pull-down
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_ALL);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_ALL);
	gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_ALL);
	gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO2);
	gpio_clear(GPIOA, GPIO_ALL);
	gpio_clear(GPIOB, GPIO_ALL);
	gpio_clear(GPIOC, GPIO_ALL);
	gpio_clear(GPIOD, GPIO2);

	// 31.4.3 - Internal pull-up and pull-down on JTAG pins
	// ● NJTRST: Input pull-up
	gpio_set_mode(GPIO_BANK_JNTRST, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_JNTRST);
	gpio_set(GPIO_BANK_JNTRST, GPIO_JNTRST);
	// ● JTDI: Input pull-up
	gpio_set_mode(GPIO_BANK_JTDI, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_JTDI);
	gpio_set(GPIO_BANK_JTDI, GPIO_JTDI);
	// ● JTMS/SWDIO: Input pull-up
	gpio_set_mode(GPIO_BANK_JTMS_SWDIO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_JTMS_SWDIO);
	gpio_set(GPIO_BANK_JTMS_SWDIO, GPIO_JTMS_SWDIO);
	// ● JTCK/SWCLK: Input pull-down
	gpio_set_mode(GPIO_BANK_JTCK_SWCLK, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_JTCK_SWCLK);
	gpio_clear(GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK);
	// ● JTDO: Input floating - Already set
	
	{	// unused pins
		gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 
			UNUSED_PORTA_PIN1 | UNUSED_PORTA_PIN2 | UNUSED_PORTA_PIN3 |
			UNUSED_PORTA_PIN4 | UNUSED_PORTA_PIN5 | UNUSED_PORTA_PIN6 |
			UNUSED_PORTA_PIN7 | UNUSED_PORTA_PIN8 | UNUSED_PORTA_PIN9);
		gpio_clear(GPIOA, UNUSED_PORTA_PIN1 | UNUSED_PORTA_PIN2 | UNUSED_PORTA_PIN3 |
			UNUSED_PORTA_PIN4 | UNUSED_PORTA_PIN5 | UNUSED_PORTA_PIN6 |
			UNUSED_PORTA_PIN7 | UNUSED_PORTA_PIN8 | UNUSED_PORTA_PIN9);
		gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
			UNUSED_PORTB_PIN1 | UNUSED_PORTB_PIN2 | UNUSED_PORTB_PIN3 |
			UNUSED_PORTB_PIN4 | UNUSED_PORTB_PIN5 | UNUSED_PORTB_PIN6 |
			UNUSED_PORTB_PIN7);
		gpio_clear(GPIOB, UNUSED_PORTB_PIN1 | UNUSED_PORTB_PIN2 | UNUSED_PORTB_PIN3 |
			UNUSED_PORTB_PIN4 | UNUSED_PORTB_PIN5 | UNUSED_PORTB_PIN6 |
			UNUSED_PORTB_PIN7);
	}
}
