package backend;

import Utils.CustomList.Node;
import backend.asmInstr.asmBinary.*;
import backend.asmInstr.asmBr.AsmBnez;
import backend.asmInstr.asmBr.AsmJ;
import backend.asmInstr.asmConv.AsmFtoi;
import backend.asmInstr.asmConv.AsmitoF;
import backend.asmInstr.asmLS.*;
import backend.asmInstr.asmTermin.AsmCall;
import backend.asmInstr.asmTermin.AsmRet;
import backend.itemStructure.*;
import backend.regs.AsmFVirReg;
import backend.regs.AsmVirReg;
import backend.regs.RegGeter;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstBool;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.convop.ConversionOperation;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.MoveInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.otherop.cmp.CmpCond;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.unaryop.FNegInstr;
import frontend.ir.structure.*;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import static backend.regs.RegGeter.ZERO;
import static frontend.ir.DataType.*;

public class IrParser {
    private Program program;
    private AsmModule asmModule = new AsmModule();
    //全局变量的c表示映射到全局变量的asm表示。全局变量的llvm表示是标签，对于asm来说没有意义
    private HashMap<Symbol, AsmGlobalVar> gvMap = new HashMap<>();
    private HashMap<Function, AsmFunction> funcMap = new HashMap<>();
    private HashMap<BasicBlock, AsmBlock> blockMap = new HashMap<>();
    //llvm的虚拟寄存器或立即数映射到asm的物理寄存器或立即数
    private HashMap<Value, AsmOperand> operandMap = new HashMap<>();
    public HashMap<AsmOperand, Value> downOperandMap = new HashMap<>();
    //指示对应浮点数值映射到的标签
    private HashMap<Integer, AsmLabel> floatLabelMap = new HashMap<>();
    private HashMap<Group<AsmBlock, Group<AsmOperand, AsmOperand>>, AsmOperand> blockDivExp2Res = new HashMap<>();
    private HashMap<Group<AsmBlock, Group<AsmOperand, AsmOperand>>, AsmOperand> blockMulExp2Res = new HashMap<>();

    public IrParser(Program program) {
        this.program = program;
    }


    public AsmModule parse() {
        parseGlobalVars();
        parseFunctions();
        return asmModule;
    }

    private void parseGlobalVars() {
        List<Symbol> globalVars = program.getGlobalVars();
        for (Symbol globalVar : globalVars) {
            AsmGlobalVar asmGlobalVar = parseGlobalVar(globalVar);
            asmModule.addGlobalVar(asmGlobalVar);
            gvMap.put(globalVar, asmGlobalVar);
        }
    }

    private AsmGlobalVar parseGlobalVar(Symbol globalVar) {
        AsmType type = globalVar.getAsmType();
        if (type == AsmType.INT) {
            boolean isInit = (globalVar.getInitVal() != null);
            if (!isInit)
                return new AsmGlobalVar(globalVar.getName());
            Number initVal = globalVar.getValue();
            return new AsmGlobalVar(globalVar.getName(), initVal);
        }
        if (type == AsmType.FLOAT) {
            boolean isInit = (globalVar.getInitVal() != null);
            if (!isInit)
                return new AsmGlobalVar(globalVar.getName());
            Number floatVal = globalVar.getValue();
            Number initVal = Float.floatToRawIntBits(floatVal.floatValue());
            return new AsmGlobalVar(globalVar.getName(), initVal);
        }
        if (type == AsmType.ARRAY) {
            boolean isInit = (globalVar.getInitVal() != null);
            if (!isInit)
                return new AsmGlobalVar(globalVar.getName(), globalVar.getLimSize() * 4);
            return new AsmGlobalVar(globalVar);
        }
        throw new RuntimeException("全局变量类型错误");
    }


    private void parseFunctions() {
        createMaps();
        for (Function f : program.getFunctionList()) {
            parseFunction(f);
        }
    }

    private void createMaps() {
        for (Function f : program.getFunctionList()) {
            AsmFunction asmFunction = new AsmFunction(f.getName());
            asmModule.addFunction(asmFunction);
            funcMap.put(f, asmFunction);
            for (Node bb : f.getBasicBlocks()) {
                AsmBlock asmBlock = new AsmBlock(f.getName() + "_" + ((BasicBlock) bb).value2string(), ((BasicBlock) bb).getLoopDepth());
                asmFunction.addBlock(asmBlock);
                blockMap.put((BasicBlock) bb, asmBlock);
            }
            for (Node bb : f.getBasicBlocks()) {
                AsmBlock asmBlock = blockMap.get(bb);
                for (BasicBlock anotherBb : ((BasicBlock) bb).getSucs()) {
                    asmBlock.sucs.add(blockMap.get(anotherBb));
                }
                for (BasicBlock anotherBb : ((BasicBlock) bb).getPres()) {
                    asmBlock.pres.add(blockMap.get(anotherBb));
                }
                for (BasicBlock anotherBb : ((BasicBlock) bb).getDoms()) {
                    asmBlock.doms.add(blockMap.get(anotherBb));
                }
                for (BasicBlock anotherBb : ((BasicBlock) bb).getIDoms()) {
                    asmBlock.iDoms.add(blockMap.get(anotherBb));
                }
            }
        }
    }

    private void parseFunction(Function f) {
        AsmFunction asmFunction = funcMap.get(f);
        int pushArgSize = 0;
        for (Node bb : f.getBasicBlocks()) {
            for (Node instr : ((BasicBlock) bb).getInstructions()) {
                if (instr instanceof CallInstr callInstr) {
                    if (f.isTailRecursive() && callInstr.getFuncDef().getName().equals(f.getName())) {
                        continue;
                    }
                    List<Value> rParams = callInstr.getRParams();
                    int size = rParams.size();
                    for (int i = 8; i < size; i++) {
                        if ((rParams.get(i)).getPointerLevel() != 0) {
                            pushArgSize += 8;
                        } else {
                            pushArgSize += 4;
                        }
                    }
                }
            }
        }
        //+8是ra的大小
        asmFunction.setArgsSize(pushArgSize);
        asmFunction.setRaSize(8);

        for (Node bb : f.getBasicBlocks()) {
            parseBlock((BasicBlock) bb, f);
        }

        BasicBlock bb = (BasicBlock) f.getBasicBlocks().getHead();
        List<FParam> args = f.getFParamValueList();
        List<Value> iargs = new ArrayList<>();
        List<Value> fargs = new ArrayList<>();
        for (int i = 0; i < args.size(); i++) {
            Value arg = args.get(i);
            if (arg.getPointerLevel() != 0) {
                iargs.add(arg);
            } else if (arg.getDataType() == FLOAT) {
                fargs.add(arg);
            } else {
                iargs.add(arg);
            }
        }
        asmFunction.addIntAndFloatFunctions(iargs, fargs);
        int iargnum = Math.min(iargs.size(), 8);
        int fargnum = Math.min(fargs.size(), 8);
        for (int i = 0; i < iargnum; i++) {
            Value arg = iargs.get(i);
            AsmOperand asmOperand = parseOperand(arg, 0, f, bb);
            AsmMove asmMove = new AsmMove(asmOperand, RegGeter.AregsInt.get(i));
            blockMap.get(bb).addInstrHead(asmMove);
        }
        for (int i = 0; i < fargnum; i++) {
            Value arg = fargs.get(i);
            AsmOperand asmOperand = parseOperand(arg, 0, f, bb);
            AsmMove asmMove = new AsmMove(asmOperand, RegGeter.AregsFloat.get(i));
            blockMap.get(bb).addInstrHead(asmMove);
        }

        int offset = asmFunction.getWholeSize();
        if (iargs.size() > 8 || fargs.size() > 8) {
            for (int i = 8; i < iargs.size(); i++) {
                Value arg = iargs.get(i);
                AsmOperand asmOperand = parseOperand(arg, 0, f, bb);
                if (arg.getPointerLevel() != 0) {
                    AsmLd asmLd = new AsmLd(asmOperand, RegGeter.SP, new AsmImm32(offset), 1);
                    blockMap.get(bb).addInstrHead(asmLd);
                    offset += 8;
                } else {
                    AsmLw asmLw = new AsmLw(asmOperand, RegGeter.SP, new AsmImm32(offset), 1);
                    blockMap.get(bb).addInstrHead(asmLw);
                    offset += 4;
                }
            }
            for (int i = 8; i < fargs.size(); i++) {
                Value arg = fargs.get(i);
                AsmOperand asmOperand = parseOperand(arg, 0, f, bb);
                AsmFlw asmFlw = new AsmFlw(asmOperand, RegGeter.SP, new AsmImm32(offset), 1);
                blockMap.get(bb).addInstrHead(asmFlw);
                offset += 4;
            }
        }
        if (f.isTailRecursive()) {
            HashSet<AsmCall> callsToBeRemoved = new HashSet<>();
            AsmBlock bstart = new AsmBlock(f.getName() + "_bstart", 0);
            AsmBlock formerStart = (AsmBlock) (asmFunction.getHead());
            AsmJ asmJ0 = new AsmJ(formerStart);
            bstart.addInstrTail(asmJ0);
            formerStart.addJumpToInstr(asmJ0);
            asmFunction.addBlockToHead(bstart);
            for (Node asmBlock : asmFunction.getBlocks()) {
                boolean flag = false;
                for (Node instr : ((AsmBlock) asmBlock).getInstrs()) {
                    if (instr instanceof AsmCall call) {
                        if (call.getFuncName().equals(f.getName())) {
                            AsmJ asmJ = new AsmJ(formerStart, call.IntArgRegNum, call.floatArgRegNum);
                            asmJ.insertBefore(call);
                            formerStart.addJumpToInstr(asmJ);
                            callsToBeRemoved.add(call);
                            flag = true;
                            continue;
                        }
                        if (flag) {
                            callsToBeRemoved.add(call);
                        }
                    }
                }
                if (!callsToBeRemoved.isEmpty()) {
                    for (AsmCall call : callsToBeRemoved) {
                        call.removeFromList();
                    }
                }
                callsToBeRemoved.clear();
            }
        }


//        offset = asmFunction.getWholeSize() - 8;
//        if (asmFunction.getRaSize() != 0) {
//            if (offset >= -2048 && offset <= 2047) {
//                AsmSd asmSd = new AsmSd(RegGeter.RA, RegGeter.SP, new AsmImm12(offset));
//                blockMap.get(bb).addInstrHead(asmSd);
//            } else {
//                AsmOperand tmpMove = genTmpReg(f);
//                AsmMove asmMove = new AsmMove(tmpMove, new AsmImm32(offset));
//                AsmOperand tmpAdd = genTmpReg(f);
//                AsmAdd asmAdd = new AsmAdd(tmpAdd, RegGeter.SP, tmpMove);
//                AsmSd asmSd = new AsmSd(RegGeter.RA, tmpAdd, new AsmImm12(0));
//                blockMap.get(bb).addInstrHead(asmSd);
//                blockMap.get(bb).addInstrHead(asmAdd);
//                blockMap.get(bb).addInstrHead(asmMove);
//            }
//        }
//        offset = -asmFunction.getWholeSize();
//        if (offset >= -2048 && offset <= 2047) {
//            AsmAdd asmAdd = new AsmAdd(RegGeter.SP, RegGeter.SP, new AsmImm12(offset));
//            blockMap.get(bb).addInstrHead(asmAdd);
//        } else {
//            AsmOperand tmpMove = genTmpReg(f);
//            AsmMove asmMove = new AsmMove(tmpMove, new AsmImm32(offset));
//            AsmAdd asmAdd = new AsmAdd(RegGeter.SP, RegGeter.SP, tmpMove);
//            blockMap.get(bb).addInstrHead(asmAdd);
//            blockMap.get(bb).addInstrHead(asmMove);
//        }
    }

