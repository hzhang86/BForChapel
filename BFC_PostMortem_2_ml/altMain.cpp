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
#include <algorithm>
#include <dirent.h> // for interating the directory

#include "BlameProgram.h"
#include "BlameFunction.h"
#include "BlameModule.h"
#include "Instances.h"

using namespace std;

//Hui for test: output file for finer stacktraces
ofstream stack_info;
nodeHash nodeIDNameMap; //Initialized in populateForkSamples

// some forward-declarations
static void glueStackTraces(string node, int InstNum, Instance &inst, 
        preInstanceHash &pre_instance_table, forkInstanceHash &fork_instance_table);
static void glueForkTrace(Instance &inst, forkInstanceHash &fork_instance_table,
                                preInstanceHash &pre_instance_table, string node);
static void gluePreTrace(Instance &inst, Instance &pre_inst, string node, int PTLN);

static void populateCompSamples(vector<Instance> &comp_instances, string traceName, string nodeName);
static void populatePreSamples(InstanceHash &pre_instances, string traceName, string nodeName);
static void populateForkSamples(vector<Instance> &fork_instances, string traceName, string nodeName);
static void populateSamplesFromDirs(compInstanceHash &comp_instance_table, 
        preInstanceHash &pre_instance_table, forkInstanceHash &fork_instance_table);

//the following util function will be used globablly
//====================vv========================================//
bool isForkStarWrapper(std::string name)
{
  if (name == "fork_wrapper" || name == "fork_nb_wrapper" ||
      name == "fork_large_wrapper" || name == "fork_nb_large_wrapper")
    return true;
  else
    return false;
}

bool isTopMainFrame(std::string name)
{
  if (name == "chpl_user_main" || name == "chpl_gen_main")
    return true;
  else
    return false;
}

bool forkInfoMatch(StackFrame &sf, Instance &inst)
{
  if (sf.info.callerNode == inst.info.callerNode &&
      sf.info.calleeNode == inst.info.calleeNode &&
      sf.info.fid == inst.info.fid &&
      sf.info.fork_num == inst.info.fork_num)
    return true;
  else
    return false;
}
//========================^^===============================//

void my_timestamp()
{
  time_t ltime; 
  ltime = time(NULL);
  struct timeval detail_time;
  gettimeofday(&detail_time,NULL);
   
  fprintf(stderr, "%s ", asctime( localtime(&ltime) ) );
  fprintf(stderr, "%d\n", detail_time.tv_usec /1000);
}


//Added by Hui 12/25/15
static void glueStackTraces(string node, int InstNum, Instance &inst, 
        preInstanceHash &pre_instance_table, forkInstanceHash &fork_instance_table)
{
  stack_info<<"In glueStackTraces for instance "<<InstNum<<" on "<<node<<endl;
  InstanceHash &preForNode = pre_instance_table[node];
  
  if (inst.needGlueFork) {//if the last frame is fork*wrapper,we skip gluePreTrace
    glueForkTrace(inst, fork_instance_table, pre_instance_table, node);//Recursive 
    if (inst.isMainThread==true || isTopMainFrame(inst.frames.back().frameName)) {
      if (inst.frames.back().frameName == "chpl_gen_main")
        inst.frames.pop_back(); //we delete chpl_gen_main frame
      stack_info<<"Finish glueStackTraces for inst "<<InstNum<<" to main"<<endl;
    }
    else
      stack_info<<"Fail glueStackTraces for inst "<<InstNum<<" to main"<<endl;
    
    return;
  }
  
  else {
    //inst != mainThread (top frame isn't chpl_*_main) && inst doesn't have 
    //fork*wrapper as top frame, then it has to be glued with preSpawn stacktrace 
    if (preForNode.count(inst.processTLNum) == 0) {
      stack_info<<"Error: worker thread has pTLN "<<inst.processTLNum<<
          " not found in preSpawn on "<<node<<endl;
      return;
    }
    Instance &pre_inst = preForNode[inst.processTLNum];
    gluePreTrace(inst, pre_inst, node, inst.processTLNum);//Non-recursive
  
    //Check if the inst comes to main now
    if (inst.isMainThread==true || isTopMainFrame(inst.frames.back().frameName)) {
      if (inst.frames.back().frameName == "chpl_gen_main")
        inst.frames.pop_back(); //we delete chpl_gen_main frame
      stack_info<<"Finish glueStackTraces for inst "<<InstNum<<" to main"<<endl;
      return;
    }
    else if (inst.needGlueFork) {//we have fork*wrapper as last frame after gluePre
      glueForkTrace(inst, fork_instance_table, pre_instance_table, node);//Recursive
      if (inst.isMainThread==true || isTopMainFrame(inst.frames.back().frameName)) {
        if (inst.frames.back().frameName == "chpl_gen_main")
          inst.frames.pop_back(); //we delete chpl_gen_main frame
        stack_info<<"Finish glueStackTraces for inst "<<InstNum<<" to main"<<endl;
      }
      else
        stack_info<<"Fail glueStackTraces for inst "<<InstNum<<" to main"<<endl;
      
      return;
    }
    else {//not come to main nor need more glueForkTrace, then it's incomplete
      stack_info<<"Fail glueStackTraces for inst "<<InstNum<<" to main"<<endl;
      return;
    }
  }
}

