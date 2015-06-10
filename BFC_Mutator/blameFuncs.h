#ifndef BLAME_FUNCS_H
#define BLAME_FUNCS_H
//////////////////////////////////
/*
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string>
*/
#include "papi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>

#include <string>

/////////////////////////////////

//typedef long long_long;  // newly added, just to avoid the debug 
//////added new function definition///////////
void handler(int EventSet, void *address, long_long overflow_vector, void *context);
/////////////////////////////////////////////
void initIndex(unsigned address);
void initializePAPI();
void stopPAPI();

#endif
