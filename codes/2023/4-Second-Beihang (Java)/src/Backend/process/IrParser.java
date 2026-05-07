package Backend.process;

import Backend.component.ObjBlock;
import Backend.component.ObjFunction;
import Backend.component.ObjGlobalVariable;
import Backend.component.ObjModule;
import Backend.instruction.*;
import Backend.operand.*;
import Driver.Config;
import Frontend.AST;
import IR.IRModule;
import IR.Type.*;
import IR.Value.*;
import IR.Value.Instructions.*;
import IR.Visitor;
import Utils.DataStruct.IList;
import Utils.DataStruct.Pair;
import jdk.swing.interop.SwingInterOpUtils;

import java.awt.*;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.RoundingMode;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Optional;

import static Backend.operand.ObjPhyReg.*;

public class IrParser {
	// test Meow
	private IRModule irModule;
	private ObjModule objModule;
	private HashMap<Function, ObjFunction> fMap;
	private HashMap<BasicBlock, ObjBlock> bMap;
	private HashMap<GlobalVar, ObjGlobalVariable> gMap;
	private HashMap<Value, ObjOperand> operandMap;
	public HashMap<ObjOperand, Value> operandMap1;
	private HashMap<Pair<ObjBlock, Pair<ObjOperand, ObjOperand>>, ObjOperand> divMap;

	private final static ObjFunction PrintFunc = new ObjFunction("@putint", true);
	private final static ObjFunction InputFunc = new ObjFunction("@getint", true);
	private final static ObjFunction PrintChFunc = new ObjFunction("@putch", true);
	private final static ObjFunction InputChFunc = new ObjFunction("@getch", true);
	private final static ObjFunction PrintArrFunc = new ObjFunction("@putarray", true);
	private final static ObjFunction InputArrFunc = new ObjFunction("@getarray", true);
	private final static ObjFunction PrintFloatFunc = new ObjFunction("@putfloat", true);
	private final static ObjFunction InputFloatFunc = new ObjFunction("@getfloat", true);
	private final static ObjFunction PrintFArrFunc = new ObjFunction("@putfarray", true);
	private final static ObjFunction InputFArrFunc = new ObjFunction("@getfarray", true);
	private final static ObjFunction MemsetFunc = new ObjFunction("@memset", true);
	private final static ObjFunction StartTimeFunc = new ObjFunction("@_sysy_starttime", true);
	private final static ObjFunction StopTimeFunc = new ObjFunction("@_sysy_stoptime", true);

	private HashMap<String, ObjFunction> Library_functions;

	public IrParser(IRModule irModule) {

		this.irModule = irModule;
		objModule = new ObjModule();
		fMap = new HashMap<>();
		bMap = new HashMap<>();
		gMap = new HashMap<>();
		operandMap = new HashMap<>();
		operandMap1 = new HashMap<>();
		Library_functions = new HashMap<>();
		divMap = new HashMap<>();

		Library_functions.put("@putint", PrintFunc);
		Library_functions.put("@getint", InputFunc);
		Library_functions.put("@putch", PrintChFunc);
		Library_functions.put("@getch", InputChFunc);
		Library_functions.put("@putarray", PrintArrFunc);
		Library_functions.put("@getarray", InputArrFunc);
		Library_functions.put("@putfloat", PrintFloatFunc);
		Library_functions.put("@getfloat", InputFloatFunc);
		Library_functions.put("@putfarray", PrintFArrFunc);
		Library_functions.put("@getfarray", InputFArrFunc);
		Library_functions.put("@memset", MemsetFunc);
		Library_functions.put("@_sysy_starttime", StartTimeFunc);
		Library_functions.put("@_sysy_stoptime", StopTimeFunc);

		SP.notNeedsColor();
		RA.notNeedsColor();
		for (int i = 0; i < 8; i++)
			A.get(0).notNeedsColor();
	}

	public ObjModule parseModule() {
		parseGlobalVariables();
		parseFunctions();
		changeFunctionNames();
		return objModule;
	}

	private void changeFunctionNames() {
		for (ObjFunction func : objModule.getFunctions()) {
			for (IList.INode<ObjBlock, ObjFunction> bb : func.getObjBlocks()) {
				bb.getValue().setName(func.getName() + "_" + bb.getValue().getName());
			}
		}

	}

	private void parseGlobalVariables() {
		for (GlobalVar g : irModule.getGlobalVars()) {
			ObjGlobalVariable objGlobalVariable = parseGlobalVariable(g);
			objModule.addGlobalVariable(objGlobalVariable);

			gMap.put(g, objGlobalVariable);
		}
	}

	private ObjGlobalVariable parseGlobalVariable(GlobalVar g) {

		Type type = ((PointerType) g.getType()).getEleType();
		ArrayList<Integer> elements = new ArrayList<>();

		if (type instanceof IntegerType) {
			Value initValue = g.getValue();
			if (initValue != null) {
				int intValue = ((ConstInteger) initValue).getValue();
				elements.add(intValue);
			}

		} else if (type instanceof FloatType) {
			Value initValue = g.getValue();
			if (initValue != null) {
				float floatValue = ((ConstFloat) initValue).getValue();
				int intValue = Float.floatToRawIntBits(floatValue);
				elements.add(intValue);
			}
		} else if (g.isArray()) {
			if (!g.isAllZero()) {
				ArrayList<Value> init = g.getValues();
				for (Value value : init) {
					int intValue = 0;
					if (value.getType() instanceof IntegerType)
						intValue = ((ConstInteger) value).getValue();
					else if (value.getType() instanceof FloatType) {
						float floatValue = ((ConstFloat) value).getValue();
						intValue = Float.floatToRawIntBits(floatValue);
					}
					elements.add(intValue);
				}
			}
		}

		if (elements.isEmpty()) {
			int totSize = 4;
			if (g.isArray())
				totSize = 4 * ((ArrayType) type).getTotalSize();
			ObjGlobalVariable objGlobalVariable = new ObjGlobalVariable(g.getName(), totSize);
			return objGlobalVariable;
		} else {
			ObjGlobalVariable objGlobalVariable = new ObjGlobalVariable(g.getName(), elements);
			return objGlobalVariable;
		}

	}

	private void iMap() {

		for (Function f : irModule.getFunctions()) {
			ObjFunction objFunction = new ObjFunction(f.getName(), f.isLibFunction());
			objModule.addFunction(objFunction);
			fMap.put(f, objFunction);

			for (IList.INode<BasicBlock, Function> b : f.getBbs()) {
				BasicBlock irBlock = b.getValue();
				ObjBlock objBlock = new ObjBlock(irBlock.getName());
				bMap.put(irBlock, objBlock);

				objFunction.addBlocks(objBlock);
			}
		}

		for (Function f : irModule.getFunctions())
			for (IList.INode<BasicBlock, Function> b : f.getBbs()) {
				BasicBlock irBlock = b.getValue();
				ObjBlock objBlock = bMap.get(irBlock);
//				System.out.println(irBlock.getLoopDepth());
				objBlock.depth = irBlock.getLoopDepth();
				ArrayList<BasicBlock> preBlocks = irBlock.getPreBlocks();
				for (BasicBlock preB : preBlocks)
					objBlock.addPreBlock(bMap.get(preB));
				ArrayList<BasicBlock> nxtBlocks = irBlock.getNxtBlocks();
				for (BasicBlock nxtB : nxtBlocks)
					objBlock.addNxtBlock(bMap.get(nxtB));
			}
	}

	private void parseFunctions() {
		iMap();
		for (Function f : irModule.getFunctions())
			if (!f.isLibFunction())
				parseFunction(f);
	}

