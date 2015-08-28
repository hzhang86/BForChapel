package blame;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.StringTokenizer;


public class BlameDataTrunc extends BlameData {

	//private int instanceCount;
	
	
	
	public BlameDataTrunc(String outputFile, BlameContainer bc)
	{
		super(outputFile,bc);
	}
	
	public int numInstances()
	{
		//System.out.println("Num Instances in BlameDataTrunc for " + nodeName + " is " + instanceCount);
		return instanceCount;
	}
	
	public void print()
	{
		System.out.println("We have " + numInstances() + "instances");
	}
	
	

	
	public String parseFrame(BufferedReader bufReader, Instance currInst, String line)
	{
        //////////////////////////////////////////
        System.out.println("parseFrame in BlameDataTrunc Called !");
		String pathTokens[] = line.split("\\s");
		//String strFrameNum = pathTokens[1];
		//int frameNum = Integer.valueOf(strFrameNum).intValue();
		String funcName = pathTokens[2];
        /////////////added by HUI 08/23/15////////////////
        String cleanFuncName = funcName.replaceAll("chpl_user_main","main");
        funcName = cleanFuncName.replace("_chpl","");
        //////////////////////////////////////////////////
		String moduleName = pathTokens[3];
		String pathName = pathTokens[4];
		//String strLineNum  = pathTokens[5];
		//int lineNumber = Integer.valueOf(strLineNum).intValue();
		String strBPType   = pathTokens[6];
		short bptType = Short.valueOf(strBPType).shortValue();
		String strFBeginLine = pathTokens[7];
		int beginLine = Integer.valueOf(strFBeginLine).intValue();
		String strFEndLine = pathTokens[8];
		int endLine = Integer.valueOf(strFEndLine).intValue();
        
		
		BlameFunction bf = bc.getOrCreateBlameFunction(funcName, moduleName, bptType);
		
		bf.setBeginNum(beginLine);
		bf.setEndNum(endLine);
		
		if (bptType > BlameFunction.NO_BLAME_POINT)
			bc.addBlamedFunction(funcName,bf);
		
		//SourceFiles sourceF = bc.getOrCreateSourceFile();
		
	
		
		if (SourceContainer.sourceFiles.get(moduleName) == null)
		{
			SourceFile sourceF = new SourceFile();
			sourceF.setFileName(moduleName);
			sourceF.setFilePath(pathName);
			
			// TODO: 6/10/10 Make this on demand
			//sourceF.setLinesOfCode();
			SourceContainer.sourceFiles.put(moduleName, sourceF);
		}
		
		//StackFrame sf = new StackFrame(frameNum, moduleName, pathName,lineNumber, bf);
		//currInst.addStackFrame(sf);
		//sf.setParentInst(currInst);
		
		try {
				
			String evLine = bufReader.readLine();
					
			while (evLine.indexOf("FRAME#") < 0)
			{
				if (evLine.indexOf("$$INSTANCE") >= 0)
				{
					return evLine;
				}
								
				if (evLine.indexOf("***No") >= 0)
				{
					evLine = bufReader.readLine();
					continue;
				}
					
				
				/*
				// Combo of Exit Variable Type and Exit Variable designators
				
				// Exit Variable Types
				// EV (Exit Variable)
				// EO (Exit Output)
				// EF (Exit Field)
				// EDF (Exit Derived Field)
				// LF (Local Field)
				// LDF (Local Derived Field)
				// VFL (Ignored Field Local)
				// VL  (Ignored Local)
				
				// Exit Variable Designators 
				//1(11) - EV lineNumbers
				//2(12) - EV declaredLine
				//3(13) - EV tempLine
				//4(14) - EV findBlamedExits
				//5(15) - BP lineNumbers
				//6(16) - BP declaredLine
				//7(17) - BP tempLine
				//8(18) - BP findBlamedExits

				//21(31) - LV lineNumbers
				//22(32) - LV declaredLine
				//23(33) - LV tempLine
				//24(34) - LV findBlamedExits

				//40(50) - SE EV seLineNumbers
				//41(51) - SE BP seLineNumbers
				//42(52) - SE LV seLineNumbers

				//61-64 - DQ EV

				//81(91) - SE Alias
				//82(92) - SE Relation

				//90 - Derived
				 */
				
				
				StringTokenizer st = new StringTokenizer(evLine);
				
				String[] esTokens = new String[st.countTokens()];
				
				
				int counter = 0;
				while (st.hasMoreTokens())
				{
					esTokens[counter] = st.nextToken();
					counter++;
				}
				
				if (counter < 4)
				{
					System.err.println("Not enough info for line " + evLine);
					System.exit(1);
				}
				
				// EV Type concatenated with EV designator
				String strEVType = esTokens[0];
				
				// Full Name
				String strVarName = esTokens[1];
				/////added by HUI 08/23/15////////////////
                strVarName = strVarName.replaceAll("_chpl","");
                /////////////////////////////////////////
			
				// Type Name
				String strTypeName = esTokens[2];

				// Struct Name (may be NULL)
				String structName = esTokens[3];
				/////added by HUI 08/23/15////////////////
                structName = structName.replaceAll("_chpl","");
                /////////////////////////////////////////
				
                //bc.addType(strTypeName, structName);
						
				ExitSuper es = new ExitSuper();
				
				if (strEVType.indexOf("EV") >= 0)
				{
					ExitVariable ev = bf.getOrCreateEV(strVarName);
					es = ev;
				}
				else if (strEVType.indexOf("EO") >= 0)
				{
					ExitOutput eo = bf.getOrCreateEO(strVarName);
					es = eo;
				}
				else if (strEVType.indexOf("U") >= 0)
				{
					ExitProgram ep = bf.getOrCreateEP(strVarName);
					es = ep;
				}
				else if (strEVType.indexOf("EF") >= 0 || strEVType.indexOf("EDF") >= 0)
				{
					ExitVariable evf = bf.getOrCreateEVField(strVarName);
					ExitVariable fieldParent;
					es = evf;
					
					while (strVarName.indexOf('.') != strVarName.lastIndexOf('.'))
					{
						String newStrVarName = strVarName.substring(0,strVarName.lastIndexOf('.'));
						
						if (strVarName.indexOf('.') == strVarName.lastIndexOf('.'))
						{
							fieldParent = bf.getOrCreateEV(newStrVarName);
						}
						else
						{
							fieldParent = bf.getOrCreateEVField(newStrVarName);
						}
						
						fieldParent.addField(evf);	
						
						//if (fieldParent.getLastInst() != currInst)
							//fieldParent.addInstanceTrunc(currInst);
						

						strVarName = newStrVarName;
						
					}
				}
				else if (strEVType.indexOf("LF") >= 0 || strEVType.indexOf("LDF") >= 0)
				{
					ExitProgram epf = bf.getOrCreateEPField(strVarName);
					ExitProgram fieldParent;
					es = epf;
					
					while (strVarName.indexOf('.') > 0)
					{
						String newStrVarName = strVarName.substring(0,strVarName.lastIndexOf('.'));
						
						if (newStrVarName.indexOf('.') < 0)
						{
							fieldParent = bf.getOrCreateEP(newStrVarName);
						}
						else
						{
							fieldParent = bf.getOrCreateEPField(newStrVarName);
						}
						
						fieldParent.addField(epf);	
						
						//if (fieldParent.getLastInst() != currInst)
							//fieldParent.addInstanceTrunc(currInst);
						
						
						strVarName = newStrVarName;
						epf = fieldParent;
					}
				}
				
				else if (strEVType.indexOf("VL") >= 0)
				{
					ExitProgram ep = bf.getOrCreateEP(strVarName);
					es = ep;
				}				
				else if (strEVType.indexOf("VFL") >= 0 )//|| strEVType.indexOf("VL") >= 0)
				{
					
					ExitProgram epf = bf.getOrCreateEPField(strVarName);
					ExitProgram fieldParent;
					es = epf;
					
					while (strVarName.indexOf('.') > 0)
					{
						String newStrVarName = strVarName.substring(0,strVarName.lastIndexOf('.'));
						
						if (newStrVarName.indexOf('.') < 0)
						{
							fieldParent = bf.getOrCreateEP(newStrVarName);
						}
						else
						{
							fieldParent = bf.getOrCreateEPField(newStrVarName);
						}
						
						fieldParent.addField(epf);	
						
						//if (fieldParent.getLastInst() != currInst)
							//fieldParent.addInstanceTrunc(currInst);
												
						strVarName = newStrVarName;
						epf = fieldParent;

					}
				}
				else
				{
				    evLine = bufReader.readLine();
				    continue;
				}
				
				/////////////////////HUI//////////////////////////////////
				System.out.println("Setting GT " + strTypeName + " ST " + structName + " for " + es.getName());
				es.setGenType(strTypeName);
				es.setStructType(structName);
				es.setType();
				bc.addType(es.getType());
	
							
				if (es.getLastInst() != currInst)
					es.addInstanceTrunc(currInst);
				
				// We set this to make sure we don't have any repeats
				es.setLastInst(currInst);

				evLine = bufReader.readLine();
			}
			
			
			return evLine;
			
		}
		catch (IOException e) {
			System.err.println("exception happened - here's what I know: ");
			e.printStackTrace();
			System.exit(-1);
		}
		
		return new String();
	}
	

