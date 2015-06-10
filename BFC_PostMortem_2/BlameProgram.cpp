/*
 *  Module.cpp
 *  
 *
 *  Created by Nick Rutar on 4/13/09.
 *  Copyright 2009 Nick Rutar All rights reserved.
 *
 */

#include "BlameProgram.h"
#include "BlameModule.h"
#include "BlameFunction.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>

using namespace std;


BlameModule *  BlameProgram::findOrCreateModule(const char * modName)
{
	//std::cout<<"In findOrCreateModule for "<<modName<<std::endl;
  BlameModule * bm = NULL;
	
  //bm = blameModules[modName];
	
	
  if (blameModules.count(modName) == 0)
	{
		//std::cout<<"Creating new blame module for "<<modName<<std::endl;
		bm = new BlameModule();
		std::string s(modName);
		bm->setName(s);
		blameModules[bm->getName().c_str()] = bm;
	}
	else
	{
		bm = blameModules[modName];
		//std::cout<<bm->getName()<<" already created "<<std::endl;
	}
	
  return bm;
	
}


void BlameProgram::addImplicitBlamePoint(const char * checkName)
{
	BlameFunction * bf = NULL;
	FunctionHash::iterator fh_i_check;  // blameFunctions;
	fh_i_check = blameFunctions.find(checkName);
	
	if (fh_i_check != blameFunctions.end())
	{
		 bf = blameFunctions[checkName];
		 bf->setBlamePoint(IMPLICIT);
	}
}

void BlameProgram::addImplicitBlamePoints()
{
	// TODO: Get this from config file
	addImplicitBlamePoint("HPL_pdtest");
	addImplicitBlamePoint("HPL_pdgesv");
	addImplicitBlamePoint("HPL_pdgesv0");
	addImplicitBlamePoint("callBottom");
}



void BlameProgram::grabUsedModules(const char * traceName)
{
	ifstream bI(traceName);
	std::string line;
	
	getline(bI, line);
	
	int numInstances = atoi(line.c_str());
	
	char * buffer = (char *) malloc(100*sizeof(char));
	
	for (int a = 0; a < numInstances; a++)
	{
		getline(bI, line);
		
		int numFrames = atoi(line.c_str());
		
		
		for (int b = 0; b < numFrames; b++)
		{
			int lineNum, frameNum;
			unsigned address;
			
			getline(bI, line);
			
			sscanf(line.c_str(), "%d %d %s %x", &lineNum, &frameNum, 
						 buffer, &address);
			
			if (strcmp(buffer,"NULL") != 0)
			{
				std::pair<set<std::string>::iterator,bool> ret;
				ret = sampledModules.insert(buffer);
				if (ret.second)
				{
					//std::cout<<"Inserting "<<buffer<<" into list of modules. "<<strlen(buffer)<<std::endl;
				}
			}
			//std::string moduleName(buffer);
			//i.frames.push_back(sf);
		}
	}
}

void BlameProgram::parseConfigFile_OA(const char * path)
{
  ifstream bI(path);
	
	std::string numExportFilesStr;
	std::string tmpStr;
	std::string numStructFilesStr;
	std::string numSEFilesStr;
	std::string numLoopFilesStr;
	
	getline(bI, numExportFilesStr);
	int nEF = atoi(numExportFilesStr.c_str());
	for (int a = 0; a < nEF; a++)
	{
		getline(bI, tmpStr);
		blameExportFiles.push_back(tmpStr);
	}
	
	getline(bI, numStructFilesStr);
	int nSF = atoi(numStructFilesStr.c_str());
	for (int a = 0; a < nSF; a++)
	{
		getline(bI, tmpStr);
		blameStructFiles.push_back(tmpStr);
	}
	
	getline(bI, numSEFilesStr);
	int nSEF = atoi(numSEFilesStr.c_str());
	for (int a = 0; a < nSEF; a++)
	{
		getline(bI, tmpStr);
		blameSEFiles.push_back(tmpStr);
	}
	
	getline(bI, numLoopFilesStr);
	int nLoop = atoi(numLoopFilesStr.c_str());
	for (int a = 0; a < nLoop; a++)
	{
		getline(bI, tmpStr);
		blameLoopFiles.push_back(tmpStr);
	}
	
}