	private void parseFunction(Function f) {
		ObjFunction objF = fMap.get(f);
		int now = 0;
		for (IList.INode<BasicBlock, Function> b : f.getBbs()) {
			BasicBlock irBlock = b.getValue();
			for (IList.INode<Instruction, BasicBlock> inst : irBlock.getInsts()) {
				Instruction irInst = inst.getValue();
				if (irInst instanceof CallInst) {
					CallInst ca = (CallInst) irInst;
					if (!(f.istailrecursive && ca.getFunction().equals(f))) {
						ArrayList<Value> operands = irInst.getOperands();
						int arg_nums = operands.size();
						for (int ii = 8; ii < arg_nums; ii++) {
							if (operands.get(ii).getType() instanceof PointerType) {
								now += 8;
							} else {
								now += 4;
							}
						}
					}
				}
			}
		}

		objF.setArgsSize(now);
		objF.setRsize();

		for (IList.INode<BasicBlock, Function> b : f.getBbs()) {
			BasicBlock irBlock = b.getValue();
			parseBlock(irBlock, f);
		}


		ObjBlock tmpb = bMap.get(f.getBbExit());
		objF.setBbExit(tmpb);
		objF.isrecursive = f.istailrecursive;
//		if(objF.isrecursive )
//		System.out.println(objF.getName()+" is tail recursive");

		for (Argument x : f.getArgs()) {
			if (x.getType() instanceof FloatType) {
				objF.fargnum += 1;
			} else {
				objF.iargnum += 1;
			}
		}
		BasicBlock b = f.getBbs().getHead().getValue();

		ArrayList<Argument> args = f.getArgs();
		ArrayList<Argument> fargs = new ArrayList<>();
		ArrayList<Argument> iargs = new ArrayList<>();
		for (Argument i : args) {
			if (i.getType().isFloatTy()) fargs.add(i);
			else iargs.add(i);
		}
		int num_f = fargs.size();
		int num_i = iargs.size();
		if (num_f > 8)
			num_f = 8;
		for (int i = 0; i < num_f; i++) {
			Value arg = fargs.get(i);
			ObjOperand operand = parseOperand(arg, 0, f, b);
			ObjMove objMove = new ObjMove(operand, ObjFPhyReg.A.get(i));
			objF.getFirstBlock().addInstrHead(objMove);
		}
		if (num_i > 8)
			num_i = 8;
		for (int i = 0; i < num_i; i++) {
			Value arg = iargs.get(i);
			ObjOperand operand = parseOperand(arg, 0, f, b);
			ObjMove objMove = new ObjMove(operand, ObjPhyReg.A.get(i));
			objF.getFirstBlock().addInstrHead(objMove);
		}
		int off = objF.getStackSize();
		num_f = fargs.size();
		num_i = iargs.size();
		for (int i = 8; i < num_f; i++) {
			Value arg = fargs.get(i);
			ObjOperand operand = parseOperand(arg, 0, f, b);
			String ty = "flw";
			getLoad(operand, SP, off, ty, false, objF, objF.getFirstBlock().getInstrs().getHead().getValue(), 1);
			off += 4;
		}
		for (int i = 8; i < num_i; i++) {

			Value arg = iargs.get(i);
			ObjOperand operand = parseOperand(arg, 0, f, b);
			String ty = null;
			if (arg.getType() instanceof IntegerType) {
				ty = "lw";
			} else {
				ty = "ld";
			}


			getLoad(operand, SP, off, ty, false, objF, objF.getFirstBlock().getInstrs().getHead().getValue(), 1);

			if (arg.getType() instanceof IntegerType) {
				off += 4;
			}  else {
				off += 8;
			}
		}

		if (objF.isrecursive) {
			HashSet<ObjInstr> toberemoved = new HashSet<>();
//			System.out.println(objF.getName()+" is tail recursive");
			ObjBlock start = new ObjBlock("bstart");
			ObjBlock prestart = objF.getObjBlocks().getHeadValue();
			start.addInstr(new ObjBranch(objF.getObjBlocks().getHeadValue()));
			start.getNode().insertListHead(objF.getObjBlocks());


			for (IList.INode<ObjBlock, ObjFunction> bb : objF.getObjBlocks()) {
				int flag = 0;
				for (IList.INode<ObjInstr, ObjBlock> inst : bb.getValue().getInstrs()) {
					if (inst.getValue() instanceof ObjCall) {
						ObjCall call = (ObjCall) (inst.getValue());
						if (call.tarFunction.equals(objF)) {
							ObjBranch br = new ObjBranch(prestart);
							br.getNode().insertBefore(inst);
							toberemoved.add(inst.getValue());
							flag = 1;
							continue;
						}
						if (flag == 1) {
							toberemoved.add(inst.getValue());
						}

					}
				}
			}
			for (ObjInstr to : toberemoved) {
				to.getNode().removeFromList();
			}

//			ObjBlock end=new ObjBlock("bend");
//			end.addInstr(new ObjBranch(objF.getBbExit()));
//			end.getNode().insertBefore(objF.getBbExit().getNode());

		}
		if (objF.getRsize() > 0) {
			getStore(RA, SP, objF.getStackSize() - 8, "sd", false,
					objF, objF.getFirstBlock().getInstrs().getHeadValue(), 0);

			getLoad(RA, SP, objF.getStackSize() - 8, "ld", false,
					objF, objF.getBbExit().getInstrs().getTail().getValue(), 0);
		}


		ObjOperand alloc = parseConstIntOperand(-objF.getStackSize(), 12, f, b);
		ObjBinary add = ObjBinary.getAdd(ObjPhyReg.SP, ObjPhyReg.SP, alloc);
		objF.getFirstBlock().addInstrHead(add);


		ObjOperand alloc1 = parseConstIntOperand(objF.getStackSize(), 12, f, b);
		ObjBinary add1 = ObjBinary.getAdd(ObjPhyReg.SP, ObjPhyReg.SP, alloc1);
		add1.getNode().insertBefore(objF.getBbExit().getInstrs().getTail());


	}

	private void parseBlock(BasicBlock b, Function f) {
		for (IList.INode<Instruction, BasicBlock> inst : b.getInsts()) {
			Instruction irInst = inst.getValue();
			parseInstruction(irInst, b, f);
		}
	}

