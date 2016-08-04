/*
 *  FunctionBFCOutput.cpp
 *  
 *  Deal with all output functions
 *  Previous contribution  by Nick Rutar on 5/14/09.
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

#include "ExitVars.h"
#include "FunctionBFC.h"

#include <sys/types.h>
#include <sys/stat.h>


void FunctionBFC::exportSideEffects(std::ostream &O)
{
	O<<"BEGIN FUNC "<<getSourceFuncName()<<" ";
	O<<getModuleName()<<" "<<getStartLineNum()<<" "<<getEndLineNum()<<" ";
	O<<getModulePathName()<<std::endl;

	std::vector< std::pair<NodeProps *, NodeProps *> >::iterator vec_pair_i;

	if (seRelations.size() > 0)
	{
		O<<"BEGIN SE_RELATIONS"<<std::endl;
		for (vec_pair_i = seRelations.begin(); vec_pair_i != seRelations.end(); vec_pair_i++)
		{
			NodeProps * fir = (*vec_pair_i).first;
			NodeProps * sec = (*vec_pair_i).second;
				
			std::set<NodeProps *> visited;	
			int firNum = fir->getParamNum(visited);
			
			visited.clear();
			int secNum = sec->getParamNum(visited);
		
			O<<"R: "<<fir->getFullName()<<"  "<<firNum<<" ";
			O<<sec->getFullName()<<" "<<secNum<<std::endl;

		}
		O<<"END SE_RELATIONS"<<std::endl;
	}
	
	
	if (seAliases.size() > 0)
	{
		O<<"BEGIN SE_ALIASES"<<std::endl;
		for (vec_pair_i = seAliases.begin(); vec_pair_i != seAliases.end(); vec_pair_i++)
		{
			NodeProps * fir = (*vec_pair_i).first;
			NodeProps * sec = (*vec_pair_i).second;
	
			std::string firName, secName;
			
			std::set<NodeProps *> visited;	
			int firNum = fir->getParamNum(visited);
			
			visited.clear();
			int secNum = sec->getParamNum(visited);
		
			O<<"A: "<<fir->getFullName()<<"  "<<firNum<<" ";
			O<<sec->getFullName()<<" "<<secNum<<std::endl;
		}
		O<<"END SE_ALIASES"<<std::endl;
	}
	
	if (seCalls.size() > 0)
	{
		O<<"BEGIN SE_CALLS"<<std::endl;
		std::vector< FuncCallSE * >::iterator vec_fcse_i;
		for (vec_fcse_i = seCalls.begin(); vec_fcse_i != seCalls.end(); vec_fcse_i++)
		{
			FuncCallSE * fcse = *vec_fcse_i;
			O<<"C: "<<fcse->callNode->name<<std::endl;
		
			std::vector<FuncCallSEElement *>::iterator vec_fcseEl_i;
			for (vec_fcseEl_i = fcse->parameters.begin(); vec_fcseEl_i != fcse->parameters.end();
					vec_fcseEl_i++)
			{
				std::set<NodeProps *> visited;
				FuncCallSEElement * fcseElement = *vec_fcseEl_i;
				O<<"P: "<<fcseElement->ifc->paramNumber<<" ";
				O<<fcseElement->paramEVNode->name<<" ";
				O<<fcseElement->paramEVNode->getParamNum(visited)<<std::endl;
			}
			
			for (vec_fcseEl_i = fcse->NEVparams.begin(); vec_fcseEl_i != fcse->NEVparams.end();
					vec_fcseEl_i++)
			{
							std::set<NodeProps *> visited;

				FuncCallSEElement * fcseElement = *vec_fcseEl_i;
				O<<"P: "<<fcseElement->ifc->paramNumber<<" ";
				O<<fcseElement->paramEVNode->name<<" ";
				O<<fcseElement->paramEVNode->getParamNum(visited)<<std::endl;
			}

			
		}
		O<<"END SE_CALLS"<<std::endl;
	}
	
	O<<"END FUNC "<<std::endl;
}



void FunctionBFC::exportEverything(std::ostream &O, bool reads)
{
	O<<"BEGIN FUNC "<<std::endl;
	
	O<<"BEGIN F_NAME"<<std::endl;
	O<<getSourceFuncName()<<std::endl;
	O<<"END F_NAME"<<std::endl;
	
	O<<"BEGIN M_NAME_PATH "<<std::endl;
	O<<getModulePathName()<<std::endl;
	O<<"END M_NAME_PATH "<<std::endl;
	
	O<<"BEGIN_M_NAME"<<std::endl;
	O<<getModuleName()<<std::endl;
	O<<"END M_NAME"<<std::endl;
	
	O<<"BEGIN F_B_LINENUM"<<std::endl;//will not be useful anymore 03/28/16
	O<<getStartLineNum()<<std::endl;
	O<<"END F_B_LINENUM"<<std::endl;
			
	O<<"BEGIN F_E_LINENUM"<<std::endl;//will not be useful anymore 03/28/16
	O<<getEndLineNum()<<std::endl;
	O<<"END F_E_LINENUM"<<std::endl;

	O<<"BEGIN F_BPOINT"<<std::endl;
	O<<isBFCPoint<<std::endl;
	O<<"END F_BPOINT"<<std::endl;

    //Added by Hui 03/28/16, keep record of all line#s in this func
    std::set<int>::iterator s_int_i;
    O<<"BEGIN ALL_LINES"<<std::endl;
    for (s_int_i = allLineNums.begin(); s_int_i != allLineNums.end(); s_int_i++)
        O<<*s_int_i<<" ";
    O<<std::endl;
    O<<"END ALL_LINES"<<std::endl;

	//ImpVertexHash::iterator ivh_i;
	std::set<NodeProps *>::iterator ivh_i;
	std::set<NodeProps *>::iterator s_i_i;
	
	for (ivh_i = impVertices.begin(); ivh_i != impVertices.end(); ivh_i++)
	{
		//O<<"-------"<<std::endl;
		NodeProps * impV = (*ivh_i);
		NodeProps * ivp = impV;
		
		if (!impV->isExported)
			continue;
		
		O<<"BEGIN VAR  "<<std::endl;
		
		O<<"BEGIN V_NAME "<<std::endl;
		O<<(*ivh_i)->name<<std::endl;
		O<<"END V_NAME "<<std::endl;
		
		O<<"BEGIN V_TYPE "<<std::endl;
		O<<(*ivh_i)->eStatus<<std::endl;
		
		O<<"END V_TYPE "<<std::endl;
		
		O<<"BEGIN N_TYPE "<<std::endl;
		for (int a = 0; a < NODE_PROPS_SIZE; a++)
			O<<impV->nStatus[a]<<" ";
			
		O<<std::endl;
		
		O<<"END N_TYPE"<<std::endl;
				
	
		O<<"BEGIN DECLARED_LINE"<<endl;
		O<<(*ivh_i)->line_num<<endl;
		O<<"END DECLARED_LINE"<<endl;
		
		O<<"BEGIN IS_WRITTEN"<<endl;
		O<<(*ivh_i)->isWritten<<endl;
		O<<"END IS_WRITTEN"<<endl;
		
		
		std::set<NodeProps *>::iterator ivp_i;
		std::set<NodeProps *>::iterator set_ivp_i;
		
		
/*		O<<"BEGIN PARENTS"<<endl;
		for (set_ivp_i = (*ivh_i)->parents.begin(); set_ivp_i != (*ivh_i)->parents.end(); set_ivp_i++)
		{
			NodeProps * ivpParent = (*set_ivp_i);
			O<<ivpParent->name<<endl;
		}
		O<<"END PARENTS"<<endl;	
	*/
		
		O<<"BEGIN CHILDREN "<<endl;
		for (set_ivp_i = (*ivh_i)->children.begin(); set_ivp_i != (*ivh_i)->children.end(); set_ivp_i++)
		{
			NodeProps * ivpChild = (*set_ivp_i);
			O<<ivpChild->name<<endl;
		}
		O<<"END CHILDREN"<<std::endl;
	
		

		O<<"BEGIN ALIASES "<<std::endl;
		for (ivp_i = (*ivh_i)->aliases.begin(); ivp_i != (*ivh_i)->aliases.end(); ivp_i++)
		{
			NodeProps * ivpAlias = (*ivp_i);
			O<<ivpAlias->name<<endl;
		}
		O<<"END ALIASES "<<std::endl;
	
		
		O<<"BEGIN DATAPTRS "<<std::endl;
		for (ivp_i = (*ivh_i)->dataPtrs.begin(); ivp_i != (*ivh_i)->dataPtrs.end(); ivp_i++)
		{
			NodeProps * ivpAlias = (*ivp_i);
			O<<ivpAlias->name<<endl;
		}
		O<<"END DATAPTRS "<<std::endl;
		

		O<<"BEGIN DFALIAS "<<std::endl;
		for (ivp_i = ivp->dfAliases.begin(); ivp_i != ivp->dfAliases.end(); ivp_i++)
		{
			NodeProps * child = *ivp_i;
			O<<child->name<<endl;
		}
		O<<"END DFALIAS "<<std::endl;
		
		O<<"BEGIN DFCHILDREN "<<std::endl;
		for (s_i_i = ivp->dfChildren.begin(); s_i_i != ivp->dfChildren.end(); s_i_i++)
		{
			NodeProps * child = *s_i_i;
			O<<child->name<<endl;
		}
		O<<"END DFCHILDREN "<<std::endl;
		

		O<<"BEGIN RESOLVED_LS "<<std::endl;
		for (ivp_i = ivp->resolvedLS.begin(); ivp_i != ivp->resolvedLS.end(); ivp_i++)
		{
			NodeProps * child = *ivp_i;
			O<<child->name<<endl;
		}
		O<<"END RESOLVED_LS "<<std::endl;		
		
		
		O<<"BEGIN RESOLVEDLS_FROM "<<std::endl;
		for (ivp_i = ivp->resolvedLSFrom.begin(); ivp_i != ivp->resolvedLSFrom.end(); ivp_i++)
		{			
			NodeProps * child = *ivp_i;
			O<<child->name<<endl;
		}
		O<<"END RESOLVEDLS_FROM "<<std::endl;

	
		O<<"BEGIN RESOLVEDLS_SE "<<std::endl;
		for (ivp_i = ivp->resolvedLSSideEffects.begin(); ivp_i != ivp->resolvedLSSideEffects.end(); ivp_i++)
		{			
			NodeProps * child = *ivp_i;
			O<<child->name<<endl;
		}		
		O<<"END RESOLVEDLS_SE "<<std::endl;
		

		O<<"BEGIN STORES_TO" <<std::endl;
		for (s_i_i = ivp->storesTo.begin(); s_i_i != ivp->storesTo.end(); s_i_i++)
		{
			NodeProps *child = *s_i_i;
			O<<child->name<<endl;
		}
		O<<"END STORES_TO"<<std::endl;
	
		
		O<<"BEGIN FIELDS "<<std::endl;
		for (ivp_i = (*ivh_i)->fields.begin(); ivp_i != (*ivh_i)->fields.end(); ivp_i++)
		{
			NodeProps * ivpAlias = (*ivp_i);
			O<<ivpAlias->name<<endl;
		}
		O<<"END FIELDS "<<std::endl;
		
		O<<"BEGIN FIELD_ALIAS"<<std::endl;
		if ((*ivh_i)->fieldAlias == NULL)
			O<<"NULL"<<std::endl;
		else 
			O<<(*ivh_i)->fieldAlias->name<<std::endl;
		O<<"END FIELD_ALIAS"<<std::endl;
		

