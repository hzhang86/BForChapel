#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <dirent.h> // for interating the directory
/*
#include "BlameProgram.h"
#include "BlameFunction.h"
#include "BlameModule.h"
*/
#include "Instances.h"

#include "BPatch.h"
#include "BPatch_process.h"
#include "BPatch_Vector.h"
#include "BPatch_statement.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
/*
#define START_LINE 0
#define ADD_LINE   1
#define END_LINE   2
*/
#define SEP_TAGS
using namespace std;
using namespace Dyninst;

//addParser lulesh    ./SSFs    preStackTrace       
//argv[0]  argv[1]    argv[2]   argv[3]

void populateFrames(Instance &inst, fstream &filestr, BPatch_process* proc)
{    
  char linebuffer[2000];
  filestr.getline(linebuffer,2000);
  string str(linebuffer);
  stringstream ss(str); // Insert the string into a stream
  //string buf;

  if (str.find("START")!=string::npos || str.find("END")!=string::npos) {
    cerr<<"Shouldn't be here ! Not ADD_LINE! it's "<<str<<endl;
    return;
  }
  else {
    int frameNum;
    unsigned long address;  //16 bits in hex
    int stackSize = str.length()/22;//15 for 32-bit, 18 for 48-bit, 22 for 64-bit system
    if (stackSize <= 0) {
      cerr<<"Null Stack Size"<<endl;
      return;
    }
    string emptyStr("NoFuncFound");
  
    for (int a = 0; a < stackSize; a++) {
      StackFrame sf;
      ss>>frameNum;
      if (frameNum != a) {
        printf("Missing a stack frame %d %d\n", frameNum, a);
        break;;  // break out the for loop, directly go to the next iteration of while loop
      }
      ss>>std::hex>>address;
      ss>>std::dec;
    
      short found = 0; // don't know what's it for
      if ( address < 0x1000 )
        found = 1;
    
      if (frameNum != 0)
        address = address - 1; //sampled IP should points to the last instruction 
      
      BPatch_Vector<BPatch_statement> sLines;
      proc->getSourceLines(address,sLines); // it can get the information associated with the 
                                    //address, the vector sLines contain pairs of filenames
                        //(source file names) and line numbers that are associated with address
    
      if (sLines.size() > 0) {
        //for test
        /*for (int i=0; i<sLines.size(); i++){
          cout<<"Line Number "<<sLines[i].lineNumber()<<" in file ";
          cout<<sLines[i].fileName()<<" stack pos "<<a<<std::endl;
        }*/

        sf.lineNumber = sLines[0].lineNumber();
        string fileN(sLines[0].fileName());
        sf.moduleName = fileN;
        sf.frameNumber = a;
        sf.address = address;
      }
      else {
        sf.lineNumber = -1;
        sf.moduleName = emptyStr;
        sf.frameNumber = a;
        sf.address = address;
      }
      inst.frames.push_back(sf);
    }// end of for loop
  }
}

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

  while(!filestr.eof()) {
    filestr.getline(linebuffer,2000);
    string str(linebuffer);
    stringstream ss(str); // Insert the string into a stream
    string buf;
   
    if (str.find("<----START") != string::npos) {
      ss>>buf; //buf = "<----START"
      ss>>buf; //buf = name of the raw stackTrace file
      /////Added by Hui 12/23/15: to get the processTLNum///////
      int processTLNum;
      ss>>processTLNum;
      Instance inst;
      inst.processTLNum = processTLNum;

      populateFrames(inst, filestr, proc); 

      filestr.getline(linebuffer,2000);//---->END
      string str2(linebuffer);
      if (str2.find("---->END") != string::npos) {
        instances.push_back(inst);
      }
    }
    else 
      cout<<"Done parsing a stack trace file"<<endl;
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
  if (argc < 3) //changed by Hui 12/23/15: it should be 4 for multi-thread code
  {
    std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
    std::cerr<<"Usage: <this.exe>  <target app>  <Directory containing node files to be transformed>"<<std::endl; 
    exit(0);
  }
 
  char buffer[128];
  gethostname(buffer,127);
  std::string whichNode(buffer);  // whichNode = pygmy
  std::string guiOut("Input_");    
  guiOut += whichNode;       // guiOut = Input_pygmy  
  std::ofstream gOut(guiOut.c_str());
  
#ifdef SEP_TAGS
  //we need to match all processTLNums to all the samples
  char linebuffer[2000];
  std::ifstream rawStackTrace(buffer);
  std::ifstream allTags("allTags");
  string dir(argv[2]);
  dir += "/";
  dir += whichNode;
  std::ofstream cookedStackTrace(dir.c_str(), std::ofstream::app);
  string tag;
  int num1=0, num2=0;
  while(!rawStackTrace.eof()) {
    rawStackTrace.getline(linebuffer, 2000);
    string strhui(linebuffer);
    if (strhui.find("<----START") != string::npos) { 
      allTags >> tag;
      cookedStackTrace<<strhui<<" "<<whichNode<<" "<<tag<<endl;
      num2++;
    }
    else
      cookedStackTrace<<strhui<<endl;
  }
  cout<<"\n----#raw samples = "<<num2<<endl;

  allTags.close();
  rawStackTrace.close();
  cookedStackTrace.close();
#endif

  vector<Instance>  instances;  
  popSamplesFromDir(instances, argv[1], argv[2], buffer);

  ////Added by Hui: 12/23/15 addParse the preStackTrace file////////
  vector<Instance> pre_instances;
  std::string pstOut("Input_");
  std::ofstream pOut;
  if(argc >3){
    std::string pstFile(argv[3]);
    pstOut += pstFile;
    pOut.open(pstOut.c_str());
    populateSamples(pre_instances, argv[1], argv[3]);
  }
  else 
    pOut.open("NoneSense");
  /////////////////////////////////////////////////////////////////

  int size = instances.size();
  gOut<<size<<endl;   // output the number of instances in Input_pygmy
    
  vector<Instance>::iterator vec_I_i;
  for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++) {
      int frameSize = (*vec_I_i).frames.size();
      gOut<<frameSize<<" "<<(*vec_I_i).processTLNum<<endl;//added processTLNum by Hui 12/25/15
      vector<StackFrame>::iterator vec_sf_i;
      for (vec_sf_i = (*vec_I_i).frames.begin(); \
              vec_sf_i != (*vec_I_i).frames.end(); vec_sf_i++) {

        // In libunwind, it is always the address of the unwind sampler, but now we use 3rd-party sw, it's the source code
        /*if ((*vec_sf_i).frameNumber == 0)//unnecessary, use lineNumber enough
        {
          gOut<<"0 "<<(*vec_sf_i).frameNumber<<" NULL "<<std::hex \
              <<(*vec_sf_i).address<<std::dec<<endl;
        }*/
        if ((*vec_sf_i).lineNumber > 0)
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

  /////Added by Hui 12/23/15: output Input_preStackTrace////////////////////////
  if(argc >3) {
    int psize = pre_instances.size();
    pOut<<psize<<endl;   // output the number of instances in Input_pygmy
    
    vector<Instance>::iterator vec_I_ip;
    for (vec_I_ip = pre_instances.begin(); vec_I_ip != pre_instances.end(); vec_I_ip++) {
      int frameSize = (*vec_I_ip).frames.size();
      pOut<<frameSize<<" "<<(*vec_I_ip).processTLNum<<endl;//added processTLNum by Hui 12/25/15
      vector<StackFrame>::iterator vec_sf_ip;
      for (vec_sf_ip = (*vec_I_ip).frames.begin(); \
            vec_sf_ip != (*vec_I_ip).frames.end(); vec_sf_ip++)
      {
        //The first frame is always the address within the handler, we don't want to count it in
        if ((*vec_sf_ip).frameNumber == 0)
        {
          pOut<<"0 "<<(*vec_sf_ip).frameNumber<<" NULL "<<std::hex \
              <<(*vec_sf_ip).address<<std::dec<<endl;
        }
        else if ((*vec_sf_ip).lineNumber > 0)
        {
          pOut<<(*vec_sf_ip).lineNumber<<" "<<(*vec_sf_ip).frameNumber<<" ";
          pOut<<(*vec_sf_ip).moduleName<<" "<<std::hex \
            <<(*vec_sf_ip).address<<std::dec<<endl;;
        }
        else
        {
          pOut<<"0 "<<(*vec_sf_ip).frameNumber<<" NULL "<<std::hex<<(*vec_sf_ip).address<<std::dec<<endl;
        }
      }
    }
  }
  ///////////////////////////////////////////////////////////////////////////////////////////
}
    //////////////////See if ss has the right info ///////////////////////////
    /*    
    string ss_cp = ss.str();
    std::cout<<"The content of ss is: "<<ss_cp<<std::endl;
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
 