void BlameProgram::parseConfigFile(const char * path)
{
  ifstream bI(path);
	
	std::string numExportFilesStr;
	std::string tmpStr;
	std::string numStructFilesStr;
	std::string numSEFilesStr;
	
	getline(bI, numExportFilesStr);
	int nEF = atoi(numExportFilesStr.c_str());
	for (int a = 0; a < nEF; a++)
	{
		getline(bI, tmpStr);
		blameExportFiles.push_back(tmpStr);
	}
	
	getline(bI, numStructFilesStr);
	int nSF = atoi(numStructFilesStr.c_str());
	for (int a = 0; a < nSF; a++)
	{
		getline(bI, tmpStr);
		blameStructFiles.push_back(tmpStr);
	}
	
	getline(bI, numSEFilesStr);
	int nSEF = atoi(numSEFilesStr.c_str());
	for (int a = 0; a < nSEF; a++)
	{
		getline(bI, tmpStr);
		blameSEFiles.push_back(tmpStr);
	}
	
}

/*
 //TODO: Obviously fix this
 bool BlameProgram::isVerbose()
 {
 std::cerr<<"IN VB"<<std::endl;
 
 if (strcmp(boolVerbose.c_str(),"VERBOSE")==0)
 {
 std::cerr<<"Verbose!!!"<<std::endl;
 return true;
 }
 else
 {
 std::cerr<<"Not Verbose!!!"<<std::endl;
 return false;
 }
 
 }
 */


void BlameProgram::addFunction(BlameFunction * bf)
{
	//std::cout<<"BF addFunction "<<bf->realName<<std::endl;

  BlameModule * bm = findOrCreateModule(bf->getModuleName().c_str());
  bm->addFunction(bf);
	bm->addFunctionSet(bf);
	
	blameFunctions[bf->getName().c_str()] = bf;
	
}

void BlameProgram::calcSideEffects()
{
	FunctionHash::iterator fh_i;
	for (fh_i = blameFunctions.begin(); fh_i != blameFunctions.end(); fh_i++)
	{
		BlameFunction * bf = (*fh_i).second;
		if (bf)
			bf->calcSideEffectDependencies();
	}
}

void BlameProgram::parseStructs()
{
	std::vector<std::string>::iterator vec_str_i;
	for (vec_str_i = blameStructFiles.begin(); vec_str_i != blameStructFiles.end(); vec_str_i++)
	{
		ifstream bI((*vec_str_i).c_str());
		
		//std::cout<<"Parsing struct file "<<(*vec_str_i)<<std::endl;
		
		string line;
		
		// BEGIN STRUCT (or END STRUCTS)
		getline(bI, line);
		
		while (line.find("BEGIN STRUCT") != std::string::npos)
		{
			StructBlame * sb = new StructBlame();
			sb->parseStruct(bI);
			blameStructs[sb->structName.c_str()] = sb;
			
			// BEGIN STRUCT (or END STRUCTS)
			getline(bI, line);
			
		}
	}
}


void BlameProgram::printStructs(std::ostream & O)
{
	StructHash::iterator vec_sb_i;
	
	//FunctionBlame * fb;
	
	for (vec_sb_i = blameStructs.begin(); vec_sb_i != blameStructs.end(); vec_sb_i++)
	{
		O<<endl<<endl;
		StructBlame * sb = (*vec_sb_i).second;
		O<<"Struct "<<sb->structName<<std::endl;
		O<<"Module Path "<<sb->modulePathName<<std::endl;
		O<<"Module Name "<<sb->moduleName<<std::endl;
		O<<"Line Number "<<sb->lineNum<<std::endl;
		
		FieldHash::iterator vec_sf_i;
		for (vec_sf_i = sb->fields.begin(); vec_sf_i != sb->fields.end(); vec_sf_i++)
		{
			StructField * sf = (*vec_sf_i).second;
			if (sf == NULL)
				continue;
			O<<"   Field # "<<sf->fieldNum<<" ";
			O<<", Name "<<sf->fieldName<<" ";
			//std::cout<<", Type "<<fb->returnTypeName(sf->llvmType, std::string(" "));
			O<<endl;
		}
	}
}



