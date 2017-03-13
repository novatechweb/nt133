#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>

#include <platform.h>
#include "nt_timer.h"
#include "io/io.h"      //NEW_CODE: Added to support new i/o board

// * * * * *   D E F I N I T I O N S  * * * * *

#ifndef MAX_TIMERS
 #define MAX_TIMERS 6
#endif

// * * * * *   G L O B A L   V A R I A B L E S  * * * * *

// should not be accessed or changed in any other ISR other than the NT_TIMER
// This will cause an issue if you want to create a timer in an interupt
// The data that is updated in this array could be interrupted in the middle
// of a change by any other interrupt handler.
struct timer_handle_t* timer_list[MAX_TIMERS];
// a counter that increments each NT_Timer tick
uint32_t tick_counter = 0;

//NEW_CODE: Added to support new i/o board
volatile uint32_t us125_tick_counter = 0; 
uint32_t prev_us125_tick_counter = 0; 

volatile uint32_t seconds_tick_counter = 0;
uint32_t arrl = TICKS_PER_125_MICRO_SEC;
uint32_t prev_arrl = TICKS_PER_125_MICRO_SEC;


enum arr_states tc = IDLE;
enum pps_status pps_quality = BAD;


void tweak_arrl (void);
//NEW_CODE: End

// ***************************************************************************
// * Description: Interupt service routine for tne NT_Timer                  *
// * Caveats: This ISR disabled global interrupts.                           *
// *     This is a low priority interrupt.                                   *
// ***************************************************************************
void NT_TIMER_IRQ(void)
{
	uint8_t timer_index;
	DBG_TIMER_ISR();
	// just a counter to know that this interrupt has been serviced
	tick_counter++;
	CLEAR_NT_TIMER_FLAG();
	// I do not want to block all interrupts while looping through the timer_list
	// therefore, all calls in this file should only happen in the main loop.
            
 
	for (timer_index=0; timer_index<MAX_TIMERS; timer_index++) {
		if (timer_list[timer_index] == NULL) {
			// Timer is not allocated. skip to next
			continue;
		}
		if (timer_list[timer_index]->tick_count == 0) {
			// This timer has already expired, but has not been handled
			continue;
		}
		// decrement the tick_count
		timer_list[timer_index]->tick_count--;
		if (timer_list[timer_index]->tick_count == 0) {
			cm_disable_interrupts();
			// This timer has timed out
			timer_list[timer_index]->timeout_flag++;
			// reload the timer
			timer_list[timer_index]->tick_count = timer_list[timer_index]->reload_val;
			cm_enable_interrupts();
		}
	}
	
	
	DBG_TIMER_ISR();
}

// ***************************************************************************
// * Function: void run_timer_events(void)                                   *
// * Description: loops through the timer handles and looks for a timer that *
// *     has timed out. Each call to this function will handle only the next *
// *     timer. If there is a callback function for the handle that function *
// *     is called. If the timer has timed out and it's reload value is 0,   *
// *     the timer will be removed from the list of serviced timers.         *
// * Inputs:                                                                 *
// * Returns:                                                                *
// * Caveats: This function was only intended to be used outside of ISRs.    *
// ***************************************************************************
void run_timer_event(void)
{
	// keep track of which index in timer_list to service next
	static uint8_t cur_list_index = (MAX_TIMERS - 1);
	struct timer_handle_t * cur_timer_handle;

	// Increment to the next timer handle
	cur_list_index++; if (cur_list_index == MAX_TIMERS) { cur_list_index = 0; }
	// There seems to be issues with % operation.
	// cur_list_index = (cur_list_index + 1) % MAX_TIMERS;

	cur_timer_handle = timer_list[cur_list_index];

	// is there an active timer at this index
	if (cur_timer_handle == NULL) {
		// Timer is not allocated. skip to next
		return;
	}

	cm_disable_interrupts();
	if (cur_timer_handle->timeout_flag != 0) {
		// ***  Need to handle this timer  ***
		// decrement the number of times this timer has expired
		cur_timer_handle->timeout_flag--;
		if ((cur_timer_handle->tick_count == 0) &&
			(cur_timer_handle->timeout_flag == 0)) {
			// The timer is not currently running and the timeout_flag has
			// been handled the apropriate number of times, therefore
			// automatically remove the timer handle.
			timer_list[cur_list_index] = NULL;
		}
		cm_enable_interrupts();
		if (cur_timer_handle->callback_funct != NULL) {
			// run the callback function
			cur_timer_handle->callback_funct(cur_timer_handle);
		}
	} else {
		// the timer has not expired
		cm_enable_interrupts();
	}
}

