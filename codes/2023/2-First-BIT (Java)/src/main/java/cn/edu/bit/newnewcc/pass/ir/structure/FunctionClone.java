package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.JumpInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.ReturnInst;

import java.util.ArrayList;
import java.util.List;

/**
 * 函数的克隆体 <br>
 * 函数的实参、返回地址等均用符号代替，需使用replaceAllUsageTo方法替换 <br>
 */
public class FunctionClone extends CloneBase {

    private final Function function;
    private final BasicBlock returnBlock;

    /// 需要外部处理的值
    private final List<BasicBlock> basicBlocks = new ArrayList<>();
    private final PhiInst returnValue;
    private final List<AllocateInst> allocateInstructions = new ArrayList<>();

    public FunctionClone(Function function, List<Value> arguments, BasicBlock returnBlock) {
        this.function = function;
        int parameterNum = function.getFormalParameters().size();
        for (int i = 0; i < parameterNum; i++) {
            setValueMapKv(function.getFormalParameters().get(i), arguments.get(i));
        }
        this.returnBlock = returnBlock;
        this.returnValue = new PhiInst(function.getReturnType());
        doFunctionClone();
    }

    public BasicBlock getEntryBlock() {
        return (BasicBlock) getReplacedValue(function.getEntryBasicBlock());
    }

    public List<BasicBlock> getBasicBlocks() {
        return basicBlocks;
    }

    public PhiInst getReturnValue() {
        return returnValue;
    }

    public List<AllocateInst> getAllocateInstructions() {
        return allocateInstructions;
    }

    private void doFunctionClone() {
        // 构建占位符
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            addBasicBlock(basicBlock);
            for (Instruction instruction : basicBlock.getInstructions()) {
                addSymbolFor(instruction);
            }
        }
        // 构建函数体
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            var clonedBlock = (BasicBlock) getReplacedValue(basicBlock);
            for (Instruction instruction : basicBlock.getInstructions()) {
                var clonedInstruction = cloneInstruction(instruction);
                setValueMapKv(instruction, clonedInstruction);
                if (instruction instanceof ReturnInst returnInst) {
                    returnValue.addEntry(clonedBlock, getReplacedValue(returnInst.getReturnValue()));
                }
                // 放置语句到合适的位置
                if (instruction instanceof AllocateInst) {
                    allocateInstructions.add((AllocateInst) clonedInstruction);
                } else {
                    clonedBlock.addInstruction(clonedInstruction);
                }
            }
            this.basicBlocks.add(clonedBlock);
        }
    }

    @Override
    protected Instruction cloneInstruction(Instruction instruction) {
        if (instruction instanceof ReturnInst) {
            return new JumpInst(returnBlock);
        } else {
            return super.cloneInstruction(instruction);
        }
    }

}
