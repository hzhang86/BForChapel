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
#include "AddrLookup.h"
#include "local_var.h"
#include "Function.h"
#include "Variable.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#define INVESTIGATE_FORK_WRAPPER
#define SEP_TAGS
#define BUFSIZE 256
using namespace Dyninst;
using namespace Dyninst::ProcControlAPI;
using namespace Dyninst::Stackwalker;
using namespace Dyninst::SymtabAPI;
using namespace std;

static Walker *walker = NULL;
static FILE *pFile;
static Dyninst::Address addr;
static Process::ptr proc; 
static char host_name[128];

/* Copied from Chapel runtime-comm, passed in as a param to fork_*_wrapper
 *
 * typedef bool chpl_bool;
 * typedef int32_t c_sublocid_t;
 * typedef int16_t chpl_fn_int_t;
 */
#ifdef INVESTIGATE_FORK_WRAPPER
typedef struct {
    uint16_t    fork_num;
    int         caller;
    int32_t     subloc;
    void*       ack;
    bool        serial_state;
    int16_t     fid;
    int         arg_size;
    char        arg[0];
} fork_t;
#endif

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
        
    //output the callstacks to the file
    fprintf(pFile,"<----START");
#ifndef SEP_TAGS
    int p_buffer;
    ret = proc->readMemory(&p_buffer, addr, 4);
    fprintf(pFile, " %s %d",host_name, p_buffer);
#endif
    fprintf(pFile, "\n");
    for (unsigned i=0; i<stackwalk.size(); i++) {
      ra = stackwalk[i].getRA();
      stackwalk[i].getName(frameName);
      fprintf(pFile, "%d 0x%016lx ", i, (unsigned long) ra);
      fprintf(pFile, "%s ", frameName.c_str());
#ifdef INVESTIGATE_FORK_WRAPPER      
      // if it's one of the fork_*_wrapper function, then we need to concatenate the call stack
      // with the pre-On stmt call stack by retrieving the "info" of this frame
      if (frameName.find("fork")!=std::string::npos && frameName.find("wrapper")!=std::string::npos) {
        Function *func = NULL;
        localVar *var = NULL;
        vector<localVar *> vars;
        int intRet;
        char outBuf[BUFSIZE];
        fork_t *info;
        int caller;
        int16_t fid;
        uint16_t fork_num;

        func = getFunctionForFrame(stackwalk[i]);
        if (func == NULL)
          cerr<<"Failed to get function for frame "<<frameName<<endl;
        else {
          ret = func->getParams(vars);
          if (ret == false)
            cerr<<"Failed to get parameters for frame "<<frameName<<endl;
          else {
            var = vars[0]; //fork_*_wrapper only has one param: fork_t *info
            if ((var->getName()).compare("f_chpl") !=0) // TO BE CONFIRMED OF the param name (formal arg)
              cerr<<"The param: "<<var->getName()<<" isn't what we want!"<<endl;
            else {
              intRet = getLocalVariableValue(var, stackwalk, i, outBuf, BUFSIZE);
              if (intRet != glvv_Success)
                cerr<<"Failed to get param "<<var->getName()<<" for frame "<<frameName<<endl;
              else {
                // Now the value of the param is stored in outBuf;
                info = (fork_t *)outBuf; //might be incompatible
                caller = info->caller;
                fid = info->fid;
                fork_num = info->fork_num;
                fprintf(pFile, "%d %d %d ",caller, fid, fork_num);
              }
            }
          }
        }
      } // End of for_*_wrapper frame
#endif
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

int main(int argc, char *argv[])
{
  vector<string> args;
  bool ret;
  char buffer[256];
  char path[] = "./SSFs/";

  gethostname(host_name, 64);
  sprintf(buffer, "%s%s",path,host_name);
#ifdef SEP_TAGS
  pFile = fopen(host_name, "a"); 
#else
  pFile = fopen(buffer, "a"); //open the file once for all
#endif
  if (pFile==NULL) {
    cerr<<"File "<<buffer<<" failed to be created"<<endl;
    return 1;
  }

  //Create a new target process
  string exec = argv[1];
  for (unsigned i=1; i<argc; i++)
    args.push_back(std::string(argv[i]));  /* e.g monitor ./lulesh --elemsPerEdge 8
                                            * then exec=lulesh, args={./lulesh, --elemsPerEdge, 8}
                                            */
  proc = Process::createProcess(exec, args);
  walker = Walker::newWalker(proc); //create a third-party walker with the target process
  //Tell ProcControlAPI about our callback function: on_signal

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

#ifndef SEP_TAGS
  Symtab *obj = NULL;
  vector<Symbol *> syms;
  bool err = Symtab::openFile(obj, exec);
  if(!err)
    cerr<<"Symtab openFile failed"<<endl;
  err = obj->findSymbol(syms, "processTLNum", Symbol::ST_OBJECT);
  if(!err)
    cerr<<"findSymbol failed"<<endl;
  Symbol *symP = syms[0];
  
  Dyninst::PID pid = proc->getPid();
  AddressLookup *addLookup = AddressLookup::createAddressLookup(pid);
  if(addLookup==NULL)
    cerr<<"createAddressLookup failed"<<endl;
  err = addLookup->getAddress(obj, symP, addr);
  if(!err)
    cerr<<"addLookup->getAddress failed"<<endl;
#endif
 
  while (!proc->isTerminated())
    Process::handleEvents(true);

  delete walker; //walker needs to be destroyed
  fclose(pFile); //close the file after all done
  return 0;
}


