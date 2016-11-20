#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
//#include <dirent.h> // for interating the directory
#include <algorithm> // for std::count

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

//cmd: addParser lulesh        
//     argv[0]  argv[1] 

void populateFrames(Instance &inst, ifstream &ifs, BPatch_process* proc, string inputFile)
{    
  char linebuffer[2000];
  ifs.getline(linebuffer,2000);
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
    string frameName;
    //int stackSize = str.length()/22;
    //above: 15 for 32-bit, 18 for 48-bit, 22 for 64-bit system
    if (str.length() <= 0) {
      cerr<<"Null Stack Size"<<endl;
      return;
    }
    //We can't use the previous way since the size of each frame is uncertain now
    //we count the occurences of '\t' since each frame outputs one '\t'
    size_t stackSize = std::count(str.begin(), str.end(), '\t');
    string emptyStr("NoFuncFound");
  
    for (int a = 0; a < stackSize; a++) {
      StackFrame sf;
      ss>>frameNum; // 1st: frameNum
      if (frameNum != a) {
        cerr<<"Missing a stack frame "<<frameNum<<" "<<a<<" in "<<inputFile<<" of inst#"<<inst.instNum<<endl;
        break;//break out the for loop, directly go to the next iteration
      }

      ss>>std::hex>>address>>std::dec; // 2nd: address
      if (frameNum != 0)
        address = address - 1; //sampled IP should points to the last instruction 
      
      BPatch_Vector<BPatch_statement> sLines;
      proc->getSourceLines(address,sLines); // it can get the information associate 
                                    // with the address, the vector sLines contain 
                                    //pairs of filenames(source file names) and 
                                    //line numbers that are associated with address

      ss>>frameName; // 3rd: frameName
      bool isForkFrame = false;
      int loc, rem, fID, f_num;
      if (isForkStarWrapper(frameName)) {
        isForkFrame = true;
        ss>>loc;
        ss>>rem;
        ss>>fID;
        ss>>f_num;
      }

      //get frameName in case it's missing in libunwind
      else if (frameName == "***") {
        BPatch_function *func = proc->findFunctionByAddr((void*)address);
        if (func) {
          frameName = func->getName();
          if (frameName.empty()) //In case we still cant get the name
            frameName = "***";
        }
        //After this, some frameName can be fork*wrapper, but we don't have
        //fork_t info of that since the stupid libunwind didn't recognize it
        //during runtime: TODO: use the nearby fork*wrapper info
      }

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
        sf.frameName = frameName;
        if (isForkFrame) //info fields only update in fork*wrapper frame
          sf.info = {loc, rem, fID, f_num};
      }
      else {
        sf.lineNumber = -1;
        sf.moduleName = emptyStr;
        sf.frameNumber = a;
        sf.address = address;
        sf.frameName = frameName;
      }
      inst.frames.push_back(sf);
    }// end of for loop
  }
}

void populateSamples(vector<Instance> &instances, char *exeName, std::ifstream &ifs, string inputFile)
{
  BPatch bpatch;
  BPatch_process* proc = NULL;
  //BPatch_image* appImage = NULL;
  //BPatch_Vector<BPatch_module*>* appModules = NULL;
  
  if (!(proc = bpatch.processCreate(exeName, NULL))) {
    cerr << "error in bpatch.createProcess" << endl;
    exit(-1);
  }
  
  char linebuffer[2000];
  int inst_count = 0;
  while(!ifs.eof()) {
    ifs.getline(linebuffer,2000);
    string str(linebuffer);
    stringstream ss(str); // Insert the string into a stream
    string buf;
   
    if (str.find("<----START") != string::npos) {
      ss>>buf; //buf = "<----START"
      ss>>buf; //buf = file name [fork, preSpawn, compute]
      int tempVal; 
      ss>>tempVal; //prcoessTLNum;

      Instance inst;
      inst.processTLNum = tempVal;
      inst.instNum = inst_count;
      inst_count++;
      //only fork file has the following fields
      if (buf.find("fork") != string::npos) { 
        if(ss>>tempVal);
          inst.info.callerNode = tempVal;
        if(ss>>tempVal);
          inst.info.calleeNode = tempVal;
        if(ss>>tempVal);
          inst.info.fid = tempVal;
        if(ss>>tempVal);
          inst.info.fork_num = tempVal;
      }

      populateFrames(inst, ifs, proc, inputFile); 

      ifs.getline(linebuffer,2000);//---->END
      string str2(linebuffer);
      if (str2.find("---->END") != string::npos) {
        instances.push_back(inst);
      }
    }
  }// while loop
  
  cout<<"Done parsing a stack trace file: "<<inputFile<<endl;
}

