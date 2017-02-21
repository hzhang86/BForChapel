#ifndef INSTANCES_H
#define INSTANCES_H 


#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <string.h>

#include <set>
/*
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

namespace std
{
  using namespace __gnu_cxx;
}
*/

#include "BlameStruct.h"
#include <unordered_map>
#define COMPUTE_INST    0
#define PRESPAWN_INST   1
#define FORK_INST       2
#define FORK_NB_INST    3
#define FORK_FAST_INST  4

class BlameModule;

extern std::ofstream stack_info;

struct eqstr
{
  bool operator() (std::string s1, std::string s2) const {
	return s1 == s2;
  }

};

typedef std::unordered_map<std::string, BlameModule *, std::hash<std::string>, eqstr> ModuleHash;
typedef std::unordered_map<std::string, StructBlame *, std::hash<std::string>, eqstr> StructHash;
typedef std::unordered_map<int, StructField*> FieldHash;

struct fork_t 
{
  int callerNode;
  int calleeNode;
  int fid;
  int fork_num;

  fork_t() : callerNode(-1),calleeNode(-1),fid(-1),fork_num(-1) {}
  fork_t(int loc, int rem, int fID, int f_num) : 
    callerNode(loc),calleeNode(rem),fid(fID),fork_num(f_num) {}

};

struct StackFrame
{
  int frameNumber;
  int lineNumber;
  std::string moduleName;
  unsigned long address;
  bool toRemove = false;
  std::string frameName;
  unsigned long task_id = 0;
};

struct Instance
{
  std::vector<StackFrame> frames;
  int instType = -1;//which file it came from, -1 is invalid
  unsigned long taskID;
  bool isMainThread = false; //whether this instance is from the main thread

  void printInstance();
  void printInstance_concise();
  void handleInstance(ModuleHash & modules, std::ostream &O, int InstanceNum, bool verbose);
  void trimFrames(ModuleHash &modules, int InstanceNum, std::string nodeName); 
  void removeRedundantFrames(ModuleHash &modules, std::string nodeName);
  //void secondTrim(ModuleHash &modules, std::string nodeName); 

  void handleInstance_OA(ModuleHash & modules, std::ostream &O, std::ostream &O2, 
         bool verbose, int InstanceNum);
};


struct FullSample
{
	set<int> instanceNumbers;  // keep track of all the matching ones so we can compare against 
	/// which set to merge using a nearest neighbor style approach
	Instance * i;  // representative instance, we only record one representative one
	int frameNumber;
	int lineNumber;
};


typedef std::unordered_map<int, Instance> InstanceHash;//added by Hui 12/25/15
                                            //NOTE: pair.second is NOT a pointer !

typedef std::unordered_map<std::string, std::vector<Instance>, std::hash<std::string>, eqstr> compInstanceHash;
typedef std::unordered_map<std::string, InstanceHash, std::hash<std::string>, eqstr> preInstanceHash;
typedef std::unordered_map<std::string, std::vector<Instance>, std::hash<std::string>, eqstr> forkInstanceHash; //NOTE: the value type is reference !

//we keep a map between the chpl_nodeID and real compute node names
typedef std::unordered_map<int, std::string> nodeHash;


#endif
