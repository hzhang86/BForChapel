/*
 *  SampleConverge.cpp
 *  
 *
 *  Created by Nick Rutar on 11/3/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


#include "BlameProgram.h"



BlameFunction * BlameFunction::parseBlameFunction_SC(ifstream & bI)
{
	std::string line;
	
	//----
	//BEGIN F_B_LINENUM
	getline(bI, line);
	
	// Begin Line Num
	getline(bI, line);
	int bln = atoi(line.c_str());
	setBLineNum(bln);
	
	//END F_B_LINENUM
	getline(bI, line);
	
	//----
	//BEGIN F_E_LINENUM
	getline(bI, line);
	
	// End Line Num
	getline(bI, line);
	int eln = atoi(line.c_str());
	setELineNum(eln);
	
	//END F_E_LINENUM
	getline(bI, line);
	
	//----
	
	getline(bI, line);
	bool proceed = true;
	while (proceed)
	{
		// SHOULD EITHER GET "BEGIN VAR" or "END FUNC"
		if (line.find("END FUNC") != std::string::npos)
		{
			proceed = false;
		}
		else
		{
			getline(bI, line);
		}
	}
	return this;
}





void BlameProgram::parseProgram_SC()
{



	std::vector<std::string>::iterator vec_str_i;
	for (vec_str_i = blameExportFiles.begin(); vec_str_i != blameExportFiles.end(); vec_str_i++)
	{
		ifstream bI((*vec_str_i).c_str());
		
		std::cout<<"Parsing file "<<(*vec_str_i)<<std::endl;
		
		string line;
		while(getline(bI,line))
		{
			size_t foundBeg;
			
			foundBeg = line.find("BEGIN FUNC");
			
			BlameFunction * bf;
			
			if (foundBeg != std::string::npos)
			{
				std::string funcName, moduleName, modulePathName;
				
				//std::cout<<endl<<endl;
				
				
				//BEGIN_F_NAME
				getline(bI, line);
				
				// Actual Nmae
				getline(bI, line);
				std::cout<<"Parsing Function "<<line<<std::endl;
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
				
				bf->BP = this;
				
				bf->parseBlameFunction_SC(bI);
				addFunction(bf);	

				
			}
		}
	}
}
