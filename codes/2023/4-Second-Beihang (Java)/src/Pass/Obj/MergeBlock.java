package Pass.Obj;

import Backend.component.ObjBlock;
import Backend.component.ObjFunction;
import Backend.component.ObjGlobalVariable;
import Backend.component.ObjModule;
import Backend.instruction.ObjBranch;
import Backend.instruction.ObjInstr;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashSet;

public class MergeBlock implements Pass.ObjPass {
	@Override
	public String getName() {
		return "MergeBlock";
	}

	@Override
	public void run(ObjModule objModule) {
		//我的想法特别naive，因为我没找到资料草
		// 循环遍历各个基本块，如果某个bb有一个prevblock，并且它是这个prevblock最后的无条件跳转到的，就直接合并
		// 其他情况就啥也不干，草，我想的一定不对
		for (ObjFunction func : objModule.getFunctions()) {
			boolean changed = true;
			int loop=1;
			while (changed) {
				changed = false;
				HashSet<ObjBlock> toberemoved = new HashSet<>();
				HashSet<ObjInstr> toberemovedinst = new HashSet<>();
				for (IList.INode<ObjBlock, ObjFunction> b : func.getObjBlocks()) {

					ObjBlock block = b.getValue();
					//如果这个bb有多个前继，但是还是和其中一个前继合并了，那另一个前继就找不到他了
					if (block.getPreBlocks().size() == 1 && !block.getPreBlocks().get(0).equals(block) && !toberemoved.contains(block.getPreBlocks().get(0))) {
						ObjBlock prev = block.getPreBlocks().get(0);

						ObjInstr brr = null;
						if(prev.getInstrs().getTail()!=null) brr=prev.getInstrs().getTailValue();
						else if( prev.getInstrs().getSize()==1)  brr=prev.getInstrs().getHeadValue();
						if (brr instanceof ObjBranch br) {
							if (!br.isHasSrc() && br.getTarget()!=null&&br.getTarget().equals(block)) {
								//可以直接合并
								boolean ok=true;
								if(prev.getInstrs().getSize()>1)
								for (IList.INode<ObjInstr, ObjBlock> inst : prev.getInstrs()) {
									if(inst.equals(prev.getInstrs().getTail()))
										break;
									if(inst.getValue() instanceof ObjBranch  brrr)
									{
										if(brrr.getTarget().equals(block)) ok=false;
									}
								}
								if(!ok) continue;
								ArrayList<ObjInstr> xx = new ArrayList<>();
								for (IList.INode<ObjInstr, ObjBlock> inst : block.getInstrs()) {
									xx.add(inst.getValue());
								}
								for (ObjInstr x : xx) {
									x.getNode().insertBefore(brr.getNode());
								}
								toberemoved.add(block);
								//System.out.println("can remove "+block.getName());
								toberemovedinst.add(br);

							}
						}

					}
				}
				if (!toberemoved.isEmpty()) {
					changed = true;
				}
				for (ObjBlock b : toberemoved) b.getNode().removeFromList();
				for (ObjInstr b : toberemovedinst) b.getNode().removeFromList();
			}
			HashSet<ObjBlock> toberemoved= new HashSet<>();
			for (IList.INode<ObjBlock, ObjFunction> b : func.getObjBlocks())
			{
				if(b.getValue().getInstrs().getSize()==0) toberemoved.add(b.getValue());
			}
			for(ObjBlock i : toberemoved)
			{
				i.getNode().removeFromList();
			}


		}

	}
}
