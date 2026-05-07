package Backend.process;

import Backend.component.ObjBlock;
import Backend.component.ObjFunction;
import Backend.component.ObjModule;
import Backend.instruction.*;
import Backend.operand.*;
import IR.Value.BasicBlock;
import IR.Value.Function;
import Utils.DataStruct.IList;
import Utils.DataStruct.Pair;
import jdk.swing.interop.SwingInterOpUtils;

import java.io.IOException;
import java.util.*;
import java.util.stream.Collectors;

import static Backend.operand.ObjPhyReg.*;
import static Backend.operand.ObjFPhyReg.*;
import static Utils.RISC_Dump.DumpObjModle;

public class RegAllo {
	private final ObjModule objModule;
	private static int rewritetime = 0;

	private int K;


	private boolean printLiveVar = false;

	private boolean printdebug = false;

	private boolean printassigncolor = false;

	private HashSet<ObjOperand> all = new HashSet<>();

	private HashSet<ObjOperand> initials = new HashSet<>();

	private HashMap<ObjFunction, Integer> originFuncStack = new HashMap<>();

	private HashSet<ObjOperand> S = new HashSet<>();

	private HashSet<ObjOperand> T = new HashSet<>();

	private int procedure;

	private int floatOrInt;

	private HashMap<ObjOperand, HashSet<ObjOperand>> adjList = new HashMap<>();

	private HashSet<Pair<ObjOperand, ObjOperand>> adjSet = new HashSet<>();

	private HashSet<ObjOperand> simplifyWorklist = new HashSet<>();

	private HashSet<ObjOperand> freezeWorklist = new HashSet<>();

	private HashSet<ObjOperand> spillWorklist = new HashSet<>();

	private HashMap<ObjOperand, ObjOperand> alias = new HashMap<>();

	private HashMap<ObjOperand, HashSet<ObjOperand>> meowalias = new HashMap<>();

	private HashSet<ObjOperand> spilledNodes = new HashSet<>();

	private HashSet<ObjOperand> coloredNodes = new HashSet<>();

	private HashSet<ObjOperand> coalescedNodes = new HashSet<>();

	private Stack<ObjOperand> selectStack = new Stack<>();

	private HashMap<ObjOperand, Integer> degree = new HashMap<>();

	private HashMap<ObjOperand, Integer> color = new HashMap<>();
	/**
	 * �µ�����Ĵ�������������������ʱ������µ�����Ĵ���
	 */
//	ObjVirReg vReg = null;

	HashMap<ObjOperand, Integer> loopDepths = new HashMap<>();
	private HashMap<ObjFunction, Integer> maxUseIa = new HashMap<>();
	private HashMap<ObjFunction, Integer> maxUseFa = new HashMap<>();


	public RegAllo(ObjModule objModule) {
		this.objModule = objModule;
	}

	public void run() {


		for (ObjFunction func : objModule.getFunctions()) {
			getNeedChangeDestination(func);
		}

		floatOrInt = 2;

		if (printdebug)
			System.out.println("allocate float");

		for (ObjFunction func : objModule.getFunctions()) {
			loop = 0;
			process(func);
			finalallocate(func);
			savestage(func);
		}

		floatOrInt = 1;
		if (printdebug)
			System.out.println("allocate int");
		for (ObjFunction func : objModule.getFunctions()) {
			loop = 0;
			process(func);
			finalallocate(func);
			savestage(func);
			checkmeow();
		}

		resetStackSize();

		for (ObjFunction func : objModule.getFunctions()) {
			resetNeedChangeDestination(func);
		}
		replaceliveinfo();

	}

