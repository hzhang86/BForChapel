/*
 *  ModuleBFC.h
 *  Data structure for a module, which is a translate unit in compilation,
 *  including global variables and functions
 *
 *  Created by Hui Zhang on 02/16/15.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


#ifndef _MODULE_BFC_H
#define _MODULE_BFC_H

#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Compiler.h"
#include "llvm/InstVisitor.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Dwarf.h" 
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/CFG.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/DominatorInternals.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.def"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/DebugInfo.h"

#include <algorithm>
#include <iostream>
#include <ostream>
//#include <boost/config.hpp>
//#include <boost/property_map.hpp>
#include <string>
#include <algorithm>
#include <set>
#include <vector>
#include <iterator>

#include "Parameters.h"

using namespace std;
using namespace llvm;

struct StructBFC;

struct StructField {

	string fieldName;
	int fieldNum;
	const llvm::Type * llvmType; // not reliable
	std::string typeName;  // different from LLVM Type

	StructBFC * parentStruct;

	StructField(int fn){fieldNum = fn;}

};

struct StructBFC {

	string structName;
	std::vector<StructField *> fields;
	
	std::string moduleName;
	std::string modulePathName;
	
	int lineNum;
	
	void setModuleNameAndPath(llvm::DIScope *contextInfo);
	//void getPathOrName(Value * v, bool isName);

	//void setModuleName(std::string rawName);
	//void setModulePathName(std::string rawName);
	
};


struct ModuleBFC {

	Module * M;
	
	ModuleBFC(Module * mod) {M = mod;}

	void addStructBFC(StructBFC * sb);
	
	bool parseDerivedType(DIType *dt, StructBFC *sb, StructField *sf, bool isField);
	bool parseCompositeType(DIType *dt, StructBFC *sb, bool isPrimary);
	void parseDITypes(DIType* dt);

	void printStructs();
	void exportStructs(std::ostream & O);
	
	StructBFC * structLookUp(std::string & sName);


	void handleStructs();
	std::vector<StructBFC *> structs;


};


#endif
