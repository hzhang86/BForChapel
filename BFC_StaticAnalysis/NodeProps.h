/*
 *  NodeProps.h
 *  
 *  Data structure to keep info of global var/local var/parameter 
 *
 *  Created by Hui Zhang on 02/26/15.
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */
 

#ifndef _NODE_PROPS_H
#define _NODE_PROPS_H
 
#include <string>
#include <set>
#include <vector>
#include "llvm/IR/Value.h"
#include "Parameters.h"

using namespace llvm;
using namespace std;

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

namespace std
{
  using namespace __gnu_cxx;
}



#define NOT_RESOLVED 0
#define PART_RESOLVED_BLAMED 1
#define PART_RESOLVED_BLAMEE 2
#define FULL_RESOLVED_BLAMED 3
#define FULL_RESOLVED_BLAMEE 4

#define NODE_PROPS_SIZE 20

//#define NO_EXIT  0

#define ANY_EXIT 0
#define EXIT_PROG 1
#define EXIT_OUTP 2
#define EXIT_VAR 3 


#define EXIT_VAR_ALIAS 4
#define EXIT_VAR_PTR   5
#define EXIT_VAR_FIELD 6
#define EXIT_VAR_FIELD_ALIAS 7

#define EXIT_VAR_A_ALIAS 8

#define LOCAL_VAR 9
#define LOCAL_VAR_ALIAS 10
#define LOCAL_VAR_PTR   11
#define LOCAL_VAR_FIELD 12
#define LOCAL_VAR_FIELD_ALIAS 13

#define LOCAL_VAR_A_ALIAS  14

#define CALL_NODE 16
#define CALL_PARAM 17
#define CALL_RETURN 18


//#define EXIT_VAR_GLOBAL      19
//#define EXIT_VAR_RETURN     20
//#define EXIT_VAR_PARAM  EXIT_VAR_RETURN

#define EXIT_VAR_UNWRITTEN -1
#define NO_EXIT          0
//#define EXIT_PROG        1
//#define EXIT_OUTP        2
#define EXIT_VAR_GLOBAL  3
#define EXIT_VAR_RETURN  4
#define EXIT_VAR_PARAM  EXIT_VAR_RETURN


//#define EXIT_VAR 99  // Temporary, will be overwritten



#define NO_EDGE 0
#define PARENT_EDGE 1
#define CHILD_EDGE  2
#define ALIAS_EDGE  3
#define DATA_EDGE   4
#define FIELD_EDGE  5
#define DF_ALIAS_EDGE 6
#define DF_INST_EDGE 7
#define DF_CHILD_EDGE 8
#define CALL_PARAM_EDGE 9
#define CALL_EDGE   10


//#define CALL_NODE 0
//#define CALL_PARAM 1
//#define CALL_RETURN 2

 
 
 typedef std::hash_map<int, int> LineReadHash;

 
class ExitSuper;
class NodeProps;
struct StructField;
struct StructBlame;
struct ImpNodeProps;
class FunctionBlameBB;
 
struct FuncCall {
  int paramNumber;
  string funcName;
	int lineNum;
  ExitSuper * es;
	short resolveStatus;
  NodeProps * param;
	bool outputEvaluated;
	
  FuncCall(int pN, string fn)
  {
    paramNumber = pN;
    funcName = fn;
		lineNum = 0;
		resolveStatus = NOT_RESOLVED;
		param = NULL;
		outputEvaluated = false;
  }
  
};

struct ImpFuncCall 
{
	int paramNumber;
	
	// For call nodes it is a 'callNode', for 
	//  parameter node this is the VP for the parameter
	NodeProps * callNode;
	
	ImpFuncCall(int pn, NodeProps * cn)
	{
		paramNumber = pn;
		callNode = cn;
	}
};


struct ReadProps
{
	int lineNumber;
	int lineCount;
};


class NodeProps {

public:

