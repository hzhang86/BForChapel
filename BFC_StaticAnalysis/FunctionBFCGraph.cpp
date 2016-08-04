/*
 *  FunctionBFCGraph.cpp
 *  Implementation of graph generation
 *
 *  Created by Hui Zhang on 03/20/15.
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#include "ExitVars.h"
#include "FunctionBFC.h"
#include <sstream>

using namespace std;

/*void NodeProps::addFuncCall(FuncCall * fc)
{
  std::set<FuncCall*>::iterator vec_fc_i;
  
  for (vec_fc_i = funcCalls.begin(); vec_fc_i != funcCalls.end(); vec_fc_i++) {
		if ((*vec_fc_i)->funcName == fc->funcName && (*vec_fc_i)->paramNumber == fc->paramNumber)
		{
			return;
		}
	}
	
	//std::cout<<"Pushing back call to "<<fc->funcName<<" param "<<fc->paramNumber<<" for node "<<name<<std::endl;
  funcCalls.insert(fc);
}*/


// This function examines vertices that have no incoming edges, 
//   but have at least out outgoing edge
void FunctionBFC::determineBFCForVertexLite(NodeProps *v)
{
	
#ifdef DEBUG_EXIT
	blame_info<<"EV__(determineBFCForVertex) -- for "<<v->name<<std::endl;
#endif 	
	
	int v_index = v->number;
	int in_d = in_degree(v_index, G);
	int out_d = out_degree(v_index, G);
	
	// NO E: UNREAD RETURNS
	// We check to see if this node is simply an unread return value
	int returnVal = checkForUnreadReturn(v, v_index);
	if (returnVal && !(v->isGlobal)) {
#ifdef DEBUG_EXIT
		blame_info<<"EXIT__(determineBFCForVertexLite) -- Program Exit (Unread Return) added for "<<v->name<<std::endl;
#endif
		//addExitProg(new ExitProgram(v->name, UNREAD_RET));
		//v->exitStatus = EXIT_PROG;
		return;
	}
	
	// EV: GLOBALS
	// By being in this function we already know that the return value was 
    // used in some form in this function (in/out degree of greater than 1) 
    // so we assign it an exit value
	if (v->isGlobal) {
#ifdef DEBUG_EXIT
        blame_info<<v->name<<" isGlobal !"<<std::endl;
#endif
		v->eStatus = EXIT_VAR_GLOBAL;
    }
	
	
	// EV:  RETURN VALUES
    // This sees if the LLVM exit variable (no incoming edges, 1+ outgoing edges) 
    // matches a previously discovered return variable (DEFAULT_RET)
    if (v->name.find("retval") != std::string::npos ) {
		for (std::vector<ExitVariable *>::iterator ev_i = exitVariables.begin(); 
				 ev_i != exitVariables.end();  ev_i++) {
			if ((*ev_i)->realName.compare("DEFAULT_RET") == 0) {
				(*ev_i)->addVertex(v);
				(*ev_i)->vertex = v;
				v->eStatus = EXIT_VAR_RETURN;
				return;
			}
		}
		
		// This would occur in a case where our prototype analyzer showed a void
		// return value and LLVM still had a return value to go with it
#ifdef DEBUG_ERROR
		std::cerr<<"Where is the return exit variable?  "<<v->name<<"\n";
#endif
	}
	
	
	// EV:  GENERAL CASE (PARAMS)
    // Looking for an exact match to the predetermined exit variables or 
    //   a match for the pointsTo NodeProps in the case of a GEP node
    for (std::vector<ExitVariable *>::iterator ev_i = exitVariables.begin(); 
        ev_i != exitVariables.end();  ev_i++) {
		// compare()==0 means two strings are equal
		if (v->name.compare((*ev_i)->realName)==0 && v->isWritten) {
			(*ev_i)->addVertex(v);
			(*ev_i)->vertex = v;
#ifdef DEBUG_EXIT 
			blame_info<<"EV__(LLVMtoExitVars2) -- Add node for "<<v->name<<std::endl;
#endif 
			v->eStatus = EXIT_VAR_PARAM + (*ev_i)->whichParam;
			v->paramNum = (*ev_i)->whichParam;
			return;
		}

		else if (v->name.compare((*ev_i)->realName) == 0 && (*ev_i)->et == PARAM) { //PARAM is the ExitType
			(*ev_i)->vertex = v;
			v->eStatus = EXIT_VAR_UNWRITTEN;
			v->paramNum = (*ev_i)->whichParam;
		}
		
		/*
		 if (v->pointsTo != NULL)
		 {
		 if (v->pointsTo->name.compare( (*ev_i)->realName ) == 0)
		 {
		 #ifdef DEBUG_EXIT
		 blame_info<<"EV__(LLVMtoExitVars2) -- Add node for "<<v->name<<"which points to "<<v->pointsTo->name<<std::endl;
		 #endif
		 (*ev_i)->addVertex(v);
		 v->exitStatus = EXIT_VAR_PTR;
		 return;
		 }
		 }
		 */
	}
	
	// NO E: UNUSED PARAMETERS
    std::string::size_type unusedParam = v->name.find(PARAM_REC,0);
  // TODO: This doesn't cover local, unused variables with PARAM_REC in the name
    if (unusedParam != std::string::npos && v->llvm_inst != NULL) {
		if (isa<Instruction>(v->llvm_inst)) {
			Instruction *l_i = cast<Instruction>(v->llvm_inst);	
			if (l_i->getOpcode() == Instruction::Alloca && in_d == 0 && out_d == 0){
#ifdef DEBUG_EXIT
	            blame_info<<"EXIT__(LLVMtoExitVars2) -- N - Unused Parameter "<<v->name<<std::endl;
#endif
	            return;
	        }   
		    else {
#ifdef DEBUG_EXIT
	            blame_info<<"T - who knows(1)  "<<v->name<<std::endl;
#endif
	            return;
	        }
		}
	}
}


void FunctionBFC::determineBFCForOutputVertexLite(NodeProps *v, int v_index)
{
#ifdef DEBUG_EXIT_OUT
	blame_info<<"OUT__(determineBFCForOutputVertex) - Entering func for "<<v->name<<std::endl;
#endif
	std::set<FuncCall *>::iterator fc_i = v->funcCalls.begin();
	
	for (; fc_i != v->funcCalls.end(); fc_i++) {
		FuncCall *fc = *fc_i;
		if (fc->funcName == v->name) 
			fc->outputEvaluated = true;
	}
	
	std::set<int> inputVertices;
	int zeroParam = -1;
	NodeProps *zParam = NULL;
	
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
    bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
	boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
	
	e_beg = boost::in_edges(v_index, G).first;		// edge iterator begin
	e_end = boost::in_edges(v_index, G).second;    // edge iterator end
	
	// iterate through the edges to find matching opcode
	for(; e_beg != e_end; ++e_beg) {
		int opCode = get(get(edge_iore, G),*e_beg);
		
		if (opCode == Instruction::Call) {
			int sourceV = get(get(vertex_index, G), source(*e_beg, G));
			//int targetV = get(get(vertex_index, G), target(*e_beg, G));
			
			NodeProps *sourceVP = get(get(vertex_props, G), source(*e_beg, G));
			NodeProps *targetVP = get(get(vertex_props, G), target(*e_beg, G));
			
			int paramNum = MAX_PARAMS + 1;
#ifdef DEBUG_EXIT_OUT
			blame_info<<"OUT__(determineBFCForOutputVertexLite) - Call from "<<sourceVP->name<<" to "<<targetVP->name<<std::endl;
#endif
			//std::vector<FuncCall *>::iterator 
			fc_i = sourceVP->funcCalls.begin();
			
			for (; fc_i != sourceVP->funcCalls.end(); fc_i++) {
				FuncCall *fc = *fc_i;
#ifdef DEBUG_EXIT_OUT
				blame_info<<"OUT__(determineBFCForOutputVertexLite) - FC -- "<<fc->funcName<<"  "<<targetVP->name<<std::endl;
#endif	
				if (fc->funcName == targetVP->name) {
					fc->outputEvaluated = true;
					paramNum = fc->paramNumber;
					if (paramNum == -1) { //changed by Hui 12/31/15: 0=>-1
						zeroParam = sourceVP->number;
						zParam = sourceVP;
#ifdef DEBUG_EXIT_OUT
						blame_info<<"Inputs to "<<v->name<<" is "<<sourceVP->name<<" "<<paramNum<<std::endl;
#endif
					}
					else if (paramNum >= 0) {//changed by Hui 12/31/15:'>' => '>='
						inputVertices.insert(sourceV);
#ifdef DEBUG_EXIT_OUT
						blame_info<<"Inputs to "<<v->name<<" is "<<sourceVP->name<<" "<<paramNum<<std::endl;
#endif
					}
					break;
				}						
			}
		}
	}			
	
	if (zeroParam > -1) {
#ifdef DEBUG_EXIT_OUT
		blame_info<<"OUT__(determineBFCForOutputVertexLite) - Removing edge to "<<v->name<<" from "<<zeroParam<<std::endl;
#endif
		int in_d = in_degree(zeroParam, G);
		if (in_d == 0)
			remove_edge(zeroParam, v->number, G);
		zParam->nStatus[CALL_RETURN] = false;	
	}
	
	std::set<int>::iterator set_i;
	for (set_i = inputVertices.begin(); set_i != inputVertices.end();  set_i++) {
		remove_edge(*set_i, v->number, G);
		tie(ed, inserted) = add_edge(v->number, *set_i, G);
		
		if (inserted) {
			edge_type[ed] = RESOLVED_OUTPUT_OP;	
			NodeProps *dNode = get(get(vertex_props, G), *set_i);
			if (dNode->eStatus == EXIT_PROG)
				dNode->eStatus = NO_EXIT;
			
			//NodeProps * blamedV = get(get(vertex_props, G), blamed.second);
			int remainingCalls = 0;
			//We can reassign call statuses since these calls have now been resolved
			boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
			oe_beg = boost::out_edges(*set_i, G).first;		// edge iterator begin
			oe_end = boost::out_edges(*set_i, G).second;       // edge iterator end
			for(; oe_beg != oe_end; ++oe_beg) {
				int opCode = get(get(edge_iore, G),*oe_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
			ie_beg = boost::in_edges(*set_i, G).first;		// edge iterator begin
			ie_end = boost::in_edges(*set_i, G).second;       // edge iterator end
			for(; ie_beg != ie_end; ++ie_beg) {
				int opCode = get(get(edge_iore, G),*ie_beg);
				if (opCode == Instruction::Call)
					remainingCalls++;
			}
			
			if (!remainingCalls) {
				dNode->nStatus[CALL_PARAM] = false;
				dNode->nStatus[CALL_RETURN] = false;
				dNode->nStatus[CALL_NODE] = false;
			}
		}	
	}		
	
	LLVMtoOutputVar(v);
	
}



bool FunctionBFC::isLibraryOutput(const char * tStr)
{
	if ( (strcmp("printf",tStr) == 0) ||
			(strcmp("fprintf",tStr) == 0) ||
			(strcmp("vfprintf",tStr) == 0) ||
			(strcmp("vprintf",tStr) == 0) ||
			(strcmp("fflush",tStr) == 0) ||
			(strcmp("fputc",tStr) == 0) ||
			(strcmp("fputs",tStr) == 0) ||
			(strcmp("putc",tStr) == 0) ||
			(strcmp("putchar",tStr) == 0) ||
			(strcmp("puts",tStr) == 0) ||
			(strcmp("fwrite",tStr) == 0) ||
			(strcmp("fprintf",tStr) == 0) ||
			(strcmp("fflush",tStr) == 0) ||
			(strcmp("perror",tStr) == 0) || 
            (strcmp("write",tStr) == 0) || //adde by Hui 12/31/15 for chpl
            (strcmp("writeln",tStr) == 0)) //added by Hui 12/31/15 for chpl
	{		
		return true;
	}
	
	/*
	 if ( (strcmp("_gfortran_st_write",tStr) == 0) ||
	 (strcmp("_gfortran_st_read",tStr) == 0))
	 {		
	 return true;
	 }
	 */
	
	
	
	return false;
	
}


void FunctionBFC::determineBFCHoldersLite()
{
    // Iterate through all vertices and determine those that have no incoming edges (blame candidates)
    // but also have some out_degrees (eliminating outlier symbols)
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		int v_index = get(get(vertex_index, G),*i);
		NodeProps *v = get(get(vertex_props, G),*i);
		int in_d = in_degree(v_index, G);
		int out_d = out_degree(v_index, G);
		
		if (v->deleted)
			continue;
		
		if (v->isLocalVar)
			v->nStatus[LOCAL_VAR] = true;
		
		if (in_d == 0){
			if (out_d > 0 || v->isLocalVar)
	            determineBFCForVertexLite(v);
		}
		else if (v->name.find(PARAM_REC) != std::string::npos) {
			// We always check the parameters, the EV sanity check will kick in ... hopefully
			// TODO: verify this
#ifdef DEBUG_EXIT
			blame_info<<"EXIT__(determineBFCHolders) for PARAM_REC with in_d > 0 "<<v->name<<std::endl;
#endif
			determineBFCForVertexLite(v);
		}
		else if (v->isGlobal && (in_d > 0 || out_d > 0)) {
#ifdef DEBUG_EXIT
			blame_info<<"EXIT__(determineBFCHolder) for GLOBAL with in_d || out_d > 0 "<<v->name<<std::endl;
#endif
			determineBFCForVertexLite(v);
		}
		else if (v->name.find("retval") != std::string::npos) {
#ifdef DEBUG_EXIT
			blame_info<<"EXIT__(determineBFCHolder) for return val with in_d || out_d > 0 "<<v->name<<std::endl;
#endif
			determineBFCForVertexLite(v);	
		}
	}
	
	// We make a second pass through the vertices to handle 
	// cases for output, we don't want it to interfere
	// with our normal pass, which is why we do a separate
	// pass
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		int v_index = get(get(vertex_index, G),*i);
		NodeProps *v = get(get(vertex_props, G),*i);
		
		if (v == NULL) {
#ifdef DEBUG_ERROR
			blame_info<<"ERROR__(DBHLite) - V is null\n"<<std::endl;
			std::cerr<<"V is null in determineBFCHoldersLite"<<std::endl;
#endif
			continue;
		}	
		
		if (v->name.c_str() == NULL)
			//std::cerr<<"V has no name"<<std::endl;
			continue;
		
		const char *tStr = getTruncStr(v->name.c_str());
		//if (v->name.find("printf--") != std::string::npos)
		
		if (tStr == NULL)
			continue;
		
		//std::cout<<tStr<<std::endl;
		
		if (isLibraryOutput(tStr))
			determineBFCForOutputVertexLite(v, v_index);
		
	}
}

void FunctionBFC::identifyExternCalls(ExternFuncBFCHash &efInfo)
{
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	
    // We need to examine all of the vertices to see those that are a function call
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps *v = get(get(vertex_props, G),*i);
		
		if (v->deleted == true)
			continue;
		
		if (v->funcCalls.size() > 0 && v->nStatus[CALL_NODE]) { //second cond added by Hui 04/11/16: we only check call node for obvious reason
#ifdef DEBUG_GRAPH_BUILD
			blame_info<<"Graph__(identifyExternCalls) - Vertex "<<v->name<<" involved"<<std::endl;
#endif
			std::set<FuncCall *>::iterator fc_i = v->funcCalls.begin();
			
			#ifdef DEBUG_GRAPH_BUILD
			for (; fc_i != v->funcCalls.end(); fc_i++) {
				FuncCall *fc = *fc_i;	
				blame_info<<"Graph__(identifyExternCalls) - Func -  "<<fc->funcName<<" Param - "<<fc->paramNumber<<std::endl;
			}
			#endif
		
			ExternFunctionBFC *efb = NULL;
			const char *tStr = getTruncStr(v->name.c_str()); //v has to be the CALL_NODE, so that it has the func name
#ifdef TRIM_NUMBERED_STR
            //std::cout<<"extern func call before trimed: "<<tStr<<endl;
			if (tStr == NULL)
				continue;
            char *trimedStr = trimTruncStr(tStr); //remove the number indices on the tail
			blame_info<<"extern func call after trimed: "<<trimedStr<<endl;

            if (trimedStr == NULL)
				continue;
			else {
                //we make a special case for this* funcs since in ChapelArray.c, it has
                //different function prototypes with different #params, most are 3 params
                if (strcmp(trimedStr, "this")==0) {
                  if (v->llvm_inst!=NULL && isa<Instruction>(v->llvm_inst)) {
                    Instruction *pi = cast<Instruction>(v->llvm_inst); 
                    if (isa<CallInst>(pi)) {
                      //getNumOperands returns #real params+1(callnode itself)
                      int numParams = pi->getNumOperands()-1;
                      char thisTag[2];
                      sprintf(thisTag, "%d", numParams);
                      strcat(trimedStr, thisTag);

                      blame_info<<"We met 'this': "<<trimedStr<<endl;
                    }
                  }
                }
                std::string tdS(trimedStr);       
				efb = efInfo[tdS];
            }
#else
			if (tStr == NULL)
				continue;
			else {
                std::string tS(tStr);
				efb = efInfo[tS];
            }
#endif
			if (efb != NULL)
				handleOneExternCall(efb,v);
		}				
	} //end of for all vertices
}


void FunctionBFC::resolvePointersHelper2(NodeProps *origV, int origPLevel, \
                            NodeProps *targetV, std::set<int> &visited, \
                            std::set<NodeProps *> &tempPointers, NodeProps *alias, NodeProps *previousTV)
{
#ifdef DEBUG_RP
	blame_info<<"In resolvePointersHelper for "<<targetV->name<<" oV - "<<origV->name;
	blame_info<<" OPL "<<origPLevel<<" via previousTV - "<<previousTV->name<<std::endl;
#endif 
	int newPtrLevel = 0;
	const llvm::Type * origT = 0;		
	if (targetV->llvm_inst != NULL) {
		if (isa<Instruction>(targetV->llvm_inst)) {
			Instruction *pi = cast<Instruction>(targetV->llvm_inst);	
			origT = pi->getType();	
			newPtrLevel = pointerLevel(origT,0);
		}
		else if (isa<ConstantExpr>(targetV->llvm_inst)) {
			ConstantExpr *ce = cast<ConstantExpr>(targetV->llvm_inst);
			origT = ce->getType();
			newPtrLevel = pointerLevel(origT, 0);
		}
	}
	
	if (targetV->isExternCallParam) {
		if (targetV->isWritten)
			origV->isWritten = true;
	}
	
	if (visited.count(targetV->number) > 0)
		return;

	visited.insert(targetV->number);
#ifdef DEBUG_RP
	blame_info<<"Ptr Level(targetV) - "<<newPtrLevel<<" "<<targetV->name<<std::endl;
#endif
	
	// Not interested anymore, we're out of pointer territory
	if (newPtrLevel == 0)//&& targetV->storeLines.size() == 0)
		return;
	else if (newPtrLevel <= origPLevel) {// still in the game
#ifdef DEBUG_RP
		blame_info<<"In inner loop for "<<targetV->name<<std::endl;
#endif 
	
		boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
		e_beg = boost::in_edges(targetV->number, G).first;	//edge iterator begin
		e_end = boost::in_edges(targetV->number, G).second; // edge iterator end
		
		// iterate through the edges trying to find relationship between pointers
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
			NodeProps *tV = get(get(vertex_props,G), source(*e_beg,G));
			
#ifdef DEBUG_RP
			blame_info<<"In Edges for "<<targetV->name<<"  "<<opCode<<std::endl;
#endif 
			
			int tVPtrLevel = 0;
			const llvm::Type * origT2;		
            int tVPointedTypeID = -100; //for whatever reason it's not calculated, then it's invalid
			if (tV->llvm_inst != NULL) {
				if (isa<Instruction>(tV->llvm_inst)) {
					Instruction * pi = cast<Instruction>(tV->llvm_inst);	
					origT2 = pi->getType();	
					tVPtrLevel = pointerLevel(origT2,0);
                    tVPointedTypeID = getPointedTypeID(origT2);
				}
				else if (isa<ConstantExpr>(tV->llvm_inst)) {
					ConstantExpr *ce = cast<ConstantExpr>(tV->llvm_inst);
					origT2 = ce->getType();
					tVPtrLevel = pointerLevel(origT2, 0);
                    tVPointedTypeID = getPointedTypeID(origT2);
				}
			}			
			
			if (opCode == Instruction::Store) {	//*targetV = load **origV;
				// treat as alias               //store *targetV **tV;
#ifdef DEBUG_RP				
				blame_info<<"RHP Store between "<<origV->name<<" and "<<targetV->name<<"the actual destination of Store is "<<tV->name<<std::endl;
				blame_info<<tVPtrLevel<<" "<<origPLevel<<" "<<origV->resolvedLSFrom.size();
				blame_info<<" "<<targetV->resolvedLSFrom.size()<<std::endl;
#endif
                //---added by Hui 04/08/16-------------------------------//
                const llvm::Type *origT0;
                int origPointedTypeID = -200; //if for whatever reason it's not calculated, then it's invalid, should NOT = -100 !!!

                if (origV->llvm_inst != NULL) {
                    if (isa<Instruction>(origV->llvm_inst)) {
                        llvm::Instruction *origVpi = cast<Instruction>(origV->llvm_inst);	
                        origT0 = origVpi->getType();	
                        origPointedTypeID = getPointedTypeID(origT0);
                    }
                    else if (isa<ConstantExpr>(origV->llvm_inst)) {
                        llvm::ConstantExpr *origVce = cast<ConstantExpr>(origV->llvm_inst);
                        origT0 = origVce->getType();
                        origPointedTypeID = getPointedTypeID(origT0);
                    }
                    else if (isa<ConstantPointerNull> (origV->llvm_inst)) { //added by Hui 04/19/16, for origV is global var case
#ifdef DEBUG_RP
                        blame_info<<"origV("<<origV->name<<")"<<"  -- ConstantPointerNullVal"<<std::endl;
#endif
                        ConstantPointerNull *origVcpn = cast<ConstantPointerNull>(origV->llvm_inst);
                        origT0 = origVcpn->getType();
                        origPointedTypeID = getPointedTypeID(origT0);		
                        //origPointerLevel++; //TC: WHY ++ ?
                    }
                }			


#ifdef DEBUG_RP
                blame_info<<"The tV's pointedTypeID="<<tVPointedTypeID<<", origV's pointedTypeID="<<origPointedTypeID<<std::endl;
#endif
                //For cases: previousTV = GEP/LOAD/LS origV; targetV = GEP previousTV; store targetV, tV;
                // origV and tV should never treated as aliases, even if pointedTypeID and ptrLevel are equal
                bool existed;
                graph_traits <MyGraphType>::edge_descriptor Edge;
                tie(Edge, existed) = edge(targetV->number, previousTV->number, G);
                // edge_type != GEP_BASE_OP added by Hui 04/26/16, for the above reason
                //*TypeID cond added by Hui 04/08/16:alias should be same ptr level with same Pointed type, eg: **double and **int aren't aliases 
				if (tVPtrLevel == origPLevel && origPointedTypeID == tVPointedTypeID
                        && existed && get(get(edge_iore, G), Edge) != GEP_BASE_OP) { 
#ifdef DEBUG_RP                                                         
    			    blame_info<<"Pointer levels are equal"<<std::endl;
#endif

					if (tV->storesTo.size() > 1){
						blame_info<<"Inserting almost alias(1) between "<<origV->name<<" and "<<tV->name<<std::endl;
						//origV->almostAlias.insert(targetV); //TODO: shouldn't it between tV and origV ?
						//targetV->almostAlias.insert(origV);
                        origV->almostAlias.insert(tV); //New added 01/28/16
                        tV->almostAlias.insert(origV); //New
#ifdef DEBUG_RP
						blame_info<<"Inserting pointer(6) "<<targetV->name<<std::endl;
#endif
						//tempPointers.insert(targetV);
                        tempPointers.insert(tV); //New
					}
					  else {
						blame_info<<"Inserting alias(1) out "<<tV->name<<" into "<<origV->name<<std::endl;
						origV->aliasesOut.insert(tV);
						blame_info<<"Inserting alias(2) in "<<origV->name<<" into "<<tV->name<<std::endl;
						tV->aliasesIn.insert(origV);
						
#ifdef DEBUG_RP
						blame_info<<"Inserting STORE ALIAS pointer(2) "<<tV->name<<std::endl;
#endif
						tempPointers.insert(tV);
					}
				}
				// treat as write within data space
				else { //how could it be not equal? like a=GEP b; store a, c; then b and c Ptrlevel could be different
				    #ifdef DEBUG_RP
					blame_info<<"Pointer levels are not equal"<<std::endl;
					#endif
					bool proceed = true;
					std::set<NodeProps *>::iterator vec_vp_i_in;
					
					for (vec_vp_i_in = origV->almostAlias.begin(); vec_vp_i_in != origV->almostAlias.end(); vec_vp_i_in++) {
					    #ifdef DEBUG_RP
						blame_info<<"dfAlias relation between "<<tV->name<<" and "<<(*vec_vp_i_in)->name<<std::endl;
					    #endif 
						tV->dfAliases.insert(*vec_vp_i_in);
						(*vec_vp_i_in)->dfaUpPtr = tV;
						
						// If it's returned than we find it as interesting as something written, even though it technically may not be
						if (tV->name.find("retval") != std::string::npos) {
							tV->isWritten = true;
							(*vec_vp_i_in)->isWritten = true;
						}
						//resolvePointersHelper2(origV, origPLevel, tV, visited, tempPointers, alias);
						proceed = false;
					}

					if (proceed) {
						origV->nonAliasStores.insert(tV);
						// if it's a local var, we'll already be examining it
						if (!tV->isLocalVar) {
							#ifdef DEBUG_RP
							blame_info<<"Not local var, call rpH2"<<std::endl;
							#endif 
						
							resolvePointersHelper2(origV, origPLevel, tV, visited, tempPointers, alias, targetV);
						}
						else {
						    #ifdef DEBUG_RP
							blame_info<<"Is local var, do something later I guess." <<std::endl;
							#endif
						}
					}
				}
			}
			else if (opCode == Instruction::Load) { //if a = load b, c = load a, then b.loads.insert(c);
				if (tVPtrLevel > 0)
					origV->loads.insert(tV);
				resolvePointersHelper2(origV, origPLevel, tV, visited, tempPointers, alias, targetV);
			}
			else if(opCode == GEP_BASE_OP) {
				std::string origTStr = returnTypeName(origT, std::string(" "));
				// Dealing with a struct pointer
				if (origTStr.find("Struct") != std::string::npos) {
					if (origV->llvm_inst != NULL) {
						boost::graph_traits<MyGraphType>::out_edge_iterator e_beg2, e_end2;
						
						e_beg2 = boost::out_edges(tV->number, G).first;//edge iter b
						e_end2 = boost::out_edges(tV->number, G).second;//iter end 
         // iterate through the edges trying to find relationship between pointers
						for(; e_beg2 != e_end2; ++e_beg2) {
							int opCodeForField = get(get(edge_iore, G),*e_beg2);
							int fNum = 0;
							if (opCodeForField >= GEP_S_FIELD_OFFSET_OP) {
								fNum = opCodeForField - GEP_S_FIELD_OFFSET_OP; 
								
								if (isa<Value>(origV->llvm_inst)) {
									Value *v = cast<Value>(origV->llvm_inst);	
#ifdef DEBUG_RP
									blame_info<<"Calling structResolve for "<<origV->name<<" of type "<<origTStr<<endl;
#endif
									structResolve(v, fNum, tV);
								}
							}
						}
					}
#ifdef DEBUG_RP
					blame_info<<"OV "<<origV->name<<" pushing back field "<<tV->name<<std::endl;
#endif
#ifdef DEBUG_RP
					blame_info<<"Transferring sBFC from "<<tV->name<<" to "<<origV->name<<std::endl;
#endif 
					if (tV->sField != NULL)
						origV->sBFC = tV->sField->parentStruct;
                    if(origV != tV){ //condition added by Hui 01/20/16
                        blame_info<<"Adding "<<tV->name<<" to "<<origV->name<<" as a field(1)"<<endl;
					    origV->fields.insert(tV);
					    tV->fieldUpPtr = origV;
                    }
				}
				else{ //not struct
					if (tVPtrLevel > 0) {
#ifdef DEBUG_RP
						blame_info<<"Adding GEP(1) "<<tV->name<<" "<<tV->isWritten<<" to load "<<origV->name<<std::endl;
#endif 
						origV->GEPs.insert(tV);
						
						if (alias && tV->isWritten) {
#ifdef DEBUG_RP
							blame_info<<"Adding "<<tV->name<<" to list of resolvedLSSideEffects for "<<alias->name<<std::endl;
#endif
							alias->resolvedLSSideEffects.insert(tV);
						}
					}
					
					// we'll use this in cases where 2 loads replace a GEP
					if (targetV != origV) { //targetV = load origV;  tV = GEP targetV ...
#ifdef DEBUG_RP
						blame_info<<"Adding GEP(2) "<<tV->name<<" to load "<<targetV->name<<std::endl;
#endif 
						targetV->GEPs.insert(tV);
					}
					resolvePointersHelper2(origV, origPLevel, tV, visited, tempPointers, alias, targetV);
				}
			}
			else if (opCode == RESOLVED_L_S_OP) {
				//std::cerr<<"Shouldn't hit this here!"<<std::endl;
#ifdef DEBUG_RP
				blame_info<<"R_L_S blame Source "<<tV->name<<" Dest "<<targetV->name<<std::endl;
#endif 
			}
		}
		
		boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
		o_beg = boost::out_edges(targetV->number, G).first;	// edge iterator begin
		o_end = boost::out_edges(targetV->number, G).second;// edge iterator end
		
		// iterate through the edges trying to find relationship between pointers
		for(; o_beg != o_end; ++o_beg) {
			int opCode = get(get(edge_iore, G),*o_beg);

			if (opCode == Instruction::Store) {	
#ifdef DEBUG_RP
				blame_info<<"Here(2) "<<newPtrLevel<<" "<<targetV->name<<std::endl;
				// We have a write to the data section of the pointer
				blame_info<<"Vertex "<<targetV->name<<" is written(5)"<<std::endl;
				blame_info<<"Vertex "<<origV->name<<" is written(6)"<<std::endl;
#endif
				origV->isWritten = true;
				targetV->isWritten = true;

                //added by Hui 03/22/16 for cases: %1 = load a;  store %2, %1  ///
                //then %2 should be counted to a's child
                NodeProps *storeValue = get(get(vertex_props,G), target(*o_beg,G));
                origV->children.insert(storeValue);
                storeValue->parents.insert(origV);
                //////////////////////////////////////////////////////////////////////
			}
		}
	}
	else { // What to do with cases like: targetV = GEP origV, targetV
	#ifdef DEBUG_RP
		blame_info<<"New pointer level is "<<newPtrLevel<<" and  old is "<<origPLevel<<std::endl;
		#endif
	
	}
}


