/*
 *  This is the much simplified version of Instance.h
 *  compared to the one in BFC_Postmortem_2, since all we
 *  care about here is the stack frames info in each Instance.
 *  
 *   Created by Hui 08/07/16
 */

#ifndef INSTANCES_H
#define INSTANCES_H 

#include <vector>
//#include <map>
#include <string>
#include <iostream>
//#include <set>
/*
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
*/
//#include "BlameStruct.h"

//#define FILE_SIZE 1500
/*
namespace std
{
  using namespace __gnu_cxx;
}
*/
//class BlameModule;
//struct StructField;
//struct StructBlame;
/*
struct eqstr
{
  bool operator() (const char * s1, const char * s2) const
  {
		return strcmp(s1,s2) == 0;
  }
};

typedef std::hash_map<const char *, BlameModule *, std::hash<const char *>, eqstr> ModuleHash;
typedef std::hash_map<const char *, StructBlame *, std::hash<const char *>, eqstr> StructHash;
typedef std::hash_map<int, StructField *> FieldHash;
*/



struct StackFrame
{
  int lineNumber;
  int frameNumber;
  std::string moduleName;
  unsigned address;
};

struct Instance
{
  std::vector<StackFrame> frames;
  int processTLNum; //Added by Hui 12/25/15: processTaskList Number
//  void printInstance();
//  void handleInstance(ModuleHash & modules, std::ostream &O, bool verbose);
//  void handleInstance_OA(ModuleHash & modules, std::ostream &O, std::ostream &O2, bool verbose,int * histogram, bool skid);

};

/*
struct FullSample
{
	set<int> instanceNumbers;  // keep track of all the matching ones so we can compare against 
	/// which set to merge using a nearest neighbor style approach
	Instance * i;  // representative instance, we only record one representative one
	int frameNumber;
	int lineNumber;
};
*/
#endif
