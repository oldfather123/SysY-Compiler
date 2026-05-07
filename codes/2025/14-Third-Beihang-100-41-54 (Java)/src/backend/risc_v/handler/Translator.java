package backend.risc_v.handler;

import backend.ir.entity.*;
import backend.ir.entity.insts.*;
import backend.risc_v.entity.*;
import backend.risc_v.entity.data.RiscVData;
import backend.risc_v.entity.data.RiscVNumberData;
import backend.risc_v.entity.data.RiscVStrData;
import backend.risc_v.entity.instrcution.*;
import backend.risc_v.entity.register.Register;
import backend.risc_v.entity.type.RiscVBasicType;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.IRBasicType;
import util.NumberConst;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.*;

import static backend.risc_v.entity.instrcution.ITypeInstruction.ITypeOperation;
import static backend.risc_v.entity.instrcution.ITypeInstruction.ITypeOperation.*;
import static backend.risc_v.entity.instrcution.RTypeInstruction.RTypeOperation;
import static backend.risc_v.entity.instrcution.RTypeInstruction.RTypeOperation.*;
import static backend.risc_v.entity.instrcution.STypeInstruction.STypeOperation;
import static backend.risc_v.entity.instrcution.STypeInstruction.STypeOperation.*;
import static java.lang.Math.abs;

public class Translator {

    private final RiscVProgram riscVProgram;
    private final StackManager stackManager;
    private final RegisterManager registerManager;

    private final HashSet<Value> globalVarSet;
    private final HashSet<Value> argumentSet;
    private final HashMap<String, RiscVBasicBlock> riscVBasicBlockMap;
    private RiscVFunction curRiscVFunction;
    private RiscVBasicBlock curRiscVBasicBlock;

    public Translator() {
        riscVProgram = new RiscVProgram();
        globalVarSet = new HashSet<>();
        argumentSet = new HashSet<>();
        stackManager = new StackManager();
        registerManager = new RegisterManager();
        riscVBasicBlockMap = new HashMap<>();
    }

    public void addMacro() {
        riscVProgram.addMacro(new RiscVMacro(SLE));
        riscVProgram.addMacro(new RiscVMacro(SGE));
        riscVProgram.addMacro(new RiscVMacro(SGT));
        riscVProgram.addMacro(new RiscVMacro(SEQ));
        riscVProgram.addMacro(new RiscVMacro(SNE));

        riscVProgram.addMacro(new RiscVMacro(FLE_S));
        riscVProgram.addMacro(new RiscVMacro(FGE_S));
        riscVProgram.addMacro(new RiscVMacro(FGT_S));
        riscVProgram.addMacro(new RiscVMacro(FNE_S));
    }

    public void translate(Program program) {
        addMacro();
        for (Value value : program.getUsees()) {
            switch (value) {
                case GlobalVar globalVar -> translateGlobalVar(globalVar);
                case Function function -> translateFunction(function);
                case Declare declare -> {
                    if (!declare.getName().equals("llvm.memset.p0i8.i64")) {
                        riscVProgram.addDeclare(new RiscVExtern(declare.getName()));
                    }
                }
                default -> throw new RuntimeException("不支持的Value类型: " + value.getClass().getName());
            }
        }
    }

    private RiscVBasicType IRBasicType2BasicType(IRBasicType irBasicType) {
        if (irBasicType == IRBasicType.FLOAT) {
            return RiscVBasicType.FLOAT;
        } else {
            return RiscVBasicType.INT;
        }
    }

    private RiscVData createGlobalVar(String name, ArrayValue initValue, RiscVBasicType basicType) {
        RiscVNumberData riscVNumberData = new RiscVNumberData(name, basicType);
        ArrayDeque<ArrayValue> arrayValueStack = new ArrayDeque<>();
        arrayValueStack.offer(initValue);
        while (!arrayValueStack.isEmpty()) {
            ArrayValue arrayValue = arrayValueStack.poll();
            for (Value value : arrayValue.arrayValueUseList) {
                if (value instanceof ArrayValue subArrayValue)
                    arrayValueStack.offer(subArrayValue);
                else
                    riscVNumberData.addData(((LiteralConst) value).getNumber());
            }
        }
        return riscVNumberData;
    }

