package cn.edu.bit.newnewcc.backend.asm;

import cn.edu.bit.newnewcc.backend.asm.allocator.AddressAllocator;
import cn.edu.bit.newnewcc.backend.asm.allocator.RegisterAllocator;
import cn.edu.bit.newnewcc.backend.asm.allocator.StackAllocator;
import cn.edu.bit.newnewcc.backend.asm.controller.GraphColoringRegisterControl;
import cn.edu.bit.newnewcc.backend.asm.controller.LifeTimeController;
import cn.edu.bit.newnewcc.backend.asm.controller.RegisterControl;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.backend.asm.optimizer.OptimizerManager;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.BackendOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.ImmediateValues;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.constant.ConstBool;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.constant.ConstLong;
import cn.edu.bit.newnewcc.pass.ir.structure.I32ValueRangeAnalyzer;

import java.util.*;
import java.util.function.Consumer;

/**
 * 函数以函数名作为唯一标识符加以区分
 */
public class AsmFunction {
    private final String functionName;

    private final AsmCode globalCode;
    private final List<AsmOperand> formalParameters = new ArrayList<>();
    private final Map<Value, AsmOperand> formalParameterMap = new HashMap<>();
    private final Map<Value, Register> parameterValueMap = new HashMap<>();
    private final List<AsmInstruction> parameterInstructions = new ArrayList<>();
    private final List<AsmBasicBlock> basicBlocks = new ArrayList<>();
    private final Map<BasicBlock, AsmBasicBlock> basicBlockMap = new HashMap<>();
    private final Map<AsmLabel, BasicBlock> asmLabelToBasicBlockMap = new HashMap<>();
    private List<AsmInstruction> instrList = new ArrayList<>();
    private final Label retBlockLabel;
    private final BaseFunction baseFunction;
    private final Register returnRegister;

    public MemoryAddress getAddress(AsmOperand address) {
        java.util.function.Function<Integer, MemoryAddress> getAddress = (Integer tmpInt) -> {
            if (address instanceof MemoryAddress addressTmp) {
                return addressTmp;
            } else if (address instanceof StackVar stackVar) {
                return transformStackVarToAddress(stackVar);
            } else {
                throw new IllegalArgumentException();
            }
        };
        return getAddress.apply(1);
    }

    public MemoryAddress getOperandToAddress(AsmOperand operand) {
        if (operand instanceof MemoryAddress address) {
            var offset = address.getOffset();
            if (ImmediateValues.bitLengthNotInLimit(offset)) {
                var tmp = getAddressToIntRegister(address);
                return new MemoryAddress(0, tmp);
            } else {
                return address.getAddress();
            }
        } else if (operand instanceof StackVar stackVar) {
            if (stackVar instanceof ExStackVarContent) {
                return stackVar.getAddress().getAddress();
            }
            return transformStackVarToAddress(stackVar);
        } else if (operand instanceof IntRegister intRegister) {
            return new MemoryAddress(0, intRegister);
        } else {
            throw new NoSuchElementException();
        }
    }

    public MemoryAddress transformStackVarToAddress(StackVar stackVar) {
        IntRegister tmp = getRegisterAllocator().allocateInt();
        MemoryAddress now = stackVar.getAddress();
        IntRegister t2 = getRegisterAllocator().allocateInt();
        appendInstruction(new AsmLoad(t2, ExStackVarOffset.transform(now.getOffset())));
        appendInstruction(new AsmAdd(tmp, t2, now.getRegister(), 64));
        return now.withBaseRegister(tmp).setOffset(0).getAddress();
    }

    public Label getJumpLabel(BasicBlock jumpBlock) {
        AsmBasicBlock block = getBasicBlock(jumpBlock);
        return block.getBlockLabel();
    }

    public Register getValueToRegister(Value value) {
        var op = getValueByType(value, value.getType());
        Register result;
        if (op instanceof Register reg) {
            result = getRegisterAllocator().allocate(reg);
            appendInstruction(new AsmMove(result, reg));
        } else {
            if (value.getType() instanceof FloatType) {
                var tmp = getRegisterAllocator().allocateFloat();
                appendInstruction(new AsmLoad(tmp, op));
                result = tmp;
            } else {
                var tmp = getRegisterAllocator().allocateInt();
                appendInstruction(new AsmLoad(tmp, op));
                result = tmp;
            }
        }
        return result;
    }

