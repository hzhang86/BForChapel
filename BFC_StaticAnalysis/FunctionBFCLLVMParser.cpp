/*
 *  FunctionBFCLLVMParser.cpp
 *  
 *
 *  Created by Hui Zhang on 03/20/15.
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#include "ExitVars.h"
#include "ModuleBFC.h"
#include "FunctionBFC.h"
#include "FunctionBFCCFG.h"

#include <iostream>
#include <sstream> 
//#include "llvm/Support/raw_os_ostream.h"
//struct fltSemantics;

void FunctionBFC::parseLLVM(std::vector<NodeProps *> &globalVars)
{
    // push back all the exit variables in this function(ret, pointer params)
    determineFunctionExitStatus();
	// set isBFCPoint to be true if no exit variables
    RegHashProps::iterator begin, end;
  
    populateGlobals(globalVars);
	
    int varCount = globalVars.size(); 
    int currentLineNum = 0; 
  
    // We iterate through the function first basic block by basic block, 
    // then instruction by instruction (within basic block) to determine the 
    // symbols that will be used to create dependencies (put in "variables")
    for (Function::iterator b = func->begin(), be = func->end(); b != be; ++b) {
		
		FunctionBFCBB *fbb = new FunctionBFCBB(b); //FunctionBFCBB is defined in FunctionBFCCFG.h
		
		cfg->LLVM_BBs[b->getName().data()] = fbb;
		
        for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
            examineInstruction(i, varCount, currentLineNum, variables, fbb);			
        }
    }
	
	cfg->genEdges(); //gen pred/succ edges between FunctionBFCBBs
	cfg->genAD();   //gen ancestors & descendants of a FunctionBFCBB
  
	adjustLocalVars();
    generateImplicits();  
}

void FunctionBFC::adjustLocalVars()
{
	RegHashProps::iterator begin, end;
    /* Iterate through variables and label them with their node IDs (integers)
	 -> first is const char * associated with their name
	 -> second is NodeProps * for node
	*/
    //std::string chplName = NULL;
    //std::string realName = NULL;

    for (begin = variables.begin(), end = variables.end(); begin != end; begin++) {
		std::vector<LocalVar *>::iterator lv_i;
		for (lv_i = localVars.begin();  lv_i != localVars.end(); lv_i++) {
            std::string chplName = std::string(begin->first);
            //std::string realName;
            //if (has_suffix(chplName, "_chpl"))
            //    realName = chplName.substr(0, chplName.size()-5);
            //else realName.assign(chplName); //for Vars not ended with _chpl
            if (chplName.compare((*lv_i)->varName) == 0) {
#ifdef DEBUG_LOCALS			
                blame_info<<"Local Var found begin="<<begin->first<<
                    ", it's line_num will be "<<(*lv_i)->definedLine<<std::endl;
#endif				
                NodeProps *v = begin->second;
                v->isLocalVar = true;
                v->line_num = (*lv_i)->definedLine;
                allLineNums.insert(v->line_num);
            
                if (((*lv_i)->varName.find(".") != std::string::npos || 
                    (*lv_i)->varName.find("0x") != std::string::npos )
                    &&  (*lv_i)->varName.find("equiv.") == std::string::npos   
                    &&  (*lv_i)->varName.find("result.") == std::string::npos) 
                    //TC: not sure about above conds
                {
                    v->isFakeLocal = true; //no use later
                }
            }
#ifdef DEBUG_LOCALS
            else
                blame_info<<"Local Var Not Found: begin = "<<begin->first<< \
                    ", lv_i = "<<(*lv_i)->varName.c_str()<<std::endl;
#endif
		}
	}
}


void FunctionBFC::populateGlobals(std::vector<NodeProps *> &gvs)
{
  std::vector<NodeProps *>::iterator gv_i, gv_e;
  for (gv_i = gvs.begin(), gv_e = gvs.end(); gv_i != gv_e; gv_i++) {
		NodeProps *global = *gv_i;
		
#ifdef DEBUG_VP_CREATE
		blame_info<<"Adding NodeProps(9) for "<<global->name<<std::endl;
#endif
		
		NodeProps *np = new NodeProps(global->number, global->name, global->line_num, global->llvm_inst);
		np->isGlobal = true;
		np->impNumber = 0;
#ifdef DEBUG_GLOBALS
		blame_info<<"Globals__(populateGlobals)--creating global NP for "<<np->name.c_str()<<std::endl;
#endif
		variables[np->name.c_str()] = np; //RegHashProps variables: char* --> NodeProps*
		//addGlobalExitVar(new ExitVariable(vp->name.c_str(), GLOBAL, -1));
		addExitVar(new ExitVariable(np->name, GLOBAL, -2, false)); //changed by Hui 03/15/16: whichParam from -1 to -2
		                                                        //because we set retVal to -1 now
	}
}


void FunctionBFC::propagateConditional(DominatorTreeBase<BasicBlock> *DT, const DomTreeNodeBase<BasicBlock> *N, 
														            const char *condName, BasicBlock *termNode)
{
	BasicBlock *rootBlock = N->getBlock();
	if (rootBlock == termNode)
		return;
  
    if (rootBlock->hasName()) {
		std::set<const char*, ltstr> t;
		ImpRegSet::iterator irs_i;
		
		irs_i = iReg.find(rootBlock->getName().data());
		if (irs_i != iReg.end()) {
			t = iReg[rootBlock->getName().data()];
		}
		
		if (condName != NULL) {
			t.insert(condName);
#ifdef DEBUG_LLVM_IMPLICIT
			blame_info<<"Inserting (4)"<<condName<<" into "<<rootBlock->getName().str()<<std::endl;
#endif
		}
		
		iReg[rootBlock->getName().data()] = t; //after update t, update t in iReg
		//blame_info<<"New iReg (2)"<<(unsigned) &(iReg[rootBlock->getNameStart()])<<std::endl;
	}
  
    for (DomTreeNodeBase<BasicBlock>::const_iterator I = N->begin(), E = N->end(); I != E; ++I)
		propagateConditional(DT, *I, condName, termNode); 	

}


void FunctionBFC::gatherAllDescendants(DominatorTreeBase<BasicBlock> *DT, BasicBlock *original, 
                BasicBlock *&b, std::set<BasicBlock *> &cfgDesc, std::set<BasicBlock *> &visited)
{
	if (visited.count(b))
		return;
	visited.insert(b);
		
	if (DT->properlyDominates(b, original)) //if b dominates original
		return;
	
	for (succ_iterator Iter = succ_begin(b), En = succ_end(b); Iter != En; ++Iter) {
		BasicBlock *bb = *Iter;
		cfgDesc.insert(bb);
		gatherAllDescendants(DT, original, bb, cfgDesc, visited);
	}
}


void FunctionBFC::handleOneConditional(DominatorTreeBase<BasicBlock> *DT, \
        const DomTreeNodeBase<BasicBlock> *N, BranchInst *br)
{
#ifdef DEBUG_LLVM_IMPLICIT
    blame_info<<"Looking at handling conditional for "<<N->getBlock()->getName().str() \
        <<" for cond name "<<br->getCondition()->getName().str()<<std::endl;
#endif
    BasicBlock *b = N->getBlock();
	
	// We essentially need to find the terminal node for each dominator 
    // and make sure that any if/else blame does not go past the terminal node for this.  
    // Based on whether there are 2 or 3 immediately dominated nodes we find the terminal node
	// in different ways
	
	size_t numChildren = N->getNumChildren();
#ifdef DEBUG_LLVM_IMPLICIT
	blame_info<<"Number of children is "<<numChildren<<std::endl;
#endif
	BasicBlock *termBB = NULL;
	std::set<DomTreeNodeBase<BasicBlock> *> blocks;
	
	// Case where we have 
	// if ( bool)
	//    { body }
	// one child is body, one child is terminal
	
	//     OR
	
	//  else if (bool)
	//  {body}
	//  else
	//   {body}
	//  both children are bodies, terminal is not accessible by dominator tree, need to use CFG
	//  term in that case is first shared node in both CFG successors
	if (numChildren == 2) {
		BasicBlock *succ1 = br->getSuccessor(0);
		BasicBlock *succ2 = br->getSuccessor(1);
		
#ifdef DEBUG_LLVM_IMPLICIT
		blame_info<<"First successor is "<<succ1->getName().str()<<std::endl;
		blame_info<<"Second successor is "<<succ2->getName().str()<<std::endl;
#endif 
		// Second successor is either
		//  a) the terminal node (in the case of a straight if() with no else if or else
		//  b) the else body in the case of a else if/else situation (if/else with no more else if,which  has>=3 children)
		
		// First, lets handle the straight up if case
		// If an immediate dominated node is equal to the second successor then it must be the terminal 
		std::set<BasicBlock *> cfgDesc;
		std::set<BasicBlock *> visited;
		
		// Insert parent so we don't traverse the other conditional in the case of a loop
		visited.insert(b);
		gatherAllDescendants(DT, succ1, succ1, cfgDesc, visited);
		
		// If one of the descendants to succ1 is succ2 then we know that succ2 is the terminal
		if (cfgDesc.count(succ2)) {
#ifdef DEBUG_LLVM_IMPLICIT
			blame_info<<"Standard if, no else -- "<<succ2->getName().str()<<" is terminal."<<std::endl;
#endif
			termBB = succ2;
		}
		
		// If we don't find a terminal that's fine, the if/else and elses only dominate the stuff they should
		// The problem with the straight up if case is their basic block can dominate all the linear code
		// that comes after it
	
		for (DomTreeNodeBase<BasicBlock>::const_iterator I = N->begin(), E = N->end(); I != E; ++I) {
			BasicBlock *dest = (*I)->getBlock(); //get the bb of its children
			if (dest == succ1) {
				DomTreeNodeBase<BasicBlock> *dbb = *I;
				blocks.insert(dbb);
			}
			else if (dest == succ2) {
			    // if the terminal equal null it means the second dominated block was an if/else or else block
				if (termBB == NULL) {
					DomTreeNodeBase<BasicBlock> *dbb = *I;
					blocks.insert(dbb);
				}
			}
		}
	} //end of if(numChildren==2)
	
	// Case where we have
	//   if ()
	//   {body}
	//   else if ()
	//   { body}
	//  else()
	//  one child is if body, one child is first else if test, one child is block that merges all conditions at end
	else if (numChildren == 3) {
		// This one is much easier.  If the successor in the CFG is in the dominator tree then it is one of the bodies,
		//  otherwise it is the terminal node
		
		for (DomTreeNodeBase<BasicBlock>::const_iterator I = N->begin(), E = N->end(); I != E; ++I) {
			BasicBlock *dest = (*I)->getBlock(); 
			int match = 0;
			
			// for all children in CFG
			for (succ_iterator Iter = succ_begin(b), En = succ_end(b); Iter != En; ++Iter) {
#ifdef DEBUG_LLVM_IMPLICIT
				blame_info<<"CFG - "<<Iter->getName().str()<<" D - "<<dest->getName().str()<<std::endl;
#endif 
				if (strcmp(Iter->getName().data(), dest->getName().data()) == 0)
					match++;
			}
			
			if (match == 0) {
#ifdef DEBUG_LLVM_IMPLICIT
				blame_info<<"Terminal for if/else if/else case is -- "<<dest->getName().str()<<std::endl;
#endif
				termBB = dest;
			}
			else if (match) {
#ifdef DEBUG_LLVM_IMPLICIT
				blame_info<<"Block "<<dest->getName().str()<<" is a block to be inserted."<<std::endl;
#endif
				DomTreeNodeBase<BasicBlock> *dbb = *I;
				blocks.insert(dbb);
			}
		}	
	}
	
	std::set< DomTreeNodeBase<BasicBlock> *>::iterator set_dbb_i;
	
	for (set_dbb_i = blocks.begin(); set_dbb_i != blocks.end(); set_dbb_i++) {
		DomTreeNodeBase<BasicBlock> *dbb = *set_dbb_i;
		if (br->getCondition()->hasName())
			propagateConditional(DT, dbb, br->getCondition()->getName().data(), termBB);		
		else {
			char tempBuf2[18];
			sprintf(tempBuf2, "0x%x", /*(unsigned)*/br->getCondition());
			std::string name(tempBuf2);
			
			if (variables.count(name.c_str())) {
				NodeProps *vp = variables[name.c_str()];
				propagateConditional(DT, dbb, vp->name.c_str(), termBB);	
			}							
		}
	}
		
	/*
	 for (DomTreeNodeBase<BasicBlock>::const_iterator I = N->begin(), E = N->end(); I != E; ++I)
	 {
	 BasicBlock * dest = (*I)->getBlock(); 
	 int match = 0;
	 
	 // for all children in CFG
	 for (succ_iterator Iter = succ_begin(b), En = succ_end(b); Iter != En; ++Iter) {
	 blame_info<<"CFG - "<<Iter->getNameStart()<<" D - "<<dest->getNameStart()<<std::endl;
	 if (strcmp(Iter->getNameStart(), dest->getNameStart()) == 0)
	 match++;
	 }
	 
	 //if (v → vchild ∈ D and CFG.v → CFG.vchild ∈ E) 
	 if ( match )
	 propagateConditional(*I, condName); 	
	 }
	 */
}