/*
		O<<"BEGIN ALIASEDFROM "<<std::endl;
		if ( (*ivh_i)->pointsTo == NULL)
			O<<"NULL"<<std::endl;
		else
			O<< (*ivh_i)->pointsTo->name<<std::endl;
		O<<"END ALIASEDFROM "<<std::endl;
	*/
	
		O<<"BEGIN GEN_TYPE"<<std::endl;
		const llvm::Type * origT = 0;	
		
		bool assigned = false;
		
		if ( (*ivh_i)->sField != NULL)
		{
			 if ( (*ivh_i)->sField->typeName.find("VOID") != std::string::npos)
			 {
				blame_info<<"Void for "<<(*ivh_i)->name<<std::endl;
				 O<<(*ivh_i)->sField->typeName<<std::endl;
				 assigned = true;
			 }
		}
			
			
		if (assigned)
		{
		
		}
		else if (ivp->llvm_inst != NULL && isa<Instruction>(ivp->llvm_inst))
		{
			Instruction * pi = cast<Instruction>(ivp->llvm_inst);	
			
            if (pi == NULL)
			{
				O<<"UNDEF"<<std::endl;
			}
			else
			{
				origT = pi->getType();					
				std::string origTStr = returnTypeName(origT, std::string(" "));
				O<<origTStr<<std::endl;
			}
		}
		else
		{
            if(ivp->llvm_inst != NULL){//it could be a constant(when ivp is a gv)
                origT = ivp->llvm_inst->getType();
                std::string origTStr = returnTypeName(origT, std::string(" "));
                O<<origTStr<<std::endl;
            }
            else
			    O<<"UNDEF"<<std::endl;
		}
		
		O<<"END GEN_TYPE"<<std::endl;
		
	
		O<<"BEGIN STRUCTTYPE"<<std::endl;
		if ( (*ivh_i)->sBFC == NULL){
            /*if((*ivh_i)->llvm_inst != NULL){ //added by Hui 01/21/16
                llvm::Type *origT = (*ivh_i)->llvm_inst->getType();
                unsigned typeVal = origT->getTypeID();
                if(typeVal == Type::StructTyID){
                    const llvm::StructType *type = cast<StructType>(origT);
                    string structNameReal = type->getStructName().str();
                    blame_info<<"Create structName("<<structNameReal<<
                        ") for "<<(*ivh_i)->name<<endl;
                    O<<structNameReal<<std::endl;
                }
                else O<<"NULL"<<std::endl;
            }*/
            //If you really need chpl_** struct, you can first make sure
            //the GEN_TYPE has "Struct" and its llvm_inst is either instruction
            //or constantExpr, then get the operand(0) of it and apply above method
            O<<"NULL"<<std::endl;
        }
		else
			O<<(*ivh_i)->sBFC->structName<<std::endl;
		O<<"END STRUCTTYPE "<<std::endl;
	
			
		O<<"BEGIN STRUCTPARENT "<<std::endl;
		if ( (*ivh_i)->sField == NULL)
			O<<"NULL"<<std::endl;
		else if ( (*ivh_i)->sField->parentStruct == NULL)
			O<<"NULL"<<std::endl;
		else
			O<< (*ivh_i)->sField->parentStruct->structName<<std::endl;
		O<<"END STRUCTPARENT "<<std::endl;
		
		O<<"BEGIN STRUCTFIELDNUM "<<std::endl;
		if ( (*ivh_i)->sField == NULL)
			O<<"NULL"<<std::endl;
		else
			O<< (*ivh_i)->sField->fieldNum<<std::endl;
			
		O<<"END STRUCTFIELDNUM "<<std::endl;
		
		O<<"BEGIN STOREFROM "<<std::endl;
		if ( (*ivh_i)->storeFrom == NULL)
			O<<"NULL"<<std::endl;
		else
			O<< (*ivh_i)->storeFrom->name<<std::endl;//changed by Hui 01/29/16, before it was just 'storeFrom'
			
		O<<"END STOREFROM "<<std::endl;


/*
		O<<"BEGIN EXITV "<<std::endl;
		if ( (*ivh_i)->exitV == NULL)
			O<<"NULL"<<std::endl;
		else
			O<< (*ivh_i)->exitV->name<<std::endl;
		O<<"END EXITV "<<std::endl;
	*/
	
	/*
		O<<"BEGIN PARAMS"<<endl;
		for (set_ivp_i = (*ivh_i)->descParams.begin(); set_ivp_i != (*ivh_i)->descParams.end(); set_ivp_i++)
		{
			NodeProps * ivpAlias = (*set_ivp_i);
			O<<ivpAlias->name<<endl;
		}
		O<<"END PARAMS"<<endl;
		*/
		
		
		O<<"BEGIN PARAMS"<<endl;
		for (set_ivp_i = (*ivh_i)->descParams.begin(); set_ivp_i != (*ivh_i)->descParams.end(); set_ivp_i++)
		{
			NodeProps * ivpAlias = (*set_ivp_i);
			O<<ivpAlias->name<<endl;
		}
		O<<"END PARAMS"<<endl;

		
			
		O<<"BEGIN CALLS"<<endl;
		std::set<ImpFuncCall *>::iterator ifc_i;
		for (ifc_i = (*ivh_i)->calls.begin(); ifc_i != (*ivh_i)->calls.end(); ifc_i++)
		{
			ImpFuncCall * iFunc = (*ifc_i);
			O<<iFunc->callNode->name<<"  "<<iFunc->paramNumber<<std::endl;
		}
		O<<"END CALLS"<<endl;
		
		std::set<int>::iterator si_i;
		O<<"BEGIN DOM_LN "<<endl;
			for (si_i = (*ivh_i)->domLineNumbers.begin(); si_i != (*ivh_i)->domLineNumbers.end(); si_i++)
		{
			O<<*si_i<<endl;
		}
		O<<"END DOM_LN "<<endl;

		
		O<<"BEGIN LINENUMS "<<endl;
			for (si_i = (*ivh_i)->descLineNumbers.begin(); si_i != (*ivh_i)->descLineNumbers.end(); si_i++)
		{
			O<<*si_i<<endl;
		}
		O<<"END LINENUMS "<<endl;
		
		
		if (reads)
		{
			O<<"BEGIN READ_L_NUMS "<<endl;
			LineReadHash::iterator lrh_i;
			
			for (lrh_i = (*ivh_i)->readLines.begin(); lrh_i != (*ivh_i)->readLines.end(); lrh_i++)
			{
				int lineNum = lrh_i->first;
				int numReads = lrh_i->second;
				O<<lineNum<<" "<<numReads<<endl;
				//blame_info<<"For EV "<<ivp->name<<" --- Line num "<<lineNum<<" has "<<numReads<<" Reads."<<std::endl;
			}
			O<<"END READ_L_NUMS "<<endl;
		}
		
		/*
		#ifdef ENABLE_FORTRAN
		O<<"BEGIN STORELINES"<<std::endl;
	std::set<int>::iterator set_i_i;
	for (set_i_i = (*ivh_i)->storeLines.begin(); set_i_i != (*ivh_i)->storeLines.end(); set_i_i++)
	{
		O<<*set_i_i<<" ";
	}
	O<<std::endl;
	O<<"END STORELINES"<<std::endl;
	
			O<<"BEGIN STOREPTRLINES"<<std::endl;
	for (set_i_i = (*ivh_i)->storePTR_Lines.begin(); set_i_i != (*ivh_i)->storePTR_Lines.end(); set_i_i++)
	{
		O<<*set_i_i<<" ";
	}
	O<<std::endl;
	O<<"END STOREPTRLINES"<<std::endl;
	#endif
		*/
		
		
		O<<"END VAR  "<<(*ivh_i)->name<<std::endl;
	}
	
	O<<"END FUNC "<<getSourceFuncName()<<std::endl;
}


