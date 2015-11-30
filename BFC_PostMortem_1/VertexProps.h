#ifndef VERTEX_PROPS_DEC
#define VERTEX_PROPS_DEC


#include <set>
#include <string>
#include <vector>


#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

namespace std
{
  using namespace __gnu_cxx;
}


#define NODE_PROPS_SIZE 20


#define ANY_EXIT 0
#define EXIT_VAR 1 
#define EXIT_PROGG 2
#define EXIT_OUTPP 3

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

#define NO_EXIT          0
#define EXIT_PROG        1
#define EXIT_OUTP        2
#define EXIT_VAR_GLOBAL  3
#define EXIT_VAR_RETURN  4
#define EXIT_VAR_PARAM  EXIT_VAR_RETURN


using namespace std;
class BlameFunction;
//struct BlameField;

struct StructBlame;
struct StructField;
struct SideEffectParam;

class VertexProps;

 typedef std::hash_map<int, int> LineReadHash;


struct FuncCall 
{
	int paramNumber;
	
	// If we're looking at the VP for
	//  any of the params, this is the 
	// call node, but if we're looking
	// at the VP for the call node, this
	// is the param node
	VertexProps * callNode;
	
	FuncCall(int pn, VertexProps * cn)
	{
		paramNumber = pn;
		callNode = cn;
	}
	
};

class VertexProps {
 
 public:

	// Name of vertex
	std::string name;
	
	// the full struct name complete with all of the struct field parents recursively up to root
	std::string fsName;
	
	// These are created fields for bookkeeping purposes, they should be deleted after 
	//  every frame that is parsed
	bool isDerived;
	
	// Line vertex operation came from
	int declaredLine;
	
	// Vertex status (everything but exit variable status)
	int nStatus[NODE_PROPS_SIZE];
	
	// Exit variable status
	int eStatus;
 
	// Line Number for just that node (not its children)
	std::set<int> lineNumbers;
	
	// Line Number for line and all its descendants
	std::set<int> descLineNumbers;
	
	std::set<int> seLineNumbers;
	
	// Line numbers dominated by the vertex
	std::set<int> domLineNumbers;
	
	std::set<VertexProps *> descParams;
	std::set<VertexProps *> aliasParams;
	
	// Was this vertex or any of its descendants written to (or just read)
	bool isWritten;
	
	// Up Pointer to the parent Blame Function 
	BlameFunction * BF;
	
	// All the calls this vertex is associatied with
	std::vector<FuncCall *> calls;

	// DATA FLOW/POINTER RELATIONS
	std::set<VertexProps *> parents; // data flow parents
	std::set<VertexProps *> children; // data flow children
	std::set<VertexProps *> aliases;  // aliases
	
	
	VertexProps * aliasUpPtr; // only used when there is an Exit Variable (which has
	// slightly different alias rules) has an alias of a local variable 
	//  local var --- aliasUpPtr---> exitVariable
	
	std::set<VertexProps *> dfAliases; // aliases dictated by data flow
	VertexProps * dfaUpPtr;
	
	
	std::set<VertexProps *> dfChildren;
	
	std::set<VertexProps *> dataPtrs; // vertices in data space
	VertexProps * dpUpPtr;
	
	std::set<VertexProps *> fields; // fields off of this node
	
	// The list of nodes that resolves to the VP through a RESOLVED_LS_OP
	set<VertexProps *> resolvedLS;
	
	// The list of nodes that are resolved from the VP through a R_LS
	set<VertexProps *> resolvedLSFrom;
	
	// A subset of the resolvedLS nodes that write to the data range
	//  thus causing potential side effects to all the other nodes that
	//  interact through the R_L_S
	set<VertexProps *> resolvedLSSideEffects;
	
	
	VertexProps * storeFrom;
	set<VertexProps *> storesTo;
	
	
	std::set<VertexProps *> params; // params in the case this node is a call
	
	
	// Only for EVs (and their fields)
	set<VertexProps *> readableVals;
	set<VertexProps *> writableVals;

	
	
	bool calcAgg;
	bool calcAggCall;
	
	
	int calleePar;
	set<int> callerPars;
	
	
	
	
	/***************** TEMP VARIABLES  *******************/
	
