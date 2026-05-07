package IR.Value.Instructions;

import IR.Type.Type;
import IR.Type.VoidType;
import IR.Value.BasicBlock;
import IR.Value.Value;

import java.util.ArrayList;

public class Move extends Instruction{
	public boolean hasname=true;
	public ArrayList<Move> pair=new ArrayList<>();
	public void setpair(ArrayList<Move> p)
	{
		pair=p;
	}

	public Move(Value src ,Type type) {
		super("%" + (++Value.valNumber), type, OP.Move);
//		super(dest.getName(), type, OP.Move);
		addOperand(src);
	}
	@Override
	public boolean hasName(){
		return hasname;
	}



	@Override
	public String getInstString() {
		return getName()+" = move " + getOperand(0).getName();
	}
}