void FunctionBFC::exportParams(std::ostream &O)
{
	O<<"FUNCTION "<<getSourceFuncName()<<" "<<voidReturn<<" "<<numPointerParams<<std::endl;
	std::vector<ExitVariable *>::iterator v_ev_i;
	for (v_ev_i = exitVariables.begin(); v_ev_i != exitVariables.end(); v_ev_i++)
	{
		ExitVariable * ev = *v_ev_i;
		if (ev->whichParam >= 0 && ev->vertex != NULL)//changed by Hui '>0'-->'>=0'
		{
			O<<ev->whichParam<<" "<<ev->realName<<" "<<ev->isStructPtr<<" "<<ev->vertex->descLineNumbers.size()<<std::endl;
			if (ev->vertex->descLineNumbers.size() > 0 && ev->vertex->descLineNumbers.size() < 20)
			{
				std::set<int>::iterator s_i_i;
				for (s_i_i = ev->vertex->descLineNumbers.begin(); s_i_i != ev->vertex->descLineNumbers.end(); s_i_i++)
					O<<*s_i_i<<" ";
				O<<std::endl;
			}
		}
	}
	O<<"END FUNCTION"<<std::endl;
}


void FunctionBFC::exportCalls(std::ostream &O, ExternFuncBFCHash & efInfo)
{
	//std::cout<<"Entering exportCalls"<<std::endl;
	std::set<const char *, ltstr>::iterator s_ch_i;
	for (s_ch_i = funcCallNames.begin(); s_ch_i != funcCallNames.end(); s_ch_i++)
	{
		O<<*s_ch_i;
        std::string ss(*s_ch_i);
		if (knownFuncNames.count(*s_ch_i))
			O<<" K ";
		else if (efInfo[ss])
			O<<" E ";
		else if (isLibraryOutput(*s_ch_i))
			O<<" O ";
		else if (strstr(*s_ch_i, "tmp"))
			O<<" P ";
		else
			O<<" U ";
			
		O<<std::endl;
	}
}

/* Prints out a series of dot files */
void FunctionBFC::printDotFiles(const char * strExtension, bool printImplicit)
{
  std::string s("DOT/");
 	
  std::string s_name = func->getName();
 	
	std::string extension(strExtension);
	
  s += s_name;
  s += extension;
 	
  std::ofstream ofs(s.c_str());
 	
	if (!printImplicit)
		printToDot(ofs, NO_PRINT, PRINT_INST_TYPE, PRINT_LINE_NUM, NULL, 0);	
  else
		printToDot(ofs, PRINT_IMPLICIT, PRINT_INST_TYPE, PRINT_LINE_NUM, NULL, 0 );
	

 std::string s2("DOTP/");
 	
 	
  s2 += s_name;
  s2 += extension;

  std::ofstream ofs2(s2.c_str());


	printToDotPretty(ofs2, PRINT_IMPLICIT, PRINT_INST_TYPE, PRINT_LINE_NUM, NULL, 0 );

	
  //int x[] = {Instruction::Load, Instruction::Store};
  //printToDot(ofs_ls, NO_PRINT, PRINT_INST_TYPE, PRINT_LINE_NUM, x, 2 );
}



void FunctionBFC::printFinalDot(bool printAllLines, std::string ext)
{
	std::string s("DOT/");
 	
  std::string s_name = func->getName();
 	
	
	  s += s_name;
	  s += ext;
	
	if (printAllLines)
	{
			std::string extension("_FINAL_AL.dot");
	  s += extension;
	}
	else
	{
			std::string extension("_FINAL.dot");
		  s += extension;
	}
 	
  std::ofstream O(s.c_str());
	
	property_map<MyTruncGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G_trunc);
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyTruncGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G_trunc); i != v_end; ++i) 
	{
		
		NodeProps * v = get(get(vertex_props, G_trunc),*i);
		if (!v)
		{
#ifdef DEBUG_ERROR		
			blame_info<<"Null IVP for "<<*i<<" in printFinalDot<<"<<std::endl;
			std::cerr<<"Null V in printFinalDot\n";
#endif			
			continue;
		}
		
		//int v_index = v->number;
		
		int lineNum = v->line_num;
		int lineNumOrder = v->lineNumOrder;
		//if (v->vp != NULL)
		//	lineNum = v->vp->line_num;
		
		// Print out the nodes
		if (v->eStatus >= EXIT_VAR_PARAM) //changed by Hui 03/15/16, from > to >=, 
		{                               //same changes to other EXIT_VAR_PARAM
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<")"<<"P# "<<v->eStatus - EXIT_VAR_PARAM;
			O<<"\",shape=invtriangle, style=filled, fillcolor=green]\n";		
		}
		else if (v->eStatus == EXIT_VAR_RETURN)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=yellow]\n";		
		}
		else if (v->eStatus == EXIT_VAR_GLOBAL)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=purple]\n";		
		}
		else if (v->nStatus[EXIT_PROG])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=dodgerblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";		
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=yellowgreen]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=wheat3]\n";	

		}
		else if (v->nStatus[EXIT_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=seagreen4]\n";	
		}
		else if (v->nStatus[EXIT_VAR_A_ALIAS])
		{
		if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->storeFrom->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=powderblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						//std::string fullName;
						//getStructName(v,fullName);
						//O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<fullName;
					
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=Mdiamond, style=filled, fillcolor=grey]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=Mdiamond, style=filled, fillcolor=grey38]\n";	
		}
		else if (v->eStatus == EXIT_OUTP)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=orange]\n";
		}
		else if (v->nStatus[LOCAL_VAR] )
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=rectangle, style=filled, fillcolor=pink]\n";
		}
		else if (v->nStatus[LOCAL_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				O<<"(&"<<v->pointsTo->name<<")("<<v->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";		
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=cornsilk]\n";			
		}
		else if (v->nStatus[LOCAL_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_A_ALIAS])
		{
			if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->storeFrom->name<<")";			
				O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=skyblue3]\n";	
		}
		else if (v->nStatus[CALL_NODE])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=tripleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_PARAM])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=doubleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_RETURN] )
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=octagon, style=filled, fillcolor=red]\n";
		}
        //added by Hui 03/22/16 to take care of important registers
        else if (v->nStatus[IMP_REG])
        {
			if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				O<<"("<<v->storeFrom->name<<")";			
				O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=square, style=filled, fillcolor=brown]\n";	
        }
		else
		{
		#ifdef DEBUG_ERROR
			blame_info<<"ERROR! why didn't this node count toward something for "<<v->name<<" "<<v->eStatus<<std::endl;
			std::cerr<<"ERROR! why didn't this node count toward something for "<<v->name<<" "<<v->eStatus<<std::endl;
	#endif
		}

	}
	
	
	//  a -> b [label="hello", style=dashed];
	graph_traits<MyGraphType>::edge_iterator ei, edge_end;
	for(tie(ei, edge_end) = edges(G_trunc); ei != edge_end; ++ei) 
	{
		int opCode = get(get(edge_iore, G_trunc),*ei);
	
		if (opCode == DF_CHILD_EDGE)
			continue;
				
		int paramNum = -1; //TOCHECK: should it be -2 ? Hui 12/31/15 
		if (opCode >= CALL_EDGE)
			paramNum = opCode - CALL_EDGE;
		
		int sourceV = get(get(vertex_index, G_trunc), source(*ei, G_trunc));
		int targetV = get(get(vertex_index, G_trunc), target(*ei, G_trunc));
		
		O<< sourceV  << "->" << targetV;
		
		if (opCode == ALIAS_EDGE )
			O<< "[label=\""<<"A"<<"\", color=powderblue]";
		//else if (opCode == CHILD_EDGE)
			//O<< "[label=\""<<"C"<<"\", color=green]";
		else if (opCode == PARENT_EDGE )
			O<< "[label=\""<<"P"<<"\", color=orange]";
		else if (opCode == DATA_EDGE )
			O<< "[label=\""<<"D"<<"\", color=green]";
		else if (opCode == FIELD_EDGE )
			O<< "[label=\""<<"F"<<"\", color=grey]";
		else if (opCode == DF_ALIAS_EDGE )
			O<< "[label=\""<<"DFA "<<"\", color=powderblue]";
	    else if (opCode == DF_CHILD_EDGE )
			O<< "[label=\""<<"DFC "<<"\", color=purple]";
		else if (opCode == DF_INST_EDGE )
			O<< "[label=\""<<"S"<<"\", color=powderblue, style=dashed]";
		else if (opCode == CALL_PARAM_EDGE )
			O<< "[label=\""<<"CP"<<"\", color=red, style=dashed]";
		else if (opCode >= CALL_EDGE )
			O<< "[label=\""<<"Call Param "<<paramNum<<"\", color=red]";


		
		O<<" ;"<<std::endl;	
	}
	
	if (printAllLines)
	{
		int iS = impVertices.size();
		for(tie(i,v_end) = vertices(G_trunc); i != v_end; ++i) 
		{
			NodeProps * v = get(get(vertex_props, G_trunc),*i);

			
			O<<iS+v->impNumber+1<<"[label=\"";
			
			std::set<int>::iterator set_i_i;
		
			//for (set_i_i = v->lineNumbers.begin(); set_i_i != v->lineNumbers.end(); set_i_i++)
			for (set_i_i = v->descLineNumbers.begin(); set_i_i != v->descLineNumbers.end(); set_i_i++)
			{
				O<<(*set_i_i)<<" ";
			}
			O<<"\", shape = rectangle]\n";
			
			O<<v->impNumber<<"->"<<iS+v->impNumber+1<<std::endl;
			
		}
	}
	O<<"}";
}


