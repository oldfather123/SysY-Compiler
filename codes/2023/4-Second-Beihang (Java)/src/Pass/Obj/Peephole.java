package Pass.Obj;

import Backend.component.ObjBlock;
import Backend.component.ObjFunction;
import Backend.component.ObjModule;
import Backend.instruction.*;
import Backend.operand.*;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.*;

public class Peephole implements Pass.ObjPass{
	@Override
	public String getName(){
		return "Peephole";
	}

	@Override
	public void run(ObjModule objModule){

		boolean loop=false;
		while(!loop){
			loop=basicPH(objModule);
//            loop&=smallDataFlow(objModule);
			for(IList.INode<ObjInstr,ObjBlock> objInstrObjBlockINode: deleteList){
				objInstrObjBlockINode.removeFromList();
			}
			deleteList.clear();
		}

	}

	public ArrayList<IList.INode<ObjInstr,ObjBlock>> deleteList=new ArrayList<>();

	public boolean basicPH(ObjModule objModule){
		boolean stop=true;
		ArrayList<ObjFunction> functions=objModule.getFunctions();
		for(ObjFunction function: functions){
			IList<ObjBlock,ObjFunction> blocks=function.getObjBlocks();
			for(IList.INode<ObjBlock,ObjFunction> instBlock: blocks){
				ObjBlock block=instBlock.getValue();
				for(IList.INode<ObjInstr,ObjBlock> instNode: block.getInstrs()){
					stop&=zero(instNode);
					stop&=uselessMove(instNode);
					stop&=uselessJump(instNode,instBlock.getNext());
					stop&=uselessMemory(instNode);
//					stop&=uselessMoveBin(instNode);
//					stop&=uselessMoveBin2(instNode);
				}
			}
		}
		return stop;
	}

	// add t0, t1, 0 => mov t0, t1
	private boolean zero(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		if(nowInstr instanceof ObjBinary nowBinary){
			if(nowBinary.isSrc2Imm12()){
				ObjImm12 nowImm=(ObjImm12) nowBinary.getSrc2();
				if(nowImm.getImmediate12()==0){
					if(nowBinary.getType().equals("add")||nowBinary.getType().equals("sll")||nowBinary.getType().equals("srl")||nowBinary.getType().equals("sra")){
						if(!nowBinary.getDst().equals(nowBinary.getSrc1())){
							ObjMove objMove=new ObjMove(nowBinary.getDst(),nowBinary.getSrc1());
							objMove.getNode().insertBefore(instNode);
						}

						deleteList.add(instNode);
						stop=false;
					}
				}
			}
		}
		return stop;
	}

	/* 乘法优化 mul t0, t1, 19 => li t0 0
								sll t2, t1, 5
								add t0, t2, t0
								sll t2, t1, 1
								add t0, t2, t0
								sll t2, t1, 0
								add t0, t2, t0
	 */
	private boolean multiple(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		IList.INode<ObjInstr,ObjBlock> beforeNode=instNode.getPrev();
		if(beforeNode==null){
			return true;
		}
		ObjInstr beforeInstr=beforeNode.getValue();
		if(nowInstr instanceof ObjBinary nowBinary&&nowBinary.getType().equals("mul")&&beforeInstr instanceof ObjMove&&((ObjMove) beforeInstr).getSrc() instanceof ObjImm){
			ObjImm nowImm;
			nowImm=(ObjImm) ((ObjMove) beforeInstr).getSrc();
			int num=nowImm.getImmediate();
			String biNum="";
			while(num>0){
				biNum+=num%2;
				num/=2;
			}
//                    String one="1";
//                    String zero="0";
//                    String oneString=biNum.replace(zero,"");
//                    if(oneString.length()>biNum.length()/2){//加法换1
			ObjOperand zeroOperand=new ObjImm12(0);
			ObjOperand newOperand=new ObjVirReg();
			ObjMove objBinary=new ObjMove(nowBinary.getDst(),zeroOperand);
			objBinary.getNode().insertAfter(instNode);
			deleteList.add(instNode);
			deleteList.add(beforeNode);
			IList.INode<ObjInstr,ObjBlock> nowNode=objBinary.getNode();
			for(int i=0;i<biNum.length();i++){
				if(biNum.charAt(i)=='1'){
					ObjOperand leftOperand=new ObjImm12(biNum.length()-i-1);
					ObjBinary newObjBinary=new ObjBinary("slli",newOperand,nowBinary.getSrc1(),leftOperand);
					newObjBinary.setIsword(true);
					newObjBinary.getNode().insertAfter(nowNode);
					nowNode=newObjBinary.getNode();
					ObjBinary newObjBinary2=new ObjBinary("add",nowBinary.getDst(),newOperand,nowBinary.getDst());
					newObjBinary2.setIsword(true);
					newObjBinary2.getNode().insertAfter(nowNode);
					nowNode=newObjBinary2.getNode();
				}
			}

//                    }
//                    else{
//                        //减法换0
//                    }
			stop=false;
		}

		return stop;
	}

