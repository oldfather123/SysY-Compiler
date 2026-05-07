package Pass;

import Backend.component.ObjBlock;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Type.Type;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.DomAnalysis;
import Pass.IR.Utils.IRLoop;
import Pass.Pass;
import Utils.DataStruct.IList;
import Utils.DataStruct.Pair;

import java.io.FileWriter;
import java.io.IOException;
import java.util.*;

public class RemovePhi implements Pass.IRPass {
	@Override
	public String getName() {
		return "RemovePhi";
	}

	@Override
	public void run(IRModule module) {
		for (Function function : module.getFunctions()) {
			if(function.getCalleeList().contains(function))
			{
				boolean istail=true;
				for (IList.INode<BasicBlock, Function> b : function.getBbs()) {
					for(IList.INode<Instruction,BasicBlock> i : b.getValue().getInsts())
					{
						if(i.getValue() instanceof CallInst)
						{
							CallInst call= (CallInst) (i.getValue());
							if(call.getFunction() .equals(function))
							{
								if(i.getNext()!=null &&( !(i.getNext().getValue()instanceof RetInst)))
									istail=false;
							}
						}
					}
				}
				function.istailrecursive=istail;
				//if(function.istailrecursive) System.out.println(function.getName()+" is tail recursive");
			}

			for (IList.INode<BasicBlock, Function> b : function.getBbs()) {
				BasicBlock curbb = b.getValue();
				for (IList.INode<Instruction, BasicBlock> i : curbb.getInsts()) {
					if (i.getValue() instanceof Phi) {

						ArrayList<Move> pairlist = new ArrayList<>();
						//new1��ǰ��Ļ������е����phi�ı����ı���
						for (int ii = 0; ii < i.getValue().getOperands().size(); ii++) {
							Value val = i.getValue().getOperand(ii);//%x1
							BasicBlock src = curbb.getPreBlocks().get(ii);//%b1
							Move mv = new Move(val, val.getType());
							pairlist.add(mv);
							if (src.getNxtBlocks().size() > 1) {
								BasicBlock tmp = null;
								for (BasicBlock bb : src.getNxtBlocks()) {
									if (curbb.getPreBlocks().contains(bb) && bb.meow) {
										tmp = bb;
									}
								}
								if (tmp != null) {

									mv.getNode().insertBefore(tmp.getLastInst().getNode());
								} else {
									BasicBlock new2 = new BasicBlock(function);
									new2.meow = true;
									new2.depth=src.getLoopDepth();
									new2.addInst(mv);
									new2.addInst(new BrInst(curbb));
									new2.insertAfter(src);
									src.setNxtBlock(new2);
									new2.setPreBlock(src);
									new2.setNxtBlock(curbb);
									curbb.setPreBlock(new2);
									assert src.getLastInst() instanceof BrInst;
									BrInst last = (BrInst) src.getLastInst();
									if (last.getJumpBlock() != null && last.getJumpBlock().equals(curbb)) {
										last.setJumpBlock(new2);
									}
									if (last.getFalseBlock() != null && last.getFalseBlock().equals(curbb)) {
										last.setFalseBlock(new2);
									}
									if (last.getTrueBlock() != null && last.getTrueBlock().equals(curbb)) {
										last.setTrueBlock(new2);
									}
								}

							} else {

								mv.getNode().insertBefore(src.getLastInst().getNode());
							}

						}
						for (Move x : pairlist) {
							x.setpair(pairlist);
						}

						Move x = new Move(pairlist.get(0), pairlist.get(0).getType());
						x.getNode().insertBefore(i);
						i.getValue().replaceUsedWith(x);
					}
				}

				ArrayList<Instruction> tobeRemoved = new ArrayList<>();
				//ɾ������phi
				for (IList.INode<Instruction, BasicBlock> i : curbb.getInsts())
					if (i.getValue() instanceof Phi) {
						tobeRemoved.add(i.getValue());
					}
				for (Instruction deleteInst : tobeRemoved) {
					deleteInst.getNode().removeFromList();
				}

			}
		}


		for (Function function : module.getFunctions()) {
			ArrayList<BasicBlock> retbbs=new ArrayList<>();
			ArrayList<Instruction> tobeRemoved = new ArrayList<>();
			ArrayList<Move> pairlist = new ArrayList<>();
			BasicBlock newbb = new BasicBlock(function);
			newbb.depth=0;

			for (IList.INode<BasicBlock, Function> b : function.getBbs()) {
				BasicBlock curbb = b.getValue();
				for (IList.INode<Instruction, BasicBlock> i : curbb.getInsts()) {
					if ( i.getValue() instanceof RetInst)
					{

						retbbs.add(curbb);
					}
				}
			}
			assert !retbbs.isEmpty();
			if(retbbs.size()==1)
			{
				function.setBbExit(retbbs.get(0));
			}else
			{
				for(BasicBlock b : retbbs)
				{
					for (IList.INode<Instruction, BasicBlock> i : b.getInsts())
					{
						if(i.getValue() instanceof  RetInst)
						{
							tobeRemoved.add(i.getValue());
							assert !((RetInst) i.getValue()).isVoid();
							Value val = ((RetInst) i.getValue()).getValue();
							Move mv = new Move(val, val.getType());
							mv.insertBefore(i.getValue());
							BrInst br=new BrInst(newbb);
							br.insertBefore(i.getValue());
							pairlist.add(mv);
						}
					}
				}

				Move x = new Move(pairlist.get(0), pairlist.get(0).getType());
				newbb.addInst(x);
				newbb.addInst(new RetInst(x));
				newbb.insertAfter(function.getBbs().getTail().getValue());
				for(BasicBlock b : retbbs)
				{
					b.setNxtBlock(newbb);
					newbb.setPreBlock(b);
				}
				function.setBbExit(newbb);
				for (Move xx : pairlist) {
					xx.setpair(pairlist);
				}
				for (Instruction deleteInst : tobeRemoved) {
					deleteInst.getNode().removeFromList();
				}
			}

		}

	}


}