	private void replaceliveinfo() {

		for (ObjFunction func : objModule.getFunctions()) {
			for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
				for(int i=0;i<bb.getValue().liveIns.size();i++)
				{
					ObjReg x = bb.getValue().liveIns.get(i);
					if(x instanceof ObjVirReg) {
						bb.getValue().liveIns.remove(i);
						bb.getValue().liveIns.add(i, ObjPhyReg.AllRegs.get(x.color));
					}
					if(x instanceof ObjFVirReg) {
						bb.getValue().liveIns.remove(i);
						bb.getValue().liveIns.add(i, ObjFPhyReg.AllRegs.get(x.color));
					}
				}
				for(int i=0;i<bb.getValue().liveOuts.size();i++)
				{
					ObjReg x = bb.getValue().liveOuts.get(i);
					if(x instanceof ObjVirReg) {
						bb.getValue().liveOuts.remove(i);
						bb.getValue().liveOuts.add(i, ObjPhyReg.AllRegs.get(x.color));
					}
					if(x instanceof ObjFVirReg) {
						bb.getValue().liveOuts.remove(i);
						bb.getValue().liveOuts.add(i, ObjFPhyReg.AllRegs.get(x.color));
					}
				}
				for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
					for(int i=0;i<inst.getValue().livein.size();i++)
					{
						ObjReg x = inst.getValue().livein.get(i);
						if(x instanceof ObjVirReg) {
							inst.getValue().livein.remove(i);
							inst.getValue().livein.add(i, ObjPhyReg.AllRegs.get(x.color));
						}
						if(x instanceof ObjFVirReg) {
							inst.getValue().livein.remove(i);
							inst.getValue().livein.add(i, ObjFPhyReg.AllRegs.get(x.color));
						}
					}
				}
			}
		}
	}

	private void checkmeow() {
		for (Pair<ObjOperand, ObjOperand> x : adjSet) {
			ObjOperand x1 = x.getFirst();
			ObjOperand x2 = x.getSecond();
			if (x1.color == x2.color)
				System.out.println("ALERT: " + x1 + " " + x2 + " -> " + ObjPhyReg.AllRegs.get(x1.color));
		}
	}

	private void resetNeedChangeDestination(ObjFunction func) {
		int newstacksize = func.getStackSize();
		int oldstacksize = originFuncStack.get(func);
		//System.out.println("newstacksize: " + newstacksize + " oldstacksize: " + oldstacksize);

		HashSet<ObjInstr> toberemoved = new HashSet<>();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				if (inst.getValue() instanceof ObjMove && ((ObjMove) (inst.getValue())).needchange) {

					ObjMove li = (ObjMove) (inst.getValue());
					int shouldbe = newstacksize + ((ObjImm) (li.getSrc())).getImmediate() - oldstacksize;
					li.setSrc(new ObjImm(shouldbe));
					boolean wenzitui = true;//一个不保证对的蚊子腿优化
					if (wenzitui)
						if (shouldbe >= -2048 && shouldbe < 2047) {
							assert inst.getNext() != null && inst.getNext().getValue() instanceof ObjBinary && inst.getNext().getNext() != null;

							if (inst.getNext().getNext().getValue() instanceof ObjLoad) {
								//li	t0,	272
								//	   add	t0,	sp,	t0
								//	lw	t1,	0(t0)
								//形如这种情况，删了前两句，改成lw t1, 272(sp)
								if (li.getDst().equals(((ObjBinary) (inst.getNext().getValue())).getDst()) && (((ObjBinary) (inst.getNext().getValue())).getSrc2()).equals(((ObjBinary) (inst.getNext().getValue())).getDst()) && (((ObjBinary) (inst.getNext().getValue())).getSrc2()).equals(((ObjLoad) (inst.getNext().getNext().getValue())).getAddr())) {
									toberemoved.add(inst.getNext().getValue());
									toberemoved.add(inst.getValue());
									ObjLoad ld = (ObjLoad) (inst.getNext().getNext().getValue());
									ld.setAddr(SP);
									ld.setOffset(new ObjImm12(shouldbe));
								}
							}
							if (inst.getNext().getNext().getValue() instanceof ObjStore) {
								//li	t0,	272
								//	   add	t0,	sp,	t0
								//	sw	t1,	0(t0)
								//形如这种情况，删了前两句，改成sw t1, 272(sp)
								if (li.getDst().equals(((ObjBinary) (inst.getNext().getValue())).getDst()) && (((ObjBinary) (inst.getNext().getValue())).getSrc2()).equals(((ObjBinary) (inst.getNext().getValue())).getDst()) && (((ObjBinary) (inst.getNext().getValue())).getSrc2()).equals(((ObjStore) (inst.getNext().getNext().getValue())).getAddr())) {
									toberemoved.add(inst.getNext().getValue());
									toberemoved.add(inst.getValue());
									ObjStore ld = (ObjStore) (inst.getNext().getNext().getValue());
									ld.setAddr(SP);
									ld.setOffset(new ObjImm12(shouldbe));
								}
							}

						}


				}
			}
		}
		for (ObjInstr x : toberemoved) {
			x.getNode().removeFromList();
		}

	}

	private void getNeedChangeDestination(ObjFunction func) {
		int stacksize = func.getStackSize();
		originFuncStack.put(func, stacksize);
	}

	private void resetStackSize() {
		for (ObjFunction func : objModule.getFunctions()) {
			ObjInstr spplus = func.getFirstBlock().getInstrs().getHead().getValue();
			ObjInstr spplus1 = func.getBbExit().getInstrs().getTail().getPrev().getValue();

			ObjOperand offset = getConstIntOperand(-func.getStackSize(), 12, func, spplus);
			((ObjBinary) spplus).setSrc2(offset);
			ObjOperand offset2 = getConstIntOperand(func.getStackSize(), 12, func, spplus1);
			((ObjBinary) spplus1).setSrc2(offset2);
		}
	}

	private boolean isS(int i) {
		return i == 8 || i == 9 || i >= 18 && i <= 27;
	}

	private ObjOperand genTmpReg(ObjFunction objFunction) {
		ObjVirReg tmpReg = new ObjVirReg();
		//System.out.println("generated "+tmpReg);
		objFunction.addUsedVirReg(tmpReg);
		return tmpReg;
	}

	private void getStore(ObjOperand src, ObjOperand addr, int immediate, String ty,
	                      boolean insertpos, ObjFunction objFunction, ObjInstr instr, int needallo) {
		// insertpos false: ???????, true: ???????
		if (immediate >= -2048 && immediate <= 2047) {
			ObjImm12 Imm = new ObjImm12(immediate);
			ObjStore objStore = new ObjStore(src, addr, Imm, ty);
			if (insertpos)
				objStore.getNode().insertAfter(instr.getNode());
			else
				objStore.getNode().insertBefore(instr.getNode());
		} else {
			ObjMove objMove = null;
			ObjBinary objAdd = null;
			ObjStore objStore = null;
			if (needallo == 1) {
				ObjImm tmpimm = new ObjImm(immediate);
				objMove = new ObjMove(ObjPhyReg.AllRegs.get(5), tmpimm);
				objAdd = ObjBinary.getAdd(ObjPhyReg.AllRegs.get(5), addr, ObjPhyReg.AllRegs.get(5));
				objStore = new ObjStore(src, ObjPhyReg.AllRegs.get(5), new ObjImm12(0), ty);
			} else {
				ObjOperand tmp = genTmpReg(objFunction);
				ObjImm tmpimm = new ObjImm(immediate);
				objMove = new ObjMove(tmp, tmpimm);
				ObjOperand addr2 = genTmpReg(objFunction);
				objAdd = ObjBinary.getAdd(addr2, addr, tmp);
				objStore = new ObjStore(src, addr2, new ObjImm12(0), ty);
//				System.out.println(objMove+"\n "+objMove.regDef+"\n "+objMove.regUse);
//				System.out.println(objAdd+"\n "+objAdd.regDef+"\n "+objAdd.regUse);
//				System.out.println(objStore+"\n "+objStore.regDef+"\n "+objStore.regUse);
			}
			if (insertpos) {
				objMove.getNode().insertAfter(instr.getNode());
				objAdd.getNode().insertAfter(objMove.getNode());
				objStore.getNode().insertAfter(objAdd.getNode());
			} else {
				objStore.getNode().insertBefore(instr.getNode());
				objAdd.getNode().insertBefore(objStore.getNode());
				objMove.getNode().insertBefore(objAdd.getNode());
			}
		}

	}

	private void getLoad(ObjOperand dst, ObjOperand addr, int immediate, String ty,
	                     boolean insertpos, ObjFunction objFunction, ObjInstr instr, int needallo) {

		// insertpos false: ???????, true: ???????
		if (immediate >= -2048 && immediate <= 2047) {
			ObjImm12 Imm = new ObjImm12(immediate);
			ObjLoad objLoad = new ObjLoad(dst, addr, Imm, ty);
			if (insertpos)
				objLoad.getNode().insertAfter(instr.getNode());
			else
				objLoad.getNode().insertBefore(instr.getNode());
		} else {
			ObjMove objMove = null;
			ObjBinary objAdd = null;
			ObjLoad objLoad = null;


			if (needallo == 1) {
				ObjImm tmpimm = new ObjImm(immediate);
				objMove = new ObjMove(ObjPhyReg.AllRegs.get(5), tmpimm);
				objAdd = ObjBinary.getAdd(ObjPhyReg.AllRegs.get(5), addr, ObjPhyReg.AllRegs.get(5));
				objLoad = new ObjLoad(dst, ObjPhyReg.AllRegs.get(5), new ObjImm12(0), ty);
			} else {
				ObjOperand tmp = genTmpReg(objFunction);
				ObjImm tmpimm = new ObjImm(immediate);
				objMove = new ObjMove(tmp, tmpimm);

				ObjOperand addr2 = genTmpReg(objFunction);
				objAdd = ObjBinary.getAdd(addr2, addr, tmp);

				objLoad = new ObjLoad(dst, addr2, new ObjImm12(0), ty);
			}
			if (insertpos) {

				objMove.getNode().insertAfter(instr.getNode());
				objAdd.getNode().insertAfter(objMove.getNode());
				objLoad.getNode().insertAfter(objAdd.getNode());
			} else {
				objLoad.getNode().insertBefore(instr.getNode());
				objAdd.getNode().insertBefore(objLoad.getNode());
				objMove.getNode().insertBefore(objAdd.getNode());
			}
		}
	}


	private void savestage(ObjFunction func) {
		HashSet<ObjPhyReg> changed = new HashSet<>();
		HashSet<ObjFPhyReg> changedf = new HashSet<>();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {

			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				/*
				�Ժ������޸Ĺ���S�Ĵ��������ֳ��ı���ͻָ�
				��һ���Ĵ����ظ�ֵ�ĵ���䣺
				1.ObjBinary
				2.lw, la
				3.move
				4.fcvt
				 */
				if (inst.getValue() instanceof ObjBinary) {
					if (((ObjBinary) inst.getValue()).getDst() instanceof ObjPhyReg && floatOrInt == 1) {
						ObjPhyReg dst = (ObjPhyReg) ((ObjBinary) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changed.add(dst);
						}
					}
					if (((ObjBinary) inst.getValue()).getDst() instanceof ObjFPhyReg && floatOrInt == 2) {
						ObjFPhyReg dst = (ObjFPhyReg) ((ObjBinary) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changedf.add(dst);
						}
					}

				}
				if (inst.getValue() instanceof ObjLoad) {
					if (((ObjLoad) inst.getValue()).getDst() instanceof ObjPhyReg && floatOrInt == 1) {
						ObjPhyReg dst = (ObjPhyReg) ((ObjLoad) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changed.add(dst);
						}
					}
					if (((ObjLoad) inst.getValue()).getDst() instanceof ObjFPhyReg && floatOrInt == 2) {
						ObjFPhyReg dst = (ObjFPhyReg) ((ObjLoad) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changedf.add(dst);
						}
					}
				}
				if (inst.getValue() instanceof ObjMove) {
					if (((ObjMove) inst.getValue()).getDst() instanceof ObjPhyReg && floatOrInt == 1) {
						ObjPhyReg dst = (ObjPhyReg) ((ObjMove) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changed.add(dst);
						}
					}
					if (((ObjMove) inst.getValue()).getDst() instanceof ObjFPhyReg && floatOrInt == 2) {
						ObjFPhyReg dst = (ObjFPhyReg) ((ObjMove) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changedf.add(dst);
						}
					}
				}
				if (inst.getValue() instanceof ObjConversion) {
					if (((ObjConversion) inst.getValue()).getDst() instanceof ObjPhyReg && floatOrInt == 1) {
						ObjPhyReg dst = (ObjPhyReg) ((ObjConversion) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changed.add(dst);
						}
					}
					if (((ObjConversion) inst.getValue()).getDst() instanceof ObjFPhyReg && floatOrInt == 2) {
						ObjFPhyReg dst = (ObjFPhyReg) ((ObjConversion) inst.getValue()).getDst();
						if (isS(dst.getIndex())) {
							changedf.add(dst);
						}
					}

				}

			}
		}