	// taken out in clearPastData()
	// (tempParents, tempChildren, tempSELines, tempIsWritten, tempLine,
	//   calcAggSE, tempIsWritten)
	
	// not taken out in clearPastData()
	// TODO: check to see why this is
	// ( tempAliases)
	
	// set if we have already calculated the side effects for a given function
	bool calcAggSE;
	
	// For dealing with transfer functions, based on the blamed node in the
	//   transfer function these values may change
	std::set<VertexProps *> tempParents;
	std::set<VertexProps *> tempChildren;
	
	int tempLine;  // for temporary call nodes, it's the declared line of the call node
	
	// We need to set this because of rules we have about propagating blame
	// through read only data values, if we say it's written (which it is somewhere
	// in the call) then we allow the line number of the statement to be propagated up
	bool tempIsWritten;
	
	
	std::set<int> tempSELines;  // for the line numbers of calls involving side effects
	
	
	std::set<SideEffectParam *> tempAliases;
	
	// Side effect relations will change depending on the calling context
	//  so we clear them out after every instance
	std::set<VertexProps *> tempRelations; // for side effects
	std::set<VertexProps *> tempRelationsParent;  // same thing, reverse mapping
	
	
	// NON TRANSFER FUNCTION/SIDE EFFECT VARIABLES
	// The following values are for creating temporary fake vertices for propagating blame
	// up fields.  We don't techincally need to create these, but it makes bookkeeping much 
	// easier, we also need to make sure to free all the memory for these vertices
	std::set<VertexProps *>  tempFields;
	
	
	// These are set up in the case where an exit variable has an alias of a local variable,
	// this stores all the immediate fields of the local variable
	std::set<VertexProps *> temptempFields;
	
	
	
	// changes every time(so temp) but always explicitly set and only used for debugging output
	//  so no need to worry about resetting the variable
	short addedFromWhere;
	
	/************* END TEMP VARIABLES  *******************/
	
	/*** SPECIAL ALIAS TEMP VARIABlE ******************/
	float weight; /// regular weight
	float skidWeight;  /// weight when trying to account for skid
	/********************************************************/
	
	
	
	// In case this is a field, this data structure contains relevant information
	//BlameField * bf;
	
	StructBlame * bs;
	StructBlame * sType;
	StructField * sField;
	VertexProps * fieldUpPtr;
	
	VertexProps * fieldAlias;
	
	// General type of this vertex
	std::string genType;

	
	VertexProps(string na)
	{
		name = na;
		//aliasedFrom = NULL;
		//bf = NULL;
		bs = NULL;
		sField = NULL;
		eStatus = NO_EXIT;
		
		weight = 0.0;
		skidWeight = 0.0;
		
		calleePar = -1;
		addedFromWhere = -1;
		
		isDerived = false;
		
		calcAgg = false;
		calcAggCall = false;
		
		calcAggSE = false;
		
		storeFrom = NULL;
		isWritten = false;
		tempIsWritten = false;
		
		fieldUpPtr = NULL;
		dpUpPtr = NULL;
		dfaUpPtr = NULL;
		
		fieldAlias = NULL;
		
		for (int a = 0; a < NODE_PROPS_SIZE; a++)
			nStatus[a] = false;
		
		tempLine = 0;
		declaredLine = 0;
		}

	void propagateTempLineUp(std::set<VertexProps *> & visited, int lineNum);
	int findBlamedExits(std::set<VertexProps *> & visited, int lineNum);
	void findSEExits(std::set<VertexProps *> & blamees);
	void populateSERBlamees(std::set<VertexProps *> & visited, std::set<VertexProps *> & blamees);


 
	void parseVertex(ifstream & bI, BlameFunction * bf);
	void adjustVertex();
	void printParsed(std::ostream & O);
	
	
	////// ALIAS ONLY FUNCTIONS/VARIABLES  /////////
	void parseVertex_OA(ifstream & bI, BlameFunction * bf);	
	int findBlamedExits_OAR(std::set<VertexProps *> & visited, int lineNum);

	std::set<int> lineReadNumbers;
	LineReadHash readLines;
	//////////////////////////////////////////
	

};

#endif