    private void parseBlock(BasicBlock bb, Function f) {
        for (Node instr : bb.getInstructions()) {
            parseInstr((Instruction) instr, f, bb);
        }
    }

    private void parseInstr(Instruction instr, Function f, BasicBlock bb) {
        if (instr instanceof ReturnInstr)
            parseRet((ReturnInstr) instr, bb, f);
        else if (instr instanceof AllocaInstr)
            parseAlloca((AllocaInstr) instr, bb, f);
        else if (instr instanceof LoadInstr)
            parseLoad((LoadInstr) instr, bb, f);
        else if (instr instanceof StoreInstr)
            parseStore((StoreInstr) instr, bb, f);
        else if (instr instanceof Cmp)
            parseCmp((Cmp) instr, bb, f);
        else if (instr instanceof AddInstr)
            parseAdd((AddInstr) instr, bb, f);
        else if (instr instanceof FAddInstr)
            parseFAdd((FAddInstr) instr, bb, f);
        else if (instr instanceof SubInstr)
            parseSub((SubInstr) instr, bb, f);
        else if (instr instanceof FSubInstr)
            parseFSub((FSubInstr) instr, bb, f);
        else if (instr instanceof MulInstr)
            parseMul((MulInstr) instr, bb, f);
        else if (instr instanceof FMulInstr)
            parseFMul((FMulInstr) instr, bb, f);
        else if (instr instanceof SDivInstr)
            parseSDiv((SDivInstr) instr, bb, f);
        else if (instr instanceof FDivInstr)
            parseFDiv((FDivInstr) instr, bb, f);
        else if (instr instanceof SRemInstr)
            parseMod((SRemInstr) instr, bb, f);
        else if (instr instanceof BranchInstr)
            parseBr((BranchInstr) instr, bb, f);
        else if (instr instanceof JumpInstr)
            parseJump((JumpInstr) instr, bb, f);
        else if (instr instanceof CallInstr)
            parseCall((CallInstr) instr, bb, f);
        else if (instr instanceof GEPInstr)
            parseGEP((GEPInstr) instr, bb, f);
        else if (instr instanceof MoveInstr)
            parseMove((MoveInstr) instr, bb, f);
        else if (instr instanceof ConversionOperation)
            parseConv((ConversionOperation) instr, bb, f);
        else if (instr instanceof ShlInstr)
            parseShl((ShlInstr) instr, bb, f);
        else if (instr instanceof AShrInstr)
            parseShr((AShrInstr) instr, bb, f);
        else if (instr instanceof FNegInstr)
            parseFNeg((FNegInstr) instr, bb, f);
        else
            throw new RuntimeException("未知指令" + instr.print());
    }

    private void parseRet(ReturnInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        Value retValue = instr.getReturnValue();
        if (retValue != null) {
            if (retValue.getPointerLevel() != 0) {
                AsmOperand asmOperand = parseOperand(retValue, 0, f, bb);
                AsmMove asmMove = new AsmMove(RegGeter.AregsInt.get(0), asmOperand);
                asmBlock.addInstrTail(asmMove);
            } else if (f.getDataType() == INT) {
                AsmOperand asmOperand = parseOperand(retValue, 32, f, bb);
                AsmMove asmMove = new AsmMove(RegGeter.AregsInt.get(0), asmOperand);
                asmBlock.addInstrTail(asmMove);
            } else {
                AsmOperand asmOperand = parseOperand(retValue, 32, f, bb);
                AsmMove asmMove = new AsmMove(RegGeter.AregsFloat.get(0), asmOperand);
                asmBlock.addInstrTail(asmMove);
            }
        }
        AsmFunction asmFunction = funcMap.get(f);
//        int offset = asmFunction.getWholeSize() - 8;
//        if (asmFunction.getRaSize() != 0) {
//            if (offset >= -2048 && offset <= 2047) {
//                AsmLd asmLd = new AsmLd(RegGeter.RA, RegGeter.SP, new AsmImm12(offset));
//                asmBlock.addInstrTail(asmLd);
//            } else {
//                AsmOperand tmpMove = genTmpReg(f);
//                AsmMove asmMove = new AsmMove(tmpMove, new AsmImm32(offset));
//                AsmOperand tmpAdd = genTmpReg(f);
//                AsmAdd asmAdd = new AsmAdd(tmpAdd, RegGeter.SP, tmpMove);
//                AsmLd asmLd = new AsmLd(RegGeter.RA, tmpAdd, new AsmImm12(0));
//                asmBlock.addInstrTail(asmMove);
//                asmBlock.addInstrTail(asmAdd);
//                asmBlock.addInstrTail(asmLd);
//            }
//        }
//        AsmAdd asmAdd = new AsmAdd(RegGeter.SP, RegGeter.SP, parseConstIntOperand(asmFunction.getWholeSize(), 12, f, bb));
//        asmBlock.addInstrTail(asmAdd);
        asmBlock.addInstrTail(new AsmRet());
    }

    private void parseAlloca(AllocaInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmFunction asmFunction = funcMap.get(f);
        AsmType type;
        if (instr.getSymbol().getAsmType() == AsmType.ARRAY) {
            type = AsmType.ARRAY;
        } else if (instr.getPointerLevel() > 1) {
            type = AsmType.POINTER;
        } else {
            type = instr.getSymbol().getAsmType();
        }
        int offset = asmFunction.getArgsSize() + asmFunction.getAllocaSize();
        AsmOperand offOp = parseConstIntOperand(offset, 12, f, bb);
        if (type == AsmType.ARRAY) {
            if (instr.getSymbol().isArrayFParam()) {
                asmFunction.addAllocaSize(8);
            } else {
                asmFunction.addAllocaSize(4 * instr.getSymbol().getLimSize());
            }
        } else if (type == AsmType.POINTER) {
            asmFunction.addAllocaSize(8);
        } else {
            asmFunction.addAllocaSize(4);
        }
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmAdd asmAdd = new AsmAdd(dst, RegGeter.SP, offOp);
        asmBlock.addInstrTail(asmAdd);
    }

