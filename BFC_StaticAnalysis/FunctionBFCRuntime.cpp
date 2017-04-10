/*
 *  FunctionBFCGraph.cpp
 *  Implementation of graph generation
 *
 *  Created by Hui Zhang on 03/03/17.
 *  Copyright 2017 __MyCompanyName__. All rights reserved.
 *
 */

#include "FunctionBFC.h"

using namespace std;

int FunctionBFC::needExProc(string callName)
{
  if (callName.find("chpl_getPrivatizedCopy") == 0)
    return GET_PRIVATIZEDCOPY;
  else if (callName.find("chpl_getPrivatizedClass") == 0)
    return GET_PRIVATIZEDCLASS;
  else if (callName.find("chpl_gen_comm_get") == 0)
    return GEN_COMM_GET;
  else if (callName.find("chpl_gen_comm_put") == 0)
    return GEN_COMM_PUT;
  else if (callName.find("accessHelper") == 0)
    return ACCESSHELPER;
  else if (callName.find("chpl__convertRuntimeTypeToValue") == 0)
    return CONVERTRTTYPETOVALUE;

  // if none of above matches
  else return NO_SPECIAL;
}


void FunctionBFC::specialProcess(Instruction *pi, int specialCall, string callName)
{
#ifdef DEBUG_SPECIAL_PROC
  blame_info<<"Entering specialProcess for "<<callName<<endl;
#endif
  if (specialCall == GET_PRIVATIZEDCOPY) { 
    spGetPrivatizedCopy(pi);
  }

  else if (specialCall == GET_PRIVATIZEDCLASS) {
    spGetPrivatizedClass(pi);
  }

  else if (specialCall == GEN_COMM_GET) {
    spGenCommGet(pi);
  }

  else if (specialCall == GEN_COMM_PUT) {
    spGenCommPut(pi);
  }

  else if (specialCall == CONVERTRTTYPETOVALUE) {
    spConvertRTTypeToValue(pi);
  }
// TODO: Check whether the following 3 functions are necessary
/*  
  else if (specialCall == ACCESSHELPER) {
    spAccessHelper(pi);
  }

  else if (specialCall == WIDE_PTR_GET_ADDR) {
    spWidePtrGetAddr(pi);
  }

  else if (specialCall == WIDE_PTR_GET_NODE) {
    spWidePtrGetNode(pi);
  }
*/
}

      
void FunctionBFC::spGetPrivatizedCopy(Instruction *pi)
{
/*
    string retName;
    if (pi->hasName()) 
      retName.insert(0,pi->getName().data());
    else {
      char tempBuf[20];
      sprintf(tempBuf, "0x%x", pi);
      string name(tempBuf);
      retName.insert(0, name.c_str());
    }
*/
    NodeProps *objectPid, *pidObject;
    Value::use_iterator ui, ue;
    for (ui=pi->use_begin(),ue=pi->use_end(); ui!=ue; ui++) {
      if (Instruction *stInst = dyn_cast<Instruction>(*ui)) {
        if (stInst->getOpcode() == Instruction::Store) {
          User::op_iterator op_i = stInst->op_begin();
          Value *first = *op_i, *second = *(++op_i);
          
          string secondName;
          if (second->hasName())
            secondName = second->getName().str();
          else {
            char tempBuf[20];
            sprintf(tempBuf, "0x%x", second);
            string name(tempBuf);
            secondName.insert(0, name.c_str());
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"Weird: unamed object from pid: "<<secondName<<endl;
#endif
          }
          
          if (variables.count(secondName) >0)
            pidObject = variables[secondName];
          else {
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"Error:(pidObject)"<<secondName<<" Unfound in vars"<<endl;
#endif
            return;
          }
          break; //Have found pidObject, break for loop
        }
      }
    }
      // get objectPid
    Value *pid = *(pi->op_begin());
    if (Instruction *pidInst = dyn_cast<Instruction>(pid)) {
      if (pidInst->getOpcode() == Instruction::Load) {
        Value *first = *(pidInst->op_begin());
        string firstName;
        if (first->hasName()) 
          firstName = first->getName().str();
        else {
          char tempBuf[20];
          sprintf(tempBuf, "0x%x", first);
          string name(tempBuf);
          firstName.insert(0, name.c_str());
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"Weird: unamed pid from object: "<<firstName<<endl;
#endif
        }

        if (variables.count(firstName) >0)
          objectPid = variables[firstName];
        else {
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"Error:(objectPid)"<<firstName<<" Unfound in vars"<<endl;
#endif
          return;
        }
      }
    }

    std::pair<NodeProps*, NodeProps*> pidToObj(objectPid, pidObject);
    distObjs.push_back(pidToObj);
    objectPid->isPid = true;
    objectPid->myObj = pidObject;
    pidObject->isObj = true;
    pidObject->myPid = objectPid;
}

