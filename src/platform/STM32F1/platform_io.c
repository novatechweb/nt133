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


//NEW_CODE: Added to support new i/o board
// Disabled code is from existing i/o board
#if 0
void io_platform_init(void)
{
	// OUTPUT PINS
	gpio_set_mode(OUTPUT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
		(OUTPUT_PORT_PIN1 | OUTPUT_PORT_PIN2 | OUTPUT_PORT_PIN3 | OUTPUT_PORT_PIN4));
	gpio_set_mode(LOCKOUT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LOCKOUT_PIN);
	// INPUT PINS
	gpio_set_mode(INPUT_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_ALL);
}
#else
void io_platform_init(void)
{
    uint16_t input_pins;	

    // INPUT PINS
    input_pins = GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO6 | GPIO7; //|GPIO8 | GPIO9;
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, input_pins);

    input_pins = GPIO0 | GPIO1 | GPIO6 | GPIO7 | GPIO8 | GPIO9;
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, input_pins);
    
    input_pins = GPIO6 | GPIO7 | GPIO8 | GPIO9;
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, input_pins);  


    // OUTPUT PINS
    // RNGN0 --> binary input threshold control
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);  //GPIO_CNF_OUTPUT_OPENDRAIN   GPIO_CNF_OUTPUT_PUSHPULL


    //gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO_BANK_TIM1_CH2);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO8);

}
#endif
//NEW_CODE: End



