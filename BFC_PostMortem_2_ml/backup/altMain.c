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

void Instance::printInstance()
{
  vector<StackFrame>::iterator vec_SF_i;
  for (vec_SF_i = frames.begin(); vec_SF_i != frames.end(); vec_SF_i++) {
    if ((*vec_SF_i).lineNumber > 0){
      cout<<"At Frame "<<(*vec_SF_i).frameNumber<<" at line num "<<(*vec_SF_i).lineNumber;
      cout<<" in module "<<(*vec_SF_i).moduleName<<endl;
    }
    else {
      cout<<std::hex;
      cout<<"At Frame "<<(*vec_SF_i).frameNumber<<" at address "<<(*vec_SF_i).address<<endl;
      cout<<std::dec;
    }
  }
}

void Instance::printInstance_concise()
{
  vector<StackFrame>::iterator vec_SF_i;
  int validFrameCount = 0;

  for (vec_SF_i = frames.begin(); vec_SF_i != frames.end(); vec_SF_i++) {
    //if((*vec_SF_i).toRemove == false){
      validFrameCount++;
      cout<<(*vec_SF_i).lineNumber<<" "<<(*vec_SF_i).frameNumber<<" "<<(*vec_SF_i).moduleName<<" ";
      cout<<std::hex;
      cout<<(*vec_SF_i).address;
      cout<<std::dec;
      cout<<endl;
    //}
  }
  std::cout<<"After trimFrames, the inst now has "<<validFrameCount<<" valid frames"<<std::endl;
  std::cout<<endl;
}

