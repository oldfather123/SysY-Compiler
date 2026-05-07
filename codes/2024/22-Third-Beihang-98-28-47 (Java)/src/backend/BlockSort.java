package backend;

import Utils.CustomList;
import backend.asmInstr.AsmInstr;
import backend.asmInstr.asmBr.AsmBnez;
import backend.asmInstr.asmBr.AsmJ;
import backend.itemStructure.*;
import frontend.ir.Value;
import frontend.syntax.Ast;

import java.util.HashMap;

public class BlockSort {
    private BlockSort() {
        // 初始化逻辑
    }

    // 单例模式下的私有静态成员变量
    private static BlockSort instance = null;

    // 提供一个公共的静态方法，用于获取单例对象
    public static BlockSort getInstance() {
        if (instance == null) {
            instance = new BlockSort();
        }
        return instance;
    }

    public void run(AsmModule module) {
        for (AsmFunction function : module.getFunctions()) {
            //initial();
            //CollectionInfo(function);
            selectMergeBlock(function);
        }
    }

    HashMap<Group, Double> BlocksEdge = new HashMap<>();

    private void initial() {
        BlocksEdge.clear();
    }

    private void CollectionInfo(AsmFunction function) {

    }

    private void selectMergeBlock(AsmFunction function) {
        AsmBlock asmBlock = (AsmBlock) (function.getBlocks().getHead());
        while (asmBlock != null) {
            while (asmBlock.getInstrTail() instanceof AsmJ asmJ && asmJ.getTarget().jumpToInstrs.size() == 1) {
                mergeBlock(asmBlock, asmJ.getTarget());
                asmJ.getTarget().removeJumpToInstr(asmJ);
            }
            asmBlock = (AsmBlock) asmBlock.getNext();
        }
    }

    private void mergeBlock(AsmBlock i, AsmBlock j) {//把j合进i
//        if (j.getInstrTail() == null) {
//            throw new RuntimeException(j.getName());
//        }
        i.getInstrTail().removeFromList();
        i.getInstrs().addCustomListToTail(j.getInstrs());
        //这个地方其实有点问题，但是希望以后不要用前驱后继了(
        i.sucs.remove(j);
        i.sucs.addAll(j.sucs);
        for (CustomList.Node instr : j.getInstrs()) {
            if (instr instanceof AsmJ asmJ) {
                AsmBlock target = asmJ.getTarget();
                target.pres.remove(j);
                target.pres.add(i);
            }
            if (instr instanceof AsmBnez asmBnez) {
                AsmBlock target = asmBnez.getTarget();
                target.pres.remove(j);
                target.pres.add(i);
            }
        }
        j.removeFromList();
    }
}