//		if (floatOrInt == 1)
//		{
//			changed.addAll(ObjPhyReg.A);
//		}
//		if (floatOrInt == 2)
//		{
//			changedf.addAll(ObjFPhyReg.A);
//		}
		if (floatOrInt == 1)
			for (ObjPhyReg x : changed) {
				x.spillPlace = func.getStackSize();
				getStore(x, SP, func.getStackSize(), "sd", true, func,
						func.getFirstBlock().getInstrs().getHeadValue(), 1);
				getLoad(x, SP, func.getStackSize(), "ld", false, func,
						func.getBbExit().getInstrs().getTail().getPrev().getValue(), 1);

				func.addAllocaSize(8);


			}
		if (floatOrInt == 2)
			for (ObjFPhyReg x : changedf) {
				x.spillPlace = func.getStackSize();
				getStore(x, SP, func.getStackSize(), "fsd", true, func,
						func.getFirstBlock().getInstrs().getHeadValue(), 1);

				getLoad(x, SP, func.getStackSize(), "fld", false, func,
						func.getBbExit().getInstrs().getTail().getPrev().getValue(), 1);

				func.addAllocaSize(8);

			}
	}

	private ObjOperand getConstIntOperand(int immediate, int canImm, ObjFunction objFunction,
	                                      ObjInstr nxt_instr) {
		ObjImm imm = new ObjImm(immediate);
		ObjImm12 imm12 = new ObjImm12(immediate);
		if (canImm == 32)
			return imm;
		else if (canImm == 12 && (immediate >= -2048 && immediate <= 2047))
			return imm12;
		else {
			ObjPhyReg dst = ObjPhyReg.AllRegs.get(5);
			objFunction.addUsedVirReg(dst);
			ObjMove objMove = new ObjMove(dst, imm);
			objMove.getNode().insertBefore(nxt_instr.getNode());
			return dst;
		}
	}

	private void init() {
		S = new HashSet<>();
		T = new HashSet<>();
		all = new HashSet<>();
		initials = new HashSet<>();
		adjList = new HashMap<>();
		adjSet = new HashSet<>();
		simplifyWorklist = new HashSet<>();
		freezeWorklist = new HashSet<>();
		spillWorklist = new HashSet<>();
		spilledNodes = new HashSet<>();
		coloredNodes = new HashSet<>();
		coalescedNodes = new HashSet<>();
		selectStack = new Stack<>();
		degree = new HashMap<>();
		color = new HashMap<>();
		alias = new HashMap<>();
		meowalias = new HashMap<>();
	}

	private void init_() {
		simplifyWorklist = new HashSet<>();
		freezeWorklist = new HashSet<>();
		spillWorklist = new HashSet<>();
		spilledNodes = new HashSet<>();
		coloredNodes = new HashSet<>();
		coalescedNodes = new HashSet<>();
		selectStack = new Stack<>();
		color = new HashMap<>();
		alias = new HashMap<>();
		meowalias = new HashMap<>();
	}

	private void GetDefUse(ObjFunction func) {
		maxUseIa.put(func, 9);
		maxUseFa.put(func, 9);

		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			bb.getValue().Use.clear();
			bb.getValue().Def.clear();
			bb.getValue().liveIns.clear();
			bb.getValue().liveOuts.clear();
			bb.getValue().LocalInterfere.clear();
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				for (ObjReg r : inst.getValue().regUse) {
					if (r instanceof ObjPhyReg) {
						ObjPhyReg pr = (ObjPhyReg) r;
						if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
							if (pr.getIndex() > maxUseIa.get(func))
								maxUseIa.put(func, pr.getIndex());
						}
					}
					if (r instanceof ObjFPhyReg) {
						ObjFPhyReg pr = (ObjFPhyReg) r;
						if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
							if (pr.getIndex() > maxUseFa.get(func))
								maxUseFa.put(func, pr.getIndex());
						}
					}

					if (floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg) {
						all.add(r);
					}

					if (!(bb.getValue().Use.contains(r)) && !(bb.getValue().Def.contains(r)) && ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg))) {//
						bb.getValue().Use.add(r);
					}
				}
				for (ObjReg r : inst.getValue().regDef) {
					if (r instanceof ObjPhyReg) {
						ObjPhyReg pr = (ObjPhyReg) r;
						if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
							if (pr.getIndex() > maxUseIa.get(func))
								maxUseIa.put(func, pr.getIndex());
						}
					}
					if (r instanceof ObjFPhyReg) {
						ObjFPhyReg pr = (ObjFPhyReg) r;
						if (pr.getIndex() >= 10 && pr.getIndex() <= 17) {
							if (pr.getIndex() > maxUseFa.get(func))
								maxUseFa.put(func, pr.getIndex());
						}
					}
					if (floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg) {
						all.add(r);
					}
					if (!(bb.getValue().Use.contains(r)) && !bb.getValue().Def.contains(r) && ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg))) {
						bb.getValue().Def.add(r);
					}
				}

			}

		}
	}

	private int GetInOut(ObjBlock bb) {

		//changed return 1
		ArrayList<ObjReg> InBackup = new ArrayList<>(bb.liveIns);
		bb.liveOuts.clear();
		for (ObjBlock bbb : bb.getNxtBlocks()) {
			for (ObjReg v : bbb.liveIns) {
				if (!bb.liveOuts.contains(v)) {
					bb.liveOuts.add(v);
				}
			}
		}
		bb.liveIns.clear();
		bb.liveIns.addAll(bb.Use);
		for (ObjReg v : bb.liveOuts) {
			if (!bb.liveIns.contains(v)) bb.liveIns.add(v);
		}
		bb.liveIns.removeIf(bb.Def::contains);
		boolean thesame = true;
		if (!(InBackup.size() == bb.liveIns.size())) thesame = false;
		for (ObjReg x : InBackup) {
			if (!bb.liveIns.contains(x)) thesame = false;
		}

		if (thesame) {
			return 0;
		} else return 1;
	}

	private ArrayList<ObjBlock> hasbeendfs = new ArrayList<>();

	private int dfs(ObjBlock bb) {
		//changed return 1
		if (hasbeendfs.contains(bb)) {
			return 0;
		}
		hasbeendfs.add(bb);
		int changed = GetInOut(bb);
		for (ObjBlock bbb : bb.getPreBlocks()) {
			if (dfs(bbb) == 1) changed = 1;
		}
		return changed;
	}

	private void DSFGetBBInOut(ObjFunction func) {

		ObjBlock tail = func.getBbExit();
		int changed = 1;
		while (changed == 1) {
			hasbeendfs.clear();
			changed = dfs(tail);
		}


	}

	private HashSet<ObjOperand> everused = new HashSet<>();

	private void GetInstInOut(ObjFunction func) {
		everused.clear();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			IList.INode<ObjInstr, ObjBlock> tmpinst = bb.getValue().getInstrs().getTail();
			ArrayList<ObjReg> tmpout = bb.getValue().liveOuts;
			while (tmpinst != null) {
				everused.addAll(tmpinst.getValue().regUse);

				if (tmpinst.getPrev() == null) {
					tmpinst.getValue().livein.clear();
					tmpinst.getValue().livein.addAll(bb.getValue().liveIns);
					bb.getValue().LocalInterfere.add(bb.getValue().liveIns);
					break;
				}
				ArrayList<ObjReg> tmpin = new ArrayList<>();
				ObjInstr ins = tmpinst.getValue();
				tmpin.addAll(tmpout);
//				System.out.println(ins);
//				System.out.println(ins.regDef);
//				System.out.println(ins.regUse);
				tmpin.removeIf(ins.regDef::contains);
				//tmpin.addAll(ins.regUse);
				for (ObjReg x : ins.regUse) {
					if ((floatOrInt == 1 && x instanceof ObjVirReg || floatOrInt == 2 && x instanceof ObjFVirReg) && !tmpin.contains(x))
						tmpin.add(x);
				}
				bb.getValue().LocalInterfere.add(tmpin);
				ins.livein.clear();
				ins.livein.addAll(tmpin);
//				System.out.println(ins);
//				System.out.println(ins.livein);
				tmpout = tmpin;
				tmpinst = tmpinst.getPrev();

			}
		}


	}

	private void LivenessAnalysis(ObjFunction func) {
		GetDefUse(func);
		DSFGetBBInOut(func);
		GetInstInOut(func);
		RemoveNotUsedDefination(func);
		FillST(func);
	}

	private void RemoveNotUsedDefination(ObjFunction func) {
		//如果有的虚拟寄存器只有定义但是没有使用，会导致这个虚拟寄存器不存在于任何指令或基本块的livein中
		//会被误认为这个虚拟寄存器任何节点都不活跃，导致其可以和任何其他虚拟寄存器分配给同一个物理寄存器，导致wa

		HashSet<ObjInstr> toberemoved = new HashSet<>();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				ObjInstr i = inst.getValue();
				if (!i.regDef.isEmpty() && ((i.regDef.get(0) instanceof ObjVirReg && floatOrInt == 1) || (i.regDef.get(0) instanceof ObjFVirReg && floatOrInt == 2)) && !everused.contains(i.regDef.get(0))) {
					toberemoved.add(i);
					all.remove(i.regDef.get(0));
				}
			}
		}
		for (ObjInstr i : toberemoved) {
			i.getNode().removeFromList();
		}
	}

	private void FillST(ObjFunction func) {
		S.clear();
		T.clear();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			S.addAll(bb.getValue().liveIns);
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				if (inst.getPrev() != null && inst.getPrev().getValue() instanceof ObjCall) {
					S.addAll(inst.getValue().livein);
				}
			}
		}
		for (ObjOperand x : all) {
			if (!S.contains(x)) T.add(x);
		}
		if (printdebug) {
			System.out.print("S: [");
			for (ObjOperand x : S) {
				System.out.print(x + ", ");
			}
			System.out.println("]");
			System.out.print("T: [");
			for (ObjOperand x : T) {
				System.out.print(x + ", ");
			}
			System.out.println("]");
		}
	}

	private void Build(ObjFunction func) {
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> tmpinst : bb.getValue().getInstrs()) {
				for (ObjReg r : tmpinst.getValue().regUse) {
//					loopDepths.put(r, bb.getValue().depth + 1);
					if ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg)) {
						if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
							degree.put(r, 0);
							adjList.put(r, new HashSet<>());
						}
					}
				}
				for (ObjReg r : tmpinst.getValue().regDef) {
//					loopDepths.put(r, bb.getValue().depth + 1);
					if ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg)) {
						if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
							degree.put(r, 0);
							adjList.put(r, new HashSet<>());
						}
					}
				}

			}
		}
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			int depth = bb.getValue().depth + 1;
			HashMap<ObjOperand, Integer> defanduse = new HashMap<>();
			for (IList.INode<ObjInstr, ObjBlock> tmpinst : bb.getValue().getInstrs()) {
				for (ObjReg r : tmpinst.getValue().regUse) {
					if ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg)) {
						if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
							defanduse.putIfAbsent(r, 0);
							int cur = defanduse.get(r);
							cur++;
							defanduse.put(r, cur);
						}
					}
				}
				for (ObjReg r : tmpinst.getValue().regDef) {
					if ((floatOrInt == 1 && r instanceof ObjVirReg || floatOrInt == 2 && r instanceof ObjFVirReg)) {
						if (procedure == 1 && S.contains(r) || procedure == 2 && T.contains(r)) {
							defanduse.putIfAbsent(r, 0);
							int cur = defanduse.get(r);
							cur++;
							defanduse.put(r, cur);
						}
					}
				}
			}
			for (HashMap.Entry<ObjOperand, Integer> entry : defanduse.entrySet()) {
				ObjOperand key = entry.getKey();
				int val = entry.getValue();
				loopDepths.putIfAbsent(key, 0);
				int cur = loopDepths.get(key);
				loopDepths.put(key, cur + 10 * depth * val);

			}

		}
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			ArrayList<ObjReg> ins = bb.getValue().liveIns;
			int len = ins.size();
			for (int i = 0; i < len; i++) {
				for (int j = i + 1; j < len; j++) {
					if (procedure == 1) {
						if (S.contains(ins.get(i)) && S.contains(ins.get(j))) AddEdge(ins.get(i), ins.get(j));
					} else {
						if (T.contains(ins.get(i)) && T.contains(ins.get(j))) AddEdge(ins.get(i), ins.get(j));
					}

				}
			}
			for (ArrayList<ObjReg> x : bb.getValue().LocalInterfere) {
				int len1 = x.size();
				for (int i = 0; i < len1; i++) {
					for (int j = i + 1; j < len1; j++) {
						if (procedure == 1) {
							if (S.contains(x.get(i)) && S.contains(x.get(j))) AddEdge(x.get(i), x.get(j));
						} else {
							if (T.contains(x.get(i)) && T.contains(x.get(j))) AddEdge(x.get(i), x.get(j));
						}
					}
				}
			}
		}

	}


	private void AddEdge(ObjOperand x, ObjOperand y) {
		if (!adjSet.contains(new Pair<>(x, y)) && !x.equals(y)) {
			if (printassigncolor)
				System.out.println("ADD EDGE " + x + " " + y);
			adjSet.add(new Pair<>(x, y));
			adjSet.add(new Pair<>(y, x));
			if (!x.isPrecolored()) {
				adjList.putIfAbsent(x, new HashSet<>());
				adjList.get(x).add(y);
				degree.compute(x, (key, value) -> value == null ? 0 : value + 1);

			}
			if (!y.isPrecolored()) {
				adjList.putIfAbsent(y, new HashSet<>());
				adjList.get(y).add(x);
				degree.compute(y, (key, value) -> value == null ? 0 : value + 1);
			}
		}

	}

	private void MakeWorkList() {
		for (ObjOperand o : initials) {

			if (degree.get(o) >= K) {
				//�߶����ڵ㣬Ǳ���������ɾ��
				spillWorklist.add(o);
			} else {
				//�Ͷ��������޹أ����ű�simplify
				simplifyWorklist.add(o);
			}
		}
	}

	private HashSet<ObjOperand> Adjacent(ObjOperand x) {
		//��ͻͼ�е����ڵĵ�

		HashSet<ObjOperand> ret = new HashSet<>(adjList.get(x));
		for (ObjOperand i : selectStack)
		//���ʽͼ��ɫ��������ʱɾ��ĵ��ջ
		{
			ret.remove(i);
		}
		for (ObjOperand i : coalescedNodes)
		//�Ѿ��ϲ��ļĴ������ϣ����u-<v�ϲ���һ�壬�Ͱ�v��������
		{
			ret.remove(i);
		}
		return ret;


	}

	private static int loop = 1;

	public void process(ObjFunction func) {

		{
			loop += 1;

			int enough = 0;
			init();
			if (printdebug)
				System.out.println(func.getName() + " LivenessAnalysis" + loop);

			LivenessAnalysis(func);

			if (printLiveVar && floatOrInt == 1)
				for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
					bb.getValue().printBbDetail();
				}
			procedure = 1;
			K = 12;


			initials = S;
			if (printdebug)
				System.out.println(func.getName() + " allocate s" + loop);
			if (printdebug)
				System.out.println(func.getName() + " build" + loop);
			Build(func);

			if (printdebug)
				System.out.println(func.getName() + " MakeWorkList" + loop);
			MakeWorkList();


			while (!(simplifyWorklist.isEmpty() && freezeWorklist.isEmpty() && spillWorklist.isEmpty())) {
				if (!simplifyWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " Simplify" + loop);
					Simplify();

				} else if (!freezeWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " Freeze" + loop);
					Freeze();
				} else if (!spillWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " SelectSpill" + loop);
					SelectSpill();

				}
			}
			if (printdebug)
				System.out.println(func.getName() + " AssignColors" + loop);
			AssignColors(func);
			if (spilledNodes.size() != 0) {
//			System.out.println("**REAL SPILL FOR S**" + loop);
				if (printdebug)
					System.out.println(spilledNodes);
				RewriteProgram(func);
				enough = 1;
				process(func);
			}
			if (enough == 1) {
				loop -= 1;
				return;
			}
			allocate();
			procedure = 2;