    private void translateGlobalVar(GlobalVar globalVar) {
        RiscVData riscVDataVar;
        RiscVBasicType basicType = IRBasicType2BasicType(globalVar.getBasicType());
        String name = globalVar.getName();

        if (globalVar.isString()) {
            riscVDataVar = new RiscVStrData(name, globalVar.getUsee(0).getName());
        } else if (!globalVar.isArray()) {
            // 如果全局变量被初始化了，那就获得初始化的值，否则为0.
            Number val = globalVar.isInit() ? ((LiteralConst) globalVar.getUsee(0)).getNumber() : 0;
            riscVDataVar = new RiscVNumberData(name, val, basicType);
        } else if (!globalVar.getUsees().isEmpty()) {
            ArrayValue initValue = (ArrayValue) globalVar.getUsee(0);
            riscVDataVar = createGlobalVar(name, initValue, basicType);
        } else {
            int arraySize = 4 * globalVar.getDimSize();
            riscVDataVar = new RiscVNumberData(name, true, arraySize, basicType);
        }
        riscVProgram.addGlobalVar(riscVDataVar);
        globalVarSet.add(globalVar);
    }

    private void translateFunction(Function function) {
        stackManager.clear();
        argumentSet.clear();
        registerManager.clear();

        curRiscVFunction = new RiscVFunction(function.getName());
        curRiscVBasicBlock = new RiscVBasicBlock(function.getName());
        curRiscVFunction.addBasicBlock(curRiscVBasicBlock);
        splitBasicBlock(function.getSuperBlock());

        for (Value argument : function.getArguments().reversed()) {
            argumentSet.add(argument);
            stackManager.addArgument(argument);
        }

        if (function.getName().equals("main"))
            riscVProgram.setMainFunction(curRiscVFunction);
        else
            riscVProgram.addFunction(curRiscVFunction);

        stackManager.allocate(new AllocateInst(function.getName(), IRBasicType.I64, false, false, null, null));
        addSTypeInstruction(SD, registerManager.spRegister(), registerManager.raRegister(), -8);
        translateBasicBlock(function.getSuperBlock());
    }

    private void translateBasicBlock(BasicBlock basicBlock) {
        if (riscVBasicBlockMap.containsKey(curRiscVFunction.getName() + basicBlock.getName())) {
            registerManager.storeAll();
            curRiscVBasicBlock = riscVBasicBlockMap.get(curRiscVFunction.getName() + basicBlock.getName());
            curRiscVFunction.addBasicBlock(curRiscVBasicBlock);
        }

        for (Value value : basicBlock.getUsees()) {
            switch (value) {
                case Inst inst -> translateInst(inst);
                case BasicBlock childBlock -> translateBasicBlock(childBlock);
                case null -> throw new RuntimeException("Value is null");
                default -> throw new RuntimeException("不支持的Value类型: " + value.getClass().getName());
            }
        }
    }

    public void transLoadInst(LoadInst loadInst) {
        Value address = loadInst.getUsee(0);
        Register loadInstRegister = registerManager.getRegister(loadInst);
        ITypeOperation operation = getLoadOp(loadInst);
        if (address instanceof GlobalVar) {
            LaInstruction instruction = new LaInstruction(registerManager.getRegister(address), address.getName());
            curRiscVBasicBlock.addInstruction(instruction);
            addITypeInstruction(operation, loadInstRegister, registerManager.getRegister(address), 0);
        } else {
            if (address.isPointer()) {
                Register addressRegister = loadRegister(address);
                addITypeInstruction(operation, loadInstRegister, addressRegister, 0);
            } else {
                int memoryPlace = stackManager.getAddress(address);
                addITypeInstruction(operation, loadInstRegister, registerManager.spRegister(), memoryPlace);
            }
        }
        storeRegister(loadInst);
    }

