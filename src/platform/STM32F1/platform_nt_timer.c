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
	// CK_PSC = CK_INT = CLK APB1 = 72MHz
	// CK_CNT = 1/1usec = 1MHz
	timer_set_prescaler(TIM2, 71);  // TIM_PSC(TMR2) = 71;      // TIM2_PSC = (CK_PSC/CK_CNT)-1 = 71
	timer_set_period(TIM2, 100);  // TIM_ARR(TMR2) = 100;   // 10/msec == 100 * CK_CNT
	timer_set_counter(TIM2, 0);     // TIM_CNT(TMR2) = 0;
	timer_enable_irq(TIM2, TIM_DIER_UIE);
	CLEAR_NT_TIMER_FLAG();
	timer_enable_counter(TIM2);
	ENABLE_NT_TIMER_INT();
}
