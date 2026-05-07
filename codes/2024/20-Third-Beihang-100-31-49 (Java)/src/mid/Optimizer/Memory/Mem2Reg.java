package mid.Optimizer.Memory;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.MoveIR;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.ControllFlow.DominAnalyzer;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class Mem2Reg {
    private final DominAnalyzer dominAnalyzer;
    private final Module module;

    public Mem2Reg() {
        this.dominAnalyzer = Optimizer.instance().getDominAnalyzer();
        this.module = IRManager.getModule();
    }

    public void optimize() {
        //插入Phi
        insertPhiLoop();
        //变量重命名
        for (Function function : module.getDecledFunctions()) {
            rename(function.getEntranceBlock(), new HashMap<>());
        }
    }

    private void insertPhiLoop() {
        //只有Alloca指令，即Alloca会产生局部变量
        for (Function function : module.getDecledFunctions()) {
            for (BasicBlock block : function.getBlocks()) {
                ArrayList<Instruction> blockInstructions = new ArrayList<>(block.getInstructionList());
                for (Instruction instruction : blockInstructions) {
                    if (instruction instanceof Alloca alloca && !alloca.getType().equals(ValueType.ARRAY)) {
                        insertPhi(alloca);
                    }
                }
            }
        }
    }

    private void insertPhi(Alloca v) {
        //将要增加phi指令的块
        LinkedList<BasicBlock> listF = new LinkedList<>();
        //v的定义集合
        LinkedList<BasicBlock> listW = new LinkedList<>();

        LinkedList<BasicBlock> defs = new LinkedList<>();

        for (Value d : v.getUserList()) {
            //只有store指令是局部变量的def
            if (d instanceof Store store) {
                defs.add(store.getBlock());
                if (!listW.contains(store.getBlock())) {
                    listW.add(store.getBlock());
                }
            }
        }

        while (!listW.isEmpty()) {
            BasicBlock blockX = listW.pop();
            if (dominAnalyzer.getDFOf(blockX) == null) {
                continue;
            }
            for (BasicBlock blockY : dominAnalyzer.getDFOf(blockX)) {
                if (!listF.contains(blockY)) {
                    blockY.addInstrAtEntry(new Phi(v, IRManager.getInstance().declareLocalVar()));
                    if (!listF.contains(blockY)) {
                        listF.add(blockY);
                    }
                    if (!defs.contains(blockY) && !listW.contains(blockY)) {
                        listW.add(blockY);
                    }
                }
            }
        }
    }

    private void rename(BasicBlock entry, HashMap<Alloca, Value> addrValue) {
        ArrayList<Instruction> instructions = new ArrayList<>(entry.getInstructionList());
        ArrayList<Instruction> instructionsToDestroy = new ArrayList<>();
        //更新到达定义
        for (Instruction instruction : instructions) {
            if (instruction instanceof Alloca alloca && !alloca.getType().equals(ValueType.ARRAY)) {
                addrValue.put(alloca, new ConstNumber(0));
                entry.removeInstruction(instruction);
                instructionsToDestroy.add(instruction);
            } else if (instruction instanceof Store store &&
                    store.getAddr() instanceof Alloca addr && !addr.getType().equals(ValueType.ARRAY)) {
                addrValue.put(addr, store.getSrc());
                entry.removeInstruction(instruction);
                instructionsToDestroy.add(instruction);
            } else if (instruction instanceof Load load &&
                    load.getAddr() instanceof Alloca addr && !addr.getType().equals(ValueType.ARRAY)) {
                load.beReplacedBy(addrValue.get(addr).withType(!load.isFloat()));
                entry.removeInstruction(instruction);
                instructionsToDestroy.add(instruction);
            } else if (instruction instanceof Phi phi) {
                addrValue.put(phi.getPhiAddr(), phi);
            }
        }


        //更新CFG中后继块中的Phi
        HashSet<BasicBlock> children = Optimizer.instance().getCFG().getChildren(entry);
        if (children != null) {
            for (BasicBlock child : children) {
                for (Instruction instruction : child.getInstructionList()) {
                    if (instruction instanceof Phi phi) {
                        Alloca addr = phi.getPhiAddr();
                        //phi必须涵盖所有前驱块，即使其中没有定义点
                        if (addrValue.containsKey(addr) && addrValue.get(addr) != null) {
                            phi.addCond(addrValue.get(addr), entry);
                        } else {
                            phi.addCond(new ConstNumber(0), entry);
                        }
                    }
                }
            }
        }

        //在支配树中dfs
        children = dominAnalyzer.getDominTree().get(entry);
        if (children != null) {
            for (BasicBlock child : children) {
                rename(child, new HashMap<>(addrValue));
            }
        }
        instructionsToDestroy.forEach(Instruction::destroy);
    }

    public void phiToMove() {
        for (Function function : module.getDecledFunctions()) {
            ArrayList<BasicBlock> blocks = new ArrayList<>(function.getBlocks());
            for (BasicBlock block : blocks) {
                ArrayList<Instruction> instructions = new ArrayList<>(block.getInstructionList());
                for (Instruction instruction : instructions) {
                    if (instruction instanceof Phi phi) {
                        phiToMoveForInstr(phi);
                        block.removeInstruction(phi);
                        //phi不能destroy，因为move实际上是对phi的value进行的赋值
                        //但是phi需要取消所有use，这会被move继承
                        for (Value operand : phi.getOperandList()) {
                            operand.removeUser(phi);
                        }
                    }
                }
            }
        }

        //之后，删除多余的move指令
        sortMove();
    }

    private void phiToMoveForInstr(Phi phi) {
        BasicBlock block = phi.getBlock();
        //获取其前驱块集合
        ArrayList<BasicBlock> prevBlocks = new ArrayList<>(Optimizer.instance().getCFG().getParents(block));
        for (BasicBlock prevBlock : prevBlocks) {
            int childrenNumber = Optimizer.instance().getCFG().getChildren(prevBlock).size();
            Phi phi_temp = new Phi(phi.getType(), IRManager.getInstance().declareLocalVar());

            if (childrenNumber == 1) {
                //直接将move指令加在块尾
                prevBlock.addInstructionAtMoveEntry(new MoveIR(phi_temp, phi.valueFromBlock(prevBlock)));
                prevBlock.addInstructionAtMoveTail(new MoveIR(phi, phi_temp));
            } else {
                //将move指令加在新建的中间块内
                //可能有多个phi指令选项来自同一个前驱，这时不能新建中间块而需要复用
                Value v = phi.valueFromBlock(prevBlock);
                BasicBlock midBlock = new BasicBlock();
                midBlock.addInstruction(new Br(block));
                prevBlock.redirectTo(block, midBlock);
                block.getFunction().addBlockBefore(block, midBlock);
                midBlock.addInstructionAtMoveEntry(new MoveIR(phi_temp, v));
                midBlock.addInstructionAtMoveTail(new MoveIR(phi, phi_temp));

//                BasicBlock midBlock = new BasicBlock();
//                midBlock.addInstruction(new MoveIR(phi, phi.valueFromBlock(prevBlock)));
//                midBlock.addInstruction(new Br(block));
//                prevBlock.redirectTo(block, midBlock);
//                block.getFunction().addBlockBefore(block, midBlock);

                //加入了中间块，那它一定被前驱支配，而不会支配后继
                //加入了中间块，重新分析
                Optimizer.instance().getCFG().addMidBlock(prevBlock, block, midBlock);
                // 不会用到支配信息，不用继续维护了
//                Optimizer.instance().getDominAnalyzer().addBlockBetween(prevBlock, block, midBlock);
            }
        }
    }

    private void sortMove() {
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock block : f.getBlocks()) {
                ArrayList<MoveIR> moves = new ArrayList<>();
                for (Instruction i : block.getInstructionList()) {
                    if (i instanceof MoveIR moveIR) {
                        moves.add(moveIR);
                    }
                }

                if (moves.isEmpty()) {
                    continue;
                }

                // 最近一次对该地址的mv
                HashMap<Phi, MoveIR> phi2Move = new HashMap<>();
                // 被使用过的src，这些src如果出现在phi里是不能被合并的
                HashMap<Phi, MoveIR> src2Move = new HashMap<>();

                for (MoveIR moveIR : moves) {
                    phi2Move.put(moveIR.getOriginPhi(), moveIR);
                    Value src = moveIR.getSrc();
                    if (src instanceof Phi srcPhi) {
                        // 如果此move的src恰好等于之前的某个phi，而且这个move的phi的原值并没有被使用过（未出现在src2Move），则可以合并
                        if (phi2Move.containsKey(srcPhi) && !src2Move.containsKey(moveIR.getOriginPhi())) {
                            MoveIR prevMove = phi2Move.get(srcPhi);
                            MoveIR newMove = new MoveIR(moveIR.getOriginPhi(), prevMove.getSrc());
                            block.addInstructionAt(block.getInstructionList().indexOf(prevMove), newMove);
                            block.removeInstruction(prevMove);
                            block.removeInstruction(moveIR);
                            prevMove.destroy();
                            moveIR.destroy();
                            src2Move.put(srcPhi, newMove);
                            phi2Move.put(newMove.getOriginPhi(), newMove);
                        } else {
                            src2Move.put(srcPhi, moveIR);
                        }
                    }
                }
            }
        }
    }
}