//			System.out.println(maxUseIa.get(func) + " " +maxUseFa.get(func));
			if (floatOrInt == 1) {
				K = 7;
				int maxa = maxUseIa.get(func);

				if (maxa < 17) {
					K += (17 - maxa);
				}
			}
			if (floatOrInt == 2) {
				K = 12;
				int maxa = maxUseFa.get(func);

				if (maxa < 17) {
					K += (17 - maxa);
				}
			}
			init_();
			initials = T;
			if (printdebug)
				System.out.println(func.getName() + " allocate t" + loop);
			if (printdebug)
				System.out.println(func.getName() + " build" + loop);
			Build(func);
			if (printdebug)
				System.out.println(func.getName() + " MakeWorkList" + loop);
			MakeWorkList();
			while (!(simplifyWorklist.isEmpty() && freezeWorklist.isEmpty() && spillWorklist.isEmpty())) {
				if (!simplifyWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " Simplify" + loop);
					Simplify();
				} else if (!freezeWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " Freeze" + loop);
					Freeze();
				} else if (!spillWorklist.isEmpty()) {
					if (printdebug)
						System.out.println(func.getName() + " SelectSpill" + loop);
					SelectSpill();

				}
			}
			if (printdebug)
				System.out.println(func.getName() + " AssignColors" + loop);
			AssignColors(func);
			if (spilledNodes.size() != 0) {
//			System.out.println("**REAL SPILL FOR T**" + loop);
				if (printdebug)
					System.out.println(spilledNodes);
				RewriteProgram(func);
				enough = 1;
				process(func);
			}
			if (enough == 1) {
				loop -= 1;
				return;
			}
			allocate();
		}

	}

	public void allocate() {
		for (HashMap.Entry<ObjOperand, Integer> entry : color.entrySet()) {
			ObjOperand key = entry.getKey();
			int val = entry.getValue();
			key.color = val;
			if (printassigncolor) {
				if (key instanceof ObjVirReg)
					System.out.println(key + " -> " + ObjPhyReg.indexToName.get(val));
				if (key instanceof ObjFVirReg)
					System.out.println(key + " -> " + ObjFPhyReg.indexToName.get(val));
			}
		}
	}

	public void finalallocate(ObjFunction func) {
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				ArrayList<ObjReg> xx = new ArrayList<>();
				xx.addAll(inst.getValue().regUse);
				for (ObjReg x : xx) {

					if ((floatOrInt == 1 && x instanceof ObjVirReg || floatOrInt == 2 && x instanceof ObjFVirReg)) {
						assert x.color != -1;
						if (floatOrInt == 1) inst.getValue().replaceReg(x, ObjPhyReg.AllRegs.get(x.color));
						if (floatOrInt == 2) inst.getValue().replaceReg(x, ObjFPhyReg.AllRegs.get(x.color));
					}
				}
				xx.clear();
				xx.addAll(inst.getValue().regDef);
				for (ObjReg x : xx) {

					if ((floatOrInt == 1 && x instanceof ObjVirReg || floatOrInt == 2 && x instanceof ObjFVirReg)) {
						assert x.color != -1;
						if (floatOrInt == 1) inst.getValue().replaceReg(x, ObjPhyReg.AllRegs.get(x.color));
						if (floatOrInt == 2) inst.getValue().replaceReg(x, ObjFPhyReg.AllRegs.get(x.color));
					}
				}
			}
		}
		ArrayList<ObjInstr> toberemoved = new ArrayList<>();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				if (inst.getValue() instanceof ObjMove && ((ObjMove) inst.getValue()).getDst().equals(((ObjMove) inst.getValue()).getSrc())) {
					toberemoved.add(inst.getValue());
				}
			}
		}
		for (ObjInstr i : toberemoved) {
			i.getNode().removeFromList();
		}
	}


	private void RewriteProgram(ObjFunction func) {

		int nowoffset = func.getStackSize();
		HashSet<ObjInstr> needrewrite = new HashSet<>();
		for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
			for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
				ArrayList<ObjOperand> tmp = new ArrayList<>(inst.getValue().regUse);
				for (ObjReg j : inst.getValue().regDef) tmp.remove(j);
				tmp.addAll(inst.getValue().regDef);
				for (ObjOperand x : tmp) {
					if (spilledNodes.contains(x)) {
						needrewrite.add(inst.getValue());
						if (x.spillPlace == -1) {
							x.spillPlace = nowoffset;
							nowoffset += 8;
							func.addAllocaSize(8);
						}
					}
				}


			}

		}
		for (ObjInstr i : needrewrite) {

			for (ObjOperand x : i.regUse) {
				if (spilledNodes.contains(x)) {
					if (x instanceof ObjFVirReg) {
						ObjFVirReg xx = new ObjFVirReg();
						getLoad(x, SP, x.spillPlace, "fld", false, func, i.getNode().getValue(), 0);
					} else {
						getLoad(x, SP, x.spillPlace, "ld", false, func, i.getNode().getValue(), 0);
					}
				}
			}
			for (ObjOperand x : i.regDef) {
				if (spilledNodes.contains(x)) {
					if (x instanceof ObjFVirReg)
						getStore(x, SP, x.spillPlace, "fsd", true, func, i.getNode().getValue(), 0);
					else
						getStore(x, SP, x.spillPlace, "sd", true, func, i.getNode().getValue(), 0);
				}
			}

		}


