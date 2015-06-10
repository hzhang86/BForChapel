#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>

#include "BlameProgram.h"
#include "BlameFunction.h"
#include "BlameModule.h"
#include "Instances.h"


#include "BPatch.h"
#include "BPatch_process.h"
#include "BPatch_Vector.h"
#include "BPatch_statement.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_point.h"
#include "BPatch_function.h"

#define START_LINE 0
#define ADD_LINE   1
#define END_LINE   2


using namespace std;
using namespace Dyninst;

//  foo     xhpl     bug00.umiacs.umd.edu
//argv[0]  argv[1]    argv[2]


void populateSamples(vector<Instance> & instances, char * exeName, const char * traceName)
{
  
  BPatch bpatch;
  
  BPatch_process* proc = NULL;
  //BPatch_image* appImage = NULL;
  //BPatch_Vector<BPatch_module*>* appModules = NULL;
  
  if(!(proc = bpatch.processCreate(exeName,NULL  )))
	{
		cerr << "error in bpatch.createProcess" << endl;
		exit(-1);
	}
  
  
	char fileName[30];
  char linebuffer[2000];
  
  fstream filestr(traceName, fstream::in);
  
   
  short lineType = START_LINE;
  //short stackSize = 0;
	short stackSize = 1;
  short stackSizeSC = 0;
  //for testing ///////
   int num_of_instances = 0;
 ///////////////////////////
  while(!filestr.eof())
	{
		filestr.getline(linebuffer,2000);
		string str(linebuffer);


		
		stackSize = str.length()/15;

		std::cout<<"Size of line is "<<str.length()<<" stack size is "<<stackSize<<std::endl;
		stringstream ss(str); // Insert the string into a stream
		string buf;
		
		if (lineType == START_LINE)
		{
			ss>>buf;
			//ss>>stackSize;
			ss>>buf;
			lineType = ADD_LINE;
		}
		else if (lineType == ADD_LINE)
		{
			int stackFrame;
			//Offset address;
			unsigned address;

			if (stackSize <= 0)
	    {
	      lineType = END_LINE;
	      cerr<<"Null Stack Size"<<endl;
	      continue;
	    }
			
			Instance inst;
			string emptyStr("NoFuncFound");
			
			for (int a = 0; a < stackSize; a++)
	    {
	      StackFrame sf;
		/////////////////////////////
		printf("I'm in the for loop \n");
				
	      ss>>stackFrame;
	      if (stackFrame != a)
				{
					printf("Missing a stack frame %d %d\n", stackFrame, a);
					break;;
				}
	      ss>>std::hex>>address;
	      ss>>std::dec;
				
	      lineType = END_LINE;
	      
	      short found = 0;
	      
	      if ( address < 0x1000 )
					found = 1;
	      

				if (stackFrame != 0)
					address = address - 1;
	      
	      BPatch_Vector<BPatch_statement> sLines;
	      
	      proc->getSourceLines(address,sLines);
	      
	      
	      if (sLines.size() > 0)
				{
					//cout<<"Line Number "<<sLines[0].lineNumber()<<" in file ";
					//cout<<sLines[0].fileName()<<" stack pos "<<a<<std::endl;
					sf.lineNumber = sLines[0].lineNumber();
					string fileN(sLines[0].fileName());
					sf.moduleName = fileN;
					sf.frameNumber = a;
					sf.address = address;
				}
	      else
				{
					sf.lineNumber = -1;
					sf.moduleName = emptyStr;
					sf.frameNumber = a;
					sf.address = address;
				}
	      inst.frames.push_back(sf);
		/////////////////////////////////////
		printf("Pushed a stack frame to this instance\n");
	    } 	// end of for loop
			instances.push_back(inst);

		////////////////////////////
			num_of_instances++;
		}
		else if (lineType == END_LINE)
		{
			lineType = START_LINE;
		}
		else 
			printf("Error, shouldn't be here\n");
	   
	}
  ////////////////////////////////////////////////////////////	
  printf("The number of instances is %d \n", num_of_instances);
  filestr.close();
}




int main(int argc, char** argv)
{ 
	if (argc != 3)
	{
		std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
		std::cerr<<"Usage: <this.exe>  <target app>  <node file to be transformed>"<<std::endl; 
		exit(0);
	}
 
	//  BlameProgram bp;
  //  bp.parseConfigFile(argv[3]);

	//	bp.parseStructs();
	//  bp.printStructs(std::cout);

	
  //  bp.parseProgram();
	
	
	//bool verbose = bp.isVerbose();
	//  bool verbose = false;
	
  std::string whichNode(argv[2]);
  std::string guiOut("Input_");
  guiOut += whichNode;
  std::ofstream gOut(guiOut.c_str());
  
  vector<Instance>  instances;	
  populateSamples(instances, argv[1], argv[2]);

	int size = instances.size();
	
	gOut<<size<<endl;
		
  vector<Instance>::iterator vec_I_i;
  for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++)
		{
			int frameSize = (*vec_I_i).frames.size();
			gOut<<frameSize<<endl;
			vector<StackFrame>::iterator vec_sf_i;
			for (vec_sf_i = (*vec_I_i).frames.begin();  vec_sf_i != (*vec_I_i).frames.end(); vec_sf_i++)
				{
					// Is always the address of the unwind sampler
					if ( (*vec_sf_i).frameNumber == 0)
						{
							gOut<<"0 "<<(*vec_sf_i).frameNumber<<" NULL "<<std::hex<<(*vec_sf_i).address<<std::dec<<endl;
						}
					else if ( (*vec_sf_i).lineNumber > 0)
						{
						gOut<<(*vec_sf_i).lineNumber<<" "<<(*vec_sf_i).frameNumber<<" ";
						gOut<<(*vec_sf_i).moduleName<<" "<<std::hex<<(*vec_sf_i).address<<std::dec<<endl;;
						}
					else
						{
							gOut<<"0 "<<(*vec_sf_i).frameNumber<<" NULL "<<std::hex<<(*vec_sf_i).address<<std::dec<<endl;
						}
			}
	}
}


/*
 g++ -g -I/fs/driver/rutar/dyninst/dyninst -I/fs/driver/rutar/dyninst/dyninst/external -I/fs/driver/rutar/dyninst/include -W -Wall -L/fs/driver/rutar/dyninst/i386-unknown-linux2.4/lib -lcommon -ldl -lsymtabAPI -linstructionAPI -liberty -o foo foo.C
 */