    //TODO: 完善浮点立即数处理
    public void transBinOpInst(BinOpInst binOpInst) {
        Value operator1 = binOpInst.getUsee(0), operator2 = binOpInst.getUsee(1);
        Register reg1 = loadRegister(operator1);
        Register reg2 = loadRegister(operator2);
        Register reg3 = registerManager.getRegister(binOpInst);
        RTypeOperation operation = null;
        if (!operator1.isFloat() && !operator2.isFloat()) {
            operation = i32OpType2RTypeOp(binOpInst.opType);
        } else if (operator1.isFloat() && operator2.isFloat()) {
            operation = floatOpType2RTypeOp(binOpInst.opType);
        }
        RTypeInstruction instruction = new RTypeInstruction(operation, reg3, reg1, reg2);
        curRiscVBasicBlock.addInstruction(instruction);
        storeRegister(binOpInst);
    }

    public void transCmpInst(CmpInst cmpInst) {
        Value operator1 = cmpInst.getUsee(0), operator2 = cmpInst.getUsee(1);
        Register reg1 = loadRegister(operator1);
        Register reg2 = loadRegister(operator2);
        Register reg3 = registerManager.getRegister(cmpInst);
        RTypeOperation operation = null;
        if (operator1.isFloat() && operator2.isFloat()) {
            operation = floatOpType2RTypeOp(cmpInst.opType);
        } else if (!operator1.isFloat() && !operator2.isFloat()) {
            operation = i32OpType2RTypeOp(cmpInst.opType);
        }
        RTypeInstruction instruction = new RTypeInstruction(operation, reg3, reg1, reg2);
        curRiscVBasicBlock.addInstruction(instruction);
        storeRegister(cmpInst);
    }

    public BinOpInst createBinOpInst(Value value1, Value value2, TokenType tokenType) {
        Register reg2 = loadRegister(value2);
        Register reg1 = loadRegister(value1);

        //Warning !! 以下语句可能产生问题
        BinOpInst binOpInst = new BinOpInst("Bin Op Inst", IRBasicType.I32, null, null);
        Register reg3 = registerManager.getRegister(binOpInst);

        RTypeOperation operation = i32OpType2RTypeOp(tokenType);
        RTypeInstruction inst = new RTypeInstruction(operation, reg3, reg1, reg2);
        curRiscVBasicBlock.addInstruction(inst);
        storeRegister(binOpInst);
        return binOpInst;
    }

