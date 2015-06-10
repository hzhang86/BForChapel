/*
 *  ExitVars.h
 *  
 *
 *  Created by Hui Zhang on 03/20/15.
 *  Previous contribution by Nick Rutar
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _EXIT_VARS_H
#define _EXIT_VARS_H

#include <string>
#include <vector>
#include <set>
#include "NodeProps.h"

class NodeProps;

using namespace std;


enum ExitTypes { PARAM, RET, GLOBAL, STATIC, UNDEFINED };

enum ExitProgramTypes { UNREAD_RET, BLAME_HOLDER };

class ExitSuper
{
public:
    void addVertex(NodeProps * p) {vertices.push_back(p);}
    std::string realName;
	NodeProps *vertex;
    // TODO: Change this to a set
    std::vector<NodeProps *> vertices;
    //std::set<int> lineNumbers;
	int lineNum;
};


class ExitOutput : public ExitSuper
{
public:
	ExitOutput() {lineNum = 0; vertex=NULL;}
	~ExitOutput() { vertices.clear(); }

};

class ExitVariable : public ExitSuper
{
public: 
    ExitVariable(std::string name){realName = name; lineNum = 0; vertex=NULL;}
    ExitVariable(std::string name, ExitTypes e, int wp, bool isSP) 
        {realName = name; et = e; whichParam = wp; lineNum = 0; vertex=NULL; isStructPtr = isSP;}
	~ExitVariable() { vertices.clear(); }
    ExitTypes et;
    int whichParam;
	bool isStructPtr;
};

class ExitProgram : public ExitSuper
{
public: 
    ExitProgram(std::string name){realName = name; lineNum = 0;}
    ExitProgram(std::string name, ExitProgramTypes e) 
		{realName = name; et = e; lineNum=0; vertex=NULL;}
	~ExitProgram() {  vertices.clear(); }
    ExitProgramTypes et;
    bool isUnusedReturn;
  
    // For cases where we have a single Vertex it refers to (as opposed to vertices)
    NodeProps * pointsTo;
};

#endif