void outputParsedSamples(vector<Instance> &instances, string inputFile, string directory)
{
  string outName = directory + "/Input-" + inputFile;
  std::ofstream ofs;
  ofs.open(outName);
  if (ofs.is_open()) {
    int size = instances.size();
    ofs<<size<<endl;   // output the number of instances in Input-
    
    vector<Instance>::iterator vec_I_i;
    for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++) {
      int frameSize = (*vec_I_i).frames.size();
      ofs<<frameSize<<" "<<(*vec_I_i).processTLNum;
      // output fork_t info for fork* inputFiles
      if (inputFile.find("fork") == 0) {//file starts with "fork"
        ofs<<" "<<(*vec_I_i).info.callerNode<<" "<<(*vec_I_i).info.calleeNode
          <<" "<<(*vec_I_i).info.fid<<" "<<(*vec_I_i).info.fork_num;
      }
      ofs<<endl;

      vector<StackFrame>::iterator vec_sf_i;
      for (vec_sf_i = (*vec_I_i).frames.begin(); 
        vec_sf_i != (*vec_I_i).frames.end(); vec_sf_i++) {

        if ((*vec_sf_i).lineNumber > 0) {
          ofs<<(*vec_sf_i).frameNumber<<" "<<(*vec_sf_i).lineNumber<<" "
            <<(*vec_sf_i).moduleName<<" "<<std::hex<<(*vec_sf_i).address
            <<std::dec<<" "<<(*vec_sf_i).frameName;
          
          if ((*vec_sf_i).info.callerNode >= 0) //since default is -1 if not changed
            ofs<<" "<<(*vec_sf_i).info.callerNode<<" "<<(*vec_sf_i).info.calleeNode
              <<" "<<(*vec_sf_i).info.fid<<" "<<(*vec_sf_i).info.fork_num;   
          ofs<<endl;
        }
        else { //when lineNumber <= 0, no module found
          ofs<<(*vec_sf_i).frameNumber<<" 0 NULL "<<std::hex
            <<(*vec_sf_i).address<<std::dec<<" "<<(*vec_sf_i).frameName<<endl;
        }
      }
    }
    // CLOSE the file
    ofs.close();
  }

  else
    cerr<<"Error: could not open file: "<<outName<<endl;
}


int main(int argc, char** argv)
{ 
  if (argc < 2) //changed by Hui 12/23/15: it should be 4 for multi-thread code
  {
    std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
    std::cerr<<"Usage: <this.exe>  <target app>"<<std::endl; 
    exit(0);
  }

  char buffer[128];
  gethostname(buffer,127);
  std::string whichNode(buffer);  // whichNode = pygmy
  std::string inputFile;
  std::ifstream ifs_compute, ifs_preSpawn;
  std::ifstream ifs_fork, ifs_fork_nb, ifs_fork_fast;
  
  vector<Instance> instances, preSpawnInstances;
  vector<Instance> forkInstances, fork_nbInstances, fork_fastInstances;
  
  // parse compute file then output Input-compute file
  ifs_compute.open(whichNode);
  if (ifs_compute.is_open()) {
    populateSamples(instances, argv[1], ifs_compute, whichNode); 
    if (instances.size())
      outputParsedSamples(instances, whichNode, "COMPUTE");
    else
      cerr<<"Error: file exists but instances empty"<<endl;
    ifs_compute.close(); //CLOSE the file
  }
    
  // parse preSpawn file then output Input-preSpawn- file
  inputFile = "preSpawn-" + whichNode;
  ifs_preSpawn.open(inputFile);
  if (ifs_preSpawn.is_open()) {
    populateSamples(preSpawnInstances, argv[1], ifs_preSpawn, inputFile);
    if (preSpawnInstances.size())
      outputParsedSamples(preSpawnInstances, inputFile, "PRESPAWN");
    else
      cerr<<"Error: file exists but instances empty"<<endl;
    ifs_preSpawn.close(); //CLOSE the file
  }

  // parse fork file then output Input-fork- file
  inputFile = "fork-" + whichNode;
  ifs_fork.open(inputFile);
  if (ifs_fork.is_open()) {
    populateSamples(forkInstances, argv[1], ifs_fork, inputFile);
    if (forkInstances.size())
      outputParsedSamples(forkInstances, inputFile, "FORK");
    else
      cerr<<"Error: file exists but instances empty"<<endl;
    ifs_fork.close(); //CLOSE the file
  }

  // parse fork_nb file then output Input-fork_nb- file
  inputFile = "fork_nb-" + whichNode;
  ifs_fork_nb.open(inputFile);
  if (ifs_fork_nb.is_open()) {
    populateSamples(fork_nbInstances, argv[1], ifs_fork_nb, inputFile);
    if (fork_nbInstances.size())
      outputParsedSamples(fork_nbInstances, inputFile, "FORK");
    else
      cerr<<"Error: file exists but instances empty"<<endl;
    ifs_fork_nb.close(); //CLOSE the file
  }

  // parse fork_fast file then output Input-fork_fast- file
  inputFile = "fork_fast-" + whichNode;
  ifs_fork_fast.open(inputFile);
  if (ifs_fork_fast.is_open()) {
    populateSamples(fork_fastInstances, argv[1], ifs_fork_fast, inputFile);
    if (fork_fastInstances.size())
      outputParsedSamples(fork_fastInstances, inputFile, "FORK");
    else
      cerr<<"Error: file exists but instances empty"<<endl;
    ifs_fork_fast.close(); //CLOSE the file
  }

  return 0;
}

 
/*
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
        // could not open directory 
        std::cerr<<"Couldn't open the directory !"<<std::endl;
}
*/
 