bool FunctionBFC::errorRetCheck(User *v)
{
#ifdef DEBUG_ERR_RET
	blame_info<<"in errorRetCheck for "<<v->getName().str()<<std::endl;
#endif
	Instruction *icmpInst = NULL;
	
	if (isa<Instruction>(v)) {
		icmpInst = dyn_cast<Instruction>(v);
#ifdef DEBUG_ERR_RET
		blame_info<<"ToBool instruction is "<<icmpInst->getName().str()<<" op "
            <<icmpInst->getOpcodeName()<<std::endl;
#endif 
	}
	else	
		return true;
	
	while (icmpInst->getOpcode() == Instruction::ICmp) {
		Value *zextVal = icmpInst->getOperand(0);   //TC: zextVal should be the keyword: 
		if (isa<Instruction>(zextVal)) {            //condition code, like eq, ugt...why would be ZExt/Load?
			Instruction *zextInst = dyn_cast<Instruction>(zextVal);		
			if (zextInst->getOpcode() == Instruction::ZExt) {
			#ifdef DEBUG_ERR_RET
				blame_info<<"ZEXT inst is "<<zextInst->getName().str()<<std::endl;
			#endif 
				Value *icmpVal = zextInst->getOperand(0);
				
				if (isa<Instruction>(icmpVal)) 
					icmpInst = dyn_cast<Instruction>(icmpVal);	
				else
					return true;
			}

            else if (zextInst->getOpcode() == Instruction::Load) {
			#ifdef DEBUG_ERR_RET
				blame_info<<"Load inst is "<<zextInst->getName().str()<<std::endl;
			#endif 
				Value *condLocal = zextInst->getOperand(0); //First operand of Load is the ptr(mem addr)
			#ifdef DEBUG_ERR_RET
			    blame_info<<"CondLocal is "<<condLocal->getName().str()<<std::endl;
			#endif 
				if (condLocal->getName().str().find("ierr") != std::string::npos) {
                    std::cout<<"errorRetCheck returns false on "<<zextInst->getName().str()<<std::endl;
                    return false;
                }
				else
					return true;
			}

			else
				return true;
		}

		else
			return true;
	}
	
	return true;
}


void FunctionBFC::handleAllConditionals(DominatorTreeBase<BasicBlock> *DT, const DomTreeNodeBase<BasicBlock> *N, 
											LoopInfoBase<BasicBlock,Loop> &LI, std::set<BasicBlock *> &termBlocks)
{
  BasicBlock *b = N->getBlock();
  
  // Only concerned with branch instructions
  if (isa<BranchInst>(b->getTerminator())) {
#ifdef DEBUG_LLVM_IMPLICIT
		blame_info<<"Branch Instruction found for basic block "<<b->getName().str()<<std::endl;
#endif
		BranchInst *bBr = cast<BranchInst>(b->getTerminator());	
		
#ifdef DEBUG_LLVM_IMPLICIT
		if (bBr->isConditional())
			blame_info<<"... is conditional"<<std::endl;
		
		if (!LI.isLoopHeader(b))
			blame_info<<"... not loop header"<<std::endl;
#endif 		
		// Only concerend with conditional non loop header instructions		
		if (bBr->isConditional() && !LI.isLoopHeader(b)) {
			bool eRC = true;	
			if (isa<Instruction>(bBr->getCondition())){	
				Instruction *icmpInst = dyn_cast<Instruction>(bBr->getCondition());
				eRC = errorRetCheck(icmpInst);	
			}			
			#ifdef DEBUG_ERR_RET
			blame_info<<"... is an error checking call."<<std::endl;
			#endif 
			if (eRC)	
				handleOneConditional(DT, N, bBr);	
		}
	}
    // for all children in dominator tree
    for (DomTreeNodeBase<BasicBlock>::const_iterator I = N->begin(), E = N->end(); I != E; ++I)
        handleAllConditionals(DT, *I, LI, termBlocks); //TC: what's termBlocks for ?		
}


void FunctionBFC::handleLoops(LoopBase<BasicBlock,Loop> *lb)
{
    if(lb == NULL) //added to end the recursion
        return; 
    else
#ifdef DEBUG_LLVM_IMPLICIT
	    blame_info<<"Entering handleLoops "<<std::endl;
#endif 
	
    //short blockCount = 0;
    BranchInst *bBr = NULL;
	// These will be the vertices for each loop that will be blamed
	std::set<NodeProps *> blamedConditions;
	
	// We get all the exit blocks in the loop using LLVM calls
	SmallVector<BasicBlock *, 8> exitBlocks;
	lb->getExitBlocks(exitBlocks);
	SmallVectorImpl<BasicBlock *>::iterator svimpl_i;
	
#ifdef DEBUG_LLVM_IMPLICIT
	for (svimpl_i = exitBlocks.begin(); svimpl_i != exitBlocks.end(); svimpl_i++) {
		BasicBlock *bb = *svimpl_i;
		blame_info<<"Exit Block is "<<bb->getName().str()<<std::endl;
	}
#endif 
	
	// We go through every basic block in the loop and find the edgs that
	// go to exit basic blocks, we identify those edges as edges that have
	// conditions that dictate the loop, and the values in the loop are 
	// dependent on them
    for (LoopBase<BasicBlock,Loop>::block_iterator li_bi = lb->block_begin(); 
        li_bi != lb->block_end(); li_bi++) {
		BasicBlock *bb = *li_bi;
		
		if (isa<BranchInst>(bb->getTerminator())) {
			bBr = cast<BranchInst>(bb->getTerminator());
			if (bBr->isConditional()) {
#ifdef DEBUG_LLVM_IMPLICIT
				blame_info<<"Terminator is "<<bBr->getCondition()->getName().str()
                    <<" "<<bBr->getNumOperands()<<std::endl;
#endif 				
				Value *bCondVal = bBr->getCondition();
				
				bool eRC = true;	//QUESTION: NOT SURE ABOUT THE REASON FOR ERC
				
				if (isa<Instruction>(bCondVal)) {	
					Instruction *icmpInst = cast<Instruction>(bCondVal);
					eRC = errorRetCheck(icmpInst);	
				}
				
				if (eRC == false) {
				#ifdef DEBUG_ERR_RET
					blame_info<<"Not adding break from loop due to error checking."<<std::endl;
					#endif 
					continue;
				}
				
				for (unsigned i = 0; i < bBr->getNumSuccessors(); i++) {
					BasicBlock *bbSucc = bBr->getSuccessor(i);
					
#ifdef DEBUG_LLVM_IMPLICIT
					blame_info<<"Examining successor "<<bbSucc->getName().str()<<std::endl;
#endif
					for (svimpl_i = exitBlocks.begin(); svimpl_i != exitBlocks.end(); svimpl_i++) {
						BasicBlock * bbMatch = *svimpl_i;			
						if (bbMatch == bbSucc) {
#ifdef DEBUG_LLVM_IMPLICIT
							blame_info<<"Matching Exit Block is "<<bbMatch->getName().str()<<" for "; 
							if (bCondVal->hasName())
								blame_info<<bCondVal->getName().str()<<std::endl;
							else
								blame_info<<std::hex<</*(unsigned)*/ bCondVal<<std::dec<<std::endl;
#endif
							if (bCondVal->hasName()) {
								if (variables.count(bCondVal->getName().data()))
									blamedConditions.insert(variables[bCondVal->getName().data()]);
							}
							else {
								char tempBuf2[18];
								sprintf(tempBuf2, "0x%x", /*(unsigned)*/bBr->getCondition());
								std::string name(tempBuf2);
								
								if (variables.count(name.c_str())) {
									blamedConditions.insert(variables[name.c_str()]);
								}							
							}// end else for having a name
						}// end if (bbMatch == bbSucc)
					}// end for (exitBlocks)
				}//end successors
			} // end isConditional
		}// end getTerminator()
	} // end basic block iterator 
	
	
	// We couldn't find any traditional ones, so now we do deeper analysis,
	//  sometimes we have to deal with switch statements
	if (blamedConditions.size() == 0) {
#ifdef DEBUG_LLVM_IMPLICIT
		blame_info<<"...Now looking through switch statements "<<std::endl;
#endif
	  for (LoopBase<BasicBlock,Loop>::block_iterator li_bi = lb->block_begin(); 
				 li_bi != lb->block_end(); li_bi++) {
			BasicBlock *bb = *li_bi;
			
			if (blamedConditions.size())
				break;
			
			for (BasicBlock::iterator i = bb->begin(), ie = bb->end(); i != ie; ++i) {
				Instruction *pi = i;
				if (pi->getOpcode() == Instruction::Switch) {
					SwitchInst *bBr = dyn_cast<SwitchInst>(pi);
					Value *v = bBr->getCondition();
					for (unsigned i = 0; i < bBr->getNumSuccessors(); i++) {
						BasicBlock *bbSucc = bBr->getSuccessor(i);
						
#ifdef DEBUG_LLVM_IMPLICIT
						blame_info<<"Examining successor "<<bbSucc->getName().str()<<std::endl;
#endif 
						for (svimpl_i = exitBlocks.begin(); svimpl_i != exitBlocks.end(); svimpl_i++) {
							BasicBlock * bbMatch = *svimpl_i;	
							if (bbMatch == bbSucc) {
#ifdef DEBUG_LLVM_IMPLICIT
								blame_info<<"Matching Exit Block is "<<bbMatch->getName().str()<<" for ";
#endif
#ifdef DEBUG_LLVM_IMPLICIT
								if (v->hasName())
									blame_info<<v->getName().str()<<std::endl;
								else
									blame_info<<std::hex<</*(unsigned)*/ v<<std::dec<<std::endl;
#endif 								
								if (v->hasName()) {
									if (variables.count(v->getName().data()))
										blamedConditions.insert(variables[v->getName().data()]);
								}
								else {
									char tempBuf2[18];
									sprintf(tempBuf2, "0x%x", /*(unsigned)*/bBr->getCondition());
									std::string name(tempBuf2);
									
									if (variables.count(name.c_str())) {
										blamedConditions.insert(variables[name.c_str()]);
									}							
								}// end else for having a name
							}// end if (bbMatch == bbSucc)
						}// end for (exitBlocks)
					}//end successors
				}// end isSwitch
			}	// end Instruction iterator
		} // end Basic Block iterator
	} // end if (blamedConditions.size())
	
	// Now we go through all the basic blocks and add the blame nodes discovered from above to the set
    // QUESTION: same blame nodes set for each bb in the loop ??
    for (LoopBase<BasicBlock,Loop>::block_iterator li_bi = lb->block_begin(); 
        li_bi != lb->block_end(); li_bi++) {
		
		std::set<const char*, ltstr> t; // every bb has a 't'
		BasicBlock *bb = *li_bi;
		ImpRegSet::iterator irs_i; //ptr to each pair: (bb_name, set 't')
		
#ifdef DEBUG_LLVM_IMPLICIT
		blame_info<<"Loop now in basic block "<<bb->getName().str()<<std::endl;
#endif
		
		if (bb->hasName()) {
			irs_i = iReg.find(bb->getName().data());
			if (irs_i != iReg.end())
				t = iReg[bb->getName().data()];
			
			std::set<NodeProps *>::iterator set_vp_i, set_vp_e;
			for (set_vp_i = blamedConditions.begin(), set_vp_e = blamedConditions.end(); set_vp_i != set_vp_e; set_vp_i++) {
				NodeProps *vp = *set_vp_i;
#ifdef DEBUG_LLVM_IMPLICIT
				blame_info<<"Implicit -- Inserting "<<vp->name<<" into loop implicit set for bb:"<<bb->getName().str()<<std::endl;
#endif 
				t.insert(vp->name.c_str());
			}
			
			iReg[bb->getName().data()] = t;
			
		}
	}
  
    for (LoopBase<BasicBlock,Loop>::iterator li_si = lb->begin(); li_si != lb->end(); li_si++) 
		handleLoops(*li_si); //start handling all subloops in this loop
                             //since it's the last call in recursion, it'll terminate anyway
#ifdef DEBUG_LLVM_IMPLICIT
	blame_info<<"Exiting handleLoops "<<std::endl;
#endif
	
}

void FunctionBFC::generateImplicits()
{
    // Create Dominator Tree for Function
    DominatorTreeBase<BasicBlock> *DT = new DominatorTreeBase<BasicBlock>(false);	
    DT->recalculate(*func);
  
    std::string dom_path("DOMINATOR/");
    std::string func_name = func->getName();
    std::string dot_extension(".dot");
    dom_path += func_name;
    dom_path += dot_extension;
    std::ofstream dot_file(dom_path.c_str()); //Not used except 'print2' below
    //DT->print2(dot_file, 0);
    
    LoopInfoBase<BasicBlock,Loop> LI;
    //LI.Calculate(*DT);
    LI.Analyze(*DT); //TC: new func in llvm 3.3 (to replace Calculate in 2.5)
    for (LoopInfoBase<BasicBlock,Loop>::iterator li_i = LI.begin(); li_i != LI.end(); li_i++)
        handleLoops(*li_i); //typedef typename std::vector<LoopT *>::const_iterator iterator
  
	std::set<BasicBlock *> terminalNodes;
	
    handleAllConditionals(DT, DT->getRootNode(), LI, terminalNodes);
		
	delete(DT);
}

