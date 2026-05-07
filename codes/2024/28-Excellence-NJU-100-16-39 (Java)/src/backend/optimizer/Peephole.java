package backend.optimizer;

import backend.RISCVCode.OperandType;
import backend.RISCVCode.RISCVBlock;
import backend.RISCVCode.RISCVFunction;
import backend.RISCVCode.RISCVInstruction.*;
import backend.RISCVCode.RISCVOperand;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * 窥孔优化
 * 1.移除多余的加载和存储：如果一个变量被加载后立即存储到同一个位置，我们可以移除这些指令
 * 2.强度削弱: 用代价更低的操作替换代价较高的操作。例如，用加法替换乘法，用位移操作替换乘法和除法
 * 3.合并相邻指令: 将连续的相同计算指令合并为一个指令。例如，连续的加法操作可以合并为一个加法
 */
public class Peephole implements OptForRISCV{
    @Override
    public void Optimize(RISCVFunction riscvFunction) {
        for (RISCVBlock block : riscvFunction.getBlocks()){
            List<RISCVInstruction> instructions = getRISCVInstructions(block);

            // 强度削弱
            List<Integer> indicesToRemove = new ArrayList<>();
            List<RISCVInstruction> instructionsToAdd = new ArrayList<>();
            List<Integer> addIndices = new ArrayList<>();

            for (Iterator<RISCVInstruction> iterator = instructions.iterator(); iterator.hasNext();) {
                RISCVInstruction instruction = iterator.next();
                if (instruction instanceof RISCVBinary mul && (((RISCVBinary)instruction).getOpt().equals("mul")
                || ((RISCVBinary)instruction).getOpt().equals("mulw"))){
                    // 乘法指令可以用位移操作替换
                    // 目的寄存器和源寄存器相同,立即数并且是2的幂次方
                    if(mul.getDest().getName().equals(mul.getSrc1().getName())) {
                        int index = instructions.indexOf(mul);
                        if (index > 0 && instructions.get(index - 1) instanceof RISCVLi li) {
                            int imm = Integer.parseInt(li.getOperand1().getName());
                            if ((imm & (imm - 1)) == 0) {
                                // 用位移操作替换乘法
                                indicesToRemove.add(index);
                                instructionsToAdd.add(new RISCVBinary(
                                        mul.getDest(), mul.getSrc1(),
                                        new RISCVOperand(OperandType.imm, String.valueOf(Integer.numberOfTrailingZeros(imm))), "slli"
                                ));
                                addIndices.add(index);
//                                System.err.println("mul to slli");
                            }
                        }
                    }
                } else if (instruction instanceof RISCVBinary div && (((RISCVBinary)instruction).getOpt().equals("div")
                        || ((RISCVBinary)instruction).getOpt().equals("divw"))){
                    // 除法指令如果是立即数并且是2的幂次方
                    if(div.getDest().equals(div.getSrc1())){
                        int index = instructions.indexOf(div);
                        if (index > 0 && instructions.get(index - 1) instanceof RISCVLi li) {
                            int imm = Integer.parseInt(li.getOperand1().getName());
                            if ((imm & (imm - 1)) == 0) {
                                // 用位移操作替换除法
                                indicesToRemove.add(index);
                                instructionsToAdd.add(new RISCVBinary(
                                        div.getDest(), div.getSrc1(),
                                        new RISCVOperand(OperandType.imm, String.valueOf(Integer.numberOfTrailingZeros(imm))), "srai"
                                ));
                                addIndices.add(index);
                                System.err.println("div to srai");
                            }
                        }
                    }
                }
            }
            // 合并相邻指令
            // 执行移除和添加操作
            for (int i = indicesToRemove.size() - 1; i >= 0; i--) {
                int index = indicesToRemove.get(i);
                instructions.remove(index);
            }
            for (int i = 0; i < instructionsToAdd.size(); i++) {
                int index = addIndices.get(i);
                instructions.add(index, instructionsToAdd.get(i));
            }
        }
    }

    private static List<RISCVInstruction> getRISCVInstructions(RISCVBlock block) {
        List<RISCVInstruction> instructions = block.getInstructions();
        // 如果一个变量被加载后立即存储到同一个位置，我们可以移除这些指令
        // 通过对每个基本块的指令进行遍历，如果发现一个加载指令后紧跟着一个存储指令，且两个指令的目的寄存器相同
        // 则可以移除这两个指令
        for (Iterator<RISCVInstruction> iterator = instructions.iterator(); iterator.hasNext();){
            RISCVInstruction instruction = iterator.next();
            if (instruction instanceof RISCVLd && iterator.hasNext()){
                RISCVInstruction nextInstruction = iterator.next();
                if (nextInstruction instanceof RISCVSd){
                    if (((RISCVLd) instruction).getDest().equals(((RISCVSd) nextInstruction).getDest())){
                        iterator.remove();
                        iterator.remove();
                        System.err.println("remove");
                    }
                }
            }
        }
        return instructions;
    }
}