#include "BFC.h"
#include "ModuleBFC.h"
#include "NodeProps.h"
#include "FunctionBFC.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void importSharedBlame(const char *path, ExternFuncBlameHash &externFuncInformation)
{
    ifstream bI(path);
    string line;

    while (getline(bI, line)) {
        string buf;
        string name;
        stringstream ss(line);

        ss >> buf;
        name = buf; //assign name of the func

        ExternFunctionBlame *ef = new ExternFunctionBlame(name);

        while (ss >> buf) {
            ef->paramNums.insert(atoi(buf.c_str()));
        }
        externFuncInformation[name.c_str()] = ef;
    }
}

/* Main pass that takes in a module and runs analysis on each function */
bool BFC::runOnModule(Module &M)
{
    cerr<<"IN run on module"<<endl;
    ModuleBFC *bm = new ModuleBFC(&M); //address of a ref=address of the 
					   //address of the object it refers to

    ////////////////// GLOBAL VARIABLES ////////////////////////////////////////////
    vector<NodeProps *> globalVars;
    int globalNumber = 0; //global variable index

/*    for (Module::global_iterator gI = M.getGlobalList().begin(), 
        gE = M.getGlobalList().end(); gI != gE; ++gI)
    {
	    GlobalVariable *gv = gI;
#ifdef DEBUG_P
        cout<<"Processing Global Variable: "<<(gv->getName()).str()<<endl;
#endif
	    if (gv->hasName() && !gv->isConstant()) { //don't bother constants
#ifdef DEBUG_P
	        cout<<"GV: "<<(gv->getName()).str()<<" is real gv to be stored"<<endl;
#endif
            if (gv->hasInitializer()) {
                NodeProps* v = new NodeProps(globalNumber, gv->getName(), 0, gv->getInitializer());
                globalNumber++;
                globalVars.push_back(v);
            }
            else {
                NodeProps* v = new NodeProps(globalNumber, gv->getName(), 0, NULL);
                globalNumber++;
                globalVars.push_back(v);
            }
	    }
    }
*/
    Finder.processModule(M);//That's it, all DIDescriptors are stored in Vecs:
			                //CUs, SPs, GVs, TYs
    cout<<"Static Analyzing DIGlobalVariables"<<endl;
    for (DebugInfoFinder::iterator I = Finder.global_variable_begin(),
	    E = Finder.global_variable_end(); I != E; I++) {

        DIGlobalVariable *dgv = new DIGlobalVariable(*I);

        if (dgv->getDirectory().equals(StringRef(PRJ_HOME_DIR))) {
        //TODO: Distinguish user funcs from library funcs in the same dir
#ifdef DEBUG_P
            cout<<"Same Path DIGlobalVariable: "<<dgv->getName().str()<<endl;
#endif
            GlobalVariable *gv = dgv->getGlobal();
            if (gv->hasName() && !gv->isConstant()) { //don't bother constants
#ifdef DEBUG_P
                cout<<"GV: "<<(gv->getName()).str()<<" is real gv to be stored"<<endl;
#endif
                if (gv->hasInitializer()) {
                    NodeProps *v = new NodeProps(globalNumber, gv->getName(), 0, gv->getInitializer());
                    globalNumber++;
                    globalVars.push_back(v);
                }
                else {
                    NodeProps *v = new NodeProps(globalNumber, gv->getName(), 0, NULL);
                    globalNumber++;
                    globalVars.push_back(v);
                }
            }
        }
    }

    cout<<"Static Analyzing DITypes"<<endl;
    for (DebugInfoFinder::iterator I = Finder.type_begin(),
	    E = Finder.type_end(); I != E; I++) {
        
        DIType *dt = new DIType(*I); //create an instance of DIType, not a ptr
        
        if (dt->getDirectory().equals(StringRef(PRJ_HOME_DIR))) {
#ifdef DEBUG_P
            cout<<"Same Path DIType: "<<dt->getName().str()<<endl;
#endif
            
            bm->parseDITypes(dt);
        }
    }

//  Make records of the information of external library functions  //
    ExternFuncBlameHash externFuncInformation;
    
    importSharedBlame("/export/home/hzhang86/BForChapel/BFC_StaticAnalysis/SHARED/mpi.bs", externFuncInformation);
    importSharedBlame("/export/home/hzhang86/BForChapel/BFC_StaticAnalysis/SHARED/fortran.bs", externFuncInformation);
    importSharedBlame("/export/home/hzhang86/BForChapel/BFC_StaticAnalysis/SHARED/cblas.bs", externFuncInformation);
//  NOT SURE THE ABOVE IS NEEDED FOR Chapel Program //

    // SETUP all the exports files 
    std::string blame_path("EXPORT/");
	std::string struct_path = blame_path;
	std::string calls_path = blame_path;
	std::string params_path = blame_path;
	std::string se_path = blame_path;
	std::string sea_path = blame_path;
	std::string alias_path = blame_path;
	std::string loops_path = blame_path;
	std::string conds_path = blame_path;
	
	std::string mod_name = M.getModuleIdentifier();
	std::string blame_extension(".blm");
	blame_path += mod_name;
	blame_path += blame_extension;
	std::ofstream blame_file(blame_path.c_str());
	
	std::string se_extension(".se");
	se_path += mod_name;
	se_path += se_extension;
	std::ofstream blame_se_file(se_path.c_str());
	
	std::string struct_extension(".structs");
	struct_path += mod_name;
	struct_path += struct_extension;
	std::ofstream struct_file(struct_path.c_str());
	
	std::string calls_extension(".calls");
	calls_path += mod_name;
	calls_path += calls_extension;
	std::ofstream calls_file(calls_path.c_str());
	
	std::string params_extension(".params");
	params_path += mod_name;
	params_path += params_extension;
	std::ofstream params_file(params_path.c_str());
	
	
	bm->exportStructs(struct_file);
   
    cout<<"Static Analyzing DISubprograms"<<endl;
    /////////////////// PRE PASS ////////////////////////////////////
    set<const char*> knownFuncNames;

    for (DebugInfoFinder::iterator I = Finder.subprogram_begin(),
	    E = Finder.subprogram_end(); I != E; I++) {

        DISubprogram *dsp = new DISubprogram(*I);
        
        if (dsp->getDirectory().equals(StringRef(PRJ_HOME_DIR))) {

            Function *F = dsp->getFunction();
#ifdef DEBUG_P
            cout<<"Same directory of DISP: "<<dsp->getName().str()<< \
                " is "<<dsp->getDirectory().str()<<endl;
            cout<<"Corresponding Function is: "<<F->getName().str()<<endl;
#endif
            if (F->isDeclaration()) //Function: public GlobalValue->isDeclaration
                continue;
            else 
                knownFuncNames.insert(F->getName().str().c_str());
        }
    }


    //////////////// FIRST PASS ////////////////////////////////////
       
    FuncBlameHash funcInformation;

    int numMissing = 0;
    int numMEV = 0;
    int numMEV2 = 0;
    int numMEV3 = 0;

    //Generate relationships from first pass over functions
    for (DebugInfoFinder::iterator I = Finder.subprogram_begin(),
	    E = Finder.subprogram_end(); I != E; I++) {

        DISubprogram *dsp = new DISubprogram(*I);
        
        if (dsp->getDirectory().equals(StringRef(PRJ_HOME_DIR))) {

            Function *F = dsp->getFunction();
            FunctionBlame *fb;

            if (F->isDeclaration())
                continue;
            else {
                // Run regular blame calculation
                fb = new FunctionBlame(F, knownFuncNames);
                fb->setModule(&M);
                fb->setModuleBlame(bm);

                calls_file<<"FUNCTION "<<F->getName().str()<<endl;
                fb->firstPass(F, globalVars, externFuncInformation, 
                    blame_file, blame_se_file, calls_file, numMissing);
                calls_file<<"END FUNCTION "<<endl;

                int numMEVL = 0;
                int numMEV2L = 0;
                int numMEV3L = 0;
                
                fb->moreThanOneEV(numMEVL, numMEV2L, numMEV3L);

                numMEV += numMEVL;
                numMEV2 += numMEV2L;
                numMEV3 += numMEV3L;

                fb->exportParams(params_file);
                params_file<<"NEVs - "<<numMEVL<<" "<<numMEV2L<<" "<<numMEV3L<< \
                    " out of "<<knownFuncNames.size()<<endl;
                params_file<<endl;

                delete(fb);
            }
        }
    }

    params_file<<"NEVs - "<<numMEV<<" "<<numMEV3<<" "<<numMEV2<<" out of "<< \
        knownFuncNames.size()<<endl;


    return false;
}