// TODO: Make constants a little more intuitive
void FunctionBFC::determineFunctionExitStatus()
{
	bool varLengthPar = varLengthParams();
	
    // The array will always be at least size 1 to account for return value
    // which always is "parameter 0", if var length, we set it to MAX_PARAMS
	if (varLengthPar) {
		numParams = MAX_PARAMS + 1; //MAX_PARAMS = 128  
		isVarLen = true;
	}
	else {
		numParams = func->arg_size() + 1; //return size_t,which is unsigned in x86
		isVarLen = false;
	}
	
	// Check the return type
    if (func->getReturnType()->getTypeID() == Type::VoidTyID) {
		voidReturn = true;
	}
    else {
		ExitVariable *ev = new ExitVariable(std::string("DEFAULT_RET"), RET, -1, false); //{realName, ExitType, whichParam, isStructPtr}, changed by Hui from 0 to -1
		addExitVar(ev); //exitVariables.push_back(ev)
	}
	
  //bool isParam = false;
#ifdef DEBUG_LLVM	
  blame_info<<"LLVM__(checkFunctionProto) - Number of args is "<<func->arg_size()<<std::endl;
#endif
	
    int whichParam = 0;
	
    // Iterates through all parameters for a function 
    for(Function::arg_iterator af_i = func->arg_begin(); af_i != func->arg_end(); af_i++) {
		//whichParam++; //commented out by Hui 03/15/16, now real params of func starts from #0
		Value * v = af_i;
		
		// EXIT VAR HERE
		
		// We are only concerned with parameters that are pointers 
		if (v->getType()->getTypeID() == Type::PointerTyID) {
#ifdef DEBUG_LLVM	
			blame_info<<v->getName().str()<<" is a pointer parameter"<<std::endl;
#endif
			numPointerParams++;
			
			std::string origTStr = returnTypeName(v->getType(), std::string(" "));
			
			bool isStructPtr = false;		
			// Dealing with a struct pointer
			if (origTStr.find("Struct") != std::string::npos) {
				isStructPtr = true;
			}
			// Add exit variable for address of parameter since that's where the blame will eventually point to
            // "SURE" FOR NOW: Here,in <=3.3 LLVM, I think deference the use_iterator (*use_iterator) will give you the Value*( or User*)
            // In ?~3.7 LLVM, you should use user_iterator instead of use_iterator directly for the following purpose 
			for (Value::use_iterator u_i = v->use_begin(), u_e = v->use_end(); u_i != u_e; ++u_i) { 
				// Verify that the Value is that of an instruction 
				if (Instruction *i = dyn_cast<Instruction>(*u_i)) {
					if (i->getOpcode() == Instruction::BitCast)	{ //All OpCode can be found in instruction.def
						//Value * bcV = i;
						for (Value::use_iterator u_i2 = i->use_begin(), u_e2 = i->use_end(); u_i2 != u_e2; ++u_i2) { // what is a use of an instruction ?
							// Verify that the Value is that of an instruction 
							if (Instruction *i2 = dyn_cast<Instruction>(*u_i2)) {
								if (i2->getOpcode() == Instruction::Store) {
									User::op_iterator op_i = i2->op_begin(); //typedef Use* op_iterator
									//Value  *first = *op_i,  
									//Hui: test which is EV? 11/30/15
                                    //Value *second = *op_i;
                                    Value *second = *(++op_i); // second is the actual mem address where to store the value
									
									if (second->hasName()) {	
										const llvm::Type * origT = second->getType();		
										std::string origTStr = returnTypeName(origT, std::string(" "));
										
										int ptrLevel = pointerLevel(origT, 0); //ptrLevel = 1 means it's a 1-level pointer, like int *ip
										
										if (ptrLevel > 1 || origTStr.find("Array") != std::string::npos || (origTStr.find("Struct") != std::string::npos)) { 						
											addExitVar(new ExitVariable(second->getName().str(), PARAM, whichParam, isStructPtr));
#ifdef DEBUG_LLVM	
											blame_info<<"LLVM_(checkFunctionProto) - Adding exit var(2) "<<second->getName().str();
											blame_info<<" Param "<<whichParam<<" in "<<getSourceFuncName()<<std::endl;
#endif
										}
									}
									else {
#ifdef DEBUG_LLVM	
										blame_info<<"LLVM_(checkFunctionProto) - what's going on here(2) for "<<second->getName().str()<<std::endl;
										//blame_info<<" and "<<first->getName()<<std::endl;
#endif
									}
								}
							}
						}
					}
					else if (i->getOpcode() == Instruction::Store) {
						User::op_iterator op_i = i->op_begin();
						//Hui: test which is the EV? 11/30/15
                        //Value *second = *op_i, *first = *(++op_i);
                        Value  *first = *op_i,  *second = *(++op_i);
						
						if (first->hasName() && second->hasName()) {	
							const llvm::Type *origT = second->getType();		
							std::string origTStr = returnTypeName(origT, std::string(" "));
							
							int ptrLevel = pointerLevel(origT, 0);
							
							if (ptrLevel > 1 || origTStr.find("Array") != std::string::npos ||
									(origTStr.find("Struct") != std::string::npos)) {						
								
								addExitVar(new ExitVariable(second->getName().str(), PARAM, whichParam, isStructPtr));
#ifdef DEBUG_LLVM	
								blame_info<<"LLVM_(checkFunctionProto) - Adding exit var "<<second->getName().str();
								blame_info<<" Param "<<whichParam<<" in "<<getSourceFuncName()<<std::endl;
#endif
							}
						}
						else {
#ifdef DEBUG_LLVM	
							blame_info<<"LLVM_(checkFunctionProto) - what's going on here for "<<second->getName().str();
							blame_info<<" and "<<first->getName().str()<<std::endl;
#endif
						}
					}
				}
			}	
        }	
        whichParam++; //added by Hui 03/15/16, moved from the beginning
	}	
	
	if (numPointerParams == 0 && voidReturn == true) {
		isBFCPoint = true; // if the func has 0 pointer param and returns nothing, then it's a blame point ???
#ifdef DEBUG_LLVM		
		blame_info<<"IS BP - "<<numPointerParams<<" "<<voidReturn<<std::endl;  
#endif		
	}
	
}



//TODO:: GlobalVariable
/*
 VoidTyID 	0: type with no size
 HalfTyID   1: 16-bit floating point type
 FloatTyID 	2: 32 bit floating point type
 DoubleTyID 	3: 64 bit floating point type
 X86_FP80TyID 	4: 80 bit floating point type (X87)
 FP128TyID 	5: 128 bit floating point type (112-bit mantissa)
 PPC_FP128TyID 	6: 128 bit floating point type (two 64-bits, PowerPC)
 LabelTyID 	7: Labels
 MetadataTyID  8: Metadata
 X86_MMXTyID    9: MMX vectors (64 bts, X86 specific)

 IntegerTyID 	10: Arbitrary bit width integers
 FunctionTyID 	11: Functions
 StructTyID 	12: Structures
 ArrayTyID 	13: Arrays
 PointerTyID 	14: Pointers
 VectorTyID 	15: SIMD 'packed' format, or other vector type
 
 NumTypeIDs     // Must remain as last defined ID
 LastPrimitiveTyID = X86_MMXTyID
 FirstDerivedTyID = IntegerTyID
*/
/* Given a type return the string that describes the type */
std::string FunctionBFC::returnTypeName(const llvm::Type *t, std::string prefix)
{
	if (t == NULL)
		return prefix += std::string("NULL");
	
	unsigned typeVal = t->getTypeID();
	
    if (typeVal == Type::VoidTyID)
        return prefix += std::string("Void");
    else if (typeVal == Type::FloatTyID)
        return prefix += std::string("Float");
    else if (typeVal == Type::DoubleTyID)
        return prefix += std::string("Double");
    else if (typeVal == Type::X86_FP80TyID)
        return prefix += std::string("80 bit FP");
    else if (typeVal == Type::FP128TyID)
        return prefix += std::string("128 bit FP");
    else if (typeVal == Type::PPC_FP128TyID)
        return prefix += std::string("2-64 bit FP");
    else if (typeVal == Type::LabelTyID)
        return prefix += std::string("Label");
    else if (typeVal == Type::MetadataTyID)
        return prefix += std::string("Metadata");
    else if (typeVal == Type::IntegerTyID)
        return prefix += std::string("Int");
    else if (typeVal == Type::FunctionTyID)
        return prefix += std::string("Function");
    else if (typeVal == Type::StructTyID)
        return prefix += std::string("Struct");
    else if (typeVal == Type::ArrayTyID)
        return prefix += std::string("Array");
    else if (typeVal == Type::PointerTyID)
        return prefix += returnTypeName(cast<PointerType>(t)->getElementType() ,
																		std::string("*"));
    else if (typeVal == Type::VectorTyID)
        return prefix += std::string("Vector");
    else
        return prefix += std::string("UNKNOWN");
}



void printConstantType(Value * compUnit)
{
	
  if (isa<GlobalValue>(compUnit))
    std::cout<<"is Global Val";
  else if (isa<ConstantStruct>(compUnit))
    std::cout<<"is ConstantStruct";
  else if (isa<ConstantPointerNull>(compUnit))
    std::cout<<"is CPNull";
  else if (isa<ConstantAggregateZero>(compUnit))
    std::cout<<"is ConstantAggZero";
  else if (isa<ConstantArray>(compUnit))
    std::cout<<"is C Array";
  else if (isa<ConstantExpr>(compUnit))
    std::cout<<"is C Expr";
  else if (isa<ConstantFP>(compUnit))
    std::cout<<"is C FP";
  else if (isa<UndefValue>(compUnit))
    std::cout<<"is UndefValue";
  else if (isa<ConstantInt>(compUnit))
    std::cout<<"is C Int";
  else
    std::cout<<"is ?";
	
  std::cout<<endl;
	
}



void FunctionBFC::structResolve(Value *v, int fieldNum, NodeProps *fieldVP)
{
	const llvm::Type *pointT = v->getType();
	unsigned typeVal = pointT->getTypeID();
	
#ifdef DEBUG_STRUCTS
	blame_info<<"structResolve Here"<<std::endl;
#endif
	
    while (typeVal == Type::PointerTyID) {		
		pointT = cast<PointerType>(pointT)->getElementType();
		//std::string origTStr = returnTypeName(pointT, std::string(" "));
		typeVal = pointT->getTypeID();
	}
#ifdef DEBUG_STRUCTS
	blame_info<<"structResolve Here(2)"<<std::endl;
#endif
    if (typeVal == Type::StructTyID) {
		const llvm::StructType * type = cast<StructType>(pointT);
		string structNameFull = type->getStructName().str();
#ifdef DEBUG_STRUCTS
		blame_info<<"structNameFull -- "<<structNameFull<<std::endl;
#endif

#ifdef USE_LLVM25
		if (structNameFull.find("struct.") == std::string::npos) {
#ifdef DEBUG_ERROR
			blame_info<<"structName is incomplete--"<<structNameFull<<std::endl;
			std::cerr<<"structName is incomplete--"<<structNameFull<<std::endl;
#endif
			return;
		}
		// need to get rid of preceding "struct." and trailing NULL character
		string justStructName = structNameFull.substr(7, structNameFull.length() - 7 );
		StructBFC *sb = mb->structLookUp(justStructName);
#else
        StructBFC *sb = mb->structLookUp(structNameFull);
#endif
		if (sb == NULL)
			return;
		
#ifdef DEBUG_STRUCTS
		blame_info<<"Found sb for "<<structNameFull<<std::endl;
#endif
		
		// TODO: Hash
		std::vector<StructField *>::iterator vec_sf_i;
		for (vec_sf_i = sb->fields.begin(); vec_sf_i != sb->fields.end(); vec_sf_i++)
        {
			StructField *sf = (*vec_sf_i);
			if (sf->fieldNum == fieldNum) {				
#ifdef DEBUG_STRUCTS
				blame_info<<"Assigning fieldVP->sfield "<<fieldVP->name<<" to "<<sf->fieldName<<std::endl;
                //blame_info<<returnTypeName(sf->llvmType, std::string(" "));
#endif
				//fieldVP->name = newFieldName;
				//fieldVP->structName = sb->structName;
				fieldVP->sField = sf;
			}
		}
	}
}


void FunctionBFC::structDump(Value * compUnit)
{
#ifdef DEBUG_STRUCTS
	unsigned numStructElements;
	const llvm::Type * pointT = compUnit->getType();
	unsigned typeVal = pointT->getTypeID();
    //llvm::raw_os_ostream OS(blame_info);
	
    while (typeVal == Type::PointerTyID) {		
		pointT = cast<PointerType>(pointT)->getElementType();
		//std::string origTStr = returnTypeName(pointT, std::string(" "));
		typeVal = pointT->getTypeID();
	}
		
    if (typeVal == Type::StructTyID) {
		const llvm::StructType *type = cast<StructType>(pointT);
		numStructElements = cast<StructType>(pointT)->getNumElements();
		blame_info<<"Num of elements "<<numStructElements<<std::endl;
		//std::cout<<"TYPE - "<<type->getDescription()<<std::endl;
		blame_info<<"TYPE - "<<returnTypeName(type, std::string(" "))<<" "<<type->getName().data()<<std::endl;
		
		for (int eleBegin = 0, eleEnd = type->getNumElements(); eleBegin != eleEnd; eleBegin++) {
			llvm::Type *elem = type->getElementType(eleBegin);
            //elem->print(OS);
            blame_info<<"typeid: "<<elem->getTypeID()<<std::endl;
			blame_info<<endl;
			//fprintf(stdout,"dump element: type=%p: ", elem);
			//elem->dump();
		}
	}
#endif
}