void FunctionBFC::printFinalDotPretty(bool printAllLines, std::string ext)
{
	std::string s("DOTP/");
	//printAllLines = false;
 	
  std::string s_name = func->getName();
 	
	
	  s += s_name;
	  s += ext;
	
	if (printAllLines)
	{
			std::string extension("_FINAL_AL.dot");
	  s += extension;
	}
	else
	{
			std::string extension("_FINAL.dot");
		  s += extension;
	}
 	
  std::ofstream O(s.c_str());
	
	property_map<MyTruncGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G_trunc);
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyTruncGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G_trunc); i != v_end; ++i) 
	{
		
		NodeProps * v = get(get(vertex_props, G_trunc),*i);
		if (!v)
		{
#ifdef DEBUG_ERROR		
			blame_info<<"Null IVP for "<<*i<<" in printFinalDot<<"<<std::endl;
			std::cerr<<"Null V in printFinalDot\n";
#endif			
			continue;
		}
		
		//int v_index = v->number;
		
		//int lineNum = v->line_num;
		//int lineNumOrder = v->lineNumOrder;
		//if (v->vp != NULL)
		//	lineNum = v->vp->line_num;
		
		// Print out the nodes
		if (v->eStatus >= EXIT_VAR_PARAM)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"P# "<<v->eStatus - EXIT_VAR_PARAM;
			O<<"\",shape=invtriangle, style=filled, fillcolor=green]\n";		
		}
		else if (v->eStatus == EXIT_VAR_RETURN)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=invtriangle, style=filled, fillcolor=yellow]\n";		
		}
		else if (v->eStatus == EXIT_VAR_GLOBAL)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=invtriangle, style=filled, fillcolor=purple]\n";		
		}
		else if (v->nStatus[EXIT_PROG])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=invtriangle, style=filled, fillcolor=dodgerblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->pointsTo->name<<")";
			}
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=yellowgreen]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=wheat3]\n";	

		}
		else if (v->nStatus[EXIT_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->pointsTo->name<<")";			
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=seagreen4]\n";	
		}
		else if (v->nStatus[EXIT_VAR_A_ALIAS])
		{
		if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->storeFrom->name<<")";			
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=powderblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						//std::string fullName;
						//getStructName(v,fullName);
						//O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<fullName;
					
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=grey]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=grey38]\n";	
		}
		else if (v->eStatus == EXIT_OUTP)
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=invtriangle, style=filled, fillcolor=orange]\n";
		}
		else if (v->nStatus[LOCAL_VAR] )
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=rectangle, style=filled, fillcolor=pink]\n";
		}
		else if (v->nStatus[LOCAL_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				//O<<"(&"<<v->pointsTo->name<<")("<<v->name<<")";
				O<<"("<<v->name<<")";

			}
			O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->pointsTo->name<<")";
			}
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=cornsilk]\n";			
		}
		else if (v->nStatus[LOCAL_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->pointsTo->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_A_ALIAS])
		{
			if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
				//O<<"("<<v->storeFrom->name<<")";			
				//O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=skyblue3]\n";	
		}
		else if (v->nStatus[CALL_NODE])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			//O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=tripleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_PARAM])
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			//O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=doubleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_RETURN] )
		{
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			//O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=octagon, style=filled, fillcolor=red]\n";
		}
        //added by Hui 03/22/16 to take care of important registers
        else if (v->nStatus[IMP_REG])
        {
			O<<get(get(vertex_index, G_trunc),*i)<<"[label=\""<<v->name;
			O<<"\",shape=square, style=filled, fillcolor=brown]\n";	
        }
		else
		{
		#ifdef DEBUG_ERROR
			blame_info<<"ERROR! why didn't this node count toward something for "<<v->name<<" "<<v->eStatus<<std::endl;
			std::cerr<<"ERROR! why didn't this node count toward something for "<<v->name<<" "<<v->eStatus<<std::endl;
	#endif
		}

	}
	
	
	//  a -> b [label="hello", style=dashed];
	graph_traits<MyGraphType>::edge_iterator ei, edge_end;
	for(tie(ei, edge_end) = edges(G_trunc); ei != edge_end; ++ei) 
	{
		int opCode = get(get(edge_iore, G_trunc),*ei);
	
		if (opCode == DF_CHILD_EDGE || opCode == DF_ALIAS_EDGE || opCode == DF_CHILD_EDGE)
			continue;
				
		int paramNum = -1;//TOCHECK: should it be -2? Hui 12/31/15
		if (opCode >= CALL_EDGE)
			paramNum = opCode - CALL_EDGE;
		
		int sourceV = get(get(vertex_index, G_trunc), source(*ei, G_trunc));
		int targetV = get(get(vertex_index, G_trunc), target(*ei, G_trunc));
		
		O<< sourceV  << "->" << targetV;
		
		if (opCode == ALIAS_EDGE )
			O<< "[label=\""<<"A"<<"\", color=powderblue]";
		else if (opCode == PARENT_EDGE )
			O<< "[label=\""<<"B"<<"\", color=orange]";
		else if (opCode == DATA_EDGE )
			O<< "[label=\""<<"D"<<"\", color=green]";
		else if (opCode == FIELD_EDGE )
			O<< "[label=\""<<"F"<<"\", color=grey]";
		//else if (opCode == DF_ALIAS_EDGE )
			//O<< "[label=\""<<"DFA "<<"\", color=powderblue]";
	    //else if (opCode == DF_CHILD_EDGE )
		//	O<< "[label=\""<<"DFC "<<"\", color=purple]";
		else if (opCode == DF_INST_EDGE )
			O<< "[label=\""<<"S"<<"\", color=powderblue, style=dashed]";
		else if (opCode == CALL_PARAM_EDGE )
			O<< "[label=\""<<"CP"<<"\", color=red, style=dashed]";
		else if (opCode >= CALL_EDGE )
			O<< "[label=\""<<"Call Param "<<paramNum<<"\", color=red]";


		
		O<<" ;"<<std::endl;	
	}
	
	if (printAllLines)
	{
		int iS = impVertices.size();
		for(tie(i,v_end) = vertices(G_trunc); i != v_end; ++i) 
		{
			NodeProps * v = get(get(vertex_props, G_trunc),*i);

			
			O<<iS+v->impNumber+1<<"[label=\"";
			
			std::set<int>::iterator set_i_i;
		
			//for (set_i_i = v->lineNumbers.begin(); set_i_i != v->lineNumbers.end(); set_i_i++)
			for (set_i_i = v->descLineNumbers.begin(); set_i_i != v->descLineNumbers.end(); set_i_i++)
			{
				O<<(*set_i_i)<<" ";
			}
			O<<"\", shape = rectangle]\n";
			
			O<<v->impNumber<<"->"<<iS+v->impNumber+1<<std::endl;
			
		}
	}
	O<<"}";
}



