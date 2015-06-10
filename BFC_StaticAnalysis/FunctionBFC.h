/*
 *  FunctionBFC.h
 *  Class definition for a function in the source program
 *  Main member function: firstPass
 *
 *  Created by Hui Zhang on 03/18/15
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _FUNCTION_BFC_H
#define _FUNCTION_BFC_H

#include "llvm/Analysis/Passes.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Compiler.h"
#include "llvm/InstVisitor.h"
#include "llvm/ADT/StringExtras.h"
//#include "llvm/Support/Streams.h" //TC not found in 3.3
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/APFloat.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Compiler.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/DominatorInternals.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.def"

#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>

#include <boost/graph/graphviz.hpp>   
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/iteration_macros.hpp>

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "NodeProps.h"
#include "ModuleBFC.h"
#include "FunctionBFCCFG.h"
#include "Parameters.h"
#include "ExitVars.h"

namespace std
{
    using namespace __gnu_cxx;
}

//TC: the way to use boost
enum vertex_var_name_t { vertex_var_name };
enum vertex_props_t {vertex_props};
enum edge_iore_t { edge_iore };
namespace boost {
  BOOST_INSTALL_PROPERTY(vertex, var_name);
  BOOST_INSTALL_PROPERTY(edge, iore);
  BOOST_INSTALL_PROPERTY(vertex, props);
}

using namespace llvm;
using namespace boost;

class ExitSuper;
class ExitVariable;
class ExitProgram;
class ExitOutput;
struct FuncParam;
class FunctionBFC;
class ExternFunctionBFC;
class FunctionBFCCFG;
class FunctionBFCBB;

struct CallInfo
{
	std::string funcName;
	int paramNumber;
};

struct FuncParam
{
	int paramNumber;
	std::string paramName;
	short writeStatus;
	std::set<FuncCall *> calledFuncs;
};

/*  Could add more metadata if we need to
 !7 = metadata !{
 i32,      ;; Tag (see below)
 metadata, ;; Context
 metadata, ;; Name
 metadata, ;; Reference to compile unit where defined
 i32,      ;; Line number where defined
 metadata  ;; Type descriptor
 }
 */


// TODO:  add this to the destructor
// This is for when we have multiple references to one field, we can then 
//  collapes all those references into one (essentially the first time we see one)
struct CollapsePair
{
// this is associated with a malloc so we'll need to free this
	const char * nameFieldCombo;
	NodeProps * collapseVertex;
	NodeProps * destVertex;
};


struct BFCLoopInfo
{
	set<int> lineNumbers;
	FunctionBFC * bf;
};

struct FuncCallSEElement
{
	ImpFuncCall * ifc;
	NodeProps * paramEVNode;
};

struct FuncCallSE
{
	NodeProps * callNode;
	std::vector<FuncCallSEElement *> parameters;
	std::vector<FuncCallSEElement *> NEVparams; // non EV params
};

struct ExternFunctionBFC {
    std::string funcName;
    std::set<int> paramNums;
    
    ExternFunctionBFC(std::string s) {
        funcName = s;
    }
};

struct LocalVar {
    int definedLine;
    std::string varName;
};

struct FuncStores {
    NodeProps *receiver;
    NodeProps *contents;
    int line_num;
    int lineNumOrder;
};


// Comparators //
struct eqstr {
    bool operator()(const char *s1, const char *s2) const {
        return strcmp(s1, s2) == 0;
    }
};

struct ltstr {
    bool operator()(const char *s1, const char *s2) const {
        return strcmp(s1, s2) < 0;
    }
};

typedef adjacency_list<hash_setS, vecS, bidirectionalS, property<vertex_props_t, NodeProps *>, property<edge_iore_t, int> > MyGraphType;

// needed a new graph type to support multiple edges coming from a node
typedef adjacency_list<vecS, vecS, bidirectionalS, property<vertex_props_t, NodeProps *>, property<edge_iore_t, int> > MyTruncGraphType;

typedef std::hash_map<const char*, ExternFunctionBFC*, std::hash<const char*>, eqstr> ExternFuncBFCHash;

typedef std::hash_map<const char*, FunctionBFC*, std::hash<const char*>, eqstr> FuncBFCHash;

typedef std::hash_map<const char*, NodeProps*, std::hash<const char*>, eqstr> RegHashProps;

typedef std::hash_map<int, int> LineNumHash;

typedef std::hash_map<const char*, std::set<const char*, ltstr>, std::hash<const char*>, eqstr> ImpRegSet; 

typedef std::hash_map<const char*, ExternFunctionBFC*, std::hash<const char*>, eqstr> ExternFuncBFCHash;


class FunctionBFC {
///////////////////////// Constructors/Destructor ///////////////////////////
public:
    FunctionBFC(Function *F, std::set<const char*> &kFN); //kFN=knownFuncNames