/*
std::string getStringFromMetadata(Value * v)
{
	if (isa<ConstantExpr>(v))
	{
		ConstantExpr * ce = cast<ConstantExpr>(v);
		
		User::op_iterator op_i = ce->op_begin();
		for (; op_i != ce->op_end(); op_i++)
		{
			Value * stringArr = *op_i;			
			
			if (isa<GlobalValue>(stringArr))
	    {
	      GlobalValue * gv = cast<GlobalValue>(stringArr);		
	      User::op_iterator op_i2 = gv->op_begin();
	      for (; op_i2 != gv->op_end(); op_i2++)
				{
					Value * stringArr2 = *op_i2;
					
					if (isa<ConstantArray>(stringArr2))
					{
						ConstantArray * ca = cast<ConstantArray>(stringArr2);
						if (ca->isString())
						{
							//std::cout<<"String name "<<ca->getAsString();
							return ca->getAsString();
						}
					}		
				}
	    }
		}
	}
	
	std::string fail("fail");
	return fail;
}


void FunctionBFC::getPathOrName(Value * v, bool isName)
{
  if (isa<ConstantExpr>(v))
	{
		ConstantExpr * op4ce = cast<ConstantExpr>(v);
		
		User::op_iterator op_i3 = op4ce->op_begin();
		for (; op_i3 != op4ce->op_end(); op_i3++)
		{
			Value * pathArr = *op_i3;			
			//std::cout<<"Op 4 of type "<<returnTypeName(pathArr->getType(), std::string(" "));//;<<std::endl; 	
			//std::cout<<" and kind ";
			//printConstantType(pathArr);
			
			if (isa<GlobalValue>(pathArr))
	    {
	      GlobalValue * gvOp2 = cast<GlobalValue>(pathArr);		
	      User::op_iterator op_i4 = gvOp2->op_begin();
	      for (; op_i4 != gvOp2->op_end(); op_i4++)
				{
					//Value * pathArr2 = op4ce->getOperand(1);
					Value * pathArr2 = *op_i4;
					
					if (isa<ConstantArray>(pathArr2))
					{
						ConstantArray * ca = cast<ConstantArray>(pathArr2);
						if (ca->isString())
						{
							if (isName)
							{
								//moduleName = ca->getAsString();
								setModuleName(ca->getAsString());
#ifdef DEBUG_LLVM	
								blame_info<<"Module Name is "<<moduleName<<std::endl;
#endif
								//std::cerr<<"Module Name is "<<moduleName<<std::endl;
							}
							else
							{
								setModulePathName(ca->getAsString());
								//modulePathName = ca->getAsString();
								//std::cout<<"Path to module is "<<modulePathName<<std::endl;
							}
							
						}
					}		
				}
	    }
		}
	}
}
*/

/*
Descriptor for local variables:
!7 = metadata !{
    i32,      ;; Tag (see below)
    metadata, ;; Context
    metadata, ;; Name
    metadata, ;; Reference to file where defined
    i32,      ;; 24 bit - Line number where defined
              ;; 8 bit - Argument number. 1 indicates 1st argument.
    metadata, ;; Type descriptor
    i32,      ;; flags
    metadata  ;; (optional) Reference to inline location
}
*/
void FunctionBFC::grabVarInformation(llvm::Value *varDeclare)
{
    if (isa<MDNode>(varDeclare)) {
        MDNode *MDVarDeclare = cast<MDNode>(varDeclare);
        if(MDVarDeclare->getNumOperands() < 7) {
#ifdef DEBUG_P
            blame_info<<"grabVarInformation failed: "<<varDeclare->getName().str()<< 
                "doesn't have complete operands"<<endl;
#endif
            return;
        }

        else {
            DIVariable *dv = new DIVariable(MDVarDeclare); 
            LocalVar *lv = new LocalVar(); // in FunctionBFC
#ifdef DEBUG_P
            blame_info<<"adding localVar "<<dv->getName().str()<<endl;
#endif
           
            lv->definedLine = dv->getLineNumber();
            lv->varName = dv->getName().str();
            localVars.push_back(lv);
        }
    }
    
    else
        cerr<<"ERROR: grabVarInformation doesn't get a MDNode"<<endl;
}

/*
void FunctionBFC::grabModuleNameAndPath(llvm::Value * compUnit)
{
	//blame_info<<"Entering grabModuleNameAndPath"<<std::endl;
  ConstantExpr * ce = cast<ConstantExpr>(compUnit);
  User::op_iterator op_i = ce->op_begin();
	
	for (; op_i != ce->op_end(); op_i++)
	{
		//blame_info<<"Op ";
		Value * bitCastOp = *op_i;
		
		if (isa<GlobalValue>(bitCastOp))
		{
			//blame_info<<"Global op "<<std::endl;
			GlobalValue * gvOp = cast<GlobalValue>(bitCastOp);		
			User::op_iterator op_i2 = gvOp->op_begin();
			for (; op_i2 != gvOp->op_end(); op_i2++)
	    {
	      Value * structOp = *op_i2;			
	      if (isa<ConstantStruct>(structOp))
				{
					//	blame_info<<"Struct Op "<<std::endl;
					ConstantStruct * csOp = cast<ConstantStruct>(structOp);
					Value * op4 = csOp->getOperand(3);
					Value * op5 = csOp->getOperand(4);
					getPathOrName(op4, true);
					getPathOrName(op5, false);
				}
	    }
		}
	}
}
*/


void FunctionBFC::printValueIDName(Value *v)
{
	if (v->getValueID() == Value::ArgumentVal)
		blame_info<<" ArgumentVal ";
	else if (v->getValueID() == Value::BasicBlockVal)
		blame_info<<" BasicBlockVal ";
	else if (v->getValueID() == Value::FunctionVal)
		blame_info<<" FunctionVal ";
	else if (v->getValueID() == Value::GlobalAliasVal)
		blame_info<<" GlobalAliasVal ";
	else if (v->getValueID() == Value::GlobalVariableVal)
		blame_info<<" GlobalVariableVal ";
	else if (v->getValueID() == Value::UndefValueVal)
		blame_info<<" UndefValueVal ";
	else if (v->getValueID() == Value::ConstantExprVal)
		blame_info<<" ConstantExprVal ";
	else if (v->getValueID() == Value::ConstantAggregateZeroVal)
		blame_info<<" ConstantAggregateZeroVal ";
	else if (v->getValueID() == Value::ConstantIntVal)
		blame_info<<" ConstantIntVal ";
	else if (v->getValueID() == Value::ConstantFPVal)
		blame_info<<" ConstantFPVal ";
	else if (v->getValueID() == Value::ConstantArrayVal)
		blame_info<<" ConstantArrayVal ";
	else if (v->getValueID() == Value::ConstantStructVal)
		blame_info<<" ConstantStructVal ";
	else if (v->getValueID() == Value::ConstantVectorVal)
		blame_info<<" ConstantVectorVal ";
	else if (v->getValueID() == Value::ConstantPointerNullVal)
		blame_info<<" ConstantPointerNullVal ";
	else if (v->getValueID() == Value::InlineAsmVal)
		blame_info<<" InlineAsmVal ";
	else if (v->getValueID() == Value:: PseudoSourceValueVal)
		blame_info<<" PseudoSourceValueVal ";
	else if (v->getValueID() >= Value::InstructionVal)
	{
		blame_info<<" InstructionVal for Instruction ";
		if (isa<Instruction>(v)) {
			Instruction * pi = cast<Instruction>(v);
			blame_info<<pi->getOpcodeName()<<" ";
		}
	}
	else
		blame_info<<"UNKNOWNVal";
	
}

const char *FunctionBFC::calcMetaFuncName(RegHashProps &variables, Value *v, bool isTradName, std::string nonTradName, int currentLineNum)
{
	
	std::string newName;
	char tempBuf[1024];
	
	if (isTradName) //isTradName = is traditional name
		sprintf (tempBuf, "%s--%i", v->getName().data(), currentLineNum);	
	else
		sprintf (tempBuf, "%s--%i", nonTradName.c_str(), currentLineNum);	
	
	newName.insert(0, tempBuf);
	
	while (variables.count(newName.c_str()))
		newName.push_back('a'); //same as append, but with only one char
	
	//std::cout<<"Returning new name of "<<newName<<std::endl;
	return newName.c_str();
}


void FunctionBFC::printCurrentVariables()
{
#ifdef DEBUG_LLVM_L2
	RegHashProps::iterator begin, end;
  /* Iterate through variables and label them with their node IDs (integers)
	 -> first is const char * associated with their name
	 -> second is NodeProps * for node
	 */
  for (begin = variables.begin(), end = variables.end(); begin != end; begin++)
	{
		std::string name(begin->first);
		NodeProps * v = begin->second;
		blame_info<<"Node "<<v->number<<" ("<<v->name<<")"<<std::endl;
	}
#endif 
}



bool FunctionBFC::firstGEPCheck(User * pi)
{
	
	if (isa<ConstantExpr>(pi))
	{
		ConstantExpr * ce = cast<ConstantExpr>(pi);
		
		if (ce->getOpcode() != Instruction::GetElementPtr)
			return true;
	}
	else if (isa<Instruction>(pi))
	{
		Instruction * i = cast<Instruction>(pi);
		if (i->getOpcode() != Instruction::GetElementPtr)
			return true;
	}
	else
	{
#ifdef DEBUG_ERROR
		std::cerr<<"What are we?!?"<<std::endl;
#endif 
		exit(0);
	}
	
	
	Value * v = pi->getOperand(0);
	
	
	//const llvm::Type * origT;		
	//std::string origTStr = returnTypeName(origT, std::string(" "));
	//origT = v->getType();	
	//int origPointerLevel = pointerLevel(origT,0);
	
	const llvm::Type * pointT = v->getType();
	unsigned typeVal = pointT->getTypeID();
	
	while (typeVal == Type::PointerTyID)
	{			
		pointT = cast<PointerType>(pointT)->getElementType();		
		typeVal = pointT->getTypeID();
	}
	
	if (typeVal == Type::StructTyID)
	{
		const llvm::StructType * type = cast<StructType>(pointT);
		string structNameFull = type->getName().str();
		
#ifdef DEBUG_LLVM
		blame_info<<"first GEP check -- structNameFull -- "<<structNameFull<<std::endl;
#endif
		
		if (structNameFull.find("struct.descriptor_dimension") != std::string::npos)
			return false;
		
		if (structNameFull.find("struct.array1") != std::string::npos)
		{
#ifdef DEBUG_LLVM
		    blame_info<<"I'm in Hui 1 for "<<structNameFull<<std::endl;
#endif
		    if (pi->getNumOperands() >= 3)
		    {
#ifdef DEBUG_LLVM
		        blame_info<<"I'm in Hui 2 for "<<structNameFull<<std::endl;
#endif
			    Value * vOp = pi->getOperand(2);
				if (vOp->getValueID() == Value::ConstantIntVal)
				{
					ConstantInt * cv = (ConstantInt *)vOp;
					int number = cv->getSExtValue();
#ifdef DEBUG_LLVM
		            blame_info<<"I'm in Hui 3, cv= "<<number<<std::endl;
#endif
				    if (number != 0)
						return false;
				}
			}
		}	
	}
	return true;
}


void FunctionBFC::genDILocationInfo(Instruction *pi, int &currentLineNum, FunctionBFCBB *fbb)
{
   if (MDNode *N = pi->getMetadata("dbg")) {
        DILocation Loc(N);
        unsigned Line = Loc.getLineNumber();
        string File = Loc.getFilename().str();
        string Dir = Loc.getDirectory().str();

        currentLineNum = Line;
        //02/02/16: we really shouldn't zero lineNumOrder before every inst
        if(lnm.find(currentLineNum) == lnm.end())
            lnm[currentLineNum] = 0;
        
        allLineNums.insert(currentLineNum);
        //In FunctionBFCBB
        fbb->lineNumbers.insert(currentLineNum);

        if (currentLineNum < startLineNum) { //startLineNum: start line of a function
            // The shortest lines should be in entry, in FORTRAN we have cases where
            // there are branches ot pseudo blocks where it includes the definition line
            // of the function which messes up the analysis
            if (fbb->getName().find("entry") != std::string::npos)
                startLineNum = currentLineNum;
        }

        if (endLineNum < currentLineNum)
            endLineNum = currentLineNum;

        if (!moduleSet) {
            moduleName = File;
            modulePathName = Dir;
            moduleSet = true;
        }
    }
}