    private void parseLoad(LoadInstr instr, BasicBlock bb, Function f) {
        //%4 = load float, float* @g       la s0,g     ;  lw s1,0(s0)
        //dst                    addr
        AsmBlock asmBlock = blockMap.get(bb);
        Symbol addr = instr.getSymbol();
        if (instr.getPtr() instanceof GlobalObject && addr.isGlobal()) {
            AsmOperand laReg = genTmpReg(f);
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            AsmOperand src = parseGlobalToOperand(addr, bb);
            AsmLa asmLa = new AsmLa(laReg, src);
            asmBlock.addInstrTail(asmLa);
            AsmOperand offset = new AsmImm12(0);
            if (dst instanceof AsmVirReg) {
                AsmLw asmLw = new AsmLw(dst, laReg, offset);
                asmBlock.addInstrTail(asmLw);
            } else {
                AsmFlw asmFLw = new AsmFlw(dst, laReg, offset);
                asmBlock.addInstrTail(asmFLw);
            }
        } else if (instr.getPointerLevel() != 0) {
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            AsmOperand src;
            if (instr.getPtr() != null) {
                src = parseOperand(instr.getPtr(), 0, f, bb);
            } else {
                src = parseOperand(addr.getAllocValue(), 0, f, bb);
            }
            AsmOperand offset = new AsmImm12(0);
            AsmLd asmLd = new AsmLd(dst, src, offset);
            asmBlock.addInstrTail(asmLd);
        } else {
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            AsmOperand src;
            if (instr.getPtr() != null) {
                src = parseOperand(instr.getPtr(), 0, f, bb);
            } else {
                src = parseOperand(addr.getAllocValue(), 0, f, bb);
            }
            AsmOperand offset = new AsmImm12(0);
            if (dst instanceof AsmVirReg) {
                AsmLw asmLw = new AsmLw(dst, src, offset);
                asmBlock.addInstrTail(asmLw);
            } else {
                AsmFlw asmFLw = new AsmFlw(dst, src, offset);
                asmBlock.addInstrTail(asmFLw);
            }
        }
    }

    private void parseStore(StoreInstr instr, BasicBlock bb, Function f) {
        //store float %4, float* %5
        //sw s0,0(s1)
        AsmBlock asmBlock = blockMap.get(bb);
        Symbol addr = instr.getSymbol();
        if (instr.getPtr() instanceof GlobalObject && addr.isGlobal()) {
            AsmOperand laReg = genTmpReg(f);
            AsmOperand addrOp = parseGlobalToOperand(addr, bb);
            AsmLa asmLa = new AsmLa(laReg, addrOp);
            asmBlock.addInstrTail(asmLa);
            AsmOperand src = parseOperand(instr.getValue(), 0, f, bb);
            AsmOperand offset = new AsmImm12(0);
            if (src instanceof AsmVirReg) {
                AsmSw asmSw = new AsmSw(src, laReg, offset);
                asmBlock.addInstrTail(asmSw);
            } else {
                AsmFsw asmFsw = new AsmFsw(src, laReg, offset);
                asmBlock.addInstrTail(asmFsw);
            }
        } else if (instr.getValue().getPointerLevel() != 0) {
            AsmOperand src = parseOperand(instr.getValue(), 0, f, bb);
            AsmOperand dst;
            if (instr.getPtr() != null) {
                dst = parseOperand(instr.getPtr(), 0, f, bb);
            } else {
                dst = parseOperand(addr.getAllocValue(), 0, f, bb);
            }
            AsmOperand offset = new AsmImm12(0);
            AsmSd asmSd = new AsmSd(src, dst, offset);
            asmBlock.addInstrTail(asmSd);
        } else {
            AsmOperand src = parseOperand(instr.getValue(), 0, f, bb);
            AsmOperand dst;
            if (instr.getPtr() != null) {
                dst = parseOperand(instr.getPtr(), 0, f, bb);
            } else {
                dst = parseOperand(addr.getAllocValue(), 0, f, bb);
            }
            AsmOperand offset = new AsmImm12(0);
            if (src instanceof AsmVirReg) {
                AsmSw asmSw = new AsmSw(src, dst, offset);
                asmBlock.addInstrTail(asmSw);
            } else {
                AsmFsw asmFsw = new AsmFsw(src, dst, offset);
                asmBlock.addInstrTail(asmFsw);
            }
        }
    }