//getPrivatizedClass + bitcast = getPrivatizedCopy
void FunctionBFC::spGetPrivatizedClass(Instruction *pi)
{
/*
    string retName;
    if (pi->hasName()) 
      retName.insert(0,pi->getName().data());
    else {
      char tempBuf[20];
      sprintf(tempBuf, "0x%x", pi);
      string name(tempBuf);
      retName.insert(0, name.c_str());
    }
*/
    NodeProps *objectPid, *pidObject;
    Value::use_iterator ui, ue, ui2, ue2;
    for (ui=pi->use_begin(),ue=pi->use_end(); ui!=ue; ui++) {
      if (Instruction *i = dyn_cast<Instruction>(*ui)) {
        if (i->getOpcode() == Instruction::BitCast) {
          for (ui2=i->use_begin(),ue2=i->use_end(); ui2!=ue2; ui2++) {
            if (Instruction *i2 = dyn_cast<Instruction>(*ui2)) {
              if (i2->getOpcode() == Instruction::Store) {

                User::op_iterator op_i = i2->op_begin();
                Value *first = *op_i, *second = *(++op_i);
                string secondName;
                if (second->hasName())
                  secondName = second->getName().str();
                else {
                  char tempBuf[20];
                  sprintf(tempBuf, "0x%x", second);
                  string name(tempBuf);
                  secondName.insert(0, name.c_str());
#ifdef DEBUG_SPECIAL_PROC
                  blame_info<<"Weird2: unamed object from pid: "<<secondName<<endl;
#endif
                }
          
                if (variables.count(secondName) >0)
                  pidObject = variables[secondName];
                else {
#ifdef DEBUG_SPECIAL_PROC
                  blame_info<<"Error2:(pidObject)"<<secondName<<" Unfound"<<endl;
#endif
                  return;
                }
              }
            }
          }
        }
      }
    }
    // get objectPid
    Value *pid = *(pi->op_begin());
    if (Instruction *pidInst = dyn_cast<Instruction>(pid)) {
      if (pidInst->getOpcode() == Instruction::Load) {
        Value *first = *(pidInst->op_begin());
        string firstName;
        if (first->hasName()) 
          firstName = first->getName().str();
        else {
          char tempBuf[20];
          sprintf(tempBuf, "0x%x", first);
          string name(tempBuf);
          firstName.insert(0, name.c_str());
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"Weird2: unamed pid from object: "<<firstName<<endl;
#endif
        }

        if (variables.count(firstName) >0)
          objectPid = variables[firstName];
        else {
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"Error2:(objectPid)"<<firstName<<" Unfound"<<endl;
#endif
          return;
        }
      }
    }

    std::pair<NodeProps*, NodeProps*> pidToObj(objectPid, pidObject);
    distObjs.push_back(pidToObj);
    objectPid->isPid = true;
    objectPid->myObj = pidObject;
    pidObject->isObj = true;
    pidObject->myPid = objectPid;
}