    ~FunctionBFC();
////////////////////////// Variables ////////////////////////////////////////
public:
    std::ofstream blame_info;

private:
    //Graph representations
    MyGraphType G; // All nodes
    MyTruncGraphType G_trunc; // only important nodes
    MyTruncGraphType G_abbr; // only exit variables and calls

    LineNumHash lnm;
    
    std::vector<FuncStores *> allStores;
    std::set<const char*> funcCallNames;

    //Underlying LLVM Objects
    Function *func;
    //Upwards pointer to module
    Module *M;
    //Pointer to ModuleBFC
    ModuleBFC *mb;

    //Exit Variables/Programs
    std::vector<ExitVariable *> exitVariables;
    std::vector<ExitProgram *> exitPrograms;
    ExitOutput *exitOutput;

    //Pointers
    std::set<NodeProps *> pointers;
    
    //Summary Information
    int startLineNum; // start of function
    int endLineNum; // End of function
    int numParams; // Num of params for function, MAX_PARAMS+1 for variable argument
    int numPointerParams; // Num of params that are pointers
    bool voidReturn; // true->void
    string moduleName; // name of module this func found
    string modulePathName; // full path to the module
    bool moduleSet; // true if above two variables have been set
    bool isVarLen; // true if parameters are of variable length
    bool isBFCPoint; // whether or not it's explicit blame point(main,
                       //   V param/ V return)
    int impVertCount; // total imp vertices minus the ones not exported

    //All Func Calls occuring in the function
    std::set<FuncCall *> funcCalls;
    //All local variables in function
    std::vector<LocalVar *> localVars;
    //Important Nodes
    std::set<NodeProps *> impVertices;
    //Implicit BFC Nodes
    ImpRegSet iReg;

    RegHashProps variables; //Hash of registers that gets turned into graph
    //CFG for the class
    FunctionBFCCFG *cfg;
    //Set of all valid line numbers in program
    std::set<int> allLineNums;
    //All other function names in this module
    std::set<const char*> knownFuncNames;

	std::vector< std::pair<NodeProps *, NodeProps *> > seAliases;
	std::vector< std::pair<NodeProps *, NodeProps *> > seRelations;
	std::vector< FuncCallSE *> seCalls;
///////////////////////// Generic Public Calls ///////////////////////////////
public:
    void firstPass(Function *F, std::vector<NodeProps *> &globalVars,
            ExternFuncBFCHash &efInfo, std::ostream &blame_file,
            std::ostream &blame_se_file, std::ostream &call_file, int &numMissing);

    std::string getSourceFuncName() {return func->getName().str();}
    std::string getModuleName() {return moduleName;}
    std::string getModulePathName() {return modulePathName;}
  
    int numExitVariables() {return exitVariables.size();}
    int getStartLineNum() {return startLineNum;}
    int getEndLineNum() {return endLineNum;}
	
	void setModule(Module *mod) {M = mod;} 
	void setModuleBFC(ModuleBFC *modb){ mb = modb; }
	
	void setModuleName(std::string rawName);
	void setModulePathName(std::string rawName);


private:
    const char *getTruncStr(const char *fullStr);

  //////////////////// Important Vertices ////////////////////////////////////////
	void populateImportantVertices();
	void recursiveExamineChildren(NodeProps *v, NodeProps *origVP, std::set<int> &visited);
	
	void resolveIVCalls();
	void resolveCallsDomLines();
	
	void resolveSideEffects();
	void recursiveSEAliasHelper(std::set<NodeProps *> & visited, NodeProps *orig, NodeProps *target);
	NodeProps *resolveSideEffectsCheckParentEV(NodeProps *vp, std::set<NodeProps *> &visited);
	NodeProps *resolveSideEffectsCheckParentLV(NodeProps *vp, std::set<NodeProps *> &visited);

	void resolveSideEffectsHelper(NodeProps *rootVP, NodeProps *vp, std::set<NodeProps *> &visited);
	void resolveSideEffectCalls();
	
	void addSEAlias(NodeProps *source, NodeProps *target);
	void addSERelation(NodeProps *source, NodeProps *target);
	void resolveLooseStructs();
	void resolveTransitiveAliases();
	void resolveFieldAliases();

	void makeNewTruncGraph();
	void trimLocalVarPointers();
	int checkCompleteness();
  /////////////////////////////////////////////////////////////////////////////////////////

//////////////////////// Graph Generation ////////////////////////////////////
    //Graph generation that calls all the rest of these functions
    void genGraph(ExternFuncBFCHash &efInfo);
    //Wrapper function for boost edge adding
    void addEdge(const char *source, const char *dest, Instruction *pi);
    //Edge generation
    void genEdges(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum, std::set<NodeProps *> &seenCall);
	//generate edges based on opcode
    void geCall(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum, std::set<NodeProps *> &seenCall);
														 