bool FunctionBFC::parseDeclareIntrinsic(Instruction *pi, int &currentLineNum, FunctionBFCBB *fbb)
{
	//User::op_iterator op_i = pi->op_begin() /*, op_e = pi->op_end() */;
	//Value *called_funcName = *op_i;
	//op_i++;
    //Value *calledFuncName = pi->getOperand(0);
    CallInst *ci = dyn_cast<CallInst>(pi);
	if (ci == NULL) {
#ifdef DEBUG_LLVM
        blame_info<<"LLVM__(parseDeclareIntrinsic) "<<pi->getName().str()<<" is not a CallInst"<<std::endl;
#endif
        return false;
    }
    
    Function *calledFunc = ci->getCalledFunction();	
    if(calledFunc != NULL && calledFunc->hasName()){
	  if(calledFunc->getName().str().find("llvm.dbg.declare") != std::string::npos) {
		//Value * varDeclare = pi->getOperand(2); //TO CHECK: if it doesn't work, then need calledFunc::arg_iterator
        //Function::arg_iterator arg_i = calledFunc->arg_begin();
        //arg_i++;
        //Value *varDeclare = calledFunc->getOperand(1);
        blame_info<<"parseDeclareIntrinsic called!"<<std::endl;
		Value *varDeclare = ci->getArgOperand(1);//Only this work, aboves NOT
        grabVarInformation(varDeclare);
	    return true;
      }
	}

    return false;
}


void FunctionBFC::ieInvoke(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb) 
{
#ifdef DEBUG_LLVM_L2
	blame_info<<"LLVM__(examineInstruction)(Invoke) -- pi "<<pi->getName().str()<<" "<<pi<<" "<<pi->getOpcodeName()<<std::endl;
#endif 
	//invokeCount++;
	//cout<<"EI Invoke Begin\n";
	// Add LHS variable to list of symbols
	/*
	 if (pi->hasName())
	 cout<<"Name is "<<pi->getNameStart()<<std::endl;
	 else
	 cout<<"No name\n";
	 */
	
	if (pi->hasName() && variables.count(pi->getName().data()) == 0) {
		std::string name = pi->getName().str();
		
#ifdef DEBUG_VP_CREATE
		blame_info<<"Adding NodeProps(10) for "<<name<<std::endl;
#endif
		NodeProps * vp = new NodeProps(varCount,name,currentLineNum,pi); //TO CHECK, what's varCount for here ?
		vp->fbb = fbb;
		
		if (currentLineNum != 0) {  //TO CHECK, what's this part for?
			int lnm_cln = lnm[currentLineNum];
			vp->lineNumOrder = lnm_cln; //there're multiple statements/insts in assembly matches to same code line
			lnm_cln++;
			lnm[currentLineNum] = lnm_cln;
		}
		
		variables[pi->getName().data()] = vp;
		varCount++;				
		////printCurrentVariables();
	}	
	
	// Add operands to list of symbols as long as it's not a basicblock
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		
		if (v->hasName() &&  v->getValueID() != Value::BasicBlockVal) {			
			if (variables.count(v->getName().data()) == 0) {
				std::string name = v->getName().str();
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(11) for "<<name<<std::endl;
#endif 
				NodeProps * vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}
				
				variables[v->getName().data()] = vp;					
				varCount++;
				//printCurrentVariables();
			}
		}
	}
	//cout<<"EI Invoke End\n";
}


void FunctionBFC::ieCall(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	// pi->getName = return value
	// op[#ops-1] = name of call (the last operand of pi)
	// op[0,1,...] = parameters
#ifdef DEBUG_LLVM
	blame_info<<"LLVM__(examineInstruction)(Call) -- pi "<<pi->getName().str()<<" "<<pi<<" "<<pi->getOpcodeName()<<std::endl;
#endif 
	// Add the Node Props for the return value
	if (pi->hasName() && variables.count(pi->getName().data()) == 0) {
		std::string name = pi->getName().str();
#ifdef DEBUG_VP_CREATE
		blame_info<<"Adding NodeProps(12) for "<<name<<std::endl;
#endif 
		NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
		vp->fbb = fbb;
		
		if (currentLineNum != 0) {
			int lnm_cln = lnm[currentLineNum];
			vp->lineNumOrder = lnm_cln;
			lnm_cln++;
			lnm[currentLineNum] = lnm_cln;
		}
		
		variables[pi->getName().data()] = vp;
		varCount++;				
		
	}
	
	int opNum = 0;
    int callNameIdx = pi->getNumOperands()-1; //called func is the last operand of this callInst
	bool isTradName = true; //is traditional name
	std::string nonTradName; //represents embedded func names 
	
    /////added by Hui,trying to get the called function///////////
    llvm::CallInst *cpi = cast<CallInst>(pi);
    llvm::Function *calledFunc = cpi->getCalledFunction();
    if(calledFunc != NULL && calledFunc->hasName())
        blame_info<<"In ieCall, calledFunc's name = "<<calledFunc->getName().data();
    blame_info<<"  pi->getNumOperands()="<<pi->getNumOperands()<<std::endl;
    //////////////////////////////////////////////////////////
    //added by Hui 12/31/15: get the callName from the last operand first
    Value *lastOp = pi->getOperand(callNameIdx);
    if(lastOp->hasName()){
        funcCallNames.insert(lastOp->getName().data());
        blame_info<<"Called function has a name: "<<lastOp->getName().data()<<std::endl;
    }
    else { //if no name, then it's an embedded func
        if (lastOp->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_LLVM
            blame_info<<"Called function is ConstantExpr"<<std::endl;
#endif
            if (isa<ConstantExpr>(lastOp)) {
                ConstantExpr *ce = cast<ConstantExpr>(lastOp);
                User::op_iterator op_i = ce->op_begin();
                for (; op_i != ce->op_end(); op_i++) {
                    Value *funcVal = *op_i;			
                    if (isa<Function>(funcVal)) {
                        Function * embFunc = cast<Function>(funcVal);
#ifdef DEBUG_LLVM
                        blame_info<<"EMB Func "<<embFunc->getName().str()<<std::endl;
#endif 
                        isTradName = false;
                        nonTradName = embFunc->getName().str();
                        funcCallNames.insert(embFunc->getName().data());
                    }
                }
            }
        }
    }
    
    const char *vName = NULL;
    const char *tempName = calcMetaFuncName(variables, lastOp, isTradName, nonTradName, currentLineNum);
    char tempBuf[1024];
    sprintf(tempBuf, "%s", tempName);
    
    char * vN = (char *)malloc(sizeof(char)*(strlen(tempBuf)+1));
    if (!vN) {
        printf("Uh oh, malloc vN failed\n");
        exit(0);
    }
    strcpy(vN,tempBuf);
    vN[strlen(tempBuf)]='\0';
    vName = vN;
    std::string callName(vName); //name for each callnode: bar--51a, bar--51aa..
    blame_info<<"After calcMetaFuncName, callName="<<callName<<std::endl;
  
    // Assigning function name VP and VPs for all the parameters
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;

		if (!(v->hasName())) {
#ifdef DEBUG_LLVM
			blame_info<<"In ieCall -- Call Operand No Name "<<v<<" ";
			printValueIDName(v);
			blame_info<<std::endl;
#endif			
        }
        else { //v has a name
#ifdef DEBUG_LLVM
			blame_info<<"In ieCall -- Call Operand "<<opNum<<" "<<v->getName().str()<<std::endl;
			blame_info<<"In ieCall -- Type "<<v->getValueID()<<std::endl;
#endif
		}
		if (isa<ConstantExpr>(v)) {// The parameter is a BitCast or GEP operation pointing to something else
			ConstantExpr *ce = cast<ConstantExpr>(v);
			
			if (ce->getOpcode() == Instruction::GetElementPtr) {
				// Overwriting 
				v = ce->getOperand(0);//get the real one 
#ifdef DEBUG_LLVM
				blame_info<<"Overwriting(1) Call Param GEP for "<<v->getName().str()<<std::endl;
#endif 
            }	
			else if (ce->getOpcode() == Instruction::BitCast) {
				v = ce->getOperand(0);//get the real one
#ifdef DEBUG_LLVM
				blame_info<<"Overwriting(2) Call Param Bitcast for "<<v->getName().str()<<std::endl;
#endif 
			}
		}
		
		// We add the VP for the actual call
		if (variables.count(vName) == 0 && opNum == callNameIdx) { 
#ifdef DEBUG_VP_CREATE
			blame_info<<"Adding NodeProps(13) for "<<callName<<std::endl;
#endif
			NodeProps *vp = new NodeProps(varCount,callName,currentLineNum,pi);
			vp->fbb = fbb;
			
			if (currentLineNum != 0) {
				int lnm_cln = lnm[currentLineNum];
				vp->lineNumOrder = lnm_cln;
				lnm_cln++;
				lnm[currentLineNum] = lnm_cln;
			}
			
			variables[vName] = vp;
			varCount++;
			//printCurrentVariables();
			
			// OpNum -1 signifies an entry for the name of the function call
			FuncCall *fp = new FuncCall(-2, callName); //CHANGEd: -1 =>-2
			fp->lineNum = currentLineNum;
			vp->addFuncCall(fp);
			addFuncCalls(fp);
			vp->nStatus[CALL_NODE] = true; //CALL_NODE = 16, NODE_PROPS_SIZE = 20
			
//We need to assign a funcCall object to the return and treat it as "parameter"-1
			if (pi->hasName()) {
				if (variables.count(pi->getName().data())) {
					NodeProps *retVP = variables[pi->getName().data()];
					
					if (retVP) {
					// Adding a funcCall object for the return value ("parameter" 0)
						FuncCall *fp = new FuncCall(-1, callName);//CHANGEd 0=>-1
						fp->lineNum = currentLineNum;
						retVP->addFuncCall(fp);
						addFuncCalls(fp);
						retVP->nStatus[CALL_RETURN] = true; //CALL_RETURN = 18
					}
				}
			}
			// TODO: Put this check in everywhere
			// FORTRAN  TC: why here use addr (pi) as name ??
			else if (pi->hasNUsesOrMore(1)) { //the return is used >=1 places
#ifdef DEBUG_LLVM
				blame_info<<"Call "<<callName<<" has at least one use.  Return value valid."<<std::endl;
#endif 					
				char tempBuf2[18];
				sprintf(tempBuf2, "0x%x", /*(unsigned)*/pi);
				std::string name(tempBuf2);
				
				NodeProps *retVP;	
				if (variables.count(name.c_str()) == 0) {
#ifdef DEBUG_VP_CREATE
					blame_info<<"Adding NodeProps(F5) for "<<name<<std::endl;
#endif
					retVP = new NodeProps(varCount,name,currentLineNum,pi);
					vp->fbb = fbb;
					
					if (currentLineNum != 0) {
						int lnm_cln = lnm[currentLineNum];
						retVP->lineNumOrder = lnm_cln;
						lnm_cln++;
						lnm[currentLineNum] = lnm_cln;
					}
					
					variables[name.c_str()] = retVP;
					varCount++;
					
					FuncCall *fp = new FuncCall(-1, callName); //CHANGEd 0=>-1
					fp->lineNum = currentLineNum;
					retVP->addFuncCall(fp);
					addFuncCalls(fp);
					retVP->nStatus[CALL_RETURN] = true; 
					
				}
			}

			else {
#ifdef DEBUG_LLVM
				blame_info<<"Call "<<callName<<" has at NO users.  Return value not valid."<<std::endl;
#endif 
			}
		}
		else if (opNum != callNameIdx) {// We take care of the parameters
			// opNum is the parameter number to call, callName is as advertised
			FuncCall *fp = new FuncCall(opNum, callName);
			fp->lineNum = currentLineNum;
#ifdef DEBUG_LLVM
			blame_info<<"Adding func call in "<<getSourceFuncName()<<" to "<<callName<<" p "<<opNum<<" for node ";
			blame_info<<v->getName().str()<<std::hex<<v<<std::dec<<std::endl;
#endif
			if (v->hasName()) {
				NodeProps *vp;
				
				if (variables.count(v->getName().data())) {
					// Param might already exist ... in fact, it's likely since they are also local vars
					vp = variables[v->getName().data()];
					// If param doesn't exist, we add it to list of variables in the program
					if (!vp) {				
#ifdef DEBUG_VP_CREATE
						blame_info<<"Adding NodeProps(14) for "<<v->getName().str()<<std::endl;
#endif						
						vp = new NodeProps(varCount, v->getName().str(), currentLineNum, pi);
						vp->fbb = fbb;
						
						if (currentLineNum != 0) {
							int lnm_cln = lnm[currentLineNum];
							vp->lineNumOrder = lnm_cln;
							lnm_cln++;
							lnm[currentLineNum] = lnm_cln;
						}
						variables[v->getName().data()] = vp;
						varCount++;						
					}
				}

				else { //Param didn't exist
#ifdef DEBUG_VP_CREATE
					blame_info<<"Adding NodeProps(14a) for "<<v->getName().str()<<std::endl;
#endif
					vp = new NodeProps(varCount, v->getName().str(), currentLineNum, pi);
					vp->fbb = fbb;
					
					if (currentLineNum != 0) {
						int lnm_cln = lnm[currentLineNum];
						vp->lineNumOrder = lnm_cln;
						lnm_cln++;
						lnm[currentLineNum] = lnm_cln;
					}
					variables[v->getName().data()] = vp;
					varCount++;
					//printCurrentVariables();
				}
				// We have a funcCall instantiation for every parameter
				vp->addFuncCall(fp);
				addFuncCalls(fp);
				vp->nStatus[CALL_PARAM] = true;
			}
            
			else { 
              unsigned valueID = v->getValueID(); //added by Hui 03/15/16
              if (valueID != Value::ConstantIntVal && valueID != Value::ConstantFPVal \
                     && valueID != Value::UndefValueVal && valueID != Value::ConstantPointerNullVal) {//v has no name and not a constant value 03/15/16
				char tempBuf2[18];
				sprintf(tempBuf2, "0x%x", /*(unsigned)*/v);
				std::string name(tempBuf2);
				
				NodeProps *vp;
				if (variables.count(name.c_str()) == 0){
#ifdef DEBUG_VP_CREATE
					blame_info<<"Adding NodeProps(F2) for "<<name<<std::endl;
#endif
					vp = new NodeProps(varCount,name,currentLineNum,pi);
					vp->fbb = fbb;	
					if (currentLineNum != 0) {
						int lnm_cln = lnm[currentLineNum];
						vp->lineNumOrder = lnm_cln;
						lnm_cln++;
						lnm[currentLineNum] = lnm_cln;
					}
					
					variables[name.c_str()] = vp;
					varCount++;
					//printCurrentVariables();
				}
				else {
					vp = variables[name.c_str()];
				}
				
				vp->addFuncCall(fp);
				addFuncCalls(fp);
				vp->nStatus[CALL_PARAM] = true;
			  }
            }
		}

		opNum++;
	}
}