    public AsmOperand getValueByType(Value value, Type type) {
        var result = getValue(value);
        if (type instanceof PointerType && result instanceof MemoryAddress address) {
            return getAddressToIntRegister(address.getAddress());
        } else {
            return result;
        }
    }

    public FloatRegister getOperandToFloatRegister(AsmOperand operand) {
        if (operand instanceof FloatRegister floatRegister) {
            return floatRegister;
        } else {
            FloatRegister tmp = getRegisterAllocator().allocateFloat();
            appendInstruction(new AsmLoad(tmp, operand));
            return tmp;
        }
    }

    public IntRegister getOperandToIntRegister(AsmOperand operand) {
        if (operand instanceof IntRegister intRegister) {
            return intRegister;
        } else if (operand instanceof MemoryAddress addressDirective) {
            return getAddressToIntRegister(addressDirective);
        }
        IntRegister result = getRegisterAllocator().allocateInt();
        appendInstruction(new AsmLoad(result, operand));
        return result;
    }

    public IntRegister getAddressToIntRegister(MemoryAddress address) {
        if (address.getOffset() == 0) {
            return address.getRegister();
        } else {
            int offset = Math.toIntExact(address.getOffset());
            var tmp = getRegisterAllocator().allocateInt();
            if (ImmediateValues.bitLengthNotInLimit(offset)) {
                var reg = getRegisterAllocator().allocateInt();
                appendInstruction(new AsmLoad(reg, new Immediate(offset)));
                appendInstruction(new AsmAdd(tmp, reg, address.getRegister(), 64));
            } else {
                appendInstruction(new AsmAdd(tmp, address.getRegister(), new Immediate(offset), 64));
            }
            return tmp;
        }
    }

    /**
     * 获取一个ir value在汇编中对应的值（函数参数、寄存器、栈上变量、全局变量、地址）
     *
     * @param value ir中的value
     * @return 对应的汇编操作数
     */
    public AsmOperand getValue(Value value) {
        if (value instanceof GlobalVariable globalVariable) {
            AsmGlobalVariable asmGlobalVariable = getGlobalCode().getGlobalVariable(globalVariable);
            IntRegister reg = getRegisterAllocator().allocateInt();
            appendInstruction(new AsmLoad(reg, asmGlobalVariable.emitNoSegmentLabel()));
            return new MemoryAddress(0, reg);
        } else if (value instanceof Function.FormalParameter formalParameter) {
            return getParameterValue(formalParameter);
        } else if (value instanceof Instruction instruction) {
            if (getRegisterAllocator().contain(instruction)) {
                return getRegisterAllocator().get(instruction);
            } else if (getStackAllocator().contain(instruction)) {
                return transformStackVar(getStackAllocator().get(instruction));
            } else if (getAddressAllocator().contain(instruction)) {
                return getAddressAllocator().get(instruction);
            } else {
                throw new RuntimeException("Value not found : " + value.getValueNameIR());
            }
        } else if (value instanceof Constant constant) {
            return getConstantVar(constant, this::appendInstruction);
        }
        throw new RuntimeException("Value type not found : " + value.getValueNameIR());
    }

    public AsmOperand getConstantVar(Constant constant, Consumer<AsmInstruction> appendInstruction) {
        if (constant instanceof ConstInt constInt) {
            var intValue = constInt.getValue();
            return getConstInt(intValue, appendInstruction);
        } else if (constant instanceof ConstFloat constFloat) {
            return transConstFloat(constFloat.getValue(), appendInstruction);
        } else if (constant instanceof ConstBool constBool) {
            return new Immediate(constBool.getValue() ? 1 : 0);
        } else if (constant instanceof ConstLong constLong) {
            if (ImmediateValues.isIntValue(constLong.getValue())) {
                return getConstInt(Math.toIntExact(constLong.getValue()), appendInstruction);
            }
            return transConstLong(constLong.getValue(), appendInstruction);
        }
        throw new RuntimeException("Constant value error");
    }

    public AsmOperand getConstInt(int intValue, Consumer<AsmInstruction> appendInstruction) {
        if (ImmediateValues.bitLengthNotInLimit(intValue)) {
            IntRegister tmp = getRegisterAllocator().allocateInt();
            appendInstruction.accept(new AsmLoad(tmp, new Immediate(intValue)));
            return tmp;
        }
        return new Immediate(intValue);
    }

    public List<AsmInstruction> getInstrList() {
        return instrList;
    }

    public boolean isExternal() {
        return basicBlocks.isEmpty();
    }