// ***************************************************************************
// * Function: void initialize_timer(void)                                   *
// * Description: Sets up the hardware timer module to be use to schedule    *
// *      code to run at particular time or for an event to occure after     *
// *      a certan amount of time.                                           *
// * Inputs:                                                                 *
// * Returns:                                                                *
// * Caveats: This function was only intended to be used in the main loop.   *
// ***************************************************************************
void initialize_timer(void)
{
	uint8_t timer_index;
	
	nt_timer_platform_init();
        
	DISABLE_NT_TIMER_INT();
	// All timers are disabled
	for (timer_index=0; timer_index<MAX_TIMERS; timer_index++) {
		timer_list[timer_index] = NULL;
	}
	ENABLE_NT_TIMER_INT();
        
        //NEW_CODE: Added to support new i/o board
        tim1_init_pps_timer();
}

// ***************************************************************************
// * Function: void add_timer(struct timer_handle_t *handle)                 *
// * Description: Places the timer handler into the list of serviced timers  *
// * Inputs: The handler for which will be serviced.                         *
// * Returns: true if the timer was added to the list, otherwise false       *
// * Caveats: This function is not designed to be use in any ISR.            *
// ***************************************************************************
bool add_timer(struct timer_handle_t *handle)
{
	uint8_t timer_index;
	for (timer_index=0; timer_index<MAX_TIMERS; timer_index++) {
		DISABLE_NT_TIMER_INT();
		if (timer_list[timer_index] == NULL) {
			// clear any previous use of number of timeouts
			handle->timeout_flag = 0;
			// enable the timer
			timer_list[timer_index] = handle;
			ENABLE_NT_TIMER_INT();
			return true;
		}
		ENABLE_NT_TIMER_INT();
	}
	return false;
}

// ***************************************************************************
// * Function: void remove_timer(struct timer_handle_t *handle)              *
// * Description: Removes the timer handle from the list of                  *
// *     timer handles that are serviced.                                    *
// * Inputs: The handler for which will no longer be serviced.               *
// * Returns:                                                                *
// * Caveats: This function is not designed to be use in any ISR.            *
// ***************************************************************************
void remove_timer(struct timer_handle_t *handle)
{
	uint8_t timer_index;
	for (timer_index=0; timer_index<MAX_TIMERS; timer_index++) {
		DISABLE_NT_TIMER_INT();
		if (timer_list[timer_index] == handle) {
			timer_list[timer_index] = NULL;
		}
		ENABLE_NT_TIMER_INT();
	}
}

// ***************************************************************************
// * Function: void timer_started(struct timer_handle_t *handle)             *
// * Description: Returns weather the timer handler is in the list to be     *
// *     serviced on the timer tick.                                         *
// * Inputs: The handler for which information is being requested            *
// * Returns: true if the handler is in the list to be serviced.             *
// *     false if the timer handle is not in the list to be serviced.        *
// * Caveats:                                                                *
// ***************************************************************************
bool timer_started(struct timer_handle_t *handle)
{
	uint8_t timer_index;
	for (timer_index=0; timer_index<MAX_TIMERS; timer_index++) {
		if (timer_list[timer_index] == handle) {
			return true;
		}
	}
	return false;
}