	/* move t0, t1
	   move t0, t2 => move t0, t2
	 */
	private boolean uselessMove(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		IList.INode<ObjInstr,ObjBlock> nextNode=instNode.getNext();
		if(nowInstr instanceof ObjMove objMove){
			if(nextNode!=null&&nextNode.getValue() instanceof ObjMove nextMove){
				if(objMove.getDst().equals(nextMove.getDst())&&!(objMove.getSrc().equals(nextMove.getSrc()))){
//                    instNode.removeFromList();
					deleteList.add(instNode);
					stop=false;
				}
			}
		}
		return stop;
	}

	/* j aaa
	   aaa:  => null
	 */
	private boolean uselessJump(IList.INode<ObjInstr,ObjBlock> instNode,IList.INode<ObjBlock,ObjFunction> nextBlock){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		if(nowInstr instanceof ObjBranch objBranch){
			if(nextBlock!=null&&!(objBranch.isHasSrc())&&(objBranch.getTarget()!=null)&&objBranch.getTarget().equals(nextBlock.getValue())){
//                instNode.removeFromList();
				deleteList.add(instNode);
				stop=false;
			}
		}
		return stop;
	}

	/* store a, c(d)   store a, c(d)
	   load b, c(d) => mov b, a
	 */
	private boolean uselessMemory(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		IList.INode<ObjInstr,ObjBlock> nextNode=instNode.getNext();
		if(nextNode!=null&&nowInstr instanceof ObjStore objStore&&nextNode.getValue() instanceof ObjLoad objLoad){
			if(objStore.getAddr().equals(objLoad.getAddr())){
				int off1=((ObjImm12) (objStore.getOffset())).getImmediate12();
				int off2=((ObjImm12) (objLoad.getOffset())).getImmediate12();
				if(off1==off2){
					if(!objStore.getSrc().equals(objLoad.getDst())){
						ObjMove newMove=new ObjMove(objLoad.getDst(),objStore.getSrc());
						newMove.getNode().insertAfter(instNode);
					}
					deleteList.add(nextNode);
					stop=false;
				}
			}
		}
		return stop;
	}

	private boolean uselessMoveBin(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		IList.INode<ObjInstr,ObjBlock> nextNode=instNode.getNext();
		if(nextNode!=null&&nowInstr instanceof ObjMove objMove&&nextNode.getValue() instanceof ObjBinary objBinary&&!(objMove.getSrc() instanceof ObjImm) &&!(objMove.getSrc() instanceof ObjImm12)){
			if(!(objMove.getSrc() instanceof ObjFPhyReg||objMove.getSrc() instanceof ObjFVirReg||objMove.getSrc() instanceof ObjLabel||objMove.getSrc() instanceof ObjImm||objMove.getSrc() instanceof ObjImm12)){
				if((objBinary.getDst().equals(objBinary.getSrc1())||objBinary.getDst().equals(objBinary.getSrc2()))&&objMove.getDst().equals(objBinary.getDst())){
					objBinary.replaceUseReg(objBinary.getDst(),objMove.getSrc());
					deleteList.add(instNode);
					stop=false;
				}
			}

		}
		return stop;
	}

