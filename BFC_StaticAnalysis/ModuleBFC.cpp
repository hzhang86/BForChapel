/*
 *  ModuleBFC.cpp
 *  
 *  Implementation of Module-level LLVM pass
 *  Created by Hui Zhang on 2/8/15.
 *  Copyright 2015 __MyCompanyName__. All rights reserved.
 *
 */

//#include "FunctionBFC.h"
#include "ModuleBFC.h"

#include <iostream>

using namespace std;


/*
std::string getStringFromMD(Value * v)
{
	std::string fail("---");
	
	if (v == NULL)
		return fail;
	
	if (isa<ConstantExpr>(v))
	{
		ConstantExpr * ce = cast<ConstantExpr>(v);
				
		User::op_iterator op_i = ce->op_begin();
		for (; op_i != ce->op_end(); op_i++)
		{
			Value * stringArr = *op_i;			
						
			if (isa<GlobalValue>(stringArr))
	    {
	      GlobalValue * gv = cast<GlobalValue>(stringArr);		
	      User::op_iterator op_i2 = gv->op_begin();
	      for (; op_i2 != gv->op_end(); op_i2++)
				{
					Value * stringArr2 = *op_i2;
					
					if (isa<ConstantArray>(stringArr2))
					{
						ConstantArray * ca = cast<ConstantArray>(stringArr2);
						if (ca->isString())
						{
							std::string rawRet = ca->getAsString();
							
							// Need to get rid of the trailing NULL
							return rawRet.substr(0, rawRet.length()-1);
						}
					}		
				}
	    }
		}
	}	
	return fail;
}


void getTypeFromBasic(Value * v, std::string & typeName)
{
	
	if (v->getName().find("derivedtype.type") != std::string::npos)
	{
		//std::cout<<"Leaving getTypeFromDerivedType (1)"<<std::endl;
		return;
	}
	
	if (!isa<GlobalVariable>(v))
	{
		//std::cout<<"Leaving getTypeFromDerivedType (2)"<<std::endl;
		return;
	}
	
	GlobalVariable * gv = cast<GlobalVariable>(v);
	
	User::op_iterator op_i2 = gv->op_begin();
	for (; op_i2 != gv->op_end(); op_i2++)
	{
		Value * structOp = *op_i2;			
		//std::cout<<"StructOp Name(getTypeFromDerivedType) - "<<structOp->getName()<<std::endl;
		
		if (isa<ConstantStruct>(structOp))
		{
			ConstantStruct * csOp = cast<ConstantStruct>(structOp);
			
			if (csOp->getNumOperands() < 9)
			{
				//std::cout<<"Leaving getTypeFromDerivedType (3)  "<<csOp->getNumOperands()<<std::endl;
				return;
			}
			
			
			std::string maybeName = getStringFromMD(csOp->getOperand(2));
			//std::cout<<"Maybe name (Basic) is "<<maybeName<<std::endl;
			typeName += maybeName;
			
		}
	}
	
	return;
}



void getTypeFromComposite(Value * v, std::string & typeName)
{
	
	return;
}


void getTypeFromDerived(Value* v, std::string & typeName)
{
	if (v->getName().find("derivedtype.type") != std::string::npos)
	{
		//std::cout<<"Leaving getTypeFromDerivedType (1)"<<std::endl;
		return;
	}
	
	if (!isa<GlobalVariable>(v))
	{
		//std::cout<<"Leaving getTypeFromDerivedType (2)"<<std::endl;
		return;
	}
	
	GlobalVariable * gv = cast<GlobalVariable>(v);
	
	User::op_iterator op_i2 = gv->op_begin();
	for (; op_i2 != gv->op_end(); op_i2++)
	{
		Value * structOp = *op_i2;			
		//std::cout<<"StructOp Name(getTypeFromDerivedType) - "<<structOp->getName()<<std::endl;
		
		if (isa<ConstantStruct>(structOp))
		{
			ConstantStruct * csOp = cast<ConstantStruct>(structOp);
			
			if (csOp->getNumOperands() < 9)
			{
				//std::cout<<"Leaving getTypeFromDerivedType (3)  "<<csOp->getNumOperands()<<std::endl;
				return;
			}
			
			
			std::string maybeName = getStringFromMD(csOp->getOperand(2));
			//std::cout<<"Maybe name(Derived) is "<<maybeName<<std::endl;
			
			
			// We found a legit type name
			if (maybeName.find("---") == std::string::npos)
			{
				typeName += maybeName;
				return;
			}
			
			// We aren't at the end of the rabbit hole quite yet, have
			//  to get through some pointers first
			typeName.insert(0, "*");
			
			
			Value * v2 = csOp->getOperand(9);
			
			if (!isa<ConstantExpr>(v2))
			{
				//std::cout<<"Leaving getTypeFromDerived (4)"<<std::endl;
				std::string voidStr("VOID");
				typeName += voidStr;
				return;
			}
			
			ConstantExpr * ce = cast<ConstantExpr>(v2);
			User::op_iterator op_i = ce->op_begin();
			
			for (; op_i != ce->op_end(); op_i++)
			{
				
				Value * bitCastOp = *op_i;
				if (bitCastOp == NULL)
					return;
				
				//std::cout<<std::endl<<"bitCastOp (getTypeFromMD)- "<<bitCastOp->getName()<<std::endl;
				
				
				if (bitCastOp->getName().find("derived") != std::string::npos)
					getTypeFromDerived(bitCastOp, typeName);
				else if (bitCastOp->getName().find("basic") != std::string::npos)
					getTypeFromBasic(bitCastOp, typeName);
				else if (bitCastOp->getName().find("composite") != std::string::npos)
					getTypeFromComposite(bitCastOp, typeName);			
				
			}
		}
	}
	return;
}



std::string getTypeFromMD(Value *v)
{
	std::string noIdea("NO_IDEA");
	
	if (v == NULL)
		return noIdea;
	
	
	//std::cout<<"(getTypeFromMD) v Name -- "<<v->getName()<<std::endl;
	
	if (!isa<ConstantExpr>(v))
	{
		//std::cout<<"Leaving getTypeFromMD (2)"<<std::endl;
		return noIdea;
	}
	
  ConstantExpr * ce = cast<ConstantExpr>(v);
  User::op_iterator op_i = ce->op_begin();
	
	for (; op_i != ce->op_end(); op_i++)
	{
		
		Value * bitCastOp = *op_i;
		if (bitCastOp == NULL)
			return noIdea;
		
		//std::cout<<std::endl<<"bitCastOp (getTypeFromMD)- "<<bitCastOp->getName()<<std::endl;
		
		std::string typeName;
		
	  if (bitCastOp->getName().find("derived") != std::string::npos)
			getTypeFromDerived(bitCastOp, typeName);
		else if (bitCastOp->getName().find("basic") != std::string::npos)
			getTypeFromBasic(bitCastOp, typeName);
		else if (bitCastOp->getName().find("composite") != std::string::npos)
			getTypeFromComposite(bitCastOp, typeName);
		
		return typeName;
		
		//if (bitCastOp->getName().find("composite") == std::string::npos)
		//	return NULL;
	}
	
	return noIdea;
}




void StructBFC::setModuleName(std::string rawName)
{
	moduleName = rawName.substr(0, rawName.length() - 1);
	
}

void StructBFC::setModulePathName(std::string rawName)
{
	modulePathName = rawName.substr(0, rawName.length() - 1);
}
*/