void StructBlame::parseStruct(ifstream & bI)
{
	std::string line;
	
	/////////
	//BEGIN_S_NAME
	getline(bI, line);
	
	// Actual Nmae
	getline(bI, line);
	structName = line;
	
	// END S_NAME
	getline(bI, line);
	
	
	
	
	/////////
	//BEGIN_M_PATH
	getline(bI, line);
	
	// Module Path
	getline(bI, line);
	modulePathName = line;
	
	// END M_PATH
	getline(bI, line);
	
	
	
	
	/////////
	//BEGIN_M_NAME
	getline(bI, line);
	
	// Module Nmae
	getline(bI, line);
	moduleName = line;
	
	// END M_NAME
	getline(bI, line);
	
	
	
	/////////
	//BEGIN LINENUM
	getline(bI, line);
	
	// LineNum string
	getline(bI, line);
	lineNum = atoi(line.c_str());
	
	// END LINENUM
	getline(bI, line);
	
	
	// Start of fields
	// BEGIN FIELDS
	getline(bI, line);
	
	getline(bI, line);
	// BEGIN FIELD (or END FIELDS)
	while(line.find("END FIELDS") == std::string::npos)
	{
		//std::cout<<line<<std::endl;
		// BEGIN F_NUM (or END FIELD which would be an error)
		getline(bI, line);
		if (line.find("END FIELD") != std::string::npos)
		{
			// BEGIN FIELD (or END FIELDS)
			getline(bI, line);
			continue;
		}
		
		// field num
		getline(bI, line);
		int fNum = atoi(line.c_str());
		StructField * sf = new StructField(fNum);
		fields[fNum] = sf;
		
		// END F_NUM
		getline(bI, line);
		
		// BEGIN F_NAME
		getline(bI, line);
		
		// field name
		getline(bI, sf->fieldName);
		
		// END F_NAME
		getline(bI, line);
		
		// BEGIN F_TYPE
		getline(bI, line);
		
		// field type
		getline(bI, sf->fieldType);
		
		// END F_TYPE
		getline(bI, line);

		
		
		// END FIELD
		getline(bI, line);
		
		// BEGIN FIELD (or END FIELDS)
		getline(bI, line);
	}
	
	// END STRUCT
	getline(bI, line);
	
}