	public void parseOutputFile(String outputFile, String nodeName)
	{				
		File f = new File(outputFile);
		
		try {	
			//System.out.println("Read in file " + outputFile);
			BufferedReader bufReader = new BufferedReader(new FileReader(f));
			String line = null;
			
			//int instanceCount = 0;
			
			while ((line = bufReader.readLine()) != null) 
			{				
				Instance currInst;
					
				if (line.indexOf("--INSTANCE") >= 0) 
				{
					currInst = new Instance(this.instanceCount, nodeName);
					this.instanceCount++;
					int sfCount = 0;
					
					line = bufReader.readLine();
					
					// We are dealing with a FRAME
					while (line.indexOf("$$INSTANCE") < 0)
					{
						line = parseFrame(bufReader, currInst, line);
						sfCount++;
					}
					
					if (sfCount == 0)
					{	
						short bpType = BlameFunction.EXPLICIT_BLAME_POINT;
						BlameFunction bf = bc.getOrCreateBlameFunction("UNRESOLVED", "NO_MODULE", bpType);
						
						bc.addBlamedFunction("UNRESOLVED",bf);
						
						ExitProgram ep = bf.getOrCreateEP("INCOMPLETE_STACK_WALK");
						Double value = ep.blameByNode.get(currInst.getNodeName());
						if (value == null)
						{
							ep.blameByNode.put(currInst.getNodeName(), 1.0);
						}
						else
						{
							value++;
							ep.blameByNode.put(currInst.getNodeName(), value);
						}		
						//ep.addInstance(currInst, null, bf);						
					}
				}
			}
				
		}
		catch (IOException e) {
			System.err.println("exception happened - here's what I know: ");
			e.printStackTrace();
			System.exit(-1);
		}
	}
	
	
}
