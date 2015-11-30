#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <dirent.h> // for interating the directory

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

void populateSamples(vector<Instance> &instances, char *exeName, const char *traceName)
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
//   int num_of_instances = 0;
 ///////////////////////////
  while(!filestr.eof())
  {
    filestr.getline(linebuffer,2000);
    string str(linebuffer);
    // argument is changed from 15 to 18 for pygmy: 48 bits address
    //stackSize = str.length()/18;
    // argument is changed from 18 to 22 because we now set addr length to be 16 bits
    stackSize = str.length()/22;

//    std::cout<<"Size of line is "<<str.length()<<" stack size is "<<stackSize<<std::endl;
    stringstream ss(str); // Insert the string into a stream
    string buf;
    
    //////////////////See if ss has the right info ///////////////////////////
//    string ss_cp = ss.str();
//    std::cout<<"The content of ss is: "<<ss_cp<<std::endl;
    /*
    string buf_test;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    ss>>buf_test;
    std::cout<<"buf_test= "<<buf_test<<std::endl;
    
    ss.str("");
    ss.str(str); // Insert the string into a stream
    cout<<"ss is back to "<<ss.str()<<endl;
    */
    //////////////////////////////////////////////////////////////////////////////
    
    if (lineType == START_LINE)
    {
      ss>>buf;
      //ss>>stackSize;
//      std::cout<<"buf= "<<buf<<std::endl;
      ss>>buf;

      lineType = ADD_LINE;
      ////////////////////////////////
//      std::cout<<"I'm in START_LINE now"<<std::endl;
//      std::cout<<"buf= "<<buf<<std::endl;
    }
    else if (lineType == ADD_LINE)
    {
      int stackFrame;
      //Offset address;
      // changed address from unsigned to unsigned long
      unsigned long address;
      ////////////////////////////////////////////////////
//      std::cout<<"initially, address= "<<address<<std::endl;

      if (stackSize <= 0)
      {
        lineType = END_LINE;
        cerr<<"Null Stack Size"<<endl;
        continue;
      }
      
      Instance inst;
      string emptyStr("NoFuncFound");
      
      for (int a = 0; a < stackSize; a++)  // shouldn't be "a<=stackSize" ?
      {
        StackFrame sf;
          
        ss>>stackFrame;
        /////////////////////////////
//        printf("I'm in the for loop, a=%d , stackFrame=%d\n", a, stackFrame);
        
        if (stackFrame != a) // ?? a should start from 0 to stackSize ??
        {
          printf("Missing a stack frame %d %d\n", stackFrame, a);
          break;;  // break out the for loop, directly go to the next iteration of while loop
        }
        ss>>std::hex>>address;
        ////////////////////////////////////////////////////
//        std::cout<<std::hex<<"Now, address= "<<address<<std::endl;
//        std::cout<<std::dec; // change output format back to decimal 
        
        ss>>std::dec;
          
        lineType = END_LINE;  //it can be moved to outside the for loop
        
        short found = 0;
        
        if ( address < 0x1000 )
          found = 1;
        
        if (stackFrame != 0)
          address = address - 1; //sampled IP should points to the last instruction 
        
        BPatch_Vector<BPatch_statement> sLines;
        
        proc->getSourceLines(address,sLines); // it can get the information associated with the address, the vector sLines contain pairs of filenames(source file names) and line numbers that are associated with address
        
        
        if (sLines.size() > 0)
        {
//        cout<<"Line Number "<<sLines[0].lineNumber()<<" in file ";
//        cout<<sLines[0].fileName()<<" stack pos "<<a<<std::endl;
          for (int i=0; i<sLines.size(); i++){
            cout<<"Line Number "<<sLines[i].lineNumber()<<" in file ";
            cout<<sLines[i].fileName()<<" stack pos "<<a<<std::endl;
          }

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
//        printf("Pushed a stack frame to this instance\n");
      }   // end of for loop
      
      instances.push_back(inst);

    ////////////////////////////
//      num_of_instances++;
    }  // end of lineTyep=ADD_LINE
    else if (lineType == END_LINE)
    {
      lineType = START_LINE;   ////?? no need to buffer out : ss>>buf ??
            // I think it should be dumped as well, just like STARTLINE
    }
    else 
      printf("Error, shouldn't be here\n");
   
    // IMPORTANT ?!? //
    //ss.clear();
  }// while loop
  ////////////////////////////////////////////////////////////  
//  printf("The number of instances is %d \n", num_of_instances);
  filestr.close();
}


void popSamplesFromDir(vector<Instance> &instances, char *exeName, const char *dirName, const char *nodeName)
{
    DIR *dir;
    struct dirent *ent;
    std::string traceName;

    if ((dir = opendir(dirName)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if((ent->d_type == DT_REG) && (strstr(ent->d_name, nodeName) != NULL)){
                traceName = std::string(dirName) + "/" + std::string(ent->d_name);
                populateSamples(instances, exeName, traceName.c_str());    
            }
            else 
                std::cerr<<""<<std::endl;
        }
        closedir(dir);
    } 

    else 
        /* could not open directory */
        std::cerr<<"Couldn't open the directory !"<<std::endl;
}


int main(int argc, char** argv)
{ 
  if (argc != 3)
  {
    std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
    std::cerr<<"Usage: <this.exe>  <target app>  <Directory containing node files to be transformed>"<<std::endl; 
    exit(0);
  }
 
  char buffer[128];
  gethostname(buffer,255);
  std::string whichNode(buffer);  // whichNode = pygmy
  std::string guiOut("Input_");    
  guiOut += whichNode;       // guiOut = Input_pygmy  
  std::ofstream gOut(guiOut.c_str());
  
  vector<Instance>  instances;  
  popSamplesFromDir(instances, argv[1], argv[2], buffer);
  
  int size = instances.size();
  gOut<<size<<endl;   // output the number of instances in Input_pygmy
    
  vector<Instance>::iterator vec_I_i;
  for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++) {
      int frameSize = (*vec_I_i).frames.size();
      gOut<<frameSize<<endl;
      vector<StackFrame>::iterator vec_sf_i;
      for (vec_sf_i = (*vec_I_i).frames.begin(); \
              vec_sf_i != (*vec_I_i).frames.end(); vec_sf_i++)
      {
              // Is always the address of the unwind sampler
        if ((*vec_sf_i).frameNumber == 0)//unnecessary, use lineNumber enough
        {
          gOut<<"0 "<<(*vec_sf_i).frameNumber<<" NULL "<<std::hex \
              <<(*vec_sf_i).address<<std::dec<<endl;
        }
        else if ((*vec_sf_i).lineNumber > 0)
        {
          gOut<<(*vec_sf_i).lineNumber<<" "<<(*vec_sf_i).frameNumber<<" ";
          gOut<<(*vec_sf_i).moduleName<<" "<<std::hex \
              <<(*vec_sf_i).address<<std::dec<<endl;;
        }
        else
        {
          gOut<<"0 "<<(*vec_sf_i).frameNumber<<" NULL "<<std::hex<<(*vec_sf_i).address<<std::dec<<endl;
        }
      }
  }
}