// This function is currently NOT fully evaluated
// It will add ProblemSpace (domain) to Pid as well due to convertRuntimeTypeToValue2
void FunctionBFC::spConvertRTTypeToValue(Instruction *pi)
{
    //Not sure if the operand#1 of this func is pid always 3/29/17
    // for case like: pidTempNode=%99, pidNode=%type_tmp_chpl3
    /* store i64* %type_tmp_chpl3, i64** %ret_to_arg_ref_tmp__chpl9
     * %99 = load i64** %ret_to_arg_ref_tmp__chpl9
     * call void @chpl__convertRuntimeTypeToValue6(i64 %98, i64* %99,..)
     * %101 = load i64* %type_tmp_chpl3
     * store i64 %101, i64* %B_chpl
     */

  //basically pidTemp is the RLS from pid, but we dont hv that info at this time
  Value *pidTemp = pi->getOperand(1);
  string typeName = returnTypeName(pidTemp->getType(), std::string("")); 
  if (typeName == "*Int") { //pid should always be i64*
    NodeProps *pidTempNode = NULL, *pidNode = NULL;
    Value *pid = NULL;
    string pidTempName, pidName;
    //get pidTemp's name first
    if (pidTemp->hasName())
      pidTempName = pidTemp->getName().str();
    else {
      char tempBuf[20];
      sprintf(tempBuf, "0x%x", pidTemp);
      string name(tempBuf);
      pidTempName.insert(0, name.c_str());
    }

    if (Instruction *i = dyn_cast<Instruction>(pidTemp)) {
      if (i->getOpcode() == Instruction::Load) {
        Value *first = i->getOperand(0); //first=%ret_to_arg_ref_tmp__chpl9
        Value::use_iterator ui, ue;
        for (ui=first->use_begin(),ue=first->use_end(); ui!=ue; ui++) {
          if (Instruction *i2 = dyn_cast<Instruction>(*ui)) {
            if (i2->getOpcode() == Instruction::Store) {
              
              Value *first2 = i2->getOperand(0);
              if (first == i2->getOperand(1) && //make sure first is the recipient of store
                  first2->getValueID() != Value::ConstantFPVal &&
                  first2->getValueID() != Value::ConstantIntVal && 
                  first2->getValueID() != Value::ConstantPointerNullVal && 
                  first2->getValueID() != Value::ConstantExprVal) {
                pid = first2; //found what we are looking for
                
                //get pid's name first
                if (pid->hasName())
                  pidName = pid->getName().str();
                else {
                  char tempBuf[20];
                  sprintf(tempBuf, "0x%x", pid);
                  string name(tempBuf);
                  pidName.insert(0, name.c_str());
                }
#ifdef DEBUG_SPECIAL_PROC
                blame_info<<"We found pid (convertRT): "<<pidName<<endl;
#endif
                break;
              }
            } //store
          }
        }
      } //load
    } //pidTemp

    if (!pidName.empty() && !pidTempName.empty()) {
      if (variables.count(pidName) && variables.count(pidTempName)) {
        pidNode = variables[pidName];
        pidTempNode = variables[pidTempName];
        if (pidNode && pidTempNode) {
          pidNode->isPid = true;
          pidTempNode->isPid = true;
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"New pid added: "<<pidName<<" "<<pidTempName<<endl;
#endif
        }
      }
    }
  }

  else {
#ifdef DEBUG_SPECIAL_PROC
    blame_info<<"This is not the convertRuntimeTypeToValue we are looking for"<<endl;
#endif
  }
}


void FunctionBFC::spGenCommGet(Instruction *pi)
{
    Value *dstAddr = pi->getOperand(0);
    NodeProps *dstAddrNode = NULL;
    dstAddrNode = getNodeBitCasted(dstAddr);
    // sometimes the dstAddrNode wasn't bitcasted, just get itself
    if (!dstAddrNode) {
#ifdef DEBUG_SPECIAL_PROC
      blame_info<<"Weird: what's opCode for comm_get?"<<endl;
#endif
      string argName;
      if (dstAddr->hasName()) 
        argName = dstAddr->getName().str();
      else {
        char tempBuf[20];
        sprintf(tempBuf, "0x%x", dstAddr);
        string name(tempBuf);
        argName.insert(0, name.c_str());
      }
        
      if (variables.count(argName) >0)
        dstAddrNode = variables[argName];
      else {
#ifdef DEBUG_SPECIAL_PROC
        blame_info<<"Error: (dstAddrNode)"<<argName<<" unFound in vars"<<endl;
#endif
        return;
      }
    }
    
    dstAddrNode->isRemoteWritten = true;
    dstAddrNode->isWritten = true; //Not sure whether we need to distinguish these 2 written
}

// Almost the same as spGenCommGet, except we getOperand(2) now
void FunctionBFC::spGenCommPut(Instruction *pi)
{
    Value *dstAddr = pi->getOperand(2);
    NodeProps *dstAddrNode = NULL;
    dstAddrNode = getNodeBitCasted(dstAddr);
    // sometimes dstAddrNode wasn't bitcasted, just get itself
    if (!dstAddrNode) {
#ifdef DEBUG_SPECIAL_PROC
      blame_info<<"Weird: what's opCode for comm_put?"<<endl;
#endif
      string argName;
      if (dstAddr->hasName()) 
        argName = dstAddr->getName().str();
      else {
        char tempBuf[20];
        sprintf(tempBuf, "0x%x", dstAddr);
        string name(tempBuf);
        argName.insert(0, name.c_str());
      }
        
      if (variables.count(argName) >0)
        dstAddrNode = variables[argName];
      else {
#ifdef DEBUG_SPECIAL_PROC
        blame_info<<"Error: (dstAddrNode)"<<argName<<" unFound in vars"<<endl;
#endif
        return;
      }
    }
    
    dstAddrNode->isRemoteWritten = true;
    dstAddrNode->isWritten = true; //Not sure whether we need to distinguish these 2 written
}


