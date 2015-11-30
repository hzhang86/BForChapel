/*
 *  BlameModule.h
 *  
 *
 *  Created by Nick Rutar on 4/13/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef BLAME_MODULE_H
#define BLAME_MODULE_H 

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>

#include "Instances.h"
#include "VertexProps.h"
#include "BlameFunction.h"

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

using namespace std;

class BlameFunction;



typedef std::hash_map<const char *, BlameFunction *, std::hash<const char *>, eqstr> FunctionHash;


class BlameModule
{
 public:
  
  std::string getName() { return realName;}
  void printParsed(std::ostream &O);
  BlameFunction * findLineRange(int lineNum);
  void setName(std::string name) { realName = name;}
  void addFunction(BlameFunction * bf){blameFunctions[bf->getName().c_str()] = bf;}
	
 private:
  std::string realName;
  FunctionHash blameFunctions;
  
};

#endif