short FunctionBFC::checkIfWritten2(NodeProps * currNode, std::set<int> & visited)
{
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<std::endl;
#endif 
	
	if (visited.count(currNode->number) > 0)
		return 0; //should it return currNode->isWritten ?
	visited.insert(currNode->number);
	
	short writeTotal = currNode->isWritten;
	set<NodeProps *>::iterator vec_vp_i;
	for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after aliasesOut, writeTotal="<<writeTotal<<std::endl;
#endif 

	for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after aliasesIn, writeTotal="<<writeTotal<<std::endl;
#endif 

	for (vec_vp_i = currNode->almostAlias.begin(); vec_vp_i != currNode->almostAlias.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after almostAlias, writeTotal="<<writeTotal<<std::endl;
#endif 
	
	for (vec_vp_i = currNode->resolvedLS.begin(); vec_vp_i != currNode->resolvedLS.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after resolvedLS, writeTotal="<<writeTotal<<std::endl;
#endif 
	
	for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after fields, writeTotal="<<writeTotal<<std::endl;
#endif 
	
	for (vec_vp_i = currNode->nonAliasStores.begin(); vec_vp_i != currNode->nonAliasStores.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after nonAliasStores, writeTotal="<<writeTotal<<std::endl;
#endif 
	
	for (vec_vp_i = currNode->arrayAccess.begin(); vec_vp_i != currNode->arrayAccess.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after arrayAccess, writeTotal="<<writeTotal<<std::endl;
#endif 
	
    //added by Hui 04/19/16: GEPs should be counted in as well
/*	for (vec_vp_i = currNode->GEPs.begin(); vec_vp_i != currNode->GEPs.end(); vec_vp_i++)
	{
		writeTotal += checkIfWritten2((*vec_vp_i), visited);
	}
*/
	// We consider any param passed into a function as written, acting conservatively
	//  At runtime through transfer functions we can figure out if this is actually the case
	for (vec_vp_i = currNode->loads.begin(); vec_vp_i != currNode->loads.end(); vec_vp_i++)
	{
		NodeProps * v = (*vec_vp_i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR		
			std::cerr<<"Null V in checkIfWritten\n";
#endif
			continue;
		}
		
		boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
		
		e_beg = boost::out_edges(v->number, G).first;		// edge iterator begin
		e_end = boost::out_edges(v->number, G).second;    // edge iterator end
		
		// iterate through the edges to find matching opcode
		for(; e_beg != e_end; ++e_beg) 
		{
			int opCode = get(get(edge_iore, G),*e_beg);
			
			if (opCode == Instruction::Call || opCode == Instruction::Invoke)
				writeTotal++;
		}
		
	}
#ifdef DEBUG_RP
	blame_info<<"In checkIfWritten for "<<currNode->name<<", after loads, writeTotal="<<writeTotal<<std::endl;
#endif 
	
	if (writeTotal > 0)
		currNode->isWritten = true;
	return writeTotal;
}


//At the very irst time this func was called, exitCand, currNode, exitV are all same
void FunctionBFC::resolveLocalAliases2(NodeProps * exitCand, NodeProps * currNode,
										std::set<int> & visited, NodeProps * exitV)
{
#ifdef DEBUG_RP
	blame_info<<"In resolveLocalAliases2 for "<<exitCand->name<<"("<<exitCand->eStatus<<")";
	blame_info<<" "<<currNode->name<<"("<<currNode->eStatus<<")"<<std::endl;
#endif
	
	if (visited.count(currNode->number) > 0)
		return;
	visited.insert(currNode->number);
	
	set<NodeProps *>::iterator vec_vp_i;
	
	if (currNode->nStatus[LOCAL_VAR] || currNode->nStatus[LOCAL_VAR_ALIAS])
	{
		for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP AO - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			//if (vp->exitStatus == NO_EXIT)
			//{
			vp->nStatus[LOCAL_VAR_ALIAS] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(1) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			//}
			
			if (vp->nStatus[EXIT_VAR])
				return;
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting alias(3) straight up in "<<vp->name<<" into "<<exitCand->name<<std::endl;
			#endif 

			exitCand->aliases.insert(vp);
			// FORTRAN
			//vp->aliasesIn.insert(exitCand);
			
			resolveLocalAliases2(exitCand, vp, visited, exitV);
		}
		
		for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP AI - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			
			//if (vp->exitStatus == NO_EXIT)
			//{
			vp->nStatus[LOCAL_VAR_ALIAS] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(2) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			//}
			
            //CHECK: WHY return if vp is exit var ?  Hui 02/04/16
			/*if (vp->nStatus[EXIT_VAR])
                blame_info<<"Because vp: "<<vp->name<<" is EXIT_VAR, so we don't add alias between it and "<<exitCand->name<<endl;
				return;
			*/
			exitCand->aliases.insert(vp);
			// FORTRAN
			//vp->aliasesIn.insert(exitCand);
			//HUI_CHECK
            blame_info<<"rLA2 called in here(1)"<<endl;
			resolveLocalAliases2(exitCand, vp, visited, exitV);
		}
		
		
		for (vec_vp_i = currNode->almostAlias.begin(); vec_vp_i != currNode->almostAlias.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			
			vp->nStatus[LOCAL_VAR_A_ALIAS] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(3) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;					
			}
			 
			 #ifdef DEBUG_RP
			 blame_info<<"Inserting dfAlias(4) "<<vp->name<<"into set for "<<exitCand->name<<std::endl;
			 #endif 

			exitCand->dfAliases.insert(vp);
			vp->dfaUpPtr = exitCand;
			
			//resolveAliases2(exitCand, vp, visited, exitV);
			
			resolveLocalAliases2(exitCand, vp, visited, exitV);
		}
	}
	
	for (vec_vp_i = currNode->nonAliasStores.begin(); vec_vp_i != currNode->nonAliasStores.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		//blame_info<<"VP NAS - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
		//if (vp->exitStatus == NO_EXIT)
		vp->nStatus[LOCAL_VAR_PTR] = true;
		
		if (vp->pointsTo == NULL)
			vp->pointsTo = currNode;
		
		if (vp->exitV == NULL)
		{
#ifdef DEBUG_RP
			blame_info<<"PTRS__(resolveLocalAliases)(4) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
			vp->exitV = exitV;
		}
		
		vp->dpUpPtr = currNode;
		
        //Hui dataPtrs.insert test 12/04/15
        blame_info<<"dataPtrs.insert 1: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
		currNode->dataPtrs.insert(vp);
		resolveLocalAliases2(exitCand, vp, visited, exitV);
	}
	
	if (currNode->nStatus[LOCAL_VAR_FIELD] || currNode->nStatus[LOCAL_VAR_FIELD_ALIAS])
	{
		for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP AO(LVF) - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_FIELD_ALIAS] = true;
			//else 
			//return;
			
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(5) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting alias(4) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
			#endif 
			exitCand->aliases.insert(vp);
			// FORTRAN
			//vp->aliases.insert(exitCand);
			
			
			if (currNode->nStatus[LOCAL_VAR_FIELD])
				resolveLocalAliases2(currNode, vp, visited, exitV);
			else if (currNode->nStatus[LOCAL_VAR_FIELD_ALIAS])
				resolveLocalAliases2(exitCand, vp, visited, exitV);
			
		}
		
		for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP AI(LVF) - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_FIELD_ALIAS] = true;
			//else
			//return;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(6) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting alias(5) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
			#endif
			
			exitCand->aliases.insert(vp);
			// FORTRAN
			//vp->aliases.insert(exitCand);
			
			if (currNode->nStatus[LOCAL_VAR_FIELD])
				resolveLocalAliases2(currNode, vp, visited, exitV);
			else if (currNode->nStatus[LOCAL_VAR_FIELD_ALIAS])
				resolveLocalAliases2(exitCand, vp, visited, exitV);
			
		}
		
	}
	
	if (currNode->nStatus[LOCAL_VAR] || currNode->nStatus[LOCAL_VAR_FIELD] ||
			currNode->nStatus[LOCAL_VAR_FIELD_ALIAS] )
	{
		for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP F - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_FIELD] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(7) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			//HUI_CHECK
            blame_info<<"rLA2 called in here(2)"<<endl;
			resolveLocalAliases2(vp, vp, visited, exitV);
		}
	}
	
	if (currNode->nStatus[LOCAL_VAR_ALIAS])
	{
		for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			//blame_info<<"VP F(LVA) - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
			
			
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_FIELD_ALIAS] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = exitCand;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(8) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			//HUI_CHECK
            blame_info<<"rLA2 called in here(3)"<<endl;
			//resolveLocalAliases2(currNode, vp, visited, exitV);
            //changed by Hui 01/20/16 from above
            resolveLocalAliases2(vp, vp, visited, exitV);
		}
	}
	
	
	for (vec_vp_i = currNode->arrayAccess.begin(); vec_vp_i != currNode->arrayAccess.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
#ifdef DEBUG_RP
		blame_info<<"VP AA - "<<vp->name<<std::endl;
#endif
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_PTR] = true;
			//else 
			//return;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = currNode;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(9) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 2: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
		
	}
	
	
	for (vec_vp_i = currNode->GEPs.begin(); vec_vp_i != currNode->GEPs.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		//blame_info<<"VP GEP - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
		
		// TODO: 10/30/10 investigate the out_d > 1 thing
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_PTR] = true;
			//else 
			//return;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = currNode;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(10) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 3: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
		
	}
	
#ifdef DATAPTRS_FROM_FIELDS
	for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		//blame_info<<"VP GEP - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
		
		// TODO: 10/30/10 investigate the out_d > 1 thing
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_PTR] = true;
			//else 
			//return;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = currNode;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(10.2) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 3(fields): insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
		
	}
#endif
	
	for (vec_vp_i = currNode->resolvedLS.begin(); vec_vp_i != currNode->resolvedLS.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
#ifdef DEBUG_RP
		blame_info<<"RA (RLS_Local) - Current Node "<<currNode->name<<" Looking at "<<vp->name<<std::endl;
#endif 
		
		int out_d = out_degree(vp->number, G);
		if (out_d > 1 || vp->GEPs.size() == 0)
		{
#ifdef DEBUG_RP
			blame_info<<"RA (RLS_Local) - Adding "<<vp->name<<std::endl;
#endif 
			
			vp->nStatus[LOCAL_VAR_PTR] = true;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = currNode;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(11) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 4: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
			
		}
	}
	
	
	
	for (vec_vp_i = currNode->loads.begin(); vec_vp_i != currNode->loads.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		
		//blame_info<<"VP L - "<<vp->name<<"("<<vp->exitStatus<<")"<<std::endl;
		
		
		int out_d = out_degree(vp->number, G);
		if (out_d > 1 || vp->GEPs.size() == 0)
		{
			//if (vp->exitStatus == NO_EXIT)
			vp->nStatus[LOCAL_VAR_PTR] = true;
			//else
			//return;
			
			if (vp->pointsTo == NULL)
				vp->pointsTo = currNode;
			
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveLocalAliases)(11) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 5: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
	}
#ifdef DEBUG_RP
	blame_info<<"Finishing up resolveLocalAliases for "<<exitCand->name<<" "<<currNode->name<<std::endl;
#endif 
}



void FunctionBFC::resolveAliases2(NodeProps * exitCand, NodeProps * currNode,
									std::set<int> & visited, NodeProps * exitV)
{
#ifdef DEBUG_RP
	blame_info<<"In resolveAliases for "<<exitCand->name<<"("<<exitCand->eStatus<<")";
	blame_info<<" "<<currNode->name<<"("<<currNode->eStatus<<")"<<std::endl;
	
	blame_info<<"Node Props for curr node "<<currNode->name<<": ";
	for (int a = 0; a < NODE_PROPS_SIZE; a++)
		blame_info<<currNode->nStatus[a]<<" ";

	blame_info<<std::endl;
#endif 	
	
	if (visited.count(currNode->number) > 0)
		return;
	visited.insert(currNode->number);
	
	set<NodeProps *>::iterator vec_vp_i;
	
	if (currNode->nStatus[EXIT_VAR] )
	{
		// Looking at the aliases in
		for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(1) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(1) - exitV "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 

	
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(2) - exitV "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 

				blame_info<<"Inserting alias(60) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
				exitCand->aliases.insert(vp);

				continue;
			}
			
			
			blame_info<<"Inserting alias(6) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
			exitCand->aliases.insert(vp);
		
			resolveAliases2(exitCand, vp, visited, exitV);
		}
		
		// Looking at the aliases out
		for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(2) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(3) - exitV "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 

				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(4) - exitV "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 

#ifdef DEBUG_RP
				blame_info<<"Inserting alias(61) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
#endif 				
				exitCand->aliases.insert(vp);

				continue;
			}
			
			#ifdef DEBUG_RP
						blame_info<<"Inserting alias(7) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
			#endif 			

			exitCand->aliases.insert(vp);
			// FORTRAN
			//vp->aliases.insert(exitCand);
			
			resolveAliases2(exitCand, vp, visited, exitV);
		}
		
		for (vec_vp_i = currNode->almostAlias.begin(); vec_vp_i != currNode->almostAlias.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			
			vp->nStatus[EXIT_VAR_A_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(3) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(5) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(6) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting dfAlias(1) "<<vp->name<<"into set for "<<exitCand->name<<std::endl;
			#endif 

			exitCand->dfAliases.insert(vp);
			vp->dfaUpPtr = exitCand;
			
			
			resolveAliases2(exitCand, vp, visited, exitV);
			
		}
	}
	
	if ( currNode->nStatus[EXIT_VAR_ALIAS])
	{
		// Looking at the aliases in
		for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			if (!vp->nStatus[EXIT_VAR])
			{
				vp->nStatus[EXIT_VAR_ALIAS] = true;
				vp->pointsTo = exitCand;
				if (vp->exitV == NULL)
				{
#ifdef DEBUG_RP
					blame_info<<"PTRS__(resolveAliases)(4) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
					vp->exitV = exitV;
				}
				else if (vp->exitV == exitV)
				{
#ifdef DEBUG_RP
					blame_info<<"PTRS__(resolveAliases)(7) "<<exitV->name<<" already there for ";
					blame_info<<vp->name<<std::endl;
#endif 
					continue;
				}	
				else
				{
#ifdef DEBUG_ERROR
					blame_info<<"PTRS__(resolveAliases)(8) "<<exitV->name<<" already there  (";
					blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
					std::cerr<<"exitV already taken"<<std::endl;
#endif 
					continue;
				}
				
				#ifdef DEBUG_RP
							blame_info<<"Inserting alias(8) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
				#endif 

				exitCand->aliases.insert(vp);
				
				// FORTRAN
				//vp->aliases.insert(exitCand);
				resolveAliases2(exitCand, vp, visited, exitV);
			}
		}
		
		// Looking at the aliases out
		for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			if (!vp->nStatus[EXIT_VAR])
			{
				vp->nStatus[EXIT_VAR_ALIAS] = true;
				vp->pointsTo = exitCand;
				if (vp->exitV == NULL)
				{
#ifdef DEBUG_RP
					blame_info<<"PTRS__(resolveAliases)(5) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
					vp->exitV = exitV;
				}
				else if (vp->exitV == exitV)
				{
#ifdef DEBUG_RP
					blame_info<<"PTRS__(resolveAliases)(9) "<<exitV->name<<" already there for ";
					blame_info<<vp->name<<std::endl;
#endif 
					continue;
				}	
				else
				{
#ifdef DEBUG_ERROR
					blame_info<<"PTRS__(resolveAliases)(10) "<<exitV->name<<" already there  (";
					blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
					std::cerr<<"exitV already taken"<<std::endl;
#endif 
					continue;
				}
				
				#ifdef DEBUG_RP
							blame_info<<"Inserting alias(9) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
				#endif 

				exitCand->aliases.insert(vp);
				
				// FORTRAN
				//vp->aliases.insert(exitCand);
				resolveAliases2(exitCand, vp, visited, exitV);
			}
		}
		
		for (vec_vp_i = currNode->almostAlias.begin(); vec_vp_i != currNode->almostAlias.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			
			vp->nStatus[EXIT_VAR_A_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(6) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(11) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(12) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting dfAlias(2) "<<vp->name<<"into set for "<<exitCand->name<<std::endl;
			#endif 			
						
			exitCand->dfAliases.insert(vp);
			vp->dfaUpPtr = exitCand;
			
			resolveAliases2(exitCand, vp, visited, exitV);
			
		}
		
	}
	
	for (vec_vp_i = currNode->nonAliasStores.begin(); vec_vp_i != currNode->nonAliasStores.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		
		
		vp->nStatus[EXIT_VAR_PTR] = true;
		vp->pointsTo = currNode;
		if (vp->exitV == NULL)
		{
#ifdef DEBUG_RP
			blame_info<<"PTRS__(resolveAliases)(7) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
			vp->exitV = exitV;
		}
		else if (vp->exitV == exitV)
		{
#ifdef DEBUG_RP
			blame_info<<"PTRS__(resolveAliases)(13) "<<exitV->name<<" already there for ";
			blame_info<<vp->name<<std::endl;
#endif 
			continue;
		}	
		else
		{
#ifdef DEBUG_ERROR
			blame_info<<"PTRS__(resolveAliases)(14) "<<exitV->name<<" already there  (";
			blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
			std::cerr<<"exitV already taken"<<std::endl;
#endif 
			continue;
		}
		
		vp->dpUpPtr = currNode;
		
        //Hui dataPtrs.insert test 12/04/15
        blame_info<<"dataPtrs.insert 6: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
		currNode->dataPtrs.insert(vp);
		resolveAliases2(exitCand, vp, visited, exitV);
	}
	
	if (currNode->nStatus[EXIT_VAR_FIELD] || currNode->nStatus[EXIT_VAR_FIELD_ALIAS])
	{
		for (vec_vp_i = currNode->aliasesOut.begin(); vec_vp_i != currNode->aliasesOut.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_FIELD_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(8) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(15) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(16) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif
				continue;
			}
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting alias(10) straight up "<<vp->name<<" into "<<currNode->name<<std::endl;
			#endif 			

			currNode->aliases.insert(vp);
			// FORTRAN
			//vp->aliases.insert(exitCand);
			
			//if (currNode->nStatus[EXIT_VAR_FIELD])
				resolveAliases2(currNode, vp, visited, exitV);
			//else if (currNode->nStatus[EXIT_VAR_FIELD_ALIAS])
				//resolveAliases2(exitCand, vp, visited, exitV);
			
		}
		
		for (vec_vp_i = currNode->aliasesIn.begin(); vec_vp_i != currNode->aliasesIn.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_FIELD_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(9) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(17) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(18) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			#ifdef DEBUG_RP
						blame_info<<"Inserting alias(11) straight up "<<vp->name<<" into "<<exitCand->name<<std::endl;
			#endif 			

			currNode->aliases.insert(vp);
			// FORTRAN
			//vp->aliases.insert(exitCand);
			
			//if (currNode->nStatus[EXIT_VAR_FIELD])
				resolveAliases2(currNode, vp, visited, exitV);
			//else if (currNode->nStatus[EXIT_VAR_FIELD_ALIAS])
				//resolveAliases2(exitCand, vp, visited, exitV);
			
		}
		
		
		for (vec_vp_i = currNode->almostAlias.begin(); vec_vp_i != currNode->almostAlias.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			
			
			//vp->nStatus[EXIT_VAR_A_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(3) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(5) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(6) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting dfAlias(1) "<<vp->name<<"into set for "<<exitCand->name<<std::endl;
			#endif 

			exitCand->dfAliases.insert(vp);
			vp->dfaUpPtr = exitCand;
			
			
			resolveAliases2(exitCand, vp, visited, exitV);
			
		}
		
	}
	
//	if (currNode->nStatus[EXIT_VAR] || currNode->nStatus[EXIT_VAR_FIELD] || \
			currNode->nStatus[EXIT_VAR_FIELD_ALIAS])
	if (currNode->nStatus[EXIT_VAR] || currNode->nStatus[EXIT_VAR_ALIAS] ||   //cond EXIT_VAR_ALIAS is added by Hui
            currNode->nStatus[EXIT_VAR_FIELD] || currNode->nStatus[EXIT_VAR_FIELD_ALIAS])  //02/08/16
	{
		for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_FIELD] = true;
            //TODO: change to be made by Hui 01/26/15 EVFA's field  should just be EVFA, not EVF
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(10) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(19) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR	
				blame_info<<"PTRS__(resolveAliases)(20) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
			}
			resolveAliases2(vp, vp, visited, exitV);
		}
	}
	
	if (currNode->nStatus[EXIT_VAR_ALIAS])
	{
		for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
		{
			NodeProps * vp = (*vec_vp_i);
			vp->nStatus[EXIT_VAR_FIELD_ALIAS] = true;
			vp->pointsTo = exitCand;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(11) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(21) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(22) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			resolveAliases2(currNode, vp, visited, exitV);
		}
		
	}
	
	
	for (vec_vp_i = currNode->GEPs.begin(); vec_vp_i != currNode->GEPs.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		
				// TODO: 10/30/10 investigate the out_d > 1 thing
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			vp->nStatus[EXIT_VAR_PTR] = true;
			vp->pointsTo = currNode;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(12) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(23) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(24) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 7: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
	}
	
#ifdef DATAPTRS_FROM_FIELDS2
	for (vec_vp_i = currNode->fields.begin(); vec_vp_i != currNode->fields.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		
				// TODO: 10/30/10 investigate the out_d > 1 thing
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			vp->nStatus[EXIT_VAR_PTR] = true;
			vp->pointsTo = currNode;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(12.2) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(23.2) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(24.2) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 7(fields): insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
	}
#endif
	
	
	for (vec_vp_i = currNode->arrayAccess.begin(); vec_vp_i != currNode->arrayAccess.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		int out_d = out_degree(vp->number, G);
		if (out_d > 1)
		{
			vp->nStatus[EXIT_VAR_PTR] = true;
			vp->pointsTo = currNode;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(13) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(25) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(26) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			vp->dpUpPtr = currNode;
			
            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 8: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
		
	}
	
	
	for (vec_vp_i = currNode->resolvedLS.begin(); vec_vp_i != currNode->resolvedLS.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
#ifdef DEBUG_RP
		blame_info<<"RA (RLS) - Current Node "<<currNode->name<<" Looking at "<<vp->name<<std::endl;
#endif 
		
		int out_d = out_degree(vp->number, G);
		if (out_d > 1 || vp->GEPs.size() == 0)
		{
#ifdef DEBUG_RP
			blame_info<<"RA (RLS) - Adding "<<vp->name<<std::endl;
#endif 
			
			vp->nStatus[EXIT_VAR_PTR] = true;
			vp->pointsTo = currNode;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(15) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(29) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(30) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			vp->dpUpPtr = currNode;

            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 9: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
	}
	
	
	for (vec_vp_i = currNode->loads.begin(); vec_vp_i != currNode->loads.end(); vec_vp_i++)
	{
		NodeProps * vp = (*vec_vp_i);
		int out_d = out_degree(vp->number, G);
		if (out_d > 1 || vp->GEPs.size() == 0)
		{
			
			
			vp->nStatus[EXIT_VAR_PTR] = true;
			vp->pointsTo = currNode;
			if (vp->exitV == NULL)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(14) - Assigning exitV "<<exitV->name<<" for "<<vp->name<<std::endl;
#endif 
				vp->exitV = exitV;
			}
			else if (vp->exitV == exitV)
			{
#ifdef DEBUG_RP
				blame_info<<"PTRS__(resolveAliases)(27) "<<exitV->name<<" already there for ";
				blame_info<<vp->name<<std::endl;
#endif 
				continue;
			}	
			else
			{
#ifdef DEBUG_ERROR
				blame_info<<"PTRS__(resolveAliases)(28) "<<exitV->name<<" already there  (";
				blame_info<<vp->exitV->name<<") for "<<vp->name<<std::endl;
				std::cerr<<"exitV already taken"<<std::endl;
#endif 
				continue;
			}
			
			vp->dpUpPtr = currNode;

            //Hui dataPtrs.insert test 12/04/15
            blame_info<<"dataPtrs.insert 10: insert to : "<<currNode->name<<" of: "<<vp->name<<std::endl;
			currNode->dataPtrs.insert(vp);
		}
	}
#ifdef DEBUG_RP
	blame_info<<"Finishing up resolveAliases for "<<exitCand->name<<" "<<currNode->name<<std::endl;
#endif 
}


void FunctionBFC::resolveArrays(NodeProps * origV, NodeProps * v, std::set<NodeProps *> & tempPointers)
{
	// At this point, we don't care if the pointer is written to or not,
	//   we'll worry about that later
	blame_info<<"In resolveArrays: Inserting pointer(4) "<<v->name<<std::endl;
	
	tempPointers.insert(v);
	
	boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
	
	e_beg = boost::in_edges(v->number, G).first;		// edge iterator begin
	e_end = boost::in_edges(v->number, G).second;    // edge iterator end
	
	
	// iterate through the edges trying to find a Store between pointers
	for(; e_beg != e_end; ++e_beg) 
	{
		int opCode = get(get(edge_iore, G),*e_beg);
		NodeProps * targetV = get(get(vertex_props,G), source(*e_beg,G));
		
		if (opCode == GEP_BASE_OP)
		{
			//v->aliasesIn.push_back(targetV);
            blame_info<<"Inserting arrayAccess for "<<origV->name<<" of "<<targetV->name<<"\n";
			origV->arrayAccess.insert(targetV);
			//blame_info<<"Inserting pointer(5) "<<targetV->name<<std::endl;
			
			tempPointers.insert(targetV);
			
			boost::graph_traits<MyGraphType>::out_edge_iterator e_beg2, e_end2;
			
			e_beg2 = boost::out_edges(targetV->number, G).first;		// edge iterator begin
			e_end2 = boost::out_edges(targetV->number, G).second;    // edge iterator end
			
			// iterate through the edges trying to find relationship between pointers
			for(; e_beg2 != e_end2; ++e_beg2) 
			{
				int opCodeForField = get(get(edge_iore, G),*e_beg2);
				//blame_info<<"Opcode "<<opCodeForField<<std::endl;
				//int fNum = 0;
				if (opCodeForField == Instruction::Store)
				{
#ifdef DEBUG_RP
					blame_info<<"Vertex "<<v->name<<" is written(1)"<<std::endl;
#endif 
					v->isWritten = true;
				}
			}
			
			resolveArrays(origV, targetV, tempPointers);
			
			//v->fields.push_back(targetV);
		}
		else if (opCode == Instruction::Store)
		{
			/*// treat as alias  why ? v and targetV have different ptr level
			#ifdef DEBUG_RP
			blame_info<<"RHP Store(2) between "<<v->name<<" and "<<targetV->name<<std::endl;
			#endif
				
			#ifdef DEBUG_RP
			blame_info<<"Inserting alias(12) out  "<<targetV->name<<" into "<<v->name<<std::endl;
			#endif 			
			v->aliasesOut.insert(targetV);
			targetV->aliasesIn.insert(v);
				
#ifdef DEBUG_RP
			blame_info<<"Inserting STORE ALIAS pointer(3) "<<targetV->name<<std::endl;
#endif
            */
			tempPointers.insert(targetV);
            
		}
	}
}