void FunctionBFC::spAccessHelper(Instruction *pi)
{
    string instName, argName;
    if (pi->hasName())
      instName = pi->getName().str();
    else {
      char tempBuf[20];
      sprintf(tempBuf, "0x%x", pi);
      string name(tempBuf);
      instName.insert(0, name.c_str());
    }

    Value *arg = pi->getOperand(1);
    if (arg->hasName())
      instName = arg->getName().str();
    else {
      char tempBuf[20];
      sprintf(tempBuf, "0x%x", arg);
      string name(tempBuf);
      argName.insert(0, name.c_str());
    }

    NodeProps *instNode, *argNode;
    if (variables.count(instName)>0 && variables.count(argName)>0) {
      instNode = variables[instName];
      argNode = variables[argName];

      //TO CHECK: what relationship should we build here?
      argNode->arrayAccess.insert(instNode);
    }
}


void FunctionBFC::resolvePidAliases()
{
#ifdef DEBUG_SPECIAL_PROC
    blame_info<<"\nWe are in resolvePidAliases now\n";
#endif
    graph_traits<MyGraphType>::vertex_iterator i, v_end; 

    // 1st round pidAlias search: backward straight
    for (tie(i,v_end) = vertices(G); i != v_end; ++i) {
	  NodeProps *v = get(get(vertex_props, G),*i);
	  int v_index = get(get(vertex_index, G),*i);
      if (!v)
        continue;

      if (v->isPid) {
        std::set<int> visited;
        resolvePidAliasForNode_bw(v, visited);
      }
    }

    // 2nd round pidAlias search: forward 
    // we need to forward check each pid to add in more aliases
    // for cases when more than one aliasOut exist in the middle
    for (tie(i,v_end) = vertices(G); i != v_end; ++i) {
      NodeProps *v = get(get(vertex_props, G),*i);
      if (!v)
        continue;
      
      if (v->isTempPid || v->isPid) {
        v->isPid = true; //set all temp pid from 1st round to be real pid
        std::set<int> visited;
        resolvePidAliasForNode_fw(v, visited);
      }
    } 

    // 3rd round pidAlias search: merge in & out
    for (tie(i,v_end) = vertices(G); i != v_end; ++i) {
	  NodeProps *v = get(get(vertex_props, G),*i);
      if (!v)
        continue;
      if (v->isTempPid)
        v->isPid = true; //set new temp pids gotten from the 2nd round search above
      //merge all pidAliasesIn&pidAliasesOut in to pidAliases
      if (v->isPid) {
        std::set<NodeProps *>::iterator si, se;
        for (si=v->pidAliasesIn.begin(), se=v->pidAliasesIn.end(); si!=se; si++)
          v->pidAliases.insert(*si);
        for (si=v->pidAliasesOut.begin(), se=v->pidAliasesOut.end(); si!=se; si++)
          v->pidAliases.insert(*si);
      }
    }

    // 4th round pidAlias search: make everyone equal
    // Transitive pidAliases, just like processing aliases
    resolveTransitivePidAliases();
}


void FunctionBFC::resolveObjAliases()
{
#ifdef DEBUG_SPECIAL_PROC
    blame_info<<"\nWe are in resolveObjAliases now\n";
#endif
    graph_traits<MyGraphType>::vertex_iterator i, v_end; 
	
    for (tie(i,v_end) = vertices(G); i != v_end; ++i) {
	  NodeProps *v = get(get(vertex_props, G),*i);
      if (!v)
        continue;

      if (v->isObj) {
        //Get the pointer level of obj
        int origPtrLevel=0;
	    if (v->llvm_inst != NULL) {
	      if (isa<Instruction>(v->llvm_inst)) {
		    Instruction *pi = cast<Instruction>(v->llvm_inst);	
            const llvm::Type *origT = pi->getType();	
		    origPtrLevel = pointerLevel(origT,0);
	      }
	      else if (isa<ConstantExpr>(v->llvm_inst)) {
		    ConstantExpr *ce = cast<ConstantExpr>(v->llvm_inst);
		    const llvm::Type *origT = ce->getType();
	        origPtrLevel = pointerLevel(origT, 0);
	      }
	    }
        if (origPtrLevel <= 1) {//should be >=2
#ifdef DEBUG_SPECIAL_PROC
          blame_info<<"Woops! obj isn't 2-level ptr"<<endl;
#endif
          continue;
        }
        v->objAliasesIn = v->aliasesIn;
        v->objAliasesOut = v->aliasesOut;
        v->objAliases = v->aliases;
      }
    }

    // Transitive objAliases, just like processing aliases
    resolveTransitiveObjAliases();
}