void FunctionBFC::ieGen_LHS_Alloca(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	std::string name;
	// Add LHS variable to list of symbols
	if (pi->hasName())
		name = pi->getName().str();
	else {
		char tempBuf[18];
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		name.insert(0,tempBuf);
	}
	
#ifdef DEBUG_VP_CREATE
	blame_info<<"Adding NodeProps(A1) for "<<name<<std::endl;
#endif
	NodeProps * vp = new NodeProps(varCount,name,currentLineNum,pi);
	vp->fbb = fbb;
	
	if (currentLineNum != 0) {
		int lnm_cln = lnm[currentLineNum];
		vp->lineNumOrder = lnm_cln;
		lnm_cln++;
		lnm[currentLineNum] = lnm_cln;
	}
	
	if (variables.count(name.c_str()) == 0) {
		variables[name.c_str()] = vp;
		varCount++;
	}
	
	//if (name.find(".") != std::string::npos || name.find("0x") != std::string::npos)  
    //"." refers to names as: *.addr in llvm 3.3, not sure what it represents in 2.5 !
	if (name.find(".")!=std::string::npos || name.find("0x")!=std::string::npos) {
        LocalVar *lv = new LocalVar();
		lv->definedLine = currentLineNum;
		lv->varName = name;
		
		localVars.push_back(lv);
	}
}

void FunctionBFC::ieGen_LHS(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb) 
{
	// Add LHS variable to list of symbols
	if (pi->hasName() && variables.count(pi->getName().data()) == 0) {
		std::string name = pi->getName().str();
		
#ifdef DEBUG_VP_CREATE
		blame_info<<"Adding NodeProps(1) for "<<name<<" currentLineNum="<<currentLineNum<<" lnm="<<lnm[currentLineNum]<<std::endl;
        if(isa<ConstantExpr>(pi)){
 		    char tempHui[24];
		    sprintf(tempHui, "0x%x", /*(unsigned)*/pi);
		    std::string Hui(tempHui);
            blame_info<<"Hui value of this inst="<<Hui<<std::endl;
        }
#endif 
		NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
		vp->fbb = fbb;
		if (currentLineNum != 0) {
			int lnm_cln = lnm[currentLineNum];
			vp->lineNumOrder = lnm_cln;
			lnm_cln++;
			lnm[currentLineNum] = lnm_cln;
		}
		
		variables[pi->getName().data()] = vp;
		varCount++;				
		//printCurrentVariables();
	}
	else { //if the instruction has NO name or it's already in "variables"
		char tempBuf[18];
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		std::string name(tempBuf); // Use the address of the instruction as its name ??
		
		NodeProps *vp = NULL;
		
		if (isa<ConstantExpr>(pi)) {
			name += ".CE";
			
			char tempBuf2[10];
			sprintf(tempBuf2, ".%d", currentLineNum);
			name.append(tempBuf2);
#ifdef DEBUG_VP_CREATE
			blame_info<<"Adding NodeProps(F1CE) for "<<name<<" currentLineNum="<<currentLineNum<<" lnm="<<lnm[currentLineNum]<<std::endl;
#endif
			vp = new NodeProps(varCount,name,currentLineNum,pi);
		}
		else {
#ifdef DEBUG_VP_CREATE
			blame_info<<"Adding NodeProps(F1) for "<<name<<" currentLineNum="<<currentLineNum<<" lnm="<<lnm[currentLineNum]<<std::endl;
#endif
			vp = new NodeProps(varCount,name,currentLineNum,pi);
		}
		
		vp->fbb = fbb;
		
		if (currentLineNum != 0) {
			int lnm_cln = lnm[currentLineNum];
			vp->lineNumOrder = lnm_cln;
            //for temp test
            blame_info<<"vp->lineNumOrder="<<lnm[currentLineNum]<<endl;
			lnm_cln++;
			lnm[currentLineNum] = lnm_cln;
            //for temp test
            blame_info<<"now lnm[currentLineNum]="<<lnm[currentLineNum]<<endl;
		}

		if (variables.count(name.c_str()) == 0) {
			variables[name.c_str()] = vp;
			varCount++;
#ifdef DEBUG_VP_CREATE
		blame_info<<"New added variable:  "<<name<<std::endl;
#endif 
	    }
		//printCurrentVariables();
	}
}


void FunctionBFC::ieGen_Operands(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	int opNum = 0;
	// Add operands to list of symbols
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		
#ifdef DEBUG_LLVM
		if (!(v->hasName())) {
			blame_info<<"Standard Operand No Name "<<" "<<v<<" ";
			printValueIDName(v);
			blame_info<<std::endl;
		}
#endif			
		
		if (v->hasName() && v->getValueID() != Value::BasicBlockVal) {
			if (variables.count(v->getName().data()) == 0) {
				std::string name = v->getName().str();
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(2) for "<<name<<std::endl;
#endif 
				NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}

				variables[v->getName().data()] = vp;					
				varCount++;
				//printCurrentVariables();
			}
		}
		else if(v->getValueID() == Value::ConstantExprVal) { //v has no name but is a constant expression
#ifdef DEBUG_LLVM
			blame_info<<"ValueID is "<<v->getValueID()<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
#ifdef DEBUG_LLVM
				blame_info<<"Value is ConstantExpr"<<std::endl;
#endif 
				ConstantExpr *ce = cast<ConstantExpr>(v);	
				createNPFromConstantExpr(ce, varCount, currentLineNum, fbb); 
			}
		}
        /*else { //added by Hui 03/22/16, we need to create nodes for constants
            //added by Hui : deal with constant operands
            string opName;
	        if (v->getValueID() == Value::ConstantIntVal) {
		        ConstantInt *cv = (ConstantInt *)v;
		        int number = cv->getSExtValue();
		
		        char tempBuf[64];
		        sprintf(tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, 0, pi->getOpcode());		
		        char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
	
		        strcpy(vN,tempBuf);
		        vN[strlen(tempBuf)]='\0';
		        const char *vName = vN;
		
		        opName.insert(0, vName);
	        } 
	        else if (v->getValueID() == Value::ConstantFPVal) {
		        char tempBuf[70];
		        ConstantFP *cfp = (ConstantFP *)v;
		        const APFloat apf = cfp->getValueAPF();
	
		        if(APFloat::semanticsPrecision(apf.getSemantics()) == 24) {
			      float floatNum = apf.convertToFloat();
			      sprintf (tempBuf, "Constant+%g+%i+%i+%i", floatNum, currentLineNum, 0, pi->getOpcode());		
		        }
		        else if(APFloat::semanticsPrecision(apf.getSemantics()) == 53) {
			      double floatNum = apf.convertToDouble();
			      sprintf (tempBuf, "Constant+%g2.2+%i+%i+%i", floatNum, currentLineNum, 0, pi->getOpcode());		
		        }
		        else {
#ifdef DEBUG_ERROR
			      std::cerr<<"Not a float or a double FPVal"<<std::endl;
			      blame_info<<"Not a float or a double FPVal"<<std::endl;
#endif 
		        }
		
		        char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
		        strcpy(vN,tempBuf);
		        vN[strlen(tempBuf)]='\0';
		        const char * vName = vN;
		
		        opName.insert(0, vName);
	        }
#ifdef DEBUG_VP_CREATE
			blame_info<<"Adding NodeProps(hui2) for "<<opName<<std::endl;
#endif 
			NodeProps *vp = new NodeProps(varCount,opName,currentLineNum,pi);
			vp->fbb = fbb;
			  	
			if (currentLineNum != 0) {
			    int lnm_cln = lnm[currentLineNum];
			    vp->lineNumOrder = lnm_cln;
				lnm_cln++;
				lnm[currentLineNum] = lnm_cln;
			}

			variables[v->getName().data()] = vp;					
			varCount++;
        }*/
		opNum++;
	}
}


