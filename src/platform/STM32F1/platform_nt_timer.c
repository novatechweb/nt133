#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>

#include "platform_nt_timer.h"
#include "../platform.h"


void nt_timer_reset_hardware(void)
{
	rcc_peripheral_reset(&RCC_APB2RSTR, RCC_APB2RSTR_TIM1RST);
	rcc_peripheral_clear_reset(&RCC_APB2RSTR, RCC_APB2RSTR_TIM1RST);
}

void nt_timer_platform_init(void)
{
	DISABLE_NT_TIMER_INT();
	timer_reset(TIM2);
        
	nvic_set_priority(NVIC_TIM2_IRQ, IRQ_PRI_TIMER);
	rcc_periph_clock_enable(RCC_TIM2);
	timer_disable_counter(TIM2);
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_enable_preload(TIM2);
	timer_continuous_mode(TIM2);
        
        // Set up for 100uS ticker
	// CK_PSC = CK_INT = CLK APB1 = 72MHz
	// CK_CNT = 1/1usec = 1MHz
	timer_set_prescaler(TIM2, 71);  // TIM_PSC(TMR2) = 71;    // TIM2_PSC = (CK_PSC/CK_CNT)-1 = 71
	timer_set_period(TIM2, 100);    // TIM_ARR(TMR2) = 100;   // 10/msec == 100 * CK_CNT
	timer_set_counter(TIM2, 0);     // TIM_CNT(TMR2) = 0;
	timer_enable_irq(TIM2, TIM_DIER_UIE);


	CLEAR_NT_TIMER_FLAG();
	timer_enable_counter(TIM2);
	ENABLE_NT_TIMER_INT();
}


//NEW_CODE: Added to support new i/o board
//capture compare of 1pps, 1 pulse per second.
//when pps signal is received, CC_IRQ is called.
void tim1_init_pps_timer (void)
{
        rcc_peripheral_disable_clock(&RCC_APB2ENR,RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN);
        rcc_peripheral_disable_clock(&RCC_APB2ENR,RCC_APB2ENR_TIM1EN);
        nvic_disable_irq(NVIC_TIM1_CC_IRQ);
        

        timer_ic_disable(TIM1,TIM_IC2);  // disable CCER
        timer_disable_counter(TIM1);     // disable CR1
        timer_slave_set_mode(TIM1,TIM_SMCR_SMS_OFF); // turn off slave mode
                
        nvic_set_priority(NVIC_TIM1_UP_IRQ, IRQ_PRI_PPS_UP);
        nvic_set_priority(NVIC_TIM1_CC_IRQ, IRQ_PRI_PPS_CC);        
        
        
        rcc_peripheral_enable_clock(&RCC_APB2ENR,RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN);
        
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_BANK_TIM1_CH2);
        
        rcc_peripheral_enable_clock(&RCC_APB2ENR,RCC_APB2ENR_TIM1EN);
        
        timer_reset(TIM1);
        timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        timer_enable_preload(TIM1);   //===> L@@k
 
        timer_ic_set_input(TIM1,TIM_IC2,TIM_IC_IN_TI2);  // config CCMR, write CC2S for input capture 1 as input
        timer_ic_set_filter(TIM1,TIM_IC2,TIM_CCMR1_IC1F_CK_INT_N_8); // filter duration
        
        timer_ic_enable(TIM1,TIM_IC2);	//CCER Enable
        

  		timer_set_period(TIM1, TICKS_PER_125_MICRO_SEC);
		timer_set_counter(TIM1, 0);
        timer_enable_counter(TIM1);  //Enable CR
        
        timer_enable_irq (TIM1,TIM_DIER_CC2IE);
        timer_enable_irq (TIM1,TIM_DIER_UIE);

         
        TIM_SR(TIM1) &= ~TIM_SR_UIF;
        nvic_enable_irq (NVIC_TIM1_CC_IRQ);
        ENABLE_TIM1_2_UP();
 }

 
//NEW_CODE: End