//NEW_CODE: Added to support new i/o board
// ***************************************************************************
// * Function: void PPS_UP_IRQ (void)                                        *
// * Description: Serviced every 10us.  Used to increment a counter needed   *                                                            *
// *    to track the elapsed time from the last rising edge of the 1 pps     *
// *    input.  Counter is also used to detect if there were any missed pps  *
// *    signals and to synchronize the TIM1 and the 1pps input.              *
// * Inputs: None                                                            *
// * Returns: None                                                           *
// * Caveats: May not be needed when a 32 bit ticker is utilized.            *                                                               *
// ***************************************************************************
void PPS_UP_IRQ (void)
{
    //NEW_CODE: Used for debug only, used to trigger oscilloscope
    gpio_set(GPIOA,GPIO8);

    if (++us125_tick_counter > PPS_TICKS+PPS_TICK_SLOP)
    {
		//if we landed in here we missed a pps edge		
		seconds_tick_counter++;
        us125_tick_counter = 0;
        
		pps_quality = BAD;  
    }
      
    //NEW_CODE: Used for debug only, used to trigger oscilloscope
    gpio_clear(GPIOA,GPIO8);

    timer_clear_flag(TIM1,TIM_SR_UIF);
}

// ***************************************************************************
// * Function: void PPS_CC_IRQ (void)                                        *
// * Description: Serviced on every rising edge of the 1 pps input.          * 
// *    us125_tick_counter represents the number of 125uS interrupts per     *
// *    rising of the pps signal as a correction factor for synchronization. *
// * Inputs: None                                                            *
// * Returns: None                                                           *
// * Caveats:                                                                *
// ***************************************************************************
void PPS_CC_IRQ (void)
{
	DISABLE_TIM1_2();

	//tweak_arrl();
	pps_quality = GOOD;

    
    us125_tick_counter = 0;
    seconds_tick_counter = 0;
    
    timer_set_counter(TIM1, 0); 
    timer_clear_flag(TIM1,TIM_SR_CC1IF|TIM_SR_CC2IF|TIM_SR_CC3IF|TIM_SR_CC4IF);
	
    ENABLE_TIM1_2();
 }


// ***************************************************************************
// * Function: void tweak_arrl (void)                                        *
// * Description: Called every rising edge of the 1 pps input.  Used to      *
// *              drift of the pps input.  When pps_avg moves above or below *
// *              PPS_TICKS, the auto-reload parameter (arrl) is ajusted.    *
// *              timer_set_period(TIM1,arrl) is the period of how often     *
// *              PPS_UP_IRQ is called.                                      *
// * Inputs: None                                                            *
// * Returns: None                                                           *
// * Caveats: Slow to update when a needed tweak is more of a bump.          *
// *************************************************************************** 
void tweak_arrl (void)
{
    #define SLOP    10
	uint32_t tick_count;

	/*
	 * tc (throttle control) is used as a state machine variable where its purpose
	 * is to control oscillations.  Meaning don't continually speed up and slow
	 * down.  The idea is to rest in the idle state for at least one seconds time.
	 *
	 * An improvment could be to move arrl (auto-reload register) in proportion to
	 * how far out pps_avg is from its ideal value of PPS_TICKS but in the event 
	 * that it's really far out there I didn't want to smack it with too large a club.
	 * All at once that is, move it gently.
	 */

	tick_count = us125_tick_counter;
	switch (tc)
	{
		case IDLE:
    		if ((tick_count >= PPS_TICKS-SLOP) &&      
        		(tick_count <= PPS_TICKS+SLOP)) 
				;
			else if (tick_count < PPS_TICKS-SLOP)  
				tc = FAST;
			else
				tc = SLOW;
		break;

		case SLOW:
			if (tick_count > PPS_TICKS+SLOP)
            	arrl += 1; // collecting too many ticks, running too slow 
            	 		   // meaning the period of pps input is too long
			tc = IDLE;
		break;

		case FAST:
			if (tick_count < PPS_TICKS+SLOP)
            	arrl -= 1; // not collecting enough ticks, running too fast 
            	 		   // meaning the period of pps input is too short
			tc = IDLE;
		break;

		default:
			tc = IDLE;
	}
         
    if (prev_arrl != arrl)
    {
        //re-adjust period of interrupt
        prev_arrl = arrl;
        timer_set_period(TIM1,arrl);
    }
}

//NEW_CODE: End