    private void transCallInst(CallInst callInst) {
        if (callInst.getUsee(0).getName().equals("llvm.memset.p0i8.i64")) {
            Value array = ((User) callInst.getUsee(1)).getUsee(0);
            int addr = stackManager.getAddress(array);
            int baseByte = array.getBasicByte();
            int totalBytes = array.getTotalBytes();
            for (int i = 0; i < totalBytes; i += baseByte) {
                addSTypeInstruction(SW, registerManager.spRegister(), registerManager.zeroRegister(), addr + i);
            }
            return;
        }
        for (int i = 1; i < callInst.getUsees().size(); i++) {
            registerManager.storeAll();
            Value param = callInst.getUsee(i);
            stackManager.addParameter(param);

            if (param.isPointer()) {
                int argRegister = registerManager.addIntParameter();
                Register paramRegister = registerManager.getTmpRegister(argRegister, true);
                addITypeInstruction(LD, paramRegister, registerManager.spRegister(), stackManager.getAddress(param));
                addSTypeInstruction(SD, registerManager.spRegister(), paramRegister, -stackManager.getStackTop());
            } else if (!param.isFloat()) {
                int argRegister = registerManager.addIntParameter();
                Register paramRegister = registerManager.getTmpRegister(argRegister, true);
                if (param instanceof LiteralConst literalConst)
                    curRiscVBasicBlock.addInstruction(new LiInstruction(paramRegister, literalConst.getIntValue()));
                else
                    addITypeInstruction(LW, paramRegister, registerManager.spRegister(),
                            stackManager.getAddress(param));

                addSTypeInstruction(SW, registerManager.spRegister(), paramRegister, -stackManager.getStackTop());
            } else {
                int argRegister = registerManager.addFloatParameter();
                if (param instanceof LiteralConst literalConst) {
                    Register paramRegister = registerManager.getTmpRegister(argRegister, true);
                    curRiscVBasicBlock.addInstruction(
                            new LiInstruction(paramRegister, NumberConst.decimalToHexBinary(literalConst.getName())));
                    addSTypeInstruction(SW, registerManager.spRegister(), paramRegister, -stackManager.getStackTop());
                } else {
                    Register paramRegister = registerManager.getTmpRegister(argRegister, false);
                    addITypeInstruction(FLW, paramRegister, registerManager.spRegister(),
                            stackManager.getAddress(param));
                    addSTypeInstruction(FSW, registerManager.spRegister(), paramRegister, -stackManager.getStackTop());
                }
            }
        }
        addITypeInstruction(ADDI, registerManager.spRegister(), registerManager.spRegister(),
                -stackManager.getStackTop());
        curRiscVBasicBlock.addInstruction(new CallInstruction(callInst.getUsee(0).getName()));
        registerManager.clear();
        if (!callInst.isVoid())
            registerManager.mapReturnValue(callInst);
        addITypeInstruction(ADDI, registerManager.spRegister(), registerManager.spRegister(),
                stackManager.getStackTop());
        stackManager.removeParameter();
        registerManager.removeParameter();

        if (!callInst.isVoid())
            storeRegister(callInst);
    }

    public void transGetElemPtrInst(GetElemPtrInst getElemPtrInst) {
        Value arrayValue = getElemPtrInst.getUsee(0);
        List<Value> indexValueList = getElemPtrInst.getIndexList();
        List<Integer> dimList = arrayValue.getFullDim();


        int sumSpace = 4;
        for (Integer dim : dimList) {
            sumSpace *= abs(dim);
        }
        Value offsetValue = new LiteralConst(0, IRBasicType.I32);
        LiteralConst sumSpaceValue = new LiteralConst(sumSpace, IRBasicType.I32);

        int idxSize = indexValueList.size();
        for (int i = 0; i < idxSize; i++) {
            if (!(indexValueList.get(i) instanceof LiteralConst lc && lc.isInt() && lc.getIntValue() == 0)) {
                BinOpInst curOffset = createBinOpInst(indexValueList.get(i), sumSpaceValue, TokenType.MUL);
                offsetValue = createBinOpInst(offsetValue, curOffset, TokenType.ADD);
            }
            if (i != idxSize - 1) {
                sumSpace /= abs(dimList.get(i));
                sumSpaceValue = new LiteralConst(sumSpace, IRBasicType.I32);
            }
        }

        Register reg1, reg2 = loadRegister(offsetValue), reg3 = registerManager.getRegister(getElemPtrInst);
        // 获取基地址
        if (globalVarSet.contains(arrayValue)) {
            reg1 = registerManager.getRegister(arrayValue);
            curRiscVBasicBlock.addInstruction(new LaInstruction(reg1, arrayValue.getName()));
        } else if (argumentSet.contains(arrayValue)) {
            reg1 = loadRegister(arrayValue);
        } else {
            int arrayMemoryPlace = stackManager.getAddress(arrayValue);
            LiteralConst immValue = new LiteralConst(Integer.toString(arrayMemoryPlace));
            reg1 = registerManager.getRegister(immValue);
            addITypeInstruction(ADDI, reg1, registerManager.spRegister(), arrayMemoryPlace);
        }

        RTypeInstruction addInstruction = new RTypeInstruction(RTypeInstruction.RTypeOperation.ADD, reg3, reg1, reg2);
        curRiscVBasicBlock.addInstruction(addInstruction);
        storeRegister(getElemPtrInst);
    }

