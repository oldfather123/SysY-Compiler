package backend;

import IR.IRConst;
import IR.IRInstruction.*;
import IR.IRModule;
import IR.IRType.*;
import IR.IRValueRef.*;
import backend.RISCVCode.*;
import backend.RISCVCode.RISCVInstruction.*;
import backend.RISCVCode.RISCVInstruction.RISCVCompare.*;
import backend.optimizer.OptimizerFactory;

import java.io.FileWriter;
import java.io.IOException;
import java.security.interfaces.ECKey;
import java.util.*;

import static IR.IRInstruction.IRInstruction.*;
import static IR.IRModule.*;
import static IR.IRModule.IRConstIntGetSExtValue;
import static IR.IRValueRef.IRBaseBlockRef.IRGetFirstInstruction;
import static IR.IRValueRef.IRBaseBlockRef.IRGetNextInstruction;
import static IR.IRValueRef.IRFunctionBlockRef.IRGetFirstBaseBlock;
import static IR.IRValueRef.IRFunctionBlockRef.IRGetNextBaseBlock;

/**
 * 将IRModule翻译为RISCVCode
 * 例如
 block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
 * 为向RISCVBlock中增加一个li指令
 */
public class RISCVBuilder {
    private IRModule module;
    private final StringBuilder outputStringBuilder = new StringBuilder();
    private final RegisterAllocator registerAllocator;
    private RISCVCode riscvCode;
    private Set<String> GEPPointers;


    private LinkedHashMap<String, String> paramsRegStoreStack;


    private int arrayLength; // 仅供截取所用
    private OptimizerFactory optimizerFactory;

    int phiIndex = 0;

    boolean blockEnd = false;

    public RISCVBuilder(IRModule module, RegisterAllocator registerAllocator) {
        this.module = module;
        this.registerAllocator = registerAllocator;
        this.riscvCode = new RISCVCode();
        this.optimizerFactory = new OptimizerFactory();
    }

