/*
 *  FunctionBFCCFG.cpp
 *  
 *  Implementation of the class FunctionBFCCFG
 *  and FunctionBFCBB: basic blocks
 *
 *  Created by Hui Zhang on 04/03/15.
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#include "FunctionBFCCFG.h"

using namespace std;
using namespace llvm;


void FunctionBFCCFG::calcPTRStoreLines()
{
	BBHash::iterator bbh_i;
	std::set<int>::iterator set_i_i;


	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++)
	{
		FunctionBFCBB * fbb = bbh_i->second;
		
		std::set<NodeProps *>::iterator set_vp_i;
		
		
		// Look at the variables genned elsewhere (IN to the BB)
		for (set_vp_i = fbb->inPTR_BB.begin(); set_vp_i != fbb->inPTR_BB.end(); set_vp_i++)
		{
			NodeProps * vp = (*set_vp_i);
			
			vp->storePTR_Lines.insert(vp->line_num);
			
			// the variable is in IN and OUT set, it doesn't get killed, we can safely 
			//   add all the line numbers for this basic block to the valid CF lines
			if (fbb->outPTR_BB.count(vp) > 0)
			{
				vp->storePTR_Lines.insert(fbb->lineNumbers.begin(), fbb->lineNumbers.end());
			}
			// the variable is not in the out set, must get killed along the way, all of
			// the line numbers leading up to being killed are relevant
			else
			{
					NodeProps * killer = NULL;
			
					std::vector<NodeProps *>::iterator vec_vp_i2;

					// Find an instruction that kills the IN instruction
					for (vec_vp_i2 = fbb->singleStores.begin(); 
							vec_vp_i2 != fbb->singleStores.end(); vec_vp_i2++)
					{
						NodeProps * potKiller = (*vec_vp_i2);
						if (potKiller->killPTR_VP.count(vp) > 0)
						{
							killer = potKiller;
							break;
						}
					}				
			
				if (killer != NULL)
				{
					for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++)
					{
						if (vp->line_num <= *set_i_i && *set_i_i < killer->line_num)
							vp->storePTR_Lines.insert(*set_i_i);
							
									
						if (*set_i_i == killer->line_num)
							vp->borderPTR_Lines.insert(*set_i_i);	
					}
				}						
			}
		}
		
		// Now we need to look at those variables that were genned in this basic block
		std::vector<NodeProps *>::iterator vec_vp_i;
		for (vec_vp_i = fbb->singleStores.begin(); 
					vec_vp_i != fbb->singleStores.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			vp->storePTR_Lines.insert(vp->line_num);

			
			NodeProps * killer = NULL;
			//std::vector<NodeProps *>::iterator vec_vp_i2 = vec_vp_i;
			std::vector<NodeProps *>::iterator vec_vp_i2 = fbb->singleStores.begin();
			
			// Find an instruction that kills the genned instruction
			for (; vec_vp_i2 != fbb->singleStores.end(); vec_vp_i2++)
			{
				NodeProps * potKiller = (*vec_vp_i2);
				if (potKiller->killPTR_VP.count(vp) > 0)
				{
					killer = potKiller;
					break;
				}
			}
			
			
			// if killer is null we take all line number from the genned line number on
			if (killer == NULL)
			{
				for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++)
				{
					if (vp->line_num <= *set_i_i)
						vp->storePTR_Lines.insert(*set_i_i);
				}
			}
			// if there is a killer then we take all line number up to (and including) that line num
			//  line number ties are resolved later on a case by case
			else if (killer->line_num > 0)
			{
				for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++)
				{
					if (vp->line_num <= *set_i_i && *set_i_i < killer->line_num)
						vp->storePTR_Lines.insert(*set_i_i);
					
					// This essentially means that
					// a) they were both defined on the same line
					// b) there is some kind of loop as the killer came before the killee
					if (vp->line_num <= *set_i_i && 
							((vp->line_num == killer->line_num) && killer->lineNumOrder < vp->lineNumOrder))
						vp->storePTR_Lines.insert(*set_i_i);
					
					if (*set_i_i == killer->line_num)
						vp->borderPTR_Lines.insert(*set_i_i);
				}				
			
			}
			else
			{
#ifdef DEBUG_CFG_ERROR			
				std::cerr<<"NULL or lineNum == 0 killer"<<std::endl;
#endif 
			}
		}
	}
	
}

void FunctionBFCBB::assignPTRGenKill()
{
	std::vector<NodeProps *>::iterator vec_vp_i;
	
	
	// Create Gen and Kill for each Instruction (VP)
	for (vec_vp_i = singleStores.begin(); vec_vp_i != singleStores.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		
		// Trivial case that you gen yourself
		vp->genPTR_VP.insert(vp);
		
		// Our kills need to be writes to other stores that have the same upPtr as us
		
		if (vp->dpUpPtr != vp)
		{
			std::set<NodeProps *>::iterator set_vp_i;
			for (set_vp_i = vp->dpUpPtr->dataPtrs.begin(); set_vp_i != vp->dpUpPtr->dataPtrs.end(); set_vp_i++)
			{
				NodeProps * kills = *set_vp_i;
				if (kills->isWritten && kills != vp)
					vp->killPTR_VP.insert(kills);
			}
		}
		
//////////////TO BE DELETED/////////////////////////////////////////////
		std::set<NodeProps *>::iterator set_vp_i;
		for (set_vp_i = vp->storeFrom->storesTo.begin(); set_vp_i != vp->storeFrom->storesTo.end(); set_vp_i++)
		{
			NodeProps * kills = *set_vp_i;
			
			// We kill all other stores that occur anywhere else besides the VP at that line
			if (kills != vp)
				vp->killVP.insert(kills);
		}
/////////////////////////////////////////////////////////////////////////
		
	}
	
	// Use that information to create Gen/Kill for entire BB
	for (vec_vp_i = singleStores.begin(); vec_vp_i != singleStores.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);

		std::set<NodeProps *>::iterator set_vp_i;
			
		for (set_vp_i = vp->genPTR_VP.begin(); set_vp_i !=  vp->genPTR_VP.end(); set_vp_i++)
			genPTR_BB.insert(*set_vp_i);
						
		for (set_vp_i = vp->killPTR_VP.begin(); set_vp_i !=  vp->killPTR_VP.end(); set_vp_i++)
		{
			NodeProps * kills = *set_vp_i;
			
			if (genPTR_BB.count(kills) > 0)
				genPTR_BB.erase(kills);
			else
				killPTR_BB.insert(kills);
		}
		
	}
}

// For Reaching Definitions
void FunctionBFCCFG::assignPTRBBGenKill()
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
		
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++)
	{
		FunctionBFCBB * fbb = bbh_i->second;
		fbb->assignPTRGenKill();
		
		if (fbb->bbName.find("entry") != std::string::npos)
			fbb->outPTR_BB.insert(fbb->genPTR_BB.begin(), fbb->genPTR_BB.end());
		
	}

}

// For Reaching Definitions
void FunctionBFCCFG::reachingPTRDefs()
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
	
	int changed = 1;
	
	while (changed)
	{	
		changed = 0;		
		for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++)
		{
			FunctionBFCBB * fbb = bbh_i->second;
			//	fbb->assignGenKill();
			
			std::set<FunctionBFCBB *>::iterator set_fbb_i;
			std::set<NodeProps *>::iterator set_vp_i;
			
			// Create IN[B] set from Union of OUT[p] where p is a predecessor
			fbb->inPTR_BB.clear();
			for (set_fbb_i = fbb->preds.begin(); set_fbb_i != fbb->preds.end(); set_fbb_i++)
			{
				FunctionBFCBB * predBB = *set_fbb_i;
				for (set_vp_i = predBB->outPTR_BB.begin(); set_vp_i != predBB->outPTR_BB.end(); set_vp_i++)
					fbb->inPTR_BB.insert(*set_vp_i);
			}
			
			// Create new OUT set form GEN[B] Union (IN[B] - KILL[B])
			
			std::set<NodeProps *> newOutBB;
			// new OUT[B] = GEN[B]
			newOutBB.insert(fbb->genPTR_BB.begin(), fbb->genPTR_BB.end());
			
			std::set<NodeProps *> inMinusKillBB;
			
			// inMinusKillBB[B] = IN[B]
			inMinusKillBB.insert(fbb->inPTR_BB.begin(), fbb->inPTR_BB.end());
			
			// inMinusKillBB[B] = IN[B] - KILL[B]
			for (set_vp_i = fbb->killPTR_BB.begin(); set_vp_i != fbb->killPTR_BB.end(); set_vp_i++)
			{
				NodeProps * killed = *set_vp_i;
				inMinusKillBB.erase(killed);
			}	
			
			// new OUT = GEN[B] U (IN[B] - KILL[B])
			for (set_vp_i = inMinusKillBB.begin(); set_vp_i != inMinusKillBB.end(); set_vp_i++)
			{
				newOutBB.insert(*set_vp_i);
			}
			
			if (newOutBB != fbb->outPTR_BB)
				changed++;
			
			fbb->outPTR_BB.swap(newOutBB);
		}
	}
	
}



// Does the first Vertex come before it in the CFG
// Auxiliary Function
bool FunctionBFCCFG::controlDep(NodeProps *target, NodeProps *anchor)
{
	FunctionBFCBB *tBB = target->fbb;
	FunctionBFCBB *aBB = anchor->fbb;
	
	if (tBB == NULL) {
#ifdef DEBUG_CFG_ERROR			
		std::cerr<<"TBB is NULL for "<<target->name<<std::endl;
#endif
		return false;
	}
	
	if (aBB == NULL) {
#ifdef DEBUG_CFG_ERROR			
		std::cerr<<"ABB is NULL for "<<anchor->name<<std::endl;
#endif
		return false;
	}
	
	// the target came before
	if (aBB->ancestors.count(tBB) > 0) {
		return true;
	}
	else if (tBB == aBB) {
		if (anchor->line_num > target->line_num)
			return true;
		else
			return false;
	}
	
	return false;
}

// Auxiliary Function
void FunctionBFCBB::genD(FunctionBFCBB *fbb, std::set<FunctionBFCBB *> &visited)
{
	if (fbb == NULL)
		return;
		
	if (fbb->bbName.empty())
		return;

	if (visited.count(fbb) > 0)
		return;
	
	visited.insert(fbb);
	
	std::set<FunctionBFCBB *>::iterator set_fbb_i;

	for (set_fbb_i = fbb->succs.begin(); set_fbb_i != fbb->succs.end(); set_fbb_i++) {
		if (*set_fbb_i != NULL) {
			descendants.insert(*set_fbb_i);
			genD(*set_fbb_i, visited);
		}
	}	
}

// Auxiliary Function
void FunctionBFCBB::genA(FunctionBFCBB *fbb, std::set<FunctionBFCBB *> &visited)
{
	if (fbb == NULL)
		return;
		
	if (fbb->bbName.empty()) 
		return;

	if (visited.count(fbb) > 0)
		return;
	
	visited.insert(fbb);
	
	std::set<FunctionBFCBB *>::iterator set_fbb_i;
	
	for (set_fbb_i = fbb->preds.begin(); set_fbb_i != fbb->preds.end(); set_fbb_i++) {
		if (*set_fbb_i != NULL) {
			ancestors.insert(*set_fbb_i);
			genA(*set_fbb_i, visited);
		}
	}
}

// Auxiliary Function
void FunctionBFCCFG::genAD()
{
	std::set<FunctionBFCBB *> visited;
	BBHash::iterator bbh_i;
	
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end(); bbh_i++) {
		visited.clear();
		FunctionBFCBB *fbb = (*bbh_i).second;
		if (fbb == NULL)
			continue;
			
		fbb->genA(fbb, visited);
		visited.clear();
		fbb->genD(fbb, visited);
	}
}


// Auxiliary Function
void FunctionBFCCFG::genEdges()
{
	BBHash::iterator bbh_i, bbh_e; //hash_map iterator: iterates over all hash elements
	for (bbh_i = LLVM_BBs.begin(), bbh_e = LLVM_BBs.end(); bbh_i != bbh_e; bbh_i++) {
		FunctionBFCBB *fbb = bbh_i->second;
		BasicBlock *b =  fbb->llvmBB;
		// Successors
		for (succ_iterator Iter = succ_begin(b), En = succ_end(b); Iter != En; ++Iter) {
			BasicBlock *succB = *Iter;
			FunctionBFCBB *succBFCBB = LLVM_BBs[succB->getName().data()];
				
			if (succBFCBB == NULL) {
#ifdef DEBUG_CFG_ERROR			
					std::cerr<<"Successor BB not found"<<std::endl;
#endif
			}
			else {
					//O<<"Adding successor "<<succFBB->bbName<<" to "<<fbb->bbName<<std::endl;
					fbb->succs.insert(succBFCBB);
			}
		}
			
		// Predecessors
		for (pred_iterator Iter = pred_begin(b), En = pred_end(b); Iter != En; ++Iter) {
			BasicBlock * predB = *Iter;
			FunctionBFCBB * predBFCBB = LLVM_BBs[predB->getName().data()];
				
			if (predBFCBB == NULL) {
#ifdef DEBUG_CFG_ERROR			
					std::cerr<<"Predecessor BB not found"<<std::endl;
#endif 
			}
			else {
			    //O<<"Adding predecessor "<<predFBB->bbName<<" to "<<fbb->bbName<<std::endl;				
				fbb->preds.insert(predBFCBB);
			}
		}
	}
}



// Auxiliary Function
void FunctionBFCCFG::assignBBGenKill()
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
		
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++) {
		FunctionBFCBB *fbb = bbh_i->second;
		fbb->assignGenKill();
		
		if (fbb->bbName.find("entry") != std::string::npos)
			fbb->outBB.insert(fbb->genBB.begin(), fbb->genBB.end());
		
	}
}


// Auxiliary Function
void FunctionBFCCFG::printCFG(std::ostream & O)
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
		
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++)
	{
		FunctionBFCBB * fbb = bbh_i->second;
		O<<"FBB name: "<<fbb->bbName<<std::endl;
		
		std::set<FunctionBFCBB *>::iterator set_fbb_i;
	
		O<<"Successors: ";
		for (set_fbb_i = fbb->succs.begin(); set_fbb_i != fbb->succs.end(); set_fbb_i++)
		{
			FunctionBFCBB * succBB = (*set_fbb_i);
			O<<succBB->bbName<<" ";
		}
		O<<endl;
		
		O<<"Predecessors: ";
		for (set_fbb_i = fbb->preds.begin(); set_fbb_i != fbb->preds.end(); set_fbb_i++)
		{
			FunctionBFCBB * predBB = (*set_fbb_i);
			O<<predBB->bbName<<" ";
		}
		O<<std::endl;
		
		O<<"Ancestors: ";
		for (set_fbb_i = fbb->ancestors.begin(); set_fbb_i != fbb->ancestors.end(); set_fbb_i++)
		{
			FunctionBFCBB * predBB = (*set_fbb_i);
			O<<predBB->bbName<<" ";
		}
		O<<std::endl;
		
		O<<"Descendants: ";
		for (set_fbb_i = fbb->descendants.begin(); set_fbb_i != fbb->descendants.end(); set_fbb_i++)
		{
			FunctionBFCBB * predBB = (*set_fbb_i);
			O<<predBB->bbName<<" ";
		}
		O<<std::endl;
		
		
		O<<"Relevant Instructions: ";
		std::vector<NodeProps *>::iterator vec_vp_i;
		
		
		std::set<NodeProps *>::iterator set_vp_i;

		for (vec_vp_i = fbb->relevantInstructions.begin(); 
			vec_vp_i != fbb->relevantInstructions.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			O<<vp->name<<"("<<vp->line_num<<")"<<std::endl;
			
			
			O<<"   Gen: ";
			
			for (set_vp_i = vp->genVP.begin(); set_vp_i !=  vp->genVP.end(); set_vp_i++)
				O<<(*set_vp_i)->name<<"  ";
			
			O<<endl;
			
			O<<"   Kill: ";
			
			for (set_vp_i = vp->killVP.begin(); set_vp_i !=  vp->killVP.end(); set_vp_i++)
				O<<(*set_vp_i)->name<<"  ";			
	
			O<<endl;
		}
		
		O<<"BB Gen: ";
		for (set_vp_i = fbb->genBB.begin(); set_vp_i != fbb->genBB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB Kill: ";
		for (set_vp_i = fbb->killBB.begin(); set_vp_i != fbb->killBB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB In: ";
		for (set_vp_i = fbb->inBB.begin(); set_vp_i != fbb->inBB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB Out: ";
		for (set_vp_i = fbb->outBB.begin(); set_vp_i != fbb->outBB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		
		
		
		O<<"Single Stores: ";
		
		for (vec_vp_i = fbb->singleStores.begin(); 
			vec_vp_i != fbb->singleStores.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			O<<vp->name<<"("<<vp->line_num<<")"<<std::endl;
			
			
			O<<"   Gen: ";
			
			for (set_vp_i = vp->genPTR_VP.begin(); set_vp_i !=  vp->genPTR_VP.end(); set_vp_i++)
				O<<(*set_vp_i)->name<<"  ";
			
			O<<endl;
			
			O<<"   Kill: ";
			
			for (set_vp_i = vp->killPTR_VP.begin(); set_vp_i !=  vp->killPTR_VP.end(); set_vp_i++)
				O<<(*set_vp_i)->name<<"  ";			
	
			O<<endl;
		}
		
		O<<"BB Gen: ";
		for (set_vp_i = fbb->genPTR_BB.begin(); set_vp_i != fbb->genPTR_BB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB Kill: ";
		for (set_vp_i = fbb->killPTR_BB.begin(); set_vp_i != fbb->killPTR_BB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB In: ";
		for (set_vp_i = fbb->inPTR_BB.begin(); set_vp_i != fbb->inPTR_BB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;
		
		O<<"BB Out: ";
		for (set_vp_i = fbb->outPTR_BB.begin(); set_vp_i != fbb->outPTR_BB.end(); set_vp_i++)
			O<<(*set_vp_i)->name<<" ";
			
		O<<endl;

		
		
		
		O<<std::endl;
	}
			
	O<<endl;		
}

// For Reaching Definitions
void FunctionBFCCFG::calcStoreLines()
{
	BBHash::iterator bbh_i;
	std::set<int>::iterator set_i_i;
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++) {
		FunctionBFCBB *fbb = bbh_i->second;
		std::set<NodeProps *>::iterator set_vp_i;
			
		// Look at the variables genned elsewhere (IN to the BB)
		for (set_vp_i = fbb->inBB.begin(); set_vp_i != fbb->inBB.end(); set_vp_i++) {
			NodeProps *vp = (*set_vp_i);
			
			// TODO: Make it so it has statement granularity for this (instead of line nums)
			vp->storeLines.insert(vp->line_num);
			
			// the variable is in IN and OUT set, it doesn't get killed, we can safely 
			// add all the line numbers for this basic block to the valid CF lines
			if (fbb->outBB.count(vp) > 0) {
				vp->storeLines.insert(fbb->lineNumbers.begin(), fbb->lineNumbers.end());
			}
			//the variable is not in the out set, must get killed along the way, all of
			// the line numbers leading up to being killed are relevant
			else {
				NodeProps *killer = NULL;
			    std::vector<NodeProps *>::iterator vec_vp_i2;
				// Find an instruction that kills the IN instruction
				for (vec_vp_i2 = fbb->relevantInstructions.begin(); 
					vec_vp_i2 != fbb->relevantInstructions.end(); vec_vp_i2++) {
					NodeProps *potKiller = (*vec_vp_i2);
					if (potKiller->killVP.count(vp) > 0) {
						killer = potKiller;
						break;
					}
				}				
			
				if (killer != NULL) {
					for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++) {
						if (vp->line_num <= *set_i_i && *set_i_i < killer->line_num)
							vp->storeLines.insert(*set_i_i);
							
						if (*set_i_i == killer->line_num)
							vp->borderLines.insert(*set_i_i);	
					}
				}						
			}
		}
		
		// Now we need to look at those variables that were genned in this basic block
		std::vector<NodeProps *>::iterator vec_vp_i;
		for (vec_vp_i = fbb->relevantInstructions.begin(); 
					vec_vp_i != fbb->relevantInstructions.end(); vec_vp_i++) {
			
            NodeProps *vp = (*vec_vp_i);
			vp->storeLines.insert(vp->line_num);
			NodeProps *killer = NULL;
			//std::vector<NodeProps *>::iterator vec_vp_i2 = vec_vp_i;
			std::vector<NodeProps *>::iterator vec_vp_i2 = fbb->relevantInstructions.begin();
			// Find an instruction that kills the genned instruction
			for (; vec_vp_i2 != fbb->relevantInstructions.end(); vec_vp_i2++) {
				NodeProps * potKiller = (*vec_vp_i2);
				if (potKiller->killVP.count(vp) > 0) {
					killer = potKiller;
					break;
				}
			}
			 
			// if killer is null we take all line number from the genned line number on
			if (killer == NULL) {
				for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++) {
					if (vp->line_num <= *set_i_i)
						vp->storeLines.insert(*set_i_i);
				}
			}
			// if there is a killer then we take all line number up to (and including) that line num
			//  line number ties are resolved later on a case by case
			else if (killer->line_num > 0) {
				for (set_i_i = fbb->lineNumbers.begin(); set_i_i != fbb->lineNumbers.end(); set_i_i++) {
					if (vp->line_num <= *set_i_i && *set_i_i < killer->line_num)
						vp->storeLines.insert(*set_i_i);
					
					// This essentially means that
					//a)they were both defined on the same line
					//b)there is some kind of loop as the killer came before the killee
					if (vp->line_num <= *set_i_i && ((vp->line_num == killer->line_num)
                            && killer->lineNumOrder < vp->lineNumOrder))
						vp->storeLines.insert(*set_i_i);
					
					if (*set_i_i == killer->line_num)
						vp->borderLines.insert(*set_i_i);
				}				
			}
			else {
#ifdef DEBUG_CFG_ERROR			
				std::cerr<<"NULL or lineNum == 0 killer"<<std::endl;
#endif				
			}
		}
	}
}

// For Reaching Definitions
void FunctionBFCCFG::reachingDefs()
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
	
	int changed = 1;
	
	while (changed) {	
		changed = 0;		
		for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end();  bbh_i++) {
			FunctionBFCBB *fbb = bbh_i->second;
			//	fbb->assignGenKill();
			
			std::set<FunctionBFCBB *>::iterator set_fbb_i;
			std::set<NodeProps *>::iterator set_vp_i;
			
			// Create IN[B] set from Union of OUT[p] where p is a predecessor
			fbb->inBB.clear();
			for (set_fbb_i = fbb->preds.begin(); set_fbb_i != fbb->preds.end(); set_fbb_i++) {
				FunctionBFCBB *predBB = *set_fbb_i;
				for (set_vp_i = predBB->outBB.begin(); set_vp_i != predBB->outBB.end(); set_vp_i++)
					fbb->inBB.insert(*set_vp_i);
			}
			
			// Create new OUT set form GEN[B] Union (IN[B] - KILL[B])
			
			std::set<NodeProps *> newOutBB;
			// new OUT[B] = GEN[B]
			newOutBB.insert(fbb->genBB.begin(), fbb->genBB.end());
			
			std::set<NodeProps *> inMinusKillBB;
			
			// inMinusKillBB[B] = IN[B]
			inMinusKillBB.insert(fbb->inBB.begin(), fbb->inBB.end());
			
			// inMinusKillBB[B] = IN[B] - KILL[B]
			for (set_vp_i = fbb->killBB.begin(); set_vp_i != fbb->killBB.end(); set_vp_i++) {
				NodeProps *killed = *set_vp_i;
				inMinusKillBB.erase(killed);
			}	
			
			// new OUT = GEN[B] U (IN[B] - KILL[B])
			for (set_vp_i = inMinusKillBB.begin(); set_vp_i != inMinusKillBB.end(); set_vp_i++) {
				newOutBB.insert(*set_vp_i);
			}
			
			if (newOutBB != fbb->outBB)
				changed++;
			
			fbb->outBB.swap(newOutBB);
		}
	}
}

// For Reaching Definitions
void FunctionBFCBB::assignGenKill()
{
	std::vector<NodeProps *>::iterator vec_vp_i;
	// Create Gen and Kill for each Instruction (VP)
	for (vec_vp_i = relevantInstructions.begin(); vec_vp_i != relevantInstructions.end(); vec_vp_i++) {
		NodeProps *vp = (*vec_vp_i);
		// Trivial case that you gen yourself
		vp->genVP.insert(vp);
		
		std::set<NodeProps *>::iterator set_vp_i;
		for (set_vp_i = vp->storeFrom->storesTo.begin(); set_vp_i != vp->storeFrom->storesTo.end(); set_vp_i++) {
			NodeProps *kills = *set_vp_i;
		//We kill all other stores that occur anywhere else besides the VP at that line
			if (kills != vp)
				vp->killVP.insert(kills);
		}
	}
	
	// Use that information to create Gen/Kill for entire BB
	for (vec_vp_i = relevantInstructions.begin(); vec_vp_i != relevantInstructions.end(); vec_vp_i++) {
		NodeProps *vp = (*vec_vp_i);
		std::set<NodeProps *>::iterator set_vp_i;
			
		for (set_vp_i = vp->genVP.begin(); set_vp_i !=  vp->genVP.end(); set_vp_i++)
			genBB.insert(*set_vp_i);
						
		for (set_vp_i = vp->killVP.begin(); set_vp_i !=  vp->killVP.end(); set_vp_i++){
			NodeProps *kills = *set_vp_i;
			
			if (genBB.count(kills) > 0)
				genBB.erase(kills);
			else
				killBB.insert(kills);
		}
	}
}


// For Reaching Definitions - Not duplicated
void FunctionBFCCFG::sortCFG()
{
	BBHash::iterator bbh_i;
	//FunctionBFCBB * entry = NULL;	
		
	for (bbh_i = LLVM_BBs.begin(); bbh_i != LLVM_BBs.end(); bbh_i++) {
		FunctionBFCBB *fbb = bbh_i->second;
		fbb->sortInstructions();
	}

}

// For Reaching Definitions - Not Duplicated
void FunctionBFCCFG::setDomLines(NodeProps * vp)
{
	FunctionBFCBB * fbb = vp->fbb;
	
	set<FunctionBFCBB *>::iterator s_fbb_i;
	for (s_fbb_i = fbb->descendants.begin(); s_fbb_i != fbb->descendants.end(); s_fbb_i++)
	{
		FunctionBFCBB * descBB = *s_fbb_i;
		vp->domLineNumbers.insert(descBB->lineNumbers.begin(), descBB->lineNumbers.end());
	}
	
	set<int>::iterator myBBLines;
	for (myBBLines = fbb->lineNumbers.begin();  myBBLines != fbb->lineNumbers.end(); myBBLines++)
	{
		if (*myBBLines > vp->line_num)
			vp->domLineNumbers.insert(*myBBLines);
	}
}





