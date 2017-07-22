/*
 * Copyright 2017 Hui Zhang
 * All right reserved
 *
 * only shared between task-fifo.c and comm-gasnet.c
 */

#ifndef _chpl_fifo_gasnet_h_
#define _chpl_fifo_gasnet_h_

#include "chpl-comm.h"
#include "chpl-tasks.h"
//
// Moved from task-fifo.c to be used by both task-fifo.c and chpl-blame.c
// Probably needs a better way but keep it here for now
// 

//
// task pool: linked list of tasks
//
typedef struct task_pool_struct* task_pool_p;

typedef struct {
  chpl_task_prvData_t prvdata;
} chpl_task_prvDataImpl_t;

typedef struct task_pool_struct {
  task_pool_p*     p_list_head;  // task list we're on, if any
  task_pool_p      list_next;    // double-link pointers for list
  task_pool_p      list_prev;
  task_pool_p      next;         // double-link pointers for pool
  task_pool_p      prev;

  chpl_task_prvDataImpl_t chpl_data;

  chpl_task_bundle_t bundle; // ends in a variable-length array
} task_pool_t;


typedef struct lockReport {
  int32_t            filename;
  int                lineno;
  uint64_t           prev_progress_cnt;
  chpl_bool          maybeLocked;
  struct lockReport* next;
} lockReport_t;


// This is the data that is private to each thread.
typedef struct {
  task_pool_p   ptask;
  lockReport_t* lockRprt;
  int           EventSet; // Set for PAPI SAMP
} thread_private_data_t;

#endif