// Backward search
void FunctionBFC::resolvePidAliasForNode_bw(NodeProps *currNode, std::set<int> &visited)
{
#ifdef DEBUG_SPECIAL_PROC
    blame_info<<"Entering recursively resolvePidAliasForNode_bw for "<<currNode->name<<endl;
#endif
    if (visited.count(currNode->number)) 
      return;
    visited.insert(currNode->number);

    boost::graph_traits<MyGraphType>::out_edge_iterator e_beg, e_end;
	e_beg = boost::out_edges(currNode->number, G).first;	//edge iterator begin
	e_end = boost::out_edges(currNode->number, G).second; // edge iterator end
    // Backward tracing for pidAliases
    for (; e_beg!=e_end; e_beg++) {
      int opCode = get(get(edge_iore, G), *e_beg);
      if (opCode == Instruction::Store) {  
        NodeProps *sourceV = get(get(vertex_props,G), target(*e_beg,G));
  
        boost::graph_traits<MyGraphType>::out_edge_iterator e_beg2, e_end2;
	    e_beg2 = boost::out_edges(sourceV->number, G).first;	//edge iterator begin
	    e_end2 = boost::out_edges(sourceV->number, G).second; // edge iterator end

        for (; e_beg2!=e_end2; e_beg2++) {
          int opCode2 = get(get(edge_iore, G), *e_beg2);
          if (opCode2 == Instruction::Load) {
            NodeProps *targetV = get(get(vertex_props,G), target(*e_beg2,G));
            // For pid, their type should always be i64* as far as I know
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"We find a pidAliasIn: "<<targetV->name<<" for "<<currNode->name<<endl;
#endif
            //Copy some atrributes from currNode
            if (!targetV->isPid && (currNode->isPid||currNode->isTempPid))
              targetV->isTempPid = true; //Later, we'll change all tempPid to pid
            if (!targetV->isWritten && currNode->isWritten)
              targetV->isWritten = true;

            currNode->pidAliasesIn.insert(targetV);
            targetV->pidAliasesOut.insert(currNode);
            //Start recursion on targetV
            resolvePidAliasForNode_bw(targetV, visited);
          } //if opCode2=Load
        }//for edges2
      }//if opCode=Store
    }//for edges
}


