#ifndef _BFC_H
#define _BFC_H

/*
 *  BFC.h    //Blame For Chapel
 *  
 *  Comments:
 *	Top file of the pass, class inherits fom ModulePass, 
 *	Most important:  runOnModule
 *
 *  Created by Hui Zhang on 02/10/15.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "llvm/Analysis/Passes.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"

//#include "Parameters.h"

using namespace llvm;

namespace {

	class BFC : public ModulePass {
	
	private:
		DebugInfoFinder Finder;
		
	public:
		static char ID; // Pass identification, replacement for typeid
		BFC() : ModulePass(ID) {}
		
		virtual bool runOnModule(Module & M);
		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
		}
		virtual void print(raw_ostream &O, const Module *M) const {}
		
	};
	
	char BFC::ID = 0;
	RegisterPass<BFC> X("bfc",
                            "Calculates Implicit/Explicit Blame for Chapel at LLVM level");
}

#endif