    public void generateASM(String outputPath) {

        // 处理全局变量
        for (IRGlobalRegRef global = IRGetFirstGlobal(module); global != null; global = IRGetNextGlobal(module, global)) {
            String name = global.getIdentity();
            IRValueRef initializer = IRGetInitializer(module, global);
            if (initializer != null) {
                if (((IRPointerType)global.getType()).getBaseType() instanceof IRInt32Type) {
                    int value = (int) IRConstIntGetSExtValue(initializer);
                    riscvCode.addGlobalVar(name, value);
                } else if((((IRPointerType)global.getType()).getBaseType() instanceof IRFloatType) ) {
                    float value = ((IRConstFloatRef) initializer).getValue();
                    riscvCode.addGlobalVar(name, value);
                }else {
                    //todo:数组
                    if(((IRArrayRef)initializer).isAllZero()){
                        //全为0
                        int size = 1;
                        IRType array = ((IRPointerType)global.getType()).getBaseType();
                        while(array instanceof IRArrayType){
                            size *= ((IRArrayType) array).getLength();
                            array = ((IRArrayType) array).getBaseType();
                        }
                        riscvCode.addGlobalAllZeroArray(name, size);
                    }else{
                        //不全是0
                        if(!((IRArrayRef) initializer).getInitFloatValue().isEmpty()){
                            List<Float> value = ((IRArrayRef)initializer).getInitFloatValue();
                            riscvCode.addGlobalFloatArrayVar(name, value);
                        }else{
                            List<Integer> value = ((IRArrayRef)initializer).getInitIntValue();
                            riscvCode.addGlobalIntArrayVar(name, value);
                        }
                    }
                }
            }
            else {
                if (((IRPointerType)global.getType()).getBaseType() instanceof IRInt32Type) {
                    int value = 0;
                    riscvCode.addGlobalVar(name, value);
                } else if((((IRPointerType)global.getType()).getBaseType() instanceof IRFloatType) ){
                    float value = 0;
                    riscvCode.addGlobalVar(name, value);
                }//数组一定有初始化值
            }
        }

        for (IRFunctionBlockRef functionBlockRef = IRGetFirstFunction(module); functionBlockRef != null; functionBlockRef = IRGetNextFunction(module, functionBlockRef)) {
            paramsRegStoreStack = new LinkedHashMap<>();
            phiIndex = 0;

            boolean hasCall = false;

            GEPPointers = new HashSet<>();
            // 为参数分配寄存器
            LinkedHashMap<String, String> paramsAndRegister = new LinkedHashMap<>();
            List<IRValueRef> params = functionBlockRef.getParams();
            int floatIndex = 0;
            int intIndex = 0;
            int paramStackSize = 0;
            int paramRegStackSize = 0;
            for(IRValueRef param : params) {
                if (param.getType() instanceof IRInt32Type) {
                    if (intIndex > 7) {
                        paramsAndRegister.put(param.getText(), Integer.toString(paramStackSize));
                        paramStackSize += 8;
                    }
                    else{
                        paramsAndRegister.put(param.getText(), "a" + intIndex);
                        paramsRegStoreStack.put("a" + intIndex, Integer.toString(paramRegStackSize));
                        paramRegStackSize += 8;
                        intIndex++;
                    }
                }
                if (param.getType() instanceof IRFloatType) {
                    if (floatIndex > 7) {
                        paramsAndRegister.put(param.getText(), Integer.toString(paramStackSize));
                        paramStackSize += 8;
                    }
                    else {
                        paramsAndRegister.put(param.getText(), "fa" + floatIndex);
                        paramsRegStoreStack.put("fa" + floatIndex, Integer.toString(paramRegStackSize));
                        paramRegStackSize += 8;
                        floatIndex++;
                    }
                }
                if (param.getType() instanceof IRPointerType) {
                    if (intIndex > 7) {
                        paramsAndRegister.put(param.getText(), Integer.toString(paramStackSize));
                        paramStackSize += 8;
                    }
                    else{
                        paramsAndRegister.put(param.getText(), "a" + intIndex);
                        paramsRegStoreStack.put("a" + intIndex, Integer.toString(paramRegStackSize));
                        paramRegStackSize += 8;
                        intIndex++;
                    }
                }
            }

            RISCVFunction riscvFunction = new RISCVFunction(functionBlockRef.getFunctionName(), paramsAndRegister, paramStackSize);
            riscvCode.addFunction(riscvFunction);
            registerAllocator.setFunction(functionBlockRef);
            registerAllocator.setModule(module);
            int paramsRegStoreStackSize = intIndex * 8 + floatIndex * 8;
            int alignedStackSize = registerAllocator.allocate() * 2 + 8 + paramsRegStoreStackSize; // lost copy
            for (String paramsReg : paramsRegStoreStack.keySet()) {
                String location = Integer.toString(Integer.parseInt(paramsRegStoreStack.get(paramsReg)) + alignedStackSize -  paramsRegStoreStackSize);
                paramsRegStoreStack.put(paramsReg, location);
            }
            int phiTmpStackSize = calculatePhiTmpStackSize(functionBlockRef); // lost copy
            riscvFunction.setStackSize(alignedStackSize + phiTmpStackSize);
            for (IRBaseBlockRef block = IRGetFirstBaseBlock(functionBlockRef); block != null; block = IRGetNextBaseBlock(functionBlockRef, block)) {
                for (IRInstruction inst = IRGetFirstInstruction(block); inst != null; inst = IRGetNextInstruction(block, inst)) {
                    String opcode = IRGetInstructionOpcode(inst);
                    if (opcode.equals("IRCall")) {
                        hasCall = true;
                        break;
                    }
                }
            }
            for (IRBaseBlockRef block = IRGetFirstBaseBlock(functionBlockRef); block != null; block = IRGetNextBaseBlock(functionBlockRef, block)) {
                blockEnd = false;
                RISCVBlock riscvBlock = new RISCVBlock(block.getLabel());

                riscvFunction.addBlock(riscvBlock);
                if (block.equals(IRGetFirstBaseBlock(functionBlockRef)) && hasCall) {
                    String raLocation = Integer.toString(Integer.parseInt(registerAllocator.getRegister("ra").get(0)) * 2);
                    riscvBlock.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ra"), new RISCVOperand(OperandType.stackRoom, raLocation + "(sp)")));
                    riscvFunction.setStackTopOffset("0");
                }
                for (IRInstruction inst = IRGetFirstInstruction(block); inst != null; inst = IRGetNextInstruction(block, inst)) {
                    String opcode = IRGetInstructionOpcode(inst);
                    switch (opcode) {
                        case "IRAllocate":
                            handleAllocateInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRStore":
                            handleStoreInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRLoad":
                            handleLoadInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRAdd":
                        case "IRFAdd":
                        case "IRSub":
                        case "IRFSub":
                        case "IRMul":
                        case "IRFMul":
                        case "IRSDiv":
                        case "IRFDiv":
                        case "IRSRem":
                            handleBinaryOperation(inst, opcode, riscvBlock, riscvFunction);
                            break;
                        case "IREq":
                        case "IRNe":
                        case "IRUne":
                        case "IRUeq":
                        case "IRUgt":
                        case "IRUge":
                        case "IRUlt":
                        case "IRUle":
                        case "IRSgt":
                        case "IRSge":
                        case "IRSlt":
                        case "IRSle":
                            handleICmpInstruction(inst, opcode, riscvBlock, riscvFunction);
                            break;
                        case "IRXor":
                            handleXorInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRZExt":
                            handleZExtInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRRet":
                            blockEnd = true;
                            if (hasCall) {
                                String raLocation = Integer.toString(Integer.parseInt(registerAllocator.getRegister("ra").get(0)) * 2);
                                riscvBlock.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ra"), new RISCVOperand(OperandType.stackRoom, raLocation + "(sp)")));
                            }
                            handleRetInstruction(inst, riscvBlock, riscvFunction, functionBlockRef);
                            break;
                        case "IRCall":
                            handleCallInstruction(inst, riscvBlock, riscvFunction, functionBlockRef);
                            break;
                        case "IRBr":
                            blockEnd = true;
                            handleBrInstruction(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRGetElementPointer" :
                            handleGetElementPointer(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRTypeTransfer":
                            handleTypeTransfer(inst, riscvBlock, riscvFunction);
                            break;
                        case "IRPhi":
                            handlePhiInstitution(inst, riscvBlock, riscvFunction, alignedStackSize);
                        default:
                            break;
                    }
                    if (blockEnd)
                        break;
                }
            }
            phiAddInSrcBlock(riscvFunction, alignedStackSize);
        }

        for (RISCVFunction function : riscvCode.getFunctions()) {
            optimizerFactory.optimize(function);
        }
            riscvCode.generateRISCVCode(outputPath);
    }

    private void handleAllocateInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        IRType allocationType = ((IRPointerType)inst.getOperands().get(0).getType()).getBaseType();
        if (allocationType instanceof IRArrayType) {
            String varName = IRGetValueName(inst.getOperands().get(0));
            String str = registerAllocator.getRegister(varName).get(0);
            if (str.startsWith("-"))
                return;
            int arrayLength = 1;
            boolean flag = ((IRArrayType) allocationType).isNeedInit();
            while (allocationType instanceof IRArrayType) {
                arrayLength = arrayLength * ((IRArrayType) allocationType).getLength();
                allocationType = ((IRArrayType)allocationType).getBaseType();
            }
            int nowFrame = Integer.parseInt(str) * 2 - arrayLength * 8;
            // riscvFunction.setStackTopOffset(Integer.toString(nowFrame + 8 * arrayLength - 8));
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"),new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(nowFrame)), "addi"));
            if(flag) {
                for (int i = 0; i < arrayLength / 2 + 1; i++) {
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "zero"), new RISCVOperand(OperandType.stackRoom, Integer.toString(nowFrame + i * 8) + "(sp)")));
                }
            }
            if (isNumeric(str)) { // 栈
                str = Integer.toString(Integer.parseInt(str) * 2);
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                riscvFunction.setStackTopOffset(str);
            } else {
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
            }
        }
    }



    private void handleStoreInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        IRValueRef value = IRGetOperand(inst, 1);
        IRValueRef pointer = IRGetOperand(inst, 2);
        if (registerAllocator.getRegister(pointer.getText()).get(0).startsWith("-"))
            return;
        boolean valueIsFloat = false;
        if (IRIsIntConstant(value)) {
            int constValue = (int) IRConstIntGetSExtValue(value);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
        } else if (IRIsFloatConstant(value)) {
            valueIsFloat = true;
            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(value));
            String top20 = "%hi(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
            String lower12 = "%lo(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
        } else {
            // 检查value是不是参数
            // TODO
            if (riscvFunction.getParamsRegister(value.getText()) != null) {
                if (riscvFunction.getParamsRegister(value.getText()).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(value.getText()))));
                else if (riscvFunction.getParamsRegister(value.getText()).startsWith("fa")) {
                    valueIsFloat = true;
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(value.getText()))));
                }
                // 参数在栈里
                else {
                    if (((IRPointerType)(inst.getOperands().get(0).getType())).getBaseType() instanceof IRFloatType) {
                        valueIsFloat = true;
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(value.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(value.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
            } else {
                String varName = IRGetValueName(value);
                String str = registerAllocator.getRegister(varName).get(0);
                String varOperateReg;
                if (value.getType() instanceof IRFloatType) {
                    valueIsFloat = true;
                    varOperateReg = "ft0";
                } else
                    varOperateReg = "t0";
                if (isNumeric(str)) { // 栈
//                    if (Integer.parseInt(str) > 10000000) {
//                        throw new RuntimeException();
//                    }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, varOperateReg), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, varOperateReg), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }


        String ptrName = IRGetValueName(pointer);

        if (IRIsAGlobalVariable(module, pointer)) {
            ptrName = ((IRGlobalRegRef) pointer).getIdentity();
            block.addInstruction(new RISCVLa(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.globalVar, ptrName)));
            if (valueIsFloat /*&& pointer.getType() instanceof IRFloatType*/) {
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
            } else
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
        } else {
            String str = registerAllocator.getRegister(ptrName).get(0);
            //判断是不是数组中元素
            if (GEPPointers.contains(ptrName)) {
                IRType arrayBase = ((IRPointerType)pointer.getType()).getBaseType();
                while(arrayBase instanceof IRArrayType)
                    arrayBase = ((IRArrayType) arrayBase).getBaseType();

                if (isNumeric(str)) { // 栈
//                    if (Integer.parseInt(str) > 10000000) {
//                        throw new RuntimeException();
//                    }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, str)));
                }
                // 根据IR的指令，分为三种情况，dest scr为float，float；int，int； int，float（也就是要将float转换为int）
                if (valueIsFloat && arrayBase instanceof IRFloatType)
                    block.addInstruction(new RISCVSw(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
                else if (valueIsFloat && arrayBase instanceof IRInt32Type) {
                    block.addInstruction(new RISCVFcvt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), "rtz"));
                    block.addInstruction(new RISCVSw(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
                } else
                    block.addInstruction(new RISCVSw(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
            } else {
                if (valueIsFloat && ((IRPointerType)pointer.getType()).getBaseType() instanceof IRFloatType) {

                    // 根据IR的指令，分为三种情况，dest scr为float，float；int，int； int，float（也就是要将float转换为int）
                    if (isNumeric(str)) { // 栈
//                        if (Integer.parseInt(str) > 10000000) {
//                            throw new RuntimeException();
//                        }
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "ft0")));
                    }
                } else if (valueIsFloat && ((IRPointerType)pointer.getType()).getBaseType() instanceof IRInt32Type) {
                    block.addInstruction(new RISCVFcvt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), "rtz"));
                    if (isNumeric(str)) { // 栈
//                        if (Integer.parseInt(str) > 10000000) {
//                            throw new RuntimeException();
//                        }
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
                    }
                } else {
                    if (isNumeric(str)) { // 栈
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
                    }
                }
            }
        }
    }

    private void handleLoadInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        if (registerAllocator.getRegister(inst.getOperands().get(0).getText()).get(0).startsWith("-"))
            return;
        IRValueRef srcValue = IRGetOperand(inst, 1);
        String srcName = IRGetValueName(srcValue);
        boolean srcIsFloat = false;
        if((srcValue.getType()) instanceof IRFloatType)
            srcIsFloat = true;

        if (IRIsAGlobalVariable(module, srcValue)) {
            srcName = ((IRGlobalRegRef)srcValue).getIdentity();
            block.addInstruction(new RISCVLa(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.globalVar, srcName)));
            if (srcIsFloat)
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, "0(t0)")));
            else
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, "0(t0)")));
        } else { // 局部变量
            String str = registerAllocator.getRegister(srcName).get(0);
            if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                str = Integer.toString(Integer.parseInt(str) * 2);
                if (srcIsFloat)
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                else
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                riscvFunction.setStackTopOffset(str);
            } else {
                if (srcIsFloat)
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, str)));
                else
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
            }
            if (GEPPointers.contains(srcName)) {
                //TODO
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t0")));
                if (srcIsFloat)
                    block.addInstruction(new RISCVLw(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
                else
                    block.addInstruction(new RISCVLw(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, "0(t1)")));
            }
        }

        IRValueRef dest = inst.getOperands().get(0);
        String destName = IRGetValueName(dest);
        String str = registerAllocator.getRegister(destName).get(0);
        if (isNumeric(str)) { // 栈
//            if (Integer.parseInt(str) > 10000000) {
//                throw new RuntimeException();
//            }
            str = Integer.toString(Integer.parseInt(str) * 2);
            if (srcIsFloat)
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
            else
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
            riscvFunction.setStackTopOffset(str);
        } else {
            if (srcIsFloat)
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "ft0")));
            else
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
        }
    }

    private void handleBinaryOperation(IRInstruction inst, String opcode, RISCVBlock block, RISCVFunction riscvFunction) {
        boolean isFloatBinary = false;
        if (registerAllocator.getRegister(IRGetValueName(inst.getOperands().get(0))).get(0).startsWith("-"))
            return;

        IRValueRef lhs = IRGetOperand(inst, 1);
        IRValueRef rhs = IRGetOperand(inst, 2);

        if (IRIsIntConstant(lhs)) {
            int constValue = (int) IRConstIntGetSExtValue(lhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
        }
        else if(IRIsFloatConstant(lhs)) {
            isFloatBinary = true;
            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(lhs));
            String top20 = "%hi(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
            String lower12 = "%lo(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
        }
        else {
            if (riscvFunction.getParamsRegister(lhs.getText()) != null) {
                if (riscvFunction.getParamsRegister(lhs.getText()).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(lhs.getText()))));
                else if (riscvFunction.getParamsRegister(lhs.getText()).startsWith("fa")) {
                    isFloatBinary = true;
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(lhs.getText()))));
                }
                // 参数在栈里
                else {
                    if (lhs.getType() instanceof IRFloatType) {
                        isFloatBinary = true;
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(lhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(lhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
            }

            else {
                if (lhs.getType() instanceof IRFloatType)
                    isFloatBinary = true;
                String varName = IRGetValueName(lhs);
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    if (isFloatBinary)
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    else
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    if (isFloatBinary)
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, str)));
                    else
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }

        if (IRIsIntConstant(rhs)) {
            int constValue = (int) IRConstIntGetSExtValue(rhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
        }
        else if(IRIsFloatConstant(rhs)) {
            isFloatBinary = true;
            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(rhs));;
            String top20 = "%hi(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, top20)));
            String lower12 = "%lo(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, lower12), "addi"));
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, "t1")));
        }
        else {
            if (riscvFunction.getParamsRegister(rhs.getText()) != null) {
                if (riscvFunction.getParamsRegister(rhs.getText()).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(rhs.getText()))));
                else if (riscvFunction.getParamsRegister(rhs.getText()).startsWith("fa")) {
                    isFloatBinary = true;
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(rhs.getText()))));
                }
                // 参数在栈里
                else {
                    if (rhs.getType() instanceof IRFloatType) {
                        isFloatBinary = true;
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(rhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(rhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
            }

            else {


                if (rhs.getType() instanceof IRFloatType)
                    isFloatBinary = true;
                String varName = IRGetValueName(rhs);
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    if (isFloatBinary)
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    else
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    if (isFloatBinary)
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, str)));
                    else
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }
        if (isFloatBinary) {
            switch (opcode) {
                case "IRFAdd":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1"), "fadd.s"));
                    break;
                case "IRFSub":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1"), "fsub.s"));
                    break;
                case "IRFMul":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1"), "fmul.s"));
                    break;
                case "IRFDiv":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1"), "fdiv.s"));
                    break;
                default:
                    break;
            }
        }
        else {
            switch (opcode) {
                case "IRAdd":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "addw"));
                    break;
                case "IRSub":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "subw"));
                    break;
                case "IRMul":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "mulw"));
                    break;
                case "IRSDiv":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "divw"));
                    break;
                case "IRSRem":
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "rem"));
                    break;
            }
        }

        String varName = IRGetValueName(inst.getOperands().get(0));
            String str = registerAllocator.getRegister(varName).get(0);
            if (isNumeric(str)) { // 栈
                str = Integer.toString(Integer.parseInt(str) * 2);
                if (isFloatBinary)
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                else
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                riscvFunction.setStackTopOffset(str);
            } else {
                if (isFloatBinary)
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "ft0")));
                else
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
            }
    }

    private void handleRetInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction, IRFunctionBlockRef functionBlockRef) {
        IRValueRef returnValue = IRGetOperand(inst, 0);
        if (returnValue == null) {
            //如果是void，直接返回
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
            block.addInstruction(new RISCVRet());
            return;
        }
        if (functionBlockRef.getRetType() instanceof IRInt32Type) {
            if (IRIsIntConstant(returnValue)) {
                int constValue = (int) IRConstIntGetSExtValue(returnValue);
                block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
            } else {
                String varName = IRGetValueName(returnValue);

                if (riscvFunction.getParamsRegister(varName) != null) {
                    if (riscvFunction.getParamsRegister(varName).startsWith("a"))
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(varName))));
                    // 参数在栈里
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
                else {
                    String str = registerAllocator.getRegister(varName).get(0);
                    if (isNumeric(str)) { // 栈
//                    if (Integer.parseInt(str) > 10000000) {
//                        throw new RuntimeException();
//                    }
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.reg, str)));
                    }
                }
            }
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
            block.addInstruction(new RISCVRet());
        }
        else if (functionBlockRef.getRetType() instanceof IRFloatType) {
            if (IRIsFloatConstant(returnValue)) {
                String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(returnValue));;
                String top20 = "%hi(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
                String lower12 = "%lo(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.reg, "t0")));
            } else {
                String varName = IRGetValueName(returnValue);

                if (riscvFunction.getParamsRegister(varName) != null) {
                     if (riscvFunction.getParamsRegister(varName).startsWith("fa")) {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(varName))));
                    }
                    // 参数在栈里
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
                else {
                    String str = registerAllocator.getRegister(varName).get(0);
                    if (isNumeric(str)) { // 栈
//                    if (Integer.parseInt(str) > 10000000) {
//                        throw new RuntimeException();
//                    }
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.reg, str)));
                    }
                }
            }
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
            block.addInstruction(new RISCVRet());
        }
    }

    private void handleCallInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction, IRFunctionBlockRef functionBlockRef) {

        //调用者保存
        LinkedHashMap<String, List<String>> callerSaveReg = new LinkedHashMap<>();
        List<IRValueRef> originParams = functionBlockRef.getParams();
        for (IRValueRef param : originParams) {
            if (riscvFunction.getParamsRegister(param.getText()).startsWith("a") || riscvFunction.getParamsRegister(param.getText()).startsWith("fa")) {
                String str = paramsRegStoreStack.get(riscvFunction.getParamsRegister(param.getText()));
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(param.getText())), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                List<String> l = new ArrayList<>();
                l.add(riscvFunction.getParamsRegister(param.getText()));
                l.add(Integer.toString(Integer.parseInt(str) / 2));
                callerSaveReg.put(param.getText(), l);
            }
        }
        boolean reachThisInst = false;
        for (IRBaseBlockRef IRBlock = IRGetFirstBaseBlock(functionBlockRef); IRBlock != null; IRBlock = IRGetNextBaseBlock(functionBlockRef, IRBlock)) {
            for (IRInstruction irInstruction = IRGetFirstInstruction(IRBlock); irInstruction != null; irInstruction = IRGetNextInstruction(IRBlock, irInstruction)) {
                if (irInstruction.equals(inst)) {
                    reachThisInst = true;
                    break;
                }
                List<IRValueRef> opts = irInstruction.getOperands();
                for (IRValueRef opt : opts) {
                    if (opt == null)
                        break;
                    List<String> locations = registerAllocator.getRegister(opt.getText());
                    if (locations != null && locations.size() > 1 &&callerSaveReg.get(opt.getText())==null) {
                        String str = Integer.toString(2 * Integer.parseInt(locations.get(1)));
                        block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, locations.get(0)), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
//                        locations.set(1, str);
                        callerSaveReg.put(opt.getText(), locations);
                    }
                }
            }
            if (reachThisInst)
                break;
        }

        List<IRValueRef> params = ((CallInstruction)inst).getParams();
        int intRegIndex = 0;
        int floatRegIndex = 0;
        int paramStackSize = 0;
        for(IRValueRef param : params) {
            if (param.getType() instanceof IRInt32Type || param.getType() instanceof IRPointerType) {
                if (intRegIndex > 7) {
                    paramStackSize += 8;
                }
                else
                    intRegIndex++;
            }
            if (IRIsFloatConstant(param) || param.getType() instanceof IRFloatType) {
                if (floatRegIndex > 7) {
                    paramStackSize += 8;
                }
                else
                    floatRegIndex++;
            }
        }
        intRegIndex = 0;
        floatRegIndex = 0;
        int paramStackLocation = 0;


        for(IRValueRef param : params) {
            if (IRIsIntConstant(param)) {
                int constValue = (int) IRConstIntGetSExtValue(param);
                block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
                if (intRegIndex < 8) {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "a" + intRegIndex), new RISCVOperand(OperandType.reg, "t0")));
                    intRegIndex++;
                }
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-paramStackSize)), "addi"));
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, paramStackLocation + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(paramStackSize)), "addi"));
                    paramStackLocation += 8;
                }
            }
            else if (IRIsFloatConstant(param)) {
                String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(param));;
                String top20 = "%hi(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
                String lower12 = "%lo(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
                if (floatRegIndex < 8) {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "fa" + floatRegIndex), new RISCVOperand(OperandType.reg, "ft0")));
                    floatRegIndex++;
                }
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-paramStackSize)), "addi"));
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, paramStackLocation + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(paramStackSize)), "addi"));
                    paramStackLocation += 8;
                }
            }
            else {
                // 检查value是不是参数
                String varName = IRGetValueName(param);

                if (riscvFunction.getParamsRegister(varName) != null) {
                    if (riscvFunction.getParamsRegister(varName).startsWith("a")) {
                        String str = Integer.toString(Integer.parseInt(callerSaveReg.get(varName).get(1)) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    }else if (riscvFunction.getParamsRegister(varName).startsWith("fa")) {
                        String str = Integer.toString(Integer.parseInt(callerSaveReg.get(varName).get(1)) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    }
                    // 参数在栈里
                    else {
                        if (param.getType() instanceof IRFloatType) {
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                        }
                        else {
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                        }
                    }
                }

                else {
                    String str = registerAllocator.getRegister(varName).get(0);
                    if (isNumeric(str)) { // 栈
                        str = Integer.toString(Integer.parseInt(str) * 2);
                        if (param.getType() instanceof IRFloatType)
                            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        else
                            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                        riscvFunction.setStackTopOffset(str);
                    } else {
                        if (param.getType() instanceof IRFloatType) {
                            if (str.startsWith("fa")) {
                                List<String> location = callerSaveReg.get(param.getText());
                                str = Integer.toString(2 * Integer.parseInt(location.get(1)));
                                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                            }
                            else
                                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, str)));
                        }
                        else {
                            if (str.startsWith("a")) {
                                List<String> location = callerSaveReg.get(param.getText());
                                str = Integer.toString(2 * Integer.parseInt(location.get(1)));
                                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                            }
                            else
                                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
                        }
                    }
                }
                    if (param.getType() instanceof IRFloatType) {
                        if (floatRegIndex < 8) {
                            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "fa" + floatRegIndex), new RISCVOperand(OperandType.reg, "ft0")));
                            floatRegIndex++;
                        } else {
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-paramStackSize)), "addi"));
                            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, paramStackLocation + "(sp)")));
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(paramStackSize)), "addi"));
                            paramStackLocation += 8;
                        }
                    }
                    if (param.getType() instanceof IRInt32Type || param.getType() instanceof IRPointerType) {
                        if (intRegIndex < 8) {
                            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "a" + intRegIndex), new RISCVOperand(OperandType.reg, "t0")));
                            intRegIndex++;
                        } else {
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-paramStackSize)), "addi"));
                            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, paramStackLocation + "(sp)")));
                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(paramStackSize)), "addi"));
                            paramStackLocation += 8;
                        }
                    }

            }
        }

        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-paramStackSize)), "addi"));
        block.addInstruction(new RISCVCall(((CallInstruction)inst).getFunction().getFunctionName()));
        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(paramStackSize)), "addi"));


        if (!((CallInstruction)inst).isVoid()) {
            String varName = IRGetValueName(inst.getOperands().get(0));
            String str = registerAllocator.getRegister(varName).get(0);
            if ( ((CallInstruction)inst).getFunction().getRetType()  instanceof IRInt32Type) {
                if (isNumeric(str)) { // 栈
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "a0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "a0")));
                }
            }
            if (((CallInstruction)inst).getFunction().getRetType() instanceof IRFloatType) {
                if (isNumeric(str)) { // 栈
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "fa0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "fa0")));
                }
            }
        }



        // 调用者恢复
        for (String key : callerSaveReg.keySet()) {
            List<String> location = callerSaveReg.get(key);
            String str = Integer.toString(2 * Integer.parseInt(location.get(1)));
            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, location.get(0)), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
        }

    }

    private void handleICmpInstruction(IRInstruction inst, String opt,RISCVBlock block, RISCVFunction riscvFunction) {
        if (registerAllocator.getRegister(inst.getOperands().get(0).getText()).get(0).startsWith("-"))
            return;

        IRValueRef lhs = IRGetOperand(inst, 1);
        IRValueRef rhs = IRGetOperand(inst, 2);
        boolean isFloatIcmp = false;
        if (IRIsIntConstant(lhs)) {
            int constValue = (int) IRConstIntGetSExtValue(lhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));


        } else if (IRIsFloatConstant(lhs)) {
            isFloatIcmp = true;
            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(lhs));
            String top20 = "%hi(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
            String lower12 = "%lo(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
        } else {
            if (riscvFunction.getParamsRegister(lhs.getText()) != null) {
                if (riscvFunction.getParamsRegister(lhs.getText()).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(lhs.getText()))));
                else if (riscvFunction.getParamsRegister(lhs.getText()).startsWith("fa")) {
                    isFloatIcmp = true;
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(lhs.getText()))));
                }
                // 参数在栈里
                else {
                    if (lhs.getType() instanceof IRFloatType) {
                        isFloatIcmp = true;
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(lhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(lhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
            }

            else {
                if (lhs.getType() instanceof IRFloatType)
                    isFloatIcmp = true;
                String varName = IRGetValueName(lhs);
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    if (isFloatIcmp)
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    else
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    if (isFloatIcmp)
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, str)));
                    else
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }

        if (IRIsIntConstant(rhs)) {
            int constValue = (int) IRConstIntGetSExtValue(rhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
        }
        else if (IRIsFloatConstant(rhs)) {
            isFloatIcmp = true;
            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(rhs));
            String top20 = "%hi(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, top20)));
            String lower12 = "%lo(" + longFormOfFloat + ")";
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, lower12), "addi"));
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, "t1")));
        }
        else {
            if (riscvFunction.getParamsRegister(rhs.getText()) != null) {
                if (riscvFunction.getParamsRegister(rhs.getText()).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(rhs.getText()))));
                else if (riscvFunction.getParamsRegister(rhs.getText()).startsWith("fa")) {
                    isFloatIcmp = true;
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(rhs.getText()))));
                }
                // 参数在栈里
                else {
                    if (rhs instanceof IRFloatType) {
                        isFloatIcmp = true;
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(rhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(rhs.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
            }
            else {

                if (rhs.getType() instanceof IRFloatType)
                    isFloatIcmp = true;
                String varName = IRGetValueName(rhs);
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    if (isFloatIcmp)
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    else
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    if (isFloatIcmp)
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, str)));
                    else
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }

        // TODO
        // IRU~ 为浮点数

        switch (opt) {
            case "IREq":
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
                block.addInstruction(new RISCVSeqz(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0")));
                break;
            case "IRUeq":
                block.addInstruction(new RISCVFeq(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1")));
                break;
            case "IRNe":
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
                block.addInstruction(new RISCVSnez(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0")));
                break;
            case "IRUne":
                block.addInstruction(new RISCVFeq(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1")));
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, "1")));
                break;
            case "IRUgt":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, "ft0")));
                break;
            case "IRUge":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1")));
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, "1")));
                break;
            case "IRUlt":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "ft1")));
                break;
            case "IRUle":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft1"), new RISCVOperand(OperandType.reg, "ft0")));
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, "1")));
                break;
            case "IRSgt":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t0")));
                break;
            case "IRSge":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, "1")));
                break;
            case "IRSlt":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
                break;
            case "IRSle":
                block.addInstruction(new RISCVSlt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t0")));
                block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, "1")));
                break;
        }


        String varName = IRGetValueName(inst.getOperands().get(0));
        String str = registerAllocator.getRegister(varName).get(0);
        if (isNumeric(str)) { // 栈
            str = Integer.toString(Integer.parseInt(str) * 2);
            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
            riscvFunction.setStackTopOffset(str);
        } else {
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));

        }
    }

    private void handleXorInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        if (registerAllocator.getRegister(inst.getOperands().get(0).getText()).get(0).startsWith("-"))
            return;

        IRValueRef lhs = IRGetOperand(inst, 1);
        IRValueRef rhs = IRGetOperand(inst, 2);

        if (IRIsIntConstant(lhs)) {
            int constValue = (int) IRConstIntGetSExtValue(lhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));

        } else {
            String varName = IRGetValueName(lhs);
            if (riscvFunction.getParamsRegister(rhs.getText()) != null) {
                if (riscvFunction.getParamsRegister(varName).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(varName))));
                // 参数在栈里
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                }
            }


            else {
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }

        if (IRIsIntConstant(rhs)) {
            int constValue = (int) IRConstIntGetSExtValue(rhs);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
        } else {
            String varName = IRGetValueName(rhs);
            if (riscvFunction.getParamsRegister(varName) != null) {
                if (riscvFunction.getParamsRegister(varName).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(varName))));
                    // 参数在栈里
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                }
            }

            else {
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }
        block.addInstruction(new RISCVXor(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
        // TODO
        String varName = IRGetValueName(inst.getOperands().get(0));
        String str = registerAllocator.getRegister(varName).get(0);
        if (isNumeric(str)) { // 栈
//            if (Integer.parseInt(str) > 10000000) {
//                throw new RuntimeException();
//            }
            str = Integer.toString(Integer.parseInt(str) * 2);
            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
            riscvFunction.setStackTopOffset(str);
        } else {
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
        }
    }

    private void handleZExtInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        IRValueRef operand = IRGetOperand(inst, 1);

        if (IRIsIntConstant(operand)) {
            int constValue = (int) IRConstIntGetSExtValue(operand);
            block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));

        } else {
            String varName = IRGetValueName(operand);
            if (riscvFunction.getParamsRegister(varName) != null) {
                if (riscvFunction.getParamsRegister(varName).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(varName))));
                    // 参数在栈里
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(varName) + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                }
            }

            else {
                String str = registerAllocator.getRegister(varName).get(0);
                if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                    str = Integer.toString(Integer.parseInt(str) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                    riscvFunction.setStackTopOffset(str);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, str)));
                }
            }
        }

        block.addInstruction(new RISCVAndi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), 1));
        String varName = IRGetValueName(inst.getOperands().get(0));
        String str = registerAllocator.getRegister(varName).get(0);
        if (isNumeric(str)) { // 栈
//            if (Integer.parseInt(str) > 10000000) {
//                throw new RuntimeException();
//            }
            str = Integer.toString(Integer.parseInt(str) * 2);
            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
            riscvFunction.setStackTopOffset(str);
        } else {
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, str), new RISCVOperand(OperandType.reg, "t0")));
        }
    }

    public void handleBrInstruction(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        if (((BranchInstruction)inst).getType() == BranchInstruction.SINGLE) {
            block.addInstruction(new RISCVJ(new RISCVOperand(OperandType.label, ((BranchInstruction) inst).getBaseBlock1().getLabel())));
        }
        else {
            String varName = IRGetValueName(inst.getOperands().get(0));
            String str = registerAllocator.getRegister(varName).get(0);
            if (isNumeric(str)) { // 栈
//                if (Integer.parseInt(str) > 10000000) {
//                    throw new RuntimeException();
//                }
                str = Integer.toString(Integer.parseInt(str) * 2);
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, str + "(sp)")));
                riscvFunction.setStackTopOffset(str);
            } else {
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"),new RISCVOperand(OperandType.reg, str)));
            }
            block.addInstruction(new RISCVBeqz(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.label, ((BranchInstruction) inst).getBaseBlock2().getLabel())));
            block.addInstruction(new RISCVJ(new RISCVOperand(OperandType.label, ((BranchInstruction) inst).getBaseBlock1().getLabel())));
        }
    }

    public void handleGetElementPointer(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        if (registerAllocator.getRegister(inst.getOperands().get(0).getText()).get(0).startsWith("-"))
            return;

        IRValueRef arrayBase = ((GetElementPointerInstruction)inst).getBase();
        String arrayBaseName = arrayBase.getText();
        String arrayBaseAllocation;

        if(arrayBase instanceof IRGlobalRegRef) {
            arrayBaseName = ((IRGlobalRegRef)arrayBase).getIdentity();
            //使用la指令加载全局变量地址
            block.addInstruction(new RISCVLa(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.label, arrayBaseName)));
            arrayBaseAllocation = "t0";
        }
        else {
            if (riscvFunction.getParamsRegister(arrayBaseName) != null) {
                if (riscvFunction.getParamsRegister(arrayBaseName).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(arrayBaseName))));
                    // 参数在栈里
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(arrayBaseName) + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                }
            } else {
                arrayBaseAllocation = registerAllocator.getRegister(arrayBaseName).get(0);
                if (isNumeric(arrayBaseAllocation)) { // 栈
//            if (Integer.parseInt(arrayBaseAllocation) > 10000000) {
//                throw new RuntimeException();
//            }
                    arrayBaseAllocation = Integer.toString(Integer.parseInt(arrayBaseAllocation) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, arrayBaseAllocation + "(sp)")));
                    riscvFunction.setStackTopOffset(arrayBaseAllocation);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, arrayBaseAllocation)));
                }
            }
        }
        // 求索引
        int index;
        if(((GetElementPointerInstruction)inst).getIndex().size() > 1 && ((GetElementPointerInstruction)inst).getIndex().get(1) instanceof IRConstIntRef ){
            //如果是一个常数作为索引
            index = Integer.parseInt(((GetElementPointerInstruction)inst).getIndex().get(1).getText());
            IRType arrayType = ((IRPointerType)((GetElementPointerInstruction)inst).getBase().getType()).getBaseType();
            arrayLength = ((IRArrayType) arrayType).getLength();
            while(((IRArrayType)arrayType).getBaseType() instanceof IRArrayType) {
                arrayLength = ((IRArrayType) arrayType).getLength();
                //计算多维数组的偏移量
                index *= ((IRArrayType)((IRArrayType)arrayType).getBaseType()).getLength();
                arrayType = ((IRArrayType) arrayType).getBaseType();
            }
            //计算偏移量，乘以4
            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"),new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(index * 4)), "addi"));
        }else if (((GetElementPointerInstruction)inst).getIndex().size() > 1){
            //todo:验证是否正确
            //是一个局部变量作为索引
            String indexName = ((GetElementPointerInstruction)inst).getIndex().get(1).getText();
            if (riscvFunction.getParamsRegister(indexName) != null) {
                if (riscvFunction.getParamsRegister(indexName).startsWith("a"))
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(indexName))));
                    // 参数在栈里
                else {
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(indexName) + "(sp)")));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                }
            }
            else {
                String indexAllocation = registerAllocator.getRegister(indexName).get(0);
                //如果是栈上的局部变量
                if (isNumeric(indexAllocation)) {
//                if (Integer.parseInt(indexAllocation) > 10000000) {
//                    throw new RuntimeException();
//                }
                    indexAllocation = Integer.toString(Integer.parseInt(indexAllocation) * 2);
                    block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, indexAllocation + "(sp)")));
                    riscvFunction.setStackTopOffset(indexAllocation);
                } else {
                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, indexAllocation)));
                }
            }
            //此时index的值在t1中，如果是多维数组，需要将index乘以数组长度
            IRType arrayType = ((IRPointerType)((GetElementPointerInstruction)inst).getBase().getType()).getBaseType();
            arrayLength = ((IRArrayType) arrayType).getLength();
                while (((IRArrayType) arrayType).getBaseType() instanceof IRArrayType) {
                    //计算多维数组的偏移量
                    arrayLength = ((IRArrayType) arrayType).getLength();
                    block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, Integer.toString(((IRArrayType)((IRArrayType)arrayType).getBaseType()).getLength()))));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t2"), "mul"));
                    arrayType = ((IRArrayType) arrayType).getBaseType();
                }
                //计算偏移量，乘以4
                //先将4存到t2中
                block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, "4")));
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, "t2"), "mul"));
                //此时t1中存放的是索引的偏移量，加到t0中

                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "add"));

        }
        else {
            // 与数组作为函数参数有关
            //todo:验证是否正确
            if (((GetElementPointerInstruction)inst).getIndex().get(0) instanceof IRConstIntRef) {
                index = Integer.parseInt(((GetElementPointerInstruction)inst).getIndex().get(0).getText());
                IRType arrayType = ((IRPointerType) ((GetElementPointerInstruction) inst).getBase().getType()).getBaseType();
                while(arrayType instanceof IRArrayType) {
                    arrayLength = ((IRArrayType) arrayType).getLength();
                    //计算多维数组的偏移量
                    index *= arrayLength;
                    arrayType = ((IRArrayType) arrayType).getBaseType();
                }
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"),new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(index * 4)), "addi"));
            }
            else {
                //是一个局部变量作为索引
                String indexName = ((GetElementPointerInstruction) inst).getIndex().get(0).getText();
                if (riscvFunction.getParamsRegister(indexName) != null) {
                    if (riscvFunction.getParamsRegister(indexName).startsWith("a"))
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(indexName))));
                        // 参数在栈里
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(indexName) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
                else {
                    String indexAllocation = registerAllocator.getRegister(indexName).get(0);
                    //如果是栈上的局部变量
                    if (isNumeric(indexAllocation)) {
//                    if (Integer.parseInt(indexAllocation) > 10000000) {
//                        throw new RuntimeException();
//                    }

                        indexAllocation = Integer.toString(Integer.parseInt(indexAllocation) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.stackRoom, indexAllocation + "(sp)")));
                        riscvFunction.setStackTopOffset(indexAllocation);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, indexAllocation)));
                    }
                }

                IRType arrayType = ((IRPointerType)((GetElementPointerInstruction)inst).getBase().getType()).getBaseType();
                while (arrayType instanceof IRArrayType) {
                    //计算多维数组的偏移量
                    arrayLength = ((IRArrayType) arrayType).getLength();
                    block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, Integer.toString(((IRArrayType)arrayType).getLength()))));
                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t2"), "mul"));
                    arrayType = ((IRArrayType) arrayType).getBaseType();
                }
                block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, "4")));
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, "t2"), "mul"));
                //此时t1中存放的是索引的偏移量，加到t0中
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "add"));
                


                //此时index的值在t1中，如果是多维数组，需要将index乘以数组长度