static void glueForkTrace(Instance &inst, forkInstanceHash &fork_instance_table,
                                preInstanceHash &pre_instance_table, string node)
{
  if (inst.isMainThread==true || isTopMainFrame(inst.frames.back().frameName)) 
    return;
  if (!inst.needGlueFork)
    return;

  stack_info<<"Glueing inst&fork_inst on "<<node<<endl;
  forkInstanceHash::iterator fh_i;
  vector<Instance>::iterator vec_I_i;
  //fh_i: <string nodeName, vector<Instance> fork_vec>
  for (fh_i=fork_instance_table.begin(); fh_i!=fork_instance_table.end(); fh_i++) {
    vector<Instance> &fork_vec = (*fh_i).second;
    for (vec_I_i=fork_vec.begin(); vec_I_i!=fork_vec.end(); vec_I_i++) {
      if (forkInfoMatch(inst.frames.back(), *vec_I_i)) {
        if (!(*vec_I_i).frames.empty()) {
          inst.frames.pop_back(); //IMPORTANT: delete the last fork* frame
          inst.frames.insert(inst.frames.end(), (*vec_I_i).frames.begin(),
                                                (*vec_I_i).frames.end());
          if (isForkStarWrapper(inst.frames.back().frameName)) {
            //We need to do recursion here
            inst.needGlueFork = true;
            //IMPORTANT: the new link info(from frames.back()) should be vec_i_i's now
            stack_info<<"Start recursively call glueForkTrace"<<endl;
            glueForkTrace(inst, fork_instance_table, pre_instance_table, node); 
            return; // we should return
          }
          else if (isTopMainFrame(inst.frames.back().frameName)) {
            inst.isMainThread = true;
            return; //
          }
          else { //new last frame isn't main nor fork*wrapper, we just glue pre
            //IMPORTANT: the new link info should be vec_i_i's now
            int forkPTLN = (*vec_I_i).processTLNum;
            string node2 = nodeIDNameMap[(*vec_I_i).info.callerNode]; 
            InstanceHash preForNode = pre_instance_table[node2];
            if (preForNode.count(forkPTLN) == 0) {
              stack_info<<"Error: fork inst has pTLN "<<forkPTLN<<
                    " not found in preSpawn on "<<node2<<endl;
              return;
            }

            Instance &pre_inst = preForNode[forkPTLN];
            gluePreTrace(inst, pre_inst, node2, forkPTLN);
            return; 
          }
        }
        else {//fork_inst's frames is empty,BUT we can still glue its pre_inst
          stack_info<<"Corresponding fork_inst is empty"<<endl;
          // since we don't need this fork*wrapper frame, we should dump it 
          inst.frames.pop_back();
          //IMPORTANT: the new link info should be vec_i_i's now
          int forkPTLN = (*vec_I_i).processTLNum;
          string node2 = nodeIDNameMap[(*vec_I_i).info.callerNode]; 
          InstanceHash preForNode = pre_instance_table[node2];
          if (preForNode.count(forkPTLN) == 0) {
            stack_info<<"Error: fork inst has pTLN "<<forkPTLN<<
                  " not found in preSpawn on "<<node2<<endl;
            return;
          }

          Instance &pre_inst = preForNode[forkPTLN];
          gluePreTrace(inst, pre_inst, node2, forkPTLN);
          return;
        }
      }//We found matched fork instance, everything should return within this block
    }
  }

  //Shouldn't come to this point since it means we can't glue fork
  StackFrame &lastFrame = inst.frames.back();
  stack_info<<"Error: we couldn't find matched fork_inst, we need: "<<
    lastFrame.info.callerNode<<" "<<lastFrame.info.calleeNode<<" "<<lastFrame.info.fid<<" "
    <<lastFrame.info.fork_num<<" for "<<lastFrame.frameName<<" on "<<node<<endl;
}

