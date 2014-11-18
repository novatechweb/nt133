#ifndef __PLATFORM_NT_TIMER_H_
#define __PLATFORM_NT_TIMER_H_

#define NT_TIMER_IRQ tim2_isr
#define	DISABLE_NT_TIMER_INT() { nvic_disable_irq(NVIC_TIM2_IRQ); }
#define	ENABLE_NT_TIMER_INT()  { nvic_enable_irq(NVIC_TIM2_IRQ); }
// I do not understand why the library call causes blocking_handler() to be run
//#define	CLEAR_NT_TIMER_FLAG()  { timer_clear_flag(TIM2, TIM_SR_UIF); }
#define	CLEAR_NT_TIMER_FLAG()  { TIM_SR(TIM2) &= ~TIM_SR_UIF; }

void nt_timer_reset_hardware(void);
void nt_timer_platform_init(void);

#endif /* __PLATFORM_NT_TIMER_H_ */