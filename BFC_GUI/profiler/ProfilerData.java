package profiler;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;
//import java.util.Vector;

//import blame.BlameFunction;



public class ProfilerData {
	
	private HashMap<String, ProfilerFunction> allFunctions;

	public ProfilerData()
	{
		allFunctions = new HashMap<String, ProfilerFunction>();
	}
	
	
	public HashMap<String, ProfilerFunction> getAllFunctions() {
		return allFunctions;
	}


	public Map<String, ProfilerFunction> getSortedFunctions(boolean standardSort) {
		
		if (standardSort)
		{
			SortedMap<String, ProfilerFunction> sortedMap = new TreeMap<String, ProfilerFunction>(allFunctions);
			return sortedMap;
		}
		else
		{
			SortedMap<String, ProfilerFunction> sortedMap = MapValueSort.getValueSortedMap(allFunctions);
			return sortedMap;
		}
	}
	
	

	public void setAllFunctions(HashMap<String, ProfilerFunction> allFunctions) {
		this.allFunctions = allFunctions;
	}

	public int parseNode(String outputFile)
	{
		String nodeName = outputFile.substring(outputFile.indexOf("PARSED_") + 7);
		return parseOutputFile(outputFile, nodeName);
	}
	
    //allFunctions has names like main.foo.bar, main.foo, main....
	public void addInstanceToEntireCallPath(String funcName, ProfileInstance pi, ProfilerFunction oldFunc)
	{
		ProfilerFunction pf = allFunctions.get(funcName);
				
		if (pf == null)
		{
			pf = new ProfilerFunction(funcName);
			allFunctions.put(funcName, pf);
			//System.out.println("Adding " + funcName);
		}
		
        // VERY IMPORTANT
		pf.addInstance(pi);

		if (oldFunc != null)
		{
			//System.out.println("Adding descendant " + oldFunc.getName() + " to " + pf.getName());
			pf.addDescendant(oldFunc);
			oldFunc.setFuncParent(pf);
		}
		
		int lastPeriod = funcName.lastIndexOf('.');
		if (lastPeriod == -1)
		{
			return;
		}
		
		String truncString = funcName.substring(0, lastPeriod);
		addInstanceToEntireCallPath(truncString, pi, pf);
	}
	
	public String parseFrame(BufferedReader bufReader, ProfileInstance currInst, String line)
	{
		String pathTokens[] = line.split("\\s"); //Q: why double back slash
		//String strFrameNum = pathTokens[1];
		//int frameNum = Integer.valueOf(strFrameNum).intValue();
		String funcName = pathTokens[2];
		//String moduleName = pathTokens[3];
		//String pathName = pathTokens[4];
		//String strLineNum  = pathTokens[5];
		//int lineNumber = Integer.valueOf(strLineNum).intValue();
		//String strBPType   = pathTokens[6];
		//short bptType = Short.valueOf(strBPType).shortValue();
		
		
		addInstanceToEntireCallPath(funcName, currInst, null);
		
		/*
		ProfilerFunction pf = allFunctions.get(funcName);
		//System.out.println("cashew");
		
		
		
		if (pf == null)
		{
			pf = new ProfilerFunction(funcName);
			allFunctions.put(funcName, pf);
			System.out.println("Adding " + funcName);
		}
		*/
		
		
		try {
			String evLine = bufReader.readLine();		
			while (evLine.indexOf("FRAME#") < 0)
			{
				if (evLine.indexOf("$$INSTANCE") >= 0)
				{
					return evLine;
				}
									
				evLine = bufReader.readLine();
			}
						
			return evLine; //until next frame
			
		}
		catch (IOException e) {
			System.err.println("exception happened - here's what I know: ");
			e.printStackTrace();
			System.exit(-1);
		}
		
		return new String();
	}
	
	
	
	
	public int parseOutputFile(String outputFile, String nodeName)
	{
				
		File f = new File(outputFile);
		int instanceCount = 0;

		
		try {	
			//System.out.println("Read in file " + outputFile);
			BufferedReader bufReader = new BufferedReader(new FileReader(f));
			String line = null;
			
			
			while ((line = bufReader.readLine()) != null) 
			{				
				ProfileInstance currInst;
				
				if (line.indexOf("--INSTANCE") >= 0) 
				{
					currInst = new ProfileInstance(instanceCount, nodeName);
					instanceCount++;
						
					line = bufReader.readLine();
					
					boolean atLeastOneFrame = false;
					
					// We are dealing with a FRAME
					if (line.indexOf("FRAME") >= 0)
					{
						atLeastOneFrame = true;
                        //just to get the func name for this frame
						line = parseFrame(bufReader, currInst, line);
					}

					while (line.indexOf("$$INSTANCE") < 0)
					{
						line = bufReader.readLine();
						//line = parseFrame(bufReader, currInst, line);
					}
					
					if (atLeastOneFrame == false)
					{
	
						String funcName = "UNRSESOLVED";
						ProfilerFunction pf = allFunctions.get(funcName);
						
						if (pf == null)
						{
							pf = new ProfilerFunction(funcName);
							allFunctions.put(funcName, pf);
							//System.out.println("Adding " + funcName);
						}
						
						pf.addInstance(currInst);
						
					}
				
				}
			}
				
		}
		catch (IOException e) {
			System.err.println("exception happened - here's what I know: ");
			e.printStackTrace();
			System.exit(-1);
		}
		return instanceCount;
		
	}
	
	
}
