package blame;

import java.util.HashMap;

public class NodeInstance {

	String varName;
	String nodeName;
	
	int numTotalInstances;
	
	//private Vector<VariableInstance> varInstances;
	private HashMap<Integer, VariableInstance > varInstances;
	private HashMap<Integer, VariableInstance > skidVarInstances;
	
	
	NodeInstance(String vN, String nN)
	{
		varName = vN;
		System.out.println("Setting Node Name(2) to " + nN);
		nodeName = nN;
		varInstances = new HashMap<Integer, VariableInstance>();
		skidVarInstances = new HashMap<Integer, VariableInstance>();
	}
	
	
	public HashMap<Integer, VariableInstance> getSkidVarInstances() {
		return skidVarInstances;
	}

	public void setSkidVarInstances(
			HashMap<Integer, VariableInstance> skidVarInstances) {
		this.skidVarInstances = skidVarInstances;
	}

	
	
	public int numInstances()
	{
		return varInstances.size();
	}
	
	public String toString()
	{
		return "Node:" + nodeName + ", Num Instances: " + numInstances();
	}
	
	void addInstance(VariableInstance vi)
	{
		varInstances.put(vi.getInst().getInstanceNum(), vi);
		//varInstances.add(vi);
	}
	
	void addInstanceSkid(VariableInstance vi)
	{
		skidVarInstances.put(vi.getInst().getInstanceNum(),vi);
	}
	
	
	public String getNName()
	{
		return nodeName;
	}

	public String getNodeName() {
		return nodeName + " " + varInstances.size();
	}

	public void setNodeName(String nodeName) {
		System.out.println("Setting node name to " + nodeName);
		this.nodeName = nodeName;
	}

	public HashMap<Integer, VariableInstance> getVarInstances() {
		return varInstances;
	}

	public void setVarInstances(HashMap<Integer, VariableInstance> varInstances) {
		this.varInstances = varInstances;
	}

	public String getVarName() {
		return varName;
	}

	public void setVarName(String varName) {
		this.varName = varName;
	}

	public int getNumTotalInstances() {
		return numTotalInstances;
	}

	public void setNumTotalInstances(int numTotalInstances) {
		this.numTotalInstances = numTotalInstances;
	}
	
}













/*
VariableInstance addInstance(int lineNumber, Instance currInst, String[] lines, Vector<VariableInstance> evI)
{
	boolean isMatch = false;
	
	Vector<Integer> lineNumbers = new Vector<Integer>();
	
	for (int i = 0; i < lines.length; i++)
	{
		Integer lineno = Integer.valueOf(lines[i]);
		lineNumbers.add(lineno);
		
		if (lineNumber == lineno.intValue())
		{
			isMatch = true;
		}
	}
	
	if (isMatch == true)
	{
		//System.out.println("Match for EV " + name + " for linenum " + lineNumber);
		
		VariableInstance vi = new VariableInstance(lineNumbers, currInst, varName);
		
		// This is specific to the stack frame
		evI.add(vi);
		
		// This is specific to the exit variable across all instances
		varInstances.add(vi);
		
		return vi;
	}
	else
		return null;
}
*/