	private void parseInstruction(Instruction irInst, BasicBlock irBlock, Function irFunction) {

		// System.out.println(irInst instanceof  CmpInst);
		if (irInst instanceof RetInst)
			parseRet((RetInst) irInst, irBlock, irFunction);
		else if (irInst instanceof AllocInst)
			parseAlloca((AllocInst) irInst, irBlock, irFunction);
		else if (irInst instanceof LoadInst)
			parseLoad((LoadInst) irInst, irBlock, irFunction);
		else if (irInst instanceof StoreInst)
			parseStore((StoreInst) irInst, irBlock, irFunction);
		else if (irInst instanceof CmpInst)
			parseCmp((CmpInst) irInst, irBlock, irFunction);

		else if (irInst instanceof BinaryInst) {
			if (irInst.getOp() == OP.Add)
				parseAdd((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Fadd)
				parseFAdd((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Mul)
				parseMul((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Fmul)
				parseFMul((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Sub)
				parseSub((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Fsub)
				parseFSub((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Div)
				parseDiv((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Fdiv)
				parseFDiv((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Mod)
				parseMod((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.And)
				parseAnd((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Or)
				parseOr((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Xor)
				parseXor((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Shl)
				parseShl((BinaryInst) irInst, irBlock, irFunction);
			if (irInst.getOp() == OP.Shr)
				parseShr((BinaryInst) irInst, irBlock, irFunction);
		} else if (irInst instanceof BrInst)
			parseBr((BrInst) irInst, irBlock, irFunction);
		else if (irInst instanceof CallInst)
			parseCall((CallInst) irInst, irBlock, irFunction);
		else if (irInst instanceof GepInst)
			parseGep((GepInst) irInst, irBlock, irFunction);
		else if (irInst instanceof Move)
			parseMove((Move) irInst, irBlock, irFunction);
		else if (irInst instanceof ConversionInst)
			parseConversionInst((ConversionInst) irInst, irBlock, irFunction);

	}

	private void parseConversionInst(ConversionInst inst, BasicBlock irBlock, Function irFunction) {
//		System.out.println(inst);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
		ObjOperand src = parseOperand(inst.getOperand(0), 0, irFunction, irBlock);
		ObjBlock objBlock = bMap.get(irBlock);
		if (inst.getOp() == OP.Ftoi) { // float to I32
			ObjConversion Ftoi = ObjConversion.getFtoi(dst, src);
//			System.out.println(Ftoi);
			objBlock.addInstr(Ftoi);
		} else if (inst.getOp() == OP.Itof) { // I32 to float
			ObjConversion Itof = ObjConversion.getItof(dst, src);
			objBlock.addInstr(Itof);
		} else if (inst.getOp() == OP.Zext) { // I1 to I32
			ObjMove objMove = new ObjMove(dst, src);
			objBlock.addInstr(objMove);
		} else if (inst.getOp() == OP.BitCast) { // to I32*
			ObjMove objMove = new ObjMove(dst, src);
			objBlock.addInstr(objMove);
		}
	}

	private void parseMove(Move inst, BasicBlock irBlock, Function irFunction) {

		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src = parseOperand(inst.getOperand(0), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
		ObjMove objMove = new ObjMove(dst, src);
		objBlock.addInstr(objMove);
	}

	private int isTimesTwo(int n) {
		if (!(n > 0 && (n & (n - 1)) == 0)) {
			return -1;
		}
		return Integer.toBinaryString(n).length() - 1;
	}


	private void parseGep(GepInst inst, BasicBlock irBlock, Function irFunction) {
		ObjFunction objFunction = fMap.get(irFunction);
		ObjBlock objBlock = bMap.get(irBlock);


		ArrayList<Value> values = inst.getUseValues();
		ArrayList<Value> indexs = inst.getIndexs();

		ObjOperand baseOperand = parseOperand(values.get(0), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjMove objMove = new ObjMove(dst, baseOperand);
		objBlock.addInstr(objMove);

		PointerType imsb = (PointerType) (values.get(0).getType());
		ArrayList<Integer> dims = new ArrayList<>();
		ArrayList<Integer> sizes = new ArrayList<>();
		if (imsb.getEleType().isArrayType())//��ά
		{
			ArrayType imsb1 = (ArrayType) (imsb.getEleType());
			int dim = imsb1.getDim();
			dims.add(1);
			for (int i = 0; i < dim - 1; i++) {
//				System.out.println(imsb1);
				dims.add(imsb1.getNum());
				imsb1 = (ArrayType) (imsb1.getEleType());
			}
			dims.add(imsb1.getNum());


			for (int i = dim; i >= 0; i--) {
				if (sizes.isEmpty()) sizes.add(4);
				else {
					sizes.add(0, sizes.get(0) * dims.get(i + 1));
				}
			}

		}
//		if(indexs.size()>1)
//			indexs.remove(0);


		if (sizes.isEmpty())
			sizes.add(4);
//		System.out.println(inst.getInstString());
//		System.out.println("origin array dim: "+dims.size());
//		System.out.println("each dim length: "+dims);
//		System.out.println("each dim size: "+sizes);
//		System.out.println("user get dim: "+(indexs.size()));
//		System.out.println("each dim index: "+indexs);
		assert sizes.size() == indexs.size();
		for (int i = indexs.size() - 1; i >= 0; i--) {
			if (indexs.get(i) instanceof ConstInteger)//����±��ǳ���
			{
				int offset = sizes.get(i) * ((ConstInteger) (indexs.get(i))).getValue();
				if (offset == 0)
					continue;
//				if(offset == 2048)
//					System.out.println(offset);
				ObjOperand offset_operand = parseConstIntOperand(offset, 12, irFunction, irBlock);
				ObjBinary bin = ObjBinary.getAdd(dst, dst, offset_operand);
				// System.out.println(bin);
				objBlock.addInstr(bin);
			} else {
				ObjOperand mul1 = parseOperand(indexs.get(i), 0, irFunction, irBlock);
				ObjOperand tmp = genTmpReg(irFunction);
				getMul_(mul1, sizes.get(i), tmp, irBlock, irFunction);
				ObjBinary bin1 = ObjBinary.getAdd(dst, dst, tmp);
				objBlock.addInstr(bin1);

//				if (isTimesTwo(sizes.get(i)) != -1) {
//					int x = isTimesTwo(sizes.get(i));
//					ObjOperand mul1 = parseOperand(indexs.get(i), 0, irFunction, irBlock);
//					ObjOperand mul2 = parseConstIntOperand(x, 12, irFunction, irBlock);
//					ObjOperand tmp = genTmpReg(irFunction);
//					ObjBinary bin = ObjBinary.getSll(tmp, mul1, mul2);
//					objBlock.addInstr(bin);
//					ObjBinary bin1 = ObjBinary.getAdd(dst, dst, tmp);
//					objBlock.addInstr(bin1);
//
//				} else {
//					ObjOperand mul1 = parseOperand(indexs.get(i), 0, irFunction, irBlock);
//					ObjOperand mul2 = parseConstIntOperand(sizes.get(i), 0, irFunction, irBlock);
//					ObjOperand tmp = genTmpReg(irFunction);
//					ObjBinary bin = ObjBinary.getMul(tmp, mul1, mul2);
//					objBlock.addInstr(bin);
//					ObjBinary bin1 = ObjBinary.getAdd(dst, dst, tmp);
//					objBlock.addInstr(bin1);
//				}

			}

		}

	}

	private void parseCall(CallInst inst, BasicBlock irBlock, Function irFunction) {

		ObjFunction objFunction = fMap.get(irFunction);
		ObjBlock objBlock = bMap.get(irBlock);
//        if(inst.getFunction().getName().equals("@memset"))
//            return ;

		ObjFunction tarFunction = fMap.get(inst.getFunction());
		if (Library_functions.containsKey(inst.getFunction().getName()))
			tarFunction = Library_functions.get(inst.getFunction().getName());

		ArrayList<Value> operands = inst.getOperands();
		int num_i,num_f;
		ArrayList<Value> foperands=new ArrayList<>();
		ArrayList<Value> ioperands=new ArrayList<>();
		for(Value i : operands)
		{
			if (i.getType().isFloatTy()) foperands.add(i);
			else ioperands.add(i);
		}
		num_i=ioperands.size();
		num_f=foperands.size();

		if (num_i > 8)
			num_i = 8;
		if (num_f > 8)
			num_f = 8;

		for (int i = 0; i < num_f; i++) {
			Value arg = foperands.get(i);
			ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);

				ObjMove objMove = new ObjMove(ObjFPhyReg.A.get(i), objOperand);
				objBlock.addInstr(objMove);

		}
		for (int i = 0; i < num_i; i++) {
			Value arg = ioperands.get(i);
			ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);

			ObjMove objMove = new ObjMove(ObjPhyReg.A.get(i), objOperand);
			objBlock.addInstr(objMove);
		}
		num_i=ioperands.size();
		num_f=foperands.size();

		if (inst.getFunction().equals(irFunction) && irFunction.istailrecursive) {
			int now = objFunction.getStackSize();
			for (int i = 8; i < num_f; i++) {

				Value arg = foperands.get(i);


				ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);
				if (objOperand instanceof ObjImm12) {
					ObjOperand tmp = null;
					if (arg.getType().isFloatTy()) {
						tmp = genFTmpReg(irFunction);
					} else tmp = genTmpReg(irFunction);
					ObjMove mv = new ObjMove(tmp, objOperand);
					objBlock.addInstr(mv);
					objOperand = tmp;
				}


					getStore(objOperand, SP, now, "fsw", true, objFunction,
							objBlock.getInstrs().getTailValue(), 1);
					now += 4;

			}
			for (int i = 8; i < num_i; i++) {

				Value arg = ioperands.get(i);
				ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);
				if (objOperand instanceof ObjImm12) {
					ObjOperand tmp = null;
					if (arg.getType().isFloatTy()) {
						tmp = genFTmpReg(irFunction);
					} else tmp = genTmpReg(irFunction);
					ObjMove mv = new ObjMove(tmp, objOperand);
					objBlock.addInstr(mv);
					objOperand = tmp;
				}

				if (arg.getType() instanceof IntegerType) {
					getStore(objOperand, SP, now, "sw", true, objFunction,
							objBlock.getInstrs().getTailValue(), 1);
					now += 4;
				} else {
					getStore(objOperand, SP, now, "sd", true, objFunction,
							objBlock.getInstrs().getTailValue(), 1);
					now += 8;
				}
			}
		} else {
			int now = 0;
			for (int i = 8; i < num_f; i++) {
				Value arg = foperands.get(i);
				ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);
				if (objOperand instanceof ObjImm12) {
					ObjOperand tmp = null;
					if (arg.getType().isFloatTy()) {
						tmp = genFTmpReg(irFunction);
					} else tmp = genTmpReg(irFunction);
					ObjMove mv = new ObjMove(tmp, objOperand);
					objBlock.addInstr(mv);
					objOperand = tmp;
				}


					getStore(objOperand, SP, now, "fsw", true, objFunction,
							objBlock.getInstrs().getTailValue(), 0);
					now += 4;

			}
			for (int i = 8; i < num_i; i++) {
				Value arg = ioperands.get(i);
				ObjOperand objOperand = parseOperand(arg, 12, irFunction, irBlock);
				if (objOperand instanceof ObjImm12) {
					ObjOperand tmp = null;
					if (arg.getType().isFloatTy()) {
						tmp = genFTmpReg(irFunction);
					} else tmp = genTmpReg(irFunction);
					ObjMove mv = new ObjMove(tmp, objOperand);
					objBlock.addInstr(mv);
					objOperand = tmp;
				}

				 if (arg.getType() instanceof IntegerType) {
					getStore(objOperand, SP, now, "sw", true, objFunction,
							objBlock.getInstrs().getTailValue(), 0);
					now += 4;
				} else {
					getStore(objOperand, SP, now, "sd", true, objFunction,
							objBlock.getInstrs().getTailValue(), 0);
					now += 8;
				}
			}
		}


		ObjCall objCall = new ObjCall(tarFunction);
		objBlock.addInstr(objCall);

		if (!(inst.getType() instanceof VoidType)) {
			ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
			if (inst.getType() instanceof IntegerType) {
				ObjMove objMove = new ObjMove(dst, ObjPhyReg.A.get(0));
				objBlock.addInstr(objMove);
			} else if (inst.getType() instanceof FloatType) {
//				ObjConversion objCov = ObjConversion.getItof(dst,ObjFPhyReg. A.get(0));
//				objBlock.addInstr(objCov);
				ObjMove objMove = new ObjMove(dst, ObjFPhyReg.A.get(0));
				objBlock.addInstr(objMove);
			}
		}
	}

	private void parseCmp(CmpInst inst, BasicBlock irBlock, Function irFunction) {
		OP op = inst.getOp();
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
		Value left = inst.getLeftVal(), right = inst.getRightVal();
		ObjBlock objBlock = bMap.get(irBlock);
		ObjFunction objFunction = fMap.get(irFunction);


		// ne eq lt le gt ge
		if (op == OP.Ne) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 12, irFunction, irBlock);
			ObjVirReg tmpReg = new ObjVirReg();
			objFunction.addUsedVirReg(tmpReg);

			ObjBinary objBinary;
			if (objRight instanceof ObjImm12)
				objBinary = ObjBinary.getXor(tmpReg, objLeft, objRight);
			else
				objBinary = ObjBinary.getXor(tmpReg, objLeft, objRight);
			objBlock.addInstr(objBinary);

			ObjBinary objBinary2 = ObjBinary.getSltu(dst, ZERO, tmpReg);
			objBlock.addInstr(objBinary2);
		} else if (op == OP.FNe) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjOperand tmpReg = genTmpReg(irFunction);

			ObjBinary Feq = ObjBinary.getFeq(tmpReg, objLeft, objRight);
			objBlock.addInstr(Feq);
			// ObjBinary objBinary = ObjBinary.getSltu(dst, ZERO, tmpReg);
			ObjBinary objBinary = ObjBinary.getXor(dst, tmpReg, new ObjImm12(1));
			objBlock.addInstr(objBinary);
		} else if (op == OP.Eq) { // ( a xor b == 0 )
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 12, irFunction, irBlock);
			ObjVirReg tmpReg = new ObjVirReg();
			objFunction.addUsedVirReg(tmpReg);

			ObjBinary objBinary;
			if (objRight instanceof ObjImm12)
				objBinary = ObjBinary.getXor(tmpReg, objLeft, objRight);
			else
				objBinary = ObjBinary.getXor(tmpReg, objLeft, objRight);
			objBlock.addInstr(objBinary);

			ObjVirReg tmpReg2 = new ObjVirReg();
			objFunction.addUsedVirReg(tmpReg2);
			ObjBinary objBinary2 = ObjBinary.getSltu(tmpReg2, ZERO, tmpReg);
			objBlock.addInstr(objBinary2);

			ObjBinary objBinary3 = ObjBinary.getXor(dst, tmpReg2, new ObjImm12(1));
			objBlock.addInstr(objBinary3);
		} else if (op == OP.FEq) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);

			ObjBinary Feq = ObjBinary.getFeq(dst, objLeft, objRight);
			objBlock.addInstr(Feq);
		} else if (op == OP.Lt) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 12, irFunction, irBlock);

			ObjBinary objBinary;
			if (objRight instanceof ObjImm12)
				objBinary = ObjBinary.getSlti(dst, objLeft, objRight);
			else
				objBinary = ObjBinary.getSlt(dst, objLeft, objRight);
			objBlock.addInstr(objBinary);
		} else if (op == OP.FLt) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjBinary Flt = ObjBinary.getFlt(dst, objLeft, objRight);
			objBlock.addInstr(Flt);
		} else if (op == OP.Le) { // left <= Right    !( Right < left )
			ObjOperand objLeft = parseOperand(left, 12, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjVirReg tmpReg = new ObjVirReg();
			objFunction.addUsedVirReg(tmpReg);

			ObjBinary objBinary;
			if (objLeft instanceof ObjImm12)
				objBinary = ObjBinary.getSlti(tmpReg, objRight, objLeft);
			else
				objBinary = ObjBinary.getSlt(tmpReg, objRight, objLeft);
			objBlock.addInstr(objBinary);

			ObjBinary objBinary2 = ObjBinary.getXor(dst, tmpReg, new ObjImm12(1));
			objBlock.addInstr(objBinary2);
		} else if (op == OP.FLe) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjBinary Fle = ObjBinary.getFle(dst, objLeft, objRight);
			objBlock.addInstr(Fle);
		} else if (op == OP.Gt) { // (left > Right)
			ObjOperand objLeft = parseOperand(left, 12, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjBinary objBinary;
			if (objLeft instanceof ObjImm12)
				objBinary = ObjBinary.getSlti(dst, objRight, objLeft);
			else
				objBinary = ObjBinary.getSlt(dst, objRight, objLeft);
			objBlock.addInstr(objBinary);
		} else if (op == OP.FGt) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjBinary Fgt = ObjBinary.getFGt(dst, objLeft, objRight);
			objBlock.addInstr(Fgt);
		} else if (op == OP.Ge) { // left >= Right    ! ( left < Right )
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 12, irFunction, irBlock);
			ObjVirReg tmpReg = new ObjVirReg();
			objFunction.addUsedVirReg(tmpReg);

			ObjBinary objBinary;
			if (objRight instanceof ObjImm12)
				objBinary = ObjBinary.getSlti(tmpReg, objLeft, objRight);
			else
				objBinary = ObjBinary.getSlt(tmpReg, objLeft, objRight);
			objBlock.addInstr(objBinary);

			ObjBinary objBinary2 = ObjBinary.getXor(dst, tmpReg, new ObjImm12(1));
			objBlock.addInstr(objBinary2);
		} else if (op == OP.FGe) {
			ObjOperand objLeft = parseOperand(left, 0, irFunction, irBlock);
			ObjOperand objRight = parseOperand(right, 0, irFunction, irBlock);
			ObjBinary Fge = ObjBinary.getFGe(dst, objLeft, objRight);
			objBlock.addInstr(Fge);
		}
	}


	private void parseBr(BrInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);

		if (!inst.isJump()) {
			Value irCondition = inst.getJudVal();
			BasicBlock irTrueBlock = inst.getTrueBlock();
			BasicBlock irFalseBlock = inst.getFalseBlock();

			if (irCondition instanceof ConstInteger) {
				int condImm = ((ConstInteger) irCondition).getValue();
				if (condImm != 0) {
					ObjBranch objBranch = new ObjBranch(bMap.get(irTrueBlock));
					objBlock.addInstr(objBranch);
					objBlock.setTrueBlock(bMap.get(irTrueBlock));
				} else {
					ObjBranch objBranch = new ObjBranch(bMap.get(irFalseBlock));
					objBlock.addInstr(objBranch);
					objBlock.setTrueBlock(bMap.get(irFalseBlock));
				}
			} else if (irCondition instanceof CmpInst) {
				CmpInst condition = (CmpInst) irCondition;
				ObjOperand cond = operandMap.get(condition);

				ObjBlock objTrueBlock = bMap.get(irTrueBlock);
				ObjBlock objFalseBlock = bMap.get(irFalseBlock);

				ObjBranch objBranch = new ObjBranch(false, cond, objTrueBlock);
				objBlock.addInstr(objBranch);
				ObjBranch objBranch1 = new ObjBranch(objFalseBlock);
				objBlock.addInstr(objBranch1);

				objBlock.setTrueBlock(bMap.get(irTrueBlock));
				objBlock.setFalseBlock(bMap.get(irFalseBlock));
			}
		} else {
			ObjBlock objTargetBlock = bMap.get(inst.getJumpBlock());
			ObjBranch objBranch = new ObjBranch(objTargetBlock);
			objBlock.addInstr(objBranch);
			objBlock.setTrueBlock(objTargetBlock);
		}
	}

	private void parseAnd(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if ((src1 instanceof ObjImm12) && (src2 instanceof ObjImm12)) {
			int ans = ((ObjImm12) src1).getImmediate12() & ((ObjImm12) src2).getImmediate12();
			ObjMove objMove = new ObjMove(dst, parseConstIntOperand(ans, 32, irFunction, irBlock));
			objBlock.addInstr(objMove);
		} else if (src1 instanceof ObjImm12) {
			ObjBinary objAddi = ObjBinary.getAdd(dst, src2, src1);
			objBlock.addInstr(objAddi);
		} else if (src2 instanceof ObjImm12) {
			ObjBinary objAddi = ObjBinary.getAdd(dst, src1, src2);
			objBlock.addInstr(objAddi);
		} else {
			ObjBinary objAnd = ObjBinary.getAnd(dst, src1, src2);
			objBlock.addInstr(objAnd);
		}
	}

	private void parseOr(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if ((src1 instanceof ObjImm12) && (src2 instanceof ObjImm12)) {
			int ans = ((ObjImm12) src1).getImmediate12() | ((ObjImm12) src2).getImmediate12();
			ObjMove objMove = new ObjMove(dst, parseConstIntOperand(ans, 32, irFunction, irBlock));
			objBlock.addInstr(objMove);
		} else if (src1 instanceof ObjImm12) {
			ObjBinary objOri = ObjBinary.getOr(dst, src2, src1);
			objBlock.addInstr(objOri);
		} else if (src2 instanceof ObjImm12) {
			ObjBinary objOri = ObjBinary.getOr(dst, src1, src2);
			objBlock.addInstr(objOri);
		} else {
			ObjBinary objOr = ObjBinary.getOr(dst, src1, src2);
			objBlock.addInstr(objOr);
		}
	}

	private void parseXor(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if ((src1 instanceof ObjImm12) && (src2 instanceof ObjImm12)) {
			int ans = ((ObjImm12) src1).getImmediate12() ^ ((ObjImm12) src2).getImmediate12();
			ObjMove objMove = new ObjMove(dst, parseConstIntOperand(ans, 32, irFunction, irBlock));
			objBlock.addInstr(objMove);
		} else if (src1 instanceof ObjImm12) {
			ObjBinary objXori = ObjBinary.getXor(dst, src2, src1);
			objBlock.addInstr(objXori);
		} else if (src2 instanceof ObjImm12) {
			ObjBinary objXori = ObjBinary.getXor(dst, src1, src2);
			objBlock.addInstr(objXori);
		} else {
			ObjBinary objXor = ObjBinary.getXor(dst, src1, src2);
			objBlock.addInstr(objXor);
		}
	}

	private void parseShl(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if ((src1 instanceof ObjImm12) && (src2 instanceof ObjImm12)) {
			int ans = ((ObjImm12) src1).getImmediate12() << ((ObjImm12) src2).getImmediate12();
			ObjMove objMove = new ObjMove(dst, parseConstIntOperand(ans, 32, irFunction, irBlock));
			objBlock.addInstr(objMove);
		} else if (src2 instanceof ObjImm12) {
			ObjBinary objSlli = ObjBinary.getSll(dst, src1, src2);
			objSlli.isword = true;
			objBlock.addInstr(objSlli);
		} else {
			ObjBinary objSll = ObjBinary.getSll(dst, src1, src2);
			objSll.isword = true;
			objBlock.addInstr(objSll);
		}
	}

	private void parseShr(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if ((src1 instanceof ObjImm12) && (src2 instanceof ObjImm12)) {
			int ans = ((ObjImm12) src1).getImmediate12() >>> ((ObjImm12) src2).getImmediate12();
			ObjMove objMove = new ObjMove(dst, parseConstIntOperand(ans, 32, irFunction, irBlock));
			objBlock.addInstr(objMove);
		} else if (src2 instanceof ObjImm12) {
			ObjBinary objSrli = ObjBinary.getSrl(dst, src1, src2);
			objSrli.isword = true;
			objBlock.addInstr(objSrli);
		} else {
			ObjBinary objSrl = ObjBinary.getSrl(dst, src1, src2);
			objSrl.isword = true;
			objBlock.addInstr(objSrl);
		}
	}

	private void parseMod(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjBinary objRem = ObjBinary.getRem(dst, src1, src2);
		objRem.isword = true;
		objBlock.addInstr(objRem);
	}


	private void parseFMul(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjBinary objMul = ObjBinary.getFMul(dst, src1, src2);
		objBlock.addInstr(objMul);
	}

	private ObjOperand getMul_(ObjOperand src1, int rightVal, ObjOperand dst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);

		int abs = rightVal >= 0 ? rightVal : -rightVal;
		if (abs == 0) {
			ObjMove objMove = new ObjMove(dst, ZERO);
			objBlock.addInstr(objMove);
			return dst;
		} else if (abs == 1) {
			if (rightVal == 1) {
				ObjMove objMove = new ObjMove(dst, src1);
				objBlock.addInstr(objMove);
			} else if (rightVal == -1) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, src1);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return dst;
		} else if ((abs & (-abs)) == abs) {
			int l = -1, x = abs;
			while (x > 0) {
				l++;
				x = x / 2;
			}
			ObjBinary objSll = ObjBinary.getSll(dst, src1, new ObjImm12(l));
			objBlock.addInstr(objSll);
			if (rightVal < 0) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return dst;
		} else if (isTimesTwo(abs + 1) != -1) {
			//x=2^t-1
			int t = isTimesTwo(abs + 1);
			ObjOperand x1 = genTmpReg(irFunction);
			ObjBinary objSll = ObjBinary.getSll(x1, src1, new ObjImm12(t));
			objBlock.addInstr(objSll);

			ObjBinary objSub1 = ObjBinary.getSub(dst, x1, src1);
			objSub1.isword = true;
			objBlock.addInstr(objSub1);
			if (rightVal < 0) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return dst;
		} else {
			int meow = 2;//接受二进制上2位为1
			String binaryString = Integer.toBinaryString(abs);
			binaryString = reverseTestOne(binaryString);

			//eg Converting integer 78 to Binary String: 1001110
			ArrayList<Integer> bitsof1 = new ArrayList<>();
			for (int i = 0; i < binaryString.length(); i++) {
				if (binaryString.charAt(i) == '1') {
					bitsof1.add(i);
				}
			}
			if (bitsof1.size() <= meow) {
				ObjBinary objSll1 = ObjBinary.getSll(dst, src1, new ObjImm12(bitsof1.get(0)));
				objBlock.addInstr(objSll1);
				for (int i = 1; i < bitsof1.size(); i++) {
					ObjOperand x2 = genTmpReg(irFunction);
					ObjBinary objSll = ObjBinary.getSll(x2, src1, new ObjImm12(bitsof1.get(i)));
					objBlock.addInstr(objSll);
					ObjBinary objadd = ObjBinary.getAdd(dst, dst, x2);
					objBlock.addInstr(objadd);
				}
				if (rightVal < 0) {
					ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
					objSub.isword = true;
					objBlock.addInstr(objSub);
				}
				return dst;
			}
		}
		ObjBinary objMul = ObjBinary.getMul(dst, src1, parseConstIntOperand(rightVal, 0, irFunction, irBlock));
		objMul.isword = true;
		objBlock.addInstr(objMul);
		return dst;
	}

	private void parseMul(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if (inst.isI64()) {
			ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
			ObjBinary objMul = ObjBinary.getMul(dst, src1, src2);
			objBlock.addInstr(objMul);
			return;
		}

		boolean isSrc2Const = inst.getRightVal() instanceof ConstInteger;
		if (!isSrc2Const) {
			ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
			ObjBinary objMul = ObjBinary.getMul(dst, src1, src2);
			objMul.isword = true;
			objBlock.addInstr(objMul);
			return;
		}

		int rightVal = ((ConstInteger) inst.getRightVal()).getValue();
		int abs = rightVal >= 0 ? rightVal : -rightVal;
		if (abs == 0) {
			ObjMove objMove = new ObjMove(dst, ZERO);
			objBlock.addInstr(objMove);
			return;
		} else if (abs == 1) {
			if (rightVal == 1) {
				ObjMove objMove = new ObjMove(dst, src1);
				objBlock.addInstr(objMove);
			} else if (rightVal == -1) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, src1);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return;
		} else if ((abs & (-abs)) == abs) {
			int l = -1, x = abs;
			while (x > 0) {
				l++;
				x = x / 2;
			}
			ObjBinary objSll = ObjBinary.getSll(dst, src1, new ObjImm12(l));
			objBlock.addInstr(objSll);
			if (rightVal < 0) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return;
		} else if (isTimesTwo(abs + 1) != -1) {
			//x=2^t-1
			int t = isTimesTwo(abs + 1);
			ObjOperand x1 = genTmpReg(irFunction);
			ObjBinary objSll = ObjBinary.getSll(x1, src1, new ObjImm12(t));
			objBlock.addInstr(objSll);

			ObjBinary objSub1 = ObjBinary.getSub(dst, x1, src1);
			objSub1.isword = true;
			objBlock.addInstr(objSub1);
			if (rightVal < 0) {
				ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
				objSub.isword = true;
				objBlock.addInstr(objSub);
			}
			return;
		} else {
			int meow = 2;//接受二进制上2位为1
			String binaryString = Integer.toBinaryString(abs);
			binaryString = reverseTestOne(binaryString);

			//eg Converting integer 78 to Binary String: 1001110
			ArrayList<Integer> bitsof1 = new ArrayList<>();
			for (int i = 0; i < binaryString.length(); i++) {
				if (binaryString.charAt(i) == '1') {
					bitsof1.add(i);
				}
			}
			if (bitsof1.size() <= meow) {
				ObjBinary objSll1 = ObjBinary.getSll(dst, src1, new ObjImm12(bitsof1.get(0)));
				objBlock.addInstr(objSll1);
				for (int i = 1; i < bitsof1.size(); i++) {
					ObjOperand x2 = genTmpReg(irFunction);
					ObjBinary objSll = ObjBinary.getSll(x2, src1, new ObjImm12(bitsof1.get(i)));
					objBlock.addInstr(objSll);
					ObjBinary objadd = ObjBinary.getAdd(dst, dst, x2);
					objBlock.addInstr(objadd);
				}
				if (rightVal < 0) {
					ObjBinary objSub = ObjBinary.getSub(dst, ZERO, dst);
					objSub.isword = true;
					objBlock.addInstr(objSub);
				}
				return;
			}
		}
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjBinary objMul = ObjBinary.getMul(dst, src1, src2);
		objMul.isword = true;
		objBlock.addInstr(objMul);
	}

	public static String reverseTestOne(String s) {
		return new StringBuffer(s).reverse().toString();
	}

	private void constDiv(ObjOperand dst, ObjOperand dividend, int divisorImm, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		int abs = divisorImm > 0 ? divisorImm : -divisorImm;
		if (divisorImm == -1) {
			ObjBinary objRsb = ObjBinary.getSub(dst, ObjPhyReg.ZERO, dividend);
			objBlock.addInstr(objRsb);
			return;
		} else if (divisorImm == 1) {
			ObjMove objMove = new ObjMove(dst, dividend);
			objBlock.addInstr(objMove);
		}
		// 如果是 2 的幂次
		else if ((abs & (abs - 1)) == 0) {
			//System.out.println("meet div times 2");
			int times = isTimesTwo(abs);
			//u/(2^k)=(u+(1<<k)-1)>>k
			ObjOperand tmp1 = genTmpReg(irFunction);
			ObjOperand tmp2 = genTmpReg(irFunction);
			ObjOperand tmp3 = genTmpReg(irFunction);
			ObjBinary i1 = ObjBinary.getSra(tmp1, dividend, new ObjImm12(31));
			ObjBinary i2 = ObjBinary.getSrl(tmp2, tmp1, new ObjImm12(32 - times));
			ObjBinary i3 = ObjBinary.getAdd(tmp3, tmp2, dividend);
			ObjBinary objShift = ObjBinary.getSra(dst, tmp3, new ObjImm12(times));
			i1.isword = true;
			i2.isword = true;
			i3.isword = true;
			objShift.isword = true;
			objBlock.addInstr(i1);
			objBlock.addInstr(i2);
			objBlock.addInstr(i3);
			objBlock.addInstr(objShift);
		} else {
			//System.out.println("meet div not times 2");
			// nc = 2^31 - 2^31 % abs - 1
			long nc = ((long) 1 << 31) - (((long) 1 << 31) % abs) - 1;
			long p = 32;
			// 2^p > (2^31 - 2^31 % abs - 1) * (abs - 2^p % abs)
			while (((long) 1 << p) <= nc * (abs - ((long) 1 << p) % abs))
				p++;
			// m = (2^p + abs - 2^p % abs) / abs
			// m 是 2^p / abs 的向上取整
			long m = ((((long) 1 << p) + (long) abs - ((long) 1 << p) % abs) / (long) abs);
			// >>> 是无符号右移的意思，所以 n = m[31:0]
			int n = (int) ((m << 32) >>> 32);
			int shift = (int) (p - 32);

			// tmp0 = n
			ObjOperand tmp0 = genTmpReg(irFunction);
			ObjMove objInstr0 = new ObjMove(tmp0, new ObjImm(n));
			objBlock.addInstr(objInstr0);

			ObjOperand tmp1 = genTmpReg(irFunction);
			ObjOperand tmp3 = genTmpReg(irFunction);
			// tmp1 = dividend + (dividend * n)[63:32]
			if (m >= 0x80000000L) {
				//System.out.println("m >= 0x80000000L");
				// ObjCoMove objMtlo = ObjCoMove.getMthi(dividend);
				// objBlock.addInstr(objMtlo);
				// 这里的 madd 要求是有符号的，具体为啥我也不知道
				// ObjBinary smmadd = ObjBinary.getSmmadd(tmp1, dividend, tmp0);
				// objBlock.addInstr(smmadd);
//				getMul_(dividend,n,tmp1,irBlock,irFunction);
				ObjBinary mulh = ObjBinary.getMul(tmp1, dividend, tmp0);
				objBlock.addInstr(mulh);
				ObjBinary mulh1 = ObjBinary.getSra(tmp3, tmp1, new ObjImm12(32));
				objBlock.addInstr(mulh1);
				ObjBinary add = ObjBinary.getAdd(tmp1, tmp3, dividend);
				// add.isword = true;
				objBlock.addInstr(add);
			}
//			 tmp1 = (dividend * n)[63:32]
			else {
				//System.out.println("m < 0x80000000L");
				// 但是这里的 smmul 则是有符号的
				// ObjBinary objInstr1 = ObjBinary.getSmmul(tmp1, dividend, tmp0);
				// objBlock.addInstr(objInstr1);
//				getMul_(dividend,n,tmp1,irBlock,irFunction);
				ObjBinary mulh = ObjBinary.getMul(tmp1, dividend, tmp0);
				objBlock.addInstr(mulh);
				ObjBinary mulh1 = ObjBinary.getSra(tmp3, tmp1, new ObjImm12(32));
				objBlock.addInstr(mulh1);
			}

			ObjOperand tmp2 = genTmpReg(irFunction);
			// tmp2 = tmp1 >> shift
			ObjBinary objInstr3 = ObjBinary.getSra(tmp2, tmp3, new ObjImm12(shift));
			objBlock.addInstr(objInstr3);
			// dst = tmp2 + dividend >> 31
			// ObjBinary srl = ObjBinary.getSrl(AT, dividend, 31);
			ObjOperand tep = genTmpReg(irFunction);
			ObjBinary srl = ObjBinary.getSra(tep, tmp2, new ObjImm12(31));
			objBlock.addInstr(srl);
			ObjBinary objInstr4 = ObjBinary.getSub(dst, tmp2, tep);
			objBlock.addInstr(objInstr4);
		}
		if (divisorImm < 0) {
			// ObjBinary objRsb = ObjBinary.getSubu(dst, ZERO, dst);
			ObjBinary objRsb = ObjBinary.getSub(dst, ZERO, dst);
			objRsb.isword = true;
			objBlock.addInstr(objRsb);
		}
		divMap.put(new Pair<>(objBlock, new Pair<>(dividend, new ObjImm(divisorImm))), dst);
	}

	private void parseDiv(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);

		boolean isSrc2Const = inst.getRightVal() instanceof ConstInteger;