    public AsmFunction(BaseFunction baseFunction, AsmCode code) {
        this.globalCode = code;
        int intParameterId = 0, floatParameterId = 0;
        this.functionName = baseFunction.getValueName();
        this.baseFunction = baseFunction;

        {
            Register a0 = IntRegister.getParameter(0);
            Register fa0 = FloatRegister.getParameter(0);
            this.returnRegister = (baseFunction.getReturnType() instanceof FloatType) ?
                    fa0 : (baseFunction.getReturnType() instanceof IntegerType ? a0 : null);
        }

        retBlockLabel = new Label(functionName + "_ret", false);
        int stackSize = 0;
        for (var parameterType : baseFunction.getParameterTypes()) {
            if (parameterType instanceof IntegerType || parameterType instanceof PointerType) {
                int needSize = parameterType instanceof IntegerType ? 4 : 8;
                if (intParameterId < 8) {
                    formalParameters.add(IntRegister.getParameter(intParameterId));
                } else {
                    formalParameters.add(new StackVar(stackSize, needSize, false));
                    stackSize += needSize;
                }
                intParameterId += 1;
            } else if (parameterType instanceof FloatType) {
                if (floatParameterId < 8) {
                    formalParameters.add(FloatRegister.getParameter(floatParameterId));
                } else {
                    formalParameters.add(new StackVar(stackSize, 4, false));
                    stackSize += 4;
                }
                floatParameterId += 1;
            }
        }
    }

    public FloatRegister transConstFloat(Float value, Consumer<AsmInstruction> appendInstruction) {
        var constantFloat = globalCode.getConstFloat(value);
        IntRegister rAddress = registerAllocator.allocateInt();
        FloatRegister tmp = registerAllocator.allocateFloat();
        appendInstruction.accept(new AsmLoad(rAddress, constantFloat.getConstantLabel()));
        appendInstruction.accept(new AsmLoad(tmp, new MemoryAddress(0, rAddress), 32));
        return tmp;
    }

    public IntRegister transConstLong(long value, Consumer<AsmInstruction> appendInstruction) {
        var constantLong = globalCode.getConstLong(value);
        IntRegister rAddress = registerAllocator.allocateInt();
        IntRegister tmp = registerAllocator.allocateInt();
        appendInstruction.accept(new AsmLoad(rAddress, constantLong.getConstantLabel()));
        appendInstruction.accept(new AsmLoad(tmp, new MemoryAddress(0, rAddress), 64));
        return tmp;
    }

    void buildParameterInstructions() {
        if (baseFunction instanceof Function function) {
            for (var value : function.getFormalParameters()) {
                var reg = getRegisterAllocator().allocate(value.getType());
                parameterValueMap.put(value, reg);
                var op = getParameterByFormal(value, parameterInstructions);
                if (op instanceof Register paraReg) {
                    parameterInstructions.add(new AsmMove(reg, paraReg));
                } else {
                    parameterInstructions.add(new AsmLoad(reg, op));
                }
            }
        }
    }

    public void emitCode() {
        //生成具体函数的汇编代码
        if (baseFunction instanceof Function function) {
            var rangeAnalyzer = I32ValueRangeAnalyzer.analysis(function);

            var functionParameterList = function.getFormalParameters();
            for (var i = 0; i < functionParameterList.size(); i++) {
                formalParameterMap.put(functionParameterList.get(i), formalParameters.get(i));
            }

            //获得基本块的bfs序
            Set<BasicBlock> visited = new HashSet<>();
            Queue<BasicBlock> queue = new ArrayDeque<>();
            BasicBlock startBlock = function.getEntryBasicBlock();
            visited.add(startBlock);
            queue.add(startBlock);
            while (!queue.isEmpty()) {
                BasicBlock nowBlock = queue.remove();
                AsmBasicBlock asmBasicBlock = new AsmBasicBlock(this, nowBlock, rangeAnalyzer);
                basicBlocks.add(asmBasicBlock);
                basicBlockMap.put(nowBlock, asmBasicBlock);
                asmLabelToBasicBlockMap.put(asmBasicBlock.getInstBlockLabel(), nowBlock);
                for (var nextBlock : nowBlock.getExitBlocks()) {
                    if (!visited.contains(nextBlock)) {
                        visited.add(nextBlock);
                        queue.add(nextBlock);
                    }
                }
            }

            buildParameterInstructions();
            for (var block : basicBlocks) {
                block.preTranslatePhiInstructions();
            }
            for (var block : basicBlocks) {
                block.emitToFunction();
            }
            instrList.addAll(1, parameterInstructions);
            for (var inst : instrList) {
                for (var x : AsmInstructions.getReadVRegSet(inst)) {
                    for (var y : AsmInstructions.getWriteVRegSet(inst)) {
                        if (x.equals(y)) {
                            throw new AssertionError();
                        }
                    }
                }
            }

            OptimizerManager optimizerManager = new OptimizerManager();
            optimizerManager.runBeforeRegisterAllocation(this);

            instrList.add(new AsmLabel(retBlockLabel));
            lifeTimeController.getAllRegLifeTime(instrList);
            //asmOptimizerBeforeRegisterAllocate(lifeTimeController);
            reAllocateRegister();
            //asmOptimizerAfterRegisterAllocate();

            List<AsmInstruction> newInstrList = new ArrayList<>();
            newInstrList.addAll(stackAllocator.emitPrologue());
            newInstrList.addAll(instrList);
            newInstrList.addAll(stackAllocator.emitEpilogue());
            instrList = newInstrList;

            optimizerManager.runAfterRegisterAllocation(this);
        }
    }

