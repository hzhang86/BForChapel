/*
 *  Module.cpp
 *  
 *
 *  Created by Nick Rutar on 4/13/09.
 *  Copyright 2009 Nick Rutar All rights reserved.
 *
 */

#include "BlameModule.h"
#include "BlameFunction.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void BlameModule::printParsed(std::ostream &O)
{
  O<<"##Module "<<realName<<std::endl;
  //std::vector<BlameFunction *>::iterator bf_i;
  FunctionHash::iterator bf_i;
	
  for (bf_i = blameFunctions.begin();  bf_i != blameFunctions.end(); bf_i++)
	{
		BlameFunction * bf = (*bf_i).second;
		bf->printParsed(O);
	}	
}

void BlameModule::addFunctionSet(BlameFunction * bf)
{
	//cout<<"Module "<<getName()<<" ---- ";
	if (funcsBySet.count(bf) > 0)
	{
		//cout<<"Conflict: "<<funcsBySet.count(bf)<<"   "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<") -- ";
		
		set<BlameFunction *>::iterator set_bf_i;
		set_bf_i = funcsBySet.find(bf);
		BlameFunction * tempBF = *set_bf_i;
		
		//cout<<tempBF->getName()<<"("<<tempBF->getBLineNum()<<"-"<<tempBF->getELineNum()<<")"<<endl;
	
	
		int tcRange = bf->getELineNum() - bf->getBLineNum();
		int currRange = tempBF->getELineNum() - tempBF->getBLineNum();
	
		// big module from FORTRAN, probably a mistake, lets change it
		if (currRange > tcRange)
		{
			tempBF->setBLineNum(bf->getELineNum()+1);
			//cout<<"Modifying(2) "<<tempBF->getName()<<"("<<tempBF->getBLineNum()<<"-"<<tempBF->getELineNum()<<")"<<std::endl;		
			//cout<<"Inserting(2) "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<")"<<std::endl;
			addFunctionSet(bf);
		}
		else if (tcRange > currRange)
		{
			// New one is bigger, leave small one and adjust big one
			bf->setBLineNum(tempBF->getELineNum()+1);
			//cout<<"Modifying(4) "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<")"<<std::endl;		
			//cout<<"Inserting(4) "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<")"<<std::endl;
			addFunctionSet(bf);
		}
		else if (tcRange == currRange)
		{
			if (bf->allLines.size() > tempBF->allLines.size())
			{
				//cout<<"Erasing(3) "<<tempBF->getName()<<"("<<tempBF->getBLineNum()<<"-"<<tempBF->getELineNum()<<")"<<std::endl;
				funcsBySet.erase(tempBF);
				funcsBySet.insert(bf);			
				//cout<<"Inserting(3) "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<")"<<std::endl;
			}
		}
	}
	else
	{
		funcsBySet.insert(bf);			
		//cout<<"Inserting(1) "<<bf->getName()<<"("<<bf->getBLineNum()<<"-"<<bf->getELineNum()<<")"<<std::endl;
	}
}


BlameFunction * BlameModule::findLineRange(int lineNum)
{
	//cout<<"Line Number -- "<<lineNum<<"  M: "<<getName()<<" -- ";

  //FunctionHash::iterator bf_i;
	
  // Because of line number issues in FORTRAN modules, we may have
  // more than one match, we take the best one
  BlameFunction * topCand = NULL;

  dummyFunc->setBLineNum(lineNum);  //in dummyFunc, set beginLineNum = lineNum
  dummyFunc->setELineNum(lineNum);  //in dummtFunc, set endLineNum = lineNum
	
/*	
  for (bf_i = blameFunctions.begin();  bf_i != blameFunctions.end(); bf_i++)
	{
		BlameFunction * bf = (*bf_i).second;
		if (lineNum >= bf->getBLineNum() && lineNum <= bf->getELineNum())
		{
			if (topCand == NULL)
			{
				topCand = bf;
			}
			else
			{
				int tcDiff = topCand->getELineNum() - lineNum;
				int currDiff = bf->getELineNum() - lineNum;
				
				if (currDiff < tcDiff)
					topCand = bf;
				
			}
		}
	}
	*/
	
	set<BlameFunction *>::iterator set_bf_i;
	
	if (funcsBySet.count(dummyFunc))
	{
		set_bf_i = funcsBySet.find(dummyFunc); // get the iterator to the dummyFunc
		topCand = *set_bf_i;  // wouldn't topCan=dummyFunc ???
	}
	else
	{
		cout<<"Top Cand is NULL(2) -- oh noes!!!"<<endl;
		return NULL;
	}
	
	
	//if (topCand == NULL)
	//	cout<<"Top Cand is NULL -- oh noes!!!"<<endl;
	//else
	//	cout <<topCand->getName()<<"("<<topCand->getBLineNum()<<"-"<<topCand->getELineNum()<<")"<<endl;
	
	return topCand;
}



/*
BlameFunction * BlameModule::findLineRange(int lineNum)
{
  //std::vector<BlameFunction *>::iterator bf_i;
  FunctionHash::iterator bf_i;
	
  // Because of line number issues in FORTRAN modules, we may have
  // more than one match, we take the best one
  BlameFunction * topCand = NULL;
	
  for (bf_i = blameFunctions.begin();  bf_i != blameFunctions.end(); bf_i++)
	{
		BlameFunction * bf = (*bf_i).second;
		if (lineNum >= bf->getBLineNum() && lineNum <= bf->getELineNum())
		{
			if (topCand == NULL)
			{
				topCand = bf;
			}
			else
			{
				int tcDiff = topCand->getELineNum() - lineNum;
				int currDiff = bf->getELineNum() - lineNum;
				
				if (currDiff < tcDiff)
					topCand = bf;
				
			}
		}
	}
  return topCand;
}
*/
