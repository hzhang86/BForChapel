/*
 *  FunctionBFC.h
 *  Class definition for a function in the source program
 *  Main member function: firstPass
 *
 *  Created by Hui Zhang on 03/18/15
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "NodeProps.h"
#include "ModuleBFC.h"

namespace std
{
    using namespace __gnu_cxx;
}

using namespace llvm;


struct eqstr {
    
    bool operator()(const char *s1, const char *s2) const {
        return strcmp(s1, s2) == 0;
    }
};

struct ExternFunctionBFC {
    
    std::string funcName;
    std::set<int> paramNums;
    
    ExternFunctionBFC(std::string s) {
        funcName = s;
    }
};

typedef std::hash_map<const char*, ExternFunctionBFC *, std::hash<const char*>, eqstr> ExternFuncBFCHash;



///////////////////////// Generic Public Calls ///////////////////////////////
public:
    void firstPass(Function *F, std::vector<NodeProps *> &globalVars,
            ExternFuncBFCHash &efInfo, std::ostream &blame_file,
            std::ostream &blame_se_file, std::ostream &call_file, int &numMissing);