//		if (printdebug && rewritetime < 3) {
//			try {
//				DumpObjModle(objModule, "rewrite_" + rewritetime + ".asm");
//				rewritetime += 1;
//			} catch (IOException e) {
//				System.out.println(e);
//			}
//		}

	}

	private void AssignColors(ObjFunction func) {
		while (!selectStack.empty()) {
			ObjOperand n = selectStack.pop();
			HashSet<Integer> okColors = new HashSet<>();
			if (floatOrInt == 1) {
				if (procedure == 1)//S
				{
					okColors.add(8);
					okColors.add(9);
					for (int i = 18; i <= 27; i++) {
						okColors.add(i);
					}
				} else//T
				{

					okColors.add(5);
					okColors.add(6);
					okColors.add(7);
					for (int i = 28; i <= 31; i++) {
						okColors.add(i);
					}
					int maxa = maxUseIa.get(func);
					for (int ii = maxa + 1; ii <= 17; ii++) {
						okColors.add(ii);
					}

				}
			}
			if (floatOrInt == 2) {
				if (procedure == 1)//S
				{
					okColors.add(8);
					okColors.add(9);
					for (int i = 18; i <= 27; i++) {
						okColors.add(i);
					}
				} else//T
				{

					for (int i = 0; i <= 7; i++) {
						okColors.add(i);
					}
					for (int i = 28; i <= 31; i++) {
						okColors.add(i);
					}
					int maxa = maxUseFa.get(func);
					for (int ii = maxa + 1; ii <= 17; ii++) {
						okColors.add(ii);
					}
				}
			}
			if (printassigncolor) {
				System.out.println("assigning color");
				System.out.println(n);
				System.out.println("adj and all its alias");
				System.out.println(adjList.get(n));
			}
			for (ObjOperand w : adjList.get(n)) {
				if (printassigncolor) {
					System.out.print(w);
					System.out.println(": " + GetAlias(w));
				}
				if (coloredNodes.contains(GetAlias(w))) {
					if (printassigncolor) System.out.println("remove its color : " + color.get(GetAlias(w)));
					okColors.remove(color.get(GetAlias(w)));
				}
			}
			if (okColors.isEmpty()) {
				spilledNodes.add(n);
			} else {
				coloredNodes.add(n);
				int c = okColors.iterator().next();
				okColors.remove(c);
				if (printassigncolor) System.out.println("give " + n + " " + c);
				color.put(n, c);
			}
		}
		if (printassigncolor) System.out.println("give coalescedNodes color");
		for (ObjOperand n : coalescedNodes) {
			if (printassigncolor) System.out.println(n + " : " + GetAlias(n) + ": " + color.get(GetAlias(n)));
			color.put(n, color.get(GetAlias(n)));
		}
	}

	HashSet<ObjOperand> spilled = new HashSet<>();

	private void SelectSpill() {
		ArrayList<ObjOperand> myspilllist = new ArrayList<>(spillWorklist);
		Collections.shuffle(myspilllist);
		ObjOperand m = myspilllist.stream().max((l, r) ->
		{
			double value1 = degree.getOrDefault(l, 0).doubleValue() / loopDepths.getOrDefault(l, 0);
			double value2 = degree.getOrDefault(r, 0).doubleValue() / loopDepths.getOrDefault(r, 0);
//			if(l.spillPlace!=-1) value1=-999999;
//			if(r.spillPlace!=-1) value2=-999999;
			return Double.compare(value1, value2);
		}).get();
		spillWorklist.remove(m);
		spilled.add(m);
		simplifyWorklist.add(m);

		spillWorklist.remove(m);
	}

	private void Freeze() {
		ObjOperand u = freezeWorklist.iterator().next();
		freezeWorklist.remove(u);
		simplifyWorklist.add(u);
	}


	private ObjOperand GetAlias(ObjOperand n) {
		if (coalescedNodes.contains(n)) {
			return GetAlias(alias.get(n));
		} else return n;
	}

	private void Simplify() {
//		System.out.println("simplify");
//		System.out.println(simplifyWorklist);
		//��ͻͼ��ȫ�ǵͶ��������޹ص㣬��Ҫһ�����ŵ�ջ��ɾ��

		ObjOperand n = simplifyWorklist.iterator().next();
		simplifyWorklist.remove(n);
		selectStack.push(n);
		for (ObjOperand m : Adjacent(n)) {
			DecrementDegree(m);
		}
	}

	private void DecrementDegree(ObjOperand m) {
		int d = degree.get(m);
		degree.put(m, d - 1);

		if (d == K) {
			HashSet<ObjOperand> tmp = new HashSet<>(Adjacent(m));
			tmp.add(m);

			spillWorklist.remove(m);
			{
				simplifyWorklist.add(m);
			}
		}

	}


}
