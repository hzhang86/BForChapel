/*
 * Create a monitor process, attach to the original execution.
 * Use third-party stackwalk to unwind the original program.
 * Basically let the Chapel runtime to generate the PAPI signals
 * Then in this monitor, you catch the signal and do the walkStack 
 * to the signal-associated thread. In this way, since the callback 
 * and the original program are in two different address space, it'll
 * be much less restrictive for the functions in the callback.
 * 
 * Created by Hui 03/02/16
 */

#include "PCProcess.h"
#include "Event.h"
#include "walker.h"
#include "frame.h"
#include "Symtab.h"
#include "local_var.h"
#include "Function.h"
#include "Variable.h"
#include "procstate.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdint.h>

//#define GET_PTLN
//#define SAFE_ADDR
//#define INVESTIGATE_FORK_WRAPPER
#define BUFSIZE 128
using namespace Dyninst;
using namespace Dyninst::ProcControlAPI;
using namespace Dyninst::Stackwalker;
using namespace Dyninst::SymtabAPI;
using namespace std;

static Walker *walker = NULL;
static FILE *pFile;
static Process::ptr proc; 
static char host_name[128];

/* Copied from Chapel runtime-comm, passed in as a param to fork_*_wrapper
 *
 * typedef bool chpl_bool;
 * typedef int32_t c_sublocid_t;
 * typedef int16_t chpl_fn_int_t;
 * typedef struct {
     int         callee;
     uint16_t    fork_num;
     int         caller;
     int32_t     subloc;
     void*       ack;
     bool        serial_state;
     int16_t     fid;
     int         arg_size;
     char        arg[0];
 * } fork_t;
 *
 */


Process::cb_ret_t on_signal(Event::const_ptr evptr)
{
  //Callback when the target process sends a signal
  if (evptr->getEventType().code() != EventType::Signal) {
    cerr<<"Error: the event is not signal!"<<endl;
    return Process::cbProcContinue;
  }

  EventSignal::const_ptr ev_signal = evptr->getEventSignal();
  int sigNo = ev_signal->getSignal();
  //int syncT = (int)(ev_signal->getSyncType());
  //cout<<"SignalNo: "<<sigNo<<", SyncType: "<<syncT<<endl;
  
  if (sigNo == 36) { //SIG36 is the overflow signal from PAPI //17 for multi-locale
    vector<Frame> stackwalk;
    bool ret;
    unsigned long ra;
    string frameName;

    if (walker == NULL) { //should've been initialized in main
      cerr<<"walker wasn't created well "<<endl;
      return Process::cbProcContinue;
    }

    Thread::const_ptr thrptr = ev_signal->getThread(); //get the thread
    //from the debug info, it seems lwp is the real tid, getTID() is broken somehow
    Dyninst::LWP lwp = thrptr->getLWP();
    Dyninst::THR_ID tid = (Dyninst::THR_ID)lwp;
    ret = walker->walkStack(stackwalk, tid);
    if (!ret) {
      cerr<<"Third-party walkStack failed on "<<(int)tid<<endl;
      return Process::cbProcContinue;
    }
        
    // output the callstacks to the file
    fprintf(pFile,"<----START compute\n");
    // Now start outputing the stack frames
    for (unsigned i=0; i<stackwalk.size(); i++) {
      ra = stackwalk[i].getRA();
      stackwalk[i].getName(frameName);
      if (frameName.empty()) // sometimes it just can't get the frame name, 
        frameName = "***";   // but we still need to hold the place
      fprintf(pFile, "%d 0x%016lx ", i, (unsigned long) ra);
      fprintf(pFile, "%s ", frameName.c_str());
      // Check thread_begin frame
      if (frameName == "thread_begin") {
        Function *func = NULL;
        localVar *var = NULL;
        vector<localVar *> vars;
        int intRet;
        char outBuf[BUFSIZE];
        uint64_t taskID;

        func = getFunctionForFrame(stackwalk[i]);
        if (func == NULL)
          cerr<<"Failed to get function for frame "<<frameName<<endl;
        else {
          ret = func->findLocalVariable(vars, "reserved_taskID");
          if (ret == false)
            cerr<<"Failed to get ptr to reserved_taskID for thread_begin"<<endl;
          else {
            var = vars[0]; //we should only have one lv with that name
            if (vars.size() >1) 
              cerr<<"Weird: we have more than one reserved_taskID on the frame!"<<endl;
            else {
              //We can directly get the lv's value  
              intRet = getLocalVariableValue(var, stackwalk, i, outBuf, BUFSIZE);
              if (intRet != glvv_Success) 
                cerr<<"Failed("<<intRet<<") to get value of reserved_taskID"<<endl;
              else {
                // Now the value of the lv is stored in outBuf;
                taskID = *((uint64_t *)outBuf);
                //Finally, we can write tid to the stack trace file
                fprintf(pFile, "%lu ", taskID);
              }
            }
          }
        }
      } // End of thread_begin frame
      // separate the next stack frame
      fprintf(pFile, "\t");
    }
    //end of this stack trace
    fprintf(pFile, "\n");
    fprintf(pFile, "---->END\n");
  }
  else
    cerr<<"sigNo="<<sigNo<<endl;

  return Process::cbProcContinue;
}

string getFileName(string &rawName) 
{
  size_t found = rawName.find_last_of("/");
  if (found == string::npos)
    return rawName;
  else 
    return rawName.substr(found+1);
}

int main(int argc, char *argv[])
{
  vector<string> args;
  bool ret;
  char buffer[256];
  Symtab *obj = NULL;
  vector<Symbol *> syms;
  Dyninst::Address loadedBaseAddr = 0;

  gethostname(host_name, 64);
  pFile = fopen(host_name, "a"); 
  if (pFile==NULL) {
    cerr<<"File "<<buffer<<" failed to be created"<<endl;
    return 1;
  }

  // Create a new target process
  string exec = argv[1];
  for (unsigned i=1; i<argc; i++)
    args.push_back(std::string(argv[i]));  /* e.g monitor ./lulesh --elemsPerEdge 8
                                            * then exec=lulesh, args={./lulesh, --elemsPerEdge, 8}
                                            */
  proc = Process::createProcess(exec, args);
  walker = Walker::newWalker(proc); //create a third-party walker with the target process

  // Tell ProcControlAPI about our callback function: on_signal
  ret = Process::registerEventCallback(EventType::Signal, on_signal);
  if (!ret) {
    cerr<<"Process::registerEventCallback Failed !"<<endl;
    return 1;
  }
  
  //Run the target process and wait for it to terminate
  ret = proc->continueProc();
  if (!ret) {
    cerr<<"Failed to continue process !"<<endl;
    return 1;
  }

  while (!proc->isTerminated())
    Process::handleEvents(true);

  delete walker; //walker needs to be destroyed
  fclose(pFile); //close the file after all done
  return 0;
}