void ModuleBFC::exportStructs(std::ostream &O)
{
	std::vector<StructBFC *>::iterator vec_sb_i;
	
	for (vec_sb_i = structs.begin(); vec_sb_i != structs.end(); vec_sb_i++) {
		StructBFC * sb = (*vec_sb_i);
		O<<"BEGIN STRUCT"<<endl;
		
		O<<"BEGIN S_NAME "<<endl;
		O<<sb->structName<<endl;
		O<<"END S_NAME "<<endl;
		
		O<<"BEGIN M_PATH"<<endl;
		O<<sb->modulePathName<<std::endl;
		O<<"END M_PATH"<<endl;
		
		O<<"BEGIN M_NAME"<<endl;
		O<<sb->moduleName<<std::endl;
		O<<"END M_NAME"<<endl;
		
		O<<"BEGIN LINENUM"<<endl;
		O<<sb->lineNum<<std::endl;
		O<<"END LINENUM"<<endl;
		
		O<<"BEGIN FIELDS"<<endl;
		std::vector<StructField *>::iterator vec_sf_i;
		for (vec_sf_i = sb->fields.begin(); vec_sf_i != sb->fields.end(); vec_sf_i++) {
			O<<"BEGIN FIELD"<<endl;
			StructField * sf = (*vec_sf_i);
			if (sf == NULL) {
				O<<"END FIELD"<<endl;
				continue;
			}
			O<<"BEGIN F_NUM"<<endl;
			O<<sf->fieldNum<<endl;
			O<<"END F_NUM"<<endl;
			
			O<<"BEGIN F_NAME"<<endl;
			O<<sf->fieldName<<endl;
			O<<"END F_NAME"<<endl;
			
			O<<"BEGIN F_TYPE"<<endl;
			O<<sf->typeName<<endl;
			O<<"END F_TYPE"<<endl;
			
			O<<"END FIELD"<<endl;
		}
		
		O<<"END FIELDS"<<endl;
		
		O<<"END STRUCT"<<endl;
	}
	
	O<<"END STRUCTS"<<endl;
}


