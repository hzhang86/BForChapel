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

struct ExternFunctionBlame {
    
    std::string funcName;
    std::set<int> paramNums;
    
    ExternFunctionBlame(std::string s) {
        funcName = s;
    }
};

typedef std::hash_map<const char*, ExternFunctionBlame *, std::hash<const char*>, eqstr> ExternFuncBlameHash;