void FunctionBFC::resolvePointersForNode2(NodeProps *v, std::set<NodeProps *> &tempPointers)
{
	if (v->resolved == true)
		return;
	else
		v->resolved = true;
	
#ifdef DEBUG_RP
	blame_info<<std::endl;
	blame_info<<"In resolvePointersForNode for "<<v->name<<std::endl;
#endif 
	int origPointerLevel = 0, collapsedPointerLevel = 0;
	const llvm::Type *origT,  *collapsedT;		
	
	if (v->llvm_inst != NULL) {
	
	#ifdef DEBUG_RP
		blame_info<<"Value ID is "<<v->llvm_inst->getValueID()<<std::endl;
	#endif 	
		
		if (isa<Instruction>(v->llvm_inst)) {
	#ifdef DEBUG_RP
		blame_info<<v->name<<" is Instruction"<<std::endl;
	#endif 	
			Instruction *pi = cast<Instruction>(v->llvm_inst);	
			origT = pi->getType();	
			origPointerLevel = pointerLevel(origT,0);
		}
		else if (isa<ConstantExpr>(v->llvm_inst)) {
	#ifdef DEBUG_RP
		blame_info<<v->name<<" is ConstantExpr"<<std::endl;
	#endif 	
		    ConstantExpr *ce = cast<ConstantExpr>(v->llvm_inst);
			origT = ce->getType();
			origPointerLevel = pointerLevel(origT, 0);
		}
		else if (isa<ConstantAggregateZero> (v->llvm_inst)) {
	#ifdef DEBUG_RP
		blame_info<<v->name<<" is ConstantAggregateZero"<<std::endl;
	#endif 	
		    ConstantAggregateZero *caz = cast<ConstantAggregateZero>(v->llvm_inst);
			origT = caz->getType();
			origPointerLevel = pointerLevel(origT, 0);
		}
		else if (isa<ConstantArray> (v->llvm_inst)) {
	#ifdef DEBUG_RP
		blame_info<<v->name<<" is ConstantArray"<<std::endl;
	#endif 	
			ConstantArray *ca = cast<ConstantArray>(v->llvm_inst);
			origT = ca->getType();
			origPointerLevel = pointerLevel(origT, 0);
		}
		else if (isa<ConstantStruct> (v->llvm_inst)) {
#ifdef DEBUG_RP
			blame_info<<"Leaving resolvePointersForNode for "<<v->name<<" Value ID - "<<v->llvm_inst->getValueID();
			blame_info<<"  -- ConstantStruct"<<std::endl;
#endif
		
			return;
			//ConstantStruct * csv = cast<ConstantStruct>(v->llvm_inst);
			//origT = csv->getType();
			//origPointerLevel = pointerLevel(origT, 0);
		}
		else if (isa<ConstantPointerNull> (v->llvm_inst)) {
#ifdef DEBUG_RP
			blame_info<<"Starting resolvePointersForNode for "<<v->name<<" Value ID - "<<v->llvm_inst->getValueID();
			blame_info<<"  -- ConstantPointerNullVal"<<std::endl;
#endif
			ConstantPointerNull *cpn = cast<ConstantPointerNull>(v->llvm_inst);
			origT = cpn->getType();
			origPointerLevel = pointerLevel(origT, 0);		
			origPointerLevel++; //TC: WHY ++ ?
		}
		else if (isa<ConstantVector> (v->llvm_inst)) {
#ifdef DEBUG_RP
			blame_info<<"Leaving resolvePointersForNode for "<<v->name<<" Value ID - "<<v->llvm_inst->getValueID();
			blame_info<<"  -- ConstantVectorVal"<<std::endl;
#endif
			return;			
		}		
		else {
#ifdef DEBUG_RP
			blame_info<<"Leaving resolvePointersForNode for "<<v->name<<" Value ID - "<<v->llvm_inst->getValueID()<<std::endl;
#endif
			return;
		}
	}

	else{
		return;
	}
	
	bool bitCastArray = false;

	if (v->collapsed_inst != NULL) {
		bool proceed = true;
	
		if (isa<Instruction>(v->collapsed_inst)) {
			Instruction *pi = cast<Instruction>(v->collapsed_inst);	
			collapsedT = pi->getType();	
			collapsedPointerLevel = pointerLevel(collapsedT,0);
		}
		else if (isa<ConstantExpr>(v->collapsed_inst)) {
			ConstantExpr *ce = cast<ConstantExpr>(v->collapsed_inst);
			collapsedT = ce->getType();
			collapsedPointerLevel = pointerLevel(collapsedT, 0);
		}
		else if (isa<ConstantAggregateZero> (v->collapsed_inst))
		{
			ConstantAggregateZero *caz = cast<ConstantAggregateZero>(v->collapsed_inst);
			collapsedT = caz->getType();
			collapsedPointerLevel = pointerLevel(collapsedT, 0);
		}
		else if (isa<ConstantArray> (v->collapsed_inst)) {
			ConstantArray *ca = cast<ConstantArray>(v->collapsed_inst);
			collapsedT = ca->getType();
			collapsedPointerLevel = pointerLevel(collapsedT, 0);
		}
		else {
			proceed = false;
		}
		
		if (proceed) { //for case that v came from a bitcast instruction
			std::string colTStr = returnTypeName(collapsedT, std::string(" "));
			
#ifdef DEBUG_RP
			blame_info<<"COL "<<collapsedPointerLevel<<" "<<colTStr<<std::endl;
#endif
			
			if (collapsedPointerLevel == 1 && colTStr.find("Array") != std::string::npos) {
				bitCastArray = true;
				
#ifdef DEBUG_RP
				blame_info<<v->name<<" has a bitcast array "<<v->collapsed_inst->getName().data()<<std::endl;
#endif
			}
		}
	}
	
	bool isGlobalField = false;
	
	NodeProps *fieldParent = v->fieldUpPtr;
	if (fieldParent != NULL) {
		if (fieldParent->isGlobal)
			isGlobalField = true;
	}
	
	std::string origTStr = returnTypeName(origT, std::string(" "));
	
#ifdef DEBUG_RP
	blame_info<<v->name<<"'s pointer level is "<<origPointerLevel<<std::endl;
	blame_info<<v->name<<" has pointer type "<<origTStr<<std::endl;
#endif 
    if(origPointerLevel >1) //added by Hui 12/16/15, doesn't matter later
        v->isPtr = true;
	
	// A pointer level of 1 is just the location in memory for a standard primitive
	// We do make an exception here for structs since we treat fields and pointers
	//   as interchangeable for this context
	
	// Changed 7/1/2010
	// and again 7/7/2010 
	// and again 7/12/2010 (bitCastArray, isGlobalField)
	if (origPointerLevel > 1 || v->storeLines.size() > 0 || ( origPointerLevel == 1 && origTStr.find("Array") != std::string::npos) || bitCastArray || isGlobalField) {
		//	||  (origPointerLevel == 1 && v->isExternCallParam))
		//if (origPointerLevel > 0 || v->storeLines.size() > 0 )
		// At this point, we don't care if the pointer is written to or not,
		//   we'll worry about that later
#ifdef DEBUG_RP
		blame_info<<"Inserting pointer(1) "<<v->name<<std::endl;
#endif 
		//pointers.insert(v);
		tempPointers.insert(v);
		
		boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
		
		e_beg = boost::in_edges(v->number, G).first;	// edge iterator begin
		e_end = boost::in_edges(v->number, G).second;   // edge iterator end
		
		std::set<int> visited; // don't know if the way we're traversing the graph,
		// a loop is even possible, but just in case
		visited.insert(v->number);
		
		// iterate through the edges trying to find a Store between pointers
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
			NodeProps *targetV = get(get(vertex_props,G), source(*e_beg,G));
			if (opCode == Instruction::Store) {	
#ifdef DEBUG_RP
				blame_info<<"STORE operation between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif 
			}
			else if (opCode == GEP_BASE_OP){
#ifdef DEBUG_RP
				blame_info<<"GEPB operation between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif          //here, targetV is the field ptr,e.g. a = GEP b, 0, 1 then a=targetV
				boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
				o_beg = boost::out_edges(targetV->number, G).first;//edge iter begin
				o_end = boost::out_edges(targetV->number, G).second;//edge iter end
				
				bool circumVent = true;
				// iterate through the edges trying to find a Store between pointers
				for(; o_beg != o_end; ++o_beg) {
					int opCode = get(get(edge_iore, G),*o_beg);
					if (opCode == RESOLVED_L_S_OP)
						circumVent = false;
				}	
				
				if (circumVent) {
					v->GEPs.insert(targetV); //a = GEP b, then b.GEPs.insert(a)
#ifdef DEBUG_RP
					blame_info<<"Adding GEP(3) "<<targetV->name<<" to load "<<v->name<<std::endl;
#endif 
					resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL, v);
				}	
				else {
#ifdef DEBUG_ERROR
					std::cerr<<"Was this supposed to happen?!"<<std::endl;
#endif 
				}
			}
			else if (opCode == Instruction::Load ) {
#ifdef DEBUG_RP
				blame_info<<"LOAD operation between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif
				boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
				o_beg = boost::out_edges(targetV->number, G).first;//edge iter begin
				o_end = boost::out_edges(targetV->number, G).second;//edge iter end
				
				bool circumVent = true;
				NodeProps *lsTarget = NULL;
				//added by Hui 04/26/16: for cases like: targetV = load v; call myFunc(targetV,..);
                //we need to add target to v's loads, further to its dataPtrs, then descParams, no matter
                //targetV has a R_L_S edge or not:TODO this modification is proven to be harmful, but we need a better way 
                bool isParam = false;

				for(; o_beg != o_end; ++o_beg) {
					int opCode = get(get(edge_iore, G),*o_beg);
					if (opCode == RESOLVED_L_S_OP) {
						circumVent = false;
						lsTarget = get(get(vertex_props,G), target(*o_beg,G));
					}
                    else if (opCode == Instruction::Call || opCode == Instruction::Invoke || opCode == RESOLVED_EXTERN_OP) {
                        isParam = true; //above last opCode added for blamed params that were fed to externFuncs after adjustment
                    }
				}	


				if (circumVent) {
					v->loads.insert(targetV);
					resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL, v);
				}
				else {
#ifdef DEBUG_RP
					blame_info<<"Load not put in because there was a R_L_S from targetV "<<targetV->name<<std::endl;
#endif

#ifdef TEMP_FOR_MINIMD
					if (isParam) { //added by Hui 04/26/16, for
                        v->loadForCalls.insert(targetV);
                    } //TODO: old addition as 'loads' is proven to be harmful, study later to see if it's necessary
#endif
                    if (lsTarget) {
#ifdef DEBUG_RP
						blame_info<<"R_L_S op between "<<lsTarget->name<<" and "<<targetV->name<<std::endl;
#endif 
						lsTarget->resolvedLS.insert(targetV);
						targetV->resolvedLSFrom.insert(lsTarget);
						resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, lsTarget, v);
						//resolvePointersHelper2(lsTarget, origPointerLevel, targetV, visited, tempPointers);
						
					}
				}
			}
			else if (opCode == RESOLVED_L_S_OP) {
#ifdef DEBUG_RP
				blame_info<<"RLS operation between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif 
				v->resolvedLS.insert(targetV);
				targetV->resolvedLSFrom.insert(v);
				resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL, v);
			}
		}
	}
	else if (origTStr.find("Struct") != std::string::npos) { //TOCONTINUE: 01/30/16
		//At this point, we don't care if the pointer is written to or not,
		//we'll worry about that later
#ifdef DEBUG_RP
		blame_info<<"Inserting pointer(3) "<<v->name<<std::endl;
#endif 
		//pointers.insert(v);
		tempPointers.insert(v);
		boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
		
		e_beg = boost::in_edges(v->number, G).first;		// edge iterator begin
		e_end = boost::in_edges(v->number, G).second;    // edge iterator end
		
		// iterate through the edges trying to find a Store between pointers
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
            //here,targetV is the coming node for v, not v itself
			NodeProps * targetV = get(get(vertex_props,G), source(*e_beg,G));
#ifdef DEBUG_RP
        blame_info<<"For the struct pointer: "<<v->name<<", targetV of this in_edge is "<<targetV->name<<std::endl;
#endif

			if (opCode == GEP_BASE_OP) {
				boost::graph_traits<MyGraphType>::out_edge_iterator e_beg2, e_end2;
				e_beg2 = boost::out_edges(targetV->number,G).first;//edge iter begin
				e_end2 = boost::out_edges(targetV->number,G).second;//edge iter  end
				
		// iterate through the edges trying to find relationship between pointers
				for(; e_beg2 != e_end2; ++e_beg2) {
					int opCodeForField = get(get(edge_iore, G),*e_beg2);
					int fNum = 0;
					if (opCodeForField >= GEP_S_FIELD_OFFSET_OP) {
						fNum = opCodeForField - GEP_S_FIELD_OFFSET_OP;//=which field
						if (isa<Value>(v->llvm_inst)) {
							Value *val = cast<Value>(v->llvm_inst);	
#ifdef DEBUG_RP
							blame_info<<"Calling structResolve for "<<v->name<<" of type "<<origTStr<<endl;
#endif 
							structResolve(val, fNum, targetV);
						}
					}
					if (opCodeForField == Instruction::Store) { //field is written
						targetV->isWritten = true;
#ifdef DEBUG_RP
						blame_info<<"Vertex "<<targetV->name<<" is written(2)"<<std::endl;
#endif 
					}
				}
#ifdef DEBUG_RP
				blame_info<<"Transferring sBFC from(sf->ps) "<<targetV->name<<" to "<<v->name<<std::endl;
#endif 

				if (targetV->sField != NULL)
					v->sBFC = targetV->sField->parentStruct;
				if(v != targetV){ //condition added by Hui 01/20/16
                    blame_info<<"Adding "<<targetV->name<<" to "<<v->name<<" as a field(2)"<<endl;
                    v->fields.insert(targetV);
                    targetV->fieldUpPtr = v;
                }
				if (v->isGlobal) {
					tempPointers.insert(targetV);
				}
			}
#ifdef HUI_CHPL
            else if (opCode == Instruction::Store) {
                boost::graph_traits<MyGraphType>::in_edge_iterator e_beg2, e_end2;
                e_beg2 = boost::in_edges(targetV->number, G).first;
                e_end2 = boost::in_edges(targetV->number, G).second;

                for (; e_beg2 != e_end2; ++e_beg2) {
                    int opCode2 = get(get(edge_iore, G), *e_beg2);
                    NodeProps *targetV2 = get(get(vertex_props,G),source(*e_beg2,G));
#ifdef DEBUG_RP
                    blame_info<<"For "<<targetV->name<<", targetV2 of this in_edge is "<<targetV2->name<<std::endl;
#endif
                    if (opCode2 == Instruction::Load) {
                        boost::graph_traits<MyGraphType>::in_edge_iterator e_beg3, 
                                                                            e_end3;
                        e_beg3 = boost::in_edges(targetV2->number, G).first;
                        e_end3 = boost::in_edges(targetV2->number, G).second;

                        for (; e_beg3 != e_end3; ++e_beg3) {
                            int opCode3 = get(get(edge_iore, G), *e_beg3);
                            NodeProps *targetV3 = get(get(vertex_props, G), source(*e_beg3, G));
#ifdef DEBUG_RP
                            blame_info<<"For "<<targetV2->name<<", targetV3 of this in_edge is "<<targetV3->name<<std::endl;

#endif
 			                if (opCode3 == GEP_BASE_OP) {
				                boost::graph_traits<MyGraphType>::out_edge_iterator e_beg4, e_end4;
				                e_beg4 = boost::out_edges(targetV3->number,G).first;//edge iter begin
				                e_end4 = boost::out_edges(targetV3->number,G).second;//edge iter  end
				
		// iterate through the edges trying to find relationship between pointers
				                for(; e_beg4 != e_end4; ++e_beg4) {
					                int opCodeForField = get(get(edge_iore, G),*e_beg4);
					                int fNum = 0;
					                if (opCodeForField >= GEP_S_FIELD_OFFSET_OP) {
						                fNum = opCodeForField - GEP_S_FIELD_OFFSET_OP; 
						                if (isa<Value>(v->llvm_inst)) {
							                Value *val = cast<Value>(v->llvm_inst);	
#ifdef DEBUG_RP
							                blame_info<<"Calling structResolve for "<<v->name<<" of type "<<origTStr<<endl;
#endif                                      //TO BE CHECKED: 08/22/15 structResolve
							                structResolve(val, fNum, targetV3);
                                        }
                                    }
					                if (opCodeForField == Instruction::Store) {
						                targetV3->isWritten = true;
#ifdef DEBUG_RP
						                blame_info<<"Vertex "<<targetV3->name<<" is written(2)"<<std::endl;
#endif 
					                }
				                }
#ifdef DEBUG_RP
				                blame_info<<"Transferring sBFC from(sf->ps) "<<targetV3->name<<" to "<<v->name<<std::endl;
#endif 

				                if (targetV3->sField != NULL)
					                v->sBFC = targetV3->sField->parentStruct;
				                if(v != targetV3){ //condition added by Hui 01/20/16
                                    blame_info<<"Adding "<<targetV3->name<<" to "<<v->name<<" as a field(3)"<<endl;
                                    v->fields.insert(targetV3);
				                    targetV3->fieldUpPtr = v;
				                }
				                if (v->isGlobal) {
					                tempPointers.insert(targetV3);
				                }
			                }// if opCode3=GEP_BASE_OP
                        }// for each in_edge of targetV2
                    }// if opCode2=Load
                }// for each in_edge of targetV
            }// else if opCode=Store
#endif
		}// for each in_edge of v
	}// origTstr.find("Struct")
	else if (origTStr.find("Array") != std::string::npos) {
		resolveArrays(v, v, tempPointers);
	}
    //added by Hui :currently doesn't give much help, so delete it, TO CHECK later
    /*
    else if(origPointerLevel ==1) { //we take care of the basic 1-level ptr
#ifdef DEBUG_RP
		blame_info<<"Inserting pointer(new) "<<v->name<<std::endl;
#endif 
		//pointers.insert(v);
		tempPointers.insert(v);
		
		boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
		
		e_beg = boost::in_edges(v->number, G).first;	// edge iterator begin
		e_end = boost::in_edges(v->number, G).second;   // edge iterator end
		
		std::set<int> visited; // don't know if the way we're traversing the graph,
		// a loop is even possible, but just in case
		visited.insert(v->number);
		
		// iterate through the edges trying to find a Store between pointers
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
#ifdef DEBUG_RP
		    blame_info<<"in_edge opCode = "<<opCode<<std::endl;
#endif 
			NodeProps *targetV = get(get(vertex_props,G), source(*e_beg,G));
			if (opCode == Instruction::Store) {	
#ifdef DEBUG_RP
				blame_info<<"STORE operation(new) between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif 
			}
			else if (opCode == GEP_BASE_OP){
#ifdef DEBUG_RP
				blame_info<<"GEPB operation(new) between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif          //here, targetV is the field ptr,e.g. a = GEP b, 0, 1 then a=targetV
				boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
				o_beg = boost::out_edges(targetV->number, G).first;//edge iter begin
				o_end = boost::out_edges(targetV->number, G).second;//edge iter end
				
				bool circumVent = true;
				// iterate through the edges trying to find a Store between pointers
				for(; o_beg != o_end; ++o_beg) {
					int opCode = get(get(edge_iore, G),*o_beg);
					if (opCode == RESOLVED_L_S_OP)
						circumVent = false;
				}	
				
				if (circumVent) {
					v->GEPs.insert(targetV); //a = GEP b, then b.GEPs.insert(a)
#ifdef DEBUG_RP
					blame_info<<"Adding GEP(new) "<<targetV->name<<" to load "<<v->name<<std::endl;
#endif 
					//resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL);
				}	
				else {
#ifdef DEBUG_ERROR
					std::cerr<<"Was this supposed to happen(new)?!"<<std::endl;
#endif 
				}
			}
			else if (opCode == Instruction::Load ) {
#ifdef DEBUG_RP
				blame_info<<"LOAD operation(new) between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif
				boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
				o_beg = boost::out_edges(targetV->number, G).first;//edge iter begin
				o_end = boost::out_edges(targetV->number, G).second;//edge iter end
				
				bool circumVent = true;
				NodeProps *lsTarget = NULL;
				
				for(; o_beg != o_end; ++o_beg) {
					int opCode = get(get(edge_iore, G),*o_beg);
					if (opCode == RESOLVED_L_S_OP) {
						circumVent = false;
						lsTarget = get(get(vertex_props,G), target(*o_beg,G));
					}
				}	
				
				if (circumVent) {
					v->loads.insert(targetV);
					//resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL);
				}
				else {
#ifdef DEBUG_RP
					blame_info<<"Load not put in(new) because there was a R_L_S from targetV "<<targetV->name<<std::endl;
#endif 
					if (lsTarget) {
#ifdef DEBUG_RP
						blame_info<<"R_L_S op(new) between "<<lsTarget->name<<" and "<<targetV->name<<std::endl;
#endif 
						lsTarget->resolvedLS.insert(targetV);
						targetV->resolvedLSFrom.insert(lsTarget);
						//resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, lsTarget);
					}
				}
			}
			else if (opCode == RESOLVED_L_S_OP) {
#ifdef DEBUG_RP
				blame_info<<"RLS operation(new) between "<<v->name<<" and "<<targetV->name<<std::endl;
#endif 
				v->resolvedLS.insert(targetV);
				targetV->resolvedLSFrom.insert(v);
				//resolvePointersHelper2(v, origPointerLevel, targetV, visited, tempPointers, NULL);
			}
		}
    }*/
	