static void gluePreTrace(Instance &inst, Instance &pre_inst, string node, int PTLN)
{
    //The following condition may not hold since we've removed all frames that 
    //coorespond to coforall/wrapcoforall functions, need to check safety !
/*
    vector<StackFrame>::iterator p_SFi = pre_inst.frames.begin();  
    StackFrame &c_SFr = inst.frames.back(); //returns a reference, not iterator
    if ((*p_SFi).lineNumber != c_SFr.lineNumber || 
            (*p_SFi).moduleName != c_SFr.moduleName) {
      stack_info<<"Can't glue child and parent stacktrace due to mismatch!"<<endl;
      return;
    }
    inst.frames.pop_back(); //delete the last element in the vector
    p_SFi++; //start from the second frame in pre_inst 
    for (; p_SFi!=parent.frames.end(); p_SFi++) {
      if ((*p_SFi).toRemove == false) {
        StackFrame sf;
        sf.lineNumber = (*p_SFi).lineNumber;
        sf.frameNumber = i.frames.back().frameNumber+1;
        sf.moduleName = (*p_SFi).moduleName;
        sf.address = (*p_SFi).address;
        sf.toRemove = (*p_SFi).toRemove;

        i.frames.push_back(sf);
      }
    }
*/ 
  stack_info<<"Glueing inst&pre_inst PTLN: "<<PTLN<<" on "<<node<<endl;
  if (!pre_inst.frames.empty()) {
    inst.frames.insert(inst.frames.end(), pre_inst.frames.begin(), 
                                            pre_inst.frames.end());
    if (isForkStarWrapper(inst.frames.back().frameName))
      inst.needGlueFork = true;
    else if (isTopMainFrame(inst.frames.back().frameName))
      inst.isMainThread = true;
  }
  else  
    stack_info<<"Corresponding pre_inst is empty PTLN:"<<inst.processTLNum<<endl;
  
}


static void populateCompSamples(vector<Instance> &comp_instances, string traceName, string nodeName)
{
  ifstream ifs_comp(traceName);
  string line;

  if (ifs_comp.is_open()) {
    getline(ifs_comp, line);
    int numInstances = atoi(line.c_str());
    cout<<"Number of instances from "<<traceName<<" is "<<numInstances<<endl;
 
    for (int i = 0; i < numInstances; i++) {
      Instance inst;
      getline(ifs_comp, line);
      
      int numFrames;
      inst.instType = COMPUTE_INST;
      sscanf(line.c_str(), "%d %d", &numFrames, &(inst.processTLNum));
    
      for (int j=0; j<numFrames; j++) {
        StackFrame sf;
        getline(ifs_comp, line);
        stringstream ss(line);
    
        ss>>sf.frameNumber;
        ss>>sf.lineNumber;
        ss>>sf.moduleName;
        ss>>std::hex>>sf.address>>std::dec;
        ss>>sf.frameName;

        if (isForkStarWrapper(sf.frameName)) {
          if (std::count(line.begin(), line.end(), ' ') > 4) {//fork_t info exists
            ss>>sf.info.callerNode;
            ss>>sf.info.calleeNode;
            ss>>sf.info.fid;
            ss>>sf.info.fork_num;
          }
        }
          
        inst.frames.push_back(sf);
      }
    
      comp_instances.push_back(inst);
    }
  }
  else
    cerr<<"Error: open file "<<traceName<<" fail"<<endl;
}


