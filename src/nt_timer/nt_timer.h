#ifndef __NT_TIMER__H__
#define __NT_TIMER__H__

#include <stdint.h>


// * * * * *   S T R U C T R U R E   D E F I N I T I O N S  * * * * *

// type deffinition of the timer time in 100's of microseconds
typedef uint32_t usec_time_t;

// timer handle
// stores all the information about the timer
struct timer_handle_t {
	// Set when the handler is first created (INIT_TIMER_HANDLE())
	// Read is only intended in run_timer_events()
	void (*callback_funct)(struct timer_handle_t *);

	// pointer to data use by the timer
	void *timer_data;

	// This variable needs to have global interrupts disabled when reading and
	// writing to it and when it's handler is in the list
	usec_time_t reload_val;

	// Set when the handler is first created (INIT_TIMER_HANDLE()),
	//   and after callback_funct in run_timer_events()
	// Read is only intended in ISR, and run_timer_events()
	usec_time_t tick_count;

	// Set when the handler is first created (INIT_TIMER_HANDLE()),
	//   in the ISR, and after callback_funct in run_timer_events()
	// Can be read anywhere as long as disable global INT around access
	unsigned short timeout_flag;
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
