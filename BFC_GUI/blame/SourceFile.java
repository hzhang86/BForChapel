package blame;

import java.io.*;
import java.util.*;

public class SourceFile{

	private String fileName;  /* Name of source file */
	private String filePath;
	private Vector<String> lines;  /* Vector of String Objects that represent text of code */
	

	public SourceFile()
	{
		lines = new Vector<String>();
	}
	
	public String getFileName()
	{
		return fileName;
	}
	
	public void setFileName(String fn)
	{
		fileName = fn;
	}
	
	public Vector<String> getLines()
	{
		//System.err.println(filePath+fileName);
		
		// Parse on demand
		if (lines.size() == 0)
		{
			setLinesOfCode();
		}
		
		return lines;		
	}
	
	public void setLinesOfCode()
	{
		int lineNumber = 1;
		String baseLine;
		String line;
	
		
		if (filePath.charAt(filePath.length()-1) != '/')
			filePath += '/';
		
		String newFilePath = filePath.replace("/hivehomes/rutar", "/Users/nickrutar/UnixStuff");
				
		String inputFString = newFilePath + fileName;
		System.err.println("Trying to open " + inputFString);
		
		File inputFile = new File(inputFString);
		
			try {
				BufferedReader bufReader =
					new BufferedReader(new FileReader(inputFile));

				
				while ((line = bufReader.readLine()) != null) 
				{
					//System.out.println(line);
					baseLine = new String(lineNumber + "    ");
					lines.add(baseLine + line);
					lineNumber++;
				}
				bufReader.close();

			} catch (IOException e) {
				
				System.err.println("File not found for " + inputFile);
				System.exit(1);
			}	
	}

	

	public String getFilePath() {
		return filePath;
	}

	static boolean DO_OVERRIDE_PATH = false;
	static String OVERRIDE_PATH = "/Users/nickrutar/UnixStuff/BLAME/TEST-PROGRAMS/HPL/OLD/hpl-openmpi-2-llvm/";
	//static String OVERRIDE_PATH = "/Users/nickrutar/UnixStuff/BLAME/TEST-PROGRAMS/HPL/";
	
	
	public void setFilePath(String pfilePath) {
		this.filePath = pfilePath;
		
		
		if (DO_OVERRIDE_PATH)
		{
			if (Global.testProgram.equals(new String("HPL")))
			{
				String tail = filePath.substring(filePath.indexOf("hpl-2.0"));
				String trimTail = tail.substring(0, tail.indexOf("LLVM")-1);
				filePath = OVERRIDE_PATH + trimTail;
			}
			else if (Global.testProgram.equals(new String("SMALL")))
			{
				filePath = "/Users/nickrutar/UnixStuff/BLAME/SMALLEXAMPLE";
			}
		}
		
	}
	
	
}