static void populatePreSamples(InstanceHash &pre_instances, string traceName, string nodeName)
{
  ifstream ifs_pre(traceName);
  string line;

  if (ifs_pre.is_open()) {
    getline(ifs_pre, line);
    int numInstances = atoi(line.c_str());
    cout<<"Number of pre_instances from "<<traceName<<" is "<<numInstances<<endl;
 
    for (int i = 0; i < numInstances; i++) {
      Instance inst;
      getline(ifs_pre, line);
      
      int numFrames;
      inst.instType = PRESPAWN_INST;
      sscanf(line.c_str(), "%d %d", &numFrames, &(inst.processTLNum));
    
      for (int j=0; j<numFrames; j++) {
        StackFrame sf;
        getline(ifs_pre, line);
        stringstream ss(line);
    
        ss>>sf.frameNumber;
        ss>>sf.lineNumber;
        ss>>sf.moduleName;
        ss>>std::hex>>sf.address>>std::dec;
        ss>>sf.frameName;

        if (isForkStarWrapper(sf.frameName)) {
          if (std::count(line.begin(), line.end(), ' ') > 4) {//fork_t info exists
            ss>>sf.info.callerNode;
            ss>>sf.info.calleeNode;
            ss>>sf.info.fid;
            ss>>sf.info.fork_num;
          }
        }
          
        inst.frames.push_back(sf);
      }
    
      if (pre_instances.count(inst.processTLNum) == 0)
        pre_instances[inst.processTLNum] = inst;
      else
        cerr<<"Error: more than 1 pre-instance mapped to the same processTLNum "
          <<inst.processTLNum<<" in "<<traceName<<endl;
    }
  }
  else
    cerr<<"Error: open file "<<traceName<<" fail"<<endl;
}


static void populateForkSamples(vector<Instance> &fork_instances, string traceName, string nodeName)
{
  ifstream ifs_fork(traceName);
  string line;
  int IT;

  if (traceName.find("fork_nb") != string::npos) 
    IT = FORK_NB_INST;
  else if (traceName.find("fork_fast") != string::npos)
    IT = FORK_FAST_INST;
  else
    IT = FORK_INST; // simply 'fork'

  if (ifs_fork.is_open()) {
    getline(ifs_fork, line);
    int numInstances = atoi(line.c_str());
    cout<<"Number of fork*_instances from "<<traceName<<" is "<<numInstances<<endl;
 
    for (int i = 0; i < numInstances; i++) {
      Instance inst;
      getline(ifs_fork, line);
      
      int numFrames;
      inst.instType = IT;
      sscanf(line.c_str(), "%d %d %d %d %d %d", &numFrames, &(inst.processTLNum),
              &(inst.info.callerNode), &(inst.info.calleeNode), 
              &(inst.info.fid), &(inst.info.fork_num));
      
      for (int j=0; j<numFrames; j++) {
        StackFrame sf;
        getline(ifs_fork, line);
        stringstream ss(line);
    
        ss>>sf.frameNumber;
        ss>>sf.lineNumber;
        ss>>sf.moduleName;
        ss>>std::hex>>sf.address>>std::dec;
        ss>>sf.frameName;

        if (isForkStarWrapper(sf.frameName)) {
          if (std::count(line.begin(), line.end(), ' ') > 4) {//fork_t info exists
            ss>>sf.info.callerNode;
            ss>>sf.info.calleeNode;
            ss>>sf.info.fid;
            ss>>sf.info.fork_num;
          }
        }
          
        inst.frames.push_back(sf);
      }
    
      fork_instances.push_back(inst);
      
      //fill in the nodeIDNameMap once for all
      if (i==0) {
        if(nodeIDNameMap.count(inst.info.callerNode)==0)
          nodeIDNameMap[inst.info.callerNode] = nodeName; 
        else {
          if (nodeIDNameMap[inst.info.callerNode] != nodeName)
            stack_info<<"Error: two compute nodes are mapped to same ID: "
                <<inst.info.callerNode<<endl;
        }
      }
    }
  }
  else
    cerr<<"Error: open file "<<traceName<<" fail"<<endl;
}