void FunctionBFC::printFinalDotAbbr(std::string ext)
{
	bool printAllLines = true;

	std::string s("DOT/");
 	
  std::string s_name = func->getName();
 	
	
	  s += s_name;
	  s += ext;

	
		std::string extension("_FINAL_abbr.dot");
	  s += extension;
	
 	
  std::ofstream O(s.c_str());
	
	property_map<MyTruncGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G_abbr);
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyTruncGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
	{
		
		NodeProps * v = get(get(vertex_props, G_abbr),*i);
		if (!v)
		{
#ifdef DEBUG_ERROR		
			blame_info<<"Null IVP for "<<*i<<" in printFinalDot<<"<<std::endl;
			std::cerr<<"Null V in printFinalDot\n";
#endif			
			continue;
		}
		
		//int v_index = v->number;
		
		int lineNum = v->line_num;
		int lineNumOrder = v->lineNumOrder;
		//if (v->vp != NULL)
		//	lineNum = v->vp->line_num;
		
		// Print out the nodes
		if (v->eStatus >= EXIT_VAR_PARAM)
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<")"<<"P# "<<v->eStatus - EXIT_VAR_PARAM;
			O<<"\",shape=invtriangle, style=filled, fillcolor=green]\n";		
		}
		else if (v->eStatus == EXIT_VAR_RETURN)
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=yellow]\n";		
		}
		else if (v->eStatus == EXIT_VAR_GLOBAL)
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=purple]\n";		
		}/*
		else if (v->nStatus[EXIT_PROG])
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=dodgerblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";		
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=yellowgreen]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=wheat3]\n";	

		}
		else if (v->nStatus[EXIT_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=seagreen4]\n";	
		}
		else if (v->nStatus[EXIT_VAR_A_ALIAS])
		{
		if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->storeFrom->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=powderblue]\n";	
		}*/
		else if (v->nStatus[EXIT_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=grey]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=cadetblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=Mdiamond, style=filled, fillcolor=grey38]\n";	
		}
		else if (v->eStatus == EXIT_OUTP)
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=invtriangle, style=filled, fillcolor=orange]\n";
		}
		else if (v->nStatus[LOCAL_VAR] )
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=rectangle, style=filled, fillcolor=pink]\n";
		}
		else if (v->nStatus[LOCAL_VAR_FIELD])
		{
			if (v->pointsTo == NULL)
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}
			}
			else
			{
				if (v->sField == NULL)
				{
					O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NFP_"<<v->name;
				}
				else
				{
					if (v->sField->parentStruct == NULL)
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<"NSP_"<<v->sField->fieldName;
					}
					else
					{
						O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->sField->parentStruct->structName;
						O<<"."<<v->sField->fieldName;
					}
				}				
				O<<"(&"<<v->pointsTo->name<<")("<<v->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			if (v->isWritten)
				O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
			else
				O<<"\",shape=parallelogram, style=filled, fillcolor=lightcyan]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_FIELD_ALIAS])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				
				if (v->pointsTo->sField == NULL)
					O<<"("<<v->pointsTo->name<<")";
				else
				{
					O<<"("<<v->pointsTo->sField->parentStruct->structName;
					O<<"."<<v->pointsTo->sField->fieldName<<")";
				}
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";						
			O<<"\",shape=parallelogram, style=filled, fillcolor=pink]\n";	
		}/*
		else if (v->nStatus[LOCAL_VAR_PTR])
		{
			if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";
			}
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";		
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=wheat3]\n";			
		}
		else if (v->nStatus[LOCAL_VAR_ALIAS])
		{
		if (v->pointsTo == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->pointsTo->name<<")";			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
		}
		else if (v->nStatus[LOCAL_VAR_A_ALIAS])
		{
			if (v->storeFrom == NULL)
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			else
			{
				O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
				O<<"("<<v->storeFrom->name<<")";			
				O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			}
			O<<"\",shape=Mdiamond, style=filled, fillcolor=skyblue3]\n";	
		}*/
		else if (v->nStatus[CALL_NODE])
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=tripleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_PARAM])
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=doubleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_RETURN] )
		{
			O<<get(get(vertex_index, G_abbr),*i)<<"[label=\""<<v->name;
			O<<":("<<lineNum<<":"<<lineNumOrder<<")";
			O<<"\",shape=octagon, style=filled, fillcolor=red]\n";
		}

	}
	
	
	//  a -> b [label="hello", style=dashed];
	graph_traits<MyGraphType>::edge_iterator ei, edge_end;
	for(tie(ei, edge_end) = edges(G_abbr); ei != edge_end; ++ei) 
	{
		
		int opCode = get(get(edge_iore, G_abbr),*ei);
		
		int paramNum = -1; //TOCHECK: should it be -2? Hui: 12/31/15
		if (opCode >= CALL_EDGE)
			paramNum = opCode - CALL_EDGE;
		
		int sourceV = get(get(vertex_index, G_abbr), source(*ei, G_abbr));
		int targetV = get(get(vertex_index, G_abbr), target(*ei, G_abbr));
		
		O<< sourceV  << "->" << targetV;
		
		if (opCode == ALIAS_EDGE )
			O<< "[label=\""<<"A"<<"\", color=powderblue]";
		//else if (opCode == CHILD_EDGE)
			//O<< "[label=\""<<"C"<<"\", color=green]";
		else if (opCode == PARENT_EDGE )
			O<< "[label=\""<<"P"<<"\", color=orange]";
		else if (opCode == DATA_EDGE )
			O<< "[label=\""<<"D"<<"\", color=green]";
		else if (opCode == FIELD_EDGE )
			O<< "[label=\""<<"F"<<"\", color=grey]";
		else if (opCode == DF_ALIAS_EDGE )
			O<< "[label=\""<<"DFA "<<"\", color=powderblue]";
		else if (opCode == DF_CHILD_EDGE )
			O<< "[label=\""<<"DFC "<<"\", color=purple]";
		else if (opCode == DF_INST_EDGE )
			O<< "[label=\""<<"S"<<"\", color=powderblue, style=dashed]";
		else if (opCode == CALL_PARAM_EDGE )
			O<< "[label=\""<<"CP"<<"\", color=red, style=dashed]";
		else if (opCode >= CALL_EDGE )
			O<< "[label=\""<<"Call Param "<<paramNum<<"\", color=red]";


		
		O<<" ;"<<std::endl;	
	}
	
	if (printAllLines)
	{
		int iS = impVertices.size();
		for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
		{
			NodeProps * ivp = get(get(vertex_props, G_abbr),*i);
			if (isTargetNode(ivp))
			{

				O<<iS+ivp->impNumber+1<<"[label=\"";
			
				std::set<int>::iterator set_i_i;
		
				//for (set_i_i = v->lineNumbers.begin(); set_i_i != v->lineNumbers.end(); set_i_i++)
				for (set_i_i = ivp->descLineNumbers.begin(); set_i_i != ivp->descLineNumbers.end(); set_i_i++)
				{
					O<<(*set_i_i)<<" ";
				}
				
				O<<"\", shape = rectangle]\n";
				O<<ivp->impNumber<<"->"<<iS+ivp->impNumber+1<<std::endl;
			}
			
		}
	}
	O<<"}";
}



void FunctionBFC::printFinalLineNums(std::ostream &O)
{
		
	O<<"EXIT VARIABLES"<<std::endl;
	
	graph_traits<MyTruncGraphType>::vertex_iterator i, v_end;

	for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
	{
		NodeProps * ivp = get(get(vertex_props, G_abbr),*i);
		if (isTargetNode(ivp))
		{
			if (ivp->eStatus >= EXIT_VAR_GLOBAL)
			{
				O<<ivp->name<<std::endl;
				std::set<int>::iterator set_i_i;		
				for (set_i_i = ivp->descLineNumbers.begin(); set_i_i != ivp->descLineNumbers.end(); set_i_i++)
				{
					O<<(*set_i_i)<<" ";
				}				
				O<<std::endl;
			}
		}
	}
	
	
	O<<"EXIT VAR FIELDS"<<std::endl;
	for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
	{
		NodeProps * ivp = get(get(vertex_props, G_abbr),*i);
		if (isTargetNode(ivp))
		{
			if (ivp->nStatus[EXIT_VAR_FIELD])
			{
				O<<ivp->getFullName()<<std::endl;
				std::set<int>::iterator set_i_i;		
				for (set_i_i = ivp->descLineNumbers.begin(); set_i_i != ivp->descLineNumbers.end(); set_i_i++)
				{
					O<<(*set_i_i)<<" ";
				}				
				O<<std::endl;
			}
		}
	}

	
	
	O<<"LOCAL VARIABLES"<<std::endl;
	for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
	{
		NodeProps * ivp = get(get(vertex_props, G_abbr),*i);
		if (isTargetNode(ivp))
		{
			if (ivp->nStatus[LOCAL_VAR])
			{
				if (ivp->isFakeLocal)
					continue;
			
				O<<ivp->name<<std::endl;
				std::set<int>::iterator set_i_i;		
				for (set_i_i = ivp->descLineNumbers.begin(); set_i_i != ivp->descLineNumbers.end(); set_i_i++)
				{
					O<<(*set_i_i)<<" ";
				}				
				O<<std::endl;
			}
		}
	}
	
	O<<"LOCAL VAR FIELDS"<<std::endl;
	for(tie(i,v_end) = vertices(G_abbr); i != v_end; ++i) 
	{
		NodeProps * ivp = get(get(vertex_props, G_abbr),*i);
		if (isTargetNode(ivp))
		{
			if (ivp->nStatus[LOCAL_VAR_FIELD])
			{
				std::set<NodeProps *> visited;
			
				NodeProps * fUpPtr = ivp->fieldUpPtr;
				NodeProps * topField = NULL;
				while (fUpPtr != NULL)
				{
					topField = fUpPtr;
					fUpPtr = fUpPtr->fieldUpPtr;
					if (visited.count(topField))
						fUpPtr = NULL;
					else
						visited.insert(topField);
				}
				
				if (topField == NULL)
					continue;
				
				if (topField->isFakeLocal)
					continue;
			
				O<<ivp->getFullName()<<std::endl;
				std::set<int>::iterator set_i_i;		
				for (set_i_i = ivp->descLineNumbers.begin(); set_i_i != ivp->descLineNumbers.end(); set_i_i++)
				{
					O<<(*set_i_i)<<" ";
				}				
				O<<std::endl;
			}
		}
	}

	//std::cout<<"Leaving printFinalLineNums for"<<std::endl;
	
	
}




