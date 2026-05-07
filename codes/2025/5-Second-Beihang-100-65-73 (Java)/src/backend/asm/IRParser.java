package backend.asm;

import backend.asm.immediate.ASMImmediate;
import backend.asm.immediate.ASMStackOffset;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirFReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.*;
import backend.opt.ASMMulPlanner;
import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.convop.ConversionOperation;
import frontend.ir.instr.convop.Fp2Si;
import frontend.ir.instr.convop.Si2Fp;
import frontend.ir.instr.memop.*;
import frontend.ir.instr.otherop.*;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.instr.unaryop.FNegInstr;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.symbol.ArrayInitVal;
import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.StringLiteral;
import frontend.ir.symbol.Symbol;
import midend.range.Range;
import midend.range.RangeAnalysisV3;
import midend.pseudoinstr.*;

import java.math.BigInteger;
import java.util.*;

public abstract class IRParser {
    protected RegStore regStore;
    private final Map<Value, ASMGlobalObject> glbObjMap = new LinkedHashMap<>();
    private final List<ASMGlobalObject> glbObjList = new ArrayList<>();
    private final Map<Integer, ASMGlobalObject> floatLabelMap = new LinkedHashMap<>(); // 用于存储浮点数的 .LC 标签
    private final Map<Function, ASMFunction> functionMap = new LinkedHashMap<>();
    protected final Map<BasicBlock, ASMBasicBlock> basicBlockMap = new LinkedHashMap<>();
    private final Map<Value, ASMValue> valueMap = new LinkedHashMap<>();
    private final Map<AllocaInstr, Integer> alloca2offset = new LinkedHashMap<>();   // alloca 指令申请的内存对应到当前函数 sp 指针位置的偏移量
    private ASMBasicBlock fillingBlock = null;
    private final Map<ASMBasicBlock, ArrayList<GEPInstr>> block2GEPs = new LinkedHashMap<>();

    public ASMModel parseIR(IRModel irModel, RegStore regStore) {
        this.regStore = regStore;

        parseGlobalValues(irModel);
        List<ASMFunction> asmFunctions = new ArrayList<>();

        initMaps(irModel.getFunctions());

        for (Function function : irModel.getFunctions()) {
            asmFunctions.add(parseFunction(function));
        }
        takeMainAhead(asmFunctions);
        return new ASMModel(glbObjList, asmFunctions, irModel.hasParallel());
    }

    private void takeMainAhead(List<ASMFunction> asmFunctions) {
        ASMFunction mainFunc = asmFunctions.getLast();
        if (!mainFunc.isMain()) {
            throw new RuntimeException("最后一个函数一定是 main");
        }
        asmFunctions.removeLast();
        asmFunctions.addFirst(mainFunc);
    }

