package blame;


public class ExitOutput extends ExitSuper {

	/*
	public double compareTo(ExitOutput o) {
        return o.numInstances() - this.numInstances();
    }
	
	
	public double compareTo(ExitProgram o) {
        return o.numInstances() - this.numInstances();
    }
    */
	
	ExitOutput(String n)
	{
		super();
		name = n;
		/*
		//varInstances = new Vector<VariableInstance>();
		nodeInstances = new HashMap<String, NodeInstance>();
		isField = false;
		//lineNumsByFunc = new HashMap<String, HashSet<Integer>>();
		hierName = null;
		eStatus = null;
		structType = null;
		
		lineNums = new HashSet<Integer>();
		*/
	}
	
	public String printDescription()
	{
		String s = new String("Exit Output - " + super.printDescription());
		return s;
	}
	
}