#ifdef DEBUG_RP_SUMMARY
	
	blame_info<<std::endl;
	blame_info<<"For PTR "<<v->name<<std::endl;
	blame_info<<"Is Pointer "<<v->isPtr<<std::endl; //TC: Should it be set to 1?
	blame_info<<"Is Written "<<v->isWritten<<std::endl;
	std::set<NodeProps *>::iterator vec_vp_i;
	std::set<NodeProps *>::iterator set_vp_i;
	
	
	blame_info<<"Aliases in ";
	for (vec_vp_i = v->aliasesIn.begin(); vec_vp_i != v->aliasesIn.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Aliases out ";
	for (vec_vp_i = v->aliasesOut.begin(); vec_vp_i != v->aliasesOut.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Fields ";
	for (vec_vp_i = v->fields.begin(); vec_vp_i != v->fields.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"GEPs ";
	for (vec_vp_i = v->GEPs.begin(); vec_vp_i != v->GEPs.end(); vec_vp_i++){
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Non-Alias-Stores ";
	for (vec_vp_i = v->nonAliasStores.begin(); vec_vp_i != v->nonAliasStores.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Loads ";
	for (vec_vp_i = v->loads.begin(); vec_vp_i != v->loads.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Almost Aliases ";
	for (vec_vp_i = v->almostAlias.begin(); vec_vp_i != v->almostAlias.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"Resolved LS ";
	for (vec_vp_i = v->resolvedLS.begin(); vec_vp_i != v->resolvedLS.end(); vec_vp_i++) {
		blame_info<<(*vec_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"StoresTo ";
	for (set_vp_i = v->storesTo.begin(); set_vp_i != v->storesTo.end(); set_vp_i++){
		blame_info<<(*set_vp_i)->name<<" ";
	}
	blame_info<<std::endl;
	
	blame_info<<"StoreLines"<<std::endl;
	std::set<int>::iterator set_i_i;
	for (set_i_i = v->storeLines.begin(); set_i_i != v->storeLines.end(); set_i_i++)
	{
		blame_info<<*set_i_i<<" ";
	}
	blame_info<<std::endl;
	
#endif 
	
}

void FunctionBFC::resolvePointers2()
{
	graph_traits < MyGraphType >::edge_descriptor ed;
    property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
	
	std::set<NodeProps *> tempPointers;
	
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		
		NodeProps *v = get(get(vertex_props, G),*i);
		//int v_index = get(get(vertex_index, G),*i);
		if (!v) {
#ifdef DEBUG_ERROR
			std::cerr<<"Null V in resolvePointers2\n";
#endif 
			continue;
		}
#ifdef ONLY_FOR_PARAM1
        //-------------------------------------------------------------------//
        //added by Hui 04/08/16: set isPtr=true for all nodes with ptrlevel>=1
		int origPointerLevel = 0;
        const llvm::Type *origT;
        if (isa<Instruction>(v->llvm_inst)) {
			Instruction *pi = cast<Instruction>(v->llvm_inst);	
			origT = pi->getType();	
			origPointerLevel = pointerLevel(origT,0);
		}
		else if (isa<ConstantExpr>(v->llvm_inst)) {
		    ConstantExpr *ce = cast<ConstantExpr>(v->llvm_inst);
			origT = ce->getType();
			origPointerLevel = pointerLevel(origT, 0);
		}
        
        if(origPointerLevel >=1)
            v->isPtr = true;
        //----------------------------------------------------------------//
#endif
		// First we want to resolve the local variables and function parameters
		if (v->isLocalVar == true || v->name.find(PARAM_REC) != std::string::npos
				|| v->isGlobal) {
			resolvePointersForNode2(v, tempPointers);
		}

        //added by Hui 03/22/16
        //we need to take care of registers that are the recipients of a STORE, 
        //it's rare but important, like %12 in: store %11, %12
        /*else { 
            bool isImpReg = false;
            int v_index = get(get(vertex_index, G),*i);
            boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
            e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
            e_end = boost::out_edges(v_index, G).second;    // edge iterator end	
            for(; e_beg != e_end; ++e_beg) {
                int opCode = get(get(edge_iore, G), *e_beg);
                if(opCode == Instruction::Store) { //when the register is 
                    isImpReg = true;
                    break;
                }
            }
            if(isImpReg)
			    resolvePointersForNode2(v, tempPointers);
        }*/ 
	}//end of for loop
	
	std::set<NodeProps *>::iterator set_vp_i;	
	
	for(set_vp_i = tempPointers.begin(); set_vp_i != tempPointers.end(); set_vp_i++)
		pointers.insert(*set_vp_i);
	
#ifdef DEBUG_RP
	blame_info<<"At this point the pointers are: ";
	for (set_vp_i = pointers.begin(); set_vp_i != pointers.end(); set_vp_i++)
		blame_info<<(*set_vp_i)->name<<" ";
	blame_info<<std::endl;
#endif 
	
	bool keepGoing = true;
	while (keepGoing) {
		tempPointers.clear();
		for (set_vp_i = pointers.begin(); set_vp_i != pointers.end(); set_vp_i++) {
			NodeProps *v = (*set_vp_i);
			resolvePointersForNode2(v, tempPointers);
			std::set<NodeProps *>::iterator vec_vp_i;
			for (vec_vp_i = v->fields.begin(); vec_vp_i != v->fields.end(); vec_vp_i++) {
				resolvePointersForNode2((*vec_vp_i), tempPointers);
			}
		}
		
		if (tempPointers.size() > 0) {
			for (set_vp_i = tempPointers.begin(); set_vp_i != tempPointers.end(); set_vp_i++)
				pointers.insert(*set_vp_i);
			
			keepGoing = true;
		}
		else {
			keepGoing = false;
		}
	}
#ifdef DEBUG_RP
	blame_info<<"At this point(2) the pointers are: ";
	for (set_vp_i = pointers.begin(); set_vp_i != pointers.end(); set_vp_i++)
		blame_info<<(*set_vp_i)->name<<" ";
	blame_info<<std::endl;
#endif 
	
	// Now assign the EXIT_VAR, EXIT_VAR_ALIAS, EXIT_VAR_PTR distinctions
	for (set_vp_i = pointers.begin(); set_vp_i != pointers.end(); set_vp_i++) {
		NodeProps *vp = (*set_vp_i);
		if ((vp->name.find(PARAM_REC) != std::string::npos && vp->isLocalVar == false) 
				|| vp->isGlobal || vp->name.find("retval") != std::string::npos) {
			// First we need to make sure that it is actually written in at least
			//   one of the aliases
			std::set<int> visited;
			if (vp->isWritten || checkIfWritten2(vp, visited)) {
#ifdef DEBUG_RP
				blame_info<<"For "<<vp->name<<" one of the aliases was written to!"<<std::endl;
#endif 
				vp->isWritten = true;
				vp->nStatus[EXIT_VAR] = true;
				vp->exitV = vp;
				visited.clear();
				resolveAliases2(vp, vp, visited, vp);
				//visited.clear();
				//resolveDataPtrs(vp, vp, visited);
			}
			else {
#ifdef DEBUG_RP
				blame_info<<"For "<<vp->name<<" one of the aliases was NOT written to!"<<std::endl;
#endif
			}
		}	
	}
	// Now assign the LOCAL_VAR, LOCAL_VAR_ALIAS, LOCAL_VAR_PTR distinctions
	for (set_vp_i = pointers.begin(); set_vp_i != pointers.end(); set_vp_i++) {
		NodeProps *vp = (*set_vp_i);
		if (vp->isLocalVar == true) {
			// First we need to make sure that it is actually written in at least
			//   one of the aliases
			std::set<int> visited;
			
			if (vp->isWritten || checkIfWritten2(vp, visited)) {
#ifdef DEBUG_RP
				blame_info<<"For "<<vp->name<<" one of the aliases was written to!"<<std::endl;
#endif 
				vp->isWritten = true;
				vp->nStatus[LOCAL_VAR] = true;
				
				if (vp->exitV == NULL)
					vp->exitV = vp;
				visited.clear();
				resolveLocalAliases2(vp, vp, visited, vp);
				//visited.clear();
				//resolveDataPtrs(vp, vp, visited);
			}
			else {
				
#ifdef DEBUG_RP
				blame_info<<"For "<<vp->name<<" not written to (that we know) but still going to resolve aliases"<<std::endl;
#endif 			 
				vp->nStatus[LOCAL_VAR] = true;
				
				if (vp->exitV == NULL)
					vp->exitV = vp;
				visited.clear();
				resolveLocalAliases2(vp, vp, visited, vp);
				
				
#ifdef DEBUG_RP
				blame_info<<"For "<<vp->name<<" one of the aliases was NOT written to!"<<std::endl;
#endif 
				
			}
		}
	}
	
	// Finally, assign the LOCAL_VAR_A_ALIAS for those that don't have it yet
	// Mainly, for those non-ponter local variables
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps *v = get(get(vertex_props, G),*i);
		//int v_index = get(get(vertex_index, G),*i);
		if (!v) {
#ifdef DEBUG_ERROR
			std::cerr<<"Null V in resolvePointers2\n";
#endif 
			continue;
		}
		// First we want to resolve the local variables and function parameters
		if (v->isLocalVar) {
			//resolveLocalDFA(v, pointers); //removed by Hui 04/11/16: storesTo shouldn't be included into dfAliases
		}
	}
}

void FunctionBFC::againCheckIfWrittenForAllNodes()
{
    
	graph_traits < MyGraphType >::edge_descriptor ed;
    property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
	
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		
		NodeProps *v = get(get(vertex_props, G),*i);
		//int v_index = get(get(vertex_index, G),*i);
		if (!v) {
#ifdef DEBUG_AGAINCHECK
			blame_info<<"Null V in againCheckIfWrittenForAllNodes\n";
#endif 
			continue;
		}
		int origPointerLevel = 0;
        const llvm::Type *origT;
        if (isa<Instruction>(v->llvm_inst)) {
			Instruction *pi = cast<Instruction>(v->llvm_inst);	
			origT = pi->getType();	
			origPointerLevel = pointerLevel(origT,0);
		}
		else if (isa<ConstantExpr>(v->llvm_inst)) {
		    ConstantExpr *ce = cast<ConstantExpr>(v->llvm_inst);
			origT = ce->getType();
			origPointerLevel = pointerLevel(origT, 0);
		}
        
        if(origPointerLevel >=1){
            std::set<int> visited;
            short writtenTo = checkIfWritten2(v, visited);
#ifdef DEBUG_AGAINCHECK
            blame_info<<v->name<<"'s writtenTo = "<<writtenTo<<endl;
#endif
        }
    }
}

void FunctionBFC::resolveLocalDFA(NodeProps * v, std::set<NodeProps *> & pointers)
{
	std::set<NodeProps *>::iterator set_vp_i;
	
	for (set_vp_i = v->storesTo.begin(); set_vp_i != v->storesTo.end(); set_vp_i++)
	{
		NodeProps * vp = *set_vp_i;
		
		if (vp->storeLines.size() == 0)
			continue;
		
		if (!vp->nStatus[EXIT_VAR_A_ALIAS])
		{
			//pointers.insert(vp);
			vp->nStatus[LOCAL_VAR_A_ALIAS] = true;
			
			#ifdef DEBUG_RP
			blame_info<<"Inserting dfAlias(3) "<<vp->name<<" into set for "<<v->name<<std::endl;
			#endif 
			
			v->dfAliases.insert(vp);
			vp->dfaUpPtr = v;
#ifdef DEBUG_RP
			blame_info<<"Vertex "<<v->name<<" is written(3)"<<std::endl;
#endif 
			v->isWritten = true; //changed from vp to v, by Hui 04/11/16: we can't say vp->isWritten=true from here
		}
	}
}

void FunctionBFC::collapseAutoCopyPairs()
{
	std::vector<CollapsePair *>::iterator vec_cp_i;
	
	for (vec_cp_i = autoCopyCollapsePairs.begin(); vec_cp_i != autoCopyCollapsePairs.end(); vec_cp_i++)
	{
		CollapsePair *cp = *vec_cp_i;
#ifdef DEBUG_GRAPH_COLLAPSE
		blame_info<<"Transferring CAP edges from "<<cp->collapseVertex->name<<" to "<<cp->destVertex->name<<endl;
#endif 
		transferEdgesAndDeleteNode(cp->collapseVertex, cp->destVertex);
	}
}

void FunctionBFC::collapseRedundantFields()
{
	std::vector<CollapsePair *>::iterator vec_cp_i;
	
	for (vec_cp_i = collapsePairs.begin(); vec_cp_i != collapsePairs.end(); vec_cp_i++)
	{
		CollapsePair * cp = *vec_cp_i;
#ifdef DEBUG_GRAPH_COLLAPSE
		blame_info<<"Transferring CRF edges from field "<<cp->collapseVertex->name<<" to "<<cp->destVertex->name;
		blame_info<<" for "<<cp->nameFieldCombo<<std::endl;
#endif 
		transferEdgesAndDeleteNode(cp->collapseVertex, cp->destVertex, false, true);
	}
}


void FunctionBFC::resolveDataReads()
{
#ifdef DEBUG_CFG
	blame_info<<"Before assignBBGenKIll"<<std::endl;
#endif 
	cfg->assignPTRBBGenKill();
	
#ifdef DEBUG_CFG
	blame_info<<"Before reachingDefs"<<std::endl;
#endif 
	cfg->reachingPTRDefs();
	
#ifdef DEBUG_CFG
	blame_info<<"Before calcStoreLines"<<std::endl;
#endif 
	cfg->calcPTRStoreLines();
	
#ifdef DEBUG_CFG
	blame_info<<"Before printCFG"<<std::endl;
	printCFG();
#endif 
	
}



void FunctionBFC::resolveTransitiveAliases()
{
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps *v = get(get(vertex_props, G),*i);
		if (!v) {
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in resolveTransitiveAlias\n";
#endif 
			continue;
		}
		std::set<NodeProps *>::iterator set_vp_i, set_vp_i2;
		for (set_vp_i = v->aliases.begin(); set_vp_i!=v->aliases.end(); set_vp_i++){
			NodeProps *al1 = *set_vp_i;
			for (set_vp_i2 = set_vp_i; set_vp_i2 != v->aliases.end(); set_vp_i2++) {
				NodeProps *al2 = *set_vp_i2;
				if (al1 == al2 )
					continue;
				
				if (al1 != v) {
				#ifdef DEBUG_RP
					blame_info<<"Transitive alias between "<<al1->name<<" and "<<al2->name<<std::endl;
				#endif 
					al1->aliases.insert(al2);
				}
				if (al2 != v) {
				#ifdef DEBUG_RP
					blame_info<<"Transitive alias(2) between "<<al2->name<<" and "<<al1->name<<std::endl;
					#endif 
					al2->aliases.insert(al1);
				}
			}
		}
	}
}

// TODO: Probably should account for the fact that a field may be aliased to more than one thing
void FunctionBFC::resolveFieldAliases()
{
    graph_traits<MyGraphType>::vertex_iterator i, v_end;

    for (tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps * v= get(get(vertex_props, G),*i);
		if (!v){
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in resolveFieldAliases\n";
#endif 
			continue;
		}
		
		if (v->nStatus[EXIT_VAR_FIELD_ALIAS]) {
			if (v->fieldUpPtr != NULL){
			#ifdef DEBUG_RP
				blame_info<<"IMPORTANT EVFA "<<v->name<<" alread has fUpPtr "<<v->fieldUpPtr->name<<std::endl;
			#endif 
				return;
			}

            blame_info<<"In resolveFieldAliases for EVFA: "<<v->name<<endl;
			
			std::set<NodeProps *>::iterator set_vp_i, set_vp_i2;
		
			for (set_vp_i = v->aliases.begin(); set_vp_i != v->aliases.end(); set_vp_i++) {
				NodeProps *al = *set_vp_i;
				
	// we might be our own alias ... probably should make it so that never happens
				// TODO: we should never be our own alias
				if (al == v)
					continue;
					
				if (al->nStatus[EXIT_VAR_FIELD] && al->fieldUpPtr != NULL && al->fieldUpPtr != v) {//last cond added by Hui 01/20/16
					v->fieldUpPtr = al->fieldUpPtr;
					v->fieldAlias = al; //TC: you can have >=1 alias right ?
					
                    blame_info<<"Adding "<<v->name<<" to "<<al->name<<"'s fieldUpPtr:"<<al->fieldUpPtr->name<<" as a field(4)"<<endl;
                    al->fieldUpPtr->fields.insert(v);
					v->nStatus[EXIT_VAR_FIELD] = true;
					#ifdef DEBUG_RP
					blame_info<<"Making EVFA variable an EVF for "<<v->name<<" to "<<v->fieldUpPtr->name<<std::endl;
					#endif 
				}	
			}
		}
	    else if (v->nStatus[LOCAL_VAR_FIELD_ALIAS]) {
			if (v->fieldUpPtr != NULL) {
			#ifdef DEBUG_RP
				blame_info<<"IMPORTANT LVFA "<<v->name<<" alread has fUpPtr "<<v->fieldUpPtr->name<<std::endl;
			#endif 
				return;
			}
			
            blame_info<<"In resolveFieldAliases for LVFA: "<<v->name<<endl;
			
            std::set<NodeProps *>::iterator set_vp_i, set_vp_i2;
		    for (set_vp_i = v->aliases.begin(); set_vp_i != v->aliases.end(); set_vp_i++) {
				NodeProps *al = *set_vp_i;
	// we might be our own alias ... probably should make it so that never happens
				// TODO: we should never be our own alias
				if (al == v)
					continue;
					
				if (al->nStatus[LOCAL_VAR_FIELD] && al->fieldUpPtr != NULL && al->fieldUpPtr != v) {//last cond added by Hui 01/20/16
					v->fieldUpPtr = al->fieldUpPtr;
					v->fieldAlias = al; //TC: what if v has >1 alias ?
					
                    blame_info<<"Adding "<<v->name<<" to "<<al->name<<"'s fieldUpPtr:"<<al->fieldUpPtr->name<<" as a field(5)"<<endl;
                    al->fieldUpPtr->fields.insert(v);
					v->nStatus[LOCAL_VAR_FIELD] = true;
					#ifdef DEBUG_RP
					blame_info<<"Making LVFA variable an LVF for "<<v->name<<" to "<<v->fieldUpPtr->name<<std::endl;
					#endif 
				}	
			}
		}
    }
}

void FunctionBFC::genGraph(ExternFuncBFCHash &efInfo)
{
    // Create graph with the exact size of the number of symbols for the function
    MyGraphType G_new(variables.size()); //variables stores all the blame nodes
    G.swap(G_new); //switch G and G_new
  
    // Create property map for node properies for each register
    property_map<MyGraphType, vertex_props_t>::type props = get(vertex_props, G);	
  
    // Create property map for edge properties for each blame relationship
    property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
  
    RegHashProps::iterator begin, end;
    /* Iterate through variables and label them with their node IDs (integers)
	 -> first is const char * associated with their name
	 -> second is NodeProps * for node
	 */
    for (begin = variables.begin(), end = variables.end(); begin != end; begin++) {
		std::string name(begin->first);
		NodeProps *v = begin->second;
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"Putting node "<<v->number<<" ("<<v->name<<") into graph"<<std::endl;
#endif 
		put(props, v->number, v);
	}
  //every block has a iSet, is a set of implicit blame nodes:Condition Names
    std::set<const char*, ltstr> iSet;	
  
    //int varCount = 0, currentLineNum = 0;
  
    int currentLineNum = 0;
#ifdef DEBUG_GRAPH_BUILD
	blame_info<<"Starting to Gen Edges "<<std::endl;
#endif 
    // We iterate through all the instructions again to create the edges (explicit and implicit)
    // generated between the registers	
	// Some times there are duplicate calls on lines, we want to make sure we don't reuse one
	std::set<NodeProps *> seenCall;
    for (Function::iterator b = func->begin(), be = func->end(); b != be; ++b) {
        iSet = iReg[b->getName().str()];
        for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; ++i) {
            //std::cout<<"CurrentLineNum is "<<currentLineNum<<std::endl;
            genEdges(i, iSet, props, edge_type, currentLineNum, seenCall);
        }
    }
	
#ifdef DEBUG_GRAPH_BUILD
	blame_info<<"Finished generating edges "<<std::endl;
#endif 
  
	// TODO: Make this a verbose or runtime decision
    //printDotFiles("_noCompress.dot", false);
	printDotFiles("_noCompressImp.dot", true);
  
    collapseGraph(); 

    //added by Hui 05/09/16
    collapseAutoCopyPairs();
	
#ifdef ENABLE_FORTRAN
	collapseRedundantFields();
#endif 
	printDotFiles("_afterCompressImp.dot", true);
	
	//Make the edges incoming to nodes that have stores based on
	//the control flow
	resolveStores();	
	
	identifyExternCalls(efInfo); 
	
	resolvePointers2();
	
    //added by Hui 05/08/16: to make up for all important registers
    againCheckIfWrittenForAllNodes();
	
    // the alias of my alias is my friend
	// this needs to be after resolvePointers2
	resolveTransitiveAliases(); 
	
	// make field aliases associated with the proper local variable/exit variable in terms of fieldUpPtrs
	resolveFieldAliases();
	
	// Most stuff concentrates on data writes, we need to figure out
	// for any given data read the reaching definition for the data write
	// that affects it and factor that into things 
	//resolveDataReads();
#ifdef DEBUG_GRAPH_BUILD
	blame_info<<"Calling DBHL "<<std::endl; 
#endif 
	determineBFCHoldersLite();
	
	resolveCallsDomLines(); 
	
#ifdef DEBUG_GRAPH_BUILD
	blame_info<<"Finished DBHL , now going to print _trunc.dot file "<<std::endl;
#endif
	printTruncDotFiles("_trunc.dot", true);
}

void FunctionBFC::printCFG()
{
	cfg->printCFG(blame_info);
}


void FunctionBFC::resolveStores()
{
    property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
    //bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
    graph_traits<MyGraphType>::vertex_iterator i, v_end; 
	
    for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps *v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"In resolveStores for "<<v->name<<std::endl;
#endif 
		
		if (!v) {
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in resolveStores\n";
#endif 
			continue;
		}
		
		boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
		
		e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
		e_end = boost::out_edges(v_index, G).second;    // edge iterator end	
		//int storeCount = 0;	
		std::set<NodeProps *> stores;	
		std::set<NodeProps *> storesSource;
		
		// iterate through the edges to find matching opcode
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
			if (opCode == Instruction::Store) {
	            NodeProps *sourceV = get(get(vertex_props,G), source(*e_beg,G));
	            NodeProps *targetV = get(get(vertex_props,G), target(*e_beg,G));
				
				#ifdef DEBUG_LINE_NUMS
				blame_info<<"Inserting line number(1a) "<<targetV->line_num<<" to "<<sourceV->name<<std::endl;
				#endif 
				sourceV->lineNumbers.insert(targetV->line_num);
				
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Store between "<<sourceV->name<<" and "<<targetV->name<<std::endl;
				blame_info<<"Vertex "<<sourceV->name<<" is written(4)"<<std::endl;
#endif 
				
				if (sourceV->name.find(PARAM_REC) == std::string::npos)
					sourceV->isWritten = true;
				                
                blame_info<<"We have a storeFrom inserted: "<<targetV->name<<"->storeFrom="<<sourceV->name<<endl;

                targetV->storeFrom = sourceV;
				sourceV->storesTo.insert(targetV);
				//storeCount++;
				stores.insert(targetV);
				storesSource.insert(sourceV);
                /*
                sourceV->storeFrom = targetV;
				targetV->storesTo.insert(sourceV);
				//storeCount++;
				stores.insert(sourceV);
				storesSource.insert(targetV);
                */
			}
		}
		
        //Since resolveDataRead is no longer used, so we put all stores into relevantInstructions
        //no singleStores anymore !!!
		if (stores.size() >= 1){//store a *b, then a is in stores, b is in storesSource..
#ifdef DEBUG_GRAPH_BUILD
			blame_info<<"Stores for V - "<<v->name<<" number "<<stores.size()<<std::endl;
#endif 
			std::set<NodeProps *>::iterator set_vp_i = stores.begin();
			for ( ; set_vp_i != stores.end(); set_vp_i++) {
				NodeProps *storeVP = (*set_vp_i);
				
				if (storeVP->fbb == NULL)
					continue;
				
				storeVP->fbb->relevantInstructions.push_back(storeVP);
			}
		}// These are single stores to one particular GEP grab of a variable, but in actuality they are one of many stores
		// (in likelihood) to the variable pointer  
		/*else if (stores.size() == 1) {
#ifdef DEBUG_GRAPH_BUILD
			blame_info<<"Stores for V(2) - "<<v->name<<" number "<<stores.size()<<std::endl;
#endif 		
			std::set<NodeProps *>::iterator set_vp_i = storesSource.begin();
			for ( ; set_vp_i != storesSource.end(); set_vp_i++) {
				NodeProps *storeVP = (*set_vp_i);
				if (storeVP->fbb == NULL)
					continue;
				
				#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Single stores pushing back "<<storeVP->name<<std::endl;
				#endif 
				storeVP->fbb->singleStores.push_back(storeVP);
			}
		}*/
	}

#ifdef DEBUG_CFG
	blame_info<<"Before CFG sort"<<std::endl;
#endif 
	cfg->sortCFG(); //for each BB, sort the edges
	
#ifdef DEBUG_CFG
	blame_info<<"Before assignBBGenKIll"<<std::endl;
#endif 
	cfg->assignBBGenKill();
	
#ifdef DEBUG_CFG
	blame_info<<"Before reachingDefs"<<std::endl;
#endif 
	cfg->reachingDefs();
	
#ifdef DEBUG_CFG
	blame_info<<"Before calcStoreLines"<<std::endl;
#endif 
	cfg->calcStoreLines();
	
#ifdef DEBUG_CFG
	blame_info<<"Before printCFG"<<std::endl;
	printCFG();
#endif 

	adjustMainGraph();
	
}


void FunctionBFC::adjustMainGraph()
{
	bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end; 
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i)  {
		NodeProps *vp = get(get(vertex_props, G),*i);
		// We have stores coming in
		if (vp->storesTo.size() > 0) { //eg. store a1 b; store a2 b (b=vp)
			boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
			
			e_beg = boost::in_edges(vp->number, G).first;	// edge iterator begin
			e_end = boost::in_edges(vp->number, G).second;  // edge iterator end
			
			std::set<int> deleteMe;
			// iterate through the edges to find matching opcode
			for(; e_beg != e_end; ++e_beg) {
				int opCode = get(get(edge_iore, G),*e_beg);
				
				if (opCode == Instruction::Load) {
					NodeProps *sourceVP = get(get(vertex_props, G), source(*e_beg, G));
					int sourceLine = sourceVP->line_num;
#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"For sourceVP(Load): "<<sourceVP->name<<" "<<sourceLine<<std::endl;
					blame_info<<"---"<<std::endl;
#endif 
					std::set<NodeProps *>::iterator set_vp_i;
					for (set_vp_i = vp->storesTo.begin(); set_vp_i != vp->storesTo.end(); set_vp_i++) {
						NodeProps *storeVP = (*set_vp_i);
#ifdef DEBUG_GRAPH_COLLAPSE
						blame_info<<"storeVP: "<<storeVP->name<<" - Store Lines - ";
                        std::set<int>::iterator set_i_i;
	                    for (set_i_i = storeVP->storeLines.begin(); set_i_i != storeVP->storeLines.end(); set_i_i++)
		                    blame_info<<*set_i_i<<" ";
	
						blame_info<<" ; Border Lines - ";
	                    for (set_i_i = storeVP->borderLines.begin(); set_i_i != storeVP->borderLines.end(); set_i_i++)
		                    blame_info<<*set_i_i<<" ";

						blame_info<<std::endl;
#endif 
						if ((storeVP->storeLines.count(sourceLine) >0 && resolveStoreLine(storeVP, sourceVP)) ||
							(storeVP->borderLines.count(sourceLine) >0 && 
                            resolveBorderLine(storeVP, sourceVP, vp, sourceLine))) {
							if (sourceVP->llvm_inst != NULL) {
								int newPtrLevel = 99;
								if (isa<Instruction>(sourceVP->llvm_inst)) {
									const llvm::Type *origT = 0;		
									Instruction *pi = cast<Instruction>(sourceVP->llvm_inst);	
									origT = pi->getType();	
									newPtrLevel = pointerLevel(origT,0);
								}
								else if (isa<ConstantExpr>(sourceVP->llvm_inst)) {
									const llvm::Type *origT = 0;		
									ConstantExpr *ce = cast<ConstantExpr>(sourceVP->llvm_inst);
									origT = ce->getType();
									newPtrLevel = pointerLevel(origT, 0);
								}
								
								//if (newPtrLevel == 0) 
                                //example:store C, b; a = load b;
                                //changed by Hui 03/24/16:we shouldn't remove a->b if C is a constant, since it could be just an initialization of b
                                if(newPtrLevel ==0 && storeVP->name.find("Constant+")==std::string::npos){
									deleteMe.insert(sourceVP->number);
#ifdef DEBUG_GRAPH_COLLAPSE
            			            blame_info<<"Remove edge(hui) from "<<sourceVP->name<<" to "<<vp->name<<" [8]"<<std::endl;
#endif 
                                }
							}
							
							tie(ed, inserted) = add_edge(sourceVP->number, storeVP->number, G);
							if (inserted){
                                blame_info<<"RESOLVED_L_S_OP added between "<<sourceVP->name<<" and "<<storeVP->name<<endl;
								edge_type[ed] = RESOLVED_L_S_OP;//load_store op
                                //TOCHECK: added by Hui 03/24/16: if sourceVP's Ptrlevel==0, we still want to keep the RLS relation
                                //However, later resolvePointers won't add them to resolvedLSFrom/resolvedLS, so we add it here in advance
                                storeVP->resolvedLS.insert(sourceVP);
                                sourceVP->resolvedLSFrom.insert(storeVP);
                            }
						}
					}
#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"---"<<std::endl;
					blame_info<<std::endl;
#endif 
				}
			}
			
			std::set<int>::iterator set_i_i;
			for (set_i_i = deleteMe.begin(); set_i_i != deleteMe.end(); set_i_i++) {
				remove_edge((*set_i_i), vp->number, G);
			}
		}
	}
}

bool FunctionBFC::resolveStoreLine(NodeProps *storeVP, NodeProps *sourceVP)
{
#ifdef DEBUG_GRAPH_COLLAPSE
	blame_info<<"In resolveStoreLine storeVP:"<<storeVP->name<<" ln="<<storeVP->line_num<<" lNO="<<storeVP->lineNumOrder
        <<"  sourceVP:"<<sourceVP->name<<" ln="<<sourceVP->line_num<<" lNO="<<sourceVP->lineNumOrder<<std::endl;
#endif 
    if(storeVP->line_num < sourceVP->line_num)
        return true;
    else if(storeVP->line_num == sourceVP->line_num){ //cases like: findme = findme*10; both load and store
        
        //if(sourceVP->lineNumOrder > storeVP->lineNumOrder) //are in the same line, but THE load shouldn't depend on
        //   return true;                                   //THE store since the store comes after the load
        //
        //Changed by Hui 05/10/16: the above condition only effect on cases when both
        //are reigisters, when one of them is variable, then the lineNumOrder isn't
        //reliable, we need to find out the corresponding Store/Load, and use their
        //lineNumOrder, here storeVP->line_num == sourceVP->line_num
        int commLine = sourceVP->line_num;
        if ((sourceVP->loadLineNumOrder).find(commLine) != (sourceVP->loadLineNumOrder).end() &&
            (storeVP->storeLineNumOrder).find(commLine) != (storeVP->storeLineNumOrder).end() &&
            (sourceVP->loadLineNumOrder)[commLine] > (storeVP->storeLineNumOrder)[commLine])
            return true;
        else{ 
            blame_info<<"resolveStoreLine fails because lineNumOrder"<<endl;
            return false; //the store comes after the load
        }
    }
    else {// when storeVP->line_num > sourceVP->line_num, clearly false
        blame_info<<"resolveStoreLine fails because line_num"<<endl;
        return false;
    }
}

bool FunctionBFC::resolveBorderLine(NodeProps *storeVP, NodeProps *sourceVP, \
                					 NodeProps * origStoreVP, int sourceLine)
{
#ifdef DEBUG_GRAPH_COLLAPSE
	blame_info<<std::endl;
	blame_info<<"In resolveBorderLine "<<storeVP->name<<" "<<sourceVP->name<<" "<<origStoreVP->name<<std::endl;
#endif 
    //int storeVPNum = storeVP->lineNumOrder;
	std::vector<FuncStores *>::iterator vec_fs_i;
	
	for (vec_fs_i = allStores.begin(); vec_fs_i != allStores.end(); vec_fs_i++) {
		FuncStores *fs = *vec_fs_i;
#ifdef DEBUG_GRAPH_COLLAPSE		
		blame_info<<"FS: ";
		if (fs->receiver)
			blame_info<<"Receiver "<<fs->receiver->name<<" ";
		else
			blame_info<<"Receiver NULL ";
		if (fs->contents)
			blame_info<<"Contents "<<fs->contents->name<<" ";
		else
			blame_info<<"Contents NULL ";
		
		blame_info<<fs->line_num<<" "<<fs->lineNumOrder<<std::endl;
#endif 
		
		if (fs->receiver == origStoreVP && fs->line_num == sourceLine) {		
#ifdef DEBUG_GRAPH_COLLAPSE
			blame_info<<"Border Line between "<<storeVP->name<<" "<<sourceVP->name<<" "<<origStoreVP->name<<std::endl;
#endif 
            //Here, the fs is the store that kills the previous store, so
            //it should come after the load(sourceVP) so that the previous store
            //would be effect on sourceVP
			if (fs->lineNumOrder > sourceVP->lineNumOrder) {
#ifdef DEBUG_GRAPH_COLLAPSE
				blame_info<<"Border Cond resolved"<<std::endl;
#endif 
				return true;
			}
		}
	}
	
#ifdef DEBUG_GRAPH_COLLAPSE
	blame_info<<"Border Cond NOT resolved"<<std::endl;
#endif 
	
	return false;
}