	private boolean uselessMoveBin2(IList.INode<ObjInstr,ObjBlock> instNode){
		boolean stop=true;
		ObjInstr nowInstr=instNode.getValue();
		IList.INode<ObjInstr,ObjBlock> midNode=instNode.getNext();
		if(midNode==null){return true;}
		IList.INode<ObjInstr,ObjBlock> nextNode=midNode.getNext();
		ObjInstr midInstr=midNode.getValue();
		if(midInstr instanceof ObjBinary midBinary){
		if(nextNode!=null&&nowInstr instanceof ObjMove objMove&&nextNode.getValue() instanceof ObjBinary objBinary&&!(objMove.getSrc() instanceof ObjImm) &&!(objMove.getSrc() instanceof ObjImm12)){
			if(!(midBinary.getDst().equals(objBinary.getDst())   || midBinary.getSrc1().equals(objBinary.getDst())||midBinary.getSrc2().equals(objBinary.getDst()))){
				if(!(objMove.getSrc() instanceof ObjFPhyReg||objMove.getSrc() instanceof ObjFVirReg||objMove.getSrc() instanceof ObjLabel||objMove.getSrc() instanceof ObjImm||objMove.getSrc() instanceof ObjImm12)){
//					if(midBinary.getSrc1().equals(objBinary.getDst())||midBinary.getSrc2().equals(objBinary.getDst())){
//						midBinary.replaceUseReg(objBinary.getDst(),objMove.getSrc());
//					}
						if((objBinary.getDst().equals(objBinary.getSrc1())||objBinary.getDst().equals(objBinary.getSrc2()))&&objMove.getDst().equals(objBinary.getDst())){
							objBinary.replaceUseReg(objBinary.getDst(),objMove.getSrc());
							deleteList.add(instNode);
							stop=false;
						}
					}
			}
		}

		}
		return stop;
	}

	private HashMap<HashMap<ObjInstr,ObjInstr>,HashMap<ObjReg,ObjInstr>> initDataFlow(ObjBlock objBlock){
		HashMap<ObjInstr,ObjInstr> write2read=new HashMap<>();
		HashMap<ObjReg,ObjInstr> changeReg=new HashMap<>();
		for(IList.INode<ObjInstr,ObjBlock> instNode: objBlock.getInstrs()){
			ObjInstr nowInstr=instNode.getValue();
			ArrayList<ObjReg> writeReg=nowInstr.getWriteRegs();
			ArrayList<ObjReg> readReg=nowInstr.getReadRegs();
			if(nowInstr instanceof ObjBranch||nowInstr instanceof ObjCall||nowInstr instanceof ObjRet||nowInstr instanceof ObjStore){
				write2read.put(nowInstr,nowInstr);
			}
			for(ObjReg reg: writeReg){
				changeReg.put(reg,nowInstr);
			}
			for(ObjReg reg: readReg){
				if(changeReg.containsKey(reg)){
					write2read.put(changeReg.get(reg),nowInstr);
				}
			}
		}
		HashMap<HashMap<ObjInstr,ObjInstr>,HashMap<ObjReg,ObjInstr>> dataFlow=new HashMap<>();
		dataFlow.put(write2read,changeReg);
		return dataFlow;
	}

	private ObjInstr lastReader=null;
	private boolean writeSp=false;
	private IList.INode<ObjInstr,ObjBlock> nextInstNode=null;