    private void transRetInst(RetInst retInst) {
        if (!retInst.isVoid()) {
            Value retValue = retInst.getUsee(0);
            if (retValue instanceof LiteralConst lc) {
                if (lc.isInt()) {
                    curRiscVBasicBlock.addInstruction(
                            new LiInstruction(registerManager.getTmpRegister(10, true), lc.getIntValue()));
                } else {
                    curRiscVBasicBlock.addInstruction(new LiInstruction(registerManager.getTmpRegister(10, true),
                            NumberConst.decimalToHexBinary(lc.getName())));
                    curRiscVBasicBlock.addInstruction(new RTypeInstruction(RTypeInstruction.RTypeOperation.FMV_S_X,
                            registerManager.getTmpRegister(10, false), registerManager.getTmpRegister(10, true)));
                }
            } else {
                ITypeOperation operation = getLoadOp(retInst);
                addITypeInstruction(operation, registerManager.getTmpRegister(10, retInst.isInt()),
                        registerManager.spRegister(), stackManager.getAddress(retValue));
            }
        }

        addITypeInstruction(LD, registerManager.raRegister(), registerManager.spRegister(), -8);
        curRiscVBasicBlock.addInstruction(new RetInstruction());
    }

    private void transStoreInst(StoreInst storeInst) {
        Register reg2;
        Value source = storeInst.getUsee(0);
        Value destination = storeInst.getUsee(1);
        reg2 = loadRegister(source);
        STypeOperation operation = getStoreOp(source);
        if (destination.isPointer()) {
            addSTypeInstruction(operation, loadRegister(destination), reg2, 0);
        } else if (globalVarSet.contains(destination)) {
            Register globalRegister = registerManager.getRegister(destination);
            curRiscVBasicBlock.addInstruction(new LaInstruction(globalRegister, destination.getName()));
            addSTypeInstruction(operation, globalRegister, reg2, 0);
        } else {
            addSTypeInstruction(operation, registerManager.spRegister(), reg2, stackManager.getAddress(destination));
        }
    }

    private void transTransInst(TransInst transInst) {
        Register rd = registerManager.getRegister(transInst);
        Value rawValue = transInst.getUsee(0);
        if (transInst.getBasicType() == IRBasicType.I32) {
            if (rawValue.getBasicType() == IRBasicType.I1) {
                if (rawValue instanceof LiteralConst literalConst) {
                    curRiscVBasicBlock.addInstruction(new LiInstruction(rd, Integer.parseInt(literalConst.getName())));
                } else {
                    addITypeInstruction(LW, rd, registerManager.spRegister(), stackManager.getAddress(rawValue));
                }
            } else {
                if (rawValue instanceof LiteralConst literalConst) {
                    curRiscVBasicBlock.addInstruction(
                            new LiInstruction(rd, Integer.parseInt(literalConst.getName().split("\\.")[0])));
                } else {
                    Register rs1 = loadRegister(rawValue);
                    curRiscVBasicBlock.addInstruction(new RTypeInstruction(RTypeOperation.FCVT_W_S, rd, rs1));
                }
            }
        } else if (transInst.getBasicType() == IRBasicType.FLOAT) {
            Register rs1;
            if (rawValue instanceof LiteralConst literalConst) {
                rs1 = registerManager.getRegister(literalConst);
                curRiscVBasicBlock.addInstruction(new LiInstruction(rs1, literalConst.getIntValue()));
            } else {
                rs1 = loadRegister(rawValue);
            }
            curRiscVBasicBlock.addInstruction(new RTypeInstruction(RTypeOperation.FCVT_S_W, rd, rs1));
        } else if (transInst.getBasicType() == IRBasicType.I1) {
            if (rawValue.getBasicType() == IRBasicType.I1) {
                if (rawValue instanceof LiteralConst literalConst) {
                    curRiscVBasicBlock.addInstruction(new LiInstruction(rd, Integer.parseInt(literalConst.getName())));
                } else {
                    addITypeInstruction(LW, rd, registerManager.spRegister(), stackManager.getAddress(rawValue));
                }
            } else {
                if (rawValue instanceof LiteralConst literalConst) {
                    if (literalConst.getFloatValue() < 1e-6)
                        curRiscVBasicBlock.addInstruction(new LiInstruction(rd, 0));
                    else
                        curRiscVBasicBlock.addInstruction(new LiInstruction(rd, 1));
                } else {
                    Register rs1 = loadRegister(rawValue);
                    Value zero = new LiteralConst(0, IRBasicType.FLOAT);
                    Register zeroRegister = registerManager.getRegister(zero);
                    curRiscVBasicBlock.addInstruction(new RTypeInstruction(RTypeInstruction.RTypeOperation.FMV_S_X,
                            registerManager.zeroRegister(), zeroRegister));
                    curRiscVBasicBlock.addInstruction(new RTypeInstruction(FEQ_S, rd, rs1, zeroRegister));
                    addITypeInstruction(XORI, rd, rd, 1);
                }
            }
        }
        storeRegister(transInst);
    }