//                IRType arrayType = ((IRPointerType) ((GetElementPointerInstruction) inst).getBase().getType()).getBaseType();
//                if (arrayType instanceof IRArrayType) {
//
//                    while (((IRArrayType) arrayType).getBaseType() instanceof IRArrayType) {
//                        //计算多维数组的偏移量
//                        block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, Integer.toString(((IRArrayType) arrayType).getLength()))));
//                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t2"), "mul"));
//                        arrayType = ((IRArrayType) arrayType).getBaseType();
//                    }
//                    //计算偏移量，乘以8
//                    //先将8存到t2中
//                    block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t2"), new RISCVOperand(OperandType.imm, "8")));
//                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.reg, "t1"), new RISCVOperand(OperandType.imm, "t2"), "mul"));
//                    //此时t1中存放的是索引的偏移量，加到t0中
//
//                    block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1"), "add"));
//                } else {
//                    block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t1")));
//                }
            }

        }

        String destLocation = registerAllocator.getRegister(inst.getOperands().get(0).getText()).get(0);
        if (isNumeric(destLocation)) { // 栈
//            if (Integer.parseInt(destLocation) > 10000000) {
//                throw new RuntimeException();
//            }
            destLocation = Integer.toString(Integer.parseInt(destLocation) * 2);
            block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
            riscvFunction.setStackTopOffset(destLocation);
        } else {
            block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, destLocation), new RISCVOperand(OperandType.reg, "t0")));
        }
        GEPPointers.add(inst.getOperands().get(0).getText());
    }

    public void handleTypeTransfer(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction) {
        IRValueRef origin = inst.getOperands().get(1);
        IRValueRef result = inst.getOperands().get(0);
        if (registerAllocator.getRegister(result.getText()).get(0).startsWith("-"))
            return;
        int transferType = ((TypeTransferInstruction)inst).getTransferType();
        if (transferType == IRConst.IntToFloat) {
            if (IRIsIntConstant(origin)) {
                int constValue = (int) IRConstIntGetSExtValue(origin);
                block.addInstruction(new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, Integer.toString(constValue))));
            }
            else {
                if (riscvFunction.getParamsRegister(origin.getText()) != null) {
                    if (riscvFunction.getParamsRegister(origin.getText()).startsWith("a"))
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(origin.getText()))));
                    // 参数在栈里
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(origin.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }

                else {
                    String originLocation = registerAllocator.getRegister(origin.getText()).get(0);
                    if (isNumeric(originLocation)) { // 栈
//                    if (Integer.parseInt(originLocation) > 10000000) {
//                        throw new RuntimeException();
//                    }
                        originLocation = Integer.toString(Integer.parseInt(originLocation) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, originLocation + "(sp)")));
                        riscvFunction.setStackTopOffset(originLocation);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, originLocation)));
                    }
                }
            }
            block.addInstruction(new RISCVFcvt(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0"), ""));
            String resultLocation = registerAllocator.getRegister(result.getText()).get(0);
            if (isNumeric(resultLocation)) { // 栈
//                if (Integer.parseInt(resultLocation) > 10000000) {
//                    throw new RuntimeException();
//                }
                resultLocation = Integer.toString(Integer.parseInt(resultLocation) * 2);
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, resultLocation + "(sp)")));
                riscvFunction.setStackTopOffset(resultLocation);
            } else {
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, resultLocation), new RISCVOperand(OperandType.reg, "ft0")));
            }
        }

        if (transferType == IRConst.FloatToInt) {
            if (IRIsFloatConstant(origin)) {
                String longFormOfFloat = "0X" + Long.toString(IRConstFloatGetSExtValue(origin), 16);
                String top20 = "%hi(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
                String lower12 = "%lo(" + longFormOfFloat + ")";
                block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
            }
            else {
                if (riscvFunction.getParamsRegister(origin.getText()) != null) {
                    if (riscvFunction.getParamsRegister(origin.getText()).startsWith("fa"))
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(origin.getText()))));
                        // 参数在栈里
                    else {
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, riscvFunction.getParamsRegister(origin.getText()) + "(sp)")));
                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                    }
                }
                else {
                    String originLocation = registerAllocator.getRegister(origin.getText()).get(0);
                    if (isNumeric(originLocation)) { // 栈
//                    if (Integer.parseInt(originLocation) > 10000000) {
//                        throw new RuntimeException();
//                    }
                        originLocation = Integer.toString(Integer.parseInt(originLocation) * 2);
                        block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, originLocation + "(sp)")));
                        riscvFunction.setStackTopOffset(originLocation);
                    } else {
                        block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, originLocation)));
                    }
                }
            }
            block.addInstruction(new RISCVFcvt(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "ft0"), "rtz"));
            String resultLocation = registerAllocator.getRegister(result.getText()).get(0);
            if (isNumeric(resultLocation)) { // 栈
//                if (Integer.parseInt(resultLocation) > 10000000) {
//                    throw new RuntimeException();
//                }
                resultLocation = Integer.toString(Integer.parseInt(resultLocation) * 2);
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, resultLocation + "(sp)")));
                riscvFunction.setStackTopOffset(resultLocation);
            } else {
                block.addInstruction(new RISCVMv(new RISCVOperand(OperandType.reg, resultLocation), new RISCVOperand(OperandType.reg, "t0")));
            }
        }
    }

    public void handlePhiInstitution(IRInstruction inst, RISCVBlock block, RISCVFunction riscvFunction, int alignedStackSize) {
//        IRValueRef dest = inst.getOperands().get(0);
//        String destLocation = registerAllocator.getRegister(dest.getText()).get(0);
//        riscvFunction.addPhiDestAndSrc(((PhiInstruction)inst).getIncomingValues(), destLocation);


        // lost copy
        IRValueRef dest = inst.getOperands().get(0);
        String destLocation = registerAllocator.getRegister(dest.getText()).get(0);
        if (destLocation.startsWith("-"))
            return;
        riscvFunction.addPhiDestAndSrc(((PhiInstruction)inst).getIncomingValues(), destLocation);
        int tmpLocation = alignedStackSize + phiIndex * 8;
        if (isNumeric(destLocation)) {
            destLocation = Integer.toString(Integer.parseInt(destLocation) * 2);
            if (dest.getType() instanceof IRFloatType) {
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, tmpLocation + "(sp)")));
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
            }
            else if (dest.getType() instanceof IRInt32Type){
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, tmpLocation + "(sp)")));
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
            }
            else if (((IRPointerType)dest.getType()).getBaseType() instanceof IRInt32Type) {
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, tmpLocation + "(sp)")));
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
            }
            else {
                assert ((IRPointerType)dest.getType()).getBaseType() instanceof IRFloatType;
                block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, tmpLocation + "(sp)")));
                block.addInstruction(new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
            }
        }
        else {
            block.addInstruction(new RISCVLd(new RISCVOperand(OperandType.reg, destLocation), new RISCVOperand(OperandType.stackRoom, tmpLocation + "(sp)")));
        }
        phiIndex++;
    }

    public  void phiAddInSrcBlock(RISCVFunction riscvFunction, int alignedStackSize) {
//        Map<Map<IRBaseBlockRef, IRValueRef>, String> phiSrcAndDest = riscvFunction.getPhiDestAndSrc();
//        List<RISCVBlock> blocks = riscvFunction.getBlocks();
//
//        for (RISCVBlock block : blocks) {
//            if (!(block.getInstructions().get(block.getInstructions().size() - 1) instanceof RISCVJ))
//                continue;
//            for (Map<IRBaseBlockRef, IRValueRef> phiSrc : phiSrcAndDest.keySet()) {
//                for (IRBaseBlockRef irBaseBlockRef : phiSrc.keySet()) {
//                    if (irBaseBlockRef.getLabel().equals(block.getBlockLabel().getName())) {
//                        BranchInstruction br = (BranchInstruction) irBaseBlockRef.getInstructionList().get(irBaseBlockRef.getInstructionList().size() - 1);
//                        int offset = 0;
//                        if (br.getType() == BranchInstruction.SINGLE) {
//                            offset = 1;
//                        }
//                        else {
//                            offset = 4;
//                        }
//                        IRValueRef srcValue = phiSrc.get(irBaseBlockRef);
//                        boolean floatSrc = false;
//                        if (IRIsIntConstant(srcValue)) {
//                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, srcValue.getText())));
//                        }
//                        else if (IRIsFloatConstant(srcValue)) {
//                            floatSrc = true;
//                            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(srcValue));
//                            String top20 = "%hi(" + longFormOfFloat + ")";
//                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
//                            String lower12 = "%lo(" + longFormOfFloat + ")";
//                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
//                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
//                        }
//                        else {
//                            String srcLocation = registerAllocator.getRegister(phiSrc.get(irBaseBlockRef).getText()).get(0);
//                            if (isNumeric(srcLocation)) {
//                                srcLocation = Integer.toString(Integer.parseInt(srcLocation) * 2);
//                                if (srcValue.getType() instanceof IRInt32Type)
//                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
//                                else {
//                                    assert srcValue.getType() instanceof IRFloatType;
//                                    floatSrc = true;
//                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
//                                }
//                            } else {
//                                if (srcValue.getType() instanceof IRInt32Type)
//                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, srcLocation)));
//                                else {
//                                    assert srcValue.getType() instanceof IRFloatType;
//                                    floatSrc = true;
//                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, srcLocation)));
//                                }
//                            }
//                        }
//                        String destLocation = phiSrcAndDest.get(phiSrc);
//                        if (isNumeric(destLocation))  {
//                            destLocation = Integer.toString(Integer.parseInt(destLocation) * 2);
//                            if (floatSrc)
//                                block.insertInstruction(block.getInstructions().size() - offset, new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
//                            else
//                                block.insertInstruction(block.getInstructions().size() - offset, new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
//                        }
//                        else {
//                            if (floatSrc)
//                                block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, destLocation), new RISCVOperand(OperandType.reg, "ft0")));
//                            else
//                                block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, destLocation), new RISCVOperand(OperandType.reg, "t0")));
//                        }
//                    }
//                }
//            }
//        }



        // lost copy

        LinkedHashMap<LinkedHashMap<IRBaseBlockRef, IRValueRef>, String> phiSrcAndDest = riscvFunction.getPhiDestAndSrc();
        List<RISCVBlock> blocks = riscvFunction.getBlocks();
        int phiIndexSrc = 0;
        for (RISCVBlock block : blocks) {
            if (!(block.getInstructions().get(block.getInstructions().size() - 1) instanceof RISCVJ))
                continue;
            phiIndexSrc = 0;
            for (Map<IRBaseBlockRef, IRValueRef> phiSrc : phiSrcAndDest.keySet()) {
                for (IRBaseBlockRef irBaseBlockRef : phiSrc.keySet()) {
                    if (irBaseBlockRef.getLabel().equals(block.getBlockLabel().getName())) {
                        BranchInstruction br = (BranchInstruction) irBaseBlockRef.getInstructionList().get(irBaseBlockRef.getInstructionList().size() - 1);
                        int offset = 0;
                        if (br.getType() == BranchInstruction.SINGLE) {
                            offset = 1;
                        }
                        else {
                            offset = 3;
                        }
                        IRValueRef srcValue = phiSrc.get(irBaseBlockRef);
                        boolean floatSrc = false;
                        if (IRIsIntConstant(srcValue)) {
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLi(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, srcValue.getText())));
                        }
                        else if (IRIsFloatConstant(srcValue)) {
                            floatSrc = true;
                            String longFormOfFloat = "0X" + Integer.toHexString((int) IRConstFloatGetSExtValue(srcValue));
                            String top20 = "%hi(" + longFormOfFloat + ")";
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLui(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, top20)));
                            String lower12 = "%lo(" + longFormOfFloat + ")";
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVBinary(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.imm, lower12), "addi"));
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, "t0")));
                        }
                        else {
                            if (riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()) != null) {
                                if (riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()).startsWith("a"))
                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()))));
                                else if (riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()).startsWith("fa")) {
                                    floatSrc = true;
                                    block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()))));
                                }
                                // 参数在栈里
                                else {
                                    if (srcValue.getType() instanceof IRFloatType) {
                                        floatSrc = true;
                                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom,  riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()) + "(sp)")));
                                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                                    }
                                    else if ((srcValue.getType() instanceof IRInt32Type)){
                                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom,  riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()) + "(sp)")));
                                        block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                                    }
                                    else if (srcValue.getType() instanceof IRPointerType) {
                                        if (((IRPointerType) srcValue.getType()).getBaseType() instanceof IRInt32Type){
                                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom,  riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()) + "(sp)")));
                                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                                        }
                                        else {
                                            floatSrc = true;
                                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(riscvFunction.getStackSize())), "addi"));
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom,  riscvFunction.getParamsRegister(phiSrc.get(irBaseBlockRef).getText()) + "(sp)")));
                                            block.addInstruction(new RISCVBinary(new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.reg, "sp"), new RISCVOperand(OperandType.imm, Integer.toString(-riscvFunction.getStackSize())), "addi"));
                                        }
                                    }
                                }
                            }

                            else {
                                String srcLocation = registerAllocator.getRegister(phiSrc.get(irBaseBlockRef).getText()).get(0);
                                if (isNumeric(srcLocation)) {
                                    srcLocation = Integer.toString(Integer.parseInt(srcLocation) * 2);
                                    if (srcValue.getType() instanceof IRInt32Type)
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
                                    else if (srcValue.getType() instanceof IRFloatType) {
                                        floatSrc = true;
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
                                    } else if (srcValue.getType() instanceof IRPointerType) {
                                        if (((IRPointerType) srcValue.getType()).getBaseType() instanceof IRInt32Type)
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
                                        else {
                                            floatSrc = true;
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVLd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, srcLocation + "(sp)")));
                                        }
                                    }
                                } else {
                                    if (srcValue.getType() instanceof IRInt32Type)
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, srcLocation)));
                                    else if (srcValue.getType() instanceof IRFloatType) {
                                        floatSrc = true;
                                        block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, srcLocation)));
                                    } else if (srcValue.getType() instanceof IRPointerType) {
                                        if (((IRPointerType) srcValue.getType()).getBaseType() instanceof IRInt32Type)
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.reg, srcLocation)));
                                        else {
                                            floatSrc = true;
                                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVMv(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.reg, srcLocation)));
                                        }

                                    }
                                }
                            }
                        }
                        String destLocation = Integer.toString(alignedStackSize + phiIndexSrc * 8);
                        if (floatSrc)
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVSd(new RISCVOperand(OperandType.reg, "ft0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));
                        else
                            block.insertInstruction(block.getInstructions().size() - offset, new RISCVSd(new RISCVOperand(OperandType.reg, "t0"), new RISCVOperand(OperandType.stackRoom, destLocation + "(sp)")));

                    }
                }
                phiIndexSrc++;
            }
        }
    }


    int calculatePhiTmpStackSize(IRFunctionBlockRef function) {
        int phiTmpStackSize = 0;
        for (IRBaseBlockRef baseBlockRef : function.getBaseBlocks()) {
            for (IRInstruction instruction : baseBlockRef.getInstructionList()) {
                if (instruction instanceof PhiInstruction)
                    phiTmpStackSize += 8;
            }
        }
        return phiTmpStackSize;

    }


    public boolean isNumeric(String str) {
        if (str == null || str.isEmpty()) {
            return false;
        }
        for (char c : str.toCharArray()) {
            if (!Character.isDigit(c) && c != '.' && c != '-') {
                return false;
            }
        }
        return true;
    }


}

