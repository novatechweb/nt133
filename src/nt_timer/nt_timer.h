#ifndef __NT_TIMER__H__
#define __NT_TIMER__H__

#include <stdint.h>


// * * * * *   S T R U C T R U R E   D E F I N I T I O N S  * * * * *

// type deffinition of the timer time in 100's of microseconds
typedef uint32_t usec_time_t;

// timer handle
// stores all the information about the timer
struct timer_handle_t {
	// a function to be called after the timer expires
	void (*callback_funct)(struct timer_handle_t *);

	// pointer to data use by the timer callback function.
	void *timer_data;

	// ***  For the following three varaibles  ***
	// Used in the nt_timer Interrupt Service Routine, therefore
	// must be accessed atomicly (disable global interrupts).
	// reload_val, and timeout_flag can be changed atomicly in other ISRs.
	// tick_count can only be changed by nt_timer ISR.

	// Value the timer will be reloaded to after it expires.
	usec_time_t reload_val;
	// The current count of the timer. Increments timeout_flag when 0.
	//   ***  Do not change value once timer is active  ***
	usec_time_t tick_count;
	// The number of times the timer has expired and not been handled.
	// The timer is expired when not zero.
	uint8_t timeout_flag;
};


// * * * * *   G L O B A L   V A R I A B L E S  * * * * *

// a counter that increments each NT_Timer tick (NEVER write to this variable)
extern uint32_t tick_counter;


// * * * * *   C  F U N C T I O N   P R O T O T Y P E S  * * * * *

void run_timer_event(void);
void initialize_timer(void);
bool add_timer(struct timer_handle_t *handle);
void remove_timer(struct timer_handle_t *handle);
bool timer_started(struct timer_handle_t *handle);
// returns weather the timer has timmed out
#define IS_TIMEOUT(handle) ( handle.timeout_flag != 0 )
// sets the initial values for the timer handle
#define INIT_TIMER_HANDLE(handle, start_tick_count, reload,     \
                          funct, handle_data)                   \
{	handle.callback_funct = funct;                              \
	handle.timer_data = (void*)handle_data;                     \
	handle.tick_count = start_tick_count;                       \
	handle.timeout_flag = 0;                                    \
	handle.reload_val = 0;                                      \
	if (reload) { handle.reload_val = start_tick_count; }       \
}





#endif  /* __NT_TIMER__H__ */