std::string returnTypeName(const llvm::Type * t, std::string prefix)
{
	if (t == NULL)
		return prefix += std::string("NULL");
	
    unsigned typeVal = t->getTypeID();
    if (typeVal == Type::VoidTyID)
        return prefix += std::string("Void");
    else if (typeVal == Type::FloatTyID)
        return prefix += std::string("Float");
    else if (typeVal == Type::DoubleTyID)
        return prefix += std::string("Double");
    else if (typeVal == Type::X86_FP80TyID)
        return prefix += std::string("80 bit FP");
    else if (typeVal == Type::FP128TyID)
        return prefix += std::string("128 bit FP");
    else if (typeVal == Type::PPC_FP128TyID)
        return prefix += std::string("2-64 bit FP");
    else if (typeVal == Type::LabelTyID)
        return prefix += std::string("Label");
    else if (typeVal == Type::IntegerTyID)
        return prefix += std::string("Int");
    else if (typeVal == Type::FunctionTyID)
        return prefix += std::string("Function");
    else if (typeVal == Type::StructTyID)
        return prefix += std::string("Struct");
    else if (typeVal == Type::ArrayTyID)
        return prefix += std::string("Array");
    else if (typeVal == Type::PointerTyID)
        return prefix += returnTypeName(cast<PointerType>(t)->getElementType(),
			std::string("*"));
    else if (typeVal == Type::MetadataTyID)
        return prefix += std::string("Metadata");
    else if (typeVal == Type::VectorTyID)
        return prefix += std::string("Vector");
    else
        return prefix += std::string("UNKNOWN");
}


void ModuleBFC::printStructs()
{
	std::vector<StructBFC *>::iterator vec_sb_i;
	
	for (vec_sb_i = structs.begin(); vec_sb_i != structs.end(); vec_sb_i++)	{
		std::cout<<endl<<endl;
		StructBFC * sb = (*vec_sb_i);
		std::cout<<"Struct "<<sb->structName<<std::endl;
		std::cout<<"Module Path "<<sb->modulePathName<<std::endl;
		std::cout<<"Module Name "<<sb->moduleName<<std::endl;
		std::cout<<"Line Number "<<sb->lineNum<<std::endl;
		
		std::vector<StructField *>::iterator vec_sf_i;
		for (vec_sf_i = sb->fields.begin(); vec_sf_i != sb->fields.end(); vec_sf_i++) {
			StructField * sf = (*vec_sf_i);
			if (sf == NULL)
				continue;
			std::cout<<"   Field # "<<sf->fieldNum<<" ";
			std::cout<<", Name "<<sf->fieldName<<" ";
			std::cout<<" Type "<<sf->typeName<<" ";
			//std::cout<<", Type "<<returnTypeName(sf->llvmType, std::string(" "));
			std::cout<<endl;
		}
	}
}


StructBFC * ModuleBFC::structLookUp(std::string &sName)
{
	std::vector<StructBFC *>::iterator vec_sb_i;
	for (vec_sb_i = structs.begin(); vec_sb_i != structs.end(); vec_sb_i++) {
		StructBFC * sb = (*vec_sb_i);
		
		if (sb->structName == sName)
			return sb;
	}
	
	return NULL;
}