void FunctionBFC::ieGen_OperandsStore(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	int opNum = 0;
	// Add operands to list of symbols
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
#ifdef DEBUG_LLVM
		if (!(v->hasName())) {
			blame_info<<"Standard Operand No Name "<<" "<<v<<" ";
			printValueIDName(v);
			blame_info<<std::endl;
		}
#endif			
		if (v->hasName() && v->getValueID() != Value::BasicBlockVal) {
            bool lnmChanged = false;
            NodeProps *vp = NULL;
			if (variables.count(v->getName().data()) == 0) { //Usually the operands in store are pre-declared
				std::string name = v->getName().str();              //so it should've been in variables already
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(2) for "<<name<<std::endl;
#endif 
				vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
                  int lnm_cln = lnm[currentLineNum];
				  vp->lineNumOrder = lnm_cln;
				  lnm_cln++;
				  lnm[currentLineNum] = lnm_cln;
				  lnmChanged = true;
                }
				variables[v->getName().data()] = vp;					
				varCount++;
			}
            else 
                vp = variables[v->getName().data()];

            //we create a FuncStore only when we met the content  
            if(opNum==0) {
                User::op_iterator op_i2 = pi->op_begin();
                Value  /**firstStr = *op_i2, */  *secondStr = *(++op_i2);
#ifdef DEBUG_LLVM
                blame_info<<"STORE to(1) "<<secondStr->getName().str()<<" from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
#endif 
                FuncStores *fs = new FuncStores();
                
                if (secondStr->hasName() && variables.count(secondStr->getName().data()))
                    fs->receiver = variables[secondStr->getName().data()];
                else
                    fs->receiver = NULL;
                fs->contents = vp;
                fs->line_num = currentLineNum;
                if(lnmChanged)
                    fs->lineNumOrder = lnm[currentLineNum]-1; //since we've increment lnm_cln before, so we need to one step back
                else
                    fs->lineNumOrder = lnm[currentLineNum];
                blame_info<<"STORE to(1) fs->lineNumOrder="<<fs->lineNumOrder<<" in line# "<<currentLineNum<<endl;

                allStores.push_back(fs);
            }	
		}
		// This is for dealing with Constants 
		else if (v->getValueID() == Value::ConstantIntVal) {
			ConstantInt *cv = (ConstantInt *)v;
			int number = cv->getSExtValue();
			
			char tempBuf[64];
			sprintf (tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, opNum, Instruction::Store);		
			char * vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
			
			strcpy(vN,tempBuf);
			vN[strlen(tempBuf)]='\0';
			const char * vName = vN;
            bool lnmChanged = false;
            NodeProps *vp = NULL;

            if (variables.count(vName) == 0) {
				std::string name(vName);
				//std::cout<<"Creating VP for Constant "<<vName<<" in "<<getSourceFuncName()<<std::endl;
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(3) for "<<name<<std::endl;
#endif 
				vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
							
				if (currentLineNum != 0) {
                  int lnm_cln = lnm[currentLineNum];
				  vp->lineNumOrder = lnm_cln;
				  lnm_cln++;
				  lnm[currentLineNum] = lnm_cln;
				  lnmChanged = true;
                }
				
				variables[vName] = vp;
				varCount++;
			}
            else
                vp = variables[vName];

            //we create a FuncStore only when we met the content  
            if(opNum==0) {
                User::op_iterator op_i2 = pi->op_begin();
                Value  /**firstStr = *op_i2, */  *secondStr = *(++op_i2);
#ifdef DEBUG_LLVM
                blame_info<<"STORE to(2) "<<secondStr->getName().str()<<" from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
#endif 
                FuncStores *fs = new FuncStores();
                
                if (secondStr->hasName() && variables.count(secondStr->getName().data()))
                    fs->receiver = variables[secondStr->getName().data()];
                else
                    fs->receiver = NULL;
                fs->contents = vp;
                fs->line_num = currentLineNum;
                if(lnmChanged)
                    fs->lineNumOrder = lnm[currentLineNum]-1;//same reason as before, we need to one step back
                else
                    fs->lineNumOrder = lnm[currentLineNum];

                blame_info<<"STORE to(2) fs->lineNumOrder="<<fs->lineNumOrder<<" in line# "<<currentLineNum<<endl;
                allStores.push_back(fs);
            }
		}
		else if (v->getValueID() == Value::ConstantFPVal) {
			ConstantFP *cfp = (ConstantFP *)v;
			const APFloat apf = cfp->getValueAPF(); //llvm::APFloat
		
			if(APFloat::semanticsPrecision(apf.getSemantics()) == 24) { //TC: why 24 ?
				float floatNum = apf.convertToFloat();
#ifdef DEBUG_LLVM
				blame_info<<"Converted to float! "<<floatNum<<std::endl;
#endif 
				char tempBuf[64];
				sprintf (tempBuf, "Constant+%g+%i+%i+%i", floatNum, currentLineNum, opNum, Instruction::Store);		
				char *vN = (char *)malloc(sizeof(char)*(strlen(tempBuf)+1));
				
				strcpy(vN,tempBuf);
				vN[strlen(tempBuf)]='\0';
				const char * vName = vN;
                bool lnmChanged = false;
				NodeProps *vp = NULL;

				if (variables.count(vName) == 0) {
					std::string name(vName);
					//std::cout<<"Creating VP for Constant "<<vName<<" in "<<getSourceFuncName()<<std::endl;
#ifdef DEBUG_VP_CREATE
					blame_info<<"Adding NodeProps(5) for "<<name<<std::endl;
#endif 
					vp = new NodeProps(varCount,name,currentLineNum,pi);
					vp->fbb = fbb;
					
					if (currentLineNum != 0) {
					    int lnm_cln = lnm[currentLineNum];
					    vp->lineNumOrder = lnm_cln;
					    lnm_cln++;
					    lnm[currentLineNum] = lnm_cln;
					    lnmChanged = true;
                    }
					
					variables[vName] = vp;
					varCount++;
				}
                else
                    vp = variables[vName];

                //we create a FuncStore only when we met the content  
                if(opNum==0) {
                    User::op_iterator op_i2 = pi->op_begin();
                    Value  /**firstStr = *op_i2, */  *secondStr = *(++op_i2);
#ifdef DEBUG_LLVM
                    blame_info<<"STORE to(3) "<<secondStr->getName().str()<<" from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
#endif 
                    FuncStores *fs = new FuncStores();
                
                    if (secondStr->hasName() && variables.count(secondStr->getName().data()))
                        fs->receiver = variables[secondStr->getName().data()];
                    else
                        fs->receiver = NULL;
                    fs->contents = vp;
                    fs->line_num = currentLineNum;
                    if(lnmChanged)
                        fs->lineNumOrder = lnm[currentLineNum]-1;
                    else
                        fs->lineNumOrder = lnm[currentLineNum];

                    blame_info<<"STORE to(3) fs->lineNumOrder="<<fs->lineNumOrder<<" in line# "<<currentLineNum<<endl;
                    allStores.push_back(fs);
                }
			}
			else if(APFloat::semanticsPrecision(apf.getSemantics()) == 53) {
				double floatNum = apf.convertToDouble();
				char tempBuf[70];
				sprintf (tempBuf, "Constant+%g2.2+%i+%i+%i", floatNum, currentLineNum, opNum, Instruction::Store);
#ifdef DEBUG_LLVM
				blame_info<<"Converted to double! "<<tempBuf<<std::endl;
#endif 
				
				char * vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
				
				strcpy(vN,tempBuf);
				vN[strlen(tempBuf)]='\0';
				const char * vName = vN;
                bool lnmChanged = false;
                NodeProps *vp = NULL;
				
				if (variables.count(vName) == 0) {
					std::string name(vName);
					//std::cout<<"Creating VP for Constant "<<vName<<" in "<<getSourceFuncName()<<std::endl;
#ifdef DEBUG_VP_CREATE
					blame_info<<"Creating VP for Constant "<<vName<<" in "<<getSourceFuncName()<<std::endl;
					blame_info<<"Adding NodeProps(6) for "<<name<<std::endl;
#endif 
					
					NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
					vp->fbb = fbb;
					
					if (currentLineNum != 0) {
					  int lnm_cln = lnm[currentLineNum];
					  vp->lineNumOrder = lnm_cln;
					  lnm_cln++;
					  lnm[currentLineNum] = lnm_cln;
                      lnmChanged = true;
					}
					
					variables[vName] = vp;
					varCount++;
				}
                else 
                    vp = variables[vName];
    
                //we create a FuncStore only when we met the content  
                if(opNum==0) {
                    User::op_iterator op_i2 = pi->op_begin();
                    Value  /**firstStr = *op_i2, */  *secondStr = *(++op_i2);
#ifdef DEBUG_LLVM
/*                    if (secondStr->hasName())
                        blame_info<<"STORE to(4) "<<secondStr->getName().str()<<" from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
                    else 
                        blame_info<<"STORE to(4b) no-name from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
*/
#endif 
                    FuncStores *fs = new FuncStores();
                
                    if (secondStr->hasName() && variables.count(secondStr->getName().data()))
                        fs->receiver = variables[secondStr->getName().data()];
                    else
                        fs->receiver = NULL;
                    fs->contents = vp;
                    fs->line_num = currentLineNum;
                    if(lnmChanged)
                        fs->lineNumOrder = lnm[currentLineNum]-1;
                    else
                        fs->lineNumOrder = lnm[currentLineNum];

                    blame_info<<"STORE to(4) fs->lineNumOrder="<<fs->lineNumOrder<<" in line# "<<currentLineNum<<endl;
                    allStores.push_back(fs);
                }
			}
		}
        //added cases when the "content" is a register
        else if(!v->hasName() && v->getValueID() != Value::ConstantFPVal && 
                v->getValueID() != Value::ConstantIntVal && v->getValueID() != Value::ConstantPointerNullVal) {
        //else if(!v->hasName() && !isa<Constant>(v)) { 
 			char tempBuf2[18];
			sprintf(tempBuf2, "0x%x", /*(unsigned)*/v);
			std::string name(tempBuf2);
			bool lnmChanged = false;
            NodeProps *vp = NULL;

			if (variables.count(name.c_str()) == 0){
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(Store Reg Operand) for "<<name<<std::endl;
#endif
				vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;					
                if (currentLineNum != 0) {
				  int lnm_cln = lnm[currentLineNum];
				  vp->lineNumOrder = lnm_cln;
				  lnm_cln++;
				  lnm[currentLineNum] = lnm_cln;
                  lnmChanged = true;
				}
					
				variables[name.c_str()] = vp;
				varCount++;
			}
            else
                vp = variables[name.c_str()];

            //we create a FuncStore only when we met the content  
            if(opNum==0) {
                User::op_iterator op_i2 = pi->op_begin();
                Value  /**firstStr = *op_i2, */  *secondStr = *(++op_i2);
#ifdef DEBUG_LLVM
                blame_info<<"STORE to(5) "<<secondStr->getName().str()<<" from "<<vp->name<<" "<<lnm[currentLineNum]<<std::endl;
#endif 
                FuncStores *fs = new FuncStores();
                
                if (secondStr->hasName() && variables.count(secondStr->getName().data()))
                    fs->receiver = variables[secondStr->getName().data()];
                else
                    fs->receiver = NULL;
                fs->contents = vp;
                fs->line_num = currentLineNum;
                if(lnmChanged)
                    fs->lineNumOrder = lnm[currentLineNum]-1;
                else
                    fs->lineNumOrder = lnm[currentLineNum];

                blame_info<<"STORE to(5) fs->lineNumOrder="<<fs->lineNumOrder<<" in line# "<<currentLineNum<<endl;
                allStores.push_back(fs);
            }
        }

		else if(v->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_LLVM
			blame_info<<"ValueID is "<<v->getValueID()<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
#ifdef DEBUG_LLVM
				blame_info<<"Value is ConstantExpr"<<std::endl;
#endif 
				
				ConstantExpr *ce = cast<ConstantExpr>(v);	
				createNPFromConstantExpr(ce, varCount, currentLineNum, fbb);
			}
			
		}
		else { //TC: ConstantExpr->getValueID != ConstantExprVal ??
#ifdef DEBUG_LLVM
			blame_info<<"ValueID is "<<v->getValueID()<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
#ifdef DEBUG_LLVM
				blame_info<<"Value is ConstantExpr"<<std::endl;
#endif 
				
#ifdef DEBUG_LLVM
				ConstantExpr *ce = cast<ConstantExpr>(v);	
				blame_info<<"Value opcode is "<<ce->getOpcode()<<std::endl;
				blame_info<<"Value opcodeName is "<<ce->getOpcodeName()<<std::endl;
#endif 
				
#ifdef DEBUG_LLVM
				for (User::op_iterator op_i2 = ce->op_begin(), op_e2 = ce->op_end(); op_i2 != op_e2; ++op_i2) {
					Value *v2 = *op_i2;
					if (v2->hasName())
						blame_info<<"CE op name "<<v2->getName().str()<<std::endl;
					else
						blame_info<<"CE op has no name"<<std::endl;
				}
#endif 
			}
		}
		opNum++;
	}
}

void FunctionBFC::ieGen_OperandsGEP(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	int opNum = 0;
	// Add operands to list of symbols, which are var nodes in AST
    //blame_info<<"NumOperands="<<pi->getNumOperands()<<std::endl;
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
#ifdef DEBUG_LLVM
		if (!(v->hasName())) {
			blame_info<<"Standard Operand No Name "<<" "<<v<<" ";
			printValueIDName(v);
			blame_info<<std::endl;
		}
#endif		
		if (v->hasName() &&  v->getValueID() != Value::BasicBlockVal) {
			if (variables.count(v->getName().data()) == 0) {
				std::string name = v->getName().str();
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(2) for "<<name<<std::endl;
#endif 
				NodeProps * vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0)
				{
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}

				variables[v->getName().data()] = vp;					
				varCount++;
			}
		}
		// This is for dealing with Constants, no matter v has name or not
		else if (v->getValueID() == Value::ConstantIntVal) {
			ConstantInt *cv = (ConstantInt *)v;	
			int number = cv->getSExtValue();
			
			if (opNum == 0 || opNum == 1) { //ignore the first two operands
				opNum++;                    //major var and its index in array
				continue;                   //default 0 if only 1 structure isf
			}
			
			char tempBuf[64];
			//sprintf (tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, opNum, pi->getOpcode());		
			sprintf (tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, opNum, Instruction::GetElementPtr);		
			char * vN = (char *)malloc(sizeof(char)*(strlen(tempBuf)+1));
			
			strcpy(vN,tempBuf);
			vN[strlen(tempBuf)]='\0';
			const char * vName = vN;
			
			if (variables.count(vName) == 0 && opNum == 2) { //TC: why opNum has to be 2 ?
				std::string name(vName);
				//std::cout<<"Creating VP for Constant "<<vName<<" in "<<getSourceFuncName()<<std::endl;
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(4) for "<<name<<std::endl;
#endif 
				NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}
				
				variables[vName] = vp;
				varCount++;
				//printCurrentVariables();
			}
	    }
		else if(v->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_LLVM
			blame_info<<"ValueID is "<<v->getValueID()<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
#ifdef DEBUG_LLVM
				blame_info<<"Value is ConstantExpr"<<std::endl;
#endif 
				ConstantExpr * ce = cast<ConstantExpr>(v);	
				createNPFromConstantExpr(ce, varCount, currentLineNum, fbb);
			}
		}
		opNum++;
	}
}

//Almost same as ieGen_Operands() except we start from the second operand
//Because the first operand is the "operation" to be executed
void FunctionBFC::ieGen_OperandsAtomic(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
	int opNum = 0;
	// Add operands to list of symbols
    User::op_iterator op_i = pi->op_begin();
    op_i++; //Start from the second operand
	for (User::op_iterator op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		
#ifdef DEBUG_LLVM
		if (!(v->hasName())) {
			blame_info<<"Standard Operand No Name "<<" "<<v<<" ";
			printValueIDName(v);
			blame_info<<std::endl;
		}
#endif			
		
		if (v->hasName() && v->getValueID() != Value::BasicBlockVal) {
			if (variables.count(v->getName().data()) == 0) {
				std::string name = v->getName().str();
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(2) for "<<name<<std::endl;
#endif 
				NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}

				variables[v->getName().data()] = vp;					
				varCount++;
				//printCurrentVariables();
			}
		}
		else if(v->getValueID() == Value::ConstantExprVal) { //v has no name but is a constant expression
#ifdef DEBUG_LLVM
			blame_info<<"ValueID is "<<v->getValueID()<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
#ifdef DEBUG_LLVM
				blame_info<<"Value is ConstantExpr"<<std::endl;
#endif 
				ConstantExpr *ce = cast<ConstantExpr>(v);	
				createNPFromConstantExpr(ce, varCount, currentLineNum, fbb); 
			}
		}
		opNum++;
	}
}