    private void parseCmp(Cmp instr, BasicBlock bb, Function f) {
        CmpCond cond = instr.getCond();
        Value op1 = instr.getOp1();
        Value op2 = instr.getOp2();
        boolean isFloat = !(op1.getDataType() == INT && op2.getDataType() == INT);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmBlock asmBlock = blockMap.get(bb);

        if (cond == CmpCond.NE) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 12, f, bb);
                AsmOperand tmp = genTmpReg(f);
                AsmXor asmXor = new AsmXor(tmp, left, right);
                asmBlock.addInstrTail(asmXor);
                AsmSltu asmSltu = new AsmSltu(dst, ZERO, tmp);
                asmBlock.addInstrTail(asmSltu);
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmOperand tmp = genTmpReg(f);
                AsmFeq asmFeq = new AsmFeq(tmp, left, right);
                asmBlock.addInstrTail(asmFeq);
                AsmXor asmXor = new AsmXor(dst, tmp, new AsmImm12(1));
                asmBlock.addInstrTail(asmXor);
            }
        } else if (cond == CmpCond.EQ) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 12, f, bb);
                AsmOperand tmp = genTmpReg(f);
                AsmXor asmXor = new AsmXor(tmp, left, right);
                asmBlock.addInstrTail(asmXor);
                AsmOperand tmp2 = genTmpReg(f);
                AsmSltu asmSltu = new AsmSltu(tmp2, ZERO, tmp);
                asmBlock.addInstrTail(asmSltu);
                AsmXor asmXor2 = new AsmXor(dst, tmp2, new AsmImm12(1));
                asmBlock.addInstrTail(asmXor2);
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmFeq asmFeq = new AsmFeq(dst, left, right);
                asmBlock.addInstrTail(asmFeq);
            }
        } else if (cond == CmpCond.LT) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 12, f, bb);
                if (right instanceof AsmImm12) {
                    AsmSlti asmSlti = new AsmSlti(dst, left, right);
                    asmBlock.addInstrTail(asmSlti);
                } else {
                    AsmSlt asmSlt = new AsmSlt(dst, left, right);
                    asmBlock.addInstrTail(asmSlt);
                }
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmFlt asmFlt = new AsmFlt(dst, left, right);
                asmBlock.addInstrTail(asmFlt);
            }
        } else if (cond == CmpCond.LE) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 12, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmOperand tmp = genTmpReg(f);
                if (left instanceof AsmImm12) {
                    AsmSlti asmSlti = new AsmSlti(tmp, right, left);
                    asmBlock.addInstrTail(asmSlti);
                } else {
                    AsmSlt asmSlt = new AsmSlt(tmp, right, left);
                    asmBlock.addInstrTail(asmSlt);
                }
                AsmXor asmXor = new AsmXor(dst, tmp, new AsmImm12(1));
                asmBlock.addInstrTail(asmXor);
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmFle asmFle = new AsmFle(dst, left, right);
                asmBlock.addInstrTail(asmFle);
            }
        } else if (cond == CmpCond.GT) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 12, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                if (left instanceof AsmImm12) {
                    AsmSlti asmSlti = new AsmSlti(dst, right, left);
                    asmBlock.addInstrTail(asmSlti);
                } else {
                    AsmSlt asmSlt = new AsmSlt(dst, right, left);
                    asmBlock.addInstrTail(asmSlt);
                }
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmFgt asmFgt = new AsmFgt(dst, left, right);
                asmBlock.addInstrTail(asmFgt);
            }
        } else if (cond == CmpCond.GE) {
            if (!isFloat) {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 12, f, bb);
                AsmOperand tmp = genTmpReg(f);
                if (right instanceof AsmImm12) {
                    AsmSlti asmSlti = new AsmSlti(tmp, left, right);
                    asmBlock.addInstrTail(asmSlti);
                } else {
                    AsmSlt asmSlt = new AsmSlt(tmp, left, right);
                    asmBlock.addInstrTail(asmSlt);
                }
                AsmXor asmXor = new AsmXor(dst, tmp, new AsmImm12(1));
                asmBlock.addInstrTail(asmXor);
            } else {
                AsmOperand left = parseOperand(op1, 0, f, bb);
                AsmOperand right = parseOperand(op2, 0, f, bb);
                AsmFge asmFge = new AsmFge(dst, left, right);
                asmBlock.addInstrTail(asmFge);
            }
        }
    }

    private void parseAdd(AddInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        Value src1 = instr.getOp1();
        Value src2 = instr.getOp2();
        boolean isConst1 = src1 instanceof ConstInt;
        boolean isConst2 = src2 instanceof ConstInt;
        if (isConst1 && isConst2) {
            int value1 = ((ConstInt) src1).getNumber();
            int value2 = ((ConstInt) src2).getNumber();
            AsmOperand imm = new AsmImm32(value1 + value2);
            AsmMove asmMove = new AsmMove(dst, imm);
            asmBlock.addInstrTail(asmMove);
        } else if (isConst1) {
            AsmOperand imm = parseOperand(src1, 12, f, bb);
            AsmOperand virReg = parseOperand(src2, 0, f, bb);
            AsmAdd asmAdd = new AsmAdd(dst, virReg, imm);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
        } else {
            AsmOperand left = parseOperand(src1, 0, f, bb);
            AsmOperand right = parseOperand(src2, 12, f, bb);
            AsmAdd asmAdd = new AsmAdd(dst, left, right);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
        }
    }

    private void parseMul(MulInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        if (instr.is64) {
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
            AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
            AsmMul asmMul = new AsmMul(dst, src1, src2);
            asmBlock.addInstrTail(asmMul);
            return;
        }
        boolean isSrc2Const = instr.getOp2() instanceof ConstInt;
        if (isSrc2Const) {
            AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
            int value = ((ConstInt) instr.getOp2()).getNumber();
            Group<AsmOperand, AsmOperand> mulExp = new Group<>(src1, new AsmImm32(value));
            Group<AsmBlock, Group<AsmOperand, AsmOperand>> mulExpInBlock = new Group<>(asmBlock, mulExp);
            if (blockMulExp2Res.containsKey(mulExpInBlock)) {
                operandMap.put(instr, blockMulExp2Res.get(mulExpInBlock));
            } else {
                mulByConst(asmBlock, instr, f, bb);
            }
        } else if (instr.getPrev() instanceof MoveInstr move && move.getSrc() instanceof ConstInt moveValue) {
            //不大清楚前端的具体处理，但据观察这样应该是充分的
            AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
            int value = moveValue.getNumber();
            Group<AsmOperand, AsmOperand> mulExp = new Group<>(src1, new AsmImm32(value));
            Group<AsmBlock, Group<AsmOperand, AsmOperand>> mulExpInBlock = new Group<>(asmBlock, mulExp);
            if (blockMulExp2Res.containsKey(mulExpInBlock)) {
                operandMap.put(instr, blockMulExp2Res.get(mulExpInBlock));
            } else {
                mulByConst(asmBlock, instr, f, bb);
            }
        } else {
            AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
            AsmMul asmMul = new AsmMul(dst, src1, src2);
            asmMul.isWord = true;
            asmBlock.addInstrTail(asmMul);
        }
    }

    public void mulByConst(AsmBlock asmBlock, MulInstr instr, Function f, BasicBlock bb) {
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        int value = ((ConstInt) instr.getOp2()).getNumber();
        if (value == 1) {
            AsmMove asmMove = new AsmMove(dst, src1);
            asmBlock.addInstrTail(asmMove);
        } else if (value == 0) {
            AsmMove asmMove = new AsmMove(dst, ZERO);
            asmBlock.addInstrTail(asmMove);
        } else if (value == -1) {
            AsmSub asmSub = new AsmSub(dst, ZERO, src1);
            asmSub.isWord = true;
            asmBlock.addInstrTail(asmSub);
        } else if (isTwoTimes(Math.abs(value))) {
            int absValue = Math.abs(value);
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmSll asmSll = new AsmSll(dst, src1, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSll);
            if (value < 0) {
                AsmSub asmSub = new AsmSub(dst, ZERO, dst);
                asmSub.isWord = true;
                asmBlock.addInstrTail(asmSub);
            }
        } else if (isTwoTimes(Math.abs(value) + 1)) {
            int absValue = Math.abs(value) + 1;
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmOperand tmpReg = genTmpReg(f);
            AsmSll asmSll = new AsmSll(tmpReg, src1, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSll);
            AsmSub asmSub1 = new AsmSub(dst, tmpReg, src1);
            asmSub1.isWord = true;
            asmBlock.addInstrTail(asmSub1);
            if (value < 0) {
                AsmSub asmSub2 = new AsmSub(dst, ZERO, dst);
                asmSub2.isWord = true;
                asmBlock.addInstrTail(asmSub2);
            }
        } else {
            ArrayList<Integer> oneSet = new ArrayList<>();
            int onePos = 0;
            int absValue = Math.abs(value);
            while (absValue != 0) {
                if ((absValue & 1) == 1) {
                    oneSet.add(onePos);
                }
                onePos++;
                absValue >>= 1;
            }
            if (oneSet.size() == 2) {
                AsmOperand tmpReg = genTmpReg(f);
                for (int i = 0; i < 2; i++) {
                    int pos = oneSet.get(i);
                    if (i == 0) {
                        AsmSll asmSll = new AsmSll(tmpReg, src1, new AsmImm12(pos));
                        asmSll.isWord = true;
                        asmBlock.addInstrTail(asmSll);
                    }
                    if (i == 1) {
                        AsmSll asmSll = new AsmSll(dst, src1, new AsmImm12(pos));
                        asmSll.isWord = true;
                        asmBlock.addInstrTail(asmSll);
                        AsmAdd asmAdd = new AsmAdd(dst, dst, tmpReg);
                        asmAdd.isWord = true;
                        asmBlock.addInstrTail(asmAdd);
                    }
                }
                if (value < 0) {
                    AsmSub asmSub2 = new AsmSub(dst, ZERO, dst);
                    asmSub2.isWord = true;
                    asmBlock.addInstrTail(asmSub2);
                }
            } else {
                AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
                AsmMul asmMul = new AsmMul(dst, src1, src2);
                asmMul.isWord = true;
                asmBlock.addInstrTail(asmMul);
            }
        }
    }

    public boolean isTwoTimes(int value) {
        return (value & (value - 1)) == 0;
    }


    private void parseFAdd(FAddInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
        AsmFAdd asmFAdd = new AsmFAdd(dst, src1, src2);
        asmBlock.addInstrTail(asmFAdd);
    }

    private void parseFMul(FMulInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
        AsmFMul asmFMul = new AsmFMul(dst, src1, src2);
        asmBlock.addInstrTail(asmFMul);
    }

    private void parseSub(SubInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        Value src1 = instr.getOp1();
        Value src2 = instr.getOp2();
        boolean isConst1 = src1 instanceof ConstInt;
        boolean isConst2 = src2 instanceof ConstInt;
        if (isConst1 && isConst2) {
            int value1 = ((ConstInt) src1).getNumber();
            int value2 = ((ConstInt) src2).getNumber();
            AsmOperand imm = new AsmImm32(value1 - value2);
            AsmMove asmMove = new AsmMove(dst, imm);
            asmBlock.addInstrTail(asmMove);
        } else {
            AsmOperand left = parseOperand(src1, 0, f, bb);
            AsmOperand right = parseOperand(src2, 0, f, bb);
            AsmSub asmSub = new AsmSub(dst, left, right);
            asmSub.isWord = true;
            asmBlock.addInstrTail(asmSub);
        }
    }

    private void parseFSub(FSubInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
        AsmFSub asmFSub = new AsmFSub(dst, src1, src2);
        asmBlock.addInstrTail(asmFSub);
    }

    private void parseSDiv(SDivInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        if (instr.getOp2() instanceof ConstInt) {
            //SSA的存在使得记录块中的除法算式不会出错
            int value = ((ConstInt) instr.getOp2()).getNumber();
            Group<AsmOperand, AsmOperand> divExp = new Group<>(dst, new AsmImm32(value));
            Group<AsmBlock, Group<AsmOperand, AsmOperand>> divExpInBlock = new Group<>(asmBlock, divExp);
            if (blockDivExp2Res.containsKey(divExpInBlock)) {
                operandMap.put(instr, blockDivExp2Res.get(divExpInBlock));
            } else {
                divByConst(dst, src1, ((ConstInt) instr.getOp2()).getNumber(), bb, f);
            }
        } else if (instr.getPrev() instanceof MoveInstr move && move.getSrc() instanceof ConstInt moveValue) {
            int value = moveValue.getNumber();
            Group<AsmOperand, AsmOperand> divExp = new Group<>(dst, new AsmImm32(value));
            Group<AsmBlock, Group<AsmOperand, AsmOperand>> divExpInBlock = new Group<>(asmBlock, divExp);
            if (blockDivExp2Res.containsKey(divExpInBlock)) {
                operandMap.put(instr, blockDivExp2Res.get(divExpInBlock));
            } else {
                divByConst(dst, src1, ((ConstInt) instr.getOp2()).getNumber(), bb, f);
            }
        } else {
            AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
            AsmDiv asmDiv = new AsmDiv(dst, src1, src2);
            if (!instr.is64) {
                asmDiv.isWord = true;
            }
            asmBlock.addInstrTail(asmDiv);
        }
    }

    private void divByConst(AsmOperand dst, AsmOperand src1, int value, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        if (value == 1) {
            AsmMove asmMove = new AsmMove(dst, src1);
            asmBlock.addInstrTail(asmMove);
        } else if (value == -1) {
            AsmSub asmSub = new AsmSub(dst, ZERO, src1);
            asmBlock.addInstrTail(asmSub);
            return;
        } else if (isTwoTimes(Math.abs(value))) {
            int absValue = Math.abs(value);
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmSra asmSra = new AsmSra(dst, src1, new AsmImm12(31));
            asmSra.isWord = true;
            asmBlock.addInstrTail(asmSra);
            AsmSrl asmSrl = new AsmSrl(dst, dst, new AsmImm12(32 - shift));
            asmSrl.isWord = true;
            asmBlock.addInstrTail(asmSrl);
            AsmAdd asmAdd = new AsmAdd(dst, dst, src1);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
            AsmSra asmSra2 = new AsmSra(dst, dst, new AsmImm12(shift));
            asmSra2.isWord = true;
            asmBlock.addInstrTail(asmSra2);
        } else {
            int absValue = Math.abs(value);
            long nc = ((long) 1 << 31) - (((long) 1 << 31) % absValue) - 1;
            long p = 32;
            while (((long) 1 << p) <= nc * (absValue - ((long) 1 << p) % absValue)) {
                p++;
            }
            long m = ((((long) 1 << p) + (long) absValue - ((long) 1 << p) % absValue) / (long) absValue);
            int n = (int) ((m << 32) >>> 32);
            int shift = (int) (p - 32);
            AsmOperand tmp0 = genTmpReg(f);
            AsmMove asmMove = new AsmMove(tmp0, new AsmImm32(n));
            asmBlock.addInstrTail(asmMove);
            AsmOperand tmp1 = genTmpReg(f);
            AsmOperand tmp3 = genTmpReg(f);
            if (m >= 0x80000000L) {
                AsmMul asmMul = new AsmMul(tmp1, src1, tmp0);
                asmBlock.addInstrTail(asmMul);
                AsmSra asmMul2 = new AsmSra(tmp3, tmp1, new AsmImm12(32));
                asmBlock.addInstrTail(asmMul2);
                AsmAdd asmAdd = new AsmAdd(tmp3, tmp3, src1);
                asmBlock.addInstrTail(asmAdd);
            } else {
                AsmMul asmMul = new AsmMul(tmp1, src1, tmp0);
                asmBlock.addInstrTail(asmMul);
                AsmSra asmMul2 = new AsmSra(tmp3, tmp1, new AsmImm12(32));
                asmBlock.addInstrTail(asmMul2);
            }
            AsmOperand tmp2 = genTmpReg(f);
            AsmSra asmSra = new AsmSra(tmp2, tmp3, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSra);
            AsmOperand tmp4 = genTmpReg(f);
            AsmSra asmSra2 = new AsmSra(tmp4, tmp2, new AsmImm12(31));
            asmBlock.addInstrTail(asmSra2);
            AsmSub asmSub = new AsmSub(dst, tmp2, tmp4);
            asmBlock.addInstrTail(asmSub);
        }
        if (value < 0) {
            AsmSub asmSub = new AsmSub(dst, ZERO, dst);
            asmSub.isWord = true;
            asmBlock.addInstrTail(asmSub);
        }
        blockDivExp2Res.put(new Group<>(asmBlock, new Group<>(src1, new AsmImm32(value))), dst);
    }

    private void parseFDiv(FDivInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 0, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 0, f, bb);
        AsmFDiv asmFDiv = new AsmFDiv(dst, src1, src2);
        asmBlock.addInstrTail(asmFDiv);
    }

    private void parseMod(SRemInstr instr, BasicBlock bb, Function f) {
        //模的时候全部采用32位乘除减
        AsmBlock asmBlock = blockMap.get(bb);
        Value src1 = instr.getOp1();
        Value src2 = instr.getOp2();
        if (src2 instanceof ConstInt) {
            if (src1 instanceof ConstInt) {
                int value1 = ((ConstInt) src1).getNumber();
                int value2 = ((ConstInt) src2).getNumber();
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmOperand imm = new AsmImm32(value1 % value2);
                AsmMove asmMove = new AsmMove(dst, imm);
                asmBlock.addInstrTail(asmMove);
            } else if (((ConstInt) src2).getNumber() == 1) {
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmMove asmMove = new AsmMove(dst, ZERO);
                asmBlock.addInstrTail(asmMove);
            } else if (isTwoTimes(((ConstInt) src2).getNumber())) {
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmOperand src = parseOperand(src1, 0, f, bb);
                int value = ((ConstInt) src2).getNumber() - 1;
                AsmAnd asmAnd = new AsmAnd(dst, src, parseConstIntOperand(value, 12, f, bb));
                asmBlock.addInstrTail(asmAnd);
            } else if (((ConstInt) src2).getNumber() == (1 << 16) - 1) {
                //32位数的一半
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmOperand src = parseOperand(src1, 0, f, bb);
                int value = ((ConstInt) src2).getNumber();
                AsmOperand tmp = genTmpReg(f);
                AsmSrl asmSrl = new AsmSrl(tmp, src, new AsmImm12(16));
                asmBlock.addInstrTail(asmSrl);
                AsmAdd asmAdd = new AsmAdd(dst, src, tmp);
                asmBlock.addInstrTail(asmAdd);
                AsmAnd asmAnd2 = new AsmAnd(dst, dst, parseConstIntOperand(value, 12, f, bb));
                asmBlock.addInstrTail(asmAnd2);
            } else {
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmOperand src1Op = parseOperand(src1, 0, f, bb);
                AsmOperand src2Op = new AsmImm32(((ConstInt) src2).getNumber());
                getConstDiv(dst, src1Op, src2Op, bb, f);
                getConstMul(dst, dst, src2Op, bb, f);
                AsmSub asmSub = new AsmSub(dst, src1Op, dst);
                asmSub.isWord = true;
                asmBlock.addInstrTail(asmSub);
            }
        } else {
            if (src1 instanceof ConstInt constInt && constInt.getNumber() == 0) {
                AsmMove asmMove = new AsmMove(parseOperand(instr, 0, f, bb), ZERO);
                asmBlock.addInstrTail(asmMove);
            } else {
                AsmOperand dst = parseOperand(instr, 0, f, bb);
                AsmOperand src1Op = parseOperand(src1, 0, f, bb);
                AsmOperand src2Op = parseOperand(src2, 0, f, bb);
                AsmDiv asmDiv = new AsmDiv(dst, src1Op, src2Op);
                asmDiv.isWord = true;
                asmBlock.addInstrTail(asmDiv);
                AsmMul asmMul = new AsmMul(dst, dst, src2Op);
                asmMul.isWord = true;
                asmBlock.addInstrTail(asmMul);
                AsmSub asmSub = new AsmSub(dst, src1Op, dst);
                asmSub.isWord = true;
                asmBlock.addInstrTail(asmSub);
            }
        }
    }

    private void parseBr(BranchInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        Value cond = instr.getCond();
        BasicBlock trueBB = instr.getThenTarget();
        BasicBlock falseBB = instr.getElseTarget();
        if (cond instanceof ConstBool) {
            int value = ((ConstBool) cond).getNumber();
            if (value != 0) {
                AsmJ asmJ = new AsmJ(blockMap.get(trueBB));
                asmBlock.addInstrTail(asmJ);
                blockMap.get(trueBB).addJumpToInstr(asmJ);
            } else {
                AsmJ asmJ = new AsmJ(blockMap.get(falseBB));
                asmBlock.addInstrTail(asmJ);
                blockMap.get(falseBB).addJumpToInstr(asmJ);
            }
        } else {
            //TODO:为什么需要区分CmpInstr?
            AsmOperand cmp = parseOperand(cond, 0, f, bb);
            AsmBlock trueAsmBlock = blockMap.get(trueBB);
            AsmBlock falseAsmBlock = blockMap.get(falseBB);
            AsmBnez asmBnez = new AsmBnez(cmp, trueAsmBlock);
            asmBlock.addInstrTail(asmBnez);
            if (trueAsmBlock == null) {
                throw new RuntimeException(instr.print() + " " + instr.getParentBB());
            }
            trueAsmBlock.addJumpToInstr(asmBnez);
            AsmJ asmJ = new AsmJ(falseAsmBlock);
            asmBlock.addInstrTail(asmJ);
            falseAsmBlock.addJumpToInstr(asmJ);
            //TODO:setTrueBlock和setFalseBlock?
        }
    }

    private void parseJump(JumpInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        BasicBlock target = instr.getTarget();
        AsmJ asmJ = new AsmJ(blockMap.get(target));
        asmBlock.addInstrTail(asmJ);
        if (blockMap.get(target) == null) {
            throw new RuntimeException(instr.print() + " " + instr.getParentBB());
        }
        blockMap.get(target).addJumpToInstr(asmJ);
        //TODO:setTrueBlock和setFalseBlock?
    }

    private void parseCall(CallInstr instr, BasicBlock bb, Function f) {
        AsmFunction asmFunction = funcMap.get(f);
        AsmBlock asmBlock = blockMap.get(bb);
        List<Value> args = instr.getRParams();
        List<Value> floatArgs = new ArrayList<>();
        List<Value> intArgs = new ArrayList<>();
        for (Value arg : args) {
            if (arg.getPointerLevel() != 0) {
                intArgs.add(arg);
            } else if (arg.getDataType() == FLOAT) {
                floatArgs.add(arg);
            } else {
                intArgs.add(arg);
            }
        }
        int intArgNum = intArgs.size();
        int floatArgNum = floatArgs.size();
        int intArgRegNum = Math.min(intArgNum, 8);
        int floatArgRegNum = Math.min(floatArgNum, 8);
        for (int i = 0; i < intArgRegNum; i++) {
            AsmOperand argReg = parseOperand(intArgs.get(i), 12, f, bb);
            AsmMove asmMove = new AsmMove(RegGeter.AregsInt.get(i), argReg);
            asmBlock.addInstrTail(asmMove);
        }
        for (int i = 0; i < floatArgRegNum; i++) {
            AsmOperand argReg = parseOperand(floatArgs.get(i), 12, f, bb);
            AsmMove asmMove = new AsmMove(RegGeter.AregsFloat.get(i), argReg);
            asmBlock.addInstrTail(asmMove);
        }
        if (instr.getFuncDef().getName().equals(f.getName()) && f.isTailRecursive()) {
            int offset = asmFunction.getWholeSize();
            for (int i = 8; i < intArgs.size(); i++) {
                Value arg = intArgs.get(i);
                AsmOperand argReg = parseOperand(arg, 0, f, bb);
                if (arg.getPointerLevel() == 0) {
                    AsmSw asmSw = new AsmSw(argReg, RegGeter.SP, new AsmImm32(offset), 1);
                    asmBlock.addInstrTail(asmSw);
                    offset += 4;
                } else {
                    AsmSd asmSd = new AsmSd(argReg, RegGeter.SP, new AsmImm32(offset), 1);
                    asmBlock.addInstrTail(asmSd);
                    offset += 8;
                }
            }
            for (int i = 8; i < floatArgs.size(); i++) {
                AsmOperand argReg = parseOperand(floatArgs.get(i), 12, f, bb);
                if (argReg instanceof AsmImm12) {
                    AsmOperand tmpReg = genFloatTmpReg(f);
                    AsmMove asmMove = new AsmMove(tmpReg, argReg);
                    asmBlock.addInstrTail(asmMove);
                    argReg = tmpReg;
                }
                AsmFsw asmFsw = new AsmFsw(argReg, RegGeter.SP, new AsmImm32(offset), 1);
                asmBlock.addInstrTail(asmFsw);
                offset += 4;
            }
        } else {
            int offset = 0;
            for (int i = 8; i < intArgs.size(); i++) {
                Value arg = intArgs.get(i);
                AsmOperand argReg = parseOperand(arg, 0, f, bb);
                if (arg.getPointerLevel() == 0) {
                    if (offset >= -2048 && offset <= 2047) {
                        AsmSw asmSw = new AsmSw(argReg, RegGeter.SP, new AsmImm12(offset));
                        asmBlock.addInstrTail(asmSw);
                    } else {
                        AsmOperand tmpReg = genTmpReg(f);
                        AsmAdd asmAdd = new AsmAdd(tmpReg, RegGeter.SP, parseConstIntOperand(offset, 12, f, bb));
                        asmBlock.addInstrTail(asmAdd);
                        AsmSw asmSw = new AsmSw(argReg, tmpReg, new AsmImm12(0));
                        asmBlock.addInstrTail(asmSw);
                    }
                    offset += 4;
                } else {
                    if (offset >= -2048 && offset <= 2047) {
                        AsmSd asmSd = new AsmSd(argReg, RegGeter.SP, new AsmImm12(offset));
                        asmBlock.addInstrTail(asmSd);
                    } else {
                        AsmOperand tmpReg = genTmpReg(f);
                        AsmAdd asmAdd = new AsmAdd(tmpReg, RegGeter.SP, parseConstIntOperand(offset, 12, f, bb));
                        asmBlock.addInstrTail(asmAdd);
                        AsmSd asmSd = new AsmSd(argReg, tmpReg, new AsmImm12(0));
                        asmBlock.addInstrTail(asmSd);
                    }
                    offset += 8;
                }
            }
            for (int i = 8; i < floatArgs.size(); i++) {
                AsmOperand argReg = parseOperand(floatArgs.get(i), 12, f, bb);
                if (argReg instanceof AsmImm12) {
                    AsmOperand tmpReg = genFloatTmpReg(f);
                    AsmMove asmMove = new AsmMove(tmpReg, argReg);
                    asmBlock.addInstrTail(asmMove);
                    argReg = tmpReg;
                }
                AsmFsw asmFsw = new AsmFsw(argReg, RegGeter.SP, new AsmImm12(offset));
                asmBlock.addInstrTail(asmFsw);
                offset += 4;
            }
        }
        AsmCall asmCall = new AsmCall(instr.getFuncDef().getName(), intArgRegNum, floatArgRegNum, instr.callsLibFunc());
        asmBlock.addInstrTail(asmCall);
        if (instr.getDataType() != VOID) {
            AsmOperand dst = parseOperand(instr, 0, f, bb);
            if (instr.getDataType() == INT) {
                AsmMove asmMove = new AsmMove(dst, RegGeter.AregsInt.get(0));
                asmBlock.addInstrTail(asmMove);
            } else {
                AsmMove asmMove = new AsmMove(dst, RegGeter.AregsFloat.get(0));
                asmBlock.addInstrTail(asmMove);
            }
        } else {
            AsmMove asmMove = new AsmMove(ZERO, ZERO);
            asmBlock.addInstrTail(asmMove);
        }
    }


    private void parseMove(MoveInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr.getDst(), 0, f, bb);
        AsmOperand src = parseOperand(instr.getSrc(), 32, f, bb);
        AsmMove asmMove = new AsmMove(dst, src);
        asmBlock.addInstrTail(asmMove);
    }

    private void parseConv(ConversionOperation instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src = parseOperand(instr.getValue(), 0, f, bb);
        if (instr.getOpName().equals("fptosi")) {
            AsmFtoi asmFtoi = new AsmFtoi(dst, src);
            asmBlock.addInstrTail(asmFtoi);
        } else if (instr.getOpName().equals("sitofp")) {
            AsmitoF asmitoF = new AsmitoF(dst, src);
            asmBlock.addInstrTail(asmitoF);
        } else {
            AsmMove asmMove = new AsmMove(dst, src);
            asmBlock.addInstrTail(asmMove);
        }
    }

    private void parseGEP(GEPInstr instr, BasicBlock bb, Function f) {
        AsmFunction asmFunction = funcMap.get(f);
        AsmBlock asmBlock = blockMap.get(bb);
        List<Value> indexList = instr.getWholeIndexList();
        AsmOperand base;
        if (instr.getPtrVal() instanceof GlobalObject) {
            base = parseGlobalToOperand(instr.getSymbol(), bb);
        } else {
            base = parseOperand(instr.getPtrVal(), 0, f, bb);
        }
        AsmOperand result = parseOperand(instr, 0, f, bb);
        AsmMove asmMove = new AsmMove(result, base);
        asmBlock.addInstrTail(asmMove);
        List<Integer> sizeList = instr.getSizeList();
        if (indexList.size() == 0) {
            throw new RuntimeException("sizeList is empty");
        }
        int offset = 0;
        boolean constFlag = true;
        for (int i = 0; i < sizeList.size(); i++) {
            if (!(indexList.get(i) instanceof ConstInt)) {
                constFlag = false;
                break;
            }
        }
        if (constFlag) {
            for (int i = 0; i < sizeList.size(); i++) {
                int index = ((ConstInt) indexList.get(i)).getNumber();
                offset += index * sizeList.get(i) * 4;
            }
            if (offset != 0) {
                AsmAdd asmAdd = new AsmAdd(result, result, parseConstIntOperand(offset, 12, f, bb));
                asmBlock.addInstrTail(asmAdd);
            }
            return;
        }
        for (int i = 0; i < sizeList.size(); i++) {
            if (indexList.get(i) instanceof ConstInt) {
                int index = ((ConstInt) indexList.get(i)).getNumber();
                offset = index * sizeList.get(i) * 4;
                if (offset != 0) {
                    AsmAdd asmAdd = new AsmAdd(result, result, parseConstIntOperand(offset, 12, f, bb));
                    asmBlock.addInstrTail(asmAdd);
                }
            } else {
                AsmOperand index = parseOperand(indexList.get(i), 0, f, bb);
                AsmOperand tmp = genTmpReg(f);
                getConstMul(tmp, index, new AsmImm32(sizeList.get(i) * 4), bb, f);
                AsmAdd asmAdd = new AsmAdd(result, result, tmp);
                asmBlock.addInstrTail(asmAdd);
            }
        }
    }

    private void getConstMul(AsmOperand dst, AsmOperand src1, AsmOperand src2, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        int value = ((AsmImm32) src2).getValue();
        if (value == 1) {
            AsmMove asmMove = new AsmMove(dst, src1);
            asmBlock.addInstrTail(asmMove);
        } else if (value == 0) {
            AsmMove asmMove = new AsmMove(dst, ZERO);
            asmBlock.addInstrTail(asmMove);
        } else if (value == -1) {
            AsmSub asmSub = new AsmSub(dst, ZERO, src1);
            asmSub.isWord = true;
            asmBlock.addInstrTail(asmSub);
        } else if (isTwoTimes(Math.abs(value))) {
            int absValue = Math.abs(value);
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmSll asmSll = new AsmSll(dst, src1, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSll);
            if (value < 0) {
                AsmSub asmSub = new AsmSub(dst, ZERO, dst);
                asmSub.isWord = true;
                asmBlock.addInstrTail(asmSub);
            }
        } else if (isTwoTimes(Math.abs(value) + 1)) {
            int absValue = Math.abs(value) + 1;
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmOperand tmpReg = genTmpReg(f);
            AsmSll asmSll = new AsmSll(tmpReg, src1, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSll);
            AsmSub asmSub1 = new AsmSub(dst, tmpReg, src1);
            asmSub1.isWord = true;
            asmBlock.addInstrTail(asmSub1);
            if (value < 0) {
                AsmSub asmSub2 = new AsmSub(dst, ZERO, dst);
                asmSub2.isWord = true;
                asmBlock.addInstrTail(asmSub2);
            }
        } else {
            ArrayList<Integer> oneSet = new ArrayList<>();
            int onePos = 0;
            int absValue = Math.abs(value);
            while (absValue != 0) {
                if ((absValue & 1) == 1) {
                    oneSet.add(onePos);
                }
                onePos++;
                absValue >>= 1;
            }
            if (oneSet.size() == 2) {
                AsmOperand tmpReg = genTmpReg(f);
                for (int i = 0; i < 2; i++) {
                    int pos = oneSet.get(i);
                    if (i == 0) {
                        AsmSll asmSll = new AsmSll(tmpReg, src1, new AsmImm12(pos));
                        asmSll.isWord = true;
                        asmBlock.addInstrTail(asmSll);
                    }
                    if (i == 1) {
                        AsmSll asmSll = new AsmSll(dst, src1, new AsmImm12(pos));
                        asmSll.isWord = true;
                        asmBlock.addInstrTail(asmSll);
                        AsmAdd asmAdd = new AsmAdd(dst, dst, tmpReg);
                        asmAdd.isWord = true;
                        asmBlock.addInstrTail(asmAdd);
                    }
                }
                if (value < 0) {
                    AsmSub asmSub2 = new AsmSub(dst, ZERO, dst);
                    asmSub2.isWord = true;
                    asmBlock.addInstrTail(asmSub2);
                }
            } else {
                AsmMul asmMul = new AsmMul(dst, src1, parseConstIntOperand(value, 0, f, bb));
                asmMul.isWord = true;
                asmBlock.addInstrTail(asmMul);
            }
        }
    }

    public void getConstDiv(AsmOperand dst, AsmOperand src1, AsmOperand src2, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        int value = ((AsmImm32) src2).getValue();
        if (value == 1) {
            AsmMove asmMove = new AsmMove(dst, src1);
            asmBlock.addInstrTail(asmMove);
        } else if (value == -1) {
            AsmSub asmSub = new AsmSub(dst, ZERO, src1);
            asmBlock.addInstrTail(asmSub);
            return;
        } else if (isTwoTimes(Math.abs(value))) {
            int absValue = Math.abs(value);
            int shift = -1;
            while (absValue != 0) {
                absValue >>= 1;
                shift++;
            }
            AsmSra asmSra = new AsmSra(dst, src1, new AsmImm12(31));
            asmSra.isWord = true;
            asmBlock.addInstrTail(asmSra);
            AsmSrl asmSrl = new AsmSrl(dst, dst, new AsmImm12(32 - shift));
            asmSrl.isWord = true;
            asmBlock.addInstrTail(asmSrl);
            AsmAdd asmAdd = new AsmAdd(dst, dst, src1);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
            AsmSra asmSra2 = new AsmSra(dst, dst, new AsmImm12(shift));
            asmSra2.isWord = true;
            asmBlock.addInstrTail(asmSra2);
        } else {
            int absValue = Math.abs(value);
            long nc = ((long) 1 << 31) - (((long) 1 << 31) % absValue) - 1;
            long p = 32;
            while (((long) 1 << p) <= nc * (absValue - ((long) 1 << p) % absValue)) {
                p++;
            }
            long m = ((((long) 1 << p) + (long) absValue - ((long) 1 << p) % absValue) / (long) absValue);
            int n = (int) ((m << 32) >>> 32);
            int shift = (int) (p - 32);
            AsmOperand tmp0 = genTmpReg(f);
            AsmMove asmMove = new AsmMove(tmp0, new AsmImm32(n));
            asmBlock.addInstrTail(asmMove);
            AsmOperand tmp1 = genTmpReg(f);
            AsmOperand tmp3 = genTmpReg(f);
            if (m >= 0x80000000L) {
                AsmMul asmMul = new AsmMul(tmp1, src1, tmp0);
                asmBlock.addInstrTail(asmMul);
                AsmSra asmMul2 = new AsmSra(tmp3, tmp1, new AsmImm12(32));
                asmBlock.addInstrTail(asmMul2);
                AsmAdd asmAdd = new AsmAdd(tmp3, tmp3, src1);
                asmBlock.addInstrTail(asmAdd);
            } else {
                AsmMul asmMul = new AsmMul(tmp1, src1, tmp0);
                asmBlock.addInstrTail(asmMul);
                AsmSra asmMul2 = new AsmSra(tmp3, tmp1, new AsmImm12(32));
                asmBlock.addInstrTail(asmMul2);
            }
            AsmOperand tmp2 = genTmpReg(f);
            AsmSra asmSra = new AsmSra(tmp2, tmp3, new AsmImm12(shift));
            asmBlock.addInstrTail(asmSra);
            AsmOperand tmp4 = genTmpReg(f);
            AsmSra asmSra2 = new AsmSra(tmp4, tmp2, new AsmImm12(31));
            asmBlock.addInstrTail(asmSra2);
            AsmSub asmSub = new AsmSub(dst, tmp2, tmp4);
            asmBlock.addInstrTail(asmSub);
        }
        if (value < 0) {
            AsmSub asmSub = new AsmSub(dst, ZERO, dst);
            asmSub.isWord = true;
            asmBlock.addInstrTail(asmSub);
        }
    }

    private AsmOperand parseOperand(Value irValue, int maxImm, Function irFunction, BasicBlock bb) {
        if (operandMap.containsKey(irValue)) {
            AsmOperand asmOperand = operandMap.get(irValue);
            if (((asmOperand instanceof AsmImm32) && maxImm < 32) ||
                    ((asmOperand instanceof AsmImm12) && maxImm < 12)) {
                if ((asmOperand instanceof AsmImm32) && (((AsmImm32) asmOperand).getValue() == 0)) {
                    return ZERO;
                }
                if ((asmOperand instanceof AsmImm12) && (((AsmImm12) asmOperand).getValue() == 0)) {
                    return ZERO;
                }
                AsmOperand tmpReg = genTmpReg(irFunction);
                AsmMove asmMove = new AsmMove(tmpReg, asmOperand);
                blockMap.get(bb).addInstrTail(asmMove);
                return tmpReg;
            }
            return asmOperand;
        }

        //TODO:关于move指令的到时候再写吧

        if (irValue instanceof ConstInt) {
            return parseConstIntOperand(((ConstInt) irValue).getNumber(), maxImm, irFunction, bb);
        }
        if (irValue instanceof ConstFloat) {
            return parseConstFloatOperand(((ConstFloat) irValue).getNumber(), maxImm, irFunction, bb);
        }
        AsmFunction asmFunction = funcMap.get(irFunction);
        if (irValue.getPointerLevel() != 0) {
            AsmVirReg tmpReg = genTmpReg(irFunction);
            operandMap.put(irValue, tmpReg);
            downOperandMap.put(tmpReg, irValue);
            return tmpReg;
        }
        if (irValue.getDataType() == FLOAT) {
            AsmFVirReg tmpReg = new AsmFVirReg();
            //TODO:这里是addUsedVirReg还是addUsedFVirReg
            asmFunction.addUsedVirReg(tmpReg);
            if (!(irValue instanceof ConstFloat)) {
                operandMap.put(irValue, tmpReg);
                downOperandMap.put(tmpReg, irValue);
            }
            return tmpReg;
        }
        AsmVirReg tmpReg = genTmpReg(irFunction);
        if (!(irValue instanceof ConstInt)) {
            operandMap.put(irValue, tmpReg);
            downOperandMap.put(tmpReg, irValue);
        }
        return tmpReg;
    }

    private AsmOperand parseConstIntOperand(int value, int maxImm, Function irFunction, BasicBlock bb) {
        AsmImm32 asmImm32 = new AsmImm32(value);
        AsmImm12 asmImm12 = new AsmImm12(value);
        if (maxImm == 32) {
            return asmImm32;
        }
        if (maxImm == 12 && (value >= -2048 && value <= 2047)) {
            return asmImm12;
        }
        AsmBlock asmBlock = blockMap.get(bb);
        AsmVirReg tmpReg = genTmpReg(irFunction);
        AsmMove asmMove = new AsmMove(tmpReg, asmImm32);
        asmBlock.addInstrTail(asmMove);
        return tmpReg;
    }

    private AsmVirReg genTmpReg(Function irFunction) {
        AsmFunction asmFunction = funcMap.get(irFunction);
        AsmVirReg tmpReg = new AsmVirReg();
        asmFunction.addUsedVirReg(tmpReg);
        return tmpReg;
    }

    private AsmFVirReg genFloatTmpReg(Function irFunction) {
        AsmFunction asmFunction = funcMap.get(irFunction);
        AsmFVirReg tmpReg = new AsmFVirReg();
        //TODO:这里是addUsedVirReg还是addUsedFVirReg
        asmFunction.addUsedVirReg(tmpReg);
        return tmpReg;
    }

    private AsmOperand parseConstFloatOperand(float value, int maxImm, Function irFunction, BasicBlock bb) {
        int intVal = Float.floatToRawIntBits(value);
        AsmFVirReg tmpReg = genFloatTmpReg(irFunction);
        AsmOperand label = getFloatLabel(intVal);
        AsmOperand tmpIntReg = genTmpReg(irFunction);
        //TODO:全局变量怎么加载局部地址？
        AsmLLa asmLLa = new AsmLLa(tmpIntReg, label);
        blockMap.get(bb).addInstrTail(asmLLa);
        AsmFlw asmFlw = new AsmFlw(tmpReg, tmpIntReg, new AsmImm12(0));
        blockMap.get(bb).addInstrTail(asmFlw);
        return tmpReg;
    }

    private AsmOperand getFloatLabel(int value) {
        if (floatLabelMap.containsKey(value)) {
            return floatLabelMap.get(value);
        }
        AsmGlobalVar asmGlobalVar = new AsmGlobalVar(value);
        asmModule.addGlobalVar(asmGlobalVar);
        AsmLabel asmLabel = new AsmLabel(asmGlobalVar.name);
        floatLabelMap.put(value, asmLabel);
        return asmLabel;
    }

    private AsmOperand parseGlobalToOperand(Symbol symbol, BasicBlock bb) {
        AsmGlobalVar asmGlobalVar = gvMap.get(symbol);
        AsmLabel asmLabel = new AsmLabel(asmGlobalVar.name);
        return asmLabel;
    }


