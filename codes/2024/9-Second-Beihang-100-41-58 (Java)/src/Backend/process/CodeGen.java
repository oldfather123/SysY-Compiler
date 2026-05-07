package Backend.process;

import Backend.component.*;
import Backend.instruction.*;
import Backend.operand.*;
import midend.*;
import midend.LLVMType.*;
import midend.Module;
import midend.analysis.LoopInfo;
import midend.instr.*;
import midend.optimism.ALUOptimize;

import java.util.*;


import static Backend.operand.AllPhysicalReg.*;

public class CodeGen {
    private final Module irModule;
    private final AsmModule asmModule;
    private final HashMap<Value, AsmComponent> valueToComponent;
    private AsmFunction curFunction;
    private AsmBlock curBlock;
    private final HashMap<Value, AsmReg> operandMap;
    private final HashMap<Integer, ArrayList<AsmInstr>> needChange;
    private final HashMap<String, AsmFunction> LibFunctions;

    public CodeGen(Module irModule) {
        this.irModule = irModule;
        this.asmModule = new AsmModule();
        this.valueToComponent = new HashMap<>();
        this.operandMap = new HashMap<>();
        this.needChange = new HashMap<>();
        LibFunctions = new HashMap<>();
        LibFunctions.put("@putint", new AsmFunction("@putint", false));
        LibFunctions.put("@getint", new AsmFunction("@getint", false));
        LibFunctions.put("@putch", new AsmFunction("@putch", false));
        LibFunctions.put("@getch", new AsmFunction("@getch", false));
        LibFunctions.put("@putarray", new AsmFunction("@putarray", false));
        LibFunctions.put("@getarray", new AsmFunction("@getarray", false));
        LibFunctions.put("@putfloat", new AsmFunction("@putfloat", false));
        LibFunctions.put("@getfloat", new AsmFunction("@getfloat", false));
        LibFunctions.put("@putfarray", new AsmFunction("@putfarray", false));
        LibFunctions.put("@getfarray", new AsmFunction("@getfarray", false));
        LibFunctions.put("@memset", new AsmFunction("@memset", false));
        LibFunctions.put("@starttime", new AsmFunction("@_sysy_starttime", false));
        LibFunctions.put("@stoptime", new AsmFunction("@_sysy_stoptime", false));
    }

    public AsmModule run() {
        for (GlobalVar globalVar : irModule.getGlobalVars()) {
            parseGlobal(globalVar);
        }
        for (Function function : irModule.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            AsmFunction objFunction = new AsmFunction(function.getName(), function.ifHasParam());
            asmModule.addFunction(objFunction);
            for (BasicBlock irBlock : function.getBlockList()) {
                AsmBlock asmBlock = new AsmBlock(irBlock.getName(), irBlock);
                valueToComponent.put(irBlock, asmBlock);
                objFunction.addBlocks(asmBlock);
            }
        }
        for (Function irFunction : irModule.getFunctions()) {
            curFunction = asmModule.getObjFunction(irFunction);
            parseFunction(irFunction);
        }
        changeLabel();
        return asmModule;
    }

    private void parseGlobal(GlobalVar globalVar) {
        ArrayList<Integer> elements = new ArrayList<>();
        Type globalType = ((PointerType) globalVar.getType()).getElementType();
        if (globalType.isArray()) {
            if (!globalVar.isAllZero()) {
                for (Value value : globalVar.getValueList()) {
                    int intValue = 0;
                    if (value.getType() instanceof IntegerType) {
                        intValue = ((ConstInt) value).getValue();
                    } else if (value.getType() instanceof FloatType) {
                        intValue = Float.floatToRawIntBits(((ConstFloat) value).getValue());
                    }
                    elements.add(intValue);
                }
            }
        } else if (globalType.isInteger()) {
            Value intValue = globalVar.getValue();
            if (intValue != null) {
                elements.add(((ConstInt) intValue).getValue());
            }
        } else if (globalType.isFloat()) {
            Value floatValue = globalVar.getValue();
            if (floatValue != null) {
                int intValue = Float.floatToRawIntBits(((ConstFloat) floatValue).getValue());
                elements.add(intValue);
            }
        }
        AsmGlobalOther asmGlobalOther = getObjGlobalOther(globalVar, elements, globalType);
        asmModule.addGlobalVariable(asmGlobalOther);
        valueToComponent.put(globalVar, asmGlobalOther);
    }

    private AsmGlobalOther getObjGlobalOther(GlobalVar globalVar, ArrayList<Integer> elements, Type globalType) {
        AsmGlobalOther asmGlobalOther;
        if (elements.isEmpty()) {
            if (globalType.isArray()) {
                int size = 4 * ((ArrayType) globalType).getSize();
                asmGlobalOther = new AsmGlobalOther(globalVar.getName(), false, size, elements);
            } else {
                asmGlobalOther = new AsmGlobalOther(globalVar.getName(), false, 4, elements);
            }
        } else {
            asmGlobalOther = new AsmGlobalOther(globalVar.getName(), true, 0, elements);
        }
        return asmGlobalOther;
    }