	bool operator<(NodeProps rhs) { return line_num < rhs.line_num; }
	
	
	static bool LinenumSort(const NodeProps * d1, const NodeProps * d2)
	{
		return d1->line_num < d2->line_num;
	}
	
	string & getFullName();
	int getParamNum(std::set<NodeProps *> & visited);
	void getStructName(std::string & structName, std::set<NodeProps *> & visited);
	
    int number;
	int impNumber;
	
	
	// this is for imp vertices, assume they are all exported
	//  default value is true, if we don't want to export,
	//  then we make this false
	bool isExported;
	
	
    string name;
	
	bool calcName;
	
	// If the Vertex is a EV value > 0, otherise 0
	int paramNum; 
	
	string fullName;
    int line_num;
	
	bool calcAgg;  // has the aggregate line number been calculated
	// for this vertex yet, used in calcAggregateLN()
	bool calcAggCall;
	
	
    bool isGlobal;
	bool isLocalVar;
	bool isFakeLocal;
	bool isStructure;
	
	// arrays treated slightly different
	bool isArr;
	bool isPtr;
	
	short ptrStatus;
	
	bool isWritten;
	
	// true if this is a param that takes the blame for an extern call
	bool isExternCallParam;
	
	bool deleted;
	
	bool resolved;
	
	//short exitStatus;
	
	// You could theoretically have one that is a return and a param (most likely)
	//  or in the case of a function pointer a call_node and a param/return
	
	//bool callStatus[3];  
	
	short eStatus;
	bool  nStatus[NODE_PROPS_SIZE];
	
	// Pointer Info (raw, refined into the sets for IVVs below
	set<NodeProps *> aliasesIn;
	set<NodeProps *> aliasesOut;
	set<NodeProps *> fields; // for structures
	set<NodeProps *> GEPs;
	set<NodeProps *> loads;
	set<NodeProps *> nonAliasStores;
	set<NodeProps *> arrayAccess;
	set<NodeProps *> almostAlias; // it's an alias to one instantiation of
	// a variable, thought technically the pointers arent' the same level
	
	// The list of nodes that resolves to the VP through a RESOLVED_LS_OP
	set<NodeProps *> resolvedLS;
	
	// The list of nodes that are resolved from the VP through a R_LS
	set<NodeProps *> resolvedLSFrom;
	
	// A subset of the resolvedLS nodes that write to the data range
	//  thus causing potential side effects to all the other nodes that
	//  interact through the R_L_S
	set<NodeProps *> resolvedLSSideEffects;
	
	
	set<NodeProps *> blameeEVs;
	
	
	// Important Vertex Vectors
	set<NodeProps *> parents;
	set<NodeProps *> children;

	// DF_CHILD_EDGE
	set<NodeProps *> dfChildren; 
	set<NodeProps *> dfParents;
	
	set<NodeProps *> aliases; // taken care of with pointer analysis
	set<NodeProps *> dfAliases; // aliases dictated by data flow
	set<NodeProps *> dataPtrs; // as is this
	set<ImpFuncCall *> calls;
	
	// Field info
	StructField * sField;
	StructBlame * sBlame;
	
    set<int> lineNumbers;
	set<int> descLineNumbers;
	
	// Mostly used with Fortran, these are the line numbers that 
	//  get put into a given paramater.  Fortran uses parm.* pain
	// in the ass struct parameters so we need to keep track of these
	// so that way the fields can access the parent struct and get the 
	// line numbers
	set<int> externCallLineNumbers;
	
	// the set of lines that are dominated by this vertex
	set<int> domLineNumbers;
	
	set<ImpFuncCall *> descCalls;
	set<NodeProps *> descParams;
	set<NodeProps *> aliasParams;
	
    set<NodeProps *> pointedTo;
	set<NodeProps *> dominatedBy;
	
	
	// For DataFlow analysis (reaching defs for primitives)
	set<NodeProps *> genVP;
	set<NodeProps *> killVP;
	
