#include "Parameters.h"
using namespace std;

#ifdef HUI_C
const char *PRJ_HOME_DIR = "/export/home/hzhang86/BForChapel/TestPrograms/c-experiment";
const char *PARAM_REC = ".addr"; //the local receiver of the formal param of a func
#endif
#ifdef HUI_CHPL
const char *PRJ_HOME_DIR = "./";
const char *PARAM_REC = "chpl_macro_tmp";
const char *PARAM_REC2 = ".addr";
//char *PARAM_REC = ".addr";
#endif
//const char *PRJ_HOME_DIR = "/export/home/hzhang86/BForChapel/TestPrograms/c-experiment";