	private boolean dataflow(ObjModule objModule){
		boolean stop=true;
		for(ObjFunction objFunction: objModule.getFunctions()){
			IList<ObjBlock,ObjFunction> blocks=objFunction.getObjBlocks();
			for(IList.INode<ObjBlock,ObjFunction> instBlock: blocks){
				ObjBlock objBlock=instBlock.getValue();
				ArrayList<ObjReg> out=objBlock.getLiveOuts();
				HashMap<HashMap<ObjInstr,ObjInstr>,HashMap<ObjReg,ObjInstr>> regPair=initDataFlow(objBlock);
				HashMap.Entry<HashMap<ObjInstr,ObjInstr>,HashMap<ObjReg,ObjInstr>> entry=regPair.entrySet().iterator().next();
				HashMap<ObjInstr,ObjInstr> write2read=entry.getKey();
				HashMap<ObjReg,ObjInstr> changeReg=entry.getValue();
//				System.out.println(write2read);
//				System.out.println(changeReg);
				for(IList.INode<ObjInstr,ObjBlock> instNode: objBlock.getInstrs()){
					ObjInstr nowInstr=instNode.getValue();
					int check1=0, check2=1;
					for(ObjReg reg: nowInstr.getWriteRegs()){
						if(!changeReg.get(reg).equals(nowInstr)){
							check1=1;
						}
						if(out.contains(reg)){
							check2=0;
						}
					}
//					boolean check1 = nowInstr.getWriteRegs().stream().allMatch(def -> changeReg.get(def).equals(nowInstr));
//					boolean check2 = nowInstr.getWriteRegs().stream().anyMatch(out::contains);
					if(check1==1||check2==1){
						if(!(nowInstr instanceof ObjBranch)||!((ObjBranch) nowInstr).isHasSrc()){
							lastReader=write2read.get(nowInstr);
							for(ObjReg reg: nowInstr.getWriteRegs()){
								if(reg.equals(ObjPhyReg.SP)){
									writeSp=true;
									break;
								}
							}
							nextInstNode=instNode.getNext();
							if(lastReader==null&&!writeSp){
//                                instNode.removeFromList();
								deleteList.add(instNode);
								stop=false;
								continue;
							}
							if(movDeleteReplace(instNode,objBlock)){
								stop=false;
							}
						}
					}
				}

			}
		}
		return stop;
	}

	private boolean movDeleteReplace(IList.INode<ObjInstr,ObjBlock> instNode,ObjBlock objBlock){
		ObjInstr nowInstr=instNode.getValue();
		if(nextInstNode==null){
			return false;
		}
		if(nowInstr instanceof ObjMove objMove){
			ObjOperand objSrc=objMove.getSrc();
			if(objSrc instanceof ObjImm12||objSrc instanceof ObjImm||objSrc instanceof ObjLabel){
				return false;
			}
			ObjInstr nextInstr=nextInstNode.getValue();
//			System.out.println(nowInstr+" "+nextInstr+" "+lastReader+"  111");
			if(lastReader==null||!(lastReader.equals(nextInstr))||nextInstr instanceof ObjCall||nextInstr instanceof ObjRet){
				return false;
			}
			ObjOperand objDst=objMove.getDst();
			nextInstr.replaceUseReg(objDst,objSrc);
//			System.out.println("222 "+nextInstr);
			deleteList.add(instNode);
			IList.INode<ObjInstr,ObjBlock> nextNode=nextInstNode;
			IList.INode<ObjInstr,ObjBlock> nowNode=nextNode;
//			System.out.println(objDst+" "+objSrc);
			while(nowNode!=null){
				nextNode=nowNode.getNext();
				if(nextNode==null){
					break;
				}
				nextInstr=nextNode.getValue();
//				System.out.print(nextInstr);
				if(!(nextInstr instanceof ObjCall||nextInstr instanceof ObjRet)){
					if(replaceReg(nextNode.getValue(),objDst,objSrc)){
						break;
					}
//					System.out.println("  =>  "+nextInstr);
				}
				nowNode=nowNode.getNext();
			}
			return true;
		}
		return false;
	}