//		isSrc2Const = false;
		if (isSrc2Const) {
			int imm = ((ConstInteger) inst.getRightVal()).getValue();

			Pair<ObjOperand, ObjOperand> div = new Pair<>(src1, new ObjImm(imm));
			Pair<ObjBlock, Pair<ObjOperand, ObjOperand>> divLookUp = new Pair<>(objBlock, div);
			if (divMap.containsKey(divLookUp))
				operandMap.put(inst, divMap.get(divLookUp));
			else {
				ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
				constDiv(dst, src1, imm, irBlock, irFunction);
			}


		} else {
			ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
			ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

			ObjBinary objDiv = ObjBinary.getDiv(dst, src1, src2);
			if (!inst.isI64())
				objDiv.isword = true;
			objBlock.addInstr(objDiv);
		}
	}

	private void parseFDiv(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjBinary objFDiv = ObjBinary.getFDiv(dst, src1, src2);
		objBlock.addInstr(objFDiv);
	}

	private void parseSub(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		boolean isOp1Const = inst.getLeftVal() instanceof ConstInteger;
		boolean isOp2Const = inst.getRightVal() instanceof ConstInteger;
		ObjOperand src1, src2;
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		if (isOp1Const && isOp2Const) {
			int op1Imm = ((ConstInteger) inst.getLeftVal()).getValue();
			int op2Imm = ((ConstInteger) inst.getRightVal()).getValue();
			ObjMove objMove = new ObjMove(dst, new ObjImm(op1Imm - op2Imm));
			objBlock.addInstr(objMove);
		} else {
			src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
			src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
			ObjBinary objSub = ObjBinary.getSub(dst, src1, src2);
			objSub.isword = true;
			objBlock.addInstr(objSub);
		}
	}

	private void parseFSub(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjBinary objFSub = ObjBinary.getFSub(dst, src1, src2);
		objBlock.addInstr(objFSub);
	}

	private void parseAdd(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		boolean isOp1Const = inst.getLeftVal() instanceof ConstInteger;
		boolean isOp2Const = inst.getRightVal() instanceof ConstInteger;
		ObjOperand src1, src2;
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
		if (isOp1Const && isOp2Const) {
			int op1Imm = ((ConstInteger) inst.getLeftVal()).getValue();
			int op2Imm = ((ConstInteger) inst.getRightVal()).getValue();
			ObjMove objMove = new ObjMove(dst, new ObjImm(op1Imm + op2Imm));
			objBlock.addInstr(objMove);
		} else if (isOp1Const) {
			src1 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
			src2 = parseOperand(inst.getLeftVal(), 12, irFunction, irBlock);
			ObjBinary objAdd = ObjBinary.getAdd(dst, src1, src2);
			objAdd.isword = true;
			objBlock.addInstr(objAdd);
		} else {
			src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
			src2 = parseOperand(inst.getRightVal(), 12, irFunction, irBlock);
			ObjBinary objAdd = ObjBinary.getAdd(dst, src1, src2);
			objAdd.isword = true;
			objBlock.addInstr(objAdd);
		}
	}

	private void parseFAdd(BinaryInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjOperand src1 = parseOperand(inst.getLeftVal(), 0, irFunction, irBlock);
		ObjOperand src2 = parseOperand(inst.getRightVal(), 0, irFunction, irBlock);
		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);

		ObjBinary objFAdd = ObjBinary.getFAdd(dst, src1, src2);
		objBlock.addInstr(objFAdd);
	}

	private void parseStore(StoreInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
//store float 1.0, float* %16
		Value irAddr = inst.getPointer();
		if (irAddr instanceof GlobalVar) {//TODO
			ObjOperand tmp = genTmpReg(irFunction);
			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			String type = "la";
			ObjLoad objLoad = new ObjLoad(tmp, addr, type);//la
			objBlock.addInstr(objLoad);

			ObjOperand src = parseOperand(inst.getValue(), 0, irFunction, irBlock);
			ObjOperand offset = new ObjImm12(0);
			String type1 = "sw";
			if (src instanceof ObjFVirReg) type1 = "fsw";
			ObjStore objStore = new ObjStore(src, tmp, offset, type1);
			objBlock.addInstr(objStore);
		} else if (((PointerType) irAddr.getType()).getEleType() instanceof PointerType) {
			ObjOperand src = parseOperand(inst.getValue(), 0, irFunction, irBlock);
			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			ObjOperand offset = new ObjImm12(0);
			String type = "sd";
			ObjStore objStore = new ObjStore(src, addr, offset, type);
			objBlock.addInstr(objStore);
		} else {
			ObjOperand src = parseOperand(inst.getValue(), 0, irFunction, irBlock);

			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			ObjOperand offset = new ObjImm12(0);
			String type = "sw";
			if (src instanceof ObjFVirReg) type = "fsw";

			ObjStore objStore = new ObjStore(src, addr, offset, type);
			objBlock.addInstr(objStore);
		}

	}

	private void getStore(ObjOperand src, ObjOperand addr, int immediate, String ty,
	                      boolean insertpos, ObjFunction objFunction, ObjInstr instr, int needmark) {
		// insertpos false: ???????, true: ???????
		if (needmark == 1) {

			ObjOperand tmp = genTmpReg(objFunction);
			ObjImm tmpimm = new ObjImm(immediate);
			ObjMove objMove = new ObjMove(tmp, tmpimm);
			objMove.needchange = true;

			ObjOperand addr2 = genTmpReg(objFunction);
			ObjBinary objAdd = ObjBinary.getAdd(addr2, addr, tmp);

			ObjStore objStore = new ObjStore(src, addr2, new ObjImm12(0), ty);
			if (insertpos) {
				objMove.getNode().insertAfter(instr.getNode());
				objAdd.getNode().insertAfter(objMove.getNode());
				objStore.getNode().insertAfter(objAdd.getNode());
			} else {
				objStore.getNode().insertBefore(instr.getNode());
				objAdd.getNode().insertBefore(objStore.getNode());
				objMove.getNode().insertBefore(objAdd.getNode());
			}

		} else {
			if (immediate >= -2048 && immediate <= 2047) {
				ObjImm12 Imm = new ObjImm12(immediate);
				ObjStore objStore = new ObjStore(src, addr, Imm, ty);
				if (insertpos)
					objStore.getNode().insertAfter(instr.getNode());
				else
					objStore.getNode().insertBefore(instr.getNode());
			} else {
				ObjOperand tmp = genTmpReg(objFunction);
				ObjImm tmpimm = new ObjImm(immediate);
				ObjMove objMove = new ObjMove(tmp, tmpimm);

				ObjOperand addr2 = genTmpReg(objFunction);
				ObjBinary objAdd = ObjBinary.getAdd(addr2, addr, tmp);

				ObjStore objStore = new ObjStore(src, addr2, new ObjImm12(0), ty);
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


	}

	private void getLoad(ObjOperand dst, ObjOperand addr, int immediate, String ty,
	                     boolean insertpos, ObjFunction objFunction, ObjInstr instr, int needmark) {

		// insertpos false: ???????, true: ???????
		if (needmark == 1) {

			ObjOperand tmp = genTmpReg(objFunction);
			ObjImm tmpimm = new ObjImm(immediate);
			ObjMove objMove = new ObjMove(tmp, tmpimm);
			objMove.needchange = true;

			ObjOperand addr2 = genTmpReg(objFunction);
			ObjBinary objAdd = ObjBinary.getAdd(addr2, addr, tmp);

			ObjLoad objLoad = new ObjLoad(dst, addr2, new ObjImm12(0), ty);
			if (insertpos) {
				objMove.getNode().insertAfter(instr.getNode());
				objAdd.getNode().insertAfter(objMove.getNode());
				objLoad.getNode().insertAfter(objAdd.getNode());
			} else {
				objLoad.getNode().insertBefore(instr.getNode());
				objAdd.getNode().insertBefore(objLoad.getNode());
				objMove.getNode().insertBefore(objAdd.getNode());
			}

		} else {
			if (immediate >= -2048 && immediate <= 2047) {
				ObjImm12 Imm = new ObjImm12(immediate);
				ObjLoad objLoad = new ObjLoad(dst, addr, Imm, ty);
				if (insertpos)
					objLoad.getNode().insertAfter(instr.getNode());
				else
					objLoad.getNode().insertBefore(instr.getNode());
			} else {
				ObjOperand tmp = genTmpReg(objFunction);
				ObjImm tmpimm = new ObjImm(immediate);
				ObjMove objMove = new ObjMove(tmp, tmpimm);

				ObjOperand addr2 = genTmpReg(objFunction);
				ObjBinary objAdd = ObjBinary.getAdd(addr2, addr, tmp);

				ObjLoad objLoad = new ObjLoad(dst, addr2, new ObjImm12(0), ty);
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

	}

	private void parseLoad(LoadInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
//%4 = load float, float* @g       la s0,g     ;  lw s1,0(s0)
//dst                    addr
		Value irAddr = inst.getPointer();

		if (irAddr instanceof GlobalVar) {
			ObjOperand tmp = genTmpReg(irFunction);
			ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			String type = "la";
			ObjLoad objLoad = new ObjLoad(tmp, addr, type);//la:tmp??????
			objBlock.addInstr(objLoad);
			String type1 = "lw";
			if (dst instanceof ObjFVirReg) type1 = "flw";

			ObjOperand offset = new ObjImm12(0);
			ObjLoad objLoad1 = new ObjLoad(dst, tmp, offset, type1);
			objBlock.addInstr(objLoad1);
		} else if (inst.getType() instanceof PointerType) {
			ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			ObjOperand offset = new ObjImm12(0);
			String type = "ld";
			ObjLoad objLoad = new ObjLoad(dst, addr, offset, type);
			objBlock.addInstr(objLoad);
		} else {
			ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
			ObjOperand addr = parseOperand(irAddr, 0, irFunction, irBlock);
			ObjOperand offset = new ObjImm12(0);
			String type = "lw";
			if (dst instanceof ObjFVirReg) type = "flw";
			ObjLoad objLoad = new ObjLoad(dst, addr, offset, type);
			objBlock.addInstr(objLoad);
		}

	}

	private void parseAlloca(AllocInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjFunction objFunction = fMap.get(irFunction);

		Type pointeeType = ((PointerType) inst.getType()).getEleType();
		int off = objFunction.getArgsSize() + objFunction.getAllocaSize();
		ObjOperand offset = parseConstIntOperand(off, 12, irFunction, irBlock);

		if (pointeeType.isArrayType())
			objFunction.addAllocaSize(4 * ((ArrayType) pointeeType).getTotalSize());
		else if (pointeeType instanceof PointerType)
			objFunction.addAllocaSize(8);
		else
			objFunction.addAllocaSize(4);

		ObjOperand dst = parseOperand(inst, 0, irFunction, irBlock);
		ObjBinary objAdd = ObjBinary.getAdd(dst, ObjPhyReg.SP, offset);
		objBlock.addInstr(objAdd);
	}

	private void parseRet(RetInst inst, BasicBlock irBlock, Function irFunction) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjFunction objFunction = fMap.get(irFunction);
		Value irRetValue = inst.getValue();
		if (irRetValue != null) {
			if (irFunction.getType().isFloatTy()) {
				ObjOperand objRet = parseOperand(irRetValue, 32, irFunction, irBlock);
				ObjMove objMove = new ObjMove(ObjFPhyReg.A.get(0), objRet);
				objBlock.addInstr(objMove);
			} else {
				ObjOperand objRet = parseOperand(irRetValue, 32, irFunction, irBlock);
				ObjMove objMove = new ObjMove(ObjPhyReg.A.get(0), objRet);
				objBlock.addInstr(objMove);
			}

		}
		objBlock.addInstr(new ObjRet());
	}

	private ObjOperand genTmpReg(Function irFunction) {
		ObjFunction objFunction = fMap.get(irFunction);
		ObjVirReg tmpReg = new ObjVirReg();
		objFunction.addUsedVirReg(tmpReg);
		return tmpReg;
	}

	private ObjOperand genTmpReg(ObjFunction objFunction) {
		ObjVirReg tmpReg = new ObjVirReg();
		objFunction.addUsedVirReg(tmpReg);
		return tmpReg;
	}

	private ObjOperand genFTmpReg(Function irFunction) {
		ObjFunction objFunction = fMap.get(irFunction);
		ObjFVirReg tmpReg = new ObjFVirReg();
		objFunction.addUsedVirReg(tmpReg);
		return tmpReg;
	}

	private ObjOperand genDstOperand(Value irValue, Function irFunction) {
		ObjFunction objFunction = fMap.get(irFunction);

		if (irValue.getType() instanceof FloatType) {
			ObjFVirReg dstReg = new ObjFVirReg();
			objFunction.addUsedVirReg(dstReg);
			if (!(irValue instanceof Const)) {
				operandMap.put(irValue, dstReg);
				operandMap1.put(dstReg, irValue);
			}
			return dstReg;
		} else {
			ObjVirReg dstReg = new ObjVirReg();
			objFunction.addUsedVirReg(dstReg);
			if (!(irValue instanceof Const)) {
				operandMap.put(irValue, dstReg);
				operandMap1.put(dstReg, irValue);
			}
			return dstReg;
		}
	}

	private ObjOperand parseOperand(Value irValue, int canImm, Function irFunction, BasicBlock irBlock) {
		if (operandMap.containsKey(irValue)) {
			ObjOperand objOperand = operandMap.get(irValue);
			if (((objOperand instanceof ObjImm) && canImm < 32)
					|| ((objOperand instanceof ObjImm12) && canImm < 12)) {

				if (((ObjImm) objOperand).getImmediate() == 0)
					return ZERO;
				else {
					ObjOperand tmp = genTmpReg(irFunction);
					ObjMove objMove = new ObjMove(tmp, objOperand);
					bMap.get(irBlock).addInstr(objMove);
					return tmp;
				}
			}
			return objOperand;
		}


		if ((irValue instanceof Move) && (((Move) irValue).pair.size() != 0)) {
			boolean contains = false;
			Value ir = null;
			for (Move x : ((Move) irValue).pair) {
				if (operandMap.containsKey(x)) {
					contains = true;
					ir = x;
					break;
				}

			}
			if (contains) {
				ObjOperand objOperand = operandMap.get(ir);
				if (((objOperand instanceof ObjImm) && canImm < 32)
						|| ((objOperand instanceof ObjImm12) && canImm < 12)) {

					if (((ObjImm) objOperand).getImmediate() == 0)
						return ZERO;
					else {
						ObjOperand tmp = genTmpReg(irFunction);
						ObjMove objMove = new ObjMove(tmp, objOperand);
						bMap.get(irBlock).addInstr(objMove);
						return tmp;
					}
				}
				return objOperand;
			}

		}
		if (irValue instanceof ConstInteger)
			return parseConstIntOperand(((ConstInteger) irValue).getValue(), canImm, irFunction, irBlock);

		if (irValue instanceof ConstFloat) {
			float floatValue = ((ConstFloat) irValue).getValue();
			int intValue = Float.floatToRawIntBits(floatValue);
			String hexString = Integer.toHexString(intValue);

			ObjOperand src = genDstOperand(irValue, irFunction);

			ObjOperand initValue = parseConstFloatOperand(intValue, 32, irFunction, irBlock);
			ObjOperand tmp = genTmpReg(irFunction);
			ObjLoad l1 = new ObjLoad(tmp, initValue, "lla");
			bMap.get(irBlock).addInstr(l1);
			ObjLoad objMove = new ObjLoad(src, tmp, new ObjImm12(0), "flw");

			bMap.get(irBlock).addInstr(objMove);
			return src;
		}

		if (irValue instanceof GlobalVar)
			return parseGlobalOperand((GlobalVar) irValue, irFunction, irBlock);

		return genDstOperand(irValue, irFunction);
	}

	private ObjOperand parseGlobalOperand(GlobalVar g, Function irFunction, BasicBlock irBlock) {
		ObjBlock objBlock = bMap.get(irBlock);
		ObjGlobalVariable objG = gMap.get(g);

		ObjLabel label = new ObjLabel(objG.getName());
		return label;
	}

	private HashMap<Integer, ObjLabel> lc = new HashMap<>();

	private ObjOperand parseConstFloatOperand(int immediate, int canImm, Function irFunction, BasicBlock irBlock) {
		if (lc.containsKey(immediate)) {
			return lc.get(immediate);

		} else {
			ObjGlobalVariable objGlobalVariable = new ObjGlobalVariable(immediate);
			objModule.addGlobalVariable(objGlobalVariable);
			ObjLabel label = new ObjLabel(objGlobalVariable.getName());
			lc.put(immediate, label);
			return label;
		}

	}

	private ObjOperand parseConstIntOperand(int immediate, int canImm, Function irFunction, BasicBlock irBlock) {
		ObjImm imm = new ObjImm(immediate);
		ObjImm12 imm12 = new ObjImm12(immediate);
		if (canImm == 32)
			return imm;
		else if (canImm == 12 && (immediate >= -2048 && immediate <= 2047))
			return imm12;
		else {
			ObjBlock objBlock = bMap.get(irBlock);
			ObjFunction objFunction = fMap.get(irFunction);
			ObjVirReg dst = new ObjVirReg();
			objFunction.addUsedVirReg(dst);
			ObjMove objMove = new ObjMove(dst, imm);
			objBlock.addInstr(objMove);
			return dst;
		}
	}
}
