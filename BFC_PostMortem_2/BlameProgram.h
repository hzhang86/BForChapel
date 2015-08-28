/*
 *  BlameModule.h
 *  
 *
 *  Created by Nick Rutar on 4/13/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef BLAME_PROGRAM_H
#define BLAME_PROGRAM_H 

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <set>

#include "Instances.h"

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "BlameModule.h"
#include "BlameStruct.h"

////added by Hui///////
#define HUI_CHPL
//////////////////////

using namespace std;


class BlameFunction;
class BlameModule;


class BlameProgram
{
 public:  
  void parseProgram();
	
	
	// FOR Sample Converger
	void parseProgram_SC();
	////////
	
	void parseSideEffects();
	void printSideEffects();
	
	
	void parseStructs();
 // bool isVerbose();
  void parseConfigFile(const char * path);
	
	
	void grabUsedModules(const char * traceName);

  void addFunction(BlameFunction * bf);
  
	void printParsed(std::ostream &O);
	void printStructs(std::ostream &O);
	
	void addImplicitBlamePoints();
	void addImplicitBlamePoint(const char * checkName);

	
	void calcRecursiveSEAliases();


	
  BlameModule * findOrCreateModule(const char *);
	BlameFunction * getOrCreateBlameFunction(std::string funcName, std::string moduleName,std::string modulePathName);
	
	// goes through all the functions and calculates the side
	//  effects for all of them
	void calcSideEffects();
	StructHash blameStructs;
  ModuleHash blameModules;
	FunctionHash blameFunctions;
	std::set<std::string> sampledModules;
	
	
	////////// ALIAS ONLY ///////////
	void parseProgram_OA();
	void parseConfigFile_OA(const char * path);
	void parseLoops_OA();

	///////////////////////////////////

  
 private:
	std::vector<std::string> blameExportFiles;
	std::vector<std::string> blameStructFiles;
	std::vector<std::string> blameSEFiles;
	std::vector<std::string> blameLoopFiles;
  //std::string boolVerbose;  
};

#endif
