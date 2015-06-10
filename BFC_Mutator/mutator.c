/*
 * mutator.C
 */

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_point.h" // new header file added,because of error: Batch_entry and Batch_exit is not defined in this scope

using namespace std;
using namespace Dyninst;

// main Dyninst driver structure
BPatch *bpatch;

// instrumentation options
char mainFuncName[] = "main";
char *exitFunc = mainFuncName;
vector<string> funcs;
//int fpinst = 0;

// error handling (boilerplate--no need to touch this!)
#define DYNINST_NO_ERROR -1
int errorPrint = 1; // external "dyninst" tracing (via errorFunc)
int expectError = DYNINST_NO_ERROR;
void errorFunc(BPatchErrorLevel level, int num, const char * const *params)
{
    if (num == 0) {
        // conditional reporting of warnings and informational messages
        if (errorPrint) {
            if ((level == BPatchInfo) || (level == BPatchWarning))
              { if (errorPrint > 1) printf("%s\n", params[0]); }
            else {
                printf("%s", params[0]);
            }
        }
    } else {
        // reporting of actual errors
        char line[256];
        const char *msg = bpatch->getEnglishErrorString(num);
        bpatch->formatErrorString(line, sizeof(line), msg, params);
        
        if (num != expectError) {
	  if(num != 112)
	    printf("Error #%d (level %d): %s\n", num, level, line);
        
            // We consider some errors fatal.
            if (num == 101) {
               exit(-1);
            }
        }
    }
}

void instrument(BPatch_addressSpace* app) // instrument has nothing to do with blameFunctions
{

  // lots of vectors...

  BPatch_Vector<BPatch_function*> mainFuncs;
	BPatch_Vector<BPatch_function*> pfinitFuncs;

  BPatch_Vector<BPatch_point*> *entryInitPoints;
  BPatch_Vector<BPatch_point*> *exitInitPoints;

  BPatch_Vector<BPatch_point*> *entryMainPoints;
  BPatch_Vector<BPatch_point*> *exitMainPoints;
  
  // get a reference to the application image
  BPatch_image *img = app->getImage();
  
  // grab references to functions
  img->findFunction("main", mainFuncs);
	//img->findFunction("Init", pfinitFuncs);
	//img->findFunction("__init_module__init", pfinitFuncs);
	/*
	if (pfinitFuncs.size() == 0)
		{
			printf("Couldn't Find Init.\n");
			exit(0);
		}
	*/
	// CHANGED 3/01/11
	entryMainPoints = mainFuncs[0]->findPoint(BPatch_entry);
	//entryInitPoints = pfinitFuncs[0]->findPoint(BPatch_entry);
	//entryInitPoints = pfinitFuncs[0]->findPoint(BPatch_exit);

	exitMainPoints = mainFuncs[0]->findPoint(BPatch_exit);

	//  printf("Instrumenting entry of init.\n");
  printf("Instrumenting exit of init.\n");
  
  // Create call to PAPI start snippet
  BPatch_Vector<BPatch_function *> initFuncs;
  img->findFunction("initializePAPI", initFuncs);

  BPatch_Vector<BPatch_snippet *> initArgs;

  BPatch_funcCallExpr initCall(*initFuncs[0], initArgs);
  
	// CHANGED 3/01/11
  app->insertSnippet(initCall, *entryMainPoints, BPatch_callBefore);
	//app->insertSnippet(initCall, *entryInitPoints, BPatch_callAfter);
    
  
	printf("Instrumenting exit of main.\n");

  // Create call to PAPI end snippet
  BPatch_Vector<BPatch_function *> stopFuncs;
  img->findFunction("stopPAPI", stopFuncs);

  BPatch_Vector<BPatch_snippet *> stopArgs;

  BPatch_funcCallExpr stopCall(*stopFuncs[0], stopArgs);

  app->insertSnippet(stopCall, *exitMainPoints, BPatch_callAfter);
}


void usage()
{

}

int main(int argc, char *argv[]) {

  // parse and save command line options (tedious...)
  char *binary = NULL;
  int binaryArg = 0;
  bool process = false;
  bool validArgs = false;
  char outFileName[] = "mutant";
  char *outFile = outFileName;
  

  if (argc != 2)
    {
      usage();
    }
  else
    binary = argv[1];
  
  printf("Opening file: %s\n", binary);
  
  // initalize DynInst library
  bpatch = new BPatch;
  bpatch->registerErrorCallback(errorFunc);
  
  // start/open application
  BPatch_addressSpace *app;
  
  // open binary
  app = bpatch->openBinary(binary);
   
  if (app != NULL) 
    {
    
      // add the instrumentation library
      //app->loadLibrary("libfpanalysis.so");

			bool success;

			//if (!success)
			//	printf("Failed to load balls\n");

      //success = app->loadLibrary("libstackwalk.so");

			//if (!success)
			//	printf("Failed to load stackwalk\n");

      //success = app->loadLibrary("libsymtabAPI.so");

			//if (!success)
			//	printf("Failed to load symtab\n");

//      success = app->loadLibrary("libpapi.so");

//			if (!success)
//				printf("Failed to load papi\n");

      success = app->loadLibrary("libblame.so");
			
			if (!success)
				printf("Failed to load blame library\n");

      
      // perform instrumentation
//     printf("Instrumenting application ...\n");
      printf("Rewriting application ...\n");
//      instrument(app);
      ((BPatch_binaryEdit*)app)->writeFile(outFile);
    }
  return(EXIT_SUCCESS);
}

