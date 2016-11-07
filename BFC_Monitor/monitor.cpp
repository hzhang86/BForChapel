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

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#define SEP_TAGS
using namespace Dyninst;
using namespace Dyninst::ProcControlAPI;
using namespace Dyninst::Stackwalker;
using namespace std;
#ifndef SEP_TAGS
using namespace SymtabAPI;
#endif

static Walker *walker = NULL;
static FILE *pFile;
static Dyninst::Address addr;
static Process::ptr proc; 
static char host_name[128];

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
  
  if (sigNo == 36) { //SIG36 is the overflow signal from PAPI
    vector<Frame> stackwalk;
    bool ret;
    unsigned long ra;

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
    ret = proc->readMemory(&p_buffer, addr, sizeof(int));
    fprintf(pFile, " %s %d",host_name, p_buffer);
#endif
    fprintf(pFile, "\n");
    for (unsigned i=0; i<stackwalk.size(); i++) {
      ra = stackwalk[i].getRA();
      fprintf(pFile, "%d 0x%016lx \t", i, (unsigned long) ra);
    }
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
  addr = symP->getOffset();
#endif

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


