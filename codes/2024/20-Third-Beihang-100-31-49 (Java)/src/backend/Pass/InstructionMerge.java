package backend.Pass;

import backend.instructions.BackBinary;
import backend.instructions.BackBranch;
import backend.instructions.BackCall;
import backend.instructions.BackInstruction;
import backend.instructions.BackLoad;
import backend.instructions.BackMov;
import backend.instructions.BackRet;
import backend.instructions.BackStore;
import backend.operand.BackIReg;
import backend.operand.BackImm;
import backend.operand.BackImm12;
import backend.operand.BackOperand;
import backend.operand.BackRegTable;
import backend.tools.BackBlock;
import backend.tools.BackFunction;
import backend.tools.BackModule;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class InstructionMerge {
    private final BackModule backModule;

    public InstructionMerge(BackModule backModule) {
        this.backModule = backModule;
    }

    public boolean optimize() {
        boolean flag = false;
        flag = peepHole();
        flag |= deadRegElim();
        flag |= loadStoreMerge();
        return flag;
    }

    private boolean peepHole() {
        boolean flag = false;
        BackIReg t6 = BackRegTable.getBackReg("t6");
        BackIReg sp = BackRegTable.getBackReg("sp");
        for (BackFunction backFunction : backModule.getFunctions()) {
            for (BackBlock block : backFunction.getBackBlocks()) {
                for (int i = 0; i < block.getBackInstructions().size(); i++) {
                    BackInstruction backInstruction = block.getBackInstructions().get(i);
                    //考虑这样的序列：li t6, a; addi r0, t6, b; 可以转为li r0, a+b, 这对应于gep n的情况
                    if (backInstruction instanceof BackMov mov && mov.getSrc() instanceof BackImm imm && mov.getDst().equals(t6) &&
                            i < block.getBackInstructions().size() - 1 && block.getBackInstructions().get(i + 1) instanceof BackBinary binary
                            && binary.getName().equals("addi") && binary.getSrc1().equals(t6)) {
                        BackImm newImm = new BackImm(imm.getImm() + ((BackImm) binary.getSrc2()).getImm());
                        BackMov newMove = new BackMov(binary.getDst(), newImm);
                        block.getBackInstructions().add(i, newMove);
                        block.getBackInstructions().remove(i + 2);
                        flag = true;
                    }


                    // 考虑的序列是：li r0, a; add r0, r0, sp; store/load r1, 0(r0); 可以将最后一个转为 sl r1, a(sp)
                    if (backInstruction instanceof BackMov mov && mov.getSrc() instanceof BackImm imm &&
                            i < block.getBackInstructions().size() - 2 && block.getBackInstructions().get(i + 1) instanceof BackBinary binary
                            && binary.getName().equals("add") && binary.getSrc1().equals(binary.getDst()) && mov.getDst().equals(binary.getDst()) &&
                            binary.getSrc2().equals(sp) && (block.getBackInstructions().get(i + 2) instanceof BackLoad backLoad &&
                            backLoad.getBase().equals(binary.getDst()) && backLoad.getOffset() == 0)) {
                        BackLoad newLoad = new BackLoad(backLoad.getName(), backLoad.getDst(), new BackImm12(imm.getImm()), sp);
                        block.getBackInstructions().remove(i + 2);
                        block.getBackInstructions().add(i + 2, newLoad);
                        flag = true;
                    }

                    if (backInstruction instanceof BackMov mov && mov.getSrc() instanceof BackImm imm &&
                            i < block.getBackInstructions().size() - 2 && block.getBackInstructions().get(i + 1) instanceof BackBinary binary
                            && binary.getName().equals("add") && binary.getSrc1().equals(binary.getDst()) && mov.getDst().equals(binary.getDst()) &&
                            binary.getSrc2().equals(sp) && (block.getBackInstructions().get(i + 2) instanceof BackStore backStore &&
                            backStore.getBase().equals(binary.getDst()) && backStore.getOffset() == 0)) {
                        BackStore newStore = new BackStore(backStore.getName(), backStore.getSrc(), new BackImm12(imm.getImm()), sp);
                        block.getBackInstructions().remove(i + 2);
                        block.getBackInstructions().add(i + 2, newStore);
                        flag = true;
                    }
                }
            }
        }


        return flag;
    }

    private boolean deadRegElim() {
        boolean flag = false;
        for (BackFunction backFunction : backModule.getFunctions()) {
            HashMap<BackOperand, BackInstruction> dstToInstr = new HashMap<>();
            HashSet<BackInstruction> deadInstructions = new HashSet<>();
            for (BackBlock backBlock : backFunction.getBackBlocks()) {
                deadInstructions.clear();
                dstToInstr.clear();
                for (BackInstruction backInstruction : backBlock.getBackInstructions()) {
                    for (BackOperand backOperand : backInstruction.getOperands()) {
                        if (!(backOperand instanceof BackImm)) {
                            dstToInstr.remove(backOperand);
                        }
                    }

                    if (backInstruction instanceof BackCall) {
                        // 设置的所有参数寄存器是有效的，所有其他临时寄存器的赋值是无效的
                        for (BackOperand backOperand : new HashSet<>(dstToInstr.keySet())) {
                            if (backOperand instanceof BackIReg reg) {
                                if (reg.isParamRegs()) {
                                    dstToInstr.remove(reg);
                                } else if (reg.isTempRegs()) {
                                    deadInstructions.add(dstToInstr.get(reg));
                                    dstToInstr.remove(reg);
                                    flag = true;
                                }
                            }
                        }
                        continue;
                    }


                    BackOperand dst = backInstruction.getDst();
                    if (dst != null) {
                        if (dstToInstr.containsKey(dst)) {
                            deadInstructions.add(dstToInstr.get(dst));
                            flag = true;
                        } else if (backInstruction instanceof BackMov mov && dstToInstr.containsKey(mov.getSrc())) {
                            // r0 <- a ... mv r1, r0 若过程中没有用到r1，则可以将此指令以及期间所有指令的r0转为r1，然后删去此mv
                            BackOperand mvSrc = mov.getSrc();
                            ArrayList<BackInstruction> instructions = new ArrayList<>();
                            BackInstruction src = dstToInstr.get(mvSrc);
                            boolean inRange = false;
                            for (BackBlock block : backFunction.getBackBlocks()) {
                                for (BackInstruction instruction : block.getBackInstructions()) {
                                    if (instruction.equals(src)) {
                                        inRange = true;
                                    }
                                    if (inRange) {
                                        instructions.add(instruction);
                                    }
                                    if (instruction.equals(mov)) {
                                        break;
                                    }
                                }
                            }
                            if (instructions.stream().noneMatch(
                                    i -> i.getOperands().contains(dst) || dst.equals(i.getDst()))) {
                                // 不考虑call，因为上面已经处理过了
                                for (BackInstruction instr : instructions) {
                                    instr.replaceOperand(mvSrc, dst);
                                }
                                deadInstructions.add(mov);
                                dstToInstr.put(dst, dstToInstr.get(mvSrc));
                                dstToInstr.remove(mvSrc);
                                continue;
                            }
                        }
                        dstToInstr.put(dst, backInstruction);
                    }
                }
                // 可能有其他块的跳入，保守起见不能跨块
                backBlock.getBackInstructions().removeAll(deadInstructions);
            }
        }
        return flag;
    }

    private boolean loadStoreMerge() {
        // 从store开始，只要src未被破坏，则可以将后续所有的load改写为move
        // 多个未被load或call，即被覆盖的store指令可以只取最后一个
        // 考虑的地址是off(sp)
        boolean flag = false;
        for (BackFunction backFunction : backModule.getFunctions()) {
            HashMap<Integer, BackStore> addrToStore = new HashMap<>();
            HashMap<Integer, BackOperand> addrToSrc = new HashMap<>();
            HashSet<BackInstruction> deadInstructions = new HashSet<>();

            for (BackBlock backBlock : backFunction.getBackBlocks()) {
                deadInstructions.clear();
                addrToStore.clear();
                addrToSrc.clear();
                for (BackInstruction backInstruction : new ArrayList<>(backBlock.getBackInstructions())) {
                    if (backInstruction instanceof BackCall) {
                        deadInstructions.clear();
                        addrToStore.clear();
                        addrToSrc.clear();
                    }
                    BackOperand dst = backInstruction.getDst();
                    if (dst instanceof BackIReg reg) {
                        if (backInstruction instanceof BackLoad backLoad && backLoad.getOffset() != null) {
                            if (!backLoad.getBase().equals(BackRegTable.getBackReg("sp"))) {
                                // 如果取得寄存器不是sp，那就不能确定是哪个被取出了
                                addrToSrc.clear();
                                addrToStore.clear();
                            } else {
                                int offset = backLoad.getOffset();
                                if (addrToSrc.containsKey(offset)) {
                                    backBlock.getBackInstructions().set(backBlock.getBackInstructions().indexOf(backLoad),
                                            new BackMov(reg, addrToSrc.get(offset)));
                                    flag = true;
                                }
                                addrToStore.remove(offset);
                            }
                        }

                        // 也不能作为src代替了
                        HashSet<Integer> uncertainSrcs = new HashSet<>();
                        for (Integer addr : addrToSrc.keySet()) {
                            if (addrToSrc.get(addr).equals(reg)) {
                                uncertainSrcs.add(addr);
                            }
                        }
                        uncertainSrcs.forEach(addrToSrc::remove);
                    }

                    // 如果是sw，且addr相同，则可以删掉了
                    if (backInstruction instanceof BackStore backStore && backStore.getOffset() != null) {
                        // 如果存的寄存器不是sp，那么就不能确定到底修改了哪个地址
                        if (!backStore.getBase().equals(BackRegTable.getBackReg("sp"))) {
                            addrToSrc.clear();
                            addrToStore.clear();
                        } else {
                            int offset = backStore.getOffset();
                            if (addrToStore.containsKey(offset)) {
                                deadInstructions.add(addrToStore.get(offset));
                                flag = true;
                                addrToStore.put(offset, backStore);
                            }
                            addrToSrc.put(offset, backStore.getSrc());
                        }
                    }
                }
                backBlock.getBackInstructions().removeAll(deadInstructions);
            }
        }
        return flag;
    }
}