    private void parseFunction(Function irFunction) {
        for (BasicBlock irBlock : irFunction.getBlockList()) {
            AsmBlock asmBlock = (AsmBlock) valueToComponent.get(irBlock);
            asmBlock.setPreNxtBlocks(valueToComponent, irBlock.getPreBlock(), irBlock.getNextBlock());
        }
        int now = stackNeedToExpand(irFunction);
        curFunction.setArgsSize(now);
        for (BasicBlock irBlock : irFunction.getBlockList()) {
            parseBlock(irBlock, irFunction);
        }
        AsmBlock exitBlock = (AsmBlock) valueToComponent.get(irFunction.getExitBlock());
        curFunction.setExitBlock(exitBlock);
        curBlock = curFunction.getFirstBlock();
        if (!curFunction.isIfHasParam()) {
            ArrayList<Param> fParams = irFunction.getFParams();
            ArrayList<Param> iParams = irFunction.getIParams();
            int offset = curFunction.getStackSize();
            for (int i = 0; i < fParams.size(); i++) {
                AsmOperand operand = getOperandForNoImm(fParams.get(i));
                if (i < 8) {
                    AsmMove objMove = new AsmMove(Arrays.asList(operand, fPhyA.get(i)));
                    curBlock.insertHead(objMove);
                } else {
                    getLoadInsertHead(operand, offset, "flw");
                    offset += 4;
                }
            }
            for (int i = 0; i < iParams.size(); i++) {
                AsmOperand operand = getOperandForNoImm(iParams.get(i));
                if (i < 8) {
                    AsmMove objMove = new AsmMove(Arrays.asList(operand, phyA.get(i)));
                    curBlock.insertHead(objMove);
                } else {
                    if (iParams.get(i).getType().isInteger()) {
                        getLoadInsertHead(operand, offset, "lw");
                        offset += 4;
                    } else {
                        getLoadInsertHead(operand, offset, "ld");
                        offset += 8;
                    }
                }
            }
            tailRecursive(irFunction);
            resetStack();
        } else {
            tailRecursive(irFunction);
            resetStack();
        }
    }

    private void tailRecursive(Function function) {
        if (function.isTailRecursive()) {
            AsmBlock newBlock = new AsmBlock(function.getName().substring(1) + "start", new BasicBlock(UndefinedType.undefined, function));
            newBlock.addInstr(new AsmJ(curBlock));
            curFunction.getObjBlocks().add(0, newBlock);
            for (AsmBlock asmBlock : curFunction.getObjBlocks()) {
                HashSet<AsmInstr> needDelete = new HashSet<>();
                boolean flag = false;
                for (AsmInstr instr : asmBlock.getInstrList()) {
                    if (instr instanceof AsmCall call) {
                        if (call.getTarFunction().equals(curFunction)) {
                            needDelete.add(instr);
                            flag = true;
                            continue;
                        }
                        if (flag) {
                            needDelete.add(instr);
                        }
                    }
                }
                for (AsmInstr instr : needDelete) {
                    asmBlock.insertBefore(instr, new AsmJ(curBlock));
                }
                for (AsmInstr instr : needDelete) {
                    asmBlock.deleteInstr(instr);
                }
            }
        }
        curBlock = curFunction.getFirstBlock();
    }

    private int stackNeedToExpand(Function function) {
        int expand = 0;
        for (BasicBlock irBlock : function.getBlockList()) {
            for (Instruction irInst : irBlock.getInstrList()) {
                if (irInst instanceof CallInstr callInstr) {
                    if (!(function.isTailRecursive() && callInstr.getFunction().equals(function))) {
                        ArrayList<Value> operands = irInst.getValueList();
                        for (int i = 8; i < operands.size(); i++) {
                            if (operands.get(i).getType().isPointer()) {
                                expand += 8;
                            } else {
                                expand += 4;
                            }
                        }
                    }
                }
            }
        }
        return expand;
    }