void ModuleBFC::addStructBFC(StructBFC * sb)
{
	//std::cout<<"Entering addStructBFC"<<std::endl;
	std::vector<StructBFC *>::iterator vec_sb_i;
#ifdef DEBUG_P
    cout<<sb->structName<<": moduleName = "<<sb->moduleName<< \
        " modulePathName = "<<sb->modulePathName<<endl;
#endif

	for (vec_sb_i = structs.begin(); vec_sb_i != structs.end(); vec_sb_i++) {
		StructBFC * sbv = (*vec_sb_i);
		if (sbv->structName == sb->structName) {
			if (sbv->moduleName == sb->moduleName &&
				sbv->modulePathName == sb->modulePathName) {
				// Already have this exact struct, repeat in the linked bitcode
				delete sb;
				return;
			}
			else {
#ifdef DEBUG_ERROR			
				std::cerr<<"Two structs with same name and different declaration sites"<<std::endl;
#endif
			}
		}
	}
	
	structs.push_back(sb);
}


void StructBFC::setModuleNameAndPath(llvm::DIScope *contextInfo)
{
	if (contextInfo == NULL)
		return;

	this->moduleName = contextInfo->getFilename().str();
    this->modulePathName = contextInfo->getDirectory().str();
}


void ModuleBFC::parseDITypes(DIType* dt) //deal with derived type and composite type
{
#ifdef DEBUG_P    //stands for debug_print
    cout<<"Entering parseDITypes for "<<dt->getName().str()<<endl;
#endif	
    StructBFC * sb;
    string dtName = dt->getName().str();
    if (dtName.compare("object") && dtName.compare("super") && dtName.find("chpl")==string::npos) {
        if (dt->isCompositeType()) { //DICompositeType:public DIDerivedType
#ifdef DEBUG_P
            cout<<"Composite Type primary for: "<<dt->getName().str()<<endl;
#endif		
            sb = new StructBFC();
            if(parseCompositeType(dt, sb, true))
                addStructBFC(sb);
            else
                delete sb;
        }	
        
        else if ((dt->isDerivedType()) && (dt->getTag()!=dwarf::DW_TAG_member)) {
#ifdef DEBUG_P
            cout<<"Derived Type: "<<dt->getName().str()<<endl;
#endif		
            sb = new StructBFC();
            if(parseDerivedType(dt, sb, NULL, false))
                addStructBFC(sb);
            else
                delete sb;
            
        }	
    }
	//std::cout<<"Leaving parseStruct for "<<gv->getNameStart()<<std::endl;
}



/*
!6 = metadata !{
   i32,      ;; Tag (see below)
   metadata, ;; Reference to context
   metadata, ;; Name (may be "" for anonymous types)
   metadata, ;; Reference to file where defined (may be NULL)
   i32,      ;; Line number where defined (may be 0)
   i64,      ;; Size in bits
   i64,      ;; Alignment in bits
   i64,      ;; Offset in bits
   i32,      ;; Flags
   metadata, ;; Reference to type derived from
   metadata, ;; Reference to array of member descriptors
   i32       ;; Runtime languages
}*/
bool ModuleBFC::parseCompositeType(DIType *dt, StructBFC *sb, bool isPrimary)
{
    if (sb == NULL)
		return false;
	
	if (dt == NULL) 
		return false;
    else if (!dt->isCompositeType()) {//verify that dt is a composite type
#ifdef DEBUG_P
        cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond1"<<endl;
#endif
        return false; }
    else if (!(getDICompositeType(*dt).Verify())) {
#ifdef DEBUG_P
        cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond2"<<endl;
#endif
        return false; }
    else {
        // There are no other typedefs that would alias to this
		if (isPrimary) {
		    sb->lineNum = dt->getLineNumber();
			sb->structName = dt->getName().str();//dt->getStringField(2).str();
            sb->setModuleNameAndPath(dt);
		}
        //DICompositeType dct = dyn_cast<DICompositeType>(dt);
        DICompositeType dct = DICompositeType(*dt);
        DIArray fields = dct.getTypeArray(); // !3 = !{!1,!2}
                                                  //return the mdnode for members
        if (fields) { 
//            if(isa<MDNode>(fields)) {
//                MDNode *MDFields = cast<MDNode>(fields); //!3=!{!1,!2}
                if(fields.getNumElements()==0) {
#ifdef DEBUG_P
                    cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond3"<<endl;
#endif
                    return false; }
                //else omitted
                int numFields = fields.getNumElements();
                int fieldNum = 0;

                for (int i=0; i<numFields; i++) {
                    DIDescriptor field = fields.getElement(i);
                    Value *fieldVal = field.operator->();

                    if (isa<MDNode>(fieldVal)) {
                        MDNode *MDFieldVal = cast<MDNode>(fieldVal); //!1
                        if (MDFieldVal->getNumOperands()==0) {
#ifdef DEBUG_P
                            cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond4"<<endl;
#endif
                            return false; }

                        else { 
                            // ?? fieldContents == tag ??
                            DIDescriptor *ddField = new DIDescriptor(MDFieldVal);
                            StructField * sf = new StructField(fieldNum);
                            if (!ddField->isType()) {
#ifdef DEBUG_P
                                cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond5"<<endl;
#endif
                                return false; }
                             
                            DIType *dtField = (DIType *)ddField;

                            bool success;
                            success = parseDerivedType(dtField, NULL, sf, true);
                            if (success) {
                                sb->fields.push_back(sf);
                                sf->parentStruct = sb;
                                fieldNum++;
                            }
                            else {
#ifdef DEBUG_P
                                cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond6"<<endl;
#endif
                                delete sf;
                                return false;
                            }
                        }
                    }
                }
#ifdef DEBUG_P
                cout<<"parseCompositeType SUCCEED @ "<<dt->getName().str()<<endl;
#endif
                return true;
                            
//            }
        }
	}
#ifdef DEBUG_P
    cout<<"parseCompositeType "<<dt->getName().str()<<" failed in Cond7"<<endl;
#endif
	//std::cout<<"Leaving parseCompositeType(3)"<<std::endl;
	return false;
}