void FunctionBFC::printTruncDotFiles(const char * strExtension, bool printImplicit)
{
  std::string s("DOT/");
 	
  std::string s_name = func->getName();
 	
	std::string extension(strExtension);
	
  s += s_name;
  s += extension;
 	
  std::ofstream ofs(s.c_str());
 	
	
	printToDotTrunc(ofs);
	
  //int x[] = {Instruction::Load, Instruction::Store};
  //printToDot(ofs_ls, NO_PRINT, PRINT_INST_TYPE, PRINT_LINE_NUM, x, 2 );
}


void FunctionBFC::printToDotTrunc(std::ostream &O)
{
	bool printLineNum = true;
	//int opSetSize = 0;
	
	
	property_map<MyGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G);
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
		
		NodeProps * v = get(get(vertex_props, G),*i);
		//int v_index = v->number;
				
		if (!v)
		{
			//std::cerr<<"Null V in printToDot\n";
			continue;
		}
		
		//if (v->fbb != NULL)
			//std::cout<<"BB is "<<v->fbb->getName()<<std::endl;

#ifdef DEBUG_OUTPUT		
		blame_info<<"V "<<v->name<<" exit Status is "<<v->eStatus;
		blame_info<<v->nStatus[CALL_NODE]<<" "<<v->nStatus[CALL_PARAM];
		blame_info<<" "<<v->nStatus[CALL_RETURN]<<std::endl;
#endif

		if (v->eStatus >= EXIT_VAR_PARAM)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")"<<"P# "<<v->eStatus - EXIT_VAR_PARAM;
				O<<"\",shape=invtriangle, style=filled, fillcolor=green]\n";		
		}
		else if (v->eStatus == EXIT_VAR_RETURN)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=invtriangle, style=filled, fillcolor=yellow]\n";		
		}
		else if (v->eStatus == EXIT_VAR_GLOBAL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=invtriangle, style=filled, fillcolor=purple]\n";		
		}
		else if (v->nStatus[EXIT_PROG])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
								if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=invtriangle, style=filled, fillcolor=dodgerblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_PTR] && v->pointsTo != NULL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"(&"<<v->pointsTo->name<<")("<<v->name<<")";
			if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";		
			if (v->isWritten)
				O<<"\",shape=invtriangle, style=filled, fillcolor=yellowgreen]\n";	
			else
				O<<"\",shape=invtriangle, style=filled, fillcolor=wheat3]\n";	

		}
		else if (v->nStatus[EXIT_VAR_ALIAS] && v->pointsTo != NULL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"~("<<v->pointsTo->name<<")";
							if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";						
				
				O<<"\",shape=invtriangle, style=filled, fillcolor=seagreen4]\n";	
		}
		else if (v->nStatus[EXIT_VAR_A_ALIAS] && v->storeFrom != NULL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"~("<<v->storeFrom->name<<")";
							if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";						
				
				O<<"\",shape=invtriangle, style=filled, fillcolor=powderblue]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD] && v->pointsTo != NULL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"~("<<v->pointsTo->name<<")";
							if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";						
				
				O<<"\",shape=invtriangle, style=filled, fillcolor=grey]\n";	
		}
		else if (v->nStatus[EXIT_VAR_FIELD_ALIAS] && v->pointsTo != NULL)
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"~("<<v->pointsTo->name<<")";
							if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";						
				
				O<<"\",shape=invtriangle, style=filled, fillcolor=grey58]\n";	
		}
		else if (v->eStatus == EXIT_OUTP)
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
								if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=invtriangle, style=filled, fillcolor=orange]\n";
		}
		else if (v->nStatus[CALL_NODE])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
								if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=tripleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_PARAM])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
								if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=doubleoctagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[CALL_RETURN])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
								if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=octagon, style=filled, fillcolor=red]\n";
		}
		else if (v->nStatus[LOCAL_VAR_PTR])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->pointsTo != NULL)
					O<<"("<<v->pointsTo->name<<")";
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
			if (v->isWritten)
				O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";	
			else
				O<<"\",shape=Mdiamond, style=filled, fillcolor=cornsilk]\n";	

		}
		else if (v->nStatus[LOCAL_VAR])
		{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=rectangle, style=filled, fillcolor=pink]\n";		
		
		}
		else if (v->nStatus[LOCAL_VAR_ALIAS] || v->nStatus[LOCAL_VAR_FIELD] ||
							v->nStatus[LOCAL_VAR_FIELD_ALIAS])
		{
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->pointsTo != NULL)
					O<<"("<<v->pointsTo->name<<")";
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=Mdiamond, style=filled, fillcolor=pink]\n";		
		}
		else if (v->llvm_inst != NULL)
		{	
			if (isa<Instruction>(v->llvm_inst))
			{
				Instruction * pi = cast<Instruction>(v->llvm_inst);	
				
				
				const llvm::Type * t = pi->getType();
				unsigned typeVal = t->getTypeID();
				
				if (pi->getOpcode() == Instruction::Alloca)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (v->fbb != NULL)
						O<<":("<<v->fbb->getName()<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					if (v->isLocalVar == true)
						O<<"\",shape=Mdiamond, style=filled, fillcolor=white]\n";
					else if ( strstr(v->name.c_str(), PARAM_REC) != NULL )
						O<<"\",shape=Mdiamond, style=filled, fillcolor=white]\n";
					else
						O<<"\",shape=Mdiamond, style=filled, fillcolor=white]\n";
				}	
				//else if (pi->getOpcode() == Instruction::GetElementPtr)
				else if (v->pointsTo != NULL)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name<<"(&"<<v->pointsTo->name<<")";
									if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=white]\n";
				}
				else if(pi->getOpcode() == Instruction::Call || pi->getOpcode() == Instruction::Invoke)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
									if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=box, style=filled, fillcolor=white]\n";		
				}
				else if (typeVal == Type::PointerTyID)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
									if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=yellow]\n";
				}
				else
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
									if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\"];"<<"\n";
					
				}
			}
			else
			{
				//cerr<<"Not an instruction "<<v->name<<"\n";
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";

				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";

				O<<"\",shape=Mdiamond, style=filled, fillcolor=purple]\n";	
				O.flush();
				
			}
		}
		else
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (v->fbb != NULL)
					O<<":("<<v->fbb->getName()<<")";
			if (printLineNum)
				O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
			O<<"\",shape=Mdiamond, style=filled, fillcolor=white]\n";
			O.flush();
		}
	}
	
  //  a -> b [label="hello", style=dashed];
  graph_traits<MyGraphType>::edge_iterator ei, edge_end;
  for(tie(ei, edge_end) = edges(G); ei != edge_end; ++ei) {
		
    int opCode = get(get(edge_iore, G),*ei);
		/*
    if (opSetSize)
		{
			for (int a = 0; a < opSetSize; a++)
			{
				if (opSet[a] == opCode)
	      {
					O<< get(get(vertex_index, G), source(*ei, G)) << "->" << get(get(vertex_index, G), target(*ei, G));
					
					if ( opCode && printInstType)
						if (opCode != ALIAS_OP)
							O<< "[label=\""<<Instruction::getOpcodeName(get(get(edge_iore, G),*ei))<<"\"]";
						else
							O<< "[label=\""<<"ALIAS"<<"\"]";
					
					O<<" ;"<<std::endl;	
	      }
			}
		}
		*/
		bool printImplicit = true;
		bool printInstType = true;
    
			if ( opCode || printImplicit )	
			{
				int sourceV = get(get(vertex_index, G), source(*ei, G));
				int targetV = get(get(vertex_index, G), target(*ei, G));
				
				
				NodeProps * sourceVP = get(get(vertex_props, G), source(*ei, G));
				NodeProps * targetVP = get(get(vertex_props, G), target(*ei, G));
				
							
				O<< sourceV  << "->" << targetV;
				
				if (!opCode && printImplicit)
					O<< "[color=grey, style=dashed]";
				else if ( opCode && printInstType)
	      {
					if (opCode == ALIAS_OP)
						O<< "[label=\""<<"ALIAS"<<"\"]";
					else if (opCode == GEP_BASE_OP )
						O<< "[label=\""<<"P--BASE"<<"\", color=powderblue]";
					else if (opCode == GEP_OFFSET_OP )
						O<< "[label=\""<<"P__OFFSET"<<"\", color=powderblue]";
					else if (opCode == GEP_S_FIELD_VAR_OFF_OP )
						O<< "[label=\""<<"P__ARR_OFF"<<"\", color=powderblue]";		
					else if (opCode == RESOLVED_L_S_OP )
						O<< "[label=\""<<"LS"<<"\", color=blue]";		
					else if (opCode >= GEP_S_FIELD_OFFSET_OP )
						O<< "[label=\""<<"P__FIELD # "<<opCode - GEP_S_FIELD_OFFSET_OP<<"\", color=powderblue]";
					else if (opCode == RESOLVED_OUTPUT_OP)
						O<< "[label=\""<<"R_OUTPUT"<<"\", color=green]";
					else if (opCode == RESOLVED_EXTERN_OP )
						O<< "[label=\""<<"R_EXTERN"<<"\", color=pink]";
					else if (opCode == RESOLVED_MALLOC_OP )
						O<< "[label=\""<<"R_MALLOC"<<"\", color=red]";
					else if (opCode == Instruction::Call )
					{
						int paramNum = MAX_PARAMS + 1;
						//std::cerr<<"Call from "<<sourceVP->name<<" to "<<targetVP->name<<std::endl;
						
						std::set<FuncCall *>::iterator fc_i = sourceVP->funcCalls.begin();
						
						for (; fc_i != sourceVP->funcCalls.end(); fc_i++)
						{
							FuncCall * fc = *fc_i;
							
							if (fc->funcName == targetVP->name)
							{
								paramNum = fc->paramNumber;
								break;
							}
							
							
							//std::cerr<<"     PN is "<<fc->paramNumber<<" for func "<<fc->funcName<<std::endl;
						}
						
						O<< "[color=red, label=\""<<Instruction::getOpcodeName(opCode)<<" "<<paramNum<<"\"]";
					}
					else if (opCode == Instruction::Store )
					{
						O<<"[label=\"";
						std::set<int>::iterator set_int_i;
						
						for (set_int_i = targetVP->storeLines.begin(); set_int_i != targetVP->storeLines.end(); set_int_i++)
							O<<(*set_int_i)<<" ";
						
						O<<"\", color = green]";
					}
					else
						O<< "[label=\""<<Instruction::getOpcodeName(opCode)<<"\"]";
	      }
				O<<" ;"<<std::endl;	
			}
		
  }
  O<<"}";
}