    public String emit() {
        StringBuilder builder = new StringBuilder();
        builder.append(".text\n.align 1\n");
        builder.append(String.format(".globl %s\n", functionName));
        builder.append(String.format(".type %s, @function\n", functionName));
        builder.append(String.format("%s:\n", functionName));
        for (var inst : instrList) {
            builder.append(inst.emit());
        }
        builder.append(String.format(".size %s, .-%s\n", functionName, functionName));
        return builder.toString();
    }


    //向函数添加指令的方法
    public void appendInstruction(AsmInstruction instruction) {
        instrList.add(instruction);
    }

    public void appendAllInstruction(Collection<AsmInstruction> instructions) {
        instrList.addAll(instructions);
    }

    //函数内部资源分配器
    private final StackAllocator stackAllocator = new StackAllocator();
    private final RegisterAllocator registerAllocator = new RegisterAllocator();
    private final AddressAllocator addressAllocator = new AddressAllocator();
    private final LifeTimeController lifeTimeController = new LifeTimeController(this);

    //资源对应的get方法
    public AsmBasicBlock getBasicBlock(BasicBlock block) {
        return basicBlockMap.get(block);
    }

    public String getFunctionName() {
        return Objects.requireNonNull(functionName);
    }

    public Label getRetBlockLabel() {
        return retBlockLabel;
    }

    public AsmCode getGlobalCode() {
        return globalCode;
    }

    public RegisterAllocator getRegisterAllocator() {
        return registerAllocator;
    }

    public StackAllocator getStackAllocator() {
        return stackAllocator;
    }

    public AddressAllocator getAddressAllocator() {
        return addressAllocator;
    }

    public LifeTimeController getLifeTimeController() {
        return lifeTimeController;
    }

    public BasicBlock getBasicBlockByLabel(AsmLabel label) {
        return asmLabelToBasicBlockMap.get(label);
    }

    public Function getBaseFunction() {
        if (baseFunction instanceof Function function) {
            return function;
        }
        throw new RuntimeException("get wrong function type");
    }

    public Register getParameterValue(Value formalParameter) {
        return parameterValueMap.get(formalParameter);
    }

    public AsmOperand getParameterByFormal(Value formalParameter, List<AsmInstruction> instructionList) {
        var result = formalParameterMap.get(formalParameter);
        if (result instanceof StackVar stackVar) {
            return transformStackVar(stackVar.flip(), instructionList);
        }
        return result;
    }

    public List<AsmOperand> getFormalParameterList() {
        return formalParameters;
    }

    public int getParameterSize() {
        return formalParameters.size();
    }

    public Register getReturnRegister() {
        return returnRegister;
    }


