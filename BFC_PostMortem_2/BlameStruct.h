/*
 *  BlameStruct.h
 *  
 *
 *  Created by Nick Rutar on 3/29/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
 

#ifndef BLAME_STRUCT_H
#define BLAME_STRUCT_H
 
 
#include <string>

#include "Instances.h"

#include <vector>
#include <map>
#include <string>
#include <iostream>

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

using namespace std;



struct StructBlame; 
 
struct StructField {

	string fieldName;
	int fieldNum;
	string fieldType;
	//const llvm::Type * llvmType;

	StructBlame * parentStruct;

	StructField() {}
	StructField(int fn){fieldNum = fn;}

};

typedef std::hash_map<int, StructField *> FieldHash;


struct StructBlame {

	string structName;
	//vector<StructField *> fields;
	FieldHash fields;
	
	
	string moduleName;
	string modulePathName;
	
	int lineNum;
	
	void parseStruct(ifstream & bI);
	
	//void grabModuleNameAndPath(llvm::Value * compUnit);
	//void getPathOrName(Value * v, bool isName);

	
};

#endif

