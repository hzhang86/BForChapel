package blame;

import java.util.Iterator;
import java.util.Vector;

public class VariableInstance {
	
	double weight;
	double skidWeight;
	private Vector<Integer> lineNumbers;
	Instance inst;
	String varName;
	ExitSuper var;
	String nodeName;
	
	public String getNodeName() {
		return nodeName;
	}

	public double getSkidWeight() {
		return skidWeight;
	}

	public void setSkidWeight(double skidWeight) {
		this.skidWeight = skidWeight;
	}

	public void setNodeName(String nodeName) {
		this.nodeName = nodeName;
	}

	public ExitSuper getVar() {
		return var;
	}

	public void setVar(ExitSuper var) {
		this.var = var;
	}
	
	VariableInstance(Vector<Integer> ln, Instance inst, String varName)
	{
		//TODO: Pass in a EP/EV parent class
		this.varName = varName;
		weight = 0.0;
		skidWeight = 0.0;
		lineNumbers = new Vector<Integer>();
		lineNumbers = ln;
		this.inst = inst;
	}
	
	VariableInstance(ExitSuper v, Instance i)
	{
		var = v;
		inst = i;
		varName = var.getName();
		weight = 0.0;
	}
	
	public boolean equals(Object o)
	{
		if (o == null)
			return false;
			
		VariableInstance vi = (VariableInstance) o;
		if (vi.getInst() == this.getInst())
			return true;
		else
			return false;
	}
	
	public String stringEV()
	{
		String s = new String();
		s += "\n";
		Iterator<Integer> i = lineNumbers.iterator();
		while (i.hasNext())
			s = s + i.next() + " ";
		s += "\n";
		return s;
	}
	
	public String toString()
	{
		return "Instance " + inst.getInstanceNum();
	}
	
	void print()
	{
		System.out.println("Instance - inst" + inst.getInstanceNum() + " varName " + varName);
	}
	
	
	void addLines(String[] lines)
	{
		for (int i = 0; i < lines.length; i++)
		{
			Integer lineno = Integer.valueOf(lines[i]);
			lineNumbers.add(lineno);
		}
	}
	

	public Instance getInst() {
		return inst;
	}

	public void setInst(Instance inst) {
		this.inst = inst;
	}

	public Vector<Integer> getLineNumbers() {
		return lineNumbers;
	}

	public void setLineNumbers(Vector<Integer> lineNumbers) {
		this.lineNumbers = lineNumbers;
	}

	public String getVarName() {
		return varName;
	}

	public void setVarName(String varName) {
		this.varName = varName;
	}

	public double getWeight() {
		return weight;
	}

	public void setWeight(double weight) {
		this.weight = weight;
	}
}
