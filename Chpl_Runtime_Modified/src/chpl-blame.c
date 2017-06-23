/*
 * Copyright 2004-2016 Cray Inc.
 * Other additional copyright holders may be indicated within.
 * 
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 * 
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// Visual Debug Support file
//

#include "chpl-blame.h"
#include "chplrt.h"
#include "chpl-comm.h"
#include "chpl-tasks.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/param.h>
#include "chpl-fifo-gasnet.h"
#include "chplcgfns.h"

extern c_nodeid_t chpl_nodeID; // unique ID for each node: 0, 1, 2, 
extern char host_name[128];
// each pre-On stacktrace also needs it in case the situation like// forall i in 1..10 do
//   on Locales[i] do
//     something...
// Then fork file only goes back to "taskListProcess" line, we'll
// need the corresponding preStack to constitute full stacktrace
extern __thread fork_t *param_plain;
extern __thread fork_t *param_nb;
extern __thread fork_t *param_large;
extern __thread fork_t *param_nb_large;
extern __thread task_pool_p task_pool_node;

static void cb_task_create (const chpl_task_cb_info_t *info);
static void cb_task_begin (const chpl_task_cb_info_t *info);
static void cb_task_end (const chpl_task_cb_info_t *info);
static void cb_comm_put_nb(const chpl_comm_cb_info_t *info);
static void cb_comm_get_nb(const chpl_comm_cb_info_t *info);
static void cb_comm_put(const chpl_comm_cb_info_t *info);
static void cb_comm_get(const chpl_comm_cb_info_t *info);
static void cb_comm_put_strd(const chpl_comm_cb_info_t *info);
static void cb_comm_get_strd(const chpl_comm_cb_info_t *info);
static void cb_comm_fork(const chpl_comm_cb_info_t *info);
static void cb_comm_fork_nb(const chpl_comm_cb_info_t *info);
static void cb_comm_fork_fast(const chpl_comm_cb_info_t *info);

int chpl_vdebug_fd = -1;
int chpl_vdebug = 0;

int chpl_dprintf (int fd, const char * format, ...) {
  char buffer[2048]; 
  va_list ap;
  int wrv;
  int retval;

  va_start (ap, format);
  retval = vsnprintf (buffer, sizeof (buffer), format, ap);
  va_end(ap);
  if (retval > 0) {
    wrv = write (fd, buffer,retval);
    if (wrv < 0) return -1;
    return retval;
  }
  return -1;
}

//static int chpl_make_vdebug_file (const char *rootname) {
//  return 0;
//}

// Record>  ChplVdebug: ver # nid # tid # seq time.sec user.time system.time 
//
//  Ver # -- version number, currently 1.1
//  nid # -- nodeID
//  tid # -- taskID
//  seq time.sec -- unique number for this run

void chpl_vdebug_start (const char *fileroot, double now) {
}

// Record>  End: time.sec user.time system.time nodeID taskID
//
// Should be the last record in the file.

void chpl_vdebug_stop (void) {
}

// Record>  VdbMark: time.sec nodeId taskId
//
// This marks taskID as being a xxxVdebug() call.   Any forks or tasks
// started by this task and descendants of this task are related to
// the xxxVdebug() call and chplvis should ignore them.

void chpl_vdebug_mark (void) {
}

// Record>  tname: tag# tagname

void chpl_vdebug_tagname (const char* tagname, int tagno) {
}

// Record>  Tag: time.sec user.time sys.time nodeId taskId tag# 

void chpl_vdebug_tag (int tagno) {
}

// Record>  Pause: time.sec user.time sys.time nodeId taskId tag#

void chpl_vdebug_pause (int tagno) {
}

// Routines to log data ... put here so other places can
// just call this code to get things logged.
// FIXME: the size argument (size_t) is being cast to int here. chplvis needs
//        to be updated to take in size_t. The elemsize field is no longer
//        relevant as well.

// Record>  nb_put: time.sec srcNodeId dstNodeId commTaskId addr raddr elemsize 
//                  typeIndex length lineNumber fileName
//

void cb_comm_put_nb (const chpl_comm_cb_info_t *info) {
  if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm *cm = &info->iu.comm;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd, 
                  "nb_put: %lld.%06ld %d %d %lu %#lx %#lx %d %d %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,  info->localNodeID,
                  info->remoteNodeID, (unsigned long) commTask, (unsigned long) cm->addr,
                  (unsigned long) cm->raddr, 1, cm->typeIndex, (int)cm->size,
                  cm->lineno, cm->filename);
  }
}

// Record>  nb_get: time.sec dstNodeId srcNodeId commTaskId addr raddr elemsize 
//                  typeIndex length lineNumber fileName
//
// Note: dstNodeId is node requesting Get

void cb_comm_get_nb (const chpl_comm_cb_info_t *info) {
  if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm *cm = &info->iu.comm;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd,
                  "nb_get: %lld.%06ld %d %d %lu %#lx %#lx %d %d %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,  info->localNodeID,
                  info->remoteNodeID, (unsigned long)commTask, (unsigned long) cm->addr,
                  (unsigned long) cm->raddr, 1, cm->typeIndex, (int)cm->size,
                  cm->lineno, cm->filename);
  }
}

// Record>  put: time.sec srcNodeId dstNodeId commTaskId addr raddr elemsize 
//               typeIndex length lineNumber fileName


void cb_comm_put (const chpl_comm_cb_info_t *info) {
  if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm *cm = &info->iu.comm;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd,
                  "put: %lld.%06ld %d %d %lu %#lx %#lx %d %d %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec, info->localNodeID,
                  info->remoteNodeID, (unsigned long) commTask, (unsigned long) cm->addr,
                  (unsigned long) cm->raddr, 1, cm->typeIndex, (int)cm->size,
                  cm->lineno, cm->filename);
  }
}

// Record>  get: time.sec dstNodeId srcNodeId commTaskId addr raddr elemsize 
//               typeIndex length lineNumber fileName
//
// Note:  dstNodeId is for the node making the request

void cb_comm_get (const chpl_comm_cb_info_t *info) {
  if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm *cm = &info->iu.comm;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd,
                  "get: %lld.%06ld %d %d %lu %#lx %#lx %d %d %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,  info->localNodeID,
                  info->remoteNodeID, (unsigned long) commTask, (unsigned long) cm->addr,
                  (unsigned long) cm->raddr, 1, cm->typeIndex, (int)cm->size,
                  cm->lineno, cm->filename);
  }
}

// Record>  st_put: time.sec srcNodeId dstNodeId commTaskId addr raddr elemsize 
//                  typeIndex length lineNumber fileName

void cb_comm_put_strd (const chpl_comm_cb_info_t *info) {
    if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm_strd *cm = &info->iu.comm_strd;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd,
                  "st_put: %lld.%06ld %d %ld %lu %#lx %#lx 1 %zd %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,  info->localNodeID, 
                  (long) info->remoteNodeID, (unsigned long) commTask,
                  (unsigned long) cm->srcaddr, (unsigned long) cm->dstaddr, cm->elemSize,
                  cm->typeIndex, cm->lineno, cm->filename);
    // printout srcstrides and dststrides and stridelevels and count?
  }

}

// Record>  st_get: time.sec dstNodeId srcNodeId commTaskId addr raddr elemsize 
//                  typeIndex length lineNumber fileName
//
// Note:  dstNode is node making request for get

void cb_comm_get_strd (const chpl_comm_cb_info_t *info) {
  if (chpl_vdebug) {
    struct timeval tv;
    const struct chpl_comm_info_comm_strd *cm = &info->iu.comm_strd;
    chpl_taskID_t commTask = chpl_task_getId();
    (void) gettimeofday (&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd,
                  "st_get: %lld.%06ld %d %ld %lu %#lx %#lx 1 %zd %d %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec, info->localNodeID,
                  (long) info->remoteNodeID, (unsigned long) commTask, 
                  (unsigned long) cm->dstaddr, (unsigned long) cm->srcaddr, cm->elemSize,
                  cm->typeIndex, cm->lineno, cm->filename);
    // print out the srcstrides and dststrides and stridelevels and count?
  }
}

// Record>  fork: time.sec nodeId forkNodeId subLoc funcId arg argSize forkTaskId

void cb_comm_fork (const chpl_comm_cb_info_t *info) {
  FILE *pFile;
  char fname[MAXPATHLEN]; 
  char funcName[128];
  
  unw_word_t wordValue;
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;
  int count;

  fork_t *param;
  int caller, callee;
  int16_t fid;
  uint16_t fork_num;

#ifdef ENABLE_OUTPUT_TO_FILE
  // Keep record of this chpl_comm_fork call
  const struct chpl_comm_info_comm_fork *cm = &info->iu.fork;
  //chpl_taskID_t forkTask = chpl_task_getId();
#endif 
  sprintf(fname, "%s-%s","fork",host_name);
  pFile = fopen (fname,"a");
  if (pFile==NULL) {
    printf("File %s failed to be created\n",fname);
    return;
  }
  
#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"<----START fork %d %d %d %d\n",  
          info->localNodeID, info->remoteNodeID, cm->fid, cm->fork_num);
#endif
  // Start unwind stack from this "on" statement
  count = 0;
  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    fprintf (stderr,"unw_init_local failed!\n");

  while (unw_step(&cursor)>0) {

    unw_get_reg(&cursor, UNW_REG_IP, &ip);
#ifdef ENABLE_OUTPUT_TO_FILE
	fprintf (pFile, "%d 0x%016lx ", count, (unsigned long) ip); 	
#endif
    unw_get_proc_name(&cursor, funcName, sizeof(funcName), &wordValue);
    if(!funcName[0]) // sometimes libunwind just can't get the name ! DONTKNOWWHY
      strcpy(funcName,"***");
#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "%s ", funcName);
#endif
    if (strstr(funcName, "fork")!=NULL && strstr(funcName, "wrapper")!=NULL) {
      if (strcmp(funcName, "fork_wrapper") == 0)
        param = param_plain;
      else if (strcmp(funcName, "fork_nb_wrapper") == 0)
        param = param_nb;
      else if (strcmp(funcName, "fork_large_wrapper") == 0)
        param = param_large;
      else if (strcmp(funcName, "fork_nb_large_wrapper") == 0)
        param = param_nb_large;
      else //should never get here
        printf("Error, what is this frame ? %s\n",funcName);
      
      caller = param->caller;
      callee = param->callee;
      fid = param->fid;
      fork_num = param->fork_num;

#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile,"%d %d %d %d ",caller, callee, fid, fork_num);
#endif
    }
    // If frame is thread_begin, we record the task ID
    else if (strcmp(funcName, "thread_begin") == 0 && task_pool_node) {
#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile, "%lu ", (uint64_t)(task_pool_node->id));   
#endif
    }

#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "\t");
#endif
    count++;
  }

#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"\n");
  fprintf(pFile,"---->END\n");
#endif
  fclose(pFile);
}

// Record>  fork_nb: time.sec nodeId forkNodeId subLoc funcId arg argSize forkTaskId


void  cb_comm_fork_nb (const chpl_comm_cb_info_t *info) {
  FILE *pFile;
  char fname[MAXPATHLEN]; 
  char funcName[128];
  
  unw_word_t wordValue;
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;
  int count;

  fork_t *param;
  int caller, callee;
  int16_t fid;
  uint16_t fork_num;
#ifdef ENABLE_OUTPUT_TO_FILE
  // Keep record of this chpl_comm_fork_nb call
  const struct chpl_comm_info_comm_fork *cm = &info->iu.fork;
  //chpl_taskID_t forkTask = chpl_task_getId();
#endif 
  sprintf(fname, "%s-%s","fork_nb",host_name);
  pFile = fopen (fname,"a");
  if (pFile==NULL) {
    printf("File %s failed to be created\n",fname);
    return;
  }
  
#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"<----START fork_nb %d %d %d %d\n", 
          info->localNodeID, info->remoteNodeID, cm->fid, cm->fork_num);
#endif
  // Start unwind stack from this "on" statement
  count = 0;
  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    fprintf (stderr,"unw_init_local failed!\n");

  while (unw_step(&cursor)>0) {

    unw_get_reg(&cursor, UNW_REG_IP, &ip);
#ifdef ENABLE_OUTPUT_TO_FILE
	fprintf (pFile, "%d 0x%016lx ", count, (unsigned long) ip); 	
#endif
    unw_get_proc_name(&cursor, funcName, sizeof(funcName), &wordValue);
    if(!funcName[0]) // sometimes libunwind just can't get the name ! DONTKNOWWHY
      strcpy(funcName,"***");
#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "%s ", funcName);
#endif
    if (strstr(funcName, "fork")!=NULL && strstr(funcName, "wrapper")!=NULL) {
      if (strcmp(funcName, "fork_wrapper") == 0)
        param = param_plain;
      else if (strcmp(funcName, "fork_nb_wrapper") == 0)
        param = param_nb;
      else if (strcmp(funcName, "fork_large_wrapper") == 0)
        param = param_large;
      else if (strcmp(funcName, "fork_nb_large_wrapper") == 0)
        param = param_nb_large;
      else //should never get here
        printf("Error, what is this frame ? %s\n",funcName);
      
      caller = param->caller;
      callee = param->callee;
      fid = param->fid;
      fork_num = param->fork_num;

#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile,"%d %d %d %d ",caller, callee, fid, fork_num);
#endif
    }
    // If frame is thread_begin, we record the task ID
    else if (strcmp(funcName, "thread_begin") == 0 && task_pool_node) {
#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile, "%lu ", (uint64_t)(task_pool_node->id));   
#endif
    }

#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "\t");
#endif
    count++;
  }

#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"\n");
  fprintf(pFile,"---->END\n");
#endif
  fclose(pFile);
}

// Record>  f_fork: time.sec nodeId forkNodeId subLoc funcId arg argSize forkTaskId

void cb_comm_fork_fast (const chpl_comm_cb_info_t *info) {
  FILE *pFile;
  char fname[MAXPATHLEN]; 
  char funcName[128];
  
  unw_word_t wordValue;
  unw_cursor_t cursor;
  unw_word_t ip;
  unw_context_t uc;
  int count;

  fork_t *param;
  int caller, callee;
  int16_t fid;
  uint16_t fork_num;
#ifdef ENABLE_OUTPUT_TO_FILE
  // Keep record of this chpl_comm_fork_fast call
  const struct chpl_comm_info_comm_fork *cm = &info->iu.fork;
  //chpl_taskID_t forkTask = chpl_task_getId();
#endif 
  sprintf(fname, "%s-%s","fork_fast",host_name);
  pFile = fopen (fname,"a");
  if (pFile==NULL) {
    printf("File %s failed to be created\n",fname);
    return;
  }
  
#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"<----START fork_fast %d %d %d %d\n",  
          info->localNodeID, info->remoteNodeID, cm->fid, cm->fork_num);
#endif
  // Start unwind stack from this "on" statement
  count = 0;
  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    fprintf (stderr,"unw_init_local failed!\n");

  while (unw_step(&cursor)>0) {

    unw_get_reg(&cursor, UNW_REG_IP, &ip);
#ifdef ENABLE_OUTPUT_TO_FILE
	fprintf (pFile, "%d 0x%016lx ", count, (unsigned long) ip); 	
#endif
    unw_get_proc_name(&cursor, funcName, sizeof(funcName), &wordValue);
    if(!funcName[0]) // sometimes libunwind just can't get the name ! DONTKNOWWHY
      strcpy(funcName,"***");
#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "%s ", funcName);
#endif
    if (strstr(funcName, "fork")!=NULL && strstr(funcName, "wrapper")!=NULL) {
      if (strcmp(funcName, "fork_wrapper") == 0)
        param = param_plain;
      else if (strcmp(funcName, "fork_nb_wrapper") == 0)
        param = param_nb;
      else if (strcmp(funcName, "fork_large_wrapper") == 0)
        param = param_large;
      else if (strcmp(funcName, "fork_nb_large_wrapper") == 0)
        param = param_nb_large;
      else //should never get here
        printf("Error, what is this frame ? %s\n",funcName);
      
      caller = param->caller;
      callee = param->callee;
      fid = param->fid;
      fork_num = param->fork_num;

#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile,"%d %d %d %d ",caller, callee, fid, fork_num);
#endif
    }
    // If frame is thread_begin, we record the task ID
    else if (strcmp(funcName, "thread_begin") == 0 && task_pool_node) {
#ifdef ENABLE_OUTPUT_TO_FILE
      fprintf(pFile, "%lu ", (uint64_t)(task_pool_node->id));   
#endif
    }

#ifdef ENABLE_OUTPUT_TO_FILE
    fprintf(pFile, "\t");
#endif
    count++;
  }

#ifdef ENABLE_OUTPUT_TO_FILE
  fprintf(pFile,"\n");
  fprintf(pFile,"---->END\n");
#endif
  fclose(pFile);
}


// Task layer callbacks

int install_callbacks (void) {
  if (chpl_task_install_callback(chpl_task_cb_event_kind_create, 
                                 chpl_task_cb_info_kind_full, cb_task_create) != 0)
    return 1;
  if (chpl_task_install_callback(chpl_task_cb_event_kind_begin, 
                                 chpl_task_cb_info_kind_full, cb_task_begin) != 0) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_task_install_callback(chpl_task_cb_event_kind_end,
                                 chpl_task_cb_info_kind_id_only, cb_task_end) != 0) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_put_nb,
                                 cb_comm_put_nb)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_get_nb,
                                 cb_comm_get_nb)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_put,
                                 cb_comm_put)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_get,
                                 cb_comm_get)) {
    (void) uninstall_callbacks();
    return 1;
  }
   if (chpl_comm_install_callback(chpl_comm_cb_event_kind_put_strd,
                                 cb_comm_put_strd)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_get_strd,
                                 cb_comm_get_strd)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_fork,
                                 cb_comm_fork)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_fork_nb,
                                 cb_comm_fork_nb)) {
    (void) uninstall_callbacks();
    return 1;
  }
  if (chpl_comm_install_callback(chpl_comm_cb_event_kind_fork_fast,
                                 cb_comm_fork_fast)) {
    (void) uninstall_callbacks();
    return 1;
  }

  return 0;
}


int uninstall_callbacks (void) {
  int rv = 0;
  rv  = chpl_task_uninstall_callback(chpl_task_cb_event_kind_create,
                                     cb_task_create);
  rv += chpl_task_uninstall_callback(chpl_task_cb_event_kind_begin,
                                     cb_task_begin);
  rv += chpl_task_uninstall_callback(chpl_task_cb_event_kind_end,
                                     cb_task_end);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_put_nb,
                                     cb_comm_put_nb);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_get_nb,
                                     cb_comm_get_nb);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_put,
                                     cb_comm_put);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_get,
                                     cb_comm_get);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_put_strd,
                                     cb_comm_put_strd);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_get_strd,
                                     cb_comm_get_strd);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_fork,
                                     cb_comm_fork);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_fork_nb,
                                     cb_comm_fork_nb);
  rv += chpl_comm_uninstall_callback(chpl_comm_cb_event_kind_fork_fast,
                                     cb_comm_fork_fast);
  return rv;
}

// Record>  task: time.sec nodeId taskId parentTaskId On/Local lineNum srcName fid

void cb_task_create (const chpl_task_cb_info_t *info) {
  struct timeval tv;
  if (!chpl_vdebug) return;
  if (chpl_vdebug_fd >= 0) {
    chpl_taskID_t taskId = chpl_task_getId();
    //printf ("taskCB: event: %d, node %d proc %s task id: %llu, new task id: %llu\n",
    //         (int)info->event_kind, (int)info->nodeID,
    //        (info->iu.full.is_fork ? "O" : "L"), taskId, info->iu.full.id);
    (void)gettimeofday(&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd, "task: %lld.%06ld %lld %ld %lu %s %ld %d %d\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,
                  (long long) info->nodeID, (long int) info->iu.full.id,
                  (unsigned long) taskId,
                  (info->iu.full.is_fork ? "O" : "L"),
                  (long int) info->iu.full.lineno,
                  info->iu.full.filename,
                  info->iu.full.fid);
   }
}

// Record>  Btask: time.sec nodeId taskId

void cb_task_begin (const chpl_task_cb_info_t *info) {
  struct timeval tv;
  if (!chpl_vdebug) return;
  if (chpl_vdebug_fd >= 0) {
    (void)gettimeofday(&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd, "Btask: %lld.%06ld %lld %lu\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,
                  (long long) info->nodeID, (unsigned long) info->iu.full.id);
 
  }
}

// Record>  Etask: time.sec nodeId taskId

void cb_task_end (const chpl_task_cb_info_t *info) {
  struct timeval tv;
  if (!chpl_vdebug) return;
  if (chpl_vdebug_fd >= 0) {
    (void)gettimeofday(&tv, NULL);
    chpl_dprintf (chpl_vdebug_fd, "Etask: %lld.%06ld %lld %lu\n",
                  (long long) tv.tv_sec, (long) tv.tv_usec,
                  (long long) info->nodeID, (unsigned long) info->iu.id_only.id);
 
  }
}