void FunctionBFC::ieDefault(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieDefault for "<<pi->getOpcodeName()<<" "<<pi->getName().str()<<std::endl;
#endif
	
	ieGen_LHS(pi, varCount, currentLineNum, fbb);
	ieGen_Operands(pi, varCount, currentLineNum, fbb);
}


void FunctionBFC::ieLoad(Instruction * pi, int & varCount, int & currentLineNum, FunctionBFCBB * fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieLoad for "<<pi->getName().str()<<std::endl;
#endif
	
	ieGen_LHS(pi, varCount, currentLineNum, fbb);
	ieGen_Operands(pi, varCount, currentLineNum, fbb);
}


void FunctionBFC::ieGetElementPtr(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieGetElementPtr for "<<pi->getName().str()<<std::endl;
#endif
	ieGen_LHS(pi, varCount, currentLineNum, fbb);
	ieGen_OperandsGEP(pi, varCount, currentLineNum, fbb);
}

void FunctionBFC::ieStore(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieStore"<<std::endl;
#endif
	//We'll generate allStores later after both nodes of the operands are built
	ieGen_OperandsStore(pi, varCount, currentLineNum, fbb);
}

void FunctionBFC::ieSelect(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieSelect"<<std::endl;
#endif 
	
#ifdef DEBUG_LLVM
	blame_info<<"LLVM__(examineInstruction)(Select) -- pi "<<pi->getName().str()<<" "<<pi<<" "<<pi->getOpcodeName()<<std::endl;
#endif	 	
	// Add LHS variable to list of symbols
	if (pi->hasName() && variables.count(pi->getName().data()) == 0) {
		std::string name = pi->getName().str();
#ifdef DEBUG_VP_CREATE
		blame_info<<"Adding NodeProps(7) for "<<name<<std::endl;
#endif 
		NodeProps *vp = new NodeProps(varCount,name,currentLineNum,pi);
		vp->fbb = fbb;
		if (currentLineNum != 0) {
			int lnm_cln = lnm[currentLineNum];
			vp->lineNumOrder = lnm_cln;
			lnm_cln++;
			lnm[currentLineNum] = lnm_cln;
		}
		
		variables[pi->getName().data()] = vp;
		varCount++;				
	}	
	
	int opNum = 0;
	
	// Add operands to list of symbols
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		if (v->hasName() &&  v->getValueID() != Value::BasicBlockVal) {
			if (variables.count(v->getName().data()) == 0) {
				std::string name = v->getName().str();
#ifdef DEBUG_VP_CREATE
				blame_info<<"Adding NodeProps(8) for "<<name<<std::endl;
#endif 
				NodeProps * vp = new NodeProps(varCount,name,currentLineNum,pi);
				vp->fbb = fbb;
				
				if (currentLineNum != 0) {
					int lnm_cln = lnm[currentLineNum];
					vp->lineNumOrder = lnm_cln;
					lnm_cln++;
					lnm[currentLineNum] = lnm_cln;
				}
				variables[v->getName().data()] = vp;					
				varCount++;
            }
		}
		opNum++;
	}
}


void FunctionBFC::ieBlank(Instruction * pi, int & currentLineNum)
{
#ifdef DEBUG_LLVM
	blame_info<<"In ieBlank for opcode "<<pi->getOpcodeName()<<" "<<currentLineNum<<std::endl;
#endif
	
}

//To be used for processing atomic memory operations
void FunctionBFC::ieMemAtomic(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
    blame_info<<"In ieMemAtomic for opocode "<<pi->getOpcodeName()<<" at "<<currentLineNum<<std::endl;
#endif
    if (pi->getOpcode() == Instruction::AtomicCmpXchg) { // same process as "Load"
#ifdef DEBUG_LLVM
        blame_info<<"In ieMemAtomic for cmpxchg"<<std::endl;
#endif                 
        ieGen_LHS(pi, varCount, currentLineNum, fbb);
        ieGen_Operands(pi, varCount, currentLineNum, fbb);
    }

    else if (pi->getOpcode() == Instruction::AtomicRMW) {
#ifdef DEBUG_LLVM
        blame_info<<"In ieMemAtomic for atomicrmw"<<std::endl;
#endif 
        ieGen_LHS(pi, varCount, currentLineNum, fbb);
        ieGen_OperandsAtomic(pi, varCount, currentLineNum, fbb);
    }
}

void FunctionBFC::ieBitCast(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb)
{
  /* We need to determine which symbols deal with debug info and ignore them */	
#ifdef DEBUG_LLVM
	blame_info<<"LLVM__(examineInstruction)(Bitcast) -- pi "<<pi->getName().str()<<" "<<pi<<" "<<pi->getOpcodeName()<<std::endl;
#endif
	
	//for (Value::use_iterator use_i = pi->use_begin(), use_e = pi->use_end(); use_i != use_e; ++use_i)
	if (pi->getNumUses() == 1) {
		Value::use_iterator use_i = pi->use_begin();
		if (Instruction *usesBit = dyn_cast<Instruction>(*use_i)) {
			if ((usesBit->getOpcode() == usesBit->Call) && isa<CallInst>(usesBit)) {
                CallInst *ci = cast<CallInst>(usesBit);
                if(ci != NULL){
                    Function *calledFunc = ci->getCalledFunction();
				    if(calledFunc != NULL)
                      if(calledFunc->hasName())
                        if(calledFunc->getName().str().find("llvm.dbg") != std::string::npos)
				          return;
                }
			}						
		}
	}
	
	ieGen_LHS(pi, varCount, currentLineNum, fbb);
	ieGen_Operands(pi, varCount, currentLineNum, fbb);
	
}

void FunctionBFC::ieAlloca(Instruction * pi, int & varCount, int & currentLineNum, FunctionBFCBB	* fbb)
{	
	ieGen_LHS_Alloca(pi, varCount, currentLineNum, fbb);
	ieGen_Operands(pi, varCount, currentLineNum, fbb);	
}




void FunctionBFC::examineInstruction(Instruction *pi, int &varCount, int &currentLineNum, 
										    RegHashProps &variables, FunctionBFCBB *fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"Entering examineInstruction "<<pi->getOpcodeName()<<" "<<pi<<" "<<currentLineNum<<" ";
	printValueIDName(pi);
	blame_info<<std::endl;
#endif
	
    // We are interested in operands from
    // - Binary Ops
	// - Comparison Operations 
	// - Cast Operations
	// - Malloc/Free/Alloca
	// - Load/Store
	
#ifdef ENABLE_FORTRAN
	if (firstGEPCheck(pi) == false) {//only useful for fortran
        blame_info<<"firstGEPCheck return false !"<<std::endl;
        return;
    }
#endif

	genDILocationInfo(pi, currentLineNum, fbb); //generate location info of the current instruction

	// These call operations are almost exclusively for llvm.dbg.declare, the
	//  more general case with "real" data will be tackled below
    if (pi->getOpcode() == Instruction::Call) {
		if (parseDeclareIntrinsic(pi, currentLineNum, fbb) == true)   //TO CHECK: should the condition be false ?
			return;
	}
  
    // 1
	// TERMINATOR OPS
	if (pi->getOpcode() == Instruction::Ret)    
		ieBlank(pi, currentLineNum);
	else if (pi->getOpcode() == Instruction::Br)
		ieBlank(pi, currentLineNum);
	else if (pi->getOpcode() == Instruction::Switch)
		ieBlank(pi, currentLineNum);
    else if (pi->getOpcode() == Instruction::IndirectBr) //TC
        ieBlank(pi, currentLineNum);
	else if (pi->getOpcode() == Instruction::Invoke)
		ieInvoke(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Resume) //TC
		ieBlank(pi, currentLineNum);
	else if (pi->getOpcode() == Instruction::Unreachable)
		ieBlank(pi, currentLineNum);
	// END TERMINATOR OPS
	
	// BINARY OPS 
	else if (pi->getOpcode() == Instruction::Add)
		ieDefault(pi, varCount, currentLineNum, fbb);
    else if (pi->getOpcode() == Instruction::FAdd)
        ieDefault(pi, varCount, currentLineNum, fbb); //TC
	else if (pi->getOpcode() == Instruction::Sub)
		ieDefault(pi, varCount, currentLineNum, fbb);
    else if (pi->getOpcode() == Instruction::FSub)    //TC
        ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Mul)
		ieDefault(pi, varCount, currentLineNum, fbb);
    else if (pi->getOpcode() == Instruction::FMul)    //TC
        ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::UDiv)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::SDiv)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FDiv)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::URem)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::SRem)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FRem)
		ieDefault(pi, varCount, currentLineNum, fbb);
	// END BINARY OPS
	
	// LOGICAL OPERATORS 
	else if (pi->getOpcode() == Instruction::Shl)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::LShr)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::AShr)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::And)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Or)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Xor)
		ieDefault(pi, varCount, currentLineNum, fbb);
	// END LOGICAL OPERATORS
	
	// MEMORY OPERATORS
	else if (pi->getOpcode() == Instruction::Alloca)
		ieAlloca(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Load)
		ieLoad(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Store)
		ieStore(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::GetElementPtr)	
		ieGetElementPtr(pi, varCount, currentLineNum, fbb);
    else if (pi->getOpcode() == Instruction::Fence)    //TC
        ieBlank(pi, currentLineNum);
    else if (pi->getOpcode() == Instruction::AtomicCmpXchg)  //TC
        ieMemAtomic(pi, varCount, currentLineNum, fbb);
    else if (pi->getOpcode() == Instruction::AtomicRMW)    //TC
        ieMemAtomic(pi, varCount, currentLineNum, fbb);
	// END MEMORY OPERATORS
	
	// CAST OPERATORS
	else if (pi->getOpcode() == Instruction::Trunc)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::ZExt)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::SExt)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FPToUI)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FPToSI)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::UIToFP)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::SIToFP)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FPTrunc)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FPExt)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::PtrToInt)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::IntToPtr)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::BitCast)
		ieBitCast(pi, varCount, currentLineNum, fbb);
	
	// END CAST OPERATORS
	
	// OTHER OPERATORS
	else if (pi->getOpcode() == Instruction::ICmp)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::FCmp)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::PHI)
		ieDefault(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Call)
		ieCall(pi, varCount, currentLineNum, fbb);
	else if (pi->getOpcode() == Instruction::Select)
		ieSelect(pi, varCount, currentLineNum, fbb);
	else //There're still other Insts unhandled as: UserOp1, UserOp2, VAArg,   //TC
	{    //IExtractElement, ShuffleVector, ExtractValue, insertValue, LandingPad    
#ifdef DEBUG_ERROR
		std::cerr<<"Other kind of opcode(2) is : "<<pi->getOpcodeName()<<"\n";
		blame_info<<"LLVM__(examineInstruction)(NCH) -- pi "<<pi->getName().str()<<" "<<pi<<" "<<pi->getOpcodeName()<<std::endl;
#endif			
	}		  
}



void FunctionBFC::createNPFromConstantExpr(ConstantExpr *ce, int &varCount, int &currentLineNum, FunctionBFCBB	*fbb)
{
#ifdef DEBUG_LLVM
	blame_info<<"In CreateVPFromConstantExpr"<<std::endl;
#endif
	
	if (ce->getOpcode() == Instruction::GetElementPtr)
		ieGetElementPtr(ce, varCount, currentLineNum, fbb);
	else if (ce->getOpcode() == Instruction::BitCast) {
		// Don't need to do anything here since for the graph we're just going to grab the first
		// element of the bitcast anyway
	}
	else {
#ifdef DEBUG_LLVM
		blame_info<<"Constant Expr cvpce for "<<ce->getOpcodeName()<<std::endl; //NOTHING TO DO WITH THIS ?
#endif
	}	
}


// In LLVM, each Param has the param number appended to it.  We are interested
// in the address of these params ".addr" appended to the name without
//  the number, THIS FUNCTION not used anywhere
void paramWithoutNumWithAddr(std::string & original)
{
	unsigned i;
	int startOfNum = -1;
	for (i=0; i < original.length(); i++)
	{
		if (original[i] >= 48 && original[i] <= 57 && startOfNum == -1)
			startOfNum = i;
		if (original[i] < 48 || original[i] > 57)
			startOfNum = -1;
	}
	
	original.replace(startOfNum, 5, ".addr");
}


bool FunctionBFC::varLengthParams()   //TO-CHECK va_arg: http://en.wikibooks.org/wiki/C++_Programming/Code/Standard_C_Library/Functions/va_arg
{
	for (Function::iterator b = func->begin(), be = func->end(); b != be; ++b) {
		for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
			Instruction *pi = i;
			//std::cout<<"Name - "<<pi->getName()<<std::endl;
			// If we make it to the first call without seeing argptr we know it's not variable list
            //variadic functions: a function that can take unfixed # of arguments, e.g: printf
			if (pi->getOpcode() == Instruction::Call) {
				return false;
			}
            // the final arg can be a list/array of args, which is called Varargs
			//if ( pi->getName().find("argptr") != std::string::npos) { //va_list argptr
            else if (pi->getOpcode() == Instruction::VAArg) {
#ifdef DEBUG_LLVM
				blame_info<<"LLVM__(varLengthParams)--Variable Length Args!"<<std::endl;
#endif
				return true;
			}
		}
	}
	return false;
}



