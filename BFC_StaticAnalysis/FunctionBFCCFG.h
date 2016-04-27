/*
 *  FunctionBFCCFG.h
 *  
 *  Function Control Flow Analysis part implementation
 *  Shared the same header file: FunctionBFC.h
 *
 *  Created by Hui Zhang on 03/25/15.
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _FUNCTION_BFC_CFG_H
#define _FUNCTION_BFC_CFG_H

//#define DEBUG_CFG_ERROR

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.def"

#include <set>

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "NodeProps.h"
#include "FunctionBFC.h"

#include <iostream>
#include <fstream>

namespace std
{
  using namespace __gnu_cxx;
}

/* namespace explanation
std::__gnu_cxx::list

1
std::__gnu_cxx::list mylist;

2
using namespace std;
__gnu_cxx::list mylist;

3
using namespace std::__gnuy_cxx;
list mylist;
*/

struct eqstr2
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) == 0;
  }
};

using namespace llvm;

class FunctionBFCBB {

public:
	string bbName;
	set<int> lineNumbers;
	string getName() {return bbName;}

	// To figure out the control flow and how it 
	//  affects variables we set up the gen/kill sets
	//  to use reaching definitions
	set<NodeProps *>  genBB;
	set<NodeProps *>  killBB;

	set<NodeProps *>  inBB;
	set<NodeProps *>  outBB;
	
	// Same thing but for the reaching defs for  Pointers
	set<NodeProps *>  genPTR_BB;
	set<NodeProps *>  killPTR_BB;

	set<NodeProps *>  inPTR_BB;
	set<NodeProps *>  outPTR_BB;
	
	// Ancestors and Descendants in CFG
	set<FunctionBFCBB *> ancestors;
	set<FunctionBFCBB *> descendants;
	
	void genD(FunctionBFCBB *fbb, std::set<FunctionBFCBB *> &visited);
	void genA(FunctionBFCBB *fbb, std::set<FunctionBFCBB *> &visited);
	// Predecessors and Successors in CFG
	set<FunctionBFCBB *>  preds;
	set<FunctionBFCBB *>  succs;
	
	void assignGenKill();
	void assignPTRGenKill();
	
	void sortInstructions() {std::sort(relevantInstructions.begin(), 
            relevantInstructions.end(), NodeProps::LinenumSort);}

	//  This should be in the same order as they are laid
	//    out int he LLVM code, we can start by sorting
	//    by line numbers, and then do tie breakers by 
	//    order in the LLVM code
	//  Only the NodeProps that contain stores are considered
	vector<NodeProps *> relevantInstructions;
	
	// relevantInstructions is usually applicable to local (non-pointer) variables
	// singleStores are for GEP grabs of pointers.  singleStores is a little misleading,
	// as the on GEP will only have one store to it, the variable the GEP grabbed from
	// may have multiple stores
	vector<NodeProps *> singleStores;
	llvm::BasicBlock *llvmBB;

public:
	FunctionBFCBB(llvm::BasicBlock * lbb) 
		{llvmBB = lbb; bbName = lbb->getName(); }

};


typedef std::hash_map<const char*, FunctionBFCBB*,std::hash<const char*>, eqstr2> BBHash;

class FunctionBFCCFG {

public:
	// order doesn't really matter since we are just iterating
	//  through until we have convergence
	BBHash  LLVM_BBs;
	
	// gen pred/succ edges between FunctionBFCBBs
	void genEdges();
	// gen ancestors & descendants
	void genAD();
	void setDomLines(NodeProps * vp);
	void printCFG(std::ostream & O);
	
	// for each BB in CFG, sort the edges
	void sortCFG(); 
	// Reaching Definitions for Primitives
	void reachingDefs();
	void calcStoreLines();
	void assignBBGenKill();

	// Reaching Definitions for Pointers
	void reachingPTRDefs();
	void calcPTRStoreLines();
	void assignPTRBBGenKill();
	
    bool controlDep(NodeProps * target, NodeProps * anchor, std::ofstream &blame_info);
};

#endif