	std::string geGetElementPtr(User *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);
														 
	void geDefault(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);
														 
	void geLoad(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);
														 
	void geStore(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);
														 
	void geMemAtomic(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);

    void geAtomicLoadPart(Value *ptr, Value *val, Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);

    void geAtomicStorePart(Value *ptr, Value *val, Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);

    void geBitCast(Instruction *pi, std::set<const char*, ltstr> &iSet, property_map<MyGraphType, vertex_props_t>::type props, property_map<MyGraphType, edge_iore_t>::type edge_type, int &currentLineNum);
														 
	void geBlank(Instruction *pi);
		
	void geInvoke();
		
	void addImplicitEdges(Value *v, std::set<const char*, ltstr> &iSet,property_map<MyGraphType, edge_iore_t>::type edge_type,const char *vName, bool useName);

//////////////////////// Graph CFGs  ////////////////////////////////////////
	// Takes care of assigning aliases based on the CFG using reaching definitions
    void resolveStores();
	// Cases where a store happens on a line where we also need the def in to count
	//  Cases like 
	//   (1) int x = 5;
	//   (2) x++
	//   Line (2) kills (1) but we still need the def of (1) to count
	int resolveBorderLine(NodeProps *storeVP, NodeProps *sourceVP, NodeProps *origStoreVP, int sourceLine);

    void sortCFG();
    void printCFG();

    void adjustMainGraph();
	void addControlFlowChildren(NodeProps *oP, NodeProps *tP);
	void goThroughAllAliases(NodeProps *oP, NodeProps *tP, std::set<NodeProps *> &visited);
//////////////////////// Graph Analysis //////////////////////////////////////
    void findOrCreateExitVariable(std::string LLVM_node, std::string var_name);
    void addExitVar(ExitVariable *ev){exitVariables.push_back(ev);}
    void addExitProg(ExitProgram *ep){exitPrograms.push_back(ep);}
    ExitProgram *findOrCreateExitProgram(std::string &name);
    void addFuncCalls(FuncCall *fc);
	void identifyExternCalls(ExternFuncBFCHash &efInfo);
	
	int checkForUnreadReturn(NodeProps *v, int v_index);
	void determineBFCHoldersLite();
	bool isLibraryOutput(const char *tStr);
	void determineBFCForOutputVertexLite(NodeProps *v, int v_index);
	void determineBFCForVertexLite(NodeProps *v);
	void LLVMtoOutputVar(NodeProps *v);
	
	void handleOneExternCall(ExternFunctionBFC *efb, NodeProps *v);
	void transferExternCallEdges(std::vector< std::pair<int, int> > &blamedNodes, int callNode, std::vector< std::pair<int, int> > &blameeNodes);
	

////////////////////  Graph Collapsing  ///////////////////////////////////////
	// Parent function
    void collapseGraph();
	// Implicit collapse -- need some work
	void collapseImplicit();
	void collapseRedundantImplicit();
	void findLocalVariable(int v_index, int top_index);
	void transferImplicitEdges(int alloc_num, int bool_num);
    void deleteOldImplicit(int v_index);
	
	// Main function that does the work
	int collapseAll(std::set<int> &collapseInstructions);

	// Sub functions that do the work
	bool shouldTransferLineNums(NodeProps *v);
	int transferEdgesAndDeleteNode(NodeProps *dN, NodeProps *rN,  bool transferLineNumbers=true);

    //It's for when multiple GEP point to one field, we don't need all the references
	void collapseRedundantFields();

	// Need some work
	void collapseEH();
    void collapseIO();
    int collapseInvoke();
	void handleMallocs();

	////////////////////////////////////////////////////////////////////////////////
	
    ///////////// Graph Analysi(Pointers) ////////////////////////////////////////
	void resolvePointers2();
	void resolvePointersHelper2(NodeProps *origV, int origPLevel, NodeProps *targetV, std::set<int> &visited, std::set<NodeProps *> &tempPointers, NodeProps *alias);
	void resolveLocalAliases2(NodeProps *exitCand, NodeProps *currNode, std::set<int> &visited, NodeProps *exitV);
																			
	void resolveAliases2(NodeProps *exitCand, NodeProps *currNode, std::set<int> &visited, NodeProps *exitV);
																		
	short checkIfWritten2(NodeProps *currNode, std::set<int> &visited);
																		
	void resolvePointersForNode2(NodeProps *v, std::set<NodeProps *> &tempPointers);		
	void resolveLocalDFA(NodeProps *v, std::set<NodeProps *> &pointers);		
	
	void resolveArrays(NodeProps *origV, NodeProps *v, std::set<NodeProps *> &tempPointers);

