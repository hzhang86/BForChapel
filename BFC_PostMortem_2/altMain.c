/*
 *  altMain.C
 *  
 *
 *  Created by Nick Rutar on 2/10/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/time.h>

#include "BlameProgram.h"
#include "BlameFunction.h"
#include "BlameModule.h"
#include "Instances.h"

using namespace std;


void my_timestamp()
{
  time_t ltime; 
  ltime = time(NULL);
  struct timeval detail_time;
  gettimeofday(&detail_time,NULL);
   
  fprintf(stderr, "%s ", asctime( localtime(&ltime) ) );
  fprintf(stderr, "%d\n", detail_time.tv_usec /1000);
}


/*
void my_timestamp()
{
  struct timeval detail_time;
  gettimeofday(&detail_time,NULL);
  fprintf(stderr, "%d %d\n",
    detail_time.tv_usec /1000,  
    detail_time.tv_usec); }
*/

void Instance::handleInstance(ModuleHash &modules, std::ostream &O, bool verbose, int InstanceNum)
{
  /*
    cout<<"Handle instances!"<<std::endl;
    cout<<"We're working with these modules:"<<std::endl;
    
    ModuleHash::iterator mh_i;
    
    for (mh_i = modules.begin(); mh_i != modules.end(); mh_i++)
    {
    BlameModule * bm = (*mh_i).second;
    O<<"Module "<<bm->getName()<<" with hash value "<<(*mh_i).first<<std::endl;
    }
  */
  
  std::cout<<"Handle instance "<<InstanceNum<<"!"<<std::endl;
  // This is true in the case where we're at the last stack frame that can be parsed
  bool isBottomParsed = true;
  //if main->foo->bar, then bar is the bottom stackframe
  vector<StackFrame>::iterator vec_SF_i;
  
  // Each instance has multiple stack frames, we apply transfer functions to propagate
  //   the stack
  for (vec_SF_i = frames.begin(); vec_SF_i != frames.end(); vec_SF_i++) {
    if ((*vec_SF_i).lineNumber > 0) {
    //cout<<endl;
    //cout<<"^^^^^At Frame "<<(*vec_SF_i).frameNumber<<" at line num "<<(*vec_SF_i).lineNumber;
    //cout<<" in module "<<(*vec_SF_i).moduleName<<endl;
    
    // Get the module from the debugging information
      BlameModule *bm = modules[(*vec_SF_i).moduleName.c_str()];   //return the pointer to a BlameModule object
    
      if (bm != NULL) {
        // Use the combination of the module and the line number to determine the function
        BlameFunction *bf = bm->findLineRange((*vec_SF_i).lineNumber);
      
        if (bf) {
        //O<<"##PATH "<<bf->getModulePathName()<<std::endl;
        //cout<<"<--ALL_INFO-- At Frame "<<(*vec_SF_i).frameNumber<<endl;
        //cout<<" At line num "<<(*vec_SF_i).lineNumber<<endl;
        //cout<<" In module "<<bf->getModulePathName()<<"/"<<(*vec_SF_i).moduleName<<endl;
        //cout<<" In function "<<bf->getName()<<endl;
      
          std::set<VertexProps *> blamedParams;
      
          if (bf->getBlamePoint() > 0) {
            cout<<"In function "<<bf->getName()<<" is BP="<<bf->getBlamePoint()<<endl;
            // frames(all frames), modules (hash of all modules), vec_SF_i (current iterator),
            //blamedParams(empty int vector), true (is a blamePoint), isBottomParsed(true/false) blamePoint can be 0,1,2, why always set true(1) here ??
            bf->resolveLineNum(frames, modules, vec_SF_i, blamedParams, true, isBottomParsed, NULL, O);
            return;
          }
          else {
            cout<<"In function "<<bf->getName()<<" is not BP"<<endl;
            bf->resolveLineNum(frames, modules, vec_SF_i, blamedParams, false, isBottomParsed, NULL, O);
            return;
          }
      
          //std::cout<<endl;
      
          //We have now parsed a frame and we are moving up the stack,no more unparsed
          // TODO: what to do when we have a break in the debugging info from the stack trace
          isBottomParsed = false;
        }
        else {
          if (isBottomParsed == false) {
            std::cerr<<"Break in stack debugging info, BF is NULL"<<endl;
            isBottomParsed = true;
          }
          cout<<"****BF NULL-- At Frame "<<(*vec_SF_i).frameNumber<<" at line num "<<(*vec_SF_i).lineNumber;
          cout<<" in module "<<(*vec_SF_i).moduleName<<" at address "<<(*vec_SF_i).address<<endl;
        }
      }
      else {
        if (isBottomParsed == false) {
          std::cerr<<"Break in stack debugging info, BM is NULL"<<endl;
          isBottomParsed = true;
        }
        
        cout<<"****BM NULL- At Frame "<<(*vec_SF_i).frameNumber<<" at line num "<<(*vec_SF_i).lineNumber;
        cout<<" in module "<<(*vec_SF_i).moduleName<<" at address "<<(*vec_SF_i).address<<endl;
      }
    }
      // ELSE case if line number not found from sample point, external library or no debugging info
    else {
      cout<<std::hex<<"****NO LINENUM- At Frame "<<(*vec_SF_i).frameNumber<<" at address "<<(*vec_SF_i).address<<endl;
      cout<<std::dec;
    }
  }
}