// get original typeID of a pointer
unsigned FunctionBFC::getPointedTypeID(const llvm::Type *t)
{
	unsigned typeVal = t->getTypeID();
	if (typeVal == Type::PointerTyID)
		return getPointedTypeID(cast<PointerType>(t)->getElementType());
	else
		return typeVal;
}



void FunctionBFC::addImplicitEdges(Value *v, std::set<const char*, ltstr> &iSet,                        	 
        property_map<MyGraphType, edge_iore_t>::type edge_type, const char *vName, bool useName)
{
    bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
	std::string impName;
	char tempBuf[18];
	
	if (useName) {
		impName.insert(0, vName);
	}
	else if (v->hasName()) {
		impName = v->getName().str();
	}
	else {
		//sprintf(tempBuf, "0x%x", (unsigned)v);
        sprintf(tempBuf, "0x%x", v);
		std::string tempStr(tempBuf);
		impName.insert(0, tempStr);			
	}
	
	// Take Care of Implicit Edges
	for (std::set<const char*, ltstr>::iterator s_iter = iSet.begin(); s_iter != iSet.end(); s_iter++) { 
        std::string s_iterString(*s_iter);
		if (variables.count(impName)>0 && variables.count(s_iterString)>0) {
#ifdef DEBUG_GRAPH_IMPLICIT		
			blame_info<<"Adding implicit edges between "<<impName<<" and "<<*s_iter<<std::endl;
#endif 
			tie(ed, inserted) = add_edge(variables[impName]->number, variables[s_iterString]->number, G); 
			if (inserted)
				edge_type[ed] = IMPLICIT_OP;	
			else {
#ifdef DEBUG_ERROR
				blame_info<<"ERROR__(genEdges) - Insertion fail for implicit edge"<<std::endl;
#endif
				//std::cerr<<"Insertion fail for implicit edge\n";
			}
		}

		else
			return;
	}
}


void FunctionBFC::geDefault(Instruction *pi, std::set<const char*, ltstr> &iSet, 
							property_map<MyGraphType, vertex_props_t>::type props,
							property_map<MyGraphType, edge_iore_t>::type edge_type,
							int &currentLineNum)

{
    //bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed; 
	
	// Take care of implicit edges for default case 
	addImplicitEdges(pi, iSet, edge_type, NULL, false);
	
	std::string instName;
	char tempBuf[18];
	
	if (pi->hasName()) {
		instName = pi->getName().str();
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		std::string tempStr(tempBuf);
		instName.insert(0, tempStr);			
	}
	
	// Take care of Explicit Edges for non-load operations with 2+ operands
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		if (v->hasName() && pi->hasName()) {
			addEdge(pi->getName().data(), v->getName().data(), pi);
		}
		else {
			std::string opName;
			
			if (v->hasName()) {
				opName = v->getName().str();
			}
			else {
                //added by Hui : deal with constant operands
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
			      return;
		        }
		
		        char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
		        strcpy(vN,tempBuf);
		        vN[strlen(tempBuf)]='\0';
		        const char * vName = vN;
		
		        opName.insert(0, vName);
	          }
            
              else{
				sprintf(tempBuf, "0x%x", /*(unsigned)*/v);
				std::string tempStr(tempBuf);
				opName.insert(0, tempStr);
              }
			}
			
			addEdge(instName.c_str(), opName.c_str(), pi);
		}
	}
	
}
//added by Hui 08/20/15  Temporary solution for multi-level structures (2-level)
string FunctionBFC::getRealStructName(string rawStructVarName, Value *v, User *pi,
                                                                string instName)
{
    if (v->getValueID() == Value::ConstantIntVal) {
        blame_info<<"The instName of GEP(1): "<<instName<<std::endl;
        /////step1 done/////////////NOT very useful///////////////////////
        //if(rawStructVarName.find("localActor") != std::string::npos)
        //    return rawStructVarName; //temporary scheme, for the first level GEP

        Value *vLoad = *(pi->op_begin());
        if (isa<Instruction>(vLoad)) {
            blame_info<<"vLoad is an instruction\n";
            Instruction *vInstLoad = cast<Instruction>(vLoad);
            if(vInstLoad->getOpcode() == Instruction::Load){
                blame_info<<"vInstLoad is a Load instruction\n";
                Value *midTemp = vInstLoad->getOperand(0);
                /*if(midTemp->hasName())
                    NodeProps *nodeTemp=variables[midTemp->getName().str().c_str()];
                    if(nodeTemp->storeFrom != NULL)
                        blame_info<<"We have nodeStore now !\n";
                        NodeProps *nodeStore = nodeTemp->storeFrom;
                */
                Value::use_iterator ui, ue;
                for(ui=midTemp->use_begin(),ue=midTemp->use_end(); ui!=ue; ++ui){
                    if(Instruction *useInst = dyn_cast<Instruction>(*ui)){
                        if(useInst->getOpcode()==Instruction::Store){
                            blame_info<<"We find the store inst for call_tmp\n";
                            Value *storeFrom = *(useInst->op_begin());
                            if(isa<Instruction>(storeFrom)){
                                Instruction *instGEP = cast<Instruction>(storeFrom);
                                if(instGEP->getOpcode()==Instruction::GetElementPtr){
                                    blame_info<<"We find GEP inst !\n";
                                    Value *topStruct = *(instGEP->op_begin());
                                    string tempStr; //hold the return string
                                    if(topStruct->hasName()){ //TO CONTINUE: 12/28/15
                                      tempStr.insert(0,topStruct->getName().str());
                                      blame_info<<"GEP topStruct hasName: "<<tempStr<<endl;
                                      User::op_iterator op_i = instGEP->op_begin();
                                      if(instGEP->getNumOperands()>2){
                                        ++op_i; //first index
                                        ++op_i; //second index
                                        Value *offset = *op_i;
                                        if(v->getValueID()==Value::ConstantIntVal){
                                          ConstantInt *cO = (ConstantInt *)offset;
                                          int offNum = cO->getSExtValue();
					                      tempStr.insert(0, ".P.");
					
					                      char fNumStr[3];
					                      sprintf(fNumStr, "%d", offNum);
					                      tempStr.insert(0, fNumStr);
                                          return tempStr;
                                        }
                                        else {
                                          blame_info<<"Fail in Cond 0\n";
                                          return rawStructVarName; //to check: should I return x.P.tempStr ?
                                        }
                                      }
                                      else{
                                        blame_info<<"We are accesing one element in an array(topStruct hasName: "<<tempStr<<")\n";
                                        return tempStr;
                                      } 
                                    }
                                    else { //topStruct doesn't have a name(still a register)
                                      User::op_iterator op_i = instGEP->op_begin();
                                      Value *vLoad2 = *op_i; //vLoad2=topStruct
                                      char tempBuff2[18];
                                      sprintf(tempBuff2, "0x%x",vLoad2);
                                      string tempStr2(tempBuff2);
                                      tempStr.clear();
                                      tempStr.insert(0,tempStr2);  
                                      if(instGEP->getNumOperands()>2){
                                        ++op_i; //first index
                                        ++op_i; //second index
                                        Value *offset = *op_i;
                                        if(v->getValueID()==Value::ConstantIntVal){
                                          ConstantInt *cO = (ConstantInt *)offset;
                                          int offNum = cO->getSExtValue();
					                      tempStr.insert(0, ".P.");
					
					                      char fNumStr[3];
					                      sprintf(fNumStr, "%d", offNum);
					                      tempStr.insert(0, fNumStr);
                                          return tempStr;
                                        }
                                        else {
                                          blame_info<<"Fail in Cond -1\n";
                                          return rawStructVarName; //to check: should I return x.P.tempStr ?
                                        }
                                      }
                                      else{
                                        blame_info<<"We are accesing one element in an array(topStruct doesn't hasName\n";
                                        return tempStr;
                                      } 
                                    }//end of !topStruct->hasName()
                                }//end of upper level GEP inst   
                                else {
                                    blame_info<<"Fail in Cond 2\n";
                                    return rawStructVarName;
                                }
                            }
                            else {
                                blame_info<<"Fail in Cond 3\n";
                                return rawStructVarName;
                            }
                        }
                        else {
                            blame_info<<"Fail in Cond4\n";
                            continue; //if the Instruction isn't Store
                        }
                    }
                    else {
                        blame_info<<"Fail in Cond5\n";
                        continue; //if the use isn't an Instruction
                    }
                }
                blame_info<<"Searched all uses, didn't find Store\n";
                return rawStructVarName;
            }
            else {
                blame_info<<"Fail in Cond6\n";
                return rawStructVarName;
            }
        }
        else {
            blame_info<<"Fail in Cond7\n";
            return rawStructVarName;
        }
    }
    else {
        blame_info<<"Fail in Cond8\n";
        return rawStructVarName;
    }
}

//added by Hui 01/14/16: get the original value back-following the S->L chain
Value* FunctionBFC::getValueFromOrig(Instruction *vInstLoad) 
{
  Value *loadFrom = *(vInstLoad->op_begin());
  if(Instruction *loadFromInst = dyn_cast<Instruction>(loadFrom)){
    if(loadFromInst->getOpcode()==Instruction::GetElementPtr) //It means there's no more store->load chains before
      return vInstLoad;                                       //we reach to GEP,simply return this load inst
    //else: store r1, tmp;  r2=load tmp;(tmp is just Alloca, we need to do store-load backtrace)
    Value::use_iterator ui, ue;
    for(ui=loadFrom->use_begin(),ue=loadFrom->use_end(); ui!=ue; ++ui){
      if(Instruction *useInst = dyn_cast<Instruction>(*ui)){
        if(useInst->getOpcode()==Instruction::Store){
          Value *valueFrom = *(useInst->op_begin());
          if(Instruction *valueFromInst = dyn_cast<Instruction>(valueFrom)){
            if(valueFromInst->getOpcode()==Instruction::Load){
              blame_info<<"Start recursion inside getValueFromOrig"<<endl;
              return getValueFromOrig(valueFromInst); //recursively call this function
            }
            else return valueFrom;//valueFromInst is the GEP retVal,simply return it
          }
          else return valueFrom;//valueFrom isn't inst,so it's an exitVar(param/gv)
        }
      }
    }
    blame_info<<"Weird: shouldn't be here!"<<endl;
    return vInstLoad;
  }
  blame_info<<"loadFrom isn't an inst(even not alloca), we deal it after return"<<endl;
  return vInstLoad;
}

//added by Hui 01/14/16: try to get the uniqueNameAsField for a GEP field node
string FunctionBFC::getUpperLevelFieldName(string rawStructVarName, User *pi, string instName)
{
  blame_info<<"In getUpperLevelFieldName for inst "<<instName<<endl;
  Value *vBase = *(pi->op_begin()); //vBase's name should be equal to rawStructVarName
  //if GEP base already has its uniqueNameAsField, simply return it
  if(variables.count(rawStructVarName)>0){
    std::string baseName = rawStructVarName;
    if(variables[baseName]->uniqueNameAsField){
      std::string retName(variables[baseName]->uniqueNameAsField);
      blame_info<<"GEP base: "<<rawStructVarName<<"  already has the uniqueNameAsField, simply return it!"<<endl;
      return retName;
    }
  }
  //else: GEP base doesn't have a uniqueNameAsField yet, we back trace
  if(Instruction *vInstLoad = dyn_cast<Instruction>(vBase)){
    if(vInstLoad->getOpcode() == Instruction::Load){
      if(vInstLoad->hasName()){ //We've arrived at top level struct TOCHECK:should be moved to back?
        blame_info<<"should-be Top struct(1): vInstLoad="<<vInstLoad->getName().str()<<" rawStructVarName="<<rawStructVarName<<endl;
        std::string nodeName = vInstLoad->getName().str();//should be equal to rawStructVarName
        if(variables.count(nodeName)>0){
          if(variables[nodeName]->uniqueNameAsField){
            std::string retName(variables[nodeName]->uniqueNameAsField);
            return retName;
          }
          else
            return vInstLoad->getName().str();
        }
      }
      else{
        Value *valueFrom = getValueFromOrig(vInstLoad);
        blame_info<<"After getValueFromOrig, we have "<<valueFrom<<endl;
        if(valueFrom->hasName()){//It's an exitVar(passed-in param): SL...->GEP (TOCHECK)
          blame_info<<"should-be Top struct(2): valueFrom="<<valueFrom->getName().str()<<" rawStructVarName="<<rawStructVarName<<endl;
          std::string nodeName = valueFrom->getName().str();
          if(variables.count(nodeName)>0){
            if(variables[nodeName]->uniqueNameAsField){
              std::string retName(variables[nodeName]->uniqueNameAsField);
              return retName;
            }
            else
              return valueFrom->getName().str();
          }
        }
        else if(Instruction *valueFromInst = dyn_cast<Instruction>(valueFrom)){
          if(valueFromInst->getOpcode()==Instruction::GetElementPtr){//1st most common case: GEP->SL...->GEP
            //return the uniqueNameAsField of this GEP retVal
            char tempBuff[18];
            sprintf(tempBuff, "0x%x",valueFrom);
            string tempStr(tempBuff);
            blame_info<<"Upper struct(1): valueFrom="<<tempStr<<" rawStructVarName="<<rawStructVarName<<endl;
            std::string nodeName = tempStr;
            if(variables.count(nodeName)>0){
              if(variables[nodeName]->uniqueNameAsField){
                std::string retName(variables[nodeName]->uniqueNameAsField);
                return retName;
              }
              else
                blame_info<<"Error: register should have uniqueNameAsField"<<endl;
            }
          }
          else if(valueFromInst->getOpcode()==Instruction::Load){
            Value *loadFrom = *(valueFromInst->op_begin());
            if(loadFrom->hasName()){//It's an exitVar(globalVar): load->SL...->GEP (TOCHECK)
              blame_info<<"should-be Top struct(3): loadFrom="<<loadFrom->getName().str()<<" rawStructVarName="<<rawStructVarName<<endl;
              std::string nodeName = loadFrom->getName().str();
              if(variables.count(nodeName)>0){
                if(variables[nodeName]->uniqueNameAsField){
                  std::string retName(variables[nodeName]->uniqueNameAsField);
                  return retName;
                }
                else
                  return loadFrom->getName().str();
              }
            }
            else if(Instruction *loadFromInst = dyn_cast<Instruction>(loadFrom)){
              if(loadFromInst->getOpcode()==Instruction::GetElementPtr){//2nd most common case: GEP->load->(SL...)->GEP
                //return the uniqueNameAsField of this GEP retVal   
                char tempBuff2[18];
                sprintf(tempBuff2, "0x%x",loadFrom);
                string tempStr2(tempBuff2);
                blame_info<<"Upper struct(2): loadFrom="<<tempStr2<<" rawStructVarName="<<rawStructVarName<<endl;
                std::string nodeName2 = tempStr2;
                if(variables.count(nodeName2)>0){
                  if(variables[nodeName2]->uniqueNameAsField){
                    std::string retName(variables[nodeName2]->uniqueNameAsField);
                    return retName;
                  }
                  else
                    blame_info<<"Error2: register should have uniqueNameAsField"<<endl;
                }
              }
              else blame_info<<"Fail in case 0"<<endl;
            }
            else if(isa<ConstantExpr>(loadFrom)){
              //return variables[0x330b2d8.CE.44]->uniqueNameAsField
              blame_info<<"The loadFrom is a ConstantExpr, there is an embedded GEP!"<<endl;// so the load is like: %1=load GEP(%2, 0, 3)
              ConstantExpr *ceGEP = cast<ConstantExpr>(loadFrom);
              if(ceGEP->getOpcode() == Instruction::GetElementPtr){
                //since we don't know the currentlinenum of this ceGEP node when it's called
                //so we can't directly get its uniqueNameAsField, have to recompute it from the base
                Value *ceBase = *(ceGEP->op_begin());
                string baseName;
                if(ceBase->hasName())
                  baseName = ceBase->getName().str();
                else{
                  char tempBuff[18];
                  sprintf(tempBuff, "0x%x", ceBase);
                  string tempStr(tempBuff);
                  baseName.clear();
                  baseName.insert(0, tempStr);
                }
                baseName = getUpperLevelFieldName(baseName, ceGEP, "ceGEP");
                //now start to manully get loadFrom's uniqueNameAsField
                if(ceGEP->getNumOperands() == 3){
                  Value *fieldNum = ceGEP->getOperand(2);
                  if(fieldNum->getValueID() == Value::ConstantIntVal){
                    ConstantInt *cv = (ConstantInt *)fieldNum;
                    int number = cv->getSExtValue();
                    
                    string fieldName = baseName;
                    fieldName.insert(0, ".P.");
                    char fieldNumStr[3];
                    sprintf(fieldNumStr, "%d", number);
                    fieldName.insert(0, fieldNumStr);
                        
                    return fieldName;
                  }
                }
                else
                  blame_info<<"Error: ceGEP has only "<<ceGEP->getNumOperands()<<" Operands"<<endl;
              }
            }
          }
          else blame_info<<"Fail in case 1"<<endl;
        }
        else blame_info<<"Fail in case 2, not likely"<<endl;
      }
    }
    else if(vInstLoad->getOpcode() == Instruction::Alloca && vInstLoad->hasName()){
      blame_info<<"should-be Top struct(0): vInstAlloca="<<vInstLoad->getName().str()<<" rawStructVarName="<<rawStructVarName<<endl;
      std::string nodeName = vInstLoad->getName().str();//should be equal to rawStructVarName
      if(variables.count(nodeName)>0){
        if(variables[nodeName]->uniqueNameAsField){
          std::string retName(variables[nodeName]->uniqueNameAsField);
          blame_info<<"Shouldn't be here(1)"<<endl;
          return retName;
        }
        else
          return vInstLoad->getName().str();
      }
    }
    else blame_info<<"If inst before GEP isn't load/alloca, then we shouldn't be here!"<<endl;
  }
  else if(vBase->hasName()){ //not likely to happen
    blame_info<<"should-be Top struct(-1): vBase="<<vBase->getName().str()<<" rawStructVarName="<<rawStructVarName<<endl;
    std::string nodeName = vBase->getName().str();//should be equal to rawStructVarName
    if(variables.count(nodeName)>0){
      if(variables[nodeName]->uniqueNameAsField){
        std::string retName(variables[nodeName]->uniqueNameAsField);
        blame_info<<"Shouldn't be here(2)"<<endl;
        return retName;
      }
      else
        return vBase->getName().str();
    }
  }
  else blame_info<<"If GEP base isn't an inst nor hasName(), then we shouldn't be here!"<<endl;

  return rawStructVarName;
}
        

string FunctionBFC::geGetElementPtr(User *pi, std::set<const char*, ltstr> &iSet,property_map<MyGraphType, 
        vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type,int &currentLineNum)
{
	bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
	addImplicitEdges(pi, iSet, edge_type, NULL, false);
	
    int opCount = 0;
	std::string instName, opName;
	
	// Name of struct variable name (essentially op0)
	std::string structVarNameGlobal;
	
	char tempBuf[18];
	
	if (pi->hasName()) {
		instName = pi->getName().str();
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		std::string tempStr(tempBuf);
		instName.insert(0, tempStr);			
		
		if (isa<ConstantExpr>(pi)) {
			instName += ".CE";
			char tempBuf2[10];
			sprintf(tempBuf2, ".%d", currentLineNum);
			instName.append(tempBuf2);
		}
	}
	
	// Go through the operands
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		if (v->hasName() && variables[instName] && variables[v->getName().str()]) {
			tie(ed, inserted) = add_edge(variables[instName]->number,variables[v->getName().str()]->number,G);
			if (inserted) {
				if (opCount == 0) {
                    /////added by Hui////////////////////
                    //remove_edge(variables[instName.c_str()]->number,variables[v->getName().data()]->number,G);//blame relation should be struct -> field, e.g: localPeople->bd, bd->day
			        //tie(ed, inserted) = add_edge(variables[v->getName().data()]->number,variables[instName.c_str()]->number,G);
                    //if(inserted){
                    /////////////////////////////////////
					edge_type[ed] = GEP_BASE_OP;//pi->getOpcode();	previously existed
                        /*blame_info<<"reGenEdge YES from "<<v->getName().data()<<" to "<<instName;
                        blame_info<<std::endl;
                    }
                    else {
                        blame_info<<"rGenEdge Failed from "<<v->getName().data()<<" to"<<instName;
                        blame_info<<std::endl;
                    }*/
                    //////////////////////////////////////////
					variables[instName]->pointsTo = variables[v->getName().str()];
					variables[v->getName().str()]->pointedTo.insert(variables[instName]);
#ifdef DEBUG_GRAPH_BUILD
					blame_info<<"GRAPH_(genEdge) - GEP "<<instName<<" points to "<<v->getName().data()<<std::endl;
#endif
                    //added by Hui 01/14/16
                    blame_info<<"GEP base "<<v->getName().data()<<" will have uniqueNameAsField of itself"<<endl;

                    if(!(variables[v->getName().str()]->uniqueNameAsField))
                      variables[v->getName().str()]->uniqueNameAsField = v->getName().data(); //topLevel struct name is itself
                    else{
                      blame_info<<"To check(v already has its uniqueNameAsField): old="<<variables[v->getName().str()]->uniqueNameAsField
                          <<"  new(its own name)="<<v->getName().data()<<endl;
                      if(strcmp(variables[v->getName().str()]->uniqueNameAsField, v->getName().data())!=0)
                        blame_info<<"Check! named node has a different uniqueNameASField!"<<endl;
                    }
					//structVarNameGlobal.clear();
					structVarNameGlobal.insert(0, v->getName().str());//only for comparison, not real useful
					opCount++;
				}
				else if (opCount == 1) {
					edge_type[ed] = GEP_OFFSET_OP;
					opCount++;
#ifdef DEBUG_GRAPH_BUILD
                    blame_info<<"GEP_OFFSET_OP: "<<v->getName().data()<<" to inst: "<<instName<<endl;
#endif
                    ////added by Hui 01/14/16/////////
                    if(pi->getNumOperands()==2){//we are accessing an element in an array
                      string upperStructName = getUpperLevelFieldName(structVarNameGlobal,pi,instName);
                      upperStructName.insert(0,"I.");
                      //const char *strAlloc = (const char*)malloc(sizeof(char)*(upperStructName.length()+1));
                      //strcpy((char*)strAlloc, upperStructName.c_str());
                      string strAllocStr(upperStructName);
                      const char *strAlloc = strAllocStr.c_str();
                      variables[instName]->uniqueNameAsField = strAlloc;
                      blame_info<<"(1)Inst "<<instName<<" has uniqueNameAsField: "<<strAlloc<<endl;
                      
                      //std::string strAllocStr(strAlloc);
                      if (cpHash.count(strAllocStr)) {
                        #ifdef DEBUG_GRAPH_COLLAPSE
                        blame_info<<"Collapsable field already exists(1).  Add inst name "<<instName<<" to collapsable pairs."<<std::endl;
                        #endif 
                        
                        CollapsePair *cp = new CollapsePair();
                        cp->nameFieldCombo = strAlloc;
                        cp->collapseVertex = variables[instName];
                        cp->destVertex = cpHash[strAllocStr];
                        collapsePairs.push_back(cp);
 	
#ifdef DEBUG_GRAPH_COLLAPSE
            			blame_info<<"Remove edge(hui) from "<<instName<<" to "<<v->getName().data()<<" [3]"<<std::endl;
#endif 
						remove_edge(variables[instName]->number,variables[v->getName().str()]->number,G);//TOCHECK: whether we need to remove_edge
                      }
                      else {
                        //collapsableFields.insert(structVarName);
                        cpHash[strAllocStr] = variables[instName]; //cpHash only insert here
                        #ifdef DEBUG_GRAPH_COLLAPSE
                        blame_info<<"Collapsable field does not exist(1).  Create field and make inst name "<<instName<<" destination node."<<std::endl;
                        #endif 
                      }
                    }
				}
				else if (opCount == 2){//barely happen in this case 
					edge_type[ed] = GEP_S_FIELD_VAR_OFF_OP;
					opCount++;
#ifdef DEBUG_GRAPH_BUILD
                    blame_info<<"GEP_S_FIELD_VAR_OFF_OP: "<<v->getName().data()<<" to inst: "<<instName<<endl;
#endif
				}
			}
			else {
#ifdef DEBUG_ERROR			
				std::cerr<<"Insertion fail in genEges for "<<instName<<" to "<<v->getName().data()<<std::endl;
#endif 
			}
		} //v hasName() = true
		else if (v->getValueID() == Value::ConstantIntVal) {
			if (opCount == 0) {
				opCount++;
				continue;
			}
            else if (opCount == 1){
                opCount++;
                if(pi->getNumOperands()==2){//we are accessing an element in an array, rarely happen in this case
                  string upperStructName = getUpperLevelFieldName(structVarNameGlobal,pi,instName);
                  upperStructName.insert(0,"I.");
                  //const char *strAlloc = (const char*)malloc(sizeof(char)*(upperStructName.length()+1));
                  //strcpy((char*)strAlloc, upperStructName.c_str());
                  std::string strAllocStr(upperStructName);
                  const char *strAlloc = strAllocStr.c_str();
                  variables[instName]->uniqueNameAsField = strAlloc;
                  blame_info<<"(2)Inst "<<instName<<" has uniqueNameAsField: "<<strAlloc<<endl;

                  //std::string strAllocStr(strAlloc);
                  if (cpHash.count(strAllocStr)) {
                    #ifdef DEBUG_GRAPH_COLLAPSE
                    blame_info<<"Collapsable field already exists(2).  Add inst name "<<instName<<" to collapsable pairs."<<std::endl;
                    #endif 
                    
                    CollapsePair *cp = new CollapsePair();
                    cp->nameFieldCombo = strAlloc;
                    cp->collapseVertex = variables[instName];
                    cp->destVertex = cpHash[strAllocStr];
                    collapsePairs.push_back(cp);
                  }
                  else {
                    //collapsableFields.insert(structVarName);
                    cpHash[strAllocStr] = variables[instName]; //cpHash only insert here
                    #ifdef DEBUG_GRAPH_COLLAPSE
                    blame_info<<"Collapsable field does not exist(2).  Create field and make inst name "<<instName<<" destination node."<<std::endl;
                    #endif 
                  }
                }
                continue;
            }

			ConstantInt *cv = (ConstantInt *)v;
			int number = cv->getSExtValue();
			
			char tempBuf[64];
			sprintf (tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, opCount, Instruction::GetElementPtr);		
			char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
			strcpy(vN,tempBuf);
			vN[strlen(tempBuf)]='\0';
			const char *vName = vN;
            std::string vNameStr(vName);
			
			if (variables.count(instName) && variables.count(vNameStr)) {
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Adding edge from "<<instName<<" to "<<vName<<std::endl;
#endif 
				tie(ed, inserted) = add_edge(variables[instName]->number,variables[vNameStr]->number,G);
				
				if (inserted) {
					edge_type[ed] = GEP_S_FIELD_OFFSET_OP + number;
#ifdef ENABLE_FORTRAN
#ifdef HUI_CHPL //still temporary, we back follow GEP->store->load->GEP
                    structVarNameGlobal = //can be changed to recursively/iteratively call
                        //getRealStructName(structVarNameGlobal,v,pi,instName);
                        getUpperLevelFieldName(structVarNameGlobal,pi,instName);
#endif
					std::string structVarName = structVarNameGlobal;
					structVarName.insert(0, ".P.");
					
					char fieldNumStr[3];
					sprintf(fieldNumStr, "%d", number);
					structVarName.insert(0, fieldNumStr);
#ifdef HUI_C
                ///added by Hui, temporary way to solve multi-level structures////
                    char lastChar = *structVarName.rbegin();
                    while('0'<=lastChar && lastChar<='9'){
                        structVarName = structVarName.substr(0,structVarName.length()-1);
                        lastChar = *structVarName.rbegin();
                    }
#endif
					blame_info<<"structVarName is "<<structVarName<<std::endl;
                    //////////////////////////////////////////////////////////////
					//const char *strAlloc = (const char *) malloc(sizeof(char) *(structVarName.length() + 1));				
					//strcpy((char *)strAlloc,structVarName.c_str()); //strAlloc: 0.P.localPeople
                    std::string strAllocStr(structVarName);
                    const char *strAlloc = strAllocStr.c_str();
                    //for name in People
					#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"Name of collapsable field candidate is "<<structVarName<<" for "<<instName<<std::endl;
					#endif 

                    /////////added by Hui 01/14/16////////////////////
                    NodeProps *instNode = variables[instName];
                    instNode->uniqueNameAsField = strAlloc;
                    blame_info<<"(3)Inst "<<instName<<" has uniqueNameAsField: "<<strAlloc<<endl;
                    //////////////////////////////////////////////////

                    //std::string strAllocStr(strAlloc);
					if (cpHash.count(strAllocStr)) {
					    #ifdef DEBUG_GRAPH_COLLAPSE
						blame_info<<"Collapsable field already exists.  Add inst name "<<instName<<" to collapsable pairs."<<std::endl;
						#endif 
						
						CollapsePair *cp = new CollapsePair();
						cp->nameFieldCombo = strAlloc;
						cp->collapseVertex = variables[instName];
						cp->destVertex = cpHash[strAllocStr];
						collapsePairs.push_back(cp);
						
#ifdef DEBUG_GRAPH_COLLAPSE //added remove[4.5] by Hui 04/05/16: when field redundant, we need to remove the edge from it to the base, either
            			blame_info<<"Remove edge(hui) from "<<instName<<" to "<<vName<<" [4]"<<std::endl;
                        //blame_info<<"Remove edge(hui) from "<<instName<<" to "<<instNode->pointsTo->name<<" [4.5]"<<std::endl;
#endif 
						remove_edge(variables[instName]->number,variables[vNameStr]->number,G);
                        //remove_edge(instNode->number, instNode->pointsTo->number, G);


					}
					else {
						//collapsableFields.insert(structVarName);
						cpHash[strAllocStr] = variables[instName]; //cpHash only insert here
						#ifdef DEBUG_GRAPH_COLLAPSE
						blame_info<<"Collapsable field does not exist.  Create field and make inst name "<<instName<<" destination node."<<std::endl;
						#endif 
					}
#endif					
				}

				else {
#ifdef DEBUG_ERROR			
					std::cerr<<"Insertion fail, GEP struct field const(F)"<<" for "<<instName<<std::endl;
#endif 
				}
			}
			else {
			    #ifdef DEBUG_ERROR
				blame_info<<"Error adding edge with(not found in variables) "<<vName<<" "<<variables.count(vNameStr)<<" and "<<instName<<" ";
				blame_info<<variables.count(instName)<<std::endl;
			    #endif 
				return instName;
			}				
		}

		else {
			char tempBuff[18];
			
			sprintf(tempBuff, "0x%x", /*(unsigned)*/v);
			std::string tempStr(tempBuff);
			opName.clear();
			opName.insert(0, tempStr);
			//structVarNameGlobal not really useful, just for comparison
			if (opCount == 0) {
				structVarNameGlobal.insert(0, tempStr);
			}
			
			if (variables.count(instName) > 0 && variables.count(opName) > 0) {
				tie(ed, inserted) = add_edge(variables[instName]->number,variables[opName]->number,G);
				if (inserted) {
					if (opCount == 0) {
						edge_type[ed] = GEP_BASE_OP;//pi->getOpcode();	
						variables[instName]->pointsTo = variables[opName];
						variables[opName]->pointedTo.insert(variables[instName]);
#ifdef DEBUG_GRAPH_BUILD
						blame_info<<"GRAPH_(genEdge) - GEP(2) "<<instName.c_str()<<" points to "<<opName.c_str()<<std::endl;
#endif
						opCount++;
                        //added by Hui 01/14/16/////////////////////
                        string upperStructName = getUpperLevelFieldName(structVarNameGlobal,pi,instName);
                        //const char *strAlloc = (const char*)malloc(sizeof(char)*(upperStructName.length()+1));
                        //strcpy((char*)strAlloc, upperStructName.c_str());
                        std::string strAllocStr(upperStructName);
                        const char *strAlloc = strAllocStr.c_str();
                        blame_info<<"GEP base "<<opName<<" will have uniqueNameAsField: "<<strAlloc<<endl;

                        if(!(variables[opName]->uniqueNameAsField))//very likely, unless in 2nd GEP of two successive GEPs
                          variables[opName]->uniqueNameAsField = strAlloc; //topLevel struct name is itself
                        else{
                          blame_info<<"To check(opName already has its uniqueNameAsField)(2): old="<<
                              variables[opName]->uniqueNameAsField<<"  new="<<strAlloc<<endl;
                          if(strcmp(variables[opName]->uniqueNameAsField, strAlloc)!=0){ //shouldn't happen
                            blame_info<<"Error: register can't hold retval from 2 diff GEPs!"<<endl;
                            variables[opName]->uniqueNameAsField = strAlloc;
                          }
                        }
                        ///////////////////////////////////////////////
					}
					else if (opCount == 1) {
						edge_type[ed] = GEP_OFFSET_OP;
						opCount++;
#ifdef DEBUG_GRAPH_BUILD
                        blame_info<<"GEP_OFFSET_OP(2): "<<opName<<" to inst: "<<instName<<endl;
#endif
                        ////added by Hui 01/14/16/////////
                        if(pi->getNumOperands()==2){//we are accessing an element in an array
                          string upperStructName = getUpperLevelFieldName(structVarNameGlobal,pi,instName);
                          upperStructName.insert(0,"I.");
                          //const char *strAlloc = (const char*)malloc(sizeof(char)*(upperStructName.length()+1));
                          //strcpy((char*)strAlloc, upperStructName.c_str());
                          std::string strAllocStr(upperStructName);
                          const char *strAlloc = strAllocStr.c_str();
                          variables[instName]->uniqueNameAsField = strAlloc;
                          blame_info<<"(4)Inst "<<instName<<" has uniqueNameAsField: "<<strAlloc<<endl;

                          //std::string strAllocStr(strAlloc);
                          if (cpHash.count(strAllocStr)) {
                            #ifdef DEBUG_GRAPH_COLLAPSE
                            blame_info<<"Collapsable field already exists(1).  Add inst name "<<instName<<" to collapsable pairs."<<std::endl;
                            #endif 
                        
                            CollapsePair *cp = new CollapsePair();
                            cp->nameFieldCombo = strAlloc;
                            cp->collapseVertex = variables[instName];
                            cp->destVertex = cpHash[strAllocStr];
                            collapsePairs.push_back(cp);
#ifdef DEBUG_GRAPH_COLLAPSE
            			    blame_info<<"Remove edge(hui) from "<<instName<<" to "<<opName<<" [5]"<<std::endl;
#endif 
						    remove_edge(variables[instName]->number,variables[opName]->number,G);//TOCHECK:whether we need to remove_edge
                          }
                          else {
                            //collapsableFields.insert(structVarName);
                            cpHash[strAllocStr] = variables[instName]; //cpHash only insert here
                            #ifdef DEBUG_GRAPH_COLLAPSE
                            blame_info<<"Collapsable field does not exist(1).  Create field and make inst name "<<instName<<" destination node."<<std::endl;
                            #endif 
                          }
                        }
                    }
					else if (opCount == 2) {//barely happen in this case
						edge_type[ed] = GEP_S_FIELD_VAR_OFF_OP;
						opCount++;
#ifdef DEBUG_GRAPH_BUILD
                        blame_info<<"GEP_S_FIELD_VAR_OFF_OP(2): "<<opName<<" to inst: "<<instName<<endl;
#endif
					}
				}
				else { //not inserted
#ifdef DEBUG_ERROR			
					std::cerr<<"Insertion fail in genEges for "<<instName<<" to "<<v->getName().data()<<std::endl;
#endif 
				}
			}
			else {	
#ifdef DEBUG_ERROR
				blame_info<<"Variables can't find value of "<<instName<<" "<<variables.count(instName);
				blame_info<<" or "<<opName<<" "<<variables.count(opName)<<std::endl;
#endif 			
			}
		} // end hasName() else 
	} // end for 
	
	return instName;
	
} 