static void populateSamplesFromDirs(compInstanceHash &comp_instance_table, 
        preInstanceHash &pre_instance_table, forkInstanceHash &fork_instance_table)
{

  DIR *dir;
  struct dirent *ent;
  string traceName;
  string dirName;
  string nodeName;
  size_t pos;

  dirName = "COMPUTE";
  if ((dir = opendir(dirName.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-compute") != NULL)) {
        traceName = dirName + "/" + std::string(ent->d_name);
        pos = traceName.find("compute");
        nodeName = traceName.substr(pos);

        vector<Instance> comp_instances;
        populateCompSamples(comp_instances, traceName, nodeName);
        if (comp_instance_table.count(nodeName) == 0)
          comp_instance_table[nodeName] = comp_instances; //push this node's sample
                                                        //info to the table
        else
          cerr<<"Error: This file was populated before: "<<traceName<<endl;
      }
      else if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-compute") == NULL)) //avoid dir entries: .&..
        cerr<<"Error: there is a unmatching file "<<ent->d_name<<" in "<<dirName<<endl;
    }
    closedir(dir);
  } 
  else 
    cerr<<"Error: couldn't open the directory "<<dirName<<endl;

  dirName = "PRESPAWN";
  if ((dir = opendir(dirName.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-preSpawn") != NULL)) {
        traceName = dirName + "/" + std::string(ent->d_name);
        pos = traceName.find("compute");
        nodeName = traceName.substr(pos);

        InstanceHash pre_instances;
        populatePreSamples(pre_instances, traceName, nodeName);
        if (pre_instance_table.count(nodeName) == 0)
          pre_instance_table[nodeName] = pre_instances; //push this node's presample
                                                        //info to the table
        else
          cerr<<"Error: This file was populated before: "<<traceName<<endl;
      }
      else if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-compute") == NULL)) //avoid dir entries: .&..
        cerr<<"Error: there is a unmatching file "<<ent->d_name<<" in "<<dirName<<endl;
    }
    closedir(dir);
  } 
  else 
    cerr<<"Error: couldn't open the directory "<<dirName<<endl;

  dirName = "FORK";
  if ((dir = opendir(dirName.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-fork") != NULL)) {
        traceName = dirName + "/" + std::string(ent->d_name);
        pos = traceName.find("compute");
        nodeName = traceName.substr(pos);
        // create a new instance vector if it's from new node
        if (fork_instance_table.count(nodeName) == 0) {
          vector<Instance> fork_instances;
          populateForkSamples(fork_instances, traceName, nodeName);
          fork_instance_table[nodeName] = fork_instances; 
        }
        else { //node's file existed before, for fork_nb, fork_fast files
          vector<Instance> &fork_instances = fork_instance_table[nodeName];
          populateForkSamples(fork_instances, traceName, nodeName);
        }
      }
      else if ((ent->d_type == DT_REG) && 
          (strstr(ent->d_name, "Input-compute") == NULL)) //avoid dir entries: .&..
        cerr<<"Error: there is a unmatching file "<<ent->d_name<<" in "<<dirName<<endl;
    }
    closedir(dir);
  } 
  else 
    cerr<<"Error: couldn't open the directory "<<dirName<<endl;

  cout<<"Done populating all Input- files"<<endl;
}


