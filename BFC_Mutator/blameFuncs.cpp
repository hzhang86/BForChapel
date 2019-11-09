/*
 *  blameFuncs.cpp
 *  
 *  source code for libblame
 *  Created by Hui Zhang on 05/25/14.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include "walker.h"
#include "procstate.h"
#include "frame.h"
#include "swk_errors.h"

#include <libunwind.h>

//#include "papi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>
#include <string>

///// Perf Util includes /////
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <err.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "perf_util.h"

//////////////////////////////

using namespace Dyninst;
using namespace Dyninst::Stackwalker;

//#define THRESHOLD 1024000    // overflow threshold of PAPI_TOT_CYC event
#define THRESHOLD 1073807359    // overflow threshold of PAPI_TOT_CYC event
static volatile unsigned long notification_received;

static perf_event_desc_t *fds = NULL;
static int num_fds = 0;
static uint64_t *val;
static int buffer_pages = 1; /* size of buffer payload (must be power of 2)*/
static char buffer[256];
static FILE * pFile;

///////////////////////////////////////////////
extern int num_skid;
///////////////////////////////////////////////

static void sigio_handler(int n, siginfo_t *info, void *uc)
{
	
	struct perf_event_header ehdr;
	int ret, id;

  	fprintf(pFile,"<----START %s \n", buffer);

	//
	// positive si_code indicate kernel generated signal
	// which is normal for SIGIO
	//
	if (info->si_code < 0)
		errx(1, "signal not generated by kernel");

	//
	// SIGPOLL = SIGIO
	// expect POLL_HUP instead of POLL_IN because we are
	// in one-shot mode (IOC_REFRESH)
	//
	if (info->si_code != POLL_HUP)
		errx(1, "signal not generated by SIGIO");

	id = perf_fd2event(fds, num_fds, info->si_fd);
	if (id == -1)
		errx(1, "no event associated with fd=%d", info->si_fd);

	ret = perf_read_buffer(fds+id, &ehdr, sizeof(ehdr));
	if (ret)
		errx(1, "cannot read event header");

	if (ehdr.type != PERF_RECORD_SAMPLE) {
		warnx("unexpected sample type=%d, skipping\n", ehdr.type);
		perf_skip_buffer(fds+id, ehdr.size);
		goto skip;
	}
	//
	// output the stacktrace
	//
	ret = perf_display_sample(fds, num_fds, 0, &ehdr, pFile);
	//
	// increment our notification counter
	//
	notification_received++;
  	fprintf(pFile,"---->END\n");
  
skip:
	//
	// rearm the counter for one more shot
	//
	ret = ioctl(info->si_fd, PERF_EVENT_IOC_REFRESH, 1);
	if (ret == -1)
		err(1, "cannot refresh");
 

  	//fclose(pFile);  

}

/*
static void sigio_handler2(int n, siginfo_t *info, void *uc)
{
	
	struct perf_event_header ehdr;
	int ret, id;
	
	char buffer[256];

	gethostname(buffer, 255);


  	int pFile; //file descriptor of the output file
  	pFile = open (buffer, O_WRONLY | O_CREAT, 0644);
  	if (pFile == -1) {
      	perror("open 3");
    	//return 3; // use `man 3 perror` to print relatively error message
    }

    char *buffer2;
    int write_ret;
    buffer2 = "<----START pygmy\n"; 
    write_ret = write(pFile, &buffer2, 17);
    if(write_ret <= 16) {
        perror("write_ret");
        //return write_ret;
    }
  	//fprintf(pFile,"<----START %s \n", buffer );

	//
	// positive si_code indicate kernel generated signal
	// which is normal for SIGIO
	//
	if (info->si_code < 0)
		errx(1, "signal not generated by kernel");

	//
	// SIGPOLL = SIGIO
	// expect POLL_HUP instead of POLL_IN because we are
	// in one-shot mode (IOC_REFRESH)
	//
	if (info->si_code != POLL_HUP)
		errx(1, "signal not generated by SIGIO");

	id = perf_fd2event(fds, num_fds, info->si_fd);
	if (id == -1)
		errx(1, "no event associated with fd=%d", info->si_fd);

	ret = perf_read_buffer(fds+id, &ehdr, sizeof(ehdr));
	if (ret)
		errx(1, "cannot read event header");

	if (ehdr.type != PERF_RECORD_SAMPLE) {
		warnx("unexpected sample type=%d, skipping\n", ehdr.type);
		perf_skip_buffer(fds+id, ehdr.size);
		goto skip;
	}
    
    buffer2 = "Notification\n";
	//printf("Notification:%lu\n ", notification_received);
	write(1, &buffer2, sizeof(buffer2));
    //
	// output the stacktrace
	//
	//ret = perf_display_sample(fds, num_fds, 0, &ehdr, pFile);
	//
	// increment our notification counter
	//
	notification_received++;
skip:
	//
	// rearm the counter for one more shot
	//
	ret = ioctl(info->si_fd, PERF_EVENT_IOC_REFRESH, 1);
	if (ret == -1)
		err(1, "cannot refresh");
 
  	//fprintf(pFile,"---->END\n");
    buffer2 = "---->END\n";
    write_ret = write(pFile, &buffer2, 9);
    if(write_ret <= 8) {
        perror("write_ret 2");
        //return write_ret;
    }
  	
    close(pFile);  

}
*/