//    private void parseAnd( instr, BasicBlock bb, Function f) {
//        AsmBlock asmBlock = blockMap.get(bb);
//        AsmOperand dst = parseOperand(instr, 0, f, bb);
//        AsmOperand src1 = parseOperand(instr.getOp1(), 12, f, bb);
//        AsmOperand src2 = parseOperand(instr.getOp2(), 12, f, bb);
//        if ((src1 instanceof AsmImm12) && (src2 instanceof AsmImm12)) {
//            int value1 = ((AsmImm12) src1).getValue();
//            int value2 = ((AsmImm12) src2).getValue();
//            AsmOperand imm = new AsmImm32(value1 & value2);
//            AsmMove asmMove = new AsmMove(dst, imm);
//            asmBlock.addInstrTail(asmMove);
//        } else if (src1 instanceof AsmImm12) {
//            AsmAnd asmAnd = new AsmAnd(dst, src2, src1);
//            asmBlock.addInstrTail(asmAnd);
//        } else {
//            AsmAnd asmAnd = new AsmAnd(dst, src1, src2);
//            asmBlock.addInstrTail(asmAnd);
//        }
//    }

//    private void parseOr(AddInstr instr, BasicBlock bb, Function f) {
//        AsmBlock asmBlock = blockMap.get(bb);
//        AsmOperand dst = parseOperand(instr, 0, f, bb);
//        AsmOperand src1 = parseOperand(instr.getOp1(), 12, f, bb);
//        AsmOperand src2 = parseOperand(instr.getOp2(), 12, f, bb);
//        if ((src1 instanceof AsmImm12) && (src2 instanceof AsmImm12)) {
//            int value1 = ((AsmImm12) src1).getValue();
//            int value2 = ((AsmImm12) src2).getValue();
//            AsmOperand imm = new AsmImm32(value1 | value2);
//            AsmMove asmMove = new AsmMove(dst, imm);
//            asmBlock.addInstrTail(asmMove);
//        } else if (src1 instanceof AsmImm12) {
//            AsmOr asmOr = new AsmOr(dst, src2, src1);
//            asmBlock.addInstrTail(asmOr);
//        } else {
//            AsmOr asmOr = new AsmOr(dst, src1, src2);
//            asmBlock.addInstrTail(asmOr);
//        }
//    }

