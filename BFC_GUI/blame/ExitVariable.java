package blame;

import java.util.*;


public class ExitVariable extends ExitSuper {

	/*
	public double compareTo(ExitVariable o) {
        return o.numInstances() - this.numInstances();
    }*/
	
	ExitVariable(String n)
	{
		super();
		name = n;
		hierName = n;
		isGlobal = false;	
	}	
	
	public String printDescription()
	{
		String s = new String("Exit Variable - " + super.printDescription());
		return s;
	}
	
	
	
	public void addField(String s, BlameFunction bf)
	{
		HashMap<String, ExitVariable> eVFields = bf.getExitVarFields();
		ExitSuper es = eVFields.get(s);
		if (es != null)
		{
			//int lastDot = es.getHierName().lastIndexOf('.');
			//lastDot++;
			
			//String truncHierName = es.getHierName().substring(lastDot);
			fields.put(es.getName(), es);
			//es.setName(truncHierName);
		}
		else
		{
			System.out.println("EV addField NULL for " + s);
		}
	}
}








/*
public String toString()
{
	
//	return name + " " + numInstances();
	double nI = (double) numInstances();
	double tI = (double) Global.totalInstances;
	double dValue = (nI/tI)*100.0;
	
     DecimalFormat df = new DecimalFormat("#.##");

	//int value = (int) dValue;
	return "<html><font color=\"blue\">" + name + " ( " + genType + " ) " + df.format(dValue) + "%" + "</font></html>";
}
*/






/*
public void transferVI()
{
	System.out.println("In TransferVI for " + hierName);
	BlameFunction bf = this.getParentBF();
	Iterator<ExitVariable> it = bf.getExitVariables().values().iterator();
	
	while (it.hasNext())
	{
		ExitVariable es = (ExitVariable) it.next();
		System.out.println("Looking(1) at " + es.getHierName());
		if (es.getHierName().indexOf('-') <= 0 )
			continue;
		
		System.out.println("Looking(2) at " + es.getHierName());
		if (es.getHierName().indexOf(hierName) < 0)
			continue;
		
		System.out.println("Transferring VI from " + es.getHierName() + " to " + hierName);
		
		// We want to go through the nodes for the matching value
		//  and append it to the existing values
	
		//HashMap<String,NodeInstance> nihash = es.getNodeInstances();
		
		Iterator<NodeInstance> ni_it = es.getNodeInstances().values().iterator();
		while(ni_it.hasNext())
		{
			
			// Existing
			NodeInstance ni = (NodeInstance) ni_it.next();
			
			// Probably not there to start
			NodeInstance matchingNI = nodeInstances.get(ni.getNName());
		
			if (matchingNI == null)
			{
				matchingNI = new NodeInstance(this.getHierName(), ni.getNName());
				nodeInstances.put(matchingNI.getNName(), matchingNI);

			}
		
			matchingNI.getVarInstances().addAll(ni.getVarInstances());


		}	 
	}
	
}
*/

