/*
 *  FunctionBFCSecondPass.cpp
 *  
 *  Function implementations to deal with extern functions
 *  Created by Hui Zhang on 04/21/15
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#include "ExitVars.h"
#include "FunctionBFC.h"
#include <iostream>
#include <map>

#include <ctype.h>
#include <string.h>

using namespace std;

void FunctionBFC::transferExternCallEdges(std::vector< std::pair<int, int> > &blamedNodes, int callNode, std::vector< std::pair<int, int> > & blameeNodes)
{
	bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
    property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
    std::vector< std::pair<int, int> >::iterator v_pair_i;
    for (v_pair_i = blameeNodes.begin(); v_pair_i != blameeNodes.end(); v_pair_i++){
		// First int (param number), Second int (node number)
		std::pair<int, int> blamee = *v_pair_i;
		tie(ed, inserted) = add_edge(callNode, blamee.second, G);
		
		blame_info<<"Adding ERCALL(1) edge for blamee param "<<blamee.first<<"("<<blamee.second<<") of "<<callNode<<" call"<<std::endl;
		
		if (inserted) {
			// We need to have as much info in the graph as possible,
			// so we want to record the param number of the call
			// since this is only partially resolved we need to keep this
			// info around as long as possible
			edge_type[ed] = RESOLVED_EXTERN_OP;	
			remove_edge(blamee.second, callNode, G);
			
			// It's resolved, no need to acknowledge it as a call node anymore
			NodeProps *callV = get(get(vertex_props, G), callNode);
			callV->nStatus[CALL_NODE] = false;
			
			NodeProps * blameeV = get(get(vertex_props, G), blamee.second);
			blameeV->line_num = 0;
			
			int remainingCalls = 0;
			// We can reassign call statuses since these calls have now been resolved
			boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
			oe_beg = boost::out_edges(blamee.second, G).first;//edge iter begin
			oe_end = boost::out_edges(blamee.second, G).second;//edge iter end
			for(; oe_beg != oe_end; ++oe_beg) {
				int opCode = get(get(edge_iore, G),*oe_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
			ie_beg = boost::in_edges(blamee.second, G).first;//edge iterator begin
			ie_end = boost::in_edges(blamee.second, G).second;//edge iterator end
			for(; ie_beg != ie_end; ++ie_beg) {
				int opCode = get(get(edge_iore, G),*ie_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			if (!remainingCalls) {
				blameeV->nStatus[CALL_PARAM] = false;
				blameeV->nStatus[CALL_RETURN] = false;
				blameeV->nStatus[CALL_NODE] = false;

#ifdef ONLY_FOR_PARAM1 // we still want all call-related nodes to be important
                blameeV->nStatus[IMP_REG] = true;
#endif
			}
		}
	}
	
    for (v_pair_i = blamedNodes.begin(); v_pair_i != blamedNodes.end(); v_pair_i++) {
		// First int (param number), Second int (node number)
		std::pair<int, int> blamed = *v_pair_i;
		remove_edge(blamed.second, callNode, G);
		tie(ed, inserted) = add_edge(blamed.second, callNode, G);
        #ifdef DEBUG_EXTERN_CALLS
		blame_info<<"Adding ERCALL(2) edge for blamed param "<<blamed.first<<"("<<blamed.second<<") of "<<callNode<<" call"<<std::endl;
		#endif
		
		if (inserted) {
			edge_type[ed] = RESOLVED_EXTERN_OP;	
			NodeProps *callV = get(get(vertex_props, G), callNode);
			callV->nStatus[CALL_NODE] = false;

			NodeProps *blamedV = get(get(vertex_props, G), blamed.second);
			//TODO: 03/10/17: Is it to set the blamed Arg to be written ? or establish a new relationship between the blamed and blamee
            //blamedV->isWritten = true;
            /*
              For an external call like: a = exfunc(b,c,d), NONE of a~d is written for sure if they are pointers (pointers are written if
              the memory address they represents are written into. If b is blamed, then b has outgoing edges to a,c,d through the callnode
              "exfunc", and we establish a blamees set for each blamed arg, when we do checkIfWritten for blamed arg "b", we count these
              blamees to its sets. Blamed args are NOT necessarily written but they are if anyone of the blamees is written
            */
            std::vector<std::pair<int, int>>::iterator v_pair_i2;
            for (v_pair_i2=blameeNodes.begin(); v_pair_i2!=blameeNodes.end(); v_pair_i2++) {
              std::pair<int, int> blamee2 = *v_pair_i2;
              NodeProps *blameeV2 = get(get(vertex_props, G), blamee2.second);
              blamedV->blameesFromExFunc.insert(blameeV2);
            }

			int remainingCalls = 0;
			//blamedV->externCallLineNumbers.insert(callNode->line_num);
			// We can reassign call statuses since these calls have now been resolved
			boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
			oe_beg = boost::out_edges(blamed.second, G).first;//edge iter begin
			oe_end = boost::out_edges(blamed.second, G).second;//edge iter end
			for(; oe_beg != oe_end; ++oe_beg) {
				int opCode = get(get(edge_iore, G),*oe_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
			ie_beg = boost::in_edges(blamed.second, G).first;//edge iter begin
			ie_end = boost::in_edges(blamed.second, G).second;//edge iter end
			for(; ie_beg != ie_end; ++ie_beg) {
				int opCode = get(get(edge_iore, G),*ie_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			if (!remainingCalls) {
				blamedV->nStatus[CALL_PARAM] = false;
				blamedV->nStatus[CALL_RETURN] = false;
				blamedV->nStatus[CALL_NODE] = false;
	
#ifdef ONLY_FOR_PARAM1  //we still want all call-related nodes to be important
                blamedV->nStatus[IMP_REG] = true;
#endif
            }
		}
	}
}

char* FunctionBFC::trimTruncStr(const char *truncStr)
{
  int last = (int)strlen(truncStr);
  if (!last) //last==0
    return NULL;

  while (isdigit(truncStr[last-1])) {
    last--;
  }

  const char *startPtr = truncStr;
  char *tempStr = new char[last + 1];
  strncpy(tempStr, startPtr, last);
  tempStr[last] = '\0';
  char *trimedStr = tempStr;
  return trimedStr;
}

const char* FunctionBFC::getTruncStr(const char *fullStr)
{
  const char *endPtr = strstr( fullStr, "--" );
  const char *startPtr = fullStr;
	
  if (endPtr == NULL || startPtr == NULL)
    return NULL;
	
  char *truncStr = new char[endPtr - startPtr + 1];
  int size = endPtr - startPtr;
  strncpy(truncStr, startPtr,size);
  truncStr[size] = '\0';
  const char *tStr = truncStr;
  return tStr;
}


void FunctionBFC::handleOneExternCall(ExternFunctionBFC *efb, NodeProps *v)
{
#ifdef DEBUG_EXTERN_CALLS
	blame_info<<"Calls__(handleOneExternCall) -looking at "<<v->name<<std::endl;
#endif
	int v_index = v->number;
	
    boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
    ie_beg = boost::in_edges(v_index, G).first;	// edge iterator begin
    ie_end = boost::in_edges(v_index, G).second;// edge iterator end
	
    // First int (param number), Second int (node number)
    std::vector< std::pair<int, int> > blameeNodes;
    std::vector< std::pair<int, int> > blamedNodes;

    for(; ie_beg != ie_end; ++ie_beg) {
		NodeProps *inTargetV = get(get(vertex_props,G), source(*ie_beg,G));
		std::set<FuncCall *>::iterator fc_i = inTargetV->funcCalls.begin();
		
		for (; fc_i != inTargetV->funcCalls.end(); fc_i++) {
			FuncCall *fc = *fc_i;
			std::pair<int, int> tmpPair(fc->paramNumber, inTargetV->number);
			
			if (fc->funcName != v->name)
				continue;
			
			if (efb->paramNums.count(fc->paramNumber)) {
	            blamedNodes.push_back(tmpPair);
				inTargetV->isBlamedExternCallParam = true;
				#ifdef DEBUG_EXTERN_CALLS
				blame_info<<inTargetV->name<<" receives blame for extern call to "<<v->name<<std::endl;
				#endif
				
				inTargetV->externCallLineNumbers.insert(v->line_num);
				// TODO: Make this part of the config file
			    if (v->name.find("gfortran_transfer") != std::string::npos) {
					inTargetV->isWritten = true;
				}
	        }
			else {
	            blameeNodes.push_back(tmpPair);
	        }
		}//end of for funcCalls		
	} //end of for in_edges
	
    if (blamedNodes.size() > 0) {
		transferExternCallEdges(blamedNodes, v->number, blameeNodes);		
	}
    else {	
#ifdef DEBUG_ERROR
		blame_info<<"The blamed node vector(extern) was empty for "<<v->name<<" in "<<getSourceFuncName()<<std::endl;
#endif
	}
}