void FunctionBFC::geCall(Instruction *pi, std::set<const char*, ltstr> &iSet,
							property_map<MyGraphType, vertex_props_t>::type props,
							property_map<MyGraphType, edge_iore_t>::type edge_type,
							int &currentLineNum, std::set<NodeProps *> &seenCall)
{
    //bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
    /////added by Hui,trying to get the called function///////////
    llvm::CallInst *cpi = cast<CallInst>(pi);
    llvm::Function *calledFunc = cpi->getCalledFunction();
    if(calledFunc != NULL && calledFunc->hasName())
        blame_info<<"In geCall, calledFunc's name = "<<calledFunc->getName().data();
    blame_info<<"  pi->getNumOperands()="<<pi->getNumOperands()<<std::endl;
    //////////////////////////////////////////////////////////
    int callNameIdx = pi->getNumOperands()-1; //called func is the last operand of this inst
    Value *call_name = pi->getOperand(callNameIdx);
	const char *callNameStr;
	bool isTradName = true;
    if(call_name->hasName())
        callNameStr = call_name->getName().data();
	//else: !call_name->hasName()
    else if (call_name->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"call_name is ConstantExpr"<<std::endl;
#endif
		if (isa<ConstantExpr>(call_name)) {
			ConstantExpr *ce = cast<ConstantExpr>(call_name);
			User::op_iterator op_i = ce->op_begin();
			for (; op_i != ce->op_end(); op_i++) {
				Value *funcVal = *op_i;			
				if (isa<Function>(funcVal)) {
					Function *embFunc = cast<Function>(funcVal);
					//blame_info<<"Func "<<embFunc->getName()<<std::endl;
					isTradName = false;
					callNameStr = embFunc->getName().data();
				}
			}
		}
	}
    //TODO: Following case is funcCall is a register, most likely is the object functions
    //NEED to figure out a way to handle
    else if (!call_name->hasName() && call_name->getValueID() != Value::ConstantExprVal) {
        blame_info<<"We met a func call with no name !(register)"<<endl;
        return;
    }

	
	if(strstr(call_name->getName().data(),"llvm.dbg") != NULL){
        blame_info<<"We don't deal with intrinsic func calls"<<endl;
		return;
    }

	
	//bool foundFunc = true;
	int sizeSuffix = 0;
	std::string tempStr;
	
	while (1) {
		tempStr.clear();
		char tempBuf[1024];
		sprintf (tempBuf, "%s--%i", callNameStr, currentLineNum);		
		char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
		
		strcpy(vN,tempBuf);
		vN[strlen(tempBuf)]='\0';
		
		tempStr.insert(0,vN);
		for (int a = 0; a < sizeSuffix; a++) {
			tempStr.push_back('a');
		}

		
		//std::cout<<vNTemp<<" "<<sizeSuffix<<std::endl;
		if (variables.count(tempStr) > 0) {
			NodeProps *vpTemp = variables[tempStr];
			
			if (seenCall.count(vpTemp)) {
				sizeSuffix++;
				continue;
			}
			else {
				seenCall.insert(vpTemp);
				break;
			}
		}

		else
			break;
	}
	
	const char *vName = tempStr.c_str(); //calculated callName, with line#
	                                     //like: child--22a
	// Add edge for the return value container 
	// call_name - function call name
	// pi - variable that receives return value, if none then function is void

    //const char* retNameNew; //name of the return value if it existed
    std::string retName;
    if (pi->hasName()) {
		addEdge(pi->getName().data(), vName, pi);
        retName.insert(0,pi->getName().data());
        //retNameNew = temp.c_str();
	}
	else if (pi->hasNUsesOrMore(1)) {
		char tempBuf2[18];
		sprintf(tempBuf2, "0x%x", /*(unsigned)*/pi);
		std::string name(tempBuf2);
		
		addEdge(name.c_str(), vName, pi);
        retName.insert(0,name.c_str());
        //retNameNew = temp.c_str();
        //blame_info<<"The retName of call is "<<retName<<endl;
	}
	
	// Take Care of Implicit Edges
	if ((call_name->hasName() || !isTradName) && variables.count(tempStr)) 	
		addImplicitEdges(pi, iSet, edge_type, vName, true);
	
	// Add edges for all parameters of the function					
	int opCount = 0;
    //const char *opName;
    std::string opName;
    User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end();
    for ( ; op_i != op_e; ++op_i) {
	  Value *v = *op_i;
      if(opCount != callNameIdx) {//call_name is the last Op, we don't want to deal with it
#ifdef DEBUG_GRAPH_BUILD
        blame_info<<"op["<<opCount<<"]: "<<v->getName().data()<<endl;
#endif
		// Normal case where the parameter has a name and we have a normal function
		if (v->hasName()) {						
			addEdge(v->getName().data(), vName, pi);
            opName.insert(0,v->getName().data());
            //opName = temp.c_str();
		}
		// corner case where parameter is a constantExpr (GEP or BitCast)
		else if (v->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
			blame_info<<"Call param for "<<vName<<" is a constant"<<std::endl;
#endif 
			if (isa<ConstantExpr>(v)) {
				ConstantExpr *ce = cast<ConstantExpr>(v);	
			    if (ce->getOpcode() == Instruction::GetElementPtr) {
					Value *vReplace = ce->getOperand(0);
					if ((call_name->hasName()||!isTradName)&&vReplace->hasName()) {						
						addEdge(vReplace->getName().data(), vName, pi);
                        opName.insert(0,vReplace->getName().data());
                        //opName = temp.c_str();
					}
					// FORTRAN support TC: still need to preserve ??
					else if ((call_name->hasName() || !isTradName) && variables.count(tempStr) > 0) {
						std::string paramName;
						char tempBuf[18];
						
						sprintf(tempBuf, "0x%x", /*(unsigned)*/vReplace);
						std::string tempStr(tempBuf);
						paramName.insert(0, tempStr);			
						
						addEdge(paramName.c_str(), vName, pi);
                        opName.insert(0,paramName.c_str());
                        //opName = temp.c_str();
					}
				} // end check to see if param is GEP instruction
				else if (ce->getOpcode() == Instruction::BitCast) {
					Value *vReplace = ce->getOperand(0);
					if ((call_name->hasName()||!isTradName) && vReplace->hasName()) {
                        addEdge(vReplace->getName().data(), vName, pi);
                        opName.insert(0,vReplace->getName().data());
                        //opName = temp.c_str();
                    }
					// FORTRAN support
					else if ((call_name->hasName() || !isTradName) && variables.count(tempStr) > 0) {
						std::string paramName;
						char tempBuf[18];
						
						sprintf(tempBuf, "0x%x", /*(unsigned)*/vReplace);
						std::string tempStr(tempBuf);
						paramName.insert(0, tempStr);			
						
						addEdge(paramName.c_str(), vName, pi);
                        opName.insert(0,paramName.c_str());
                        //opName = temp.c_str();
					}
				} // end check to see if param is a BitCast Instruction
			}// end if isa<ConstantExpr>					
		} // end if ValueID is a ConstantExprVal
		// Case where params have no name (FORTRAN support)
		else if (variables.count(tempStr)) {
			char tempBuf[18];
			sprintf(tempBuf, "0x%x", /*(unsigned)*/v);
			std::string paramName(tempBuf);
			
			addEdge(paramName.c_str(), vName, pi);
            opName.insert(0,paramName.c_str());
            //opName = temp.c_str();
		}
#ifdef TEMP_FOR_MINIMD
        //added by Hui 05/09/16, for chpl__autoCopy* funcs, we need to collapse the related edges
        //since the retVal and param are exactly the same. 
        if(strstr(vName, "chpl__autoCopy") != NULL){
          //blame_info<<"retName="<<retName<<", opName="<<opName<<endl; //TEST ONLY, it's already changed here!
          if(variables.count(retName) && variables.count(opName)){
            blame_info<<"We have a CollapsePair("<<retName<<", "<<opName<<") of "<<vName<<endl;
            CollapsePair *cp = new CollapsePair();
            cp->collapseVertex = variables[opName];
            cp->destVertex = variables[retName];
            autoCopyCollapsePairs.push_back(cp);
          }
        }
#endif
      }
      opCount++;
	}// end for loop going through ops
}

void FunctionBFC::geLoad(Instruction *pi, std::set<const char*, ltstr> &iSet,
						property_map<MyGraphType, vertex_props_t>::type props,
						property_map<MyGraphType, edge_iore_t>::type edge_type,
						int & currentLineNum)
{	
	//bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
	std::string instName;
	char tempBuf[18];
	
	if (pi->hasName()) {
		instName = pi->getName();
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		std::string tempStr(tempBuf);
		
		instName.insert(0, tempStr);			
	}
	
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		if (v->hasName() && pi->hasName()) {
			addEdge(pi->getName().data(), v->getName().data(), pi);
		}
		else if (isa<ConstantExpr>(v)) {
			ConstantExpr *ce = cast<ConstantExpr>(v);	
			if (ce->getOpcode() == Instruction::GetElementPtr) {
				std::string opName = geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
				if(opName.length() == 0)
					return;

				if (variables.count(instName) && variables.count(opName)) {
					#ifdef DEBUG_GRAPH_BUILD
					blame_info<<"Adding edge(20) from "<<instName<<" to "<<opName<<std::endl;
					#endif
					addEdge(instName.c_str(), opName.c_str(), pi);
				}
			}

			else if (ce->getOpcode() == Instruction::BitCast) {
				Value *vRepl = ce->getOperand(0);
				
				if (vRepl == NULL) {
#ifdef DEBUG_ERROR
					std::cerr<<"Embedded double+ pointer for GEP Load"<<std::endl;
					blame_info<<"Embedded double+ pointer for GEP Load"<<std::endl;
#endif 
					return;
				}
				
				if (vRepl->hasName()){
					NodeProps *firstVP = variables[instName];
					NodeProps *secondVP = variables[vRepl->getName().str()];
					
					if (firstVP == NULL) {
#ifdef DEBUG_ERROR
						std::cerr<<"firstVP NULL in genEdges "<<std::endl;
						blame_info<<"firstVP NULL in genEdges "<<std::endl;
#endif 
						return;
					} 				
					if (secondVP == NULL) {
#ifdef DEBUG_ERROR
						std::cerr<<"secondVP NULL in genEdges "<<std::endl;
						blame_info<<"secondVP NULL in genEdges "<<std::endl;
#endif 
						return;
					}
					#ifdef DEBUG_GRAPH_BUILD
					blame_info<<"Adding edge(19) from "<<instName<<" to "<<vRepl->getName().data()<<std::endl;
					#endif 
					// TODO: Add debug integer param to addEdge so we know which addEdge it came from 
					addEdge(instName.c_str(), vRepl->getName().data(), pi);
				}
			}	
		}

		else {
			std::string opName;	
			if (v->hasName()) {
				opName = v->getName().str();
			}
			else {
				sprintf(tempBuf, "0x%x", /*(unsigned)*/v);
				std::string tempStr(tempBuf);
				
				opName.insert(0, tempStr);
			}
			addEdge(instName.c_str(), opName.c_str(), pi);
		}
	}	
}		


void FunctionBFC::geMemAtomic(Instruction *pi, std::set<const char*, ltstr> &iSet, 
                            property_map<MyGraphType, vertex_props_t>::type props, 
                            property_map<MyGraphType, edge_iore_t>::type edge_type,  
                            int &currentLineNum)
{
    //TODO: Add edges between pi and ops: mem atomic = load+store
#ifdef DEBUG_GRAPH_BUILD
    blame_info<<"gen edges for MemAtomic Inst: "<<pi->getOpcodeName()<<" at line"<<currentLineNum<<std::endl;
#endif

    if (pi->getOpcode() == Instruction::AtomicCmpXchg) {
        User::op_iterator op_i = pi->op_begin();
        Value *first = *op_i; //mem location: ptr
        Value *second = *(++op_i); //comp
        Value *third = *(++op_i); //value
    
        geAtomicLoadPart(first, third, pi, iSet, props, edge_type, currentLineNum);
        geAtomicStorePart(first, third, pi, iSet, props, edge_type, currentLineNum);
        //add edge between comp and ptr
	    char tempBuf[18];
    	
        std::string instName;
	    if (pi->hasName()) {
		    instName = pi->getName();
	    }
    	else {
	    	sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		    std::string tempStr(tempBuf);	
    		instName.insert(0, tempStr);			
    	}

        std::string cmpName;
        if (second->hasName())
            cmpName = second->getName().str();
        else {
			sprintf(tempBuf, "0x%x", /*(unsigned)*/second);
			std::string tempStr(tempBuf);
			cmpName.insert(0, tempStr);
		}
        
        //get the ptr representation and add edge
		if (first->hasName() && second->hasName()) {
			addEdge(first->getName().data(), cmpName.c_str(), pi);
		}
		else if (isa<ConstantExpr>(first)) {
			ConstantExpr *ce = cast<ConstantExpr>(first);	
			if (ce->getOpcode() == Instruction::GetElementPtr) {
				std::string ptrName = geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
				if(ptrName.length() == 0)
					return;

				if (variables.count(cmpName) && variables.count(ptrName)) {
					addEdge(ptrName.c_str(), cmpName.c_str(), pi);
				}
			}

			else if (ce->getOpcode() == Instruction::BitCast) {
				Value *vRepl = ce->getOperand(0);
				
				if (vRepl == NULL) {
#ifdef DEBUG_ERROR
					std::cerr<<"ptr bitcast get NULL"<<std::endl;
					blame_info<<"ptr bitcast get NULL"<<std::endl;
#endif 
					return;
				}
				
				if (vRepl->hasName()){
                    if (variables.count(cmpName) && \
                            variables.count(vRepl->getName().str())) 
					addEdge(vRepl->getName().data(), cmpName.c_str(), pi);
				}
			}	
		}

		else {
			std::string ptrName;	
			if (first->hasName()) {
				ptrName = first->getName().str();
			}
			else {
				sprintf(tempBuf, "0x%x", /*(unsigned)*/first);
				std::string tempStr(tempBuf);
				
				ptrName.insert(0, tempStr);
			}
			addEdge(ptrName.c_str(), cmpName.c_str(), pi);
        }
        //end of add edge between ptr and cmp
    } // end of if Opcode = AtomicCmpXchg

    else if (pi->getOpcode() == Instruction::AtomicRMW) {
        User::op_iterator op_i = pi->op_begin();
        Value *first = *op_i; // operation 
        Value *second = *(++op_i); //ptr
        Value *third = *(++op_i); //value
    
        geAtomicLoadPart(second, third, pi, iSet, props, edge_type, currentLineNum);
        geAtomicStorePart(second, third, pi, iSet, props, edge_type, currentLineNum);
    }
}


void FunctionBFC::geAtomicLoadPart(Value *ptr, Value *val, 
                            Instruction *pi, std::set<const char*, ltstr> &iSet, 
                            property_map<MyGraphType, vertex_props_t>::type props, 
                            property_map<MyGraphType, edge_iore_t>::type edge_type,  
                            int &currentLineNum)
{
	std::string instName;
	char tempBuf[18];
	
	if (pi->hasName()) {
		instName = pi->getName();
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/pi);
		std::string tempStr(tempBuf);
		instName.insert(0, tempStr);			
	}
	
    //// start adding edges //////
    if (ptr->hasName() && pi->hasName()) {
        addEdge(pi->getName().data(), ptr->getName().data(), pi);
    }
    else if (isa<ConstantExpr>(ptr)) {
        ConstantExpr *ce = cast<ConstantExpr>(ptr);	
        if (ce->getOpcode() == Instruction::GetElementPtr) {
            std::string opName = geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
            if(opName.length() == 0)
                return;
            if (variables.count(instName)&&variables.count(opName)){
                #ifdef DEBUG_GRAPH_BUILD
                blame_info<<"Add edge(21) from "<<instName<<"-"<<opName<<std::endl;
                #endif
                addEdge(instName.c_str(), opName.c_str(), pi);
            }
        }

        else if (ce->getOpcode() == Instruction::BitCast) {
            Value *vRepl = ce->getOperand(0);
            if (vRepl == NULL) {
#ifdef DEBUG_ERROR
                std::cerr<<"ptr bitcast get NULL in geAtomicLoadPart"<<std::endl;
                blame_info<<"ptr bitcast get NULL in geAtomicLoadPart"<<std::endl;
#endif 
                return;
            }
            
            if (vRepl->hasName()){
                NodeProps *firstVP = variables[instName];
                NodeProps *secondVP = variables[vRepl->getName().str()];
                
                if (firstVP == NULL) {
#ifdef DEBUG_ERROR
                    std::cerr<<"firstVP NULL in geAtomicloadPart"<<std::endl;
                    blame_info<<"firstVP NULL in geAtomicLoadPart"<<std::endl;
#endif 
                    return;
                } 				
                if (secondVP == NULL) {
#ifdef DEBUG_ERROR
                    std::cerr<<"secondVP NULL in geAtomicLoadPart"<<std::endl;
                    blame_info<<"secondVP NULL in geAtomicLoadPart"<<std::endl;
#endif 
                    return;
                }
                #ifdef DEBUG_GRAPH_BUILD
                blame_info<<"Adding edge(22) from "<<instName<<"-"<<vRepl->getName().data()<<std::endl;
                #endif 
                // TODO: Add debug integer param to addEdge so we know which addEdge it came from 
                addEdge(instName.c_str(), vRepl->getName().data(), pi);
            }
        }	
    }

    else {
        std::string opName;	
        if (ptr->hasName()) {
            opName = ptr->getName().str();
        }
        else {
            sprintf(tempBuf, "0x%x", /*(unsigned)*/ptr);
            std::string tempStr(tempBuf);
            
            opName.insert(0, tempStr);
        }
        addEdge(instName.c_str(), opName.c_str(), pi);
    }
}	


void FunctionBFC::geAtomicStorePart(Value *ptr, Value *val, 
                            Instruction *pi, std::set<const char*, ltstr> &iSet, 
                            property_map<MyGraphType, vertex_props_t>::type props, 
                            property_map<MyGraphType, edge_iore_t>::type edge_type,  
                            int &currentLineNum)
{
	//first=value,second=location to store
	std::string firstName, secondName;
	char tempBuf[18];
	
	if (ptr->hasName()) {
		secondName = ptr->getName();
		addImplicitEdges(ptr, iSet, edge_type, NULL, false);
	}
	else if (ptr->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"Second value in Store is ConstantExpr"<<std::endl;
#endif 
		// Just to make sure
		if (isa<ConstantExpr>(ptr)) {	
			ConstantExpr *ce = cast<ConstantExpr>(ptr);	
			if (ce->getOpcode() == Instruction::BitCast) {
				// Overwriting 
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Overwriting GEP Store second with "<<ce->getOperand(0)->getName().data()<<std::endl;
#endif 
				ptr = ce->getOperand(0);
				
				if (ptr->hasName()) 
					secondName= ptr->getName().str();
			
				else {
					sprintf(tempBuf, "0x%x", /*(unsigned)*/ptr);
					std::string tempStr(tempBuf);
					secondName.insert(0, tempStr);
				}
			}
			else if (ce->getOpcode() == Instruction::GetElementPtr)
				secondName= geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
		}	
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/ptr);
		std::string tempStr(tempBuf);
		secondName.insert(0, tempStr);
	}

    // deal with val
	if (val->hasName()){
		firstName = val->getName().str();
	    addImplicitEdges(val, iSet, edge_type, NULL, false);
	}

	else if (val->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"First value in Store is ConstantExpr"<<std::endl;
#endif 
		// Just to make sure
		if (isa<ConstantExpr>(val)) {
			ConstantExpr *ce = cast<ConstantExpr>(val);	
			if (ce->getOpcode() == Instruction::BitCast) {
				// Overwriting 
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Overwriting BitCast Store first with "<<ce->getOperand(0)->getName().data()<<std::endl;
#endif 
				val = ce->getOperand(0);
				if (val->hasName()) 
					firstName= val->getName().str();
				else {
					sprintf(tempBuf, "0x%x", /*(unsigned)*/val);
					std::string tempStr(tempBuf);
		    		firstName.insert(0, tempStr);
				}
			}
			else if (ce->getOpcode() == Instruction::GetElementPtr) {
				firstName= geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
			}
		}	
	}
	else if (val->getValueID() == Value::ConstantIntVal) {
		ConstantInt *cv = (ConstantInt *)val;
		int number = cv->getSExtValue();
		
		char tempBuf[64];
		sprintf(tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, 0, pi->getOpcode());		
		char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
	
		strcpy(vN,tempBuf);
		vN[strlen(tempBuf)]='\0';
		const char *vName = vN;
		
		firstName.insert(0, vName);

		addImplicitEdges(pi, iSet, edge_type, vName, true);
	} 
	else { // FORTRAN support
		sprintf(tempBuf, "0x%x", /*(unsigned)*/val);
		std::string tempStr(tempBuf);
		firstName.insert(0, tempStr);
	}
	
	if (variables.count(secondName) && variables.count(firstName))
		addEdge(secondName.c_str(), firstName.c_str(), pi);
}