void BlameFunction::calcRecursiveSEAliasesRecursive(std::set<BlameFunction *> & visited)
{
	if (seAliasCalc)
		return;
	
	//std::cout<<"Entering calcRSEAR for "<<realName<<std::endl;
	
	
	seAliasCalc = true;
	visited.insert(this);
	
	std::vector<SideEffectCall *>::iterator vec_sec_i;
	
	
	if (sideEffectCalls.size() > 0)
	{
		//std::cout<<"Calls:"<<std::endl;
		for (vec_sec_i = sideEffectCalls.begin(); vec_sec_i != sideEffectCalls.end();
				 vec_sec_i++)
		{
			SideEffectCall * sec = *vec_sec_i;
			//std::cout<<"C: "<<sec->callNode<<std::endl;
			
			size_t endChar = sec->callNode.find("--");
			if (endChar != std::string::npos)
			{
				string callTrunc = sec->callNode.substr(0, endChar);
				//BlameFunction * bf = BP->blameFunctions[callTrunc.c_str()];
				
				BlameFunction * bf = NULL;
				FunctionHash::iterator fh_i_check;  // blameFunctions;
				fh_i_check = BP->blameFunctions.find(callTrunc.c_str());
				if (fh_i_check != BP->blameFunctions.end())
				{
					bf = BP->blameFunctions[callTrunc.c_str()];
				}
				
		
				
				if (bf == NULL )
				{
					if (callTrunc.find("tmp") == std::string::npos)
					  {
					    // TODO: Look into this
					    //std::cerr<<"Call "<<callTrunc<<" not found!"<<std::endl;
						//std::cout<<"Call "<<callTrunc<<" not found!"<<std::endl;
					  }
				}
				else
				{
					if (realName == bf->getName())
					{
						//std::cout<<"RECURSIVE calcSREAR call"<<std::endl;
						continue;
					}
				
					bf->calcRecursiveSEAliasesRecursive(visited);
					
					if (bf->sideEffectAliases.size() > 0)
					{
						//std::cout<<"Aliases:"<<std::endl;
						std::vector<SideEffectAlias *>::iterator vec_sea_i;
						for (vec_sea_i = bf->sideEffectAliases.begin(); vec_sea_i != bf->sideEffectAliases.end();
								 vec_sea_i++)
						{
							SideEffectAlias * sea = *vec_sea_i;
							//std::cout<<"SideEffectAlias(REC) "<<sea<<std::endl;
							//std::cout<<"A:(1) "<<std::endl;
							std::set<SideEffectParam *>::iterator set_s_i;        // callee iterator
							std::vector<SideEffectParam *>::iterator vec_s_i_caller; // caller iterator
							std::set<SideEffectParam *>::iterator set_s_i_2;        // callee iterator stepper
							//std::set<SideEffectParam *>::iterator set_s_i_caller_2; // caller iterator stepper
							
							
							SideEffectParam * sepCaller = NULL;
							SideEffectParam * sepCaller2 = NULL;
							string localVarName("LOCAL-VAR");
							
							
							for (set_s_i = sea->aliases.begin(); set_s_i != sea->aliases.end(); set_s_i++)
							{
								SideEffectParam * sep = *set_s_i;
								//std::cout<<"A:(2) "<<sep->paramName<<std::endl;

								//int calleeParam = sep->paramNumber;
								
								
								SideEffectParam * sep2 = NULL;
								
								set_s_i_2 = set_s_i;
								set_s_i_2++;
								for (; set_s_i_2 != sea->aliases.end(); set_s_i_2++)
								{
									sep2 = *set_s_i_2;
									//std::cout<<"A:(3) "<<sep->paramName<<" "<<sep2->paramName<<std::endl;

									
									//if (sep->paramName == sep2->paramName)
										//continue;
									
									for (vec_s_i_caller = sec->params.begin(); vec_s_i_caller != sec->params.end(); vec_s_i_caller++)
									{
										//std::cout<<"A:(4) "<<(*vec_s_i_caller)->paramName<<std::endl;
										//std::cout<<"P: "<<sep->paramNumber<<" "<<sep->paramName<<std::endl;
										
										
										
										if (sep->paramNumber == (*vec_s_i_caller)->calleeNumber)
											sepCaller = (*vec_s_i_caller);
										else if (sep2->paramNumber == (*vec_s_i_caller)->calleeNumber)
											sepCaller2 = (*vec_s_i_caller);
									}
									
								}
								
								if (sep2 == NULL)
									continue;
								
								if (sepCaller == NULL)
								{
									sepCaller = new SideEffectParam();
									sepCaller->paramName = localVarName;
									sepCaller->calleeNumber = 0;
									sepCaller->paramNumber = 0;
								}
								
								if (sepCaller2 == NULL)
								{
									sepCaller2 = new SideEffectParam();
									sepCaller2->paramName = localVarName;
									sepCaller2->calleeNumber = 0;
									sepCaller2->paramNumber = 0;
								}								
								
								
								if ((sepCaller && sepCaller2) && sepCaller->paramNumber != sepCaller2->paramNumber)
								{
									size_t dot1 = sep->paramName.find_first_of('.');
									size_t dot2 = sep2->paramName.find_first_of('.');
									
									string ali1 = sepCaller->paramName;
									string ali2 = sepCaller2->paramName;
									
									if (dot1 != std::string::npos)
									{
										ali1 += sep->paramName.substr(dot1);
									}
									
									if (dot2 != std::string::npos)
									{
										ali2 += sep2->paramName.substr(dot2);
									}
									
									//std::cout<<"Alias relation between "<<ali1<<" and "<<ali2<<std::endl;
									
									SideEffectAlias * seaRes = new SideEffectAlias();
									SideEffectParam * sepRes1 = new SideEffectParam();
									SideEffectParam * sepRes2 = new SideEffectParam();
									
									sepRes1->paramNumber = sepCaller->paramNumber;
									sepRes2->paramNumber = sepCaller2->paramNumber;
									
									sepRes1->paramName = ali1;
									sepRes2->paramName = ali2;
									
									seaRes->aliases.insert(sepRes1);
									seaRes->aliases.insert(sepRes2);
									sideEffectAliases.push_back(seaRes);
									
								}
								
								
								
								//std::cout<<sep->paramNumber<<" "<<sep->paramName<<std::endl;
							}
							//std::cout<<endl;
						}
					}	
				}
			}
		}									
	}
	
	//std::cout<<"Exiting calcRSEAR for "<<realName<<std::endl;
	
	
}


