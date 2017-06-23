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
// Moved from comm-gasnet.c to be used by both task and comm layer
// Probably needs a better way but keep it here for now
// 

typedef struct {
  int           callee;       // added by Hui: for easy matching local and remote locales
  uint16_t      fork_num;     // added by Hui: as a unique tag for each "comm_fork*" call
  int           caller;
  c_sublocid_t  subloc;
  void*         ack;
  chpl_bool     serial_state; // true if not allowed to spawn new threads
  chpl_fn_int_t fid;
  int           arg_size;
  char          arg[0];       // variable-sized data here
} fork_t;


/////-----------moved from tasks-fifo.c-------------------------------------/////

//
// task pool: linked list of tasks
//
typedef struct task_pool_struct* task_pool_p;

typedef struct {
  chpl_task_prvData_t prvdata;
} chpl_task_prvDataImpl_t;

typedef struct task_pool_struct {
  chpl_taskID_t    id;           // task identifier
  chpl_fn_p        fun;          // function to call for task
  void*            arg;          // argument to the function
  chpl_bool        begun;        // whether execution of this task has begun
  chpl_task_list_p ltask;        // points to the task list entry, if there is one
  c_string         filename;
  int              lineno;
  chpl_task_prvDataImpl_t chpl_data;
  task_pool_p      next;
  task_pool_p      prev;
} task_pool_t;


// This struct is intended for use in a circular linked list where the pointer
// to the list actually points to the tail of the list, i.e., the last entry
// inserted into the list, making it easier to append items to the end of the list.
// Since it is part of a circular list, the last entry will, of course,
// point to the first entry in the list.
struct chpl_task_list {
  chpl_fn_p fun;
  void* arg;
  chpl_task_prvDataImpl_t chpl_data;
  volatile task_pool_p ptask; // when null, execution of the associated task has begun
  c_string filename;
  int lineno;
  chpl_task_list_p next;
};


//
// This is a descriptor for movedTaskWrapper().
//
typedef struct {
  chpl_fn_p fp;
  void* arg;
  chpl_bool countRunning;
  chpl_task_prvDataImpl_t chpl_data;
} movedTaskWrapperDesc_t;


typedef struct lockReport {
  const char*        filename;
  int                lineno;
  uint64_t           prev_progress_cnt;
  chpl_bool          maybeLocked;
  struct lockReport* next;
} lockReport_t;


// This is the data that is private to each thread.
typedef struct {
  task_pool_p   ptask;
  lockReport_t* lockRprt;
#ifdef USE_PAPI
  int           EventSet; //NEW
#endif
} thread_private_data_t;

#endif
