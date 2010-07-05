/* Version of sbrk for no operating system.  */

#include <_syslist.h>

//#include "gui_config.h"

#ifdef USE_FREE_RTOS_CRITICAL_SECTION

	#include "../../../rtos/freertos.h"
	#include "../../../rtos/task.h"
	#include "../../../rtos/queue.h"
	#include "../../../rtos/semphr.h"

	static xSemaphoreHandle heapMutex = 0;
	struct _reent;

	void __malloc_lock( struct _reent *_r )
	{
		//portENTER_CRITICAL();

		if( !heapMutex ) heapMutex = xSemaphoreCreateRecursiveMutex();
		while( heapMutex && xSemaphoreTakeRecursive( heapMutex, configTICK_RATE_HZ ) != pdTRUE );
	}

	void __malloc_unlock( struct _reent *_r )
	{
		//portEXIT_CRITICAL();

		if( heapMutex ) xSemaphoreGiveRecursive( heapMutex );
	}

#endif

extern char heap_start;
extern char heap_end;
char *current_heap_end = 0;

void *_sbrk( int incr )
{
	char *prev_heap_end;

	if( current_heap_end == 0 ) current_heap_end = &heap_start;

	if( ( current_heap_end + incr ) <= &heap_end )
	{
		prev_heap_end = current_heap_end;
		current_heap_end += incr;

		return (void *) prev_heap_end;
	}

	return (void *) -1;
}