	public boolean replaceReg(ObjInstr objInstr,ObjOperand objDst,ObjOperand objSrc){
		if(objInstr instanceof ObjBinary){
//			System.out.println(objInstr+""+objDst);
			ObjBinary objBinary=(ObjBinary) objInstr;
			if(objBinary.getSrc1().equals(objDst)||objBinary.getSrc2().equals(objDst)){
				objInstr.replaceUseReg(objDst,objSrc);

			}
			if(objBinary.getDst().equals(objDst)||objBinary.getDst().equals(objSrc)){
				return true;
			}
		}
		if(objInstr instanceof ObjLoad){
			ObjLoad objLoad=(ObjLoad) objInstr;
			if(objLoad.getAddr().equals(objDst)||objLoad.getOffset()!=null&&objLoad.getOffset().equals(objDst)){
				objInstr.replaceUseReg(objDst,objSrc);
			}
			if(objLoad.getDst().equals(objDst)||objLoad.getDst().equals(objSrc)){
				return true;
			}
		}
		if(objInstr instanceof ObjStore){
			ObjStore objStore=(ObjStore) objInstr;
			if(objStore.getSrc().equals(objDst)||objStore.getAddr().equals(objDst)||objStore.getOffset().equals(objDst)){
				objInstr.replaceUseReg(objDst,objSrc);
			}
		}
		if(objInstr instanceof ObjMove){
			ObjMove objmove=(ObjMove) objInstr;
			if(objmove.getSrc().equals(objDst)){
				objInstr.replaceUseReg(objDst,objSrc);
			}
			if(objmove.getDst().equals(objDst)||objmove.getDst().equals(objSrc)){
				return true;
			}
		}
		if(objInstr instanceof ObjBranch){
			ObjBranch objbranch=(ObjBranch) objInstr;
			if(objbranch.isHasSrc() && objbranch.getSrc().equals(objDst)){
				objInstr.replaceUseReg(objDst,objSrc);
			}
		}

		return false;
	}

	private boolean smallDataFlow(ObjModule objModule){
		boolean stop=true;
		for(ObjFunction objFunction: objModule.getFunctions()){
			IList<ObjBlock,ObjFunction> blocks=objFunction.getObjBlocks();
			for(IList.INode<ObjBlock,ObjFunction> instBlock: blocks){
				ObjBlock objBlock=instBlock.getValue();
				for(IList.INode<ObjInstr,ObjBlock> instNode: objBlock.getInstrs()){
					ObjInstr nowInstr=instNode.getValue();
					if(nowInstr instanceof ObjMove objMove){
						nextInstNode=instNode.getNext();
						if(nextInstNode==null){
							break;
						}
						ObjInstr nextInstr=nextInstNode.getValue();
						ObjOperand objSrc=objMove.getSrc();
						if(objSrc instanceof ObjImm12||objSrc instanceof ObjImm||objSrc instanceof ObjLabel){
							continue;
						}
//						System.out.println(nextInstr);
						if(nextInstr instanceof ObjCall||nextInstr instanceof ObjRet || (nextInstr instanceof ObjMove objMove1 && (objMove1.getSrc() instanceof ObjImm || objMove1.getSrc() instanceof ObjImm12))){
							continue;
						}
						ObjOperand objDst=objMove.getDst();
						if(objBlock.getLiveOuts().contains(objDst)){
							continue;
						}
//						System.out.println(objMove+" @ "+objMove.getDst());
						ObjPhyReg phyReg = (ObjPhyReg) objMove.getDst();
						if(phyReg.toString().equals(ObjPhyReg.indexToName.get(10))||phyReg.toString().equals(ObjPhyReg.indexToName.get(11))){continue;}
//						System.out.println(objSrc+" "+objDst);
//						System.out.println(objBlock.getLiveOuts());

						IList.INode<ObjInstr,ObjBlock> nextNode=nextInstNode;
						IList.INode<ObjInstr,ObjBlock> nowNode=nextNode;
						while(nowNode!=null){
							nextNode=nowNode.getNext();
							if(nextNode==null){
								break;
							}
							nextInstr=nowNode.getValue();
							if(nextInstr instanceof ObjCall||nextInstr instanceof ObjRet || (nextInstr instanceof ObjBranch objBranch && !objBranch.isHasSrc())){break;}
							if(!(nextInstr instanceof ObjCall||nextInstr instanceof ObjRet || (nextInstr instanceof ObjBranch objBranch && !objBranch.isHasSrc()))){
//								System.out.print(nextInstr);

								if(replaceReg(nextInstr,objDst,objSrc)){
//									System.out.println("  =>  "+nextInstr);
									deleteList.add(instNode);
									stop=false;
									break;
								}
//								System.out.println("  =>  "+nextInstr);
							}
							nowNode=nowNode.getNext();
						}

					}
				}
			}
		}
		return stop;
	}
}