    /**
     * 将move指令转成risc-v
     * move指令包含源寄存器source和目标寄存器target，LLVM中表示 move %target %source, 从source移动至target
     * 或许可以考虑用source 加一个立即数0来代替?
     */
    private void transMoveInst(MoveInst moveInst) {
        Value target = moveInst.getTarget(), source = moveInst.getSource();
        Register targetRegister = registerManager.getRegister(target);
        Register sourceRegister = loadRegister(source);
        addITypeInstruction(ADDI, targetRegister, sourceRegister, 0);
    }

    private void translateInst(Inst inst) {
        switch (inst) {
            case AllocateInst allocateInst -> stackManager.allocate(allocateInst);
            case BinOpInst binOpInst -> transBinOpInst(binOpInst);
            case BranchInst branchInst -> {
                Register rs1 = loadRegister(branchInst.getUsee(0));
                curRiscVBasicBlock.addInstruction(
                        new BTypeInstruction(BTypeInstruction.BTypeOperation.BNE, rs1, registerManager.zeroRegister(),
                                curRiscVFunction.getName() + branchInst.getUsee(1).getName()));
                curRiscVBasicBlock.addInstruction(new JTypeInstruction(JTypeInstruction.JTypeOperation.J,
                        curRiscVFunction.getName() + branchInst.getUsee(2).getName()));
            }
            case CallInst callInst -> transCallInst(callInst);
            case CmpInst cmpInst -> transCmpInst(cmpInst);
            case LoadInst loadInst -> transLoadInst(loadInst);
            case GetElemPtrInst getElemPtrInst -> transGetElemPtrInst(getElemPtrInst);
            case PCopyInst pCopyInst -> System.out.println("PCopyInst");
            case PHIInst phiInst -> System.out.println("PHIInst");
            case MoveInst moveInst -> transMoveInst(moveInst);
            case RetInst retInst -> transRetInst(retInst);
            case StoreInst storeInst -> transStoreInst(storeInst);
            case TransInst transInst -> transTransInst(transInst);
            case UCJumpInst ucJumpInst -> curRiscVBasicBlock.addInstruction(
                    new JTypeInstruction(JTypeInstruction.JTypeOperation.J,
                            curRiscVFunction.getName() + ucJumpInst.getUsee(0).getName()));
            case null -> throw new RuntimeException("Inst is null");
            default -> throw new RuntimeException("不支持的Inst类型: " + inst.getClass().getName());
        }
    }