void BlameProgram::calcRecursiveSEAliases()
{
	FunctionHash::iterator fh_i;  // blameFunctions;
	
	//std::cout<<"Size of blameFunctions(2) is "<<blameFunctions.size()<<std::endl;
	for (fh_i = blameFunctions.begin(); fh_i != blameFunctions.end(); fh_i++)
	{
		BlameFunction * bf = (*fh_i).second;
		if (bf)
		{	
			std::set<BlameFunction *> visited;
			visited.insert(bf);
			
			//std::cout<<"Calcing recursive side effect aliases for "<<bf->getName()<<std::endl;
			bf->calcRecursiveSEAliasesRecursive(visited);	
		}
		else
		{
			std::cerr<<"BF is NULL in calcRecursiveSEAliases"<<std::endl;
		}
	}
}

//std::vector<SideEffectAlias *>  sideEffectAliases;
//std::vector<SideEffectRelation *> sideEffectRelations;
//std::vector<SideEffectCall *> sideEffectCalls;
void BlameProgram::printSideEffects()
{
	FunctionHash::iterator fh_i;  // blameFunctions;
	
	std::cout<<"Size of blameFunctions is "<<blameFunctions.size()<<std::endl;
	for (fh_i = blameFunctions.begin(); fh_i != blameFunctions.end(); fh_i++)
	{
		BlameFunction * bf = (*fh_i).second;
		if (bf)
		{
			if (bf->sideEffectAliases.size() > 0 || bf->sideEffectRelations.size() > 0 
					|| bf->sideEffectCalls.size() > 0)
			{
				std::cout<<"Function "<<bf->realName<<" has side effects."<<std::endl;
				
				std::vector<SideEffectAlias *>::iterator vec_sea_i;
				std::vector<SideEffectRelation *>::iterator vec_ser_i;
				std::vector<SideEffectCall *>::iterator vec_sec_i;
				
				
				if (bf->sideEffectAliases.size() > 0)
				{
					std::cout<<"Aliases:"<<std::endl;
					for (vec_sea_i = bf->sideEffectAliases.begin(); vec_sea_i != bf->sideEffectAliases.end();
							 vec_sea_i++)
					{
						SideEffectAlias * sea = *vec_sea_i;
						std::cout<<"A: ";
						std::set<SideEffectParam *>::iterator set_s_i;
						for (set_s_i = sea->aliases.begin(); set_s_i != sea->aliases.end(); set_s_i++)
						{
							SideEffectParam * sep = *set_s_i;
							std::cout<<sep->paramNumber<<" "<<sep->paramName<<std::endl;
						}
						std::cout<<endl;
					}
				}
				
				
				
				
				if (bf->sideEffectRelations.size() > 0)
				{
					std::cout<<"Relations:"<<std::endl;
					for (vec_ser_i = bf->sideEffectRelations.begin(); vec_ser_i != bf->sideEffectRelations.end();
							 vec_ser_i++)
					{
						SideEffectRelation * sea = *vec_ser_i;
						std::cout<<"R: "<<sea->parent->paramName;
						std::set<SideEffectParam *>::iterator set_s_i;
						for (set_s_i = sea->children.begin(); set_s_i != sea->children.end(); set_s_i++)
						{
							SideEffectParam * sep = *set_s_i;
							std::cout<<sep->paramNumber<<" "<<sep->paramName<<std::endl;
						}
						std::cout<<endl;
					}				
				}
				
				
				
				
				
				if (bf->sideEffectCalls.size() > 0)
				{
					std::cout<<"Calls:"<<std::endl;
					for (vec_sec_i = bf->sideEffectCalls.begin(); vec_sec_i != bf->sideEffectCalls.end();
							 vec_sec_i++)
					{
						SideEffectCall * sea = *vec_sec_i;
						std::cout<<"C: "<<sea->callNode<<std::endl;
						std::vector<SideEffectParam *>::iterator set_s_i;
						for (set_s_i = sea->params.begin(); set_s_i != sea->params.end(); set_s_i++)
						{
							SideEffectParam * sep = *set_s_i;
							std::cout<<"P: "<<sep->paramNumber<<" "<<sep->paramName<<std::endl;
						}
						std::cout<<endl;
					}									
				}
				
				
			}
		}
		else
		{
			std::cout<<"BF NULL in printSideEffects"<<std::endl;
		}
	}
}

