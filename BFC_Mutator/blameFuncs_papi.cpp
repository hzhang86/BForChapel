#include "walker.h"
#include "procstate.h"
#include "frame.h"
#include "swk_errors.h"

#include <libunwind.h>

#include "papi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>

#include <string>

using namespace Dyninst;
using namespace Dyninst::Stackwalker;

#define THRESHOLD 1073807359   // overflow threshold of PAPI_TOT_CYC event
//#define THRESHHOLD 0x7fffffff

static int EventSet = PAPI_NULL;
static unsigned topAddr;
static char buffer[256];
static FILE * pFile;
static unsigned long runcount;

//////////////////////////////////////////
time_t begin, end;
//////////////////////////////////////////
/*
void handlerSW(int EventSet, void *address, long_long overflow_vector, void *context) // It's not used anymore !!! See handler
{

  int i;
  ucontext_t * uct = (ucontext_t *) context;

  std::vector<Frame> stackwalk;
  std::string s;
  Walker * walker = Walker::newWalker();
  
  Frame * f = Frame::newFrame(uct->uc_mcontext.gregs[14], uct->uc_mcontext.gregs[7], uct->uc_mcontext.gregs[6], walker);
  walker->walkStackFromFrame(stackwalk,*f);
  
	int * loopIndex = (int *) topAddr;

  fprintf(pFile,"<----START %d %s %d \n",stackwalk.size(), buffer, *loopIndex);
  
  for (i = 0; i < stackwalk.size(); i++)
    { 
      fprintf(pFile,"%d 0x%lx \t", i, stackwalk[i].getRA()); // changed %x to %lx 
    }
  
  fprintf(pFile,"\n");
  
  fprintf(pFile,"---->END\n");
  
  fclose(pFile);  
}
*/
void handler(int EventSet, void *address, long_long overflow_vector, void *context)
{
	
  unw_cursor_t cursor;
  unw_word_t ip, sp;	// in libunwind-x86_64:  unint64_t = unw_word_t, which is 64 bits
  unw_context_t uc;
  int ret;
	
  unw_word_t reg_EAX, reg_EDX, reg_ECX, reg_EBX, reg_ESI, reg_EDI;

  int * loopIndex = (int *) topAddr;
	//printf("Here\n");
  fprintf(pFile,"<----START %s \n", buffer );
  
  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    fprintf (stderr,"unw_init_local failed!\n");
	
	int count = 0;

  do {
    unw_get_reg (&cursor, UNW_REG_IP, &ip); // UNW_REG_IP: is the instruction pointer(program counter), identified by MACRO in libunwind
    unw_get_reg (&cursor, UNW_REG_SP, &sp); // UNW_REG_SP: is the stack pointer(SP) identified by MACRO in libunwind
	fprintf (pFile, "%d 0x%016lx \t", count, (unsigned long) ip); //changed output format from %08lx, (long) ip 
	count++;

    ret = unw_step (&cursor);
    if (ret < 0) {
	  unw_get_reg (&cursor, UNW_REG_IP, &ip);
	  fprintf (stderr, "FAILURE: unw_step() returned %d for ip=%lx\n", ret, (long) ip);
	}
  }
  while (ret > 0);
    
  fprintf(pFile,"\n");
  
  fprintf(pFile,"---->END\n");
  ++runcount;
}



extern "C" void initIndex(unsigned address)
{
	topAddr = address;
}

extern "C" void __attribute__ ((constructor)) initializePAPI()
{
  gethostname(buffer, 255);
  pFile = fopen (buffer,"a");
  if (pFile==NULL) {
    printf("File failed to be created\n");
    return;
  }
  
  printf("Blame started.\n");

/////////////////////////////////////////////////////////////
  begin = time(NULL);
////////////////////////////////////////////////////////////

  fprintf(stderr, "In initializePAPI\n");
  int retval;

  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT && retval > 0) {
    fprintf(stderr, "PAPI library version mismatch!\n");
  }
  if (retval < 0)
    fprintf(stderr, "PAPI_library_init() is not okay.\n");

  retval = PAPI_is_initialized();
  if (retval != PAPI_LOW_LEVEL_INITED)
    fprintf(stderr, "Verification via PAPI_is_initialize() failed.\n");
  else
    fprintf(stderr, "PAPI_library_init() success.\n");

  if (PAPI_create_eventset(&EventSet) != PAPI_OK)
    fprintf(stderr, "PAPI_create_eventset() is not okay.\n");
  else
    fprintf(stderr, "PAPI_create_eventset() success.\n");

  if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK)
    fprintf(stderr, "PAPI_add_event() is not okay.\n");
  else
    fprintf(stderr, "PAPI_add_event() success.\n");
  
  unsigned long thresh = THRESHOLD;
  if (getenv("CYC_THRESH"))
    thresh = atol(getenv("CYC_THRESH"));

  //retval = PAPI_overflow(EventSet, PAPI_TOT_CYC, thresh, 0, handlerSW);

  //PAPI_add_event(EventSet, PAPI_TOT_INS);
  //retval = PAPI_overflow(EventSet, PAPI_TOT_INS, thresh, 0, handler);
  if (PAPI_overflow(EventSet, PAPI_TOT_CYC, thresh, 0, handler) != PAPI_OK)
    fprintf(stderr, "PAPI_overflow() is not okay.\n");
  else
    fprintf(stderr, "PAPI_overflow() success.\n");

  if (PAPI_start(EventSet) != PAPI_OK)
    fprintf(stderr, "PAPI_start() is not okay.\n");
  else
    fprintf(stderr, "PAPI_start() success.\n");
}



extern "C" void __attribute__ ((destructor)) stopPAPI()
{
  printf("Blame handler ran %d times.\n", runcount);
  fclose(pFile);  
  printf("In stopPAPI\n");
  PAPI_stop(EventSet, NULL);
////////////////////////////////////////////////////////////
  end = time(NULL);
  printf("Time running mutant = %d\n",(int)(end-begin));
///////////////////////////////////////////////////////////////
}
