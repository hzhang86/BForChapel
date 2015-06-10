#include "papi.h"
#include <stdio.h>

#define THRESHOLD  100000

int total = 0;      /* total overflows */

void handler(int EventSet, void *address, long_long overflow_vector, void *context)
{
fprintf(stderr, "handler(%d) Overflow at %p! vector=0x%llx\n",
        EventSet, address, overflow_vector);
	total++;
}

main()
{
  	int retval, EventSet = PAPI_NULL;

  	/* Initialize the PAPI library */
  	retval = PAPI_library_init(PAPI_VER_CURRENT);
  	if (retval != PAPI_VER_CURRENT)
 //   	   handle_error(1);
    fprintf(stderr, "PAPI_library_init() is not okay.\n");

  	/* Create the EventSet */
  	if (PAPI_create_eventset(&EventSet) != PAPI_OK)
//    	   handle_error(1);
    fprintf(stderr, "PAPI_create_eventset() is not okay.\n");

  	/* Add Total Instructions Executed to our EventSet */
  	if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK)
//    	   handle_error(1);
    fprintf(stderr, "PAPI_add_event() is not okay.\n");

  	/* Call handler every 100000 instructions */
  	retval = PAPI_overflow(EventSet, PAPI_TOT_INS, THRESHOLD, 0, handler);
  	if (retval != PAPI_OK)
//    	   handle_error(1);
    fprintf(stderr, "PAPI_overflow() is not okay.\n");

  	/* Start counting */
  	if (PAPI_start(EventSet) != PAPI_OK)
//    	   handle_error(1);
    fprintf(stderr, "PAPI_start() is not okay.\n");

}
