/*
 *  Copyright 2014-2017 Hui Zhang
 *  Previous contribution by Nick Rutar 
 *  All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package blame;

import java.io.BufferedReader;
import java.util.Iterator;
import java.util.Vector;

public class ParentData {

	
	Vector<Instance> instances;
	String nodeName;
	protected int instanceCount;
	//int nodeNum;
	
	// "Pointer" up to the blameContainer that contains everything
	BlameContainer bc;
	
	
	
	public String parseFrame(BufferedReader bufReader, Instance currInst, String line)
	{
		return null;
	}
	
	public void parseOutputFile(String outputFile, String nodeName)
	{
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
		return 0;
	}
	
	
}