/* Prints blame relationships to dot file
 printImplicit - also prints implicit relationship
 printInstType - also prints the type for each instruction 
 printLineNum - also prints linenum the instruction generated from
 */
void FunctionBFC::printToDot(std::ostream &O, bool printImplicit, bool printInstType, 
															 bool printLineNum,int * opSet, int opSetSize)
{  
  property_map<MyGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G);
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
	#ifdef DEBUG_OUTPUT		
		blame_info<<"Getting v for "<<*i<<std::endl;
#endif
		
		NodeProps * v = get(get(vertex_props, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR				
			blame_info<<"V is NULL"<<std::endl;
			std::cerr<<"Null V in printToDot\n";
			#endif
			continue;
		}
		
		int v_index = v->number;
		int in_d = in_degree(v_index, G);
		int out_d = out_degree(v_index, G);
		
		

		
		if (in_d == 0 && out_d == 0)
		{

			bool shouldIContinue = true;
			
			if (v->isLocalVar)
				shouldIContinue = false;
		
			//if (!(v->isLocalVar))
				//continue;
			
			
			//if (isa<Instruction>(v->llvm_inst))
		//	{
			//	Instruction * pi = cast<Instruction>(v->llvm_inst);	
				//if (pi->getOpcode() == Instruction::Call)
					//shouldIContinue = false;
			//}	
			
			if (v->name.find("--") != std::string::npos)
				shouldIContinue = false;
			
			if (shouldIContinue)
				continue;
		}
		
		if (v->llvm_inst != NULL)
		{	
			if (isa<Instruction>(v->llvm_inst))
			{
				Instruction * pi = cast<Instruction>(v->llvm_inst);	
								
				const llvm::Type * t = pi->getType();
				unsigned typeVal = t->getTypeID();
				
				if (pi->getOpcode() == Instruction::Alloca)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					if (v->isLocalVar == true)
						O<<"\",shape=Mdiamond, style=filled, fillcolor=forestgreen]\n";
					else if ( strstr(v->name.c_str(), PARAM_REC) != NULL )
						O<<"\",shape=Mdiamond, style=filled, fillcolor=green]\n";
					else
						O<<"\",shape=Mdiamond, style=filled, fillcolor=darkseagreen]\n";
				}	
				//else if (pi->getOpcode() == Instruction::GetElementPtr)
				else if (v->pointsTo != NULL)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<"&"<<v->pointsTo->name<<"("<<v->name<<")";
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=powderblue]\n";
				}
				else if(pi->getOpcode() == Instruction::Call || pi->getOpcode() == Instruction::Invoke)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=box, style=filled, fillcolor=red]\n";		
				}
				else if (typeVal == Type::PointerTyID)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=yellow]\n";
				}
				else
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\"];"<<"\n";
					
				}
			}
			else
			{
				//cerr<<"Not an instruction "<<v->name<<"\n";
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=Mdiamond, style=filled, fillcolor=purple]\n";	
				
			}
		}
		else
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
			if (printLineNum)
				O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
			O<<"\",shape=Mdiamond, style=filled, fillcolor=purple]\n";
		}
	}
	
  //  a -> b [label="hello", style=dashed];
  graph_traits<MyGraphType>::edge_iterator ei, edge_end;
  for(tie(ei, edge_end) = edges(G); ei != edge_end; ++ei) {
		
    int opCode = get(get(edge_iore, G),*ei);
		
    if (opSetSize)
	{
	    for (int a = 0; a < opSetSize; a++)
		{
			if (opSet[a] == opCode) 
            {
				O<< get(get(vertex_index, G), source(*ei, G)) << "->" << get(get(vertex_index, G), target(*ei, G));
					
				if ( opCode && printInstType) {
					if (opCode != ALIAS_OP)
						O<< "[label=\""<<Instruction::getOpcodeName(get(get(edge_iore, G),*ei))<<"\"]";
					else
						O<< "[label=\""<<"ALIAS"<<"\"]";
                }
				O<<" ;"<<std::endl;	
	        }
		}
	}
		
    else
		{
			if ( opCode || printImplicit )	
			{
				int sourceV = get(get(vertex_index, G), source(*ei, G));
				int targetV = get(get(vertex_index, G), target(*ei, G));
				
				
				NodeProps * sourceVP = get(get(vertex_props, G), source(*ei, G));
				NodeProps * targetVP = get(get(vertex_props, G), target(*ei, G));
				
				/*
				 if (opCode == Instruction::Store)
				 {
				 NodeProps *v = get(get(vertex_props, G), source(*ei,G));
				 if (!v)
				 {
				 cerr<<"Null V(2) in printToDot\n";
				 continue;
				 }
				 
				 O<<sourceV<<"[label=\""<<v->name;
				 if (printLineNum)
				 O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				 O<<"\",shape=Mdiamond]\n";
				 }
				 */
				
				O<< sourceV  << "->" << targetV;
				
				if (!opCode && printImplicit)
					O<< "[color=grey, style=dashed]";
				else if ( opCode && printInstType)
	      {
					if (opCode == ALIAS_OP)
						O<< "[label=\""<<"ALIAS"<<"\"]";
					else if (opCode == GEP_BASE_OP )
						O<< "[label=\""<<"P--BASE"<<"\", color=powderblue]";
					else if (opCode == GEP_OFFSET_OP )
						O<< "[label=\""<<"P__OFFSET"<<"\", color=powderblue]";
					else if (opCode == RESOLVED_OUTPUT_OP)
						O<< "[label=\""<<"R_OUTPUT"<<"\", color=green]";
					else if (opCode == RESOLVED_EXTERN_OP )
						O<< "[label=\""<<"R_EXTERN"<<"\", color=pink]";
					else if (opCode == RESOLVED_MALLOC_OP )
						O<< "[label=\""<<"R_MALLOC"<<"\", color=red]";
					else if (opCode == Instruction::Call )
					{
						int paramNum = MAX_PARAMS + 1;
						//std::cerr<<"Call from "<<sourceVP->name<<" to "<<targetVP->name<<std::endl;
						
						std::set<FuncCall *>::iterator fc_i = sourceVP->funcCalls.begin();
						
						for (; fc_i != sourceVP->funcCalls.end(); fc_i++)
						{
							FuncCall * fc = *fc_i;
							
							if (fc->funcName == targetVP->name)
							{
								paramNum = fc->paramNumber;
								break;
							}
							
							
							//std::cerr<<"     PN is "<<fc->paramNumber<<" for func "<<fc->funcName<<std::endl;
						}
						
						
						
						O<< "[color=red, label=\""<<Instruction::getOpcodeName(opCode)<<" "<<paramNum<<"\"]";
					}
					else
						O<< "[label=\""<<Instruction::getOpcodeName(opCode)<<"\"]";
	      }
				O<<" ;"<<std::endl;	
			}
		}
  }
  O<<"}";
	
}



/* Prints blame relationships to dot file
 printImplicit - also prints implicit relationship
 printInstType - also prints the type for each instruction 
 printLineNum - also prints linenum the instruction generated from
 */