//    private void parseXor(AddInstr instr, BasicBlock bb, Function f) {
//        AsmBlock asmBlock = blockMap.get(bb);
//        AsmOperand dst = parseOperand(instr, 0, f, bb);
//        AsmOperand src1 = parseOperand(instr.getOp1(), 12, f, bb);
//        AsmOperand src2 = parseOperand(instr.getOp2(), 12, f, bb);
//        if ((src1 instanceof AsmImm12) && (src2 instanceof AsmImm12)) {
//            int value1 = ((AsmImm12) src1).getValue();
//            int value2 = ((AsmImm12) src2).getValue();
//            AsmOperand imm = new AsmImm32(value1 ^ value2);
//            AsmMove asmMove = new AsmMove(dst, imm);
//            asmBlock.addInstrTail(asmMove);
//        } else if (src1 instanceof AsmImm12) {
//            AsmXor asmXor = new AsmXor(dst, src2, src1);
//            asmBlock.addInstrTail(asmXor);
//        } else {
//            AsmXor asmXor = new AsmXor(dst, src1, src2);
//            asmBlock.addInstrTail(asmXor);
//        }
//    }

    private void parseShl(ShlInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 12, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 12, f, bb);
        if ((src1 instanceof AsmImm12) && (src2 instanceof AsmImm12)) {
            int value1 = ((AsmImm12) src1).getValue();
            int value2 = ((AsmImm12) src2).getValue();
            AsmOperand imm = new AsmImm32(value1 << value2);
            AsmMove asmMove = new AsmMove(dst, imm);
            asmBlock.addInstrTail(asmMove);
        } else if (src1 instanceof AsmImm12) {
            AsmOperand tmpReg = genTmpReg(f);
            AsmAdd asmAdd = new AsmAdd(tmpReg, ZERO, src1);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
            AsmSll asmSll = new AsmSll(dst, tmpReg, src2);
            asmSll.isWord = true;
            asmBlock.addInstrTail(asmSll);
        } else {
            AsmSll asmSll = new AsmSll(dst, src1, src2);
            asmSll.isWord = true;
            asmBlock.addInstrTail(asmSll);
        }
    }

    private void parseShr(AShrInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src1 = parseOperand(instr.getOp1(), 12, f, bb);
        AsmOperand src2 = parseOperand(instr.getOp2(), 12, f, bb);
        if ((src1 instanceof AsmImm12) && (src2 instanceof AsmImm12)) {
            int value1 = ((AsmImm12) src1).getValue();
            int value2 = ((AsmImm12) src2).getValue();
            AsmOperand imm = new AsmImm32(value1 >> value2);
            AsmAdd asmAdd = new AsmAdd(dst, ZERO, imm);
            asmBlock.addInstrTail(asmAdd);
        } else if (src1 instanceof AsmImm12) {
            AsmOperand tmpReg = genTmpReg(f);
            AsmAdd asmAdd = new AsmAdd(tmpReg, ZERO, src1);
            asmAdd.isWord = true;
            asmBlock.addInstrTail(asmAdd);
            AsmSra asmSra = new AsmSra(dst, tmpReg, src2);
            asmSra.isWord = true;
            asmBlock.addInstrTail(asmSra);
        } else {
            AsmSra asmSra = new AsmSra(dst, src1, src2);
            asmSra.isWord = true;
            asmBlock.addInstrTail(asmSra);
        }
    }

    private void parseFNeg(FNegInstr instr, BasicBlock bb, Function f) {
        AsmBlock asmBlock = blockMap.get(bb);
        AsmOperand dst = parseOperand(instr, 0, f, bb);
        AsmOperand src = parseOperand(instr.getValue(), 0, f, bb);
        AsmFneg asmFneg = new AsmFneg(dst, src);
        asmBlock.addInstrTail(asmFneg);
    }
}