// Forward search
void FunctionBFC::resolvePidAliasForNode_fw(NodeProps *currNode, std::set<int> &visited)
{
#ifdef DEBUG_SPECIAL_PROC
    blame_info<<"Entering recursively resolvePidAliasForNode_fw for "<<currNode->name<<endl;
#endif
    if (visited.count(currNode->number)) 
      return;
    visited.insert(currNode->number);

    boost::graph_traits<MyGraphType>::in_edge_iterator e_beg, e_end;
	e_beg = boost::in_edges(currNode->number, G).first;	//edge iterator begin
	e_end = boost::in_edges(currNode->number, G).second; // edge iterator end
    // farward tracing for pidAliases
    for (; e_beg!=e_end; e_beg++) {
      int opCode = get(get(edge_iore, G), *e_beg);
      if (opCode == Instruction::Load) {  
        NodeProps *sourceV = get(get(vertex_props,G), source(*e_beg,G));
  
        boost::graph_traits<MyGraphType>::in_edge_iterator e_beg2, e_end2;
	    e_beg2 = boost::in_edges(sourceV->number, G).first;	//edge iterator begin
	    e_end2 = boost::in_edges(sourceV->number, G).second; // edge iterator end

        for (; e_beg2!=e_end2; e_beg2++) {
          int opCode2 = get(get(edge_iore, G), *e_beg2);
          if (opCode2 == Instruction::Store) {
            NodeProps *targetV = get(get(vertex_props,G), source(*e_beg2,G));
            // For pid, their type should always be i64* as far as I know
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"We find a pidAliasOut: "<<targetV->name<<" for "<<currNode->name<<endl;
#endif
            //Copy some atrributes from currNode
            if (!targetV->isPid && (currNode->isPid||currNode->isTempPid))
              targetV->isTempPid = true; //Later, we'll change all tempPid to pid
            if (!targetV->isWritten && currNode->isWritten)
              targetV->isWritten = true;

            currNode->pidAliasesOut.insert(targetV);
            targetV->pidAliasesIn.insert(currNode);
            
            //Start recursion on targetV
            resolvePidAliasForNode_fw(targetV, visited);
          } //if opCode2=Store
        }//for edges2
      }//if opCode=Load
    }//for edges

    //03/30/17: we need to take care of some special case: a/b are fields
    //When a(early) and b(late) are collapsePair, and b is not finally deleted
    //due to some reasons we added, then we need to mark b as Pid if a is Pid
    string fieldName;
    if (currNode->uniqueNameAsField)
      fieldName = string(currNode->uniqueNameAsField);
    if (!fieldName.empty()) {
      if (cpHash.count(fieldName) >0) {
        if (currNode == cpHash[fieldName]) { //currNode is destVertex
          std::set<NodeProps *>::iterator si = currNode->collapseNodes.begin();
          std::set<NodeProps *>::iterator se = currNode->collapseNodes.end();
          for (; si != se; si++) {
            NodeProps *cpNode = *si;
            int v_index = cpNode->number;
            int in_d = in_degree(v_index, G);
            int out_d = out_degree(v_index, G);
            // if collapseVertex still has edges attached, then it wasn't deleted
            if ((in_d+out_d) > 0) {
#ifdef DEBUG_SPECIAL_PROC
              blame_info<<"We find a pidAliasOut(cp): "<<cpNode->name<<" for "<<currNode->name<<endl;
#endif
              //Copy some atrributes from currNode
              if (!cpNode->isPid && (currNode->isPid||currNode->isTempPid))
                cpNode->isTempPid = true; //Later, we'll change all tempPid to pid
              //We set cpNode as currNode's pidAliasOut because destVertex always
              //appears early than the collapseVertex
              currNode->pidAliasesOut.insert(cpNode);
              cpNode->pidAliasesIn.insert(currNode);
            
              //Start recursion on targetV
              resolvePidAliasForNode_fw(cpNode, visited);
            } // cpNode not actually deleted
          } // iterate all collapseNodes
        } // currNode exists as destVertex (the one that's retained)
      } // if the collapsable pair exists
    } // if currNode is a field
}


void FunctionBFC::resolveTransitivePidAliases()
{
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
    for (tie(i, v_end)=vertices(G); i!=v_end; i++) {
      NodeProps *v = get(get(vertex_props, G), *i);
      if (!v)
        continue;
      
      std::set<NodeProps*>::iterator si, si2;
      for (si=v->pidAliases.begin(); si!=v->pidAliases.end(); si++) {
        NodeProps *al1 = *si;
        for (si2=si; si2!=v->pidAliases.end(); si2++) {
          NodeProps *al2 = *si2;
          if (al1 == al2)
            continue;
          if (al1 != v) {
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"Transi pA: "<<al1->name<<" and "<<al2->name<<endl;
#endif
            al1->pidAliases.insert(al2);
          }
          if (al2 != v) {
#ifdef DEBUG_SPECIAL_PROC
            blame_info<<"Transi pA: "<<al2->name<<" and "<<al1->name<<endl;
#endif
            al2->pidAliases.insert(al1);
          }
        } //3rd for loop
      } //2nd for loop
    } //1sr for loop
}

// Since objAliases was computed before, we just do the copy
void FunctionBFC::resolveTransitiveObjAliases()
{
    graph_traits<MyGraphType>::vertex_iterator i, v_end;
    for (tie(i, v_end)=vertices(G); i!=v_end; i++) {
      NodeProps *v = get(get(vertex_props, G), *i);
      if (!v)
        continue;
      if (v->isObj) {
        std::set<NodeProps*>::iterator si;
        for (si=v->objAliases.begin(); si!=v->objAliases.end(); si++) {
          NodeProps *vA = *si;
          vA->isObj = true;
          vA->objAliasesIn = vA->aliasesIn;
          vA->objAliasesOut = vA->aliasesOut;
          vA->objAliases = vA->aliases;
        }
      }
    }
}
