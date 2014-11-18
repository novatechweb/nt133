#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>

#include <platform.h>
#include "nt_timer.h"


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


// ***************************************************************************
// * Description: Interupt service routine for tne NT_Timer                  *
// * Caveats: This ISR disabled global interrupts.                           *
// *     This is a low priority interrupt.                                   *
// ***************************************************************************
void NT_TIMER_IRQ(void)
{
	uint8_t timer_index;
	DBG_TIMER_TICK();
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
		if (timer_list[timer_index]->tick_count != 0) {
			// decrement the tick_count
			timer_list[timer_index]->tick_count--;
		} else {
			// timer has expired
			cm_disable_interrupts();
			if (timer_list[timer_index]->reload_val == 0) {
				// This timer has timed out
				// set flag to 1 until handle is removed from timer_list
				timer_list[timer_index]->timeout_flag = 1;
			} else {
				// reload this timer
				timer_list[timer_index]->tick_count = timer_list[timer_index]->reload_val;
				// This timer has timed out
				timer_list[timer_index]->timeout_flag++;
			}
			cm_enable_interrupts();
		}
	}
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

	usec_time_t tick_count;
	void (*callback_funct)(struct timer_handle_t *);

	// Increment to the next timer handle
	cur_list_index = (cur_list_index + 1) % MAX_TIMERS;
	DISABLE_NT_TIMER_INT();
	tick_count = timer_list[cur_list_index]->tick_count;
	callback_funct = timer_list[cur_list_index]->callback_funct;
	ENABLE_NT_TIMER_INT();
	if (timer_list[cur_list_index] == NULL) {
		// Timer is not allocated. skip to next
		return;
	}
	if (tick_count != 0) {
		// this timer has not expired
		return;
	}
	if (callback_funct != NULL) {
		callback_funct(timer_list[cur_list_index]);
	}
	cm_disable_interrupts();
	// We have handled the call for this timer handle
	timer_list[cur_list_index]->timeout_flag--;
	if (timer_list[cur_list_index]->reload_val == 0) {
		// remove the timer
		timer_list[cur_list_index] = NULL;
	}
	cm_enable_interrupts();
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