    //调用另一个函数的汇编代码
    // update: 参数求值方式调整为在函数开始块中将参数load出来，因此函数参数寄存器不再需要保存，
    public Collection<AsmInstruction> call(AsmFunction calledFunction, List<AsmOperand> parameters, Register returnRegister) {
        stackAllocator.callFunction();
        List<AsmInstruction> instrList = new ArrayList<>();
        //参数数量不匹配
        if (parameters.size() != calledFunction.formalParameters.size()) {
            throw new RuntimeException("Function parameter mismatch");
        }

        //若形参中使用了参数保存寄存器，则需先push寄存器保存原有内容，避免受到下一层影响

        for (int i = 0; i < parameters.size(); i++) {
            var formalParam = calledFunction.formalParameters.get(i);
            if (formalParam instanceof StackVar stackVar) {
                formalParam = transformStackVar(stackVar, instrList);
            }

            //为下一层的函数参数存储开辟栈空间
            if (!(formalParam instanceof Register)) {
                if (!(formalParam instanceof ExStackVarContent)) {
                    throw new RuntimeException("formal parameter in wrong place");
                }
                stackAllocator.pushBack((StackVar) formalParam);
            }

            //将参数存储到形参对应位置
            var para = parameters.get(i);
            if (para instanceof Register reg) {
                if (formalParam instanceof Register)
                    instrList.add(new AsmMove((Register) formalParam, reg));
                else
                    instrList.add(new AsmStore(reg, (StackVar) formalParam));
            } else {
                if (formalParam instanceof Register formalReg) {
                    instrList.add(new AsmLoad(formalReg, para));
                } else {
                    IntRegister tmp = registerAllocator.allocateInt();
                    instrList.add(new AsmLoad(tmp, para));
                    instrList.add(new AsmStore(tmp, (StackVar) formalParam));
                }
            }
        }
        instrList.add(new AsmCall(new Label(calledFunction.getFunctionName(), true), calledFunction.getParamRegs(), calledFunction.getReturnRegister()));
        if (returnRegister != null) {
            instrList.add(new AsmMove(returnRegister, calledFunction.getReturnRegister()));
        }
        return instrList;
    }

    public List<Register> getParamRegs() {
        List<Register> paramRegList = new ArrayList<>();
        for (var para : getFormalParameterList()) {
            if (para instanceof Register paramReg) {
                paramRegList.add(paramReg);
            }
        }
        return paramRegList;
    }

    public ExStackVarContent transformStackVar(StackVar stackVar, List<AsmInstruction> newInstructionList) {
        MemoryAddress stackAddress = stackVar.getAddress();
        ExStackVarOffset offset = ExStackVarOffset.transform(stackAddress.getOffset());
        IntRegister tmp = registerAllocator.allocateInt();
        newInstructionList.add(new AsmLoad(tmp, offset));
        IntRegister t2 = registerAllocator.allocateInt();
        newInstructionList.add(new AsmAdd(t2, tmp, stackAddress.getRegister(), 64));
        MemoryAddress now = new MemoryAddress(0, t2);
        return ExStackVarContent.transform(stackVar, now);
    }

    public ExStackVarContent transformStackVar(StackVar stackVar) {
        return transformStackVar(stackVar, instrList);
    }

    //未分配寄存器的分配方法

    private void reAllocateRegister() {
        //RegisterControl registerController = new LinearScanRegisterControl(this, stackAllocator);
        //var newInstructionList = registerController.work(instrList);

        RegisterControl registerController = new GraphColoringRegisterControl(this, stackAllocator);
        var newInstructionList = registerController.work(instrList);

        //RegisterControl registerController = new LinearScanRegisterControlR1(this, stackAllocator);
        //var newInstructionList = registerController.work(instrList);

        for (var inst : newInstructionList) {
            for (int j = 1; j <= 3; j++) {
                AsmOperand operand = inst.getOperand(j);
                if (operand instanceof ExStackVarAdd operandAdd) {
                    inst.setOperand(j, operandAdd.add(-stackAllocator.getExSize()));
                }
            }
        }
        instrList = new ArrayList<>();
        instrList.addAll(registerController.emitPrologue());
        instrList.addAll(newInstructionList);
        instrList.addAll(registerController.emitEpilogue());
    }

    //寄存器分配前的优化器，用于合并copy后的权值
    private void asmOptimizerBeforeRegisterAllocate(LifeTimeController lifeTimeController) {
        instrList = BackendOptimizer.beforeAllocateScanForward(instrList, lifeTimeController);
    }


    private void asmOptimizerAfterRegisterAllocate() {
        LinkedList<AsmInstruction> linkedInstructionList = new LinkedList<>(instrList);
        linkedInstructionList = BackendOptimizer.afterAllocateScanForward(linkedInstructionList);
        linkedInstructionList = BackendOptimizer.afterAllocateScanBackward(linkedInstructionList);
        instrList = new ArrayList<>(linkedInstructionList);
    }

}
