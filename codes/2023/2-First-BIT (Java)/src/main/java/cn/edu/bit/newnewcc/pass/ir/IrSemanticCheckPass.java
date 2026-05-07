package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.SemanticCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.pass.ir.structure.DomTree;
import cn.edu.bit.newnewcc.pass.ir.util.UtilFunctions;

import java.util.*;

/**
 * IR 语义分析
 * <p>
 * 用于检查 IR 语义是否正确，可以发现部分优化过程产生的错误
 * <p>
 * 此 Pass 不会对 IR 进行任何修改
 */
public class IrSemanticCheckPass {

    private final Set<Value> globalValues = new HashSet<>();
    private DomTree domTree;
    private Set<Value> localValues;
    private Map<BasicBlock, Set<BasicBlock>> basicBlockEntries;

    private IrSemanticCheckPass() {
    }

    private void verifyUsage(Value value) {
        // 检查使用该语句值的语句合法性
        for (Operand usage : value.getUsages()) {
            if (usage.getInstruction().getBasicBlock() == null) {
                System.out.printf("In %s, it is used by %s, which is not in any block.\n",
                        value.getClass(),
                        usage.getInstruction().getClass()
                );
                throw new SemanticCheckFailedException("Value being used by a free instruction.");
            } else if (usage.getInstruction().getBasicBlock().getFunction() == null) {
                System.out.printf("In %s, it is used by %s, which is not in any function.\n",
                        value.getClass(),
                        usage.getInstruction().getClass()
                );
                throw new SemanticCheckFailedException("Value being used by a free instruction.");
            }
        }
    }

    private void verifyInstruction(Instruction instruction) {
        // 检查操作数合法性
        // Phi 指令的实际使用位置是每个入口块末尾，因此不在这里检查（Phi指令检查的localValues不是此时的）
        if (!(instruction instanceof PhiInst)) {
            for (Operand operand : instruction.getOperandList()) {
                if (!operand.hasValueBound()) {
                    throw new SemanticCheckFailedException("Operand has no value bound");
                }
                if (!(UtilFunctions.isGlobalValue(operand.getValue()) || localValues.contains(operand.getValue()))) {
                    throw new SemanticCheckFailedException("Value bound cannot be used as an operand");
                }
            }
        }
        // 检查使用该语句值的语句合法性
        verifyUsage(instruction);
    }

    private void verifyBlocksOverDomTree(BasicBlock basicBlock, boolean isFunctionEntry) {
        basicBlock.getLeadingInstructions().forEach(instruction -> {
            if (instruction instanceof AllocateInst && !isFunctionEntry) {
                throw new SemanticCheckFailedException("Alloca instruction must be placed in entry block");
            }
        });
        if (basicBlock.getTerminateInstruction() == null) {
            throw new SemanticCheckFailedException("Basic block has no terminate instruction");
        }
        for (Instruction instruction : basicBlock.getInstructions()) {
            localValues.add(instruction);
            verifyInstruction(instruction);
        }
        // 检查Phi指令
        for (Operand usage : basicBlock.getUsages()) {
            if (usage.getInstruction() instanceof PhiInst phiInst) {
                if (!phiInst.hasEntry(basicBlock)) {
                    throw new SemanticCheckFailedException("One-way usage binding.");
                }
                var value = phiInst.getValue(basicBlock);
                if (value == null) {
                    throw new SemanticCheckFailedException("Operand has no value bound.");
                }
                if (!(UtilFunctions.isGlobalValue(value) || localValues.contains(value))) {
                    throw new SemanticCheckFailedException("Value bound cannot be used as an operand");
                }
            }
        }
        verifyUsage(basicBlock);
        for (BasicBlock domSon : domTree.getDomSons(basicBlock)) {
            verifyBlocksOverDomTree(domSon, false);
        }
        basicBlock.getInstructions().forEach(localValues::remove);
    }

    private void verifyControlFlowMap(Function function) {
        var basicBlocks = function.getBasicBlocks();
        var set = new HashSet<>(basicBlocks);
        for (BasicBlock block : basicBlocks) {
            for (BasicBlock exitBlock : block.getExitBlocks()) {
                if (!set.contains(exitBlock)) {
                    throw new SemanticCheckFailedException("Control flow leads to a block outside function.");
                }
                if (!exitBlock.getEntryBlocks().contains(block)) {
                    throw new SemanticCheckFailedException("Block entry was not properly maintained.");
                }
            }
            for (BasicBlock entryBlock : block.getEntryBlocks()) {
                if (!set.contains(entryBlock)) {
                    throw new SemanticCheckFailedException("Control flow leads to a block outside function.");
                }
                if (!entryBlock.getExitBlocks().contains(block)) {
                    throw new SemanticCheckFailedException("Block entry was not properly maintained.");
                }
            }
        }
        var unreachableBlocks = new HashSet<>(basicBlocks);
        Queue<BasicBlock> bfsQueue = new LinkedList<>();
        bfsQueue.add(function.getEntryBasicBlock());
        unreachableBlocks.remove(function.getEntryBasicBlock());
        while (!bfsQueue.isEmpty()) {
            var u = bfsQueue.remove();
            for (BasicBlock exitBlock : u.getExitBlocks()) {
                if (unreachableBlocks.contains(exitBlock)) {
                    bfsQueue.add(exitBlock);
                    unreachableBlocks.remove(exitBlock);
                }
            }
        }
        if (!unreachableBlocks.isEmpty()) {
            throw new SemanticCheckFailedException("Control flow map contains unreachable blocks.");
        }
    }

    private void verifyFunction(Function function) {
        // 收集局部变量列表
        localValues = new HashSet<>(globalValues);
        localValues.addAll(function.getFormalParameters());
        localValues.addAll(function.getBasicBlocks());
        verifyControlFlowMap(function);
        domTree = DomTree.buildOver(function);
        verifyBlocksOverDomTree(function.getEntryBasicBlock(), true);
        // 用 getExitBlocks 方法收集块入口列表
        basicBlockEntries = new HashMap<>();
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            basicBlockEntries.put(basicBlock, new HashSet<>());
        }
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            basicBlock.getExitBlocks().forEach(
                    exitBlock -> basicBlockEntries.get(exitBlock).add(basicBlock)
            );
        }
        // 检查 Phi 语句
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getLeadingInstructions()) {
                if (instruction instanceof PhiInst phiInst) {
                    if (!Objects.equals(phiInst.getEntrySet(), basicBlockEntries.get(basicBlock))) {
                        throw new SemanticCheckFailedException("Phi instruction's entry map does not match basic block entries");
                    }
                }
            }
        }
        // 检查基本块维护的入口块列表是否正确
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            if (!Objects.equals(basicBlock.getEntryBlocks(), basicBlockEntries.get(basicBlock))) {
                System.out.println(basicBlock.getEntryBlocks().getClass());
                System.out.println(basicBlockEntries.get(basicBlock).getClass());
                throw new SemanticCheckFailedException("Block entry was not properly maintained");
            }
        }
        verifyUsage(function);
    }

    private void verifyModule(Module module) {
        globalValues.addAll(module.getExternalFunctions());
        globalValues.addAll(module.getFunctions());
        globalValues.addAll(module.getGlobalVariables());
        module.getGlobalVariables().forEach(this::verifyUsage);
        module.getFunctions().forEach(this::verifyUsage);
        module.getFunctions().forEach(this::verifyFunction);
    }

    public static void verify(Module module) {
        new IrSemanticCheckPass().verifyModule(module);
    }

}
