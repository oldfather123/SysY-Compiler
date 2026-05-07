package Backend.operand;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class ObjFPhyReg extends ObjReg {
	public final static HashMap<Integer, String> indexToName = new HashMap<>();
	private final static HashMap<String, Integer> nameToIndex = new HashMap<>();
	public final static ArrayList<ObjFPhyReg> A = new ArrayList<>();
	public final static ArrayList<ObjFPhyReg> AllRegs = new ArrayList<>();

	static {
		for(int i = 0; i <= 7; i ++)
		{
			indexToName.put(i, "ft" + i);
		}

		indexToName.put(8, "fs0");
		indexToName.put(9, "fs1");

		for(int i = 10; i <= 17; i ++)
		{
			indexToName.put(i, "fa" + String.valueOf(i - 10));
		}
		for(int i = 18; i <= 27; i ++)
		{
			indexToName.put(i, "fs" + String.valueOf(i - 16));

		}
		for(int i = 28; i <= 31; i ++)
		{
			indexToName.put(i, "ft" + String.valueOf(i - 20));

		}

		for (Map.Entry<Integer, String> entry : indexToName.entrySet())
			nameToIndex.put(entry.getValue(), entry.getKey());

		for(int i = 0; i <= 7; i ++)
		{

			AllRegs.add(new ObjFPhyReg("ft" + i));
		}


		AllRegs.add(new ObjFPhyReg("fs0"));

		AllRegs.add(new ObjFPhyReg("fs1"));

		for(int i = 10; i <= 17; i ++)
		{

			ObjFPhyReg opr= new ObjFPhyReg("fa" + String.valueOf(i - 10));
			AllRegs.add(opr);
			A.add(opr);
		}
		for(int i = 18; i <= 27; i ++)
		{

			AllRegs.add(new ObjFPhyReg("fs" + String.valueOf(i - 16)));
		}
		for(int i = 28; i <= 31; i ++)
		{

			AllRegs.add(new ObjFPhyReg("ft" + String.valueOf(i - 20)));
		}

	}

	private final int index;
	private final String name;
	private boolean isAllocated;

	public ObjFPhyReg(String name) {
		this.name = name;
		this.index = nameToIndex.get(name);
		this.isAllocated = false;
		this.color=this.index;
	}
	public ObjFPhyReg(int index) {
		this.name = indexToName.get(index);
		this.index = index;
		this.isAllocated = false;
	}
	public ObjFPhyReg(int index, boolean isAllocated) {
		this.name = indexToName.get(index);
		this.index = index;
		this.isAllocated = isAllocated;
	}
	public void setAllocated(boolean isAllocated) {
		this.isAllocated = isAllocated;
	}
	public int getIndex() {
		return index;
	}

	@Override
	public String toString() {
		return  name;
	}
}