void BlameProgram::parseSideEffects()
{
	std::vector<std::string>::iterator vec_str_i;
	for (vec_str_i = blameSEFiles.begin(); vec_str_i != blameSEFiles.end(); vec_str_i++)
	{
		ifstream bI((*vec_str_i).c_str());
		
		//std::cout<<"Parsing SE file "<<(*vec_str_i)<<std::endl;
		
		char scratch1[6], scratch2[5], scratchName[100], scratchPN[200];
		char scratchMN[100];
		
		int bLine, eLine;
		
		string line;
		
		BlameFunction * bf = NULL;
		
		while(getline(bI, line))
		{
			
			if (line.find("BEGIN FUNC") != std::string::npos)
			{
				sscanf(line.c_str(), "%s %s %s %s %d %d %s",
							 scratch1, scratch2, scratchName, scratchMN, &bLine, &eLine, scratchPN);
				
				bf = new BlameFunction(scratchName);
				
				bf->BP = this;
				
				//bf->realName.insert(0, scratch3);
				bf->modulePathName.insert(0, scratchPN);
				bf->moduleName.insert(0, scratchMN);
				bf->beginLineNum = bLine;
				bf->endLineNum = eLine;
				
				//std::cout<<"RN - "<<bf->realName<<" MPN - "<<bf->modulePathName<<" MN "<<bf->moduleName;
				//std::cout<<" BLN - "<<bf->beginLineNum<<" ELN "<<bf->endLineNum<<std::endl;
				
				addFunction(bf);
			}
			else if (line.find("END FUNC") != std::string::npos)
			{
				bf = NULL;
			}
			else if (line.find("BEGIN SE_RELATIONS") != std::string::npos)
			{
				bool keepGoing = true;
				while (keepGoing)
				{
					getline(bI, line);
					if (line.find("END SE_RELATIONS") != std::string::npos)
						keepGoing = false;
					else
					{
						SideEffectRelation * se = new SideEffectRelation();
						
						int p1, p2;
						sscanf(line.c_str(), "%s %s %d %s %d", 
									 scratch1, scratchName, &p1, scratchMN, &p2);
						string s1(scratchName);
						string s2(scratchMN);
						
						SideEffectParam * parent = new SideEffectParam();
						SideEffectParam * child = new SideEffectParam();
						
						parent->paramNumber = p1;
						parent->paramName = s1;
						
						child->paramNumber = p2;
						child->paramName = s2;
						
						se->parent = parent;
						se->children.insert(child);
						
						if (bf!= NULL)
						{
							bf->sideEffectRelations.push_back(se);
							bf->hasSE = true;
						}
						else
							std::cerr<<"BF Null(1) in parseSideEffects"<<std::endl;
					}
				}
			}
			else if (line.find("BEGIN SE_ALIASES") != std::string::npos)
			{
				bool keepGoing = true;
				while (keepGoing)
				{
					getline(bI, line);
					if (line.find("END SE_ALIASES") != std::string::npos)
						keepGoing = false;
					else
					{
						SideEffectAlias * se = new SideEffectAlias();
						
						int p1, p2;
						sscanf(line.c_str(), "%s %s %d %s %d", 
									 scratch1, scratchName, &p1, scratchMN, &p2);
						string s1(scratchName);
						string s2(scratchMN);
						
						SideEffectParam * a1 = new SideEffectParam();
						SideEffectParam * a2 = new SideEffectParam();
						
				
						
						a1->paramNumber = p1;
						a1->paramName = s1;
						
						a2->paramNumber = p2;
						a2->paramName = s2;	
						
						//std::cout<<"SideEffectAlias "<<se<<std::endl;
						//std::cout<<"Adding param1 "<<a1->paramNumber<<" "<<a1->paramName<<std::endl;
						//std::cout<<"Adding param2 "<<a2->paramNumber<<" "<<a2->paramName<<std::endl;
						
						
						se->aliases.insert(a1);
						se->aliases.insert(a2);
						
						if (bf!= NULL)
						{
							//std:cout<<"Pushing back "<<se<<std::endl;
							bf->sideEffectAliases.push_back(se);
							bf->hasSE = true;
						}
						else
							std::cerr<<"BF Null(2) in parseSideEffects"<<std::endl;
						
					}
				}	
				
				
			}
			else if (line.find("BEGIN SE_CALLS") != std::string::npos)
			{
				bool keepGoing = true;
				SideEffectCall * se = NULL;
				while (keepGoing)
				{
					getline(bI, line);
					if (line.find("END SE_CALLS") != std::string::npos)
						keepGoing = false;
					else if (line.find("C:") != std::string::npos)
					{
						se = new SideEffectCall();
						sscanf(line.c_str(), "%s %s", scratch1, scratchMN);
						string s1(scratchMN);
						se->callNode = s1;
						
						if (bf!= NULL)
						{
							bf->hasSE = true;
							bf->sideEffectCalls.push_back(se);
						}
						else
							std::cerr<<"BF Null(2) in parseSideEffects"<<std::endl;
						
					}
					else if (line.find("P:") != std::string::npos)
					{
						int paramNum;
						int calleeNum;
						SideEffectParam * sep = new SideEffectParam();
						sscanf(line.c_str(), "%s %d %s %d", scratchMN, &calleeNum, scratchPN, &paramNum);
						string s1(scratchPN);
						sep->paramName = s1;
						sep->paramNumber = paramNum;
						sep->calleeNumber = calleeNum;
						
						
						if (se != NULL)
							se->params.push_back(sep);
					}
					else
					{
						std::cerr<<"Invalid SE file in parseSideEffects"<<std::endl;
					}
				}	
			}
		}
	}
	
	//std::cout<<"ALL DONE!"<<std::endl;
	
}

