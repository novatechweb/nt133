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

//NEW_CODE: Added to support new i/o board
#define PPS_TICKS               8000    // 8,000 iterations of a 125us timer in 1 sec
#define PPS_TICK_SLOP           3       // 125us x 3 ==> 375us
#define TICKS_PER_125_MICRO_SEC 9000    // 9,000 --> 1/72MHz = 13.88ns, 13.88ns x 9000 = 125us
                                        // used in timer_set_period(TIM1, TICKS_PER_125_MICRO_SEC);
                                        // a value of 719 works better by experiment.

#define PPS_TICKS_BUFFER_DEPTH  8
#define PPS_TICKS_BUFFER_AVG    3

#define PPS_CC_IRQ tim1_cc_isr
#define PPS_UP_IRQ tim1_up_isr

#define	DISABLE_TIM1_2() { nvic_disable_irq(NVIC_TIM1_CC_IRQ); }
#define	ENABLE_TIM1_2()  { nvic_enable_irq(NVIC_TIM1_CC_IRQ); }

#define	DISABLE_TIM1_2_UP() { nvic_disable_irq(NVIC_TIM1_UP_IRQ); }
#define	ENABLE_TIM1_2_UP()  { nvic_enable_irq(NVIC_TIM1_UP_IRQ); }

void tim1_init_pps_timer(void);
enum arr_states
{
    IDLE = 0,
    SLOW,
    FAST,
    ERROR,
};

enum pps_status
{
    GOOD = 1,
    OUT_OF_SYNCH,
    BAD,
};

typedef struct 
{
    uint16_t pin_state;
    uint16_t seconds;
    uint32_t micro_sec_counts;  //multiply value x 125us
    uint32_t nano_sec_counts;   //multiply value x 13.9ns, probably not required
    enum pps_status  quality;
} binary_input_event;
 
extern volatile uint32_t us125_tick_counter;
extern volatile uint32_t seconds_tick_counter;
extern uint32_t arrl;
extern enum pps_status pps_quality;

//NEW_CODE: End    
    

#endif /* __PLATFORM_NT_TIMER_H_ */