//Added by Hui 12/20/15 : set all invalid frames's toRemove tag to TRUE in an instance
void Instance::trimFrames(ModuleHash &modules, int InstanceNum/*, vector<StackFrame> &oldFrames*/)
{
  cout<<"Triming Instance "<<InstanceNum<<endl;
  vector<StackFrame>::iterator vec_SF_i;
  vector<StackFrame> newFrames(frames);
  bool isBottomParsed = true;
  frames.clear();
  
  for (vec_SF_i=newFrames.begin(); vec_SF_i!=newFrames.end(); vec_SF_i++) {
    if ((*vec_SF_i).lineNumber <= 0) {
        cout<<"LineNum <=0, delete frame "<<(*vec_SF_i).frameNumber<<endl;
        (*vec_SF_i).toRemove = true;
    }
    else {
      BlameModule *bm = NULL;
      const char* tmpptr = (*vec_SF_i).moduleName.c_str();
      if (tmpptr)
          bm = modules[tmpptr];
      if (bm == NULL) {
        cout<<"BM is NULL, delete frame "<<(*vec_SF_i).frameNumber<<endl;
        (*vec_SF_i).toRemove = true;
      }
      else {
        // Use the combination of the module and the line number to determine the function
        BlameFunction *bf = bm->findLineRange((*vec_SF_i).lineNumber);
        if (bf == NULL) {
          cout<<"BF is NULL, delete frame "<<(*vec_SF_i).frameNumber<<endl;
          (*vec_SF_i).toRemove = true;
        }
        else {
          if (bf->getName().compare("chpl_user_main")==0 && bf->getBLineNum()==(*vec_SF_i).lineNumber) {
            cout<<"Frame cannot be main while the ln is BLineNum, delete frame "<<(*vec_SF_i).frameNumber<<endl;
            (*vec_SF_i).toRemove = true;
          }
          else {
            if (isBottomParsed == true) {
              isBottomParsed = false; //if it's first frame, then it doesn't
              continue;               //have to have callNodes, but later does
            }
            else {
              std::vector<VertexProps*>::iterator vec_vp_i;
              VertexProps *callNode = NULL;
              std::vector<VertexProps*> matchingCalls;
              
              for (vec_vp_i = bf->callNodes.begin(); vec_vp_i != bf->callNodes.end(); vec_vp_i++) {
                VertexProps *vp = *vec_vp_i;
                if (vp->declaredLine == (*vec_SF_i).lineNumber) {
                  //just for test 
                  cout<<"matching callNode: "<<vp->name<<endl;
                  matchingCalls.push_back(vp);
                }
              }
              if (matchingCalls.size() == 0) {
                cout<<"There's no matching callNode in this line, delete frame "<<(*vec_SF_i).frameNumber<<endl;
                (*vec_SF_i).toRemove = true;
              }
              else if (matchingCalls.size() > 1) { //exclude frame that maps to "forall/coforall" loop lines
                callNode = NULL;
                std::cout<<"More than one call node at that line number"<<std::endl;
                // figure out which call is appropriate
                vector<StackFrame>::iterator minusOne = vec_SF_i - 1;
                BlameModule *bmCheck = modules[(*minusOne).moduleName.c_str()];
                if (bmCheck == NULL) {
                  cout<<"BM of previous frame is null ! delete frame "<<(*vec_SF_i).frameNumber<<endl;
                  (*vec_SF_i).toRemove = true;
                }

                else {
                  BlameFunction *bfCheck = bmCheck->findLineRange((*minusOne).lineNumber);
                  if (bfCheck == NULL) {
                    cout<<"BF of previous frame is null ! delete frame "<<(*vec_SF_i).frameNumber<<endl;
                    (*vec_SF_i).toRemove = true;
                  }
                  else {
                    std::vector<VertexProps *>::iterator vec_vp_i2;
                    for (vec_vp_i2 = matchingCalls.begin(); vec_vp_i2 != matchingCalls.end(); vec_vp_i2++) {
                      VertexProps *vpCheck = *vec_vp_i2;
                      // Look for subsets since vpCheck will have the line number concatenated
                      if (vpCheck->name.find(bfCheck->getName()) != std::string::npos)
                        callNode = vpCheck;
                    }
              
                    if (callNode == NULL) {
                      cout<<"No matching call nodes from multiple matches, delete frame "<<(*vec_SF_i).frameNumber<<endl;
                      (*vec_SF_i).toRemove = true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  //pick all the valid frames and push_back to "frames" again
  for (vec_SF_i=newFrames.begin(); vec_SF_i!=newFrames.end(); vec_SF_i++) {
    if ((*vec_SF_i).toRemove == false) {
        StackFrame sf;
        sf.lineNumber = (*vec_SF_i).lineNumber;
        sf.frameNumber = (*vec_SF_i).frameNumber;
        //sf.moduleName.assign((*vec_SF_i).moduleName.c_str());
        sf.moduleName = (*vec_SF_i).moduleName;
        sf.address = (*vec_SF_i).address;
        sf.toRemove = (*vec_SF_i).toRemove;

        frames.push_back(sf);
    }
  }

  //thorougly free the memory of newFrames
  vector<StackFrame>().swap(newFrames); //here is why this works: 
                //http://prateekvjoshi.com/2013/10/20/c-vector-memory-release/
  // print the new instance
  cout<<"After trimFrames, Instance #"<<InstanceNum<<endl;
  printInstance_concise();
}

void Instance::handleInstance(ModuleHash &modules, std::ostream &O, bool verbose, int InstanceNum)
{
  
  std::cout<<"Handle instance "<<InstanceNum<<"! Originally it has "<<frames.size()<<" frames"<<std::endl;

  //Added by Hui 12/20/15/////////////////////////////
  trimFrames(modules, InstanceNum);
  ////////////////////////////////////////////////////
  // This is true in the case where we're at the last stack frame that can be parsed
  bool isBottomParsed = true;
  //if main->foo->bar, then bar is the bottom stackframe
  vector<StackFrame>::iterator vec_SF_i;
  // Each instance has multiple stack frames, we apply transfer functions to propagate
  //   the stack
  for (vec_SF_i = frames.begin(); vec_SF_i != frames.end(); vec_SF_i++) {
    if ((*vec_SF_i).lineNumber > 0 && (*vec_SF_i).toRemove == false) {
    // Get the module from the debugging information
    BlameModule *bm = modules[(*vec_SF_i).moduleName.c_str()];   //return the pointer to a BlameModule object
    
      ////////Hui test funcBySet 12/07/15/////////////////
      /*cout<<"In funcsBySet: ";
      set<BlameFunction *>::iterator sbi;
      for(sbi=bm->funcsBySet.begin();sbi!=bm->funcsBySet.end();sbi++)
           cout<<(*sbi)->getName()<<" ";
      cout<<endl;*/
      ////////////////////////////////////////////////////

      if (bm != NULL) {
        // Use the combination of the module and the line number to determine the function
        BlameFunction *bf = bm->findLineRange((*vec_SF_i).lineNumber);
        
        if (bf) {
          //Added by Hui 12/20/15 /////////////////////////////////
          /*
          if (bf->getName().compare("chpl_user_main")==0 && bf->getBLineNum()==(*vec_SF_i).lineNumber) {
            cout<<"Bottom frame can't be main when the lineNum is BegLine, Ignore it"<<endl;
            continue;
          }
          */
          //////////////////////////////////////////////////////
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
            return; //once the resolveLineNum returns from a frame, then the whole instance done
          }
          else {
            cout<<"In function "<<bf->getName()<<" is not BP"<<endl;
            bf->resolveLineNum(frames, modules, vec_SF_i, blamedParams, false, isBottomParsed, NULL, O);
            return;//once the resolveLineNum returns from a frame, the whole instance is done
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
      sf.toRemove = false;//Added by Hui 12/20/15
      i.frames.push_back(sf);
    }
    
    instances.push_back(i);
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
  vector<Instance>  pre_instances; //Added by Hui 12/25/15
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