BlameFunction * BlameProgram::getOrCreateBlameFunction(std::string funcName, std::string moduleName,
																											 std::string modulePathName)
{
	BlameFunction * bf = NULL;
	//bf = blameFunctions[funcName.c_str()];
	FunctionHash::iterator fh_i_check;  // blameFunctions;
	fh_i_check = blameFunctions.find(funcName.c_str());
	if (fh_i_check != blameFunctions.end())
	{
		bf = blameFunctions[funcName.c_str()];
	}
	
	
	if (bf != NULL)
	{
		// Do a sanity check to make sure the modules are the same
		if (bf->moduleName == moduleName)
		{
			return bf;
		}
		else
		{
			std::cerr<<"Diff names for func "<<funcName<<" M1: "<<bf->getModuleName();
			std::cerr<<" M2: "<<moduleName<<std::endl;
			return bf;
		}
	}
	
	bf = new BlameFunction(funcName);
	bf->moduleName = moduleName;
	bf->modulePathName = modulePathName;
	
	return bf;
}

void BlameProgram::parseProgram()
{
	std::vector<std::string>::iterator vec_str_i;
	for (vec_str_i = blameExportFiles.begin(); vec_str_i != blameExportFiles.end(); vec_str_i++)
	{
		ifstream bI((*vec_str_i).c_str());
//////////////////////////////////////////////////////////////////////////		
//		std::cout<<"Parsing file "<<(*vec_str_i)<<std::endl;
//////////////////////////////////////////////////////////////////////////		
		string line;
		int count=0; // keep record of the number of iterations in while loop
		while(getline(bI,line))
		{
			size_t foundBeg;
//////////////////////////////////////////////////////
//			count ++;
/////////////////////////////////////////////////////
			
			foundBeg = line.find("BEGIN FUNC"); //The position of the first character of the first match.If no matches were found, the function returns string::npos.
			
			BlameFunction * bf;
			
			if (foundBeg != std::string::npos) //static constant, max size_t
			{
				std::string funcName, moduleName, modulePathName;
				
				//std::cout<<endl<<endl;
				//BEGIN_F_NAME
				getline(bI, line);
				
				// Actual Nmae
				getline(bI, line);
				//std::cout<<"Parsing Function "<<line<<std::endl;
				funcName = line;
				
				// END F_NAME
				getline(bI, line);
				
				
				//----
				// BEGIN M_NAME_PATH
				getline(bI, line);
				
				//std::cout<<"Should be M_NAME_PATH -- is -- "<<line<<std::endl;
				
				// Get Module Path Name()
				getline(bI, line);
				//setModulePathName(line);
				modulePathName = line;
				
				// END M_NAME_PATH
				getline(bI, line);
				
				//----
				// BEGIN M_NAME
				getline(bI, line);
				
				// Get Module Name
				getline(bI, line);
				//setModuleName(line);
				moduleName = line;
				
				// END M_NAME
				getline(bI, line);
				
				bf = getOrCreateBlameFunction(funcName, moduleName, modulePathName);
				//bf = new BlameFunction(line);
				
				bf->BP = this;
				
				bool isMain = false;	
				
				// TODO: Make this determined at runtime
				if (funcName.find("main") != std::string::npos 
						&& funcName.length() == 4)
				{
					//std::cout<<"Setting main as a blame point"<<std::endl;
					isMain = true;
				}
				
				
				// This will not return NULL if the function in question is one
				//  of the sampled functions from the run
				if (bf->parseBlameFunction(bI) != NULL)
				{
					// TODO: Optimize this
					bf->calcAggregateLN();
					bf->isFull = true;
					
					//addFunction(bf);	
					
					if (isMain)
						bf->setBlamePoint(EXPLICIT);
				}
			}
		}
	///////////////////////////////////
//	cout<<"There are "<<count<<" loops in while"<<endl;
	//////////////////////////////////
	}
}

void BlameProgram::printParsed(std::ostream &O)
{
  //cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<std::endl;
  //cout<<"For module "<<realName<<std::endl<<std::endl;
  //std::vector<BlameModule *>::iterator bm_i;
  
  std::hash_map<const char *, BlameModule *, std::hash<const char *>, eqstr>::iterator bm_i;
	
  for (bm_i = blameModules.begin();  bm_i != blameModules.end(); bm_i++)
	{
		BlameModule * bm = (*bm_i).second;
		if (bm)
			bm->printParsed(O);
		else
		{
			O<<"NULL Blame Module -- "<< (*bm_i).first<<std::endl;
		}
	}	
}