    /**
     * 创建好所有汇编函数和基本块，并与 IR 进行对应，为后续翻译跳转指令做好准备
     */
    private void initMaps(List<Function> functions) {
        for (Function function : functions) {
            ASMFunction asmFunc = new ASMFunction(function.getName(), function.isTailRecursive(), function.isParallelLoopBody());
            functionMap.put(function, asmFunc);
            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                basicBlockMap.put(basicBlock, new ASMBasicBlock(asmFunc, basicBlock.getLoopDepth(), basicBlock.value2string()));
            }
        }
    }

    private ASMFunction parseFunction(Function function) {
        ASMFunction asmFunction = functionMap.get(function);

        asmFunction.setReservedParamSize(getMaxParamSize(function));  // 该函数调用的函数中，参数最多的那个需要多少栈空间

        ASMBasicBlock bstartBlock = asmFunction.createBasicBlockAtHead();

        genPrologue(asmFunction, bstartBlock);

        popRParams(function, (ASMBasicBlock) bstartBlock.getNext());

        for (BasicBlock basicBlock : function.getBasicBlockList()) {
            fillingBlock = basicBlockMap.get(basicBlock);
            List<Instruction> instrList = basicBlock.getInstrList();
            for (int i = 0; i < instrList.size(); i++) {
                Instruction ins = instrList.get(i);
                if (i < instrList.size() - 1 && isIntBranch(ins, instrList.get(i + 1))) {
                    parseIntBranch((CmpInstr) ins, (BranchInstr) instrList.get(i + 1), fillingBlock);
                    i++;
                } else {
                    parseInstruction(ins, fillingBlock, basicBlock);
                }
            }
        }

        asmFunction.initAncientLoopInfo(function.getAncientLoopInfo(), basicBlockMap);

        return asmFunction;
    }

    /**
     * 用来判断连续两条指令的组合是不是根据整数之间关系判断跳转的独立指令组合
     * （其实就是 ICMP + Branch 并且 ICMP 的结果只被该 Branch 用到）
     */
    private boolean isIntBranch(Instruction ins1, Instruction ins2) {
        if (!(ins1 instanceof CmpInstr cmpInstr && ins2 instanceof BranchInstr)) {
            return false;
        }

        Value op1 = cmpInstr.getOperand1();
        Value op2 = cmpInstr.getOperand2();
        if (!(op1.getType() instanceof TInt && op2.getType() instanceof TInt)) {
            return false;
        }

        return ins1.getUserList().size() == 1 && ins1.getUserList().getFirst() == ins2;
    }

    private void parseInstruction(Instruction ins, ASMBasicBlock newBlock, BasicBlock basicBlock) {
        switch (ins) {
            case BinaryOperation binaryOperation -> parseBinaryOperation(binaryOperation, newBlock, basicBlock);
            case ConversionOperation conversionOperation -> parseConversionOperation(conversionOperation, newBlock);
            case MemoryOperation memoryOperation -> parseMemoryOperation(memoryOperation, newBlock);
            case Terminator terminator -> parseTerminator(terminator, newBlock);
            case CmpInstr cmpInstr -> parseCmpInstr(cmpInstr, newBlock);
            case CallInstr callInstr -> parseCallInstr(callInstr, newBlock);
            case MoveInstr moveInstr -> parseMoveInstr(moveInstr, newBlock);
            case FNegInstr fNegInstr -> parseFNegInstr(fNegInstr, newBlock);
            case AtomaticAddInstr atomaticAddInstr -> parseAtomaticAddInstr(atomaticAddInstr, newBlock);
            case VecMoveFromScaInstr vecMoveFromScaInstr -> parseVecMoveFromScaInstr(vecMoveFromScaInstr, newBlock);
            case VecDuplicateInstr vecDuplicateInstr -> parseVecDuplicateInstr(vecDuplicateInstr, newBlock);
            case VecMulAddInstr vecMulAddInstr -> parseVecMulAddInstr(vecMulAddInstr, newBlock);
            case VecAddInstr vecAddInstr -> parseVecAddInstr(vecAddInstr, newBlock);
            case VecStoreInstr vecStore -> parseVecStore(vecStore, newBlock);
            case VecLoadInstr vecLoad -> parseVecLoad(vecLoad, newBlock);
            case null, default -> throw new RuntimeException("出现了未知的指令类型：" + ins);
        }
    }

    private void parseVecAddInstr(VecAddInstr ins, ASMBasicBlock newBlock) {
        VirFReg op1 = (VirFReg) getFValue(ins.getOp1(), newBlock, true);
        VirFReg op2 = (VirFReg) getFValue(ins.getOp2(), newBlock, true);
        genVecAdd(op1, op2, newBlock, (VirFReg) getFValue(ins, newBlock, true));
    }

    private void parseVecMulAddInstr(VecMulAddInstr ins, ASMBasicBlock newBlock) {
        VirFReg op1 = (VirFReg) getFValue(ins.getOp1(), newBlock, true);
        VirFReg op2 = (VirFReg) getFValue(ins.getOp2(), newBlock, true);
        VirFReg valueToAdd = (VirFReg) getFValue(ins.getValToAdd(), newBlock, true);
        renewValueMap(ins, valueToAdd);
        genVecMulAdd(op1, op2, newBlock, (VirFReg) getFValue(ins, newBlock, true));
    }

    private void parseVecDuplicateInstr(VecDuplicateInstr ins, ASMBasicBlock newBlock) {
        Value scaVal = ins.getScalarValue();
        VirReg scaReg;
        switch (scaVal.getType()) {
            case TInt _ -> scaReg = (VirIReg) getValue(scaVal);
            case TFloat _ -> scaReg = (VirFReg) getFValue(scaVal, newBlock, false);
            default -> throw new IllegalStateException("Unexpected value: " + scaVal.getType());
        }
        genVecDup(scaReg, newBlock, (VirFReg) getFValue(ins, newBlock, true));
    }

    private void parseVecLoad(VecLoadInstr ins, ASMBasicBlock newBlock) {
        Value ptr = ins.getBasicPointer();
        VirFReg virFReg = (VirFReg) getFValue(ins, newBlock, true);
        genVecLoad((Reg) getPtrVal(ptr, newBlock), newBlock, virFReg);
    }

    private void parseVecMoveFromScaInstr(VecMoveFromScaInstr ins, ASMBasicBlock newBlock) {
        VirFReg dst = (VirFReg) getFValue(ins, newBlock, true);
        List<VirReg> srcValList = new ArrayList<>();
        for (Value srcVal : ins.getSrcValueList()) {
            ASMValue asmValue = getValue(srcVal);
            if (asmValue instanceof ASMImmediate imm) {
                asmValue = immInt2reg(imm, newBlock);
            }
            srcValList.add((VirReg) asmValue);
        }
        for (int i = 0; i < srcValList.size(); i++) {
            genVecMoveFromSca(srcValList.get(i), i, newBlock, dst);
        }
    }

    private void parseVecStore(VecStoreInstr ins, ASMBasicBlock newBlock) {
        Value ptr = ins.getBasicPointer();

        VirFReg valueToStore = (VirFReg) getFValue(ins.getValueToStore(), newBlock, true);
        genVecStore(valueToStore, (Reg) getPtrVal(ptr, newBlock), newBlock);
    }

    private void parseMoveInstr(MoveInstr ins, ASMBasicBlock newBlock) {
        Value dst = ins.getDst();
        Value src = ins.getSrc();
        ASMValue dstVal;
        ASMValue srcVal;

        if (dst.getType() instanceof TVector && src.getType() instanceof TVector) {
            dstVal = getFValue(dst, newBlock, true);
            srcVal = getFValue(src, newBlock, true);
            genVecMove((VirFReg) srcVal, newBlock, (VirFReg) dstVal);
            return;
        }
        if (dst.getType() instanceof TVector && !(src.getType() instanceof TVector)) {
            dstVal = getFValue(dst, newBlock, true);
            if (src.getType() instanceof TFloat) {
                srcVal = getFValue(src, newBlock, false);
            } else {
                srcVal = getValue(src);
            }
            genVecRegClear(newBlock, (VirFReg) dstVal);
            genVecMoveFromSca((VirReg) srcVal, 0, newBlock, (VirFReg) dstVal);
            return;
        }
        if (!(dst.getType() instanceof TVector) && src.getType() instanceof TVector) {
            srcVal = getFValue(src, newBlock, true);
            if (dst.getType() instanceof TFloat) {
                dstVal = getFValue(dst, newBlock, false);
                genAddv((VirFReg) srcVal, newBlock, (VirFReg) dstVal);
            } else {
                dstVal = getValue(dst);
                VirFReg virFReg = new VirFReg(false);
                genAddv((VirFReg) srcVal, newBlock, virFReg);
                genVecMoveToSca(virFReg, 0, newBlock, (VirReg) dstVal);
            }
            return;
        }

        if (dst.getType() instanceof TFloat) {
            dstVal = getFValue(dst, newBlock, false);
        } else {
            dstVal = getValue(dst);
        }
        if (src.getType() instanceof TFloat) {
            srcVal = getFValue(src, newBlock, false);
        } else {
            srcVal = getValue(src);
        }

        boolean frozenFlag = dst instanceof Instruction dstIns && dstIns.getRegIndex() == -1;
        if (dstVal instanceof VirIReg) {
            if (frozenFlag) {
                genFrozenMove(srcVal, newBlock, (VirIReg) dstVal);
            } else {
                genMove(srcVal, newBlock, (VirIReg) dstVal);
            }
        } else if (dstVal instanceof VirFReg) {
            if (frozenFlag) {
                genFrozenMove(srcVal, newBlock, (VirFReg) dstVal);
            } else {
                genMove(srcVal, newBlock, (VirFReg) dstVal);
            }
        } else {
            throw new RuntimeException("Move 指令的源和目标类型不匹配");
        }
    }

    private void parseCallInstr(CallInstr ins, ASMBasicBlock newBlock) {
        pushRParams(ins.getRParams(), newBlock, ins.getCallee());
        if (newBlock.getParentFunction().isTailRecursive() && functionMap.get(ins.getCallee()) == newBlock.getParentFunction()) {
            ASMFunction asmFunction = newBlock.getParentFunction();
            genJump((ASMBasicBlock) asmFunction.getBasicBlockList().getHead().getNext(), newBlock);
            return;
        }
        if (functionMap.get(ins.getCallee()) == null) {
            // 库函数
            genJal(new ASMFunction(ins.getCallee().getName(), true, regStore), newBlock);
        } else {
            genJal(functionMap.get(ins.getCallee()), newBlock);
        }
        if (ins.getType() instanceof TInt) {
            VirIReg virReg = (VirIReg) getValue(ins);
            genMove(regStore.getIntRetVal(), newBlock, virReg); // $a0
        } else if (ins.getType() instanceof TFloat) {
            VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
            genMove(regStore.getFltRetVal(), newBlock, fvirReg); // $fa0
        }
    }

    private void pushRParams(List<Value> rParams, ASMBasicBlock newBlock, Function callee) {
        int intBaseCnt = 0, floatBaseCnt = 0;
        for (Value param : rParams) {
            if (param.getType() instanceof TFloat) {
                ASMValue fParam = getFValue(param, newBlock, false);
                genMove(fParam, newBlock, regStore.getFltArgRegByNumber(floatBaseCnt));
                floatBaseCnt++;
            } else if (param.getType() instanceof TInt || param.getType() instanceof TPointer) {
                ASMValue intParam = getValue(param);
                genMove(intParam, newBlock, regStore.getIntArgRegByNumber(intBaseCnt));
                intBaseCnt++;
            }
//            else if (param instanceof Function function) {
//                genLa(function.getName(), newBlock, regStore.getIntArgRegByNumber(intBaseCnt));
//                intBaseCnt++;
//            }
            if (intBaseCnt >= 8 || floatBaseCnt >= 8) {
                break; // 超过 8 个参数就不再继续放入寄存器
            }
        }
        if (intBaseCnt < 8 && floatBaseCnt < 8) {
            return; // 没有超过 8 个参数就不需要放入栈中
        }

        List<Value> intParams = new ArrayList<>();
        List<Value> intWordParams = new ArrayList<>();
        List<Value> intDoubleParams = new ArrayList<>();
        List<Value> floatParams = new ArrayList<>();
        for (int i = intBaseCnt + floatBaseCnt; i < rParams.size(); i++) {
            Value param = rParams.get(i);
            switch (param.getType()) {
                case TFloat _ -> floatParams.add(param);
                case TInt _ -> intWordParams.add(param);
                case TPointer _ -> intDoubleParams.add(param);
                case null, default ->
                        throw new RuntimeException("出现了尚未支持的参数类型：" + param.getType().printIRType());
            }
        }
        intParams.addAll(intDoubleParams);  // 先放需要双字对齐的避免对齐出问题
        intParams.addAll(intWordParams);

        ASMFunction parentFunc = newBlock.getParentFunction();
        if (parentFunc.isTailRecursive() && functionMap.get(callee) == parentFunc) {
            int offset = 0;
            for (Value rParam : intParams) {
                ASMValue valueToStore = getValue(rParam);
                if (valueToStore instanceof ASMImmediate imm) {
                    valueToStore = immInt2reg(imm, newBlock);
                }
                if (rParam.getType() instanceof TInt) {
                    genStore(new ASMStackOffset(parentFunc, offset), regStore.getStackPtr(), valueToStore, newBlock, false, false);
                    offset += 4;
                } else if (rParam.getType() instanceof TPointer) {
                    genStore(new ASMStackOffset(parentFunc, offset), regStore.getStackPtr(), valueToStore, newBlock, true, false);
                    offset += 8;
                } else {
                    throw new RuntimeException("出现了尚未支持的参数类型：" + rParam.getType().printIRType());
                }
            }
            for (Value rParam : floatParams) {
                ASMValue valueToStore = getFValue(rParam, newBlock, false);
                genStore(new ASMStackOffset(parentFunc, offset), regStore.getStackPtr(), valueToStore, newBlock, false, true);
                offset += 4;
            }
            return;
        }
        int offset = 0;
        for (Value rParam : intParams) {
            ASMValue valueToStore = getValue(rParam);
            if (valueToStore instanceof ASMImmediate imm) {
                valueToStore = immInt2reg(imm, newBlock);
            }
            if (rParam.getType() instanceof TInt) {
                genStore(new ASMImmediate(offset), regStore.getStackPtr(), valueToStore, newBlock, false, false);
                offset += 4;
            } else if (rParam.getType() instanceof TPointer) {
                genStore(new ASMImmediate(offset), regStore.getStackPtr(), valueToStore, newBlock, true, false);
                offset += 8;
            } else {
                throw new RuntimeException("出现了尚未支持的参数类型：" + rParam.getType().printIRType());
            }
        }
        for (Value rParam : floatParams) {
            ASMValue valueToStore = getFValue(rParam, newBlock, false);
            genStore(new ASMImmediate(offset), regStore.getStackPtr(), valueToStore, newBlock, false, true);
            offset += 4;
        }
    }

    protected void parseCmpInstr(CmpInstr ins, ASMBasicBlock newBlock) {
        Value op1 = ins.getOperand1();
        Value op2 = ins.getOperand2();
        boolean isFloat = !(op1.getType() instanceof TInt && op2.getType() instanceof TInt);
        if (isFloat) {
            ASMValue value1 = getFValue(op1, newBlock, false);
            ASMValue value2 = getFValue(op2, newBlock, false);
            renewValueMap(ins, switch (ins.getCond()) {
                case FLT -> generateFLT(value1, value2, newBlock);
                case FGT -> generateFGT(value1, value2, newBlock);
                case FEQ -> generateFEQ(value1, value2, newBlock);
                case FNE -> generateFNE(value1, value2, newBlock);
                case FLE -> generateFLE(value1, value2, newBlock);
                case FGE -> generateFGE(value1, value2, newBlock);
                default -> throw new RuntimeException("未定义的浮点比较指令");
            });
        } else {
            ASMValue value1 = getValue(op1);
            ASMValue value2 = getValue(op2);
            renewValueMap(ins, switch (ins.getCond()) {
                // 以下调用的函数实际上生成出来主要是 SLT
                case ILT -> generateSLT(value1, value2, newBlock);
                case IGT -> generateSGT(value1, value2, newBlock);
                case IEQ -> generateSEQ(value1, value2, newBlock);
                case INE -> generateSNE(value1, value2, newBlock);
                case ILE -> generateSLE(value1, value2, newBlock);
                case IGE -> generateSGE(value1, value2, newBlock);
                default -> throw new RuntimeException("未定义的比较指令");
            });
        }
    }

    protected abstract ASMValue generateSGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateSLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateSNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateSEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateSGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateSLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    protected abstract ASMValue generateFLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock);

    private void parseTerminator(Terminator ins, ASMBasicBlock newBlock) {
        switch (ins) {
            case BranchInstr branchInstr -> parseBranchInstr(branchInstr, newBlock);
            case JumpInstr jumpInstr -> parseJumpInstr(jumpInstr, newBlock);
            case ReturnInstr returnInstr -> parseReturnInstr(returnInstr, newBlock);
            case null, default -> throw new RuntimeException("出现了意料之外的终结指令");
        }
    }

    private void parseReturnInstr(ReturnInstr ins, ASMBasicBlock newBlock) {
        ASMFunction curFunc = newBlock.getParentFunction();
        genLoad(new ASMStackOffset(curFunc, -8), regStore.getStackPtr(), newBlock, regStore.getRetAddr(), true, false); // 恢复 $ra
        Value irRetVal = ins.getReturnValue();
        if (irRetVal != null) {
            if (irRetVal.getType() instanceof TFloat) {
                ASMValue retVal = getFValue(irRetVal, newBlock, false);
                genMove(retVal, newBlock, regStore.getFltRetVal()); // $fa0
            } else if (irRetVal.getType() instanceof TInt || irRetVal.getType() instanceof TPointer) {
                ASMValue retVal = getValue(irRetVal);
                genMove(retVal, newBlock, regStore.getIntRetVal()); // $a0
            } else {
                throw new RuntimeException("出现了尚未支持的返回值类型：" + irRetVal.getType().printIRType());
            }
        }
        ASMFunction parentFunc = newBlock.getParentFunction();
        genAdd(regStore.getStackPtr(), new ASMStackOffset(parentFunc, 0), newBlock, regStore.getStackPtr()); // 移动栈指针
        genJr(regStore.getRetAddr(), newBlock);
    }

    private void parseJumpInstr(JumpInstr ins, ASMBasicBlock newBlock) {
        genJump(basicBlockMap.get(ins.getTarget()), newBlock);
    }

    protected void parseBranchInstr(BranchInstr ins, ASMBasicBlock newBlock) {
        ASMValue cond = getValue(ins.getCondition());
        if (cond instanceof ASMImmediate imm) {
            cond = immInt2reg(imm, newBlock);
        }
        genBeq(regStore.getZero(), (Reg) cond, basicBlockMap.get(ins.getElseBlk()), newBlock);
        genJump(basicBlockMap.get(ins.getThenBlk()), newBlock);
    }

    private void parseMemoryOperation(MemoryOperation ins, ASMBasicBlock newBlock) {
        switch (ins) {
            case AllocaInstr allocaInstr -> parseAllocaInstr(allocaInstr, newBlock);
            case GEPInstr gepInstr -> parseGEPInstr(gepInstr, newBlock);
            case LoadInstr loadInstr -> parseLoadInstr(loadInstr, newBlock);
            case StoreInstr storeInstr -> parseStoreInstr(storeInstr, newBlock);
            case null, default -> throw new RuntimeException("出现了尚未支持的内存指令");
        }
    }

    private void parseStoreInstr(StoreInstr ins, ASMBasicBlock newBlock) {
        Value ptr = ins.getPointer();
        Type type = ins.getValueToStore().getType();
//        System.out.println(type.printIRType());
        switch (type) {
            case TInt _ -> {
                ASMValue valueToStore = getValue(ins.getValueToStore());
                if (valueToStore instanceof ASMImmediate imm) {
                    valueToStore = immInt2reg(imm, newBlock);
                }
                genStore(getPtrVal(ptr, newBlock), valueToStore, newBlock, false, false);
            }
            case TFloat _ -> {
                VirFReg fvirReg = (VirFReg) getFValue(ins.getValueToStore(), newBlock, false);
                genStore(getPtrVal(ptr, newBlock), fvirReg, newBlock, false, true);
            }
            case TPointer _ -> {
                if (ins.getValueToStore() instanceof AllocaInstr allocaInstr) {
                    VirIReg virReg = new VirIReg();
                    virReg.setDouble(true);
                    int offset = alloca2offset.get(allocaInstr);
                    genAdd(regStore.getStackPtr(), new ASMImmediate(offset), newBlock, virReg);
                    genStore(getPtrVal(ptr, newBlock), virReg, newBlock, true, false);
                } else {
                    VirIReg virReg = (VirIReg) getValue(ins.getValueToStore());
                    genStore(getPtrVal(ptr, newBlock), virReg, newBlock, true, false);
                }
            }
            case null, default -> {
                assert type != null;
                throw new RuntimeException("出现了尚未支持的 store 指令类型：" + type.printIRType());
            }
        }
    }

    private void parseLoadInstr(LoadInstr ins, ASMBasicBlock newBlock) {
        Value ptr = ins.getPointer();
        Type type = ins.getType();
        switch (type) {
            case TInt _ -> {
                VirIReg virReg = (VirIReg) getValue(ins);
                genLoad(getPtrVal(ptr, newBlock), newBlock, virReg, false, false, false);
            }
            case TFloat _ -> {
                VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
                genLoad(getPtrVal(ptr, newBlock), newBlock, fvirReg, false, true, false);
            }
            case TPointer _ -> {
                VirIReg virReg = (VirIReg) getValue(ins);
                genLoad(getPtrVal(ptr, newBlock), newBlock, virReg, true, false, false);
            }
            case null, default -> {
                assert type != null;
                throw new RuntimeException("出现了尚未支持的 load 指令类型：" + type.printIRType());
            }
        }
    }

    private ASMValue getPtrVal(Value ptr, ASMBasicBlock newBlock) {
        if (ptr instanceof AllocaInstr alloca) {
            VirIReg tmpVirReg = new VirIReg();
            tmpVirReg.setDouble(true);
            genAdd(regStore.getStackPtr(), new ASMImmediate(alloca2offset.get(alloca)),
                    newBlock, tmpVirReg);
            return tmpVirReg;
        }
        if (ptr instanceof GlbObjPtr) {
            VirIReg tmpVirReg = new VirIReg();
            tmpVirReg.setDouble(true);
            genLa(glbObjMap.get(ptr), newBlock, tmpVirReg);
            return tmpVirReg;
        }
        return getValue(ptr);
    }

    private void parseGEPInstr(GEPInstr ins, ASMBasicBlock newBlock) {
        List<Value> indexList = ins.getIndexList();
        Value ptrVal = ins.getPtrVal();
        Type ptrType = ptrVal.getType();
        Type referencedType = ((TPointer) ptrType).getReferencedType();
        List<Integer> sizeList = referencedType.getSizeList();
        VirIReg virReg = (VirIReg) getValue(ins);
        if (tryMergeGep(ins, newBlock, sizeList, virReg)) {
            return;
        }
        int indexSize = indexList.size();
        ASMValue ptrBase;
        if (ptrVal instanceof AllocaInstr) {
            VirIReg tmpVirReg = new VirIReg();
            tmpVirReg.setDouble(true);
            genAdd(regStore.getStackPtr(), new ASMImmediate(alloca2offset.get(ptrVal)), newBlock, tmpVirReg);
            ptrBase = tmpVirReg;
        } else if (ptrVal instanceof GEPInstr || ptrVal instanceof LoadInstr || ptrVal instanceof Function.FParam || ptrVal instanceof PhiInstr) {
            ptrBase = getValue(ptrVal);
        } else if (ptrVal instanceof GlbObjPtr) {
            VirIReg tmpVirReg = new VirIReg();
            tmpVirReg.setDouble(true);
            genLa(glbObjMap.get(ptrVal), newBlock, tmpVirReg);
            ptrBase = tmpVirReg;
        } else {
            throw new RuntimeException("出现了未曾设想的指针");
        }

        for (int i = 0; i < indexSize; i++) {
            ASMValue indexValue = getValue(indexList.get(i));
            VirIReg tmpReg = new VirIReg();
            tmpReg.setDouble(true);
            if (indexValue instanceof ASMImmediate imm) {
                int index = imm.getImm();
                int baseOffset = sizeList.get(i);
                ASMImmediate offset = new ASMImmediate(index * baseOffset);
                genAdd((Reg) ptrBase, offset, newBlock, tmpReg);
            } else {
                ASMImmediate baseOffset = new ASMImmediate(sizeList.get(i));
                VirIReg tmpVirReg = new VirIReg();
                // mulByConstInstr((Reg) indexValue, immInt2reg(baseOffset, newBlock), newBlock, tmpVirReg);
//                mulByConstInstr(tmpVirReg, indexValue, baseOffset.getImm(), newBlock);
                ASMMulPlanner.mulConst(tmpVirReg, (Reg) indexValue, baseOffset.getImm(), newBlock, regStore);
                genAdd((Reg) ptrBase, tmpVirReg, newBlock, tmpReg);
            }
            ptrBase = tmpReg; // 更新 ptrBase 为新的地址
        }
        genMove(ptrBase, newBlock, virReg); // 将计算出的地址存入目标寄存器
    }

    private boolean tryMergeGep(GEPInstr nowGep, ASMBasicBlock newBlock, List<Integer> dimSizeList, VirIReg dstReg) {
        if (!block2GEPs.containsKey(newBlock)) {
            ArrayList<GEPInstr> gepListNew = new ArrayList<>();
            gepListNew.add(nowGep);
            block2GEPs.put(newBlock, gepListNew);
            return false;
        }
        ArrayList<GEPInstr> gepList = block2GEPs.get(newBlock);
        for (GEPInstr recordGep : gepList) {
            int offset = 0;
            boolean canMerge = true;
            if (recordGep.getPtrVal().equals(nowGep.getPtrVal())) {
                List<Value> recordIndexList = recordGep.getIndexList();
                List<Value> nowIndexList = nowGep.getIndexList();
                if (recordIndexList.size() != nowIndexList.size()) {
                    continue;
                }
                int indexSize = recordIndexList.size();
                for (int i = 0; i < indexSize; i++) {
                    Value recordIndex = recordIndexList.get(i);
                    Value nowIndex = nowIndexList.get(i);
                    if (recordIndex instanceof IRConst recordConst && nowIndex instanceof IRConst nowConst) {
                        offset += (nowConst.getConstVal().intValue() - recordConst.getConstVal().intValue()) * dimSizeList.get(i);
                    } else {
                        if (nowIndex instanceof AddInstr addInstr
                                && addInstr.getOperand1().equals(recordIndex)
                                && addInstr.getOperand2() instanceof IRConst addConst) {
                            offset += addConst.getConstVal().intValue() * dimSizeList.get(i);
                        } else if (nowIndex instanceof SubInstr subInstr
                                && subInstr.getOperand1().equals(recordIndex)
                                && subInstr.getOperand2() instanceof IRConst subConst) {
                            offset -= subConst.getConstVal().intValue() * dimSizeList.get(i);
                        } else if (recordIndex.equals(nowIndex)) {
                            offset += 0;
                        } else {
                            canMerge = false;
                            break;
                        }
                    }
                }
                if (canMerge) {
                    VirIReg recordReg = (VirIReg) getValue(recordGep);
                    genAdd(recordReg, new ASMImmediate(offset), newBlock, dstReg);
                    return true;
                }
            }
        }
        block2GEPs.get(newBlock).add(nowGep);
        return false;
    }


    private void parseAllocaInstr(AllocaInstr ins, ASMBasicBlock newBlock) {
        ASMFunction parentFunc = newBlock.getParentFunction();
        int basicOffset = parentFunc.getReservedParamSize() + parentFunc.getAllocatedSize();
        if (ins.getReferencedType() instanceof TArray completeArray) {
            int paddingBefore = (16 - (basicOffset & 15)) & 15;
            alloca2offset.put(ins, basicOffset + paddingBefore);
            int size = 4 * completeArray.getFullSize();
            int paddingAfter = (16 - (size & 15)) & 15;
            parentFunc.addAllocatedSize(size + paddingBefore + paddingAfter);
        } else if (ins.getReferencedType() instanceof TPointer) {
            alloca2offset.put(ins, basicOffset);
            parentFunc.addAllocatedSize(8);
        } else {
            alloca2offset.put(ins, basicOffset);
            parentFunc.addAllocatedSize(4);
        }
    }


    private void parseConversionOperation(ConversionOperation ins, ASMBasicBlock newBlock) {
        Value base = ins.getValue();
        //由于实际上没有char类型，trunc可以不做处理
        if (ins instanceof Fp2Si) {
            parseFp2Si((Fp2Si) ins, base, newBlock);
        } else if (ins instanceof Si2Fp) {
            parseSi2Fp((Si2Fp) ins, base, newBlock);
        } else {
            genMove(getValue(base), newBlock, (Reg) getValue(ins));
        }
    }

    private void parseFp2Si(Fp2Si ins, Value base, ASMBasicBlock newBlock) {
        ASMValue baseValue = getFValue(base, newBlock, false);
        ASMValue targetValue = getValue(ins);
        if (targetValue instanceof ASMImmediate imm) {
            targetValue = immInt2reg(imm, newBlock);
        }
        genFtoi((Reg) baseValue, newBlock, (Reg) targetValue);
    }

    private void parseSi2Fp(Si2Fp ins, Value base, ASMBasicBlock newBlock) {
        ASMValue baseValue = getValue(base);
        if (baseValue instanceof ASMImmediate imm) {
            baseValue = immInt2reg(imm, newBlock);
        }
        ASMValue targetValue = getFValue(ins, newBlock, false);
        genItof((Reg) baseValue, newBlock, (Reg) targetValue);
    }

    private void parseBinaryOperation(BinaryOperation ins, ASMBasicBlock newBlock, BasicBlock basicBlock) {
        Value operand1 = ins.getOperand1();
        Value operand2 = ins.getOperand2();
        boolean isFloat = !(operand1.getType() instanceof TInt && operand2.getType() instanceof TInt);
        if (isFloat) {
            ASMValue val1 = getFValue(operand1, newBlock, false);
            ASMValue val2 = getFValue(operand2, newBlock, false);
            switch (ins) {
                case FAddInstr fAddInstr -> parseFAddInstr(fAddInstr, val1, val2, newBlock);
                case FSubInstr fSubInstr -> parseFSubInstr(fSubInstr, val1, val2, newBlock);
                case FMulInstr fMulInstr -> parseFMulInstr(fMulInstr, val1, val2, newBlock);
                case FDivInstr fDivInstr -> parseFDivInstr(fDivInstr, val1, val2, newBlock);
                default -> throw new RuntimeException("出现了尚未支持的浮点二元运算");
            }
        } else {
            ASMValue val1 = getValue(operand1);
            ASMValue val2 = getValue(operand2);
            switch (ins) {
                case AddInstr addInstr -> parseAddInstr(addInstr, val1, val2, newBlock);
                case SubInstr subInstr -> parseSubInstr(subInstr, val1, val2, newBlock);
                case MulInstr mulInstr -> parseMulInstr(mulInstr, val1, val2, newBlock);
                // operand1, basicBlock is for divByConstInstr
                case SDivInstr sDivInstr -> parseDivInstr(sDivInstr, val1, val2, newBlock, operand1, basicBlock);
                case SRemInstr sRemInstr -> parseRemInstr(sRemInstr, val1, val2, newBlock, operand1, basicBlock);
                default -> throw new RuntimeException("出现了尚未支持的整数二元运算");
            }
        }
    }

    private void parseRemInstr(SRemInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock, Value operand1, BasicBlock basicBlock) {
        // 这里的处理方式是，如果两个操作数都是常数，则直接计算结果；否则，生成除法和乘法指令来计算余数
        // 这种方式可以避免在除数为0时出现异常
//        if (val1 instanceof ASMImmediate imm1) {
//            if (val2 instanceof ASMImmediate imm2) {
//                renewValueMap(ins, new ASMImmediate(imm1.getImm() % imm2.getImm()));
//            } else {
//                VirIReg virReg = (VirIReg) getValue(ins);
//                genRem(immInt2reg(imm1, newBlock), (Reg) val2, newBlock, virReg);
//            }
//        } else {
//            if (val2 instanceof ASMImmediate imm2) {
//                VirIReg virReg = (VirIReg) getValue(ins);
//                genRem((Reg) val1, immInt2reg(imm2, newBlock), newBlock, virReg);
//            } else {
//                VirIReg virReg = (VirIReg) getValue(ins);
//                genRem((Reg) val1, (Reg) val2, newBlock, virReg);
//            }
//        }
        if (val1 instanceof ASMImmediate imm1 && val2 instanceof ASMImmediate imm2) {
            renewValueMap(ins, new ASMImmediate(imm1.getImm() % imm2.getImm()));
        } else {
            VirIReg virReg = (VirIReg) getValue(ins);
            if (val2 instanceof ASMImmediate imm2) {
                remByConstInstr(virReg, val1, imm2.getImm(), newBlock, operand1, basicBlock);
            } else {
                if (val1 instanceof ASMImmediate imm1) {
                    val1 = immInt2reg(imm1, newBlock);
                }
                genRem((Reg) val1, (Reg) val2, newBlock, virReg);
            }
        }
    }

    private void remByConstInstr(VirIReg dstReg, ASMValue val1, int imm, ASMBasicBlock newBlock, Value operand1, BasicBlock basicBlock) {
        if (imm == 1) {
            genMove(regStore.getZero(), newBlock, dstReg);
        } else if (imm == -1) {
            genMove(regStore.getZero(), newBlock, dstReg);
        } else if (imm > 0 && isTwoTimes(imm)) {
//            VirIReg tmpVirReg1 = new VirIReg();
//            genSrl((Reg) val1, new ASMImmediate(16), newBlock, tmpVirReg1);
//            VirIReg tmpVirReg2 = new VirIReg();
//            genAdd(tmpVirReg1, val1, newBlock, tmpVirReg2);
//            genAnd(tmpVirReg2, new ASMImmediate(0xFFFF), newBlock, dstReg);
            Range val1Range = RangeAnalysisV3.getRange(operand1, basicBlock);
            if (val1Range.min >= 0) {
                int mask = imm - 1;
                genAnd((Reg) val1, new ASMImmediate(mask), newBlock, dstReg);
            } else {
                VirIReg tmp0 = new VirIReg();
                VirIReg tmp1 = new VirIReg();
                genSra((Reg) val1, new ASMImmediate(31), newBlock, tmp0);
                genAnd(tmp0, new ASMImmediate(imm - 1), newBlock, tmp0);
                genAdd((Reg) val1, tmp0, newBlock, tmp1);
                genSra(tmp1, new ASMImmediate(log2(imm)), newBlock, tmp1);
                genSll(tmp1, new ASMImmediate(log2(imm)), newBlock, tmp0);
                genSub((Reg) val1, tmp0, newBlock, dstReg);
            }
        } else {
            VirIReg tmpVirReg1 = new VirIReg();
            divByConstInstr(tmpVirReg1, val1, imm, newBlock, operand1, basicBlock);
//            Reg immReg0 = immInt2reg(new ASMImmediate(imm), newBlock);
//            genDiv((Reg) val1, immReg0, newBlock, tmpVirReg1);
            newBlock = fillingBlock;
            VirIReg tmpVirReg2 = new VirIReg();
//            mulByConstInstr(tmpVirReg2, tmpVirReg1, imm, newBlock);
            ASMMulPlanner.mulConst(tmpVirReg2, tmpVirReg1, imm, newBlock, regStore);
            genSub((Reg) val1, tmpVirReg2, newBlock, dstReg);
        }
    }

    private void parseDivInstr(SDivInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock, Value operand1, BasicBlock basicBlock) {
        if (val1 instanceof ASMImmediate imm1 && val2 instanceof ASMImmediate imm2) {
            renewValueMap(ins, new ASMImmediate(imm1.getImm() / imm2.getImm()));
        } else {
            VirIReg virReg = (VirIReg) getValue(ins);
            if (val2 instanceof ASMImmediate imm2) {
                divByConstInstr(virReg, val1, imm2.getImm(), newBlock, operand1, basicBlock);
            } else {
                if (val1 instanceof ASMImmediate imm1) {
                    val1 = immInt2reg(imm1, newBlock);
                }
                genDiv((Reg) val1, (Reg) val2, newBlock, virReg);
            }
        }
//        if (val1 instanceof ASMImmediate imm1) {
//            val1 = immInt2reg(imm1, newBlock);
//        }
//        if (val2 instanceof ASMImmediate imm2) {
//            val2 = immInt2reg(imm2, newBlock);
//        }
//        genDiv((Reg) val1, (Reg) val2, newBlock, (VirIReg) getValue(ins));
    }

    private void divByConstInstr(VirIReg dstReg, ASMValue val1, int imm, ASMBasicBlock newBlock, Value operand1, BasicBlock basicBlock) {
        if (imm == 1) {
            genMove(val1, newBlock, dstReg);
            return;
        }
        if (imm == -1) {
            genSub((Reg) val1, regStore.getZero(), newBlock, dstReg);
            return;
        }
//        boolean allPositive = RangeAnalysisV3.getRange(operand1, basicBlock).min >= 0 && imm > 0;
//        if (allPositive) {
//            fillPositiveDivInstrs(dstReg, val1, imm, newBlock);
//        } else {
//            fillAllDivInstrs(dstReg, val1, imm, newBlock);
//        }
        fillAllDivInstrs(dstReg, val1, imm, newBlock);
    }

    private ASMBasicBlock createEmptyBlock(BasicBlock oriBlock) {
        BasicBlock newIrBlock = new BasicBlock(oriBlock.getParentFunc().getAndAddBlkIdx());
        newIrBlock.setParentFunc(oriBlock.getParentFunc());
        ASMBasicBlock newAsmBlock = new ASMBasicBlock(functionMap.get(oriBlock.getParentFunc()), newIrBlock.getLoopDepth(), newIrBlock.value2string());
        basicBlockMap.put(newIrBlock, newAsmBlock);
        return newAsmBlock;
    }

    /**
     * 使用正数除法的魔数乘法优化，无需处理负数情况的修正步骤
     * 算法：对于 n/d，找到 m 和 sh，使得 n/d = (n * m) >> sh
     * 1. 计算 l = ceil(log2(d))
     * 2. 计算 m = ceil((2^(32+l-1)) / d)
     * 3. 结果 = (val1 * m)的高32位 >> (l-1)
     */
    private void fillPositiveDivInstrs(VirIReg dstReg, ASMValue val1, int imm, ASMBasicBlock newBlock) {
        int absValue = Math.abs(imm);
        if (isTwoTimes(absValue)) {
            int shift = getShift(absValue);
            genSra((Reg) val1, new ASMImmediate(shift), newBlock, dstReg);
        } else {
            int l = 32 - Integer.numberOfLeadingZeros(absValue - 1);
            int shift = l - 1;
            long m_long = ((1L << (32 + l - 1)) + absValue - 1) / absValue;

            Reg tmp0 = new VirIReg();
            genMove(new ASMImmediate(m_long), newBlock, tmp0);

            VirIReg tmp1 = new VirIReg();
            tmp1.setDouble(true);
            genMul((Reg) val1, tmp0, newBlock, tmp1);

            VirIReg tmp2 = new VirIReg();
            tmp2.setDouble(true);
            genSrl(tmp1, new ASMImmediate(32), newBlock, tmp2);

            if (shift > 0) {
                genSra(tmp2, new ASMImmediate(shift), newBlock, dstReg);
            } else {
                genMove(tmp2, newBlock, dstReg);
            }
        }
        if (imm < 0) {
            genSub(regStore.getZero(), dstReg, newBlock, dstReg);
        }
    }

    private void fillAllDivInstrs(VirIReg dstReg, ASMValue val1, int imm, ASMBasicBlock newBlock) {
        boolean isImmNeg = imm < 0;
        imm = Math.abs(imm);
        VirIReg ans = isImmNeg ? new VirIReg() : dstReg;
        if (isTwoTimes(imm)) {
            int x = log2(imm);
            if (x <= 30) {
                VirIReg op1 = new VirIReg();
                op1.setDouble(true);
                VirIReg op2 = new VirIReg();
                genSrl((Reg) val1, new ASMImmediate(64 - x), newBlock, op1);
                genAdd((Reg) val1, op1, newBlock, op2);
                genSra(op2, new ASMImmediate(x), newBlock, ans);
            } else {
                VirIReg immReg = immInt2reg(new ASMImmediate(imm), newBlock);
                genDiv((Reg) val1, immReg, newBlock, ans);
            }
        } else {
            int sh = log2(imm);
            BigInteger tmp = BigInteger.ONE;
            long low = tmp.shiftLeft(32 + sh).divide(BigInteger.valueOf(imm)).longValue();
            long high = tmp.shiftLeft(32 + sh).add(tmp.shiftLeft(sh + 1)).divide(BigInteger.valueOf(imm)).longValue();
            while (((low / 2) < (high / 2)) && sh > 0) {
                low /= 2;
                high /= 2;
                sh--;
            }
            if (high < (1L << 31)) {
                VirIReg op1 = new VirIReg();
                op1.setDouble(true);
                VirIReg op2 = new VirIReg();
                op2.setDouble(true);
                VirIReg op3 = new VirIReg();
                VirIReg immReg = new VirIReg();
                immReg.setDouble(true);
                genMove(new ASMImmediate(high), newBlock, immReg);
                genMul((Reg) val1, immReg, newBlock, op1);
                genSra(op1, new ASMImmediate(32 + sh), newBlock, op2);
                genSra((Reg) val1, new ASMImmediate(31), newBlock, op3);
                genSub(op2, op3, newBlock, ans);
            } else {
                high -= (1L << 32);
                VirIReg op1 = new VirIReg();
                op1.setDouble(true);
                VirIReg op2 = new VirIReg();
                op2.setDouble(true);
                VirIReg op3 = new VirIReg();
                op3.setDouble(true);
                VirIReg op4 = new VirIReg();
                op4.setDouble(true);
                VirIReg op5 = new VirIReg();
                VirIReg immReg = new VirIReg();
                immReg.setDouble(true);
                genMove(new ASMImmediate(high), newBlock, immReg);
                genMul((Reg) val1, immReg, newBlock, op1);
                genSra(op1, new ASMImmediate(32), newBlock, op2);
                genAdd(op2, val1, newBlock, op3);
                genSra(op3, new ASMImmediate(sh), newBlock, op4);
                genSra((Reg) val1, new ASMImmediate(31), newBlock, op5);
                genSub(op4, op5, newBlock, ans);
            }
            if (isImmNeg) {
                genSub(regStore.getZero(), ans, newBlock, dstReg);
            }
        }


//        if (isTwoTimes(Math.abs(imm))) {
//            int absValue = Math.abs(imm);
//            int shift = getShift(absValue);
//            VirIReg tmp1 = new VirIReg();
//            VirIReg tmp2 = new VirIReg();
//            tmp1.setDouble(true);
//            genSrl((Reg) val1, new ASMImmediate(63 - shift), newBlock, tmp1);
//            genAdd(tmp1, val1, newBlock, tmp2);
//            genSra(tmp2, new ASMImmediate(shift), newBlock, dstReg);
//        } else {
//            int absValue = Math.abs(imm);
//            long nc = ((long) 1 << 31) - (((long) 1 << 31) % absValue) - 1;
//            long p = 32;
//            while (((long) 1 << p) <= nc * (absValue - ((long) 1 << p) % absValue)) {
//                p++;
//            }
//            long m = ((((long) 1 << p) + (long) absValue - ((long) 1 << p) % absValue) / (long) absValue);
//            int n = (int) ((m << 32) >>> 32);
//            int shift = (int) (p - 32);
//            Reg tmp0 = new VirIReg();
//            genMove(new ASMImmediate(n), newBlock, tmp0);
//            VirIReg tmp1 = new VirIReg();
//            tmp1.setDouble(true);
//            VirIReg tmp3 = new VirIReg();
//            tmp3.setDouble(true);
//            genMul((Reg) val1, tmp0, newBlock, tmp1);
//            VirIReg tmp2 = new VirIReg();
//            tmp2.setDouble(true);
//            VirIReg tmp4 = new VirIReg();
//            if (m >= 0x80000000L) {
//                genSra(tmp1, new ASMImmediate(32), newBlock, tmp3);
//                genAdd(tmp3, val1, newBlock, tmp3);
//                genSra(tmp3, new ASMImmediate(shift), newBlock, tmp2);
//
//            } else {
//                genSra(tmp1, new ASMImmediate(32 + shift), newBlock, tmp2);
//            }
//            genSra(tmp2, new ASMImmediate(31), newBlock, tmp4);
//            genSub(tmp2, tmp4, newBlock, dstReg);
//        }
//        if (imm < 0) {
//            genSub(regStore.getZero(), dstReg, newBlock, dstReg);
//        }
    }

    private void parseFDivInstr(FDivInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        // 浮点数都会预先存储到寄存器中，所以我们只需要加一条fdiv即可
        VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
        genFDiv((VirFReg) val1, (VirFReg) val2, newBlock, fvirReg);
    }

    private void parseMulInstr(MulInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        if (val1 instanceof ASMImmediate imm1 && val2 instanceof ASMImmediate imm2) {
            renewValueMap(ins, new ASMImmediate(imm1.getImm() * imm2.getImm()));
        } else {
            VirIReg virReg = (VirIReg) getValue(ins);
            if (val1 instanceof ASMImmediate imm1) {
                ASMMulPlanner.mulConst(virReg, (Reg) val2, imm1.getImm(), newBlock, regStore);
            } else if (val2 instanceof ASMImmediate imm2) {
                ASMMulPlanner.mulConst(virReg, (Reg) val1, imm2.getImm(), newBlock, regStore);
            } else {
                genMul((Reg) val1, (Reg) val2, newBlock, virReg);
            }
        }
//        if (val1 instanceof ASMImmediate imm1) {
//            val1 = immInt2reg(imm1, newBlock);
//        }
//        if (val2 instanceof ASMImmediate imm2) {
//            val2 = immInt2reg(imm2, newBlock);
//        }
//        genMul((Reg) val1, (Reg) val2, newBlock, (VirIReg) getValue(ins));
    }

    private void mulByConstInstr(VirIReg dstReg, ASMValue val1, int imm, ASMBasicBlock newBlock) {
        if (imm == 1) {
            genMove(val1, newBlock, dstReg);
        } else if (imm == 0) {
            genMove(regStore.getZero(), newBlock, dstReg);
        } else if (imm == -1) {
            genSub(regStore.getZero(), val1, newBlock, dstReg);
        } else if (isTwoTimes(Math.abs(imm))) {
            int absValue = Math.abs(imm);
            int shift = getShift(absValue);
            genSll((Reg) val1, new ASMImmediate(shift), newBlock, dstReg);
            if (imm < 0) {
                genSub(regStore.getZero(), dstReg, newBlock, dstReg);
            }
        } else if (isTwoTimes(Math.abs(imm) + 1)) {
            int absValue = Math.abs(imm) + 1;
            int shift = getShift(absValue);
            VirIReg tmp0 = new VirIReg();
            tmp0.setDouble(true);
            genSll((Reg) val1, new ASMImmediate(shift), newBlock, tmp0);
            VirIReg tmp1 = new VirIReg();
            tmp1.setDouble(true);
            genSub(tmp0, val1, newBlock, tmp1);
            genMove(tmp1, newBlock, dstReg);
            if (imm < 0) {
                genSub(regStore.getZero(), dstReg, newBlock, dstReg);
            }
        } else if (isTwoTimes(Math.abs(imm) - 1)) {
            int absValue = Math.abs(imm) - 1;
            int shift = getShift(absValue);
            VirIReg tmp0 = new VirIReg();
            genSll((Reg) val1, new ASMImmediate(shift), newBlock, tmp0);
            genAdd(tmp0, val1, newBlock, dstReg);
            if (imm < 0) {
                genSub(regStore.getZero(), dstReg, newBlock, dstReg);
            }
        } else {
            Reg val2 = immInt2reg(new ASMImmediate(imm), newBlock);
            genMul((Reg) val1, val2, newBlock, dstReg);
        }
    }

    public int getShift(int value) {
        int shift = 0;
        while (value != 0) {
            value >>= 1;
            shift++;
        }
        return shift - 1;
    }

    public boolean isTwoTimes(int value) {
        return (value & (value - 1)) == 0;
    }

    public int log2(int value) {
        return 31 - Integer.numberOfLeadingZeros(value);
    }

    private void parseFMulInstr(FMulInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        // 浮点数都会预先存储到寄存器中，所以我们只需要加一条fmul即可
        VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
        genFMul((VirFReg) val1, (VirFReg) val2, newBlock, fvirReg);
    }

    private void parseSubInstr(SubInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        if (val1 instanceof ASMImmediate imm1) {
            if (val2 instanceof ASMImmediate imm2) {
                renewValueMap(ins, new ASMImmediate(imm1.getImm() - imm2.getImm()));
            } else {
                VirIReg virReg = (VirIReg) getValue(ins);
                genSub(immInt2reg(imm1, newBlock), val2, newBlock, virReg);
            }
        } else {
            if (val2 instanceof ASMImmediate imm2) {
                ASMImmediate imm = new ASMImmediate(imm2.getImm() * -1);
                VirIReg virReg = (VirIReg) getValue(ins);
                genAdd((VirIReg) val1, imm, newBlock, virReg);
            } else {
                VirIReg virReg = (VirIReg) getValue(ins);
                genSub((VirIReg) val1, val2, newBlock, virReg);
            }
        }
    }

    private void parseFSubInstr(FSubInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        // 浮点数都会预先存储到寄存器中，所以我们只需要加一条fsub即可
        VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
        genFSub((VirFReg) val1, (VirFReg) val2, newBlock, fvirReg);
    }

    private void parseAddInstr(AddInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        if (val1 instanceof ASMImmediate imm1) {
            if (val2 instanceof ASMImmediate imm2) {
                renewValueMap(ins, new ASMImmediate(imm1.getImm() + imm2.getImm()));
            } else {
                VirIReg virReg = (VirIReg) getValue(ins);
                genAdd((Reg) val2, imm1, newBlock, virReg);
            }
        } else {
            VirIReg virReg = (VirIReg) getValue(ins);
            genAdd((Reg) val1, val2, newBlock, virReg);
        }
    }

    private void parseFAddInstr(FAddInstr ins, ASMValue val1, ASMValue val2, ASMBasicBlock newBlock) {
        // 浮点数都会预先存储到寄存器中，所以我们只需要加一条fadd即可
        VirFReg fvirReg = (VirFReg) getFValue(ins, newBlock, false);
        genFAdd((VirFReg) val1, (VirFReg) val2, newBlock, fvirReg);
    }

    private void parseFNegInstr(FNegInstr ins, ASMBasicBlock newBlock) {
        VirFReg src = (VirFReg) getFValue(ins.getValue(), newBlock, false);
        VirFReg dst = (VirFReg) getFValue(ins, newBlock, false);
        genFNeg(src, newBlock, dst);
    }

    private void parseAtomaticAddInstr(AtomaticAddInstr ins, ASMBasicBlock newBlock) {
        ASMValue ptr = getValue(ins.getPtr());
        ASMValue increment = getValue(ins.getInc());
        if (increment instanceof ASMImmediate imm) {
            increment = immInt2reg(imm, newBlock);
        }
        genAtomAdd(ptr, increment, newBlock, regStore.getZero());
    }


    protected void renewValueMap(Value value, ASMValue newAsmValue) {
        valueMap.put(value, newAsmValue);
    }

    protected ASMValue getValue(Value value) {
        if (value instanceof IRConst) {
            //return new ASMImmediate(((IRConst) value).getConstVal());
            if (value instanceof IntConst intConst) {
                return new ASMImmediate(intConst.getConstVal().intValue());
            } else if (value instanceof FloatConst) {
                throw new RuntimeException("浮点数常量单独处理");
            } else {
                throw new RuntimeException("不支持的常量类型");
            }
        } else {
            if (valueMap.containsKey(value)) {
                return valueMap.get(value);
            } else if (value.getType() instanceof TFloat) {
                throw new RuntimeException("浮点数变量单独处理");
            } else {
                VirIReg virReg = new VirIReg();
                renewValueMap(value, virReg);
                if (value.getType() instanceof TPointer) {
                    virReg.setDouble(true);
                }
                return virReg;
            }
        }
    }

    protected ASMValue getFValue(Value value, ASMBasicBlock block, boolean isVector) {
        if (value instanceof FloatConst floatConst) {
            int intValue = Float.floatToRawIntBits(floatConst.getConstVal().floatValue());
            VirFReg fvirReg = new VirFReg(isVector);
            ASMGlobalObject floatLabel = genFloatLabel(intValue);
            VirIReg virReg = new VirIReg();
            virReg.setDouble(true);
            genLa(floatLabel, block, virReg);
            genLoad(virReg, block, fvirReg, false, true, true); // 加载浮点数到虚拟寄存器
            return fvirReg;
        } else {
            if (valueMap.containsKey(value)) {
                return valueMap.get(value);
            } else {
                VirFReg fvirReg = new VirFReg(isVector);
                renewValueMap(value, fvirReg);
                return fvirReg;
            }
        }
    }

    private ASMGlobalObject genFloatLabel(int intValue) {
        if (floatLabelMap.containsKey(intValue)) {
            return floatLabelMap.get(intValue);
        } else {
            ASMGlobalObject asmGlobalObject = new ASMGlobalObject(intValue);
            floatLabelMap.put(intValue, asmGlobalObject);
            glbObjList.add(asmGlobalObject);
            return asmGlobalObject;
        }
    }


    /**
     * 将 a 寄存器或者运行栈中的参数取出来并建立映射
     */
    private void popRParams(Function function, ASMBasicBlock firstBlock) {
        int intBaseCnt = 0, floatBaseCnt = 0;
        for (Value fParam : function.getFParams()) {
            if (fParam.getType() instanceof TFloat) {
                genMove(regStore.getFltArgRegByNumber(floatBaseCnt++), firstBlock, (VirFReg) getFValue(fParam, firstBlock, false));
            } else if (fParam.getType() instanceof TInt) {
                genMove(regStore.getIntArgRegByNumber(intBaseCnt++), firstBlock, (VirIReg) getValue(fParam));
            } else if (fParam.getType() instanceof TPointer) {
                VirIReg virIReg = (VirIReg) getValue(fParam);
                virIReg.setDouble(true);
                genMove(regStore.getIntArgRegByNumber(intBaseCnt++), firstBlock, virIReg);
            }
            if (intBaseCnt >= 8 || floatBaseCnt >= 8) {
                break;
            }
        }
        if (intBaseCnt < 8 && floatBaseCnt < 8) {
            return;
        }

        ASMFunction asmFunction = functionMap.get(function);
        List<Function.FParam> fParams = function.getFParams();
        List<Function.FParam> intParams = new ArrayList<>();
        List<Function.FParam> intWordParams = new ArrayList<>();
        List<Function.FParam> intDoubleParams = new ArrayList<>();
        List<Function.FParam> floatParams = new ArrayList<>();
        for (int i = intBaseCnt + floatBaseCnt; i < fParams.size(); i++) {
            Function.FParam fParam = fParams.get(i);
            switch (fParam.getType()) {
                case TInt _ -> intWordParams.add(fParam);
                case TPointer _ -> intDoubleParams.add(fParam);
                case TFloat _ -> floatParams.add(fParam);
                case null, default -> throw new RuntimeException("不支持的形参类型：" + fParam.getType());
            }
        }
        intParams.addAll(intDoubleParams);
        intParams.addAll(intWordParams);

        // 如果参数超过 8 个，则需要从栈中加载剩余的参数
        int offset = 0;
        for (Function.FParam fParam : intParams) {
            VirIReg virReg = (VirIReg) getValue(fParam);
            if (fParam.getType() instanceof TPointer) {
                genLoad(new ASMStackOffset(asmFunction, offset), regStore.getStackPtr(), firstBlock, virReg, true, false);
                offset += 8; // 指针类型参数占 8 字节
            } else if (fParam.getType() instanceof TInt) {
                genLoad(new ASMStackOffset(asmFunction, offset), regStore.getStackPtr(), firstBlock, virReg, false, false);
                offset += 4; // 整型或浮点型参数占 4 字节
            } else {
                throw new RuntimeException("不支持的形参类型：" + fParam.getType());
            }
        }
        for (Function.FParam fParam : floatParams) {
            VirFReg virReg = (VirFReg) getFValue(fParam, firstBlock, false);
            if (fParam.getType() instanceof TFloat) {
                genLoad(new ASMStackOffset(asmFunction, offset), regStore.getStackPtr(), firstBlock, virReg, false, true);
                offset += 4; // 整型或浮点型参数占 4 字节
            } else {
                throw new RuntimeException("不支持的形参类型：" + fParam.getType());
            }
        }
    }

    private int getMaxParamSize(Function function) {
        int maxParamSize = 0;
        for (BasicBlock blk : function.getBasicBlockList()) {
            for (Instruction instr : blk.getInstrList()) {
                if (instr instanceof CallInstr callInstr) {
                    if (function.isTailRecursive() && callInstr.getCallee() == function) {
                        continue; // 尾递归调用不需要计算参数大小
                    }
                    int paramSize = getParamSize(callInstr);
                    maxParamSize = Math.max(maxParamSize, paramSize);
                }
            }
        }
        return maxParamSize;
    }

    private int getParamSize(CallInstr callInstr) {
        List<Value> rParams = callInstr.getRParams();
        int paramSize = 0;
        int paramCount = rParams.size();
        //前8个参数直接放在寄存器中
        for (int i = 8; i < paramCount; i++) {
            Value param = rParams.get(i);
            if (param.getType() instanceof TPointer) {
                paramSize += 8;  // 指针类型参数占 8 字节
            } else if (param.getType() instanceof TInt || param.getType() instanceof TFloat) {
                paramSize += 4;  // 整型或浮点型参数占 4 字节
            } else {
                throw new RuntimeException("不支持的参数类型：" + param.getType());
            }
        }
        if ((paramSize & 7) != 0) {
            paramSize += 4; // 确保栈指针 8 字节对齐
        }
        return paramSize;
    }


    private void parseGlobalValues(IRModel irModel) {
        for (Symbol irGlbObj : irModel.getGlobalSymTab().getObjectList()) {
            if (irGlbObj.isAbandoned()) {
                continue;
            }

            Value initVal = irGlbObj.getInitVal();
            ASMGlobalObject newObj;
            boolean isFloat = irGlbObj.getType() instanceof TFloat;

            if (initVal instanceof StringLiteral strInitVal) {
                newObj = new ASMGlobalObject(irGlbObj.getIdent(), strInitVal.getContent(), isFloat);
            } else {
                List<ASMGlbInitVal> initList = new ArrayList<>();
                parseGlbInitVal(initVal, initList);
                newObj = new ASMGlobalObject(irGlbObj.getIdent(), initList, isFloat);
            }

            glbObjList.add(newObj);
            glbObjMap.put(irGlbObj.getPointer(), newObj);
        }
    }

    /**
     * 递归获取所有初始值，并且将它们转换成一维形式，遇到有值的就是 word，零初始化就是对应大小的 zero
     *
     * @param initValList 递归维护同一个initValList
     */
    private void parseGlbInitVal(Value initVal, List<ASMGlbInitVal> initValList) {
        switch (initVal) {
            case IntConst intConst -> initValList.add(new ASMGlbInitVal.ASMGlbWord(intConst.getConstVal().intValue()));
            case FloatConst floatConst -> {
                int transferValue = Float.floatToRawIntBits(floatConst.getConstVal().floatValue());
                initValList.add(new ASMGlbInitVal.ASMGlbWord(transferValue));
            }
            case ArrayInitVal.ArrayZeroInitVal zeroInitVal -> {
                TArray arrType = (TArray) zeroInitVal.getType();
                initValList.add(new ASMGlbInitVal.ASMGlbZero(arrType.getFullSize()));
            }
            case ArrayInitVal arrayInitVal -> {
                List<Value> initValues = arrayInitVal.getInitValues();
                for (Value innerInit : initValues) {
                    parseGlbInitVal(innerInit, initValList);
                }

                int rest = arrayInitVal.getLim() - initValues.size();
                if (rest > 0) {
                    Value defaultVal = arrayInitVal.getDefaultVal();
                    if (defaultVal instanceof ArrayInitVal.ArrayZeroInitVal) {
                        while (rest > 0) {
                            parseGlbInitVal(defaultVal, initValList);
                            rest--;
                        }
                    } else {
                        initValList.add(new ASMGlbInitVal.ASMGlbZero(rest));
                    }
                }
            }
            case null, default -> throw new RuntimeException("全局对象的初始值应该是确定的");
        }
    }

    /**
     * 将 imm 塞到新建的虚拟整数寄存器里
     */
    protected VirIReg immInt2reg(ASMImmediate imm, ASMBasicBlock newBlock) {
        VirIReg liReg = new VirIReg();
        genLi(imm, newBlock, liReg);
        return liReg;
    }

    protected abstract void parseIntBranch(CmpInstr cmpInstr, BranchInstr branchInstr, ASMBasicBlock newBlock);

    /**
     * 生成序言，独立出来是为了适应不同架构的指令
     */
    protected abstract void genPrologue(ASMFunction asmFunction, ASMBasicBlock firstBlock);

    protected abstract void genMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst);

    protected abstract void genFrozenMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst);

    protected abstract void genLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat);

    protected abstract void genLoad(ASMValue ptr, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat, boolean isFltLi);

    protected abstract void genStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat);

    protected abstract void genStore(ASMValue ptr, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat);

    protected abstract void genLa(ASMGlobalObject globalObject, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg);

    protected abstract void genFAdd(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genFSub(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genFMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genFDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genSub(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genRem(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genFtoi(Reg src, ASMBasicBlock block, Reg reg);

    protected abstract void genItof(Reg src, ASMBasicBlock block, Reg reg);

    protected abstract void genJump(ASMBasicBlock target, ASMBasicBlock parentBlock);

    protected abstract void genBeq(Reg operand1, Reg operand2, ASMBasicBlock target, ASMBasicBlock parentBlock);

    protected abstract void genJr(Reg target, ASMBasicBlock parentBlock);

    protected abstract void genJal(ASMFunction targetFunc, ASMBasicBlock parentBlock);

    protected abstract void genFNeg(ASMValue src, ASMBasicBlock parentBlock, Reg reg);

    protected abstract void genSll(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genSra(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genSrl(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genAnd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genAtomAdd(ASMValue ptr, ASMValue inc, ASMBasicBlock parentBB, Reg reg);

    protected abstract void genVecMoveFromSca(VirReg srcVal, int index, ASMBasicBlock parentBB, VirFReg dst);

    protected abstract void genVecMoveToSca(VirFReg srcVal, int index, ASMBasicBlock parentBB, VirReg dst);

    protected abstract void genVecStore(VirFReg valueToStore, Reg basicPointer, ASMBasicBlock parentBB);

    protected abstract void genVecLoad(Reg basicPointer, ASMBasicBlock parentBB, VirFReg reg);

    protected abstract void genVecDup(VirReg scalarValue, ASMBasicBlock parentBB, VirFReg reg);

    protected abstract void genVecMulAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg);

    protected abstract void genVecMove(VirFReg src, ASMBasicBlock parentBlock, VirFReg dst);

    protected abstract void genAddv(VirFReg srcVec, ASMBasicBlock parentBlock, VirFReg register);

    protected abstract void genBitCopy(Reg src, ASMBasicBlock block, Reg reg);  // 整数与浮点寄存器之间不考虑语义直接移动二进制位

    protected abstract void genVecAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg);

    protected abstract void genVecRegClear(ASMBasicBlock parentBB, VirFReg reg);
}