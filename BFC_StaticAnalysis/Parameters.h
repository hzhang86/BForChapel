/*
 * Parameters.h
 * Define all the MACRO or const values
 *
 * Created by Hui Zhang on 03/01/15
 * Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#include <string>
#include <cstring>


#define DEBUG_P
#define DEBUG_SUMMARY
#define DEBUG_ERROR
//////////////////////////
#define ENABLE_FORTRAN 
#define HUI_C
//////////////////////////

#define DEBUG_GRAPH_BUILD
#define DEBUG_GRAPH_BUILD_EDGES
#define DEBUG_GRAPH_COLLAPSE


#define DEBUG_GRAPH_IMPLICIT
#define DEBUG_LLVM_IMPLICIT


#define DEBUG_GRAPH_TRUNC

#define DEBUG_STRUCTS

#define DEBUG_VP_CREATE

#define DEBUG_LLVM
#define DEBUG_LLVM_L2

#define DEBUG_LLVM_IMPLICIT

#define DEBUG_GLOBALS
#define DEBUG_LOCALS

#define DEBUG_CFG

#define DEBUG_RP
#define DEBUG_RP_SUMMARY
#define DEBUG_IMPORTANT_VERTICES
#define DEBUG_RECURSIVE_EX_CHILDREN
#define DEBUG_LINE_NUMS
////////////////////////////
#define DEBUG_PRINT_LINE_NUMS
/////////////////////////
#define DEBUG_COMPLETENESS

#define DEBUG_EXTERN_CALLS
#define DEBUG_OUTPUT

#define DEBUG_EXIT
#define DEBUG_EXIT_OUT

#define DEBUG_SIDE_EFFECTS

#define DEBUG_CALC_RECURSIVE
#define DEBUG_ERR_RET

#define DEBUG_A_READS
#define DEBUG_A_LINE_NUMS
#define DEBUG_A_ERROR
#define DEBUG_A_IMPORTANT_VERTICES
#define DEBUG_A_LOOPS
#define DEBUG_A_GRAPH_BUILD
#define DEBUG_A_CFG
#define DEBUG_A_GRAPH_COLLAPSE
#define DEBUG_A_GRAPH_TRUNC
#define DEBUG_A_RP
#define DEBUG_A_RP_SUMMARY
#define DEBUG_A_EXIT
#define DEBUG_A_SUMMARY
#define DEBUG_A_RECURSIVE_EX_CHILDREN
#define DEBUG_A_SIDE_EFFECTS

#define PRINT_IMPLICIT 1
#define PRINT_INST_TYPE 1
#define PRINT_LINE_NUM 1
#define NO_PRINT 0

#define ALIAS_OP          100

#define MAX_PARAMS       128

#define GEP_COLLAPSE      0
#define LOAD_COLLAPSE     1
#define BITCAST_COLLAPSE  2
#define INVOKE_COLLAPSE   3

#define NO_DEF            0

#define IMPLICIT_OP           0
#define RESOLVED_EXTERN_OP    500
#define RESOLVED_MALLOC_OP    501
#define RESOLVED_L_S_OP       502
#define RESOLVED_OUTPUT_OP    997
#define GEP_BASE_OP           1000
#define GEP_OFFSET_OP         1001
#define GEP_S_FIELD_VAR_OFF_OP 1002
#define GEP_S_FIELD_OFFSET_OP  1003

extern const char* PRJ_HOME_DIR;




#endif