void populateSamples(vector<Instance> & instances, char * exeName,
        const char *traceName)
{
  ifstream bI(traceName);
  std::string line;
  
  getline(bI, line);
  
  int numInstances = atoi(line.c_str());
 
  fprintf(stderr,"Number of instances is %d\n", numInstances);
 
  char *buffer = (char *) malloc(100*sizeof(char));
  
  for (int a = 0; a < numInstances; a++) {
    Instance i;
    getline(bI, line);
      
    int numFrames = atoi(line.c_str());
         
    for (int b = 0; b < numFrames; b++) {
      StackFrame sf;
      getline(bI, line);
    
      sscanf(line.c_str(), "%d %d %s %x", &(sf.lineNumber), &(sf.frameNumber), 
      buffer, &(sf.address));
      //printf("%d %d %s %x\n", sf.lineNumber, sf.frameNumber, buffer, sf.address);
      sf.moduleName.assign(buffer);
      i.frames.push_back(sf);
    }
    
    instances.push_back(i);
  }
}


void Instance::printInstance()
{
  vector<StackFrame>::iterator vec_SF_i;
  for (vec_SF_i = frames.begin(); vec_SF_i != frames.end(); vec_SF_i++)
    {
      if ((*vec_SF_i).lineNumber > 0)
  {
    cout<<"At Frame "<<(*vec_SF_i).frameNumber<<" at line num "<<(*vec_SF_i).lineNumber;
    cout<<" in module "<<(*vec_SF_i).moduleName<<endl;
  }
      else
  {
    cout<<std::hex;
    cout<<"At Frame "<<(*vec_SF_i).frameNumber<<" at address "<<(*vec_SF_i).address<<endl;
    cout<<std::dec;
  }
    }
}


// <altMain> <exe Name(not used)> <trace name> <config file name> 

int main(int argc, char** argv)
{ 
  if (argc != 4)
    {
      std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
      exit(0);
    }
  
  bool verbose = false;
  
  fprintf(stderr,"START - ");
  my_timestamp();
  
  BlameProgram bp;
  bp.parseConfigFile(argv[3]);
  
  //std::cout<<"Parsing structs "<<std::endl;
  bp.parseStructs();
  //std::cout<<"Printing structs"<<std::endl;
  //bp.printStructs(std::cout);
  
  //std::cout<<"Parsing side effects "<<std::endl;
  bp.parseSideEffects();
  
  //std::cout<<"Printing side effects(PRE)"<<std::endl;
  //bp.printSideEffects();
  
  //std::cout<<"Calc Rec side effects"<<std::endl;
  bp.calcRecursiveSEAliases();
  
  //std::cout<<"Printing side effects(POST)"<<std::endl;
  //bp.printSideEffects();
  
  //std::cout<<"Grab used modules "<<std::endl;
  bp.grabUsedModules(argv[2]);
  
  //std::cout<<"Parsing program"<<std::endl;
  bp.parseProgram();
  
  //std::cout<<"Adding implicit blame points."<<std::endl;
  bp.addImplicitBlamePoints();
  
  
  //std::cout<<"Printing parsed output to 'output.txt'"<<std::endl;
  //ofstream outtie("output.txt");
  //bp.printParsed(outtie);
  
  
  
  std::string whichNode(argv[2]);
  //std::string guiOut("Output_");
  //guiOut += whichNode;
  
  std::string truncNode(whichNode.substr(whichNode.find("Input_")+6));
  truncNode.insert(0,"PARSED_");// here you get: truncNode=PARSED_pygmy
  
  //whichNode.insert(whichNode.find("Input"), "Output_");

  std::ofstream gOut(truncNode.c_str());

  //std::ofstream gOut(whichNode.c_str());
//  std::ofstream gOut(guiOut.c_str());

  
  vector<Instance>  instances;
  
  //bp.calcSideEffects();
  
  
  //std::cout<<"Populating samples."<<std::endl;

  fprintf(stderr,"SAMPLES - ");
  my_timestamp();

  populateSamples(instances, argv[1], argv[2]);
  
  unsigned iCounter = 0;
  
  vector<Instance>::iterator vec_I_i;
  /*
  for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++)
    {
      (*vec_I_i).printInstance();
    }
  */

  for (vec_I_i = instances.begin(); vec_I_i != instances.end(); vec_I_i++)
  {
    gOut<<"---INSTANCE "<<iCounter<<"  ---"<<std::endl;
      
    //std::cout<<std::endl<<std::endl;
    //std::cout<<"---INSTANCE "<<iCounter<<"  ---"<<std::endl;
    
    (*vec_I_i).handleInstance(bp.blameModules,gOut,verbose,iCounter);
    //(*vec_I_i).handleInstance(bp.blameModules,std::cout,verbose);
    gOut<<"$$$INSTANCE "<<iCounter<<"  $$$"<<std::endl;
    //std::cout<<"$$$INSTANCE "<<iCounter<<"  $$$"<<std::endl;
    
    iCounter++;
    
    //cout<<"-----------------\n";
  }
  
  fprintf(stderr,"DONE - ");
  my_timestamp();
  
}