	void resolveDataReads();

	int pointerLevel(const llvm::Type *t, int counter);

	void calcAggregateLN();
	void calcAggregateLNRecursive(NodeProps *ivp, std::set<NodeProps *> &vStack, std::set<NodeProps *> &vRevisit);
	void calcAggCallRecursive(NodeProps *ivp);
	bool isTargetNode(NodeProps *ivp);
	
	///////////////////////////////////////////////////////////////////////////////

    //////////////////////// LLVM Parser /////////////////////////////////////////
public:
    std::string returnTypeName(const llvm::Type *t, std::string prefix);
private:
    // This is the function that calls all the rest of these
    void parseLLVM(std::vector<NodeProps *> &globalVars);

    // LLVM UTILITY
    void structDump(Value *compUnit);
    void structResolve(Value *v, int fieldNum, NodeProps *fieldVP);

    // GENERAL
    void populateGlobals(std::vector<NodeProps *> &gvs);
    void adjustLocalVars();
    bool varLengthParams();
    void printValueIDName(Value *v);
    const char * calcMetaFuncName(RegHashProps &variables, Value *v, bool isTradName, int opNum, std::string nonTradName, int currentLineNum);
    void printCurrentVariables();

    // IMPLICIT
    void generateImplicits();
    void handleLoops(LoopBase<BasicBlock, Loop> *lb);
    bool errorRetCheck(User *v);
    
    void propagateConditional(DominatorTreeBase<BasicBlock> *DT , const DomTreeNodeBase<BasicBlock> *N, const char *condName, BasicBlock *termNode);
    void handleOneConditional(DominatorTreeBase<BasicBlock> *DT , const DomTreeNodeBase<BasicBlock> *N, BranchInst *br);
    void handleAllConditionals(DominatorTreeBase<BasicBlock> *DT, const DomTreeNodeBase<BasicBlock> *N, LoopInfoBase<BasicBlock, Loop> &LI, std::set<BasicBlock *> &termBlocks);
	void gatherAllDescendants(DominatorTreeBase<BasicBlock> *DT , BasicBlock *original, BasicBlock *&b, std::set<BasicBlock *> &cfgDesc, std::set<BasicBlock *> &visited);
	
    // EXPLICIT
    // Hash that contains unique monotonically increasing ID hashed to name of var
    void determineFunctionExitStatus();
    void examineInstruction(Instruction *pi, int &varCount, int &currentLineNum, RegHashProps &variables, FunctionBFCBB *fbb);
    void createNPFromConstantExpr(ConstantExpr *ce, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void genDILocationInfo(Instruction *pi, int &currentLineNum, FunctionBFCBB *fbb);
    bool parseDeclareIntrinsic(Instruction *pi, int &currentLineNum, FunctionBFCBB *fbb);
    void grabVarInformation(llvm::Value *varDeclare);    
    
    void ieGen_LHS(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGen_LHS_Alloca(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGen_Operands(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGen_OperandsStore(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGen_OperandsGEP(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGen_OperandsAtomic(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieDefault(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieBlank(Instruction *pi, int &currentLineNum);
    void ieMemAtomic(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieInvoke(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieCall(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieLoad(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieGetElementPtr(User *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieAlloca(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieStore(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieBitCast(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
    void ieSelect(Instruction *pi, int &varCount, int &currentLineNum, FunctionBFCBB *fbb);
 
    
    ////////////////////  OUTPUT ////////////////////////////////////////////////////////////
public:
	void exportEverything(std::ostream &O, bool reads);
	void exportSideEffects(std::ostream &O);
	void exportCalls(std::ostream &O, ExternFuncBFCHash &efInfo);
	void exportParams(std::ostream &O);
	void moreThanOneEV(int &numMultipleEV, int &afterOp1, int &afterOp2);
	void debugPrintLineNumbers(NodeProps *ivp, NodeProps *target, int locale);
    
    void printFunctionDetails(std::ostream &O);
    void printDotFiles(const char *strExtension, bool printImplicit);
	void printTruncDotFiles(const char *strExtension, bool printImplicit);
	void printFinalDot(bool printAllLines, std::string ext);
	void printFinalDotPretty(bool printAllLines, std::string ext);
	void printFinalDotAbbr(std::string ext);
	
private:
    void printToDot(std::ostream &O, bool printImplicit, bool printInstType, 
									bool printLineNum,int *opSet, int opSetSize);
	void printToDotPretty(std::ostream &O, bool printImplicit, bool printInstType, 
									bool printLineNum,int *opSet, int opSetSize );				
    void printToDotTrunc(std::ostream &O);
	void printFinalLineNums(std::ostream &O);
	
  /////////////////////////////////////////////////////////////////////////////////////////
	

};

#endif