void FunctionBFC::printToDotPretty(std::ostream &O, bool printImplicit, bool printInstType, 
															 bool printLineNum,int * opSet, int opSetSize)
{  
  property_map<MyGraphType, edge_iore_t>::type edge_type
	= get(edge_iore, G);
	
 printLineNum = false;	
	
  O<<"digraph G {"<<std::endl;
  graph_traits<MyGraphType>::vertex_iterator i, v_end;
  for(tie(i,v_end) = vertices(G); i != v_end; ++i) 
	{
	#ifdef DEBUG_OUTPUT		
		blame_info<<"Getting v for "<<*i<<std::endl;
#endif
		
		NodeProps * v = get(get(vertex_props, G),*i);
		
		if (!v)
		{
#ifdef DEBUG_ERROR				
			blame_info<<"V is NULL"<<std::endl;
			std::cerr<<"Null V in printToDot\n";
			#endif
			continue;
		}
		
		int v_index = v->number;
		int in_d = in_degree(v_index, G);
		int out_d = out_degree(v_index, G);
		
		

		
		if (in_d == 0 && out_d == 0)
		{

			bool shouldIContinue = true;
			
			if (v->isLocalVar)
				shouldIContinue = false;
		
			//if (!(v->isLocalVar))
				//continue;
			
			
			//if (isa<Instruction>(v->llvm_inst))
		//	{
			//	Instruction * pi = cast<Instruction>(v->llvm_inst);	
				//if (pi->getOpcode() == Instruction::Call)
					//shouldIContinue = false;
			//}	
			
			if (v->name.find("--") != std::string::npos)
				shouldIContinue = false;
			
			if (shouldIContinue)
				continue;
		}
		
		if (v->llvm_inst != NULL)
		{	
			if (isa<Instruction>(v->llvm_inst))
			{
				Instruction * pi = cast<Instruction>(v->llvm_inst);	
								
				const llvm::Type * t = pi->getType();
				unsigned typeVal = t->getTypeID();
				
				if (pi->getOpcode() == Instruction::Alloca)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					if (v->isLocalVar == true)
						O<<"\",shape=rectangle, style=filled, fillcolor=pink]\n";
					else if ( strstr(v->name.c_str(), PARAM_REC) != NULL )
						O<<"\",shape=invtriangle, style=filled, fillcolor=green]\n";
					else
						O<<"\",shape=Mdiamond, style=filled, fillcolor=darkseagreen]\n";
				}	
				//else if (pi->getOpcode() == Instruction::GetElementPtr)
				else if (v->pointsTo != NULL)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=yellowgreen]\n";
				}
				else if(pi->getOpcode() == Instruction::Call || pi->getOpcode() == Instruction::Invoke)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=tripleoctagon, style=filled, fillcolor=red]\n";		
				}
				else if (typeVal == Type::PointerTyID && v->name.find("Constant") == string::npos)
				{
					O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\",shape=Mdiamond, style=filled, fillcolor=yellow]\n";
				}
				else
				{
					std::string constNum;
                    std::size_t plusPos = v->name.find("+");
					if (plusPos != string::npos)
					{
						unsigned plusPos2 = v->name.find("+",plusPos+1);
						constNum = v->name.substr(plusPos+1, plusPos2-plusPos-1);
						
						//constNum.append(0,v->name.substr(plusPos, plusPos2-plusPos));
					}
				
					if (v->name.find("Constant") == string::npos)
						O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
					else
						O<<get(get(vertex_index, G),*i)<<"[label=\""<<"C: "<<constNum;
					if (printLineNum)
						O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
					O<<"\"];"<<"\n";
					
				}
			}
			else
			{
				//cerr<<"Not an instruction "<<v->name<<"\n";
				O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
				if (printLineNum)
					O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				O<<"\",shape=Mdiamond, style=filled, fillcolor=purple]\n";	
				
			}
		}
		else
		{
			O<<get(get(vertex_index, G),*i)<<"[label=\""<<v->name;
			if (printLineNum)
				O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
			O<<"\",shape=Mdiamond, style=filled, fillcolor=purple]\n";
		}
	}
	
  //  a -> b [label="hello", style=dashed];
  graph_traits<MyGraphType>::edge_iterator ei, edge_end;
  for(tie(ei, edge_end) = edges(G); ei != edge_end; ++ei) {
		
    int opCode = get(get(edge_iore, G),*ei);
		
    if (opSetSize) {
	  for (int a = 0; a < opSetSize; a++) {
		if (opSet[a] == opCode)
	    {
		  O<< get(get(vertex_index, G), source(*ei, G)) << "->" << get(get(vertex_index, G), target(*ei, G));
					
		  if ( opCode && printInstType) {
			if (opCode != ALIAS_OP)
				O<< "[label=\""<<Instruction::getOpcodeName(get(get(edge_iore, G),*ei))<<"\"]";
			else
			    O<< "[label=\""<<"ALIAS"<<"\"]";
					
          }
		  O<<" ;"<<std::endl;	
	    }
	  }
	}
		
    else
		{
			if ( opCode || printImplicit )	
			{
				int sourceV = get(get(vertex_index, G), source(*ei, G));
				int targetV = get(get(vertex_index, G), target(*ei, G));
				
				
				NodeProps * sourceVP = get(get(vertex_props, G), source(*ei, G));
				NodeProps * targetVP = get(get(vertex_props, G), target(*ei, G));
				
				/*
				 if (opCode == Instruction::Store)
				 {
				 NodeProps *v = get(get(vertex_props, G), source(*ei,G));
				 if (!v)
				 {
				 cerr<<"Null V(2) in printToDot\n";
				 continue;
				 }
				 
				 O<<sourceV<<"[label=\""<<v->name;
				 if (printLineNum)
				 O<<":("<<v->line_num<<":"<<v->lineNumOrder<<")";
				 O<<"\",shape=Mdiamond]\n";
				 }
				 */
				
				O<< sourceV  << "->" << targetV;
				
				if (!opCode && printImplicit)
					O<< "[color=grey, style=dashed]";
				else if ( opCode && printInstType)
	      {
					if (opCode == ALIAS_OP)
						O<< "[label=\""<<"ALIAS"<<"\"]";
					else if (opCode == GEP_BASE_OP )
						O<< "[label=\""<<"BASE"<<"\", color=powderblue]";
					else if (opCode == GEP_OFFSET_OP )
						O<< "[label=\""<<"OFFSET"<<"\", color=powderblue]";
					else if (opCode == GEP_S_FIELD_VAR_OFF_OP)
						O<< "[label=\""<<"GEP--FIELD--OFFSET"<<"\", color=powderblue]";
					else if (opCode >= GEP_S_FIELD_OFFSET_OP)
						O<< "[label=\""<<"FIELD"<<"\", color=powderblue]";
					else if (opCode == RESOLVED_OUTPUT_OP)
						O<< "[label=\""<<"R_OUTPUT"<<"\", color=green]";
					else if (opCode == RESOLVED_EXTERN_OP )
						O<< "[label=\""<<"R_EXTERN"<<"\", color=pink]";
					else if (opCode == RESOLVED_MALLOC_OP )
						O<< "[label=\""<<"R_MALLOC"<<"\", color=red]";
					else if (opCode == Instruction::Call )
					{
						int paramNum = MAX_PARAMS + 1;
						//std::cerr<<"Call from "<<sourceVP->name<<" to "<<targetVP->name<<std::endl;
						
						std::set<FuncCall *>::iterator fc_i = sourceVP->funcCalls.begin();
						
						for (; fc_i != sourceVP->funcCalls.end(); fc_i++)
						{
							FuncCall * fc = *fc_i;
							
							if (fc->funcName == targetVP->name)
							{
								paramNum = fc->paramNumber;
								break;
							}
							
							
							//std::cerr<<"     PN is "<<fc->paramNumber<<" for func "<<fc->funcName<<std::endl;
						}
						
						
						
						O<< "[color=red, label=\""<<Instruction::getOpcodeName(opCode)<<" "<<paramNum<<"\"]";
					}
					else
					{
						O<< "[label=\""<<Instruction::getOpcodeName(opCode)<<"\"]";
						//O<< "[label=\""<<Instruction::getOpcodeName(opCode)<<" "<<opCode<<"\"]";

					}
	      }
				O<<" ;"<<std::endl;	
			}
		}
  }
  O<<"}";
	
}

/* 
 This function is for debugging purposes and prints out for each function
 - Name
 - Return Type 
 - Parameters
 - Name
 - Return Type
 */
void FunctionBFC::printFunctionDetails(std::ostream & O)
{
  O<<"Function "<<func->getName().data()<<" has return type of "<<returnTypeName(func->getReturnType(), std::string(" "))<<"\n";
  O<<"Parameters: \n";
	
  for(Function::arg_iterator af_i = func->arg_begin(); af_i != func->arg_end(); af_i++)
	{
		Value *v = af_i;
		
		if (v->hasName())
			O<<v->getName().str()<<" of type " << returnTypeName(v->getType(), std::string(" "))<< "\n";
	}
	
}