// <altMain> <exe Name(not used)> <config file name> 
int main(int argc, char** argv)
{ 
  if (argc <3){
      std::cerr<<"Wrong Number of Arguments! "<<argc<<std::endl;
      exit(0);
  }
  
  bool verbose = false;
  
  fprintf(stderr,"START - ");
  my_timestamp();
  
  BlameProgram bp;
  bp.parseConfigFile(argv[2]);
  
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
  bp.grabUsedModulesFromDir();
  
  //std::cout<<"Parsing program"<<std::endl;
  bp.parseProgram();
  
  //std::cout<<"Adding implicit blame points."<<std::endl;
  bp.addImplicitBlamePoints();
  
  //std::cout<<"Printing parsed output to 'output.txt'"<<std::endl;
  //ofstream outtie("output.txt");
  //bp.printParsed(outtie);
  

  //bp.calcSideEffects();
  //std::cout<<"Populating samples."<<std::endl;
  fprintf(stderr,"SAMPLES - ");
  my_timestamp();

  stack_info.open("stackDebug", ofstream::out);
  compInstanceHash  comp_instance_table;
  preInstanceHash  pre_instance_table; //Added by Hui 12/25/15
  forkInstanceHash fork_instance_table; //Added by Hui 11/16/16

  // Import sample info from all Input- files
  populateSamplesFromDirs(comp_instance_table, pre_instance_table, 
                                                fork_instance_table);
  
  int validInstances = 0;

  //Trim pre_instances frames first  
  preInstanceHash::iterator ph_i;
  InstanceHash::iterator ih_i;
  //ph_i: <string nodeName, InstanceHash pre_hash>
  for (ph_i=pre_instance_table.begin(); ph_i!=pre_instance_table.end(); ph_i++) {
    InstanceHash &pre_hash = (*ph_i).second;
    //ih_i: <int processTLNum, Instance inst>
    int iCounter = 0;
    for (ih_i=pre_hash.begin(); ih_i!=pre_hash.end(); ih_i++) {
      (*ih_i).second.trimFrames(bp.blameModules, iCounter, (*ph_i).first);
      iCounter++;
    }
  }

  //Trim fork_instances frames 
  forkInstanceHash::iterator fh_i;
  vector<Instance>::iterator vec_I_i;
  //fh_i: <string nodeName, vector<Instance> fork_vec>
  for (fh_i=fork_instance_table.begin(); fh_i!=fork_instance_table.end(); fh_i++) {
    vector<Instance> &fork_vec = (*fh_i).second;
    int iCounter = 0;
    for (vec_I_i=fork_vec.begin(); vec_I_i!=fork_vec.end(); vec_I_i++) {
      (*vec_I_i).trimFrames(bp.blameModules, iCounter, (*fh_i).first);
      iCounter++;
    }
  }

  //Trim comp_instances frames
  compInstanceHash::iterator ch_i; //vec_I_i can be reused from above
  //fh_i: <string nodeName, vector<Instance> fork_vec>
  for (ch_i=comp_instance_table.begin(); ch_i!=comp_instance_table.end(); ch_i++) {
    vector<Instance> &comp_vec = (*ch_i).second;
    int iCounter = 0;
    for (vec_I_i=comp_vec.begin(); vec_I_i!=comp_vec.end(); vec_I_i++) {
      (*vec_I_i).trimFrames(bp.blameModules, iCounter, (*ch_i).first);
      iCounter++;
    }
  }

  //Now we need to glue those stack traces, we use a lazy way: 
  //Follow this order: comp->pre->fork->(fork/pre) until to top MAIN frame
  for (ch_i=comp_instance_table.begin(); ch_i!=comp_instance_table.end(); ch_i++) {
    vector<Instance> &comp_vec = (*ch_i).second;
    string node = (*ch_i).first;
    string outName = "PARSED_" + node;
    ofstream gOut(outName.c_str());
    int iCounter = 0;

    for (vec_I_i=comp_vec.begin(); vec_I_i!=comp_vec.end(); vec_I_i++) {
      gOut<<"---INSTANCE "<<iCounter<<"  ---"<<std::endl;

      // glue stacktraces whenever necessary !
      if ((*vec_I_i).isMainThread == false) 
        glueStackTraces(node, iCounter, (*vec_I_i),
                        pre_instance_table, fork_instance_table);

      //TODO:Polish stacktrace again: rm chpl_gen_main for node0, (same line&file diff fName)frames

      // concise_print the final "perfect" stacktrace for each compute sample
      stack_info<<"NOW final stacktrace for inst#"<<iCounter<<" on "<<node<<endl;
      (*vec_I_i).printInstance_concise();

      // handle the perfect instance now !
      (*vec_I_i).handleInstance(bp.blameModules, gOut, iCounter, verbose);
    
      gOut<<"$$$INSTANCE "<<iCounter<<"  $$$"<<std::endl; 
      iCounter++;
    }
  }
      
//    ////for testing/////////////////////
//    if((*vec_I_i).frames.size() !=0)
//      validInstances++;
//    //////////////////////////////////
//  
//  fprintf(stderr, "#total instances = %d, #validInstances = %d\n",iCounter,validInstances);
  fprintf(stderr,"DONE - ");
  my_timestamp();
 
}