	set<NodeProps *> inVP;
	set<NodeProps *> outVP;	
	
	NodeProps * storeFrom;
	set<NodeProps *> storesTo;
	set<int> storeLines;
	set<int> borderLines;
	/////////////////////////
	
	
	// More DataFlow Analysis (reaching defs for pointers)
	set<NodeProps *> genPTR_VP;
	set<NodeProps *> killPTR_VP;
	
	set<NodeProps *> inPTR_VP;
	set<NodeProps *> outPTR_VP;	

	set<int> storePTR_Lines;
	set<int> borderPTR_Lines;
	
	set<NodeProps *>  dataWritesFrom;
	/////////////////////////////////////////////
	
	
	
	///////// For Alias only operations /////
	LineReadHash readLines;
	//////////////////////////////////////////////
	
	
	
	
	set<NodeProps *> suckedInEVs;
	
	
    set<FuncCall *> funcCalls;
    Value * llvm_inst;
	
	// For BitCast Instructions
	Value * collapsed_inst;
	
	BasicBlock * bb;
    FunctionBlameBB * fbb;
	
  //Up Pointers
	NodeProps * dpUpPtr;
	NodeProps * dfaUpPtr;
	
	// TODO: Probably should tweak how we handle this for
	// field aliases
	NodeProps * fieldUpPtr;

	// For field aliases
	// TODO: probably should make this a vector
	NodeProps * fieldAlias;
	
    NodeProps * pointsTo;
    NodeProps * exitV;
  
    // For Constants
    int constValue;
	
	
	// For each line number, the order at which the statement appeared
	int lineNumOrder;
  
    NodeProps(int nu, string na, int ln, Value * pi)
    {
        number = nu;
		impNumber = -1;
        name = na;
		
		paramNum = 0;
		
		// We have calculated the more elaborate name
		calcName = false;
		
        line_num = ln;
		lineNumOrder = 0;
        llvm_inst = pi; // the value could be a constant here
		collapsed_inst = NULL;
		
		dpUpPtr = this;
		dfaUpPtr = NULL;
		fieldUpPtr = NULL;
		fieldAlias = NULL;
		
		isExported = true;
		
		storeFrom = NULL;
		
		deleted = false;
		
		calcAgg = false;
		calcAggCall = false;
		
		resolved = false;
		
		sField = NULL;
		sBlame = NULL;
		
		bb = NULL;
		fbb = NULL;
		
		//exitStatus = NO_EXIT;
		
		eStatus = NO_EXIT;
		
		//callStatus[CALL_NODE] = false;
		//callStatus[CALL_PARAM] = false;
		//callStatus[CALL_RETURN] = false;
				
		for (int a = 0; a < NODE_PROPS_SIZE; a++)
			nStatus[a] = false;
		
		
        isGlobal = false;
		isLocalVar = false;
		isFakeLocal = false;
		
		isStructure = false;
		isPtr = false;
		isWritten = false;
		
		
		isExternCallParam = false;
		
        //printf("Address of pointsTo for %s is 0x%x\n", name.c_str(), pointsTo);
        pointsTo = NULL;
		exitV = NULL;
	
  }
	
	~NodeProps()
	{
		parents.clear();
		children.clear();
		aliases.clear();
		dataPtrs.clear();
		aliasesIn.clear();
		aliasesOut.clear();
		
		funcCalls.clear();
		
		fields.clear();
		GEPs.clear();
		loads.clear();
		nonAliasStores.clear();
		
		lineNumbers.clear();
		descLineNumbers.clear();
		pointedTo.clear();
		dominatedBy.clear();
		
		std::set<ImpFuncCall *>::iterator vec_ifc_i;
		for (vec_ifc_i = calls.begin(); vec_ifc_i != calls.end(); vec_ifc_i++)
			delete (*vec_ifc_i);
			
		calls.clear();
			
	}
  
    void addFuncCall(FuncCall * fc);// {funcCalls.push_back(fc);}
  
};

#endif