    private void splitBasicBlock(BasicBlock basicBlock) {
        for (Value value : basicBlock.getUsees()) {
            if (value instanceof UCJumpInst ucJumpInst) {
                riscVBasicBlockMap.putIfAbsent(curRiscVFunction.getName() + ucJumpInst.getUsee(0).getName(),
                        new RiscVBasicBlock(curRiscVFunction.getName() + ucJumpInst.getUsee(0).getName()));
            } else if (value instanceof BranchInst branchInst) {
                riscVBasicBlockMap.putIfAbsent(curRiscVFunction.getName() + branchInst.getUsee(1).getName(),
                        new RiscVBasicBlock(curRiscVFunction.getName() + branchInst.getUsee(1).getName()));
                riscVBasicBlockMap.putIfAbsent(curRiscVFunction.getName() + branchInst.getUsee(2).getName(),
                        new RiscVBasicBlock(curRiscVFunction.getName() + branchInst.getUsee(2).getName()));
            } else if (value instanceof BasicBlock childBlock) {
                splitBasicBlock(childBlock);
            }
        }

    }

    private ITypeOperation getLoadOp(Value value) {
        if (value.isPointer()) {
            return LD;
        } else if (value.isFloat()) {
            return FLW;
        } else {
            return LW;
        }
    }

//    public Register parseLoadOperation(Value value) {
//        Register loadRegister = registerManager.getRegister(value);
//        int memoryPlace = memoryManager.getAddress(value);
//        ITypeOperation operation = getLoadOp(value);
//        addITypeInstruction(operation, loadRegister, registerManager.spRegister(), memoryPlace);
//        return loadRegister;
//    }

    private void addITypeInstruction(ITypeInstruction.ITypeOperation operation, Register rd, Register rs1,
                                     int immediate) {
        if (immediate >= -2048 && immediate <= 2047) {
            curRiscVBasicBlock.addInstruction(new ITypeInstruction(operation, rd, rs1, immediate));
        } else {
            LiteralConst immValue = new LiteralConst(immediate, IRBasicType.I32);
            Register immRegister = registerManager.getRegister(immValue);
            curRiscVBasicBlock.addInstruction(new LiInstruction(immRegister, immediate));
            if (operation == LW || operation == FLW || operation == LD) {
                curRiscVBasicBlock.addInstruction(new RTypeInstruction(ADD, immRegister, rs1, immRegister));
                curRiscVBasicBlock.addInstruction(new ITypeInstruction(operation, rd, immRegister, 0));
            } else {
                curRiscVBasicBlock.addInstruction(
                        new RTypeInstruction(operation.toRTypeOperation(), rd, rs1, immRegister));
            }
        }
    }

    private void addSTypeInstruction(STypeInstruction.STypeOperation operation, Register rs1, Register rs2,
                                     int immediate) {
        if (immediate < -2048 || immediate > 2047) {
            LiteralConst immValue = new LiteralConst(immediate, IRBasicType.I32);
            Register addressRegister = registerManager.getRegister(immValue);
            curRiscVBasicBlock.addInstruction(new LiInstruction(addressRegister, immediate));
            curRiscVBasicBlock.addInstruction(new RTypeInstruction(ADD, addressRegister, rs1, addressRegister));
            curRiscVBasicBlock.addInstruction(new STypeInstruction(operation, addressRegister, rs2, 0));
        } else {
            curRiscVBasicBlock.addInstruction(new STypeInstruction(operation, rs1, rs2, immediate));
        }
    }

    private void addLiInstruction(Register reg, LiteralConst lc) {
        if (lc.isFloat()) {
            Register constReg = registerManager.getTmpRegister(true);
            LiInstruction li = new LiInstruction(constReg, NumberConst.decimalToHexBinary(lc.getName()));
            RTypeInstruction fmvInstruction =
                    new RTypeInstruction(RTypeInstruction.RTypeOperation.FMV_S_X, reg, constReg);
            curRiscVBasicBlock.addInstruction(li);
            curRiscVBasicBlock.addInstruction(fmvInstruction);
        } else {
            LiInstruction li = new LiInstruction(reg, lc.getIntValue());
            curRiscVBasicBlock.addInstruction(li);
        }
    }