void FunctionBFC::addEdge(const char *source, const char *dest, Instruction *pi)
{
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
    bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
#ifdef DEBUG_GRAPH_BUILD_EDGES
	blame_info<<"Adding edge between "<<source<<" and "<<dest<<" of type "<<pi->getOpcodeName()<<std::endl;
#endif
	
    std::string sourceStr(source), destStr(dest);
	if (variables.count(sourceStr) == 0 || variables.count(destStr) == 0) {
#ifdef DEBUG_ERROR
		blame_info<<"Variables can't find value of "<<source<<" "<<variables.count(sourceStr);
		blame_info<<" or "<<dest<<" "<<variables.count(destStr)<<std::endl;
#endif 			
		return;
	}
	
	tie(ed, inserted) = add_edge(variables[sourceStr]->number,variables[destStr]->number,G);
	if (inserted)
		edge_type[ed] = pi->getOpcode();
	else {
#ifdef DEBUG_ERROR
		blame_info<<"Insertion fail in genEdges for "<<source<<" to "<<dest<<std::endl;
		std::cerr<<"Insertion fail in genEdges for "<<source<<" to "<<dest<<std::endl;
#endif 
	}
}

void FunctionBFC::geStore(Instruction *pi, std::set<const char*, ltstr> &iSet,
							property_map<MyGraphType, vertex_props_t>::type props,
							property_map<MyGraphType, edge_iore_t>::type edge_type,
							int & currentLineNum)
{	
	//bool inserted;
	// graph_traits < MyGraphType >::edge_descriptor ed;
	User::op_iterator op_i = pi->op_begin();
	Value  *first = *op_i,  *second = *(++op_i);
	//first=value,second=location to store
	std::string firstName, secondName;
	
	char tempBuf[18];
	
	if (second->hasName()) {
		secondName = second->getName();
		// TODO: generate this automatically
		/*
		if (secondName.find("ierr") != std::string::npos)
		{
			std::cout<<"Ignoring ierr store"<<std::endl;
			return;
		}*/
		addImplicitEdges(second, iSet, edge_type, NULL, false);
	}
	else if (second->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"Second value in Store is ConstantExpr"<<std::endl;
#endif 
		// Just to make sure
		if (isa<ConstantExpr>(second)){
			
			ConstantExpr *ce = cast<ConstantExpr>(second);	
			if (ce->getOpcode() == Instruction::BitCast) {
				// Overwriting 
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Overwriting GEP Store second with "<<ce->getOperand(0)->getName().data()<<std::endl;
#endif 
				second = ce->getOperand(0);
				
				if (second->hasName()) 
					secondName= second->getName().str();
			
				else {
					sprintf(tempBuf, "0x%x", /*(unsigned)*/second);//TC: old:first
					std::string tempStr(tempBuf);
					secondName.insert(0, tempStr);
				}
			}
			else if (ce->getOpcode() == Instruction::GetElementPtr)
				secondName= geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
		}	
	}
	else {
		sprintf(tempBuf, "0x%x", /*(unsigned)*/second);
		std::string tempStr(tempBuf);
		secondName.insert(0, tempStr);
	}

	if (first->hasName()){
		firstName = first->getName().str();
		
		// TODO: generate this automatically
		/*
		if (firstName.find("ierr") != std::string::npos)
		{
			std::cout<<"Ignoring ierr store"<<std::endl;
			return;
		}*/
		
		//	addEdge(second->getName().data(), first->getName().data(), pi);
		addImplicitEdges(first, iSet, edge_type, NULL, false);
	}

	else if (first->getValueID() == Value::ConstantExprVal) {
#ifdef DEBUG_GRAPH_BUILD
		blame_info<<"First value in Store is ConstantExpr"<<std::endl;
#endif 
		// Just to make sure
		if (isa<ConstantExpr>(first)) {
			
			ConstantExpr * ce = cast<ConstantExpr>(first);	
			if (ce->getOpcode() == Instruction::BitCast) {
				// Overwriting 
#ifdef DEBUG_GRAPH_BUILD
				blame_info<<"Overwriting BitCast Store first with "<<ce->getOperand(0)->getName().data()<<std::endl;
#endif 
				first = ce->getOperand(0);
				
				if (first->hasName()) 
					firstName= first->getName().str();
				else {
					sprintf(tempBuf, "0x%x", /*(unsigned)*/first);
					std::string tempStr(tempBuf);
		    		firstName.insert(0, tempStr);
				}
			}
			else if (ce->getOpcode() == Instruction::GetElementPtr) {
				firstName= geGetElementPtr(ce, iSet, props, edge_type, currentLineNum);
			}
		}	
	}

	else if (first->getValueID() == Value::ConstantIntVal) {
		
		ConstantInt *cv = (ConstantInt *)first;
		int number = cv->getSExtValue();
		
		char tempBuf[64];
		sprintf(tempBuf, "Constant+%i+%i+%i+%i", number, currentLineNum, 0, pi->getOpcode());		
		char *vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
	
		strcpy(vN,tempBuf);
		vN[strlen(tempBuf)]='\0';
		const char *vName = vN;
		
		firstName.insert(0, vName);
		
		addImplicitEdges(pi, iSet, edge_type, vName, true);
	} 

	else if (first->getValueID() == Value::ConstantFPVal) {
		char tempBuf[70];
		ConstantFP * cfp = (ConstantFP *)first;
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
			return;
		}
		
		char * vN = (char *) malloc(sizeof(char)*(strlen(tempBuf)+1));
		strcpy(vN,tempBuf);
		vN[strlen(tempBuf)]='\0';
		const char * vName = vN;
		
		firstName.insert(0, vName);
		
		addImplicitEdges(pi, iSet, edge_type, vName, true);
	}

	else  // FORTRAN support
	{
		sprintf(tempBuf, "0x%x", /*(unsigned)*/first);
		std::string tempStr(tempBuf);
		firstName.insert(0, tempStr);
	}
	
	
	if (variables.count(secondName) && variables.count(firstName))
	//changed by Hui 08/21/15: switch first and second
    //    addEdge(firstName.c_str(), secondName.c_str(), pi);
        addEdge(secondName.c_str(), firstName.c_str(), pi);
}


void FunctionBFC::geBitCast(Instruction *pi, std::set<const char*, ltstr> &iSet,
							property_map<MyGraphType, vertex_props_t>::type props,
							property_map<MyGraphType, edge_iore_t>::type edge_type,
							int &currentLineNum)
{
	bool inserted;
    graph_traits < MyGraphType >::edge_descriptor ed;
	
	if (pi->getNumUses() == 1) {
		Value::use_iterator use_i = pi->use_begin();
		if (Instruction *usesBit = dyn_cast<Instruction>(*use_i)) {
			if (usesBit->getOpcode() == usesBit->Call) {
				User::op_iterator op_i = usesBit->op_begin();//,op_e = pi->op_end();
				Value *call_name = *op_i;
				op_i++;
				if(strstr(call_name->getName().data(),"llvm.dbg") != NULL)
					return;
			}
		}
	}
	
	for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) {
		Value *v = *op_i;
		if (v->hasName() && pi->hasName()) {
			tie(ed, inserted) = add_edge(variables[v->getName().str()]->number,variables[pi->getName().str()]->number,G);
			if (inserted)
				edge_type[ed] = pi->getOpcode();	
			else {	
#ifdef DEBUG_ERROR
				blame_info<<"ERROR__(genEdges) - Insertion fail in genEdges for ";
				blame_info<<v->getName().data()<<" to "<<pi->getName().data()<<std::endl;
				std::cerr<<"Insertion fail in genEdges for "<<v->getName().data()<<" to "<<pi->getName().data()<<std::endl;
#endif 
			}
		}
		// FORTRAN TC: Should this be preserved ?
		else {
			std::string opName, instName;
			char tempBuff[18];
			
			if (pi->hasName()) {
				instName = pi->getName().str();
			}
			else {
				sprintf(tempBuff, "0x%x", /*(unsigned)*/pi);
				std::string tempStr(tempBuff);
				instName.insert(0, tempStr);			
			}
			if (v->hasName()) {
				opName = v->getName().str();
			}
			else {
				sprintf(tempBuff, "0x%x", /*(unsigned)*/v);
				std::string tempStr(tempBuff);
				opName.insert(0, tempStr);
			}
			
			if (variables.count(opName) && variables.count(instName)) {
				tie(ed, inserted) = add_edge(variables[opName]->number,variables[instName]->number,G);
				//tie(ed,inserted) = add_edge(variables[pi->getName().data()]->number, variables[v->getName().data()]->number,G);
				if (inserted)
					edge_type[ed] = pi->getOpcode();		
			}
		}
	}      
}


void FunctionBFC::geBlank(Instruction *pi)
{
					#ifdef DEBUG_GRAPH_BUILD
	blame_info<<"Not generating any edges for opcode "<<pi->getOpcodeName()<<std::endl;
	#endif
}


void FunctionBFC::geInvoke()
{
#ifdef DEBUG_ERROR			
	std::cerr<<"Why are we calling invoke?"<<std::endl;
#endif 
	/*
	 //cout<<"Begin Invoke\n";
	 // Take care of Explicit Edges for non-load operations with 2+ operands
	 for (User::op_iterator op_i = pi->op_begin(), op_e = pi->op_end(); op_i != op_e; ++op_i) 
	 {
	 Value *v = *op_i;
	 
	 //cout<<"Invoke pi->getNameStart is "<<pi->getName().data()<<std::endl;
	 //cout<<"Invoke v->getNameStart is "<<v->getName().data()<<std::endl;
	 
	 if (v->hasName() && pi->hasName() && variables[pi->getName().data()] 
	 && variables[v->getName().data()] )  
	 {
	 
	 
	 tie(ed, inserted) = add_edge(variables[pi->getName().data()]->number,variables[v->getName().data()]->number,G);
	 if (inserted)
	 edge_type[ed] = pi->getOpcode();				
	 }
	 }	
	 
	 //cout<<"End Invoke\n";
	 */
	
}


void FunctionBFC::genEdges(Instruction *pi, std::set<const char*, ltstr> &iSet,
							property_map<MyGraphType, vertex_props_t>::type props,
							property_map<MyGraphType, edge_iore_t>::type edge_type,
							int &currentLineNum, std::set<NodeProps *> &seenCall)
{  
	if (pi == NULL) {
#ifdef DEBUG_ERROR
		std::cerr<<"GE -- pi is NULL"<<std::endl;
		blame_info<<"GE -- pi is NULL"<<std::endl;
#endif
		return;
	}
	
#ifdef DEBUG_GRAPH_BUILD
	if (pi->hasName())
		blame_info<<"GE Instruction "<<pi->getName().str()<<" "<<pi->getOpcodeName()<<std::endl;
	else
		blame_info<<"GE No name "<<pi->getOpcodeName()<<std::endl;
#endif 
	
	// Instruction Num 43, Call Instruction, we only look at the debug information right now
/*    if (pi->getOpcode() == Instruction::Call) {
		User::op_iterator op_i = pi->op_begin() ;
		Value *call_name = *op_i;
		op_i++;
		
		// Ignore debug calls within LLVM except for stoppoint which gives 
		// us line number information
		if(strstr(call_name->getName().data(),"llvm.dbg") == NULL)
		{ }
		else
		{
			if(strstr(call_name->getName().data(),"stoppoint") != NULL) {
	            ConstantInt * ci = dyn_cast<ConstantInt>(pi->getOperand(1));
	            if (ci)
					currentLineNum = ci->getZExtValue();
	        }
			return;
		}
	}
*/	// get the line number information for each instruction
	if (MDNode *N = pi->getMetadata("dbg")) {
        DILocation Loc(N);
        unsigned Line = Loc.getLineNumber();
        currentLineNum = Line;
    }

	if (pi->getOpcode() == Instruction::GetElementPtr) { //corresponding to firstGEPCheck in LLVMParser.cpp,
		Value *v = pi->getOperand(0);                   //probably only useful for fortran
			
		const llvm::Type *pointT = v->getType();
		unsigned typeVal = pointT->getTypeID();
		
		while (typeVal == Type::PointerTyID) {			
			pointT = cast<PointerType>(pointT)->getElementType();		
			typeVal = pointT->getTypeID();
		}
		
		if (typeVal == Type::StructTyID) {
			const llvm::StructType *type = cast<StructType>(pointT);
			string structNameFull = type->getStructName().str(); //parent module M
			#ifdef DEBUG_GRAPH_BUILD
			blame_info<<"GenEdges -- structNameFull -- "<<structNameFull<<std::endl;
			#endif 
			
			if (structNameFull.find("struct.descriptor_dimension") != \
                std::string::npos) {
#ifdef DEBUG_P
                cout<<structNameFull<<" find struct.descriptor_dimension"<<endl;
#endif
                return;
            }
			
			if (structNameFull.find("struct.array1") != std::string::npos) {
#ifdef DEBUG_P
                cout<<structNameFull<<" find struct.array1"<<endl;
#endif
	    		if (pi->getNumOperands() >= 3) {
					Value *vOp = pi->getOperand(2);
					if (vOp->getValueID() == Value::ConstantIntVal) {
						ConstantInt *cv = (ConstantInt *)vOp;
						int number = cv->getSExtValue();
						if (number != 0)//TOCHECK: why is this cond
							return;
					}
				}
			}	
		}
	}
	
	// 1
	// TERMINATOR OPS
	if (pi->getOpcode() == Instruction::Ret)
		geBlank(pi);
	else if (pi->getOpcode() == Instruction::Br)
		geBlank(pi);
	else if (pi->getOpcode() == Instruction::Switch)
		geBlank(pi);
    else if (pi->getOpcode() == Instruction::IndirectBr)
        geBlank(pi);
	else if (pi->getOpcode() == Instruction::Invoke)
		geInvoke();
	else if (pi->getOpcode() == Instruction::Resume)
		geBlank(pi);
	else if (pi->getOpcode() == Instruction::Unreachable)
		geBlank(pi);
	// END TERMINATOR OPS
	
	// BINARY OPS 
	else if (pi->getOpcode() == Instruction::Add)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
    else if (pi->getOpcode() == Instruction::FAdd)
        geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Sub)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FSub)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Mul)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FMul)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::UDiv)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::SDiv)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FDiv)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::URem)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::SRem)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FRem)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	// END BINARY OPS
	
	// LOGICAL OPERATORS 
	else if (pi->getOpcode() == Instruction::Shl)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::LShr)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::AShr)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::And)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Or)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Xor)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	// END LOGICAL OPERATORS
	
	// MEMORY OPERATORS
	else if (pi->getOpcode() == Instruction::Fence) //TC
		geBlank(pi);
	else if (pi->getOpcode() == Instruction::AtomicCmpXchg) //TC
		geMemAtomic(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::AtomicRMW) //TC
		geMemAtomic(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Alloca)
		geBlank(pi);
	else if (pi->getOpcode() == Instruction::Load)
		geLoad(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Store)
		geStore(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::GetElementPtr)	
		geGetElementPtr(pi, iSet, props, edge_type, currentLineNum);
	// END MEMORY OPERATORS
	
	// CAST OPERATORS
	else if (pi->getOpcode() == Instruction::Trunc)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::ZExt)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::SExt)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FPToUI)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FPToSI)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::UIToFP)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::SIToFP)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FPTrunc)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FPExt)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::PtrToInt)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::IntToPtr)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::BitCast)
		geBitCast(pi, iSet, props, edge_type, currentLineNum);
	// END CAST OPERATORS
	
	// OTHER OPERATORS
	else if (pi->getOpcode() == Instruction::ICmp)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::FCmp)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::PHI)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else if (pi->getOpcode() == Instruction::Call)
		geCall(pi, iSet, props, edge_type, currentLineNum, seenCall);
	else if (pi->getOpcode() == Instruction::Select)
		geDefault(pi, iSet, props, edge_type, currentLineNum);
	else {
#ifdef DEBUG_ERROR
		std::cerr<<"Other kind of opcode(1) is : "<<pi->getOpcodeName()<<"\n";
#endif 
	}		  
}


/*void FunctionBFC::transferImplicitEdges(int alloc_num, int bool_num)
{
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	bool inserted;
	
	// iterate through the out edges and delete edges to target, move other edges
	boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
	oe_beg = boost::out_edges(bool_num, G).first;		// edge iterator begin
	oe_end = boost::out_edges(bool_num, G).second;       // edge iterator end
	
	//std::vector<int> destNodes;
	
	// bool_num = delete Node
	// alloc_num = recipient Node
	NodeProps * rN =  get(get(vertex_props, G),alloc_num);
	NodeProps * dN =  get(get(vertex_props, G),bool_num);	
	
#ifdef DEBUG_GRAPH_IMPLICIT
	blame_info<<"Collapse_(transferImplicitEdges) - transfer lineNumber "<<dN->line_num<<" from "<<dN->name<<" to "<<rN->name<<std::endl;
#endif
	rN->lineNumbers.insert(dN->line_num);
	
						#ifdef DEBUG_LINE_NUMS
	blame_info<<"Inserting line number(3) "<<dN->line_num<<" to "<<rN->name<<std::endl;
	#endif
	
	if (dN->nStatus[CALL_PARAM])
	{
		dN->nStatus[CALL_PARAM] = false;
		rN->nStatus[CALL_PARAM] = true;
	}
	if (dN->nStatus[CALL_RETURN])
	{
		dN->nStatus[CALL_RETURN] = false;
		rN->nStatus[CALL_RETURN] = true;
	}
	if (dN->nStatus[CALL_NODE])
	{
		dN->nStatus[CALL_NODE] = false;
		rN->nStatus[CALL_NODE] = true;
	}
	
	std::set<FuncCall *>::iterator fc_i = dN->funcCalls.begin();
	
	for (; fc_i != dN->funcCalls.end(); fc_i++)
	{
		//FuncCall * fc = *fc_i;
		//std::cout<<"IMPLICIT Transferring call to node "<<alloc_num<<" from "<<fc->funcName<<" param num "<<fc->paramNumber<<std::endl;
		rN->addFuncCall(*fc_i);
	}
	
	
	
	for(; oe_beg != oe_end; ++oe_beg) 
	{
		
		int movedOpCode = get(get(edge_iore, G),*oe_beg);
		
		NodeProps * outTargetV = get(get(vertex_props,G), target(*oe_beg,G));
		
		//destNodes.push_back(outTargetV->number);
		
		// Don't want a self loop edge
		if (outTargetV->number != alloc_num )
		{
			tie(ed, inserted) = add_edge(alloc_num, outTargetV->number, G);
			if (inserted)
				edge_type[ed] = movedOpCode;	
		}
		
	}
	
	
	// iterate through the in edges and create new edge to destination, delete old edge
	boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
	ie_beg = boost::in_edges(bool_num, G).first;		// edge iterator begin
	ie_end = boost::in_edges(bool_num, G).second;       // edge iterator end
	
	for(; ie_beg != ie_end; ++ie_beg) 
	{
		int movedOpCode = get(get(edge_iore, G),*ie_beg);
		
		NodeProps * inTargetV = get(get(vertex_props,G), source(*ie_beg,G));
		
		// Don't want a self loop edge
		if (inTargetV->number != alloc_num )//&& movedOpCode > 0)
		{
			tie(ed, inserted) = add_edge(inTargetV->number, alloc_num, G);
			if (inserted)
				edge_type[ed] = movedOpCode;	
			
		}
		
	}
	
}*/


/*void FunctionBFC::deleteOldImplicit(int v_index)
{
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	//bool inserted;
	
	boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
	boost::graph_traits<MyGraphType>::in_edge_iterator i_beg, i_end;
	
	
	e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
	e_end = boost::out_edges(v_index, G).second;    // edge iterator end
	
	i_beg = boost::in_edges(v_index, G).first;		// edge iterator begin
	i_end = boost::in_edges(v_index, G).second;    // edge iterator end
	
	std::vector<int> destNodes;
	std::vector<int> sourceNodes;
	std::vector<int> allocNodes;
	
	// iterate through the edges trying to find a non-implicit instruction
	for(; e_beg != e_end; ++e_beg) 
	{
		int opCode = get(get(edge_iore, G),*e_beg);
		
		NodeProps * targetV = get(get(vertex_props,G), target(*e_beg,G));
		
		destNodes.push_back(targetV->number);
		
		if (opCode != 0)
		{
			if (targetV->llvm_inst != NULL)
			{
				if (isa<Instruction>(targetV->llvm_inst))
				{
					Instruction * pi = cast<Instruction>(targetV->llvm_inst);	
					
					
					//const llvm::Type * t = pi->getType();
					//unsigned typeVal = t->getTypeID();
					
					if (pi->getOpcode() != Instruction::Alloca)
						allocNodes.push_back(targetV->number);
				}
			}
		}
	}	
	
	for(; i_beg != i_end; ++i_beg) 
	{
		//int movedOpCode = get(get(edge_iore, G),*i_beg);
		NodeProps * inTargetV = get(get(vertex_props,G), source(*i_beg,G));
		sourceNodes.push_back(inTargetV->number);
	}
	
	
	std::vector<int>::iterator v_i;
	
	for (v_i = destNodes.begin(); v_i != destNodes.end(); ++v_i)
		remove_edge(v_index, *v_i, G);
	
	for (v_i = sourceNodes.begin(); v_i != sourceNodes.end(); ++v_i)
		remove_edge(*v_i, v_index, G);
	
	for (v_i = allocNodes.begin(); v_i != allocNodes.end(); ++v_i)
		deleteOldImplicit(*v_i);
}*/




void FunctionBFC::collapseIO()
{
	//bool inserted;
	//int collapsedEdges = 0;
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end; 
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		NodeProps *v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v) {
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in collapseIO\n";
#endif 
			continue;
		}
		boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
		e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
		e_end = boost::out_edges(v_index, G).second;    // edge iterator end
		
		std::vector< std::pair<int,int> > deleteEdges;
		// iterate through the edges to find matching opcode
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
			
			if (opCode == Instruction::Call || opCode == Instruction::Invoke) {										
				NodeProps * sourceV = get(get(vertex_props,G), source(*e_beg,G));
				NodeProps * targetV = get(get(vertex_props,G), target(*e_beg,G));
				//c++ "<<" basic_ostream
                //TOCHECK: Hui 12/14/15: should add "writeln"? why check targetV
                //since edge is like: writeln -> a ??
				std::string::size_type loc = targetV->name.find("basic_ostream", 0);
				std::string::size_type loc2 = targetV->name.find("SolsEi", 0);
				
				if (loc != std::string::npos) {
					std::string tmpStr("COUT");
					ExitProgram *ep = findOrCreateExitProgram(tmpStr);
					if (ep != NULL) {
						//ep->addVertex(v);
					}
					else {
#ifdef DEBUG_ERROR			
						std::cerr<<"WTF!  Why is this shiznit NULL!(2) for "<<v->name<<std::endl;	
#endif 
						continue;
					}
					
					std::pair<int, int> tmpPair(sourceV->number, targetV->number);
#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"Remove edge(hui) from "<<sourceV->name<<" to "<<targetV->name<<" [6]"<<std::endl;
#endif
					deleteEdges.push_back(tmpPair);
				}

				else if (loc2 != std::string::npos) {	
					std::string tmpStr("COUT-VAR-");
					if (v->llvm_inst != NULL) {
						if (isa<Instruction>(v->llvm_inst)) {
							Instruction *l_i = cast<Instruction>(v->llvm_inst);	
							if (l_i->getOpcode() == Instruction::Alloca) {
								tmpStr += sourceV->name;
								ExitProgram *ep = findOrCreateExitProgram(tmpStr);
								
								if (ep != NULL){
									//ep->addVertex(v);
								}
								else
								{
#ifdef DEBUG_ERROR			
									std::cerr<<"WTF!  Why is this shiznit NULL!(3) for "<<v->name<<std::endl;	
#endif 
									continue;
								}
							}
						}
					}
					
					std::pair<int, int> tmpPair(sourceV->number, targetV->number);
#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"Remove edge(hui) from "<<sourceV->name<<" to "<<targetV->name<<" [7]"<<std::endl;
#endif
                    deleteEdges.push_back(tmpPair);
				}
			}
		}
		
		std::vector<std::pair<int,int> >::iterator vecPair_i;
		
		for (vecPair_i = deleteEdges.begin();  vecPair_i != deleteEdges.end(); vecPair_i++) {
			std::pair<int,int> del = *vecPair_i;
			//std::cout<<"Remove edge for "<<del.first<<" to "<<del.second<<std::endl;
			remove_edge(del.first, del.second, G);
		}
	}
}




// recursively descend edges until a local variable (or parameter) is found 
/*void FunctionBFC::findLocalVariable(int v_index, int top_index)
{
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
	
	e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
	e_end = boost::out_edges(v_index, G).second;    // edge iterator end
	
	// iterate through the edges trying to find a non-implicit instruction
	for(; e_beg != e_end; ++e_beg) 
	{
		int opCode = get(get(edge_iore, G),*e_beg);
		
		if (opCode != 0)
		{
			//cout<<"Op type "<<opCode<<" from "<<v->name<<std::endl;
			NodeProps * targetV = get(get(vertex_props,G), target(*e_beg,G));
			
			if (targetV->llvm_inst != NULL)
			{
				if (isa<Instruction>(targetV->llvm_inst))
				{
					Instruction * pi = cast<Instruction>(targetV->llvm_inst);	
					
					//const llvm::Type * t = pi->getType();
					//unsigned typeVal = t->getTypeID();
					
					if (pi->getOpcode() == Instruction::Alloca)
					{
						//cout<<"Found one - "<<targetV->name<<std::endl;
						transferImplicitEdges(targetV->number, top_index);
					}
					else
					{
						findLocalVariable(targetV->number, top_index);
					}
				}
				else
				{
					transferImplicitEdges(targetV->number, top_index);
				}
			}
			else
			{
				transferImplicitEdges(targetV->number, top_index);
			}
		}
	}	
}*/