/*
!5 = metadata !{
  i32,      ;; Tag (see below)
  metadata, ;; Reference to context
  metadata, ;; Name (may be "" for anonymous types) //
  metadata, ;; Reference to file where defined (may be NULL)
  i32,      ;; Line number where defined (may be 0)
  i64,      ;; Size in bits
  i64,      ;; Alignment in bits
  i64,      ;; Offset in bits
  i32,      ;; Flags to encode attributes, e.g. private
  metadata, ;; Reference to type derived from
  metadata, ;; (optional) Name of the Objective C property associated with
            ;; Objective-C an ivar, or the type of which this
            ;; pointer-to-member is pointing to members of.
  metadata, ;; (optional) Name of the Objective C property getter selector.
  metadata, ;; (optional) Name of the Objective C property setter selector.
  i32       ;; (optional) Objective C property attributes.
}
 */
//void ModuleBFC::parseDerivedType(GlobalVariable *gv)
bool ModuleBFC::parseDerivedType(DIType *dt, StructBFC *sb, StructField *sf, bool isField)
{    
	if (dt == NULL) // both sf and sb can be NULL here
		return false;
    else if (!dt->isDerivedType()) {//verify that dt is a Derived type
#ifdef DEBUG_P
        cout<<"parseDerivedType "<<dt->getName().str()<<" failed in Cond1"<<endl;
#endif
        return false; }
    else if (!(dt->operator->()->getNumOperands() >= 10 && dt->operator->()->getNumOperands() <= 14)) {
#ifdef DEBUG_P
        cout<<"parseDerivedType "<<dt->getName().str()<<" failed in Cond2"<<endl;
#endif
        return false; }
    else {
        DIDerivedType *ddt = (DIDerivedType *)dt;
		DIType derivedFromType = ddt->getTypeDerivedFrom(); 
		
        if (isField == false) {
            if (derivedFromType.isCompositeType()) { //only care about 
                                                    //direct derived composite
                DICompositeType *dfct = (DICompositeType *) &derivedFromType; //derivedFromCompositeType -> dfct
                //TODO:replace the below func with easier method//
                //DIScope dfct_scope = dfct->getContext();
                //sb->setModuleNameAndPath(&dfct_scope);
                return parseCompositeType(dfct, sb, true);//not see a case for 
            }                                      //isPrimary=false yet
        }
        else {
            sf->fieldName = dt->getName().str();
            //DIType *dft = dyn_cast_or_null<DIType>(&derivedFromType);
            sf->typeName = derivedFromType.getName().str();//getStringField(2).str();
#ifdef DEBUG_P
            cout<<"parseDerivedType (as field) SUCCEED @name: "<<sf->fieldName \
                <<" fieldType: "<<sf->typeName<<endl;
#endif
            return true;
        }
    }
#ifdef DEBUG_P
    cout<<"parseDerivedType "<<dt->getName().str()<<" failed in Cond3"<<endl;
#endif
	//std::cout<<"Leaving parseDerivedType (4)"<<std::endl;
	return false;
}