/*
extern "C" void initIndex(unsigned address)
{
	topAddr = address;
}
*/
extern "C" void __attribute__ ((constructor)) initializePerfEvent()
{
  	fprintf(stderr, "In initializePerfEvent\n");
    gethostname(buffer, 255);
    pFile = fopen (buffer,"a");
    if (pFile==NULL) {
        printf("File failed to be created\n");
        return;
    }
 
    unsigned long thresh = THRESHOLD;
    if (getenv("CYC_THRESH"))
        thresh = atol(getenv("CYC_THRESH"));
 
	struct sigaction act;
	sigset_t news, old;

	size_t sz, pgsz;
	int ret, i, retCode;

	setlocale(LC_ALL, "");

	ret = pfm_initialize();
	if (ret != PFM_SUCCESS)
		errx(1, "Cannot initialize library: %s", pfm_strerror(ret));

	pgsz = sysconf(_SC_PAGESIZE);

	/*
	 * Install the signal handler (SIGIO)
	 * need SA_SIGINFO because we need the fd
	 * in the signal handler
	 */
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = sigio_handler;
	act.sa_flags = SA_SIGINFO;
	retCode = sigaction (SIGIO, &act, 0);
    if (retCode != 0)
        err(1, "sigaction setup failed");

	sigemptyset(&old);
	sigemptyset(&news);
	sigaddset(&news, SIGIO);

	ret = sigprocmask(SIG_SETMASK, NULL, &old);
	if (ret)
		err(1, "sigprocmask failed");

	if (sigismember(&old, SIGIO)) {
		warnx("program started with SIGIO masked, unmasking it now\n");
		ret = sigprocmask(SIG_UNBLOCK, &news, NULL);
		if (ret)
			err(1, "sigprocmask failed");
	}

	/*
 	 * allocates fd for us
 	 */
	ret = perf_setup_list_events("cycles",
					//"instructions",
                    //"cache-references",
					&fds, &num_fds);
	if (ret || (num_fds == 0))
		exit(1);

	fds[0].fd = -1;
	for(i=0; i < num_fds; i++) {

		/* want a notification for every each added to the buffer */
		fds[i].hw.disabled = !i;
		if (!i) {
			fds[i].hw.wakeup_events = 1;
			fds[i].hw.sample_type = PERF_SAMPLE_CALLCHAIN;// | bitwise or operator: 01 | 10 = 11
			fds[i].hw.sample_period = thresh;
////////////////////////////////////////////////////////////////////////
            fds[i].hw.precise_ip = 2; // HERE IS HOW YOU ENABLE THE PEBS !!! the header.misc would also be automatically set accordingly 
////////////////////////////////////////////////////////////////////////
			/* read() returns event identification for signal handler */
			fds[i].hw.read_format = PERF_FORMAT_GROUP|PERF_FORMAT_ID|PERF_FORMAT_SCALE;
		}

		fds[i].fd = perf_event_open(&fds[i].hw, 0, -1, fds[0].fd, 0);
		if (fds[i].fd == -1)
			err(1, "cannot attach event %s", fds[i].name);
	}
	
	sz = (3+2*num_fds)*sizeof(uint64_t);
	val = static_cast<uint64_t*>(malloc(sz));
	if (!val)
		err(1, "cannot allocated memory");
	/*
	 * On overflow, the non lead events are stored in the sample.
	 * However we need some key to figure the order in which they
	 * were laid out in the buffer. The file descriptor does not
	 * work for this. Instead, we extract a unique ID for each event.
	 * That id will be part of the sample for each event value.
	 * Therefore we will be able to match value to events
	 *
	 * PERF_FORMAT_ID: returns unique 64-bit identifier in addition
	 * to event value.
	 */
	if (fds[0].fd == -1)
		errx(1, "cannot create event 0");

	ret = read(fds[0].fd, val, sz);
	if (ret == -1)
		err(1, "cannot read id %zu", sizeof(val));

	/*
	 * we are using PERF_FORMAT_GROUP, therefore the structure
	 * of val is as follows:
	 *
	 *      { u64           nr;
	 *        { u64         time_enabled; } && PERF_FORMAT_ENABLED
	 *        { u64         time_running; } && PERF_FORMAT_RUNNING
	 *        { u64         value;                  
	 *          { u64       id;           } && PERF_FORMAT_ID
	 *        }             cntr[nr];               
	 * We are skipping the first 3 values (nr, time_enabled, time_running)
	 * and then for each event we get a pair of values.
	 */
 
//	for(i=0; i < num_fds; i++) {
//		fds[i].id = val[2*i+1+3];
//		printf("%"PRIu64"  %s\n", fds[i].id, fds[i].name);
//	} // this is the beginning of output
	 
	fds[0].buf = mmap(NULL, (buffer_pages+1)*pgsz, PROT_READ|PROT_WRITE, MAP_SHARED, fds[0].fd, 0);
	if (fds[0].buf == MAP_FAILED)
		err(1, "cannot mmap buffer");
	
	fds[0].pgmsk = (buffer_pages * pgsz) - 1;

	/*
	 * setup asynchronous notification on the file descriptor
	 */
	ret = fcntl(fds[0].fd, F_SETFL, fcntl(fds[0].fd, F_GETFL, 0) | O_ASYNC);
	if (ret == -1)
		err(1, "cannot set ASYNC");

	/*
 	 * necessary if we want to get the file descriptor for
 	 * which the SIGIO is sent in siginfo->si_fd.
 	 * SA_SIGINFO in itself is not enough
 	 */
	ret = fcntl(fds[0].fd, F_SETSIG, SIGIO);
	if (ret == -1)
		err(1, "cannot setsig");

	/*
	 * get ownership of the descriptor
	 */
	ret = fcntl(fds[0].fd, F_SETOWN, getpid());
	if (ret == -1)
		err(1, "cannot setown");

	/*
	 * enable the group for one period
	 */
	ret = ioctl(fds[0].fd, PERF_EVENT_IOC_REFRESH , 1);
	if (ret == -1)
		err(1, "cannot refresh");

}



extern "C" void __attribute__ ((destructor)) stopPerfEvent()
{
	printf("In stopPerfEvent\n");
    //fprintf(stderr, "0x%lx\n", sigio_handler);
    printf("Handler totally ran %ld times\n",notification_received);
	fclose(pFile);

    int ret;
	ret = ioctl(fds[0].fd, PERF_EVENT_IOC_DISABLE, 1);
	if (ret == -1)
		err(1, "cannot disable");

	/*
	 * destroy our session
	 */
	int i;
	for(i=0; i < num_fds; i++)
		close(fds[i].fd);

	perf_free_fds(fds, num_fds);
	free(val);

	/* free libpfm resources cleanly */
	pfm_terminate();
		
/////////////////////////////////////////////////////////////
	//printf("num_skid = %d\n",num_skid);
/////////////////////////////////////////////////////////////	
}