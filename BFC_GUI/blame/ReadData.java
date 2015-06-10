package blame;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.Vector;

public class ReadData extends ParentData {
	
	
	ReadData(String outputFile, BlameContainer bc)
	{
		this.bc = bc;
		//this.nodeNum = nodeNum;
		
		nodeName = outputFile.substring(outputFile.indexOf("PARSED_") + 7);
		
		parseOutputFile(outputFile, nodeName);
	}
	
	

	
	public String parseFrame(BufferedReader bufReader, Instance currInst, String line)
	{
		String pathTokens[] = line.split("\\s");
		String strFrameNum = pathTokens[1];
		int frameNum = Integer.valueOf(strFrameNum).intValue();
		String funcName = pathTokens[2];
		String moduleName = pathTokens[3];
		String pathName = pathTokens[4];
		String strLineNum  = pathTokens[5];
		int lineNumber = Integer.valueOf(strLineNum).intValue();
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
		
		StackFrame sf = new StackFrame(frameNum, moduleName, pathName,lineNumber, bf);
		currInst.addStackFrame(sf);
		sf.setParentInst(currInst);
		
		try {
				String evLine = bufReader.readLine();
					
			while (evLine.indexOf("FRAME(OAR)#") < 0)
			{
				if (evLine.indexOf("$$INSTANCE") >= 0)
				{
					//sf.resolveFrameES();

					return evLine;
				}
								
				if (evLine.indexOf("***No") >= 0)
				{
					evLine = bufReader.readLine();
					continue;
				}
					
				//String esTokens[] = evLine.split("\\s");
				
				//int len = esTokens.length;
				
			
				
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
				System.out.println("EV TYPE - " + strEVType);
				
				// Full Name
				String strVarName = esTokens[1];
				System.out.println("STR VAR NAME " + strVarName);
				
				// Type Name
				String strTypeName = esTokens[2];
				System.out.println("STR TYPE NAME " + strTypeName);

				// Struct Name (may be NULL)
				String structName = esTokens[3];
				System.out.println("Struct Name " + structName);
				
				
				
				//bc.addType(strTypeName, structName);
				
				String rawWeightName = esTokens[4];
				System.out.println("Raw Weight Name " + rawWeightName);
				
				
				String skidRawWeightName = esTokens[5];
				System.out.println("Skid Raw Weight Name " + rawWeightName);
				
				
				double rawWeight = Double.valueOf(rawWeightName).doubleValue();
				double skidRawWeight = Double.valueOf(skidRawWeightName).doubleValue();		
				
				ExitSuper es = new ExitSuper();
								
				if (strEVType.indexOf("EV") >= 0)
				{
					ExitVariable ev = bf.getOrCreateEV(strVarName);
					es = ev;

				}
				else if (strEVType.indexOf("EGV") >= 0)
				{
					ExitVariable ev = bf.getOrCreateEV(strVarName);
					System.out.println("Exit Variable " + strVarName + " added to Function " + bf.getName());
					ev.isGlobal = true;
					es = ev;
					
					if (strEVType.indexOf('-') < 0)
						currInst.addNoSkidVariable(es);
						
					currInst.addVariable(es);
					
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
					currInst.addVariable(es);
					
					if (strEVType.indexOf('-') < 0)
						currInst.addNoSkidVariable(es);

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
						fieldParent.addInstance(currInst, sf);

						strVarName = newStrVarName;		
					}
					
					if (strEVType.indexOf('-') < 0)
						currInst.addNoSkidVariable(es);
					
					currInst.addVariable(es);
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
						fieldParent.addInstance(currInst, sf);
						
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
						fieldParent.addInstance(currInst, sf);
						
						strVarName = newStrVarName;
						epf = fieldParent;

					}
				}
				else
				{
				    evLine = bufReader.readLine();
				    continue;
				}
								
				System.out.println("Setting GT " + strTypeName + " ST " + structName + " for " + es.getName());
				es.setGenType(strTypeName);
				es.setStructType(structName);
				es.setType();
				es.setRawWeight(rawWeight);
				es.setSkidRawWeight(skidRawWeight);
				es.setEvTypeName(strEVType);

				
				bc.addType(es.getType());
				//es.setParentName(strParentName);
	
				es.addInstance(currInst, sf);
			
				
				evLine = bufReader.readLine();
			}
			
			//sf.resolveFrameES();
			
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
		instances = new Vector<Instance>();
		
		//File f = new File("/Users/nickrutar/Documents/workspace/BlameGUI/blame/Output_bug00.umiacs.umd.edu");
		
		File f = new File(outputFile);
		
		try {	
			System.out.println("Read in file " + outputFile);
			BufferedReader bufReader = new BufferedReader(new FileReader(f));
			String line = null;
			
			int instanceCount = 0;
			
			while ((line = bufReader.readLine()) != null) 
			{
				//System.out.println("LINE --- " + line);
				
				Instance currInst;
				
				
				if (line.indexOf("--INSTANCE") >= 0) 
				{
					currInst = new Instance(instanceCount, nodeName);
					instanceCount++;
					instances.add(currInst);	
					
					line = bufReader.readLine();
					
					// We are dealing with a FRAME
					while (line.indexOf("$$INSTANCE") < 0)
					{
						line = parseFrame(bufReader, currInst, line);
					}
					
					if (currInst.getStackFrames().size() == 0)
					{
						
						short bpType = BlameFunction.EXPLICIT_BLAME_POINT;
						BlameFunction bf = bc.getOrCreateBlameFunction("UNRESOLVED", "NO_MODULE", bpType);
						
						bc.addBlamedFunction("UNRESOLVED",bf);
						
						ExitProgram ep = bf.getOrCreateEP("INCOMPLETE_STACK_WALK");
						ep.addInstance(currInst, null);


											
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
	
	
	public void print()
	{
		System.out.println("We have " + instances.size() + "instances");
		
		Iterator<Instance> itr = instances.iterator();
		while (itr.hasNext()) 
		{
			((Instance)itr.next()).print();
		}
	}
	
	public int numInstances()
	{
		return instances.size();
	}
	
	
	

}