// TODO:  Right now we are looking at parents
// x --> y --> z
// |     |     |
// v     v     v
// i  <-- <----
//	 
// This works if you have a chain, but if 'y'
// doesn't have the link to 'i' than it doesn't work
// This function is more lightweight analysis, but
// should work for most cases
void FunctionBFC::collapseRedundantImplicit()
{
	// Iterator that goes through all the vertices, we
	// need to find vertices that have incoming implicit edges
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
		NodeProps * v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in collapseRedundantImplicit\n";
#endif 
			continue;
		}
		
		std::set<int> edgesToDelete;
		
		//graph_traits < MyGraphType >::edge_descriptor ed;
		property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
		//bool inserted;
		boost::graph_traits<MyGraphType>::in_edge_iterator i_beg, i_end;
		
		i_beg = boost::in_edges(v_index, G).first;		// edge iterator begin
		i_end = boost::in_edges(v_index, G).second;    // edge iterator end
		
		
		// This checks all incoming edges and looks for any incoming implicit edges
		for(; i_beg != i_end; ++i_beg) 
		{
			int opCode = get(get(edge_iore, G),*i_beg);
			
			//  opcode 0 means an IMPLICIT edge
			// TODO: Make 0 a constant for IMPLICIT
			if (opCode == 0)
			{
				NodeProps * inTargetV = get(get(vertex_props,G), source(*i_beg,G));
				
				// We want to check if there is a parent that also connects to an implicit
				// if so, we want to delete the edge from the parent to implicit target
				
				//graph_traits < MyGraphType >::edge_descriptor ed;
				//property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
				//bool inserted;
				boost::graph_traits<MyGraphType>::in_edge_iterator i_beg2, i_end2;
				
				i_beg2 = boost::in_edges(inTargetV->number, G).first;		// edge iterator begin
				i_end2 = boost::in_edges(inTargetV->number, G).second;    // edge iterator end
				
				// We look over all the parent edges to the one that has the implicit look
				for(; i_beg2 != i_end2; ++i_beg2) 
				{
					int opCode3 = get(get(edge_iore, G),*i_beg2);
					
					if (opCode3 == 0)
					{
						
						NodeProps * parentV = get(get(vertex_props,G), source(*i_beg2,G));
						
						// Check for loops, if there is a loop we're aren't touching it
						boost::graph_traits<MyGraphType>::in_edge_iterator i_beg3, i_end3;
						
						i_beg3 = boost::in_edges(parentV->number, G).first;		// edge iterator begin
						i_end3 = boost::in_edges(parentV->number, G).second;    // edge iterator end
						
						for (; i_beg3 != i_end3; ++i_beg3)
						{
							NodeProps * maybeLoopV = get(get(vertex_props,G), source(*i_beg3,G));
							if (maybeLoopV->number == inTargetV->number)
							{
#ifdef DEBUG_GRAPH_IMPLICIT
								blame_info<<"Implicit__(collapseRedundantImplicit) - Loop detected between ";
								blame_info<<parentV->name<<" and "<<inTargetV->name<<std::endl;
#endif
								continue;
							}
						}
						
						// Now we check to see if any of the parents outgoing edges goes to the original
						//  implicit source, if so, we can target that edge for deletion
						boost::graph_traits<MyGraphType>::out_edge_iterator o_beg, o_end;
						
						o_beg = boost::out_edges(parentV->number, G).first;		// edge iterator begin
						o_end = boost::out_edges(parentV->number, G).second;    // edge iterator end
						
						for (; o_beg != o_end; ++o_beg)
						{
							int opCode2 = get(get(edge_iore, G), *o_beg);
							if (opCode2 == 0)
							{
								NodeProps * implicitV = get(get(vertex_props,G), target(*o_beg,G));
								if (v->number == implicitV->number)
								{
#ifdef DEBUG_GRAPH_IMPLICIT
									blame_info<<"Implicit__(collapseRedundantImplicit) - NODE(TFD) "<<parentV->name;
									blame_info<<" has implicit edge to "<<inTargetV->name<<" and "<<v->name;
									blame_info<<" Delete imp edge from "<<inTargetV->name<<" to "<<v->name<<std::endl;
#endif
									edgesToDelete.insert(inTargetV->number);
								}
							}
						}
						
					}
				}
			}
		}
		
		std::set<int>::iterator deleteIterator;
		
		for (deleteIterator = edgesToDelete.begin();  deleteIterator != edgesToDelete.end(); deleteIterator++)
		{
			remove_edge(*deleteIterator, v_index , G);
			//std::cout<<"CRI - Removeing edge from "<<*deleteIterator<<" to "<<v_index<<std::endl;
		}
		
		//edgesToDelete.erase();
	}
}



// This function takes a graph of the form
// toBool --> tmp --> ... --> <local variable>
// and deletes everything up to the local variable, moving
// all of the edges accordingly
/*
void FunctionBFC::collapseImplicit()
{
	
	//graph_traits < MyGraphType >::edge_descriptor ed;
	//property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
		
		NodeProps * v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in collapseImplicit\n";
#endif 
			continue;
		}
		
		if ( strstr(v->name.c_str(), "toBool") != NULL )
		{
			//cout<<"For "<<v->name<<std::endl;
			findLocalVariable(v_index, v_index);
			//cout<<std::endl;
			deleteOldImplicit(v_index);
		}
	}
}*/

// For now we don't want to transfer over line nums for loads we are collapsing if they
// feed into calls.  The reason for this is that the transfer function will take care
// of this eventually.  We don't want line numbers feeding in when the parameter (or 
//  variable) is passed in as read only and can't contribute to the blame, so we
//  don't want the line number appended on
bool FunctionBFC::shouldTransferLineNums(NodeProps * v)
{
	bool shouldTransfer = true;

	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	// iterate through the out edges and delete edges to target, move other edges
	boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
	oe_beg = boost::out_edges(v->number, G).first;		// edge iterator begin
	oe_end = boost::out_edges(v->number, G).second;       // edge iterator end
	
	for(; oe_beg != oe_end; ++oe_beg) {
		int opCode = get(get(edge_iore, G),*oe_beg);
		
		if (opCode == Instruction::Call) {
			shouldTransfer = false;
		}
		//NodeProps * outTargetV = get(get(vertex_props,G), target(*oe_beg,G));
	}
	return shouldTransfer;
}

/*
    Added by Hui 04/06/16
    Basically, for redundant field, we shall collapse it when it's a write 
    but keep it when it's a read:
    %6<--STORE--A-<--LOAD--%7==STORE==>newVal :     write (currently, we only consider one STORE-LOAD redundancy)
    %6<--STORE--A-<--LOAD--%7<--GEP--%8 :           read
    %6<--LOAD--%8:                                  read
    %6<--GEP--%8:                                   read

    //The conditions in this func can be extended in the future 
*/
bool FunctionBFC::shouldKeepEdge(int movedOpCode, NodeProps *inTargetV)
{
    if (movedOpCode==Instruction::Load || movedOpCode==GEP_BASE_OP) 
        return true;
    else if (movedOpCode==Instruction::Store) {
        // iterate through the in_edges 
        boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
        ie_beg = boost::in_edges(inTargetV->number, G).first;		// edge iterator begin
        ie_end = boost::in_edges(inTargetV->number, G).second;       // edge iterator end
        
        for(; ie_beg != ie_end; ++ie_beg) {
            int opCode = get(get(edge_iore, G),*ie_beg);
            
            if (opCode == Instruction::Load) {
                NodeProps *sV = get(get(vertex_props,G),source(*ie_beg,G));
                //we check the out_edge of sV 
                boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
                oe_beg = boost::out_edges(sV->number, G).first;		// edge iterator begin
                oe_end = boost::out_edges(sV->number, G).second;       // edge iterator end
                for(; oe_beg != oe_end; ++oe_beg) {
                    int opCode2 = get(get(edge_iore, G), *oe_beg);
                    //conservative way: as long as there's a write to the redundant field, it should collapse
                    if (opCode2 == Instruction::Store)
                        return false; 
                }
            }
        }
    }
    return true;
}

int FunctionBFC::transferEdgesAndDeleteNode(NodeProps *dN, NodeProps *rN, bool transferLineNumbers, bool fromGEP)
{
	//std::cerr<<"Made it TO here\n";
	//safe check, dN and rN can NOT be the same node, otherwise it'll go into an infinite loop
    if(dN == rN) {
        blame_info<<"Error, dN == rN  in transferEdgesAndDeleteNode!"<<endl;
        return -1;
    }

	int deleteNode = dN->number;
	int recipientNode = rN->number;
	
	if (shouldTransferLineNums(dN) && transferLineNumbers) {
#ifdef DEBUG_GRAPH_COLLAPSE
		blame_info<<"Collapse_(transferEdgesAndDeleteNode) - tranfer lineNumber "<<dN->line_num<<" from "<<dN->name<<" to "<<rN->name<<std::endl;
		blame_info<<"Inserting line number(4) "<<dN->line_num<<" to "<<rN->name<<std::endl;
#endif		
		rN->lineNumbers.insert(dN->line_num);
	}
#ifdef DEBUG_GRAPH_COLLAPSE
	blame_info<<"Node Props for delete node "<<dN->name<<": ";
	for (int a = 0; a < NODE_PROPS_SIZE; a++)
		blame_info<<dN->nStatus[a]<<" ";
	
	blame_info<<std::endl;	
	
	
	blame_info<<"Node Props for rec node "<<rN->name<<": ";
	for (int a = 0; a < NODE_PROPS_SIZE; a++)
		blame_info<<rN->nStatus[a]<<" ";
	
	blame_info<<std::endl;	
#endif 
	
	for (int a = 0; a < NODE_PROPS_SIZE; a++) {
		if (dN->nStatus[a])
			rN->nStatus[a] = dN->nStatus[a];
		
		dN->nStatus[a] = false;
	}
	
	std::set<FuncCall *>::iterator fc_i = dN->funcCalls.begin();
	
	for (; fc_i != dN->funcCalls.end(); fc_i++) {
        //FuncCall * fc = *fc_i;
		//std::cout<<getSourceFuncName()<<" - TEADN Transferring call to node "<<rN->name<<" from ";
		//std::cout<<dN->name<<" --- "<<fc->funcName<<" param num "<<fc->paramNumber<<std::endl;	
		rN->addFuncCall(*fc_i);
	}
	
	if (dN->pointsTo != NULL) {
		if (rN->pointsTo == NULL)
			rN->pointsTo = dN->pointsTo;
		else {
#ifdef DEBUG_ERROR
			blame_info<<"More than one pointsTo involved, fromGEP="<<fromGEP<<std::endl;
#endif	
		}
	}
	
	
	dN->deleted = true;
	
	bool inserted;
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	// iterate through the out edges and delete edges to target, move other edges
	boost::graph_traits<MyGraphType>::out_edge_iterator oe_beg, oe_end;
	oe_beg = boost::out_edges(deleteNode, G).first;		// edge iterator begin
	oe_end = boost::out_edges(deleteNode, G).second;       // edge iterator end

	/* Figure out all the destination nodes the would-be deleted node connects to
	 
	 |----> D1
	 R-->S --|----> T
	 |----> D2
	 */
	
	std::vector<int> destNodes;  // D1, T, D2
	
	for(; oe_beg != oe_end; ++oe_beg) {
		int movedOpCode = get(get(edge_iore, G),*oe_beg);
		NodeProps *outTargetV = get(get(vertex_props,G), target(*oe_beg,G));

        /* Added by Hui 04/06/16: tricky ways to avoid include more lines than needed:
           if the out_edge is GEP_BASE_OP, and the dN/rN are redundant fields, then we
           choose NOT to transfer this edge since rN already has its base, and if it's another
           READ of the field,instead of WRITE, then new base will likely introduce a wrong line:
           a = s.f1; ----line 1
           b = s.f1; ----line 2
           for %6/%7=GEP s,0,1 (&f1), %6 and %7 should be regarded as two different nodes(2 line#), 
           while if %6 and %7 are both WRITE, they should be seen as the same node. So we'll keep
           the edge %7->s, so a, %6 won't have wrong line# 2
        */

        if (fromGEP && movedOpCode == GEP_BASE_OP) {
#ifdef DEBUG_GRAPH_COLLAPSE
            blame_info<<"We met a GEP_BASE_OP out_edge: "<<dN->name<<"-->"<<outTargetV->name<<" so we'll keep this edge and dN"<<std::endl;
#endif 
            continue;
        }
/*
#ifdef ONLY_FOR_PARAM1   The below method is WRONG
        //added by Hui 04/13/16: this needs to be CHECKED for better solution since 
        //it'll remove loop line(for(;;) from redundant fields no matter they will be read or written
        else if (fromGEP && movedOpCode == IMPLICIT_OP) {
#ifdef DEBUG_GRAPH_COLLAPSE
            blame_info<<"We met a IMPLICIT_OP out_edge: "<<dN->name<<"-->"<<outTargetV->name<<" so we'll keep this edge and dN"<<std::endl;
#endif 
            continue;
        }
#endif
*/
		destNodes.push_back(outTargetV->number);
		
		// Don't want a self loop edge
		if (outTargetV->number != recipientNode ) {
		// May be some implicit edges already there and we want explicit to trump implicit ... for now
            if(edge(recipientNode, outTargetV->number, G).second)
			    remove_edge(recipientNode, outTargetV->number, G);
			tie(ed, inserted) = add_edge(recipientNode, outTargetV->number, G);
			
			//std::cout<<"**DN -- Adding edge from "<<rN->name<<"("<<recipientNode<<") to ";
			//std::cout<<outTargetV->name<<"("<<outTargetV->number<<")"<<std::endl;
			if (inserted)
				edge_type[ed] = movedOpCode;	
			else {
#ifdef DEBUG_ERROR			
				std::cerr<<"Insertion fail for "<<rN->name<<" to "<<outTargetV->name<<std::endl;
#endif 
			}
		}

#ifdef DEBUG_GRAPH_COLLAPSE
        blame_info<<"Remove edge(hui) from "<<dN->name<<" to "<<outTargetV->name<<" [1]"<<std::endl;
#endif 
	}
	
	std::vector<int>::iterator v_i, v_e = destNodes.end();
	
	for (v_i = destNodes.begin(); v_i != v_e; ++v_i) {
		remove_edge(deleteNode, *v_i, G);
		//std::cout<<"**DN -- Removing edge from "<<dN->name<<"("<<dN->number<<") to "<<*v_i<<std::endl;
	}	
	
	/* Figure out all the root nodes the would-be deleted node has incoming edges from
	 
	 |----> D1
	 R-->S   T--| 
	 |----> D2
	 */
	
	
	std::vector<int> sourceNodes;  // R
	
	// iterate through the in edges and create new edge to destination, delete old edge
	boost::graph_traits<MyGraphType>::in_edge_iterator ie_beg, ie_end;
	ie_beg = boost::in_edges(deleteNode, G).first;		// edge iterator begin
	ie_end = boost::in_edges(deleteNode, G).second;       // edge iterator end
	
	for(; ie_beg != ie_end; ++ie_beg) {
		int movedOpCode = get(get(edge_iore, G),*ie_beg);
		NodeProps *inTargetV = get(get(vertex_props,G), source(*ie_beg,G));
		
        /* Added by Hui 04/06/16: tricky ways to avoid include more lines than needed:
           if the in_edge is LOAD, and the dN/rN are redundant fields, then LOAD means 
           this field is to be READ, we don't want the blaming nodes to include line# of
           rn(usually it's earlier). So we simply NOT remove and transfer the edge: sN->dN;
           if the in_edge is STORE(usally it'll fowllowed by in_edge load, and out_edge store)
           it means the field is to be WRITE, then we do want the edges transffered so that all
           blame can go to the same field(rN)
        */
        if (fromGEP && shouldKeepEdge(movedOpCode, inTargetV)) {
#ifdef DEBUG_GRAPH_COLLAPSE
            blame_info<<"We met a READ in_edge: "<<inTargetV->name<<"-->"<<dN->name<<" so we'll keep this edge and dN"<<std::endl;
#endif 
            continue;
        }

		sourceNodes.push_back(inTargetV->number);
		// Don't want a self loop edge
		if (inTargetV->number != recipientNode) {//&& movedOpCode > 0)
			// May be some implicit edges already there and we want explicit to trump implicit ... for now
            if(edge(inTargetV->number, recipientNode, G).second) 
                //edge(u,v,g) returns pair<e_d, bool>, bool=>whether edge exists
			    remove_edge(inTargetV->number, recipientNode, G);
			tie(ed, inserted) = add_edge(inTargetV->number, recipientNode, G);
			//std::cout<<"**SN -- Adding edge from "<<inTargetV->name<<"("<<inTargetV->number<<") to ";
			//std::cout<<rN->name<<"("<<recipientNode<<")"<<std::endl;
			
			if (inserted)
				edge_type[ed] = movedOpCode;	
			else {
#ifdef DEBUG_ERROR			
				std::cerr<<"Insertion fail for "<<inTargetV->name<<" to "<<rN->name<<std::endl;
#endif
			}
		}

#ifdef DEBUG_GRAPH_COLLAPSE
        blame_info<<"Remove edge(hui) from "<<inTargetV->name<<" to "<<dN->name<<" [2]"<<std::endl;
#endif 
	}
	
	v_e = sourceNodes.end();
	
	for (v_i = sourceNodes.begin(); v_i != v_e; ++v_i) {
		remove_edge(*v_i, deleteNode, G);
		//std::cout<<"**SN -- Removing edge from "<<*v_i<<" to "<<dN->name<<"("<<dN->number<<")"<<std::endl;
		
	}	
	
	/* Finish with something like this
	 
	 |----> D1
	 S	 R--> T--| 
	 |----> D2
	 
	 Due to some weird graph side effects, we don't delete the 'S' node, but
	 rather just get rid of all the edges so it essentially ceases to exist
	 in terms of the graph
	 */
	
	return 0;
}



int FunctionBFC::collapseAll(std::set<int>& collapseInstructions)
{
	//bool inserted;
	int collapsedEdges = 0;
	
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type = get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end; 
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		
		NodeProps *v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v) {
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in collapseAll\n";
#endif 
			continue;
		}
		
		boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
		e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
		e_end = boost::out_edges(v_index, G).second;    // edge iterator end
		
		// iterate through the edges to find matching opcode
		for(; e_beg != e_end; ++e_beg) {
			int opCode = get(get(edge_iore, G),*e_beg);
			// It's one of the opcodes we are trying to collapse
			if (collapseInstructions.count(opCode)) {								
				// would be delete node
				NodeProps *sourceV = get(get(vertex_props,G), source(*e_beg,G));
				// would be recipient node
				NodeProps *targetV = get(get(vertex_props,G), target(*e_beg,G));
				
				if (sourceV->isLocalVar || sourceV->name.find(PARAM_REC) != std::string::npos || sourceV->isGlobal) {
					if ((opCode == Instruction::BitCast || opCode == Instruction::Load) 
						&& (!targetV->isLocalVar && targetV->name.find(PARAM_REC) == std::string::npos && !targetV->isGlobal)) {
						// Swap the order for these cases 
						transferEdgesAndDeleteNode(targetV, sourceV);
						collapsedEdges++;
#ifdef DEBUG_GRAPH_COLLAPSE
						blame_info<<"Graph__(collapseAll)--Swapping order "<<opCode<<" "<<sourceV->name<<" recipient ";
						blame_info<<" with "<<targetV->name<<" being deleted."<<std::endl;
#endif
						break;
					}
					else {
#ifdef DEBUG_GRAPH_COLLAPSE
						blame_info<<"Graph__(collapseAll)--Source is not a temp variable for op "<<opCode<<" "<<sourceV->name<<std::endl;
#endif
						break;
					}
				}
				else {
					targetV->collapsed_inst = sourceV->llvm_inst;
					transferEdgesAndDeleteNode(sourceV, targetV);
#ifdef DEBUG_GRAPH_COLLAPSE
					blame_info<<"Graph__(collapseAll)--standard delete "<<opCode<<" "<<targetV->name<<" recipient ";
					blame_info<<" with "<<sourceV->name<<" being deleted."<<std::endl;
#endif
					collapsedEdges++;
					break;
					
				}
			}
		}
	}
	
	// Not going to be calling this function again, lets
	// update our GEP pointers
	if (collapsedEdges == 0) {
		for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
			NodeProps * v = get(get(vertex_props, G),*i);
			int v_index = get(get(vertex_index, G),*i);
			
			if (!v) {
#ifdef DEBUG_ERROR			
				std::cerr<<"Null V in collapseAll\n";
#endif 
				continue;
			}
			
			if (v->pointsTo == NULL)
				continue;
			
			boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
			e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
			e_end = boost::out_edges(v_index, G).second;    // edge iterator end
			
			// iterate through the edges to find matching opcode
			for(; e_beg != e_end; ++e_beg) {
				int opCode = get(get(edge_iore, G),*e_beg);
				
				if (opCode == GEP_BASE_OP) {
					NodeProps * targetV = get(get(vertex_props,G), target(*e_beg,G));
					v->pointsTo = targetV;
					targetV->pointedTo.insert(v);
				}
			}
		}
	}
	
	return collapsedEdges;
}


int FunctionBFC::collapseInvoke()
{
	//bool inserted;
	int collapsedEdges = 0;
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
		
		NodeProps *v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in collapseInvoke\n";
#endif 
			continue;
		}
		
		
		boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
		
		e_beg = boost::out_edges(v_index, G).first;		// edge iterator begin
		e_end = boost::out_edges(v_index, G).second;    // edge iterator end
		
		std::vector<std::pair<int,int>  > deleteEdges;
		
		// iterate through the edges to find matching opcode
		for(; e_beg != e_end; ++e_beg) 
		{
			int opCode = get(get(edge_iore, G),*e_beg);
			
			if (opCode == Instruction::Invoke)
			{										
				NodeProps * sourceV = get(get(vertex_props,G), source(*e_beg,G));
				NodeProps * targetV = get(get(vertex_props,G), target(*e_beg,G));
				
				std::string::size_type sourceTmp = sourceV->name.find( "tmp", 0 );
				std::string::size_type targetTmp = targetV->name.find( "tmp", 0 );
				
				if (sourceV->llvm_inst != NULL && targetV->llvm_inst != NULL)
				{
					if (isa<Instruction>(sourceV->llvm_inst) && isa<Instruction>(targetV->llvm_inst))
					{
						Instruction * sourceI = cast<Instruction>(sourceV->llvm_inst);
						Instruction * targetI = cast<Instruction>(targetV->llvm_inst);
						
						// Corner case where both Tmp and Var could be true is case
						//   where you have a ifdefd local variable in the program
						//   with the substring "tmp" in it
						
						bool isSourceTmp = false, isSourceVar = false;
						bool isTargetTmp = false, isTargetVar = false;
						
						// This sees if the source node is a tmp register (as opposed to 
						//			local variable with "tmp" in its name)
						if (sourceTmp != std::string::npos)
							isSourceTmp = true;
						
						if ( sourceI->getOpcode() == Instruction::Alloca || sourceV->pointsTo != NULL )
							isSourceVar = true;
						
						if (isSourceTmp && isSourceVar)
						{
							isSourceTmp = false;
						}
						
						if (targetTmp != std::string::npos)
							isTargetTmp = true;
						
						if (targetI->getOpcode() == Instruction::Alloca || targetV->pointsTo != NULL )
							isTargetVar = true;
						
						if (isTargetTmp && isTargetVar)
						{
							isTargetTmp = false;
						}
						
						
						if ( isSourceTmp && (isTargetTmp || isTargetVar) )
						{
							collapsedEdges++;
							//std::cout<<"Collapsing (1) op "<<opCode<<" for "<<targetV->name<<" and "<<sourceV->name<<std::endl;
#ifdef DEBUG_GRAPH_COLLAPSE
							blame_info<<"Collapse(4)"<<std::endl;
#endif 
							transferEdgesAndDeleteNode(sourceV, targetV);
							
							break;
						}
						
					}
				}
			}
		}
	}
	
	return collapsedEdges;
}


// This collapses exception handling nodes/edges.  We don't handle them yet.
// Technically, we don't handle anything c++ but accidentally we do, but not EH
void FunctionBFC::collapseEH()
{
	
	graph_traits < MyGraphType >::edge_descriptor ed;
	property_map<MyGraphType, edge_iore_t>::type edge_type;
	edge_type = get(edge_iore, G);
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	                                                   
	// For data structure and continuity reasons, we don't delete nodes,
	// Rather we delete edges orphaning nodes(single nodes) essentially removing them
	// implicitly from the graph
	std::set<graph_traits < MyGraphType >::edge_descriptor> edgesToCollapse;
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) {
		//TC: below could be v = vertex_props[*i];
		NodeProps *v = get(get(vertex_props, G),*i);
		
		// TODO: Elegant way to make it so we aren't completely screwed if
		//    someone throws a wrench(make difficulties) in this by 
        //    actually naming one of their
		//    local variables one of these names
		if (v->name.find("save_eptr") != string::npos ||
				v->name.find("eh_ptr") != string::npos ||
				v->name.find("eh_value") != string::npos ||
				v->name.find("eh_select") != string::npos ||
				v->name.find("save_filt") != string::npos ||
				v->name.find("eh_exception") != string::npos) 
        {	
			boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
			
			e_beg = boost::out_edges(v->number, G).first;	// edge iterator begin
			e_end = boost::out_edges(v->number, G).second;  // edge iterator end
			for (; e_beg != e_end; e_beg++) {
				edgesToCollapse.insert(*e_beg);
			}
		}
	}
	
	std::set<graph_traits < MyGraphType >::edge_descriptor>::iterator set_ed_i;
	
	for (set_ed_i = edgesToCollapse.begin(); set_ed_i != edgesToCollapse.end(); set_ed_i++) {
		remove_edge(*set_ed_i, G);
	}
	
}

// Do some edge manipulation for mallocs
// Return ----->
//                Malloc
// Size Calc -->
//
//     TO
//
// Return --> Size Calc
void FunctionBFC::handleMallocs()
{	
	//std::cout<<"Entering handleMallocs"<<std::endl;
	property_map<MyGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G);
	
	bool inserted;
	graph_traits < MyGraphType >::edge_descriptor ed;
	
	graph_traits<MyGraphType>::vertex_iterator i, v_end;
	
	for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
		NodeProps * v = get(get(vertex_props, G),*i);
		int v_index = get(get(vertex_index, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR			
			std::cerr<<"Null V in handleMallocs\n";
#endif
			continue;
		}
		
		if ( strstr(v->name.c_str(),"malloc--") != NULL )
		{
			boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
			
			e_beg = boost::in_edges(v_index, G).first;		// edge iterator begin
			e_end = boost::in_edges(v_index, G).second;    // edge iterator end
			
			
			// First param (oneParam) is associated with calculation of allocation size
			// Return val  (zeroParam) is associated with location for the allocation
			int oneParam = -1;
			int zeroParam = -1;
			
			// iterate through the edges to find matching opcode
			for(; e_beg != e_end; ++e_beg) 
			{
				int opCode = get(get(edge_iore, G),*e_beg);
				
				if (opCode == Instruction::Call)
				{
					//int sourceV = get(get(vertex_index, G), source(*e_beg, G));
					//int targetV = get(get(vertex_index, G), target(*e_beg, G));
					
					NodeProps * sourceVP = get(get(vertex_props, G), source(*e_beg, G));
					NodeProps * targetVP = get(get(vertex_props, G), target(*e_beg, G));
					
					int paramNum = MAX_PARAMS + 1;
					//std::cerr<<"Call from "<<sourceVP->name<<" to "<<targetVP->name<<std::endl;
					
					std::set<FuncCall *>::iterator fc_i = sourceVP->funcCalls.begin();
					
					for (; fc_i != sourceVP->funcCalls.end(); fc_i++)
					{
						FuncCall * fc = *fc_i;
						
						//std::cerr<<"FC -- "<<fc->funcName<<"  "<<targetVP->name<<std::endl;
						
						if (fc->funcName == targetVP->name)
						{
							paramNum = fc->paramNumber;
							if (paramNum == -1)//changed by Hui 12/31/15, 0=>-1
								zeroParam = sourceVP->number;
							else if (paramNum == 0)//changed by Hui 12/31/15 1=>0
								oneParam = sourceVP->number;
							
							break;
						}						
					}
					
					//std::cerr<<"Param Num "<<paramNum<<" for "<<sourceVP->name<<std::endl;
					
				}
			}			
			
			if (oneParam > -1 && zeroParam > -1 && oneParam != zeroParam)
			{
				
				remove_edge(oneParam, v->number, G);
				remove_edge(zeroParam, v->number, G);
				tie(ed, inserted) = add_edge(zeroParam, oneParam, G);
				
				//std::cerr<<"Adding edge for "<<v->name<<" "<<zeroParam<<" to "<<oneParam<<std::endl;
				
				if (inserted)
					edge_type[ed] = RESOLVED_MALLOC_OP;	
			}
		}
	}
	//std::cout<<"Leaving handleMallocs"<<std::endl;
}


void FunctionBFC::collapseGraph()
{
	int collapsedAny = 1;
	
	// Collapse Implicit  --- need better description here
	//collapseImplicit();
	//printDotFiles("_afterFirstCompressImp.dot", true);
	
	// Collapse Exception Handling
	collapseEH();
	
	std::set<int> collapseInstructions;
	
	//collapseInstructions.insert(Instruction::Load);
	collapseInstructions.insert(Instruction::BitCast);
	//collapseInstructions.insert(Instruction::FPExt);
	//collapseInstructions.insert(Instruction::Trunc);
	//collapseInstructions.insert(Instruction::ZExt);
	//collapseInstructions.insert(Instruction::SExt);
	//collapseInstructions.insert(Instruction::FPToUI);
	//collapseInstructions.insert(Instruction::FPToSI);
	//collapseInstructions.insert(Instruction::UIToFP);
	//collapseInstructions.insert(Instruction::SIToFP);
	//collapseInstructions.insert(Instruction::FPTrunc);
	//collapseInstructions.insert(Instruction::PtrToInt);
	//collapseInstructions.insert(Instruction::IntToPtr);
	
	while (collapsedAny) {
		collapsedAny = collapseAll(collapseInstructions);	
	}
	
	//collapsedAny = 1;
	
	//while (collapsedAny)
	//{
	//collapsedAny = collapseInvoke();
	//}
	collapseIO();
	//handleMallocs();
	//printDotFiles("_beforeCompressImp.dot", true);
	//collapseRedundantImplicit();
}