    private void resetStack() {
        if (curFunction.getStackSize() - 8 >= -2048 && curFunction.getStackSize() <= 2047) {
            AsmStore store = new AsmStore(Arrays.asList(RA, SP, new AsmImm(curFunction.getStackSize() - 8, true)), "sd");
            curBlock.insertHead(store);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(curFunction.getStackSize() - 8, false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
            AsmStore store = new AsmStore(Arrays.asList(RA, add.getDst(), new AsmImm(0, true)), "sd");
            curBlock.insertHead(store);
            curBlock.insertHead(add);
            curBlock.insertHead(move);
        }
        AsmOperand alloc;
        AsmOperand resetAlloc;
        if (-curFunction.getStackSize() >= -2048 && -curFunction.getStackSize() <= 2047) {
            alloc = new AsmImm(-curFunction.getStackSize(), true);
        } else {
            alloc = getIntOperandForImmMoreThan12(-curFunction.getStackSize());
        }
        if (curFunction.getStackSize() >= -2048 && curFunction.getStackSize() <= 2047) {
            resetAlloc = new AsmImm(curFunction.getStackSize(), true);
        } else {
            resetAlloc = getIntOperandForImmMoreThan12(curFunction.getStackSize());
        }
        AsmBinary objAdd = new AsmBinary("add", Arrays.asList(SP, SP, alloc));
        curFunction.getFirstBlock().insertHead(objAdd);
        AsmBinary objAdd1 = new AsmBinary("add", Arrays.asList(SP, SP, resetAlloc));
        curFunction.getExitBlock().insertBefore(curFunction.getExitBlock().getInstrList().getLast(), objAdd1);
    }

    private void parseBlock(BasicBlock irBlock, Function function) {
        curBlock = (AsmBlock) valueToComponent.get(irBlock);
        for (Instruction inst : irBlock.getInstrList()) {
            parseInstruction(inst, irBlock, function);
        }
    }

    private void parseInstruction(Instruction irInst, BasicBlock irBlock, Function function) {
        if (irInst instanceof RetInstr) {
            parseRet((RetInstr) irInst);
        } else if (irInst instanceof AllocaInstr) {
            parseAlloc((AllocaInstr) irInst);
        } else if (irInst instanceof LoadInstr) {
            parseLoad((LoadInstr) irInst);
        } else if (irInst instanceof StoreInstr) {
            parseStore((StoreInstr) irInst);
        } else if (irInst instanceof CmpInstr) {
            parseCmp((CmpInstr) irInst);
        } else if (irInst instanceof BrInstr) {
            parseBr((BrInstr) irInst);
        } else if (irInst instanceof CallInstr) {
            parseCall((CallInstr) irInst, function);
        } else if (irInst instanceof GetElementPtrInstr) {
            parseGep((GetElementPtrInstr) irInst, irBlock, function);
        } else if (irInst instanceof ConversionInstr) {
            parseConversionInst((ConversionInstr) irInst);
        } else if (irInst instanceof ALUInstr) {
            if (irInst.getInstrType() == InstrType.ADD) {
                parseAdd((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.FADD) {
                parseFAdd((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.MUL || irInst.getInstrType() == InstrType.FMUL) {
                parseMul((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.SUB) {
                parseSub((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.FSUB) {
                parseFSub((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.SDIV || irInst.getInstrType() == InstrType.FDIV) {
                parseDiv((ALUInstr) irInst, irBlock, function);
            } else if (irInst.getInstrType() == InstrType.SHL) {
                parseShl((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.LSHR) {
                parseLShl((ALUInstr) irInst);
            } else if (irInst.getInstrType() == InstrType.ASHR) {
                parseAShr((ALUInstr) irInst);
            }
        } else if (irInst instanceof MoveInstr) {
            parseMove((MoveInstr) irInst);
        }
    }

    private void parseRet(RetInstr inst) {
        Value irRetValue = inst.getValue();
        if (irRetValue != null && !irRetValue.getType().isVoid()) {
            if (irRetValue.getType().isFloat()) {
                AsmOperand objRet = getOperandFor32Imm(irRetValue);
                AsmMove objM = new AsmMove(Arrays.asList(fPhyA.get(0), objRet));
                curBlock.addInstr(objM);
            } else {
                AsmOperand objRet = getOperandFor32Imm(irRetValue);
                AsmMove objM = new AsmMove(Arrays.asList(phyA.get(0), objRet));
                curBlock.addInstr(objM);
            }
        }
        loadRa(curFunction.getStackSize() - 8);
        curBlock.addInstr(new AsmRet());
    }

    private void parseAlloc(AllocaInstr inst) {
        Type pointerType = ((PointerType) inst.getType()).getElementType();
        int off = curFunction.getAllocSize() + curFunction.getArgsSize();
        AsmOperand offset;
        if (off >= -2048 && off <= 2047) {
            offset = new AsmImm(off, true);
        } else {
            offset = getIntOperandForImmMoreThan12(off);
        }
        if (pointerType.isArray()) {
            curFunction.addAllocSize(4 * ((ArrayType) pointerType).getSize());
        } else if (pointerType instanceof PointerType) {
            curFunction.addAllocSize(8);
        } else {
            curFunction.addAllocSize(4);
        }
        AsmOperand dst = getOperandForNoImm(inst);
        curBlock.addInstr(new AsmBinary("add", Arrays.asList(dst, SP, offset)));
    }

    private void parseLoad(LoadInstr inst) {
        Value irAddress = inst.getValue();
        if (irAddress instanceof GlobalVar) {
            AsmOperand dst = getOperandForNoImm(inst);
            AsmOperand address = getGlobalOperand((GlobalVar) irAddress);
            AsmLa la = new AsmLa(Arrays.asList(new VirtualReg(), address));
            curBlock.addInstr(la);
            AsmLoad load;
            if (dst instanceof FVirtualReg) {
                load = new AsmLoad(Arrays.asList(dst, la.getDst(), new AsmImm(0, true)), "flw");
            } else {
                load = new AsmLoad(Arrays.asList(dst, la.getDst(), new AsmImm(0, true)), "lw");
            }
            curBlock.addInstr(load);
        } else if (inst.getType() instanceof PointerType) {
            AsmOperand dst = getOperandForNoImm(inst);
            AsmOperand address = getOperandForNoImm(irAddress);
            AsmLoad load = new AsmLoad(Arrays.asList(dst, address, new AsmImm(0, true)), "ld");
            curBlock.addInstr(load);
        } else {
            AsmOperand dst = getOperandForNoImm(inst);
            AsmOperand address = getOperandForNoImm(irAddress);
            AsmLoad load;
            if (dst instanceof FVirtualReg) {
                load = new AsmLoad(Arrays.asList(dst, address, new AsmImm(0, true)), "flw");
            } else {
                load = new AsmLoad(Arrays.asList(dst, address, new AsmImm(0, true)), "lw");
            }
            curBlock.addInstr(load);
        }
    }

    private void parseStore(StoreInstr inst) {
        Value irAddress = inst.getPointer();
        if (irAddress instanceof GlobalVar) {
            AsmOperand address = getGlobalOperand((GlobalVar) irAddress);
            AsmLa la = new AsmLa(Arrays.asList(new VirtualReg(), address));
            curBlock.addInstr(la);
            AsmOperand src = getOperandForNoImm(inst.getValue());
            AsmStore store;
            if (src instanceof FVirtualReg) {
                store = new AsmStore(Arrays.asList(src, la.getDst(), new AsmImm(0, true)), "fsw");
            } else {
                store = new AsmStore(Arrays.asList(src, la.getDst(), new AsmImm(0, true)), "sw");
            }
            curBlock.addInstr(store);
        } else if (((PointerType) irAddress.getType()).getElementType() instanceof PointerType) {
            AsmOperand src = getOperandForNoImm(inst.getValue());
            AsmOperand address = getOperandForNoImm(irAddress);
            AsmStore store = new AsmStore(Arrays.asList(src, address, new AsmImm(0, true)), "sd");
            curBlock.addInstr(store);
        } else {
            AsmOperand src = getOperandForNoImm(inst.getValue());
            AsmOperand address = getOperandForNoImm(irAddress);
            AsmStore store;
            if (src instanceof FVirtualReg) {
                store = new AsmStore(Arrays.asList(src, address, new AsmImm(0, true)), "fsw");
            } else {
                store = new AsmStore(Arrays.asList(src, address, new AsmImm(0, true)), "sw");
            }
            curBlock.addInstr(store);
        }
    }

    private AsmBinary helpDealWithCmp(AsmOperand objLeft, AsmOperand objRight) {
        if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12() && objLeft instanceof AsmImm && ((AsmImm) objLeft).Is12()) {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), objLeft));
            curBlock.addInstr(move);
            return new AsmBinary("xor", Arrays.asList(new VirtualReg(), move.getDst(), objRight));
        } else {
            if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12()) {
                return new AsmBinary("xor", Arrays.asList(new VirtualReg(), objLeft, objRight));
            } else {
                return new AsmBinary("xor", Arrays.asList(new VirtualReg(), objRight, objLeft));
            }
        }
    }

    private void parseCmp(CmpInstr inst) {
        OP op = inst.getOp();
        AsmOperand dst = getOperandForNoImm(inst);
        Value left = inst.getLeft();
        Value right = inst.getRight();
        AsmOperand objLeft;
        AsmOperand objRight;
        if (op == OP.NE || op == OP.EQ || op == OP.SLT || op == OP.SGE || op == OP.SLE || op == OP.SGT) {
            objLeft = getOperandFor12Imm(left);
            objRight = getOperandFor12Imm(right);
        } else {
            objLeft = getOperandForNoImm(left);
            objRight = getOperandForNoImm(right);
        }
        if (op == OP.NE) {
            AsmBinary binary = helpDealWithCmp(objLeft, objRight);
            curBlock.addInstr(binary);
            AsmBinary binary1 = new AsmBinary("sltu", Arrays.asList(dst, ZERO, binary.getDst()));
            curBlock.addInstr(binary1);
        } else if (op == OP.EQ) {
            AsmBinary binary = helpDealWithCmp(objLeft, objRight);
            curBlock.addInstr(binary);
            AsmBinary binary1 = new AsmBinary("sltu", Arrays.asList(new VirtualReg(), ZERO, binary.getDst()));
            curBlock.addInstr(binary1);
            AsmBinary binary2 = new AsmBinary("xor", Arrays.asList(dst, binary1.getDst(), new AsmImm(1, true)));
            curBlock.addInstr(binary2);
        } else if (op == OP.SLT) {
            AsmBinary binary;
            if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12()) {
                binary = new AsmBinary("slti", Arrays.asList(dst, objLeft, objRight));
            } else if (objLeft instanceof AsmImm && ((AsmImm) objLeft).Is12()) {
                AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), objLeft));
                curBlock.addInstr(move);
                binary = new AsmBinary("sgt", Arrays.asList(dst, objRight, move.getDst()));
            } else {
                binary = new AsmBinary("slt", Arrays.asList(dst, objLeft, objRight));
            }
            curBlock.addInstr(binary);
        } else if (op == OP.SLE) {
            AsmBinary binary;
            if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12()) {
                AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), objRight));
                curBlock.addInstr(move);
                binary = new AsmBinary("sgt", Arrays.asList(new VirtualReg(), objLeft, move.getDst()));
            } else if (objLeft instanceof AsmImm && ((AsmImm) objLeft).Is12()) {
                binary = new AsmBinary("slti", Arrays.asList(new VirtualReg(), objRight, objLeft));
            } else {
                binary = new AsmBinary("sgt", Arrays.asList(new VirtualReg(), objLeft, objRight));
            }
            curBlock.addInstr(binary);
            AsmBinary asmBinary1 = new AsmBinary("xor", Arrays.asList(dst, binary.getDst(), new AsmImm(1, true)));
            curBlock.addInstr(asmBinary1);
        } else if (op == OP.SGT) {
            AsmBinary binary;
            if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12()) {
                AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), objRight));
                curBlock.addInstr(move);
                binary = new AsmBinary("sgt", Arrays.asList(dst, objLeft, move.getDst()));
            } else if (objLeft instanceof AsmImm && ((AsmImm) objLeft).Is12()) {
                binary = new AsmBinary("slti", Arrays.asList(dst, objRight, objLeft));
            } else {
                binary = new AsmBinary("sgt", Arrays.asList(dst, objLeft, objRight));
            }
            curBlock.addInstr(binary);
        } else if (op == OP.SGE) {
            AsmBinary binary;
            if (objRight instanceof AsmImm && ((AsmImm) objRight).Is12()) {
                binary = new AsmBinary("slti", Arrays.asList(new VirtualReg(), objLeft, objRight));
            } else if (objLeft instanceof AsmImm && ((AsmImm) objLeft).Is12()) {
                AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), objLeft));
                curBlock.addInstr(move);
                binary = new AsmBinary("sgt", Arrays.asList(new VirtualReg(), objRight, move.getDst()));
            } else {
                binary = new AsmBinary("slt", Arrays.asList(new VirtualReg(), objLeft, objRight));
            }
            curBlock.addInstr(binary);
            AsmBinary binary1 = new AsmBinary("xor", Arrays.asList(dst, binary.getDst(), new AsmImm(1, true)));
            curBlock.addInstr(binary1);
        } else if (op == OP.ONE) {
            AsmBinary Feq = new AsmBinary("feq.s", Arrays.asList(new VirtualReg(), objLeft, objRight));
            curBlock.addInstr(Feq);
            AsmBinary binary = new AsmBinary("xor", Arrays.asList(dst, Feq.getDst(), new AsmImm(1, true)));
            curBlock.addInstr(binary);
        } else if (op == OP.OEQ) {
            AsmBinary Feq = new AsmBinary("feq.s", Arrays.asList(dst, objLeft, objRight));
            curBlock.addInstr(Feq);
        } else if (op == OP.OLT) {
            AsmBinary Flt = new AsmBinary("flt.s", Arrays.asList(dst, objLeft, objRight));
            curBlock.addInstr(Flt);
        } else if (op == OP.OLE) {
            AsmBinary Fle = new AsmBinary("fle.s", Arrays.asList(dst, objLeft, objRight));
            curBlock.addInstr(Fle);
        } else if (op == OP.OGT) {
            AsmBinary Fgt = new AsmBinary("fgt.s", Arrays.asList(dst, objLeft, objRight));
            curBlock.addInstr(Fgt);
        } else if (op == OP.OGE) {
            AsmBinary Fge = new AsmBinary("fge.s", Arrays.asList(dst, objLeft, objRight));
            curBlock.addInstr(Fge);
        }
    }

    private void parseBr(BrInstr inst) {
        if (inst.getIsIf()) {
            if (inst.getValue() instanceof ConstInt) {
                AsmJ j;
                if (((ConstInt) inst.getValue()).getValue() == 0) {
                    j = new AsmJ((AsmBlock) valueToComponent.get(inst.getIfFalseBlock()));
                    curBlock.addInstr(j);
                } else {
                    j = new AsmJ((AsmBlock) valueToComponent.get(inst.getIfTrueBlock()));
                    curBlock.addInstr(j);
                }
            } else if (inst.getValue() instanceof CmpInstr condition) {
                AsmBeqz beqZ = new AsmBeqz(getOperandForNoImm(condition), (AsmBlock) valueToComponent.get(inst.getIfFalseBlock()));
                curBlock.addInstr(beqZ);
                AsmJ j = new AsmJ((AsmBlock) valueToComponent.get(inst.getIfTrueBlock()));
                curBlock.addInstr(j);
            }
        } else {
            AsmJ j = new AsmJ((AsmBlock) valueToComponent.get(inst.getDestBlock()));
            curBlock.addInstr(j);
        }
    }

    private void parseGep(GetElementPtrInstr inst, BasicBlock irBlock, Function function) {
        AsmOperand dst = getOperandForNoImm(inst);
        if (inst.getValueList().get(0) instanceof GlobalVar) {
            AsmLa la = new AsmLa(Arrays.asList(dst, getGlobalOperand((GlobalVar) inst.getValueList().get(0))));
            curBlock.addInstr(la);
        } else {
            AsmMove objMove = new AsmMove(Arrays.asList(dst, getOperandForNoImm(inst.getValueList().get(0))));
            curBlock.addInstr(objMove);
        }
        ArrayList<Integer> sizes = getSizes(inst.getValueList().get(0));
        for (int i = inst.getIndexes().size() - 1; i >= 0; i--) {
            if (inst.getIndexes().get(i) instanceof ConstInt) {
                int offset = sizes.get(i) * ((ConstInt) inst.getIndexes().get(i)).getValue();
                if (offset == 0) {
                    continue;
                }
                AsmOperand operand;
                if (offset >= -2048 && offset <= 2047) {
                    operand = new AsmImm(offset, true);
                } else {
                    operand = getIntOperandForImmMoreThan12(offset);
                }
                AsmBinary bin = new AsmBinary("add", Arrays.asList(dst, dst, operand));
                curBlock.addInstr(bin);
            } else {
                ALUInstr aluInstr = new ALUInstr(IntegerType.i32, Arrays.asList(inst.getIndexes().get(i), new ConstInt(IntegerType.i32, sizes.get(i))), InstrType.MUL, irBlock);
                ArrayList<Instruction> gepSim = ALUOptimize.simplifyMul(aluInstr, irBlock);
                for (Instruction instr : gepSim) {
                    parseInstruction(instr, irBlock, function);
                }
                AsmInstr lastInstr = curBlock.getInstrList().getLast();
                AsmOperand resultReg = lastInstr.getRegDef();
                AsmBinary iniOffset = new AsmBinary("add", Arrays.asList(dst, dst, resultReg));
                curBlock.addInstr(iniOffset);
            }
        }
    }

    private ArrayList<Integer> getSizes(Value value) {
        PointerType pointer = (PointerType) value.getType();
        ArrayList<Integer> sizes;
        if (pointer.getElementType().isArray()) {
            ArrayType array = (ArrayType) pointer.getElementType();
            ArrayList<Integer> dims = new ArrayList<>(List.of(1));
            int dim = array.getPos();
            for (int i = 0; i < dim - 1; i++) {
                dims.add(array.getNum());
                array = (ArrayType) array.getElementType();
            }
            dims.add(array.getNum());
            sizes = new ArrayList<>(List.of(4));
            for (int i = dim - 1; i >= 0; i--) {
                sizes.add(0, sizes.get(0) * dims.get(i + 1));
            }
        } else {
            sizes = new ArrayList<>(List.of(4));
        }
        return sizes;
    }

    private AsmOperand getOperandFor32Imm(Value value) {
        if (operandMap.containsKey(value)) {
            return operandMap.get(value);
        } else if (value instanceof ConstInt) {
            return new AsmImm(((ConstInt) value).getValue(), false);
        } else if (value instanceof ConstFloat) {
            return getConstFloatOperand(value);
        }
        return getDstOperand(value);
    }

    private AsmOperand getOperandFor12Imm(Value value) {
        if (operandMap.containsKey(value)) {
            return operandMap.get(value);
        } else {
            if (value instanceof ConstInt) {
                int intValue = ((ConstInt) value).getValue();
                if (intValue >= -2048 && intValue <= 2047) {
                    return new AsmImm(intValue, true);
                } else {
                    return getIntOperandForImmMoreThan12(intValue);
                }
            } else if (value instanceof ConstFloat) {
                return getConstFloatOperand(value);
            }
            return getDstOperand(value);
        }
    }

    private AsmOperand getOperandForNoImm(Value value) {
        if (operandMap.containsKey(value)) {
            return operandMap.get(value);
        } else {
            if (value instanceof ConstInt) {
                return getIntOperandForImmMoreThan12(((ConstInt) value).getValue());
            } else if (value instanceof ConstFloat) {
                return getConstFloatOperand(value);
            }
            return getDstOperand(value);
        }
    }

    private AsmOperand getIntOperandForImmMoreThan12(int immediate) {
        AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
        curBlock.addInstr(move);
        return move.getDst();
    }

    private AsmOperand getConstFloatOperand(Value value) {
        float floatValue = ((ConstFloat) value).getValue();
        int intValue = Float.floatToRawIntBits(floatValue);
        AsmOperand src = getDstOperand(value);
        AsmGlobalFloat constFloat = new AsmGlobalFloat(intValue);
        AsmLa la = new AsmLa(Arrays.asList(new VirtualReg(), new AsmLabel(constFloat.getName())));
        if (needChange.containsKey(intValue)) {
            needChange.get(intValue).add(la);
        } else {
            needChange.put(intValue, new ArrayList<>(List.of(la)));
        }
        curBlock.addInstr(la);
        AsmLoad asmLoad = new AsmLoad(Arrays.asList(src, la.getDst(), new AsmImm(0, true)), "flw");
        curBlock.addInstr(asmLoad);
        return src;
    }

    private AsmOperand getGlobalOperand(GlobalVar globalVar) {
        AsmGlobalVariable objG = ((AsmGlobalVariable) valueToComponent.get(globalVar));
        return new AsmLabel(objG.getName());
    }

    private void parseCall(CallInstr inst, Function function) {
        AsmFunction tarFunction = asmModule.getObjFunction(inst.getFunction());
        if (LibFunctions.containsKey(inst.getFunction().getName())) {
            tarFunction = LibFunctions.get(inst.getFunction().getName());
        }
        ArrayList<Value> fParams = new ArrayList<>();
        ArrayList<Value> iParams = new ArrayList<>();
        for (Value value : inst.getValueList()) {
            if (value.getType().isFloat()) {
                fParams.add(value);
            } else {
                iParams.add(value);
            }
        }
        int offset;
        boolean isTailRecursive = false;
        if (inst.getFunction().equals(function) && function.isTailRecursive()) {
            offset = curFunction.getStackSize();
            isTailRecursive = true;
        } else {
            offset = 0;
        }
        for (int i = 0; i < fParams.size(); i++) {
            AsmOperand operand = getOperandFor12Imm(fParams.get(i));
            if (i < 8) {
                AsmMove move = new AsmMove(Arrays.asList(fPhyA.get(i), operand));
                curBlock.addInstr(move);
            } else {
                if (operand instanceof AsmImm && ((AsmImm) operand).Is12()) {
                    AsmMove move;
                    if (fParams.get(i).getType().isFloat()) {
                        move = new AsmMove(Arrays.asList(new FVirtualReg(), operand));
                    } else {
                        move = new AsmMove(Arrays.asList(new VirtualReg(), operand));
                    }
                    curBlock.addInstr(move);
                    operand = move.getDst();
                }
                if (isTailRecursive) {
                    getStoreOpeForTail(operand, offset, "fsw");
                } else {
                    getStoreOpeForNoTail(operand, offset, "fsw");
                }
                offset += 4;
            }
        }
        for (int i = 0; i < iParams.size(); i++) {
            AsmOperand operand = getOperandFor12Imm(iParams.get(i));
            if (i < 8) {
                AsmMove move = new AsmMove(Arrays.asList(phyA.get(i), operand));
                curBlock.addInstr(move);
            } else {
                if (operand instanceof AsmImm && ((AsmImm) operand).Is12()) {
                    AsmOperand tmp;
                    if (iParams.get(i).getType().isFloat()) {
                        tmp = new FVirtualReg();
                    } else {
                        tmp = new VirtualReg();
                    }
                    AsmMove move = new AsmMove(Arrays.asList(tmp, operand));
                    curBlock.addInstr(move);
                    operand = tmp;
                }
                if (iParams.get(i).getType() instanceof IntegerType) {
                    if (isTailRecursive) {
                        getStoreOpeForTail(operand, offset, "sw");
                    } else {
                        getStoreOpeForNoTail(operand, offset, "sw");
                    }
                    offset += 4;
                } else {
                    if (isTailRecursive) {
                        getStoreOpeForTail(operand, offset, "sd");
                    } else {
                        getStoreOpeForNoTail(operand, offset, "sd");
                    }
                    offset += 8;
                }
            }
        }
        AsmCall asmCall = new AsmCall(tarFunction);
        curBlock.addInstr(asmCall);
        //返回值
        if (!(inst.getType() instanceof VoidType)) {
            AsmOperand dst = getOperandForNoImm(inst);
            if (inst.getType() instanceof IntegerType) {
                AsmMove move = new AsmMove(Arrays.asList(dst, phyA.get(0)));
                curBlock.addInstr(move);
            } else if (inst.getType() instanceof FloatType) {
                AsmMove move = new AsmMove(Arrays.asList(dst, fPhyA.get(0)));
                curBlock.addInstr(move);
            }
        }
    }

    private void parseConversionInst(ConversionInstr inst) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src = getOperandForNoImm(inst.getValue(0));
        if (inst.getInstrType() == InstrType.FPTOSI) {
            AsmConversion objC = new AsmConversion("fcvt.w.s", Arrays.asList(dst, src));
            curBlock.addInstr(objC);
        } else if (inst.getInstrType() == InstrType.SITOFP) {
            AsmConversion objC = new AsmConversion("fcvt.s.w", Arrays.asList(dst, src));
            curBlock.addInstr(objC);
        } else if (inst.getInstrType() == InstrType.ZEXT) {
            AsmMove objM = new AsmMove(Arrays.asList(dst, src));
            curBlock.addInstr(objM);
        } else if (inst.getInstrType() == InstrType.BITCAST) {
            AsmMove objM = new AsmMove(Arrays.asList(dst, src));
            curBlock.addInstr(objM);
        }
    }

    private void parseAdd(ALUInstr inst) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src1, src2;
        if (inst.getLeft() instanceof ConstInt && inst.getRight() instanceof ConstInt) {
            AsmMove move = new AsmMove(Arrays.asList(dst, new AsmImm(((ConstInt) inst.getLeft()).getValue() + ((ConstInt) inst.getRight()).getValue(), false)));
            curBlock.addInstr(move);
            return;
        } else {
            if (inst.getLeft() instanceof ConstInt) {
                src1 = getIntOperandForImmMoreThan12(((ConstInt) inst.getLeft()).getValue());
            } else {
                src1 = getOperandForNoImm(inst.getLeft());
            }
            if (inst.getRight() instanceof ConstInt) {
                src2 = getIntOperandForImmMoreThan12(((ConstInt) inst.getRight()).getValue());
            } else {
                src2 = getOperandForNoImm(inst.getRight());
            }
        }
        AsmBinary add = new AsmBinary("addw", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(add);
    }

    private void parseFAdd(ALUInstr inst) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandForNoImm(inst.getRight());
        AsmBinary objFAdd = new AsmBinary("fadd.s", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(objFAdd);
    }

    private void parseMul(ALUInstr inst) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandForNoImm(inst.getRight());
        if (inst.getInstrType() == InstrType.MUL) {
            AsmBinary objMul;
            if (inst.getIs64()) {
                objMul = new AsmBinary("mul", Arrays.asList(dst, src1, src2));
            } else {
                objMul = new AsmBinary("mulw", Arrays.asList(dst, src1, src2));
            }
            curBlock.addInstr(objMul);
        } else if (inst.getInstrType() == InstrType.FMUL) {
            AsmBinary objMul = new AsmBinary("fmul.s", Arrays.asList(dst, src1, src2));
            curBlock.addInstr(objMul);
        }
    }

    private void parseSub(ALUInstr inst) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src1, src2;
        if (inst.getLeft() instanceof ConstInt && inst.getRight() instanceof ConstInt) {
            AsmMove move = new AsmMove(Arrays.asList(dst, new AsmImm(((ConstInt) inst.getLeft()).getValue() - ((ConstInt) inst.getRight()).getValue(), false)));
            curBlock.addInstr(move);
            return;
        } else {
            if (inst.getLeft() instanceof ConstInt) {
                src1 = getIntOperandForImmMoreThan12(((ConstInt) inst.getLeft()).getValue());
            } else {
                src1 = getOperandForNoImm(inst.getLeft());
            }
            if (inst.getRight() instanceof ConstInt) {
                src2 = getIntOperandForImmMoreThan12(((ConstInt) inst.getRight()).getValue());
            } else {
                src2 = getOperandForNoImm(inst.getRight());
            }
        }
        AsmBinary sub = new AsmBinary("subw", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(sub);
    }

    private void parseFSub(ALUInstr inst) {
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandForNoImm(inst.getRight());
        AsmOperand dst = getOperandForNoImm(inst);
        AsmBinary objFSub = new AsmBinary("fsub.s", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(objFSub);
    }

    private void parseDiv(ALUInstr inst, BasicBlock irBlock, Function function) {
        AsmOperand dst = getOperandForNoImm(inst);
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandForNoImm(inst.getRight());
        if (inst.getInstrType() == InstrType.SDIV) {
            ArrayList<Instruction> instructions = ALUOptimize.simplifyDiv(inst, irBlock);
            if (instructions.isEmpty()) {
                AsmBinary objSDiv = new AsmBinary("divw", Arrays.asList(dst, src1, src2));
                curBlock.addInstr(objSDiv);
            } else {
                for (Instruction instr : instructions) {
                    parseInstruction(instr, irBlock, function);
                }
                AsmInstr lastInstr = curBlock.getInstrList().getLast();
                AsmReg defReg = lastInstr.getRegDef();
                operandMap.replace(inst, defReg);
            }
        } else if (inst.getInstrType() == InstrType.FDIV) {
            AsmBinary objFDiv = new AsmBinary("fdiv.s", Arrays.asList(dst, src1, src2));
            curBlock.addInstr(objFDiv);
        }
    }

    private void parseShl(ALUInstr inst) {
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandFor12Imm(inst.getRight());
        AsmOperand dst = getOperandForNoImm(inst);
        AsmBinary objShl = new AsmBinary("sll", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(objShl);
    }

    private void parseLShl(ALUInstr inst) {
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandFor12Imm(inst.getRight());
        AsmOperand dst = getOperandForNoImm(inst);
        AsmBinary objLShl = new AsmBinary("srl", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(objLShl);
    }

    private void parseAShr(ALUInstr inst) {
        AsmOperand src1 = getOperandForNoImm(inst.getLeft());
        AsmOperand src2 = getOperandFor12Imm(inst.getRight());
        AsmOperand dst = getOperandForNoImm(inst);
        AsmBinary objAShr = new AsmBinary("sra", Arrays.asList(dst, src1, src2));
        curBlock.addInstr(objAShr);
    }

    private AsmOperand getDstOperand(Value irValue) {
        if (irValue.getType() instanceof FloatType) {
            FVirtualReg dstReg = new FVirtualReg();
            if (!(irValue instanceof Constant)) {
                operandMap.put(irValue, dstReg);
            }
            return dstReg;
        } else {
            VirtualReg dstReg = new VirtualReg();
            if (!(irValue instanceof Constant)) {
                operandMap.put(irValue, dstReg);
            }
            return dstReg;
        }
    }

    private void getStoreOpeForNoTail(AsmOperand src, int immediate, String type) {
        if (immediate >= -2048 && immediate <= 2047) {
            AsmStore store = new AsmStore(Arrays.asList(src, SP, new AsmImm(immediate, true)), type);
            curBlock.addInstr(store);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
            AsmStore store = new AsmStore(Arrays.asList(src, add.getDst(), new AsmImm(0, true)), type);
            curBlock.addInstr(move);
            curBlock.addInstr(add);
            curBlock.addInstr(store);
        }
    }

    private void getStoreOpeForTail(AsmOperand src, int immediate, String type) {
        AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
        move.setNeedChange();
        AsmBinary binary = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
        AsmStore store = new AsmStore(Arrays.asList(src, binary.getDst(), new AsmImm(0, true)), type);
        curBlock.addInstr(move);
        curBlock.addInstr(binary);
        curBlock.addInstr(store);
    }

    private void loadRa(int immediate) {
        if (immediate >= -2048 && immediate <= 2047) {
            AsmLoad load = new AsmLoad(Arrays.asList(RA, SP, new AsmImm(immediate, true)), "ld");
            curBlock.addInstr(load);
        } else {
            AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
            AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
            AsmLoad load = new AsmLoad(Arrays.asList(RA, add.getDst(), new AsmImm(0, true)), "ld");
            curBlock.addInstr(load);
            curBlock.insertBefore(load, add);
            curBlock.insertBefore(add, move);
        }
    }

    private void getLoadInsertHead(AsmOperand dst, int immediate, String type) {
        AsmMove move = new AsmMove(Arrays.asList(new VirtualReg(), new AsmImm(immediate, false)));
        move.setNeedChange();
        AsmBinary add = new AsmBinary("add", Arrays.asList(new VirtualReg(), SP, move.getDst()));
        AsmLoad load = new AsmLoad(Arrays.asList(dst, add.getDst(), new AsmImm(0, true)), type);
        curBlock.insertHead(load);
        curBlock.insertHead(add);
        curBlock.insertHead(move);
    }

    private void parseMove(MoveInstr inst) {
        AsmOperand src = getOperandFor12Imm(inst.getSrc());
        AsmOperand dst = getOperandForNoImm(inst.getDst());
        AsmMove move = new AsmMove(Arrays.asList(dst, src));
        curBlock.addInstr(move);
    }

    private void changeLabel() {
        for (int key : needChange.keySet()) {
            int size = needChange.get(key).size();
            if (size >= 1) {
                AsmGlobalFloat constFloat = new AsmGlobalFloat(key);
                for (AsmInstr instr : needChange.get(key)) {
                    AsmLabel newLabel = new AsmLabel(constFloat.getName());
                    ((AsmLa) instr).setAddress(newLabel);
                }
                asmModule.addGlobalVariable(constFloat);
            }
        }
    }
}