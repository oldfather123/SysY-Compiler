package cn.edu.nju.software.pass;

import cn.edu.nju.software.ir.basicblock.BasicBlockRef;
import cn.edu.nju.software.ir.instruction.Instruction;
import cn.edu.nju.software.ir.instruction.Load;
import cn.edu.nju.software.ir.instruction.Store;
import cn.edu.nju.software.ir.module.ModuleRef;
import cn.edu.nju.software.ir.value.FunctionValue;
import cn.edu.nju.software.ir.value.ValueRef;

public class EliminateLoadStorePass implements ModulePass {
    private static EliminateLoadStorePass instance;
    public static EliminateLoadStorePass getInstance() {
        if (instance == null) {
            instance = new EliminateLoadStorePass();
        }
        return instance;
    }
    private EliminateLoadStorePass() {}

    private int pairs = 0;
    @Override
    public boolean runOnModule(ModuleRef module) {
        for (int i = 0; i < module.getFunctionNum(); i++) {
            FunctionValue fv = module.getFunction(i);
            if (fv.isLib()) {
                continue;
            }
            for (int j = 0; j < fv.getBlockNum(); j++) {
                BasicBlockRef bb = fv.getBlock(j);
                for (int k = 0; k < bb.getIrNum(); k++) {
                    Instruction instruction = bb.getIr(k);
                    if (instruction instanceof Store store) {
                        if (k != 0) {
                            Instruction before = bb.getIr(k - 1);
                            if (before instanceof Load load) {
                                ValueRef destMemory = store.getOperand(1), srcMemory = load.getOperand(0);
                                if (destMemory.equals(srcMemory)) {
                                    // no need for this pair of load|store
                                    bb.dropIr(before);
                                    bb.dropIr(instruction);
                                    k -= 2;
                                    pairs++;
                                }
                            }
                        }
                    }
                }
            }
        }
//        System.err.println("redundancy: " + pairs);
        return false;
    }

    @Override
    public String getName() {
        return "";
    }

    @Override
    public void printDbgInfo() {

    }

    @Override
    public void setDbgFlag() {

    }
}