    private Register loadRegister(Value value) {
        Register reg;
        if (value instanceof LiteralConst lc) {
            reg = registerManager.getRegister(lc);
            addLiInstruction(reg, lc);
        } else {
            reg = registerManager.getRegister(value);
            int memoryPlace = stackManager.getAddress(value);
            ITypeOperation operation = getLoadOp(value);
            addITypeInstruction(operation, reg, registerManager.spRegister(), memoryPlace);
        }
        return reg;
    }

    private STypeOperation getStoreOp(Value value) {
        if (value.isPointer()) {
            return SD;
        } else if (value.isFloat()) {
            return FSW;
        } else {
            return SW;
        }
    }

    private void storeRegister(Value source) {
        STypeOperation operation = getStoreOp(source);
        addSTypeInstruction(operation, registerManager.spRegister(), registerManager.getRegister(source),
                stackManager.getAddress(source));
    }

    public void printRiscV(BufferedWriter out) throws IOException {
        riscVProgram.printRiscV(out);
    }

    // TODO: BinOpInst和CmpInst的合并
    //  假设RISC_V支持更多的比较类型（不只是SLT），那么CmpInst也可以使用相同的逻辑进行书写。否则因为
    //  CmpInst必须调整Register的先后顺序，就很难进行统一的重构。
    //  我这里加上了相关的内容，假装RISC_V是支持的。
    //  另外，我提一句，LLVM IR里面的比较语句是 **区分了整数和浮点数的**， 其中SLT表示的是 整数的小于。
    //  我估计RISC_V也有类似的要求，但是我们现在的CmpInst在翻译的时候，使用的都是整数的比较关系，我觉得
    //  BUG可能就出现在这里。
    private RTypeOperation i32OpType2RTypeOp(TokenType tokenType) {
        if (tokenType == TokenType.ADD) {
            return ADD;
        } else if (tokenType == TokenType.SUB) {
            return SUB;
        } else if (tokenType == TokenType.MUL) {
            return MUL;
        } else if (tokenType == TokenType.DIV) {
            return DIV;
        } else if (tokenType == TokenType.MOD) {
            return REM;
        } else if (tokenType == TokenType.XOR) {
            return XOR;
        } else if (tokenType == TokenType.LT) {
            return SLT;
        } else if (tokenType == TokenType.LE) {
            return SLE;
        } else if (tokenType == TokenType.GT) {
            return SGT;
        } else if (tokenType == TokenType.GE) {
            return SGE;
        } else if (tokenType == TokenType.EQ) {
            return SEQ;
        } else if (tokenType == TokenType.NE) {
            return SNE;
        }
        throw new RuntimeException("Unknown TokenType: " + tokenType.toString());
    }

    private RTypeOperation floatOpType2RTypeOp(TokenType tokenType) {
        if (tokenType == TokenType.ADD) {
            return FADD_S;
        } else if (tokenType == TokenType.SUB) {
            return FSUB_S;
        } else if (tokenType == TokenType.MUL) {
            return FMUL_S;
        } else if (tokenType == TokenType.DIV) {
            return FDIV_S;
        } else if (tokenType == TokenType.LT) {
            return FLT_S;
        } else if (tokenType == TokenType.LE) {
            return FLE_S;
        } else if (tokenType == TokenType.GT) {
            return FGT_S;
        } else if (tokenType == TokenType.GE) {
            return FGE_S;
        } else if (tokenType == TokenType.EQ) {
            return FEQ_S;
        } else if (tokenType == TokenType.NE) {
            return FNE_S;
        }
        throw new RuntimeException("Unknown TokenType: " + tokenType.toString());
    }
}
