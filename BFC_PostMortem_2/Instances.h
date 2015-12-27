#ifndef INSTANCES_H
#define INSTANCES_H 


#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <string.h>

#include <set>

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "BlameStruct.h"

namespace std
{
  using namespace __gnu_cxx;
}

class BlameModule;
//struct StructField;
//struct StructBlame;

struct eqstr
{
  bool operator() (const char * s1, const char * s2) const
  {
		return strcmp(s1,s2) == 0;
  }
};

typedef std::hash_map<const char *, BlameModule *, std::hash<const char *>, eqstr> ModuleHash;
typedef std::hash_map<const char *, StructBlame *, std::hash<const char *>, eqstr> StructHash;
typedef std::hash_map<int, StructField*> FieldHash;

struct StackFrame
{
  int lineNumber;
  int frameNumber;
  std::string moduleName;
  unsigned address;
  bool toRemove;//Added by Hui 12/20/15
};

struct Instance
{
  std::vector<StackFrame> frames;
  int processTLNum; //added by Hui 12/25/15: processTaskList called#
  bool isMainThread; //added by Hui 12/25/15: this instance is from the main thread
  void printInstance();
  void printInstance_concise();//Added by Hui 12/20/15
  void handleInstance(ModuleHash & modules, std::ostream &O, bool verbose);
  void trimFrames(ModuleHash &modules, int InstanceNum, int whichStack); //Added by Hui 12/20/15
  void secondTrim(ModuleHash &modules); //added by Hui 12/25/15

  void handleInstance_OA(ModuleHash & modules, std::ostream &O, std::ostream &O2, bool verbose, int InstanceNum);
};


struct FullSample
{
	set<int> instanceNumbers;  // keep track of all the matching ones so we can compare against 
	/// which set to merge using a nearest neighbor style approach
	Instance * i;  // representative instance, we only record one representative one
	int frameNumber;
	int lineNumber;
};

typedef std::hash_map<int, Instance> InstanceHash;//added by Hui 12/25/15
                                            //NOTE: pair.second is NOT a pointer !

#endif
