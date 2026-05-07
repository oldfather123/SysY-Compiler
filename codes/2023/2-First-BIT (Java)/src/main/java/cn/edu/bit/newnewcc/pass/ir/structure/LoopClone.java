package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.JumpInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;

import java.util.*;

public class LoopClone extends CloneBase {

    private final BasicBlock headBlock;
    private final Collection<BasicBlock> loopBasicBlocks;

    private final List<BasicBlock> clonedBasicBlocks = new ArrayList<>();
    private final Map<PhiInst, PhiInst> headPhiMap = new HashMap<>();
    private final BasicBlockSymbol nextLoopHeadSymbol = new BasicBlockSymbol();

    public LoopClone(BasicBlock headBlock, Collection<BasicBlock> loopBasicBlocks) {
        this.headBlock = headBlock;
        this.loopBasicBlocks = loopBasicBlocks;
        doLoopClone();
    }

    public void setNextLoopHead(BasicBlock nextLoopHead) {
        nextLoopHeadSymbol.replaceAllUsageTo(nextLoopHead);
    }

    public void addEntry(BasicBlock basicBlock, Map<PhiInst, Value> valueMap) {
        if (!valueMap.keySet().equals(headPhiMap.keySet())) {
            throw new CompilationProcessCheckFailedException();
        }
        valueMap.forEach((entryPhi, entryValue) -> {
            headPhiMap.get(entryPhi).addEntry(basicBlock, entryValue);
        });
    }

    public BasicBlock getClonedLoopHead() {
        return (BasicBlock) getReplacedValue(headBlock);
    }

    public List<BasicBlock> getClonedBasicBlocks() {
        return Collections.unmodifiableList(clonedBasicBlocks);
    }

    public Value getClonedValue(Value originalValue) {
        return getReplacedValue(originalValue);
    }

    private void doLoopClone() {
        // 构建占位符
        for (BasicBlock basicBlock : loopBasicBlocks) {
            addBasicBlock(basicBlock);
            for (Instruction instruction : basicBlock.getInstructions()) {
                addSymbolFor(instruction);
            }
        }
        // 构建函数体
        for (BasicBlock basicBlock : loopBasicBlocks) {
            var clonedBlock = (BasicBlock) getReplacedValue(basicBlock);
            for (Instruction instruction : basicBlock.getInstructions()) {
                Instruction clonedInstruction;
                if (basicBlock == headBlock && instruction instanceof PhiInst) {
                    clonedInstruction = new PhiInst(instruction.getType());
                    headPhiMap.put((PhiInst) instruction, (PhiInst) clonedInstruction);
                } else if (instruction instanceof JumpInst jumpInst && jumpInst.getExit() == headBlock) {
                    clonedInstruction = new JumpInst(nextLoopHeadSymbol);
                } else {
                    clonedInstruction = cloneInstruction(instruction);
                }
                setValueMapKv(instruction, clonedInstruction);
                clonedBlock.addInstruction(clonedInstruction);
            }
            this.clonedBasicBlocks.add(clonedBlock);
        }
    }

}
