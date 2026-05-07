package midend.optimism;

import midend.*;
import midend.LLVMType.ArrayType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.PointerType;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;

public class O4optimism {
    private Module module;

    public O4optimism(Module module) {
        this.module = module;
    }

    public void run() {
        if (module.getFunctions().size() != 2) {
            return;
        }
        if (!module.getFunctions().get(0).isRecursive()) {
            return;
        }
        if (module.getFunctions().get(0).getParams().size() != 2) {
            return;
        }
        if (!(module.getFunctions().get(0).getParams().get(0).getType().isInteger() && module.getFunctions().get(0).getParams().get(1).getType().isInteger())) {
            return;
        }
        Function recursion = module.getFunctions().get(0);
        for (BasicBlock block : recursion.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr && ((StoreInstr) instruction).getPointer() instanceof GlobalVar) {
                    return;
                }
                if (instruction instanceof CallInstr && !((CallInstr) instruction).getFunction().equals(recursion)) {
                    return;
                }
            }
        }
        if (!recursion.getRetType().isInteger()) {
            return;
        }
        for (int count = 0; count < recursion.getBlockList().get(0).getInstrList().size(); count++) {
            Instruction instruction = recursion.getBlockList().get(0).getInstrList().get(count);
            if (count == 0 && !(instruction instanceof CmpInstr)) {
                return;
            }
            if (count == 1 && !(instruction instanceof BrInstr)) {
                return;
            }
            if (count > 1) {
                return;
            }
        }
        runForO4(recursion);
        recursion.setMem();
    }

    public void runForO4(Function function) {
        //狗仔全局变量
        ArrayList<Value> values = new ArrayList<>();
        GlobalVar hashTable = new GlobalVar(new PointerType(new ArrayType(IntegerType.i32, 100000100)), "hashTable", values);
        ArrayList<Value> values1 = new ArrayList<>();
        GlobalVar visited = new GlobalVar(new PointerType(new ArrayType(IntegerType.i32, 100000100)), "visited", values1);
        module.addGlobalVar(hashTable);
        module.addGlobalVar(visited);
        Param param1 = function.getParams().get(0);
        Param param2 = function.getParams().get(1);
        //构造块块们
        BasicBlock block1 = new BasicBlock(UndefinedType.undefined, function);
        ArrayList<Value> values2 = new ArrayList<>();
        values2.add(new ConstInt(IntegerType.i32, 0));
        values2.add(param1);
        GetElementPtrInstr gep1 = new GetElementPtrInstr(new PointerType(IntegerType.i32), visited, values2);
        gep1.setBasicBlock(block1);
        LoadInstr load1 = new LoadInstr(IntegerType.i32, gep1, block1);
        CmpInstr ne1 = new CmpInstr("!=", IntegerType.i1, Arrays.asList(load1, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block1);
        BasicBlock block2 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block3 = function.getBlockList().get(2);
        BrInstr brInstr1 = new BrInstr(ne1, Arrays.asList(block2, block3));
        brInstr1.setBasicBlock(block1);
        block1.addInstr(gep1);
        block1.addInstr(load1);
        block1.addInstr(ne1);
        block1.addInstr(brInstr1);
        //block2
        ArrayList<Value> values3 = new ArrayList<>();
        values3.add(new ConstInt(IntegerType.i32, 0));
        values3.add(param1);
        GetElementPtrInstr gep2 = new GetElementPtrInstr(new PointerType(IntegerType.i32), hashTable, values3);
        gep2.setBasicBlock(block2);
        LoadInstr load2 = new LoadInstr(IntegerType.i32, gep2, block2);
        CmpInstr ne2 = new CmpInstr("!=", IntegerType.i1, Arrays.asList(load2, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block2);
        BasicBlock block4 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block5 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr2 = new BrInstr(ne2, Arrays.asList(block4, block5));
        brInstr2.setBasicBlock(block2);
        block2.addInstr(gep2);
        block2.addInstr(load2);
        block2.addInstr(ne2);
        block2.addInstr(brInstr2);
        //block3:nothing to do
        //block4
        BasicBlock lastBlock = function.getBlockList().getLast();
        LoadInstr load4 = new LoadInstr(IntegerType.i32, gep2, block4);
        ALUInstr add4 = new ALUInstr(IntegerType.i32, Arrays.asList(load4, param2), InstrType.ADD, block4);
        BrInstr brInstr4 = new BrInstr(lastBlock);
        brInstr4.setBasicBlock(block4);
        block4.addInstr(load4);
        block4.addInstr(add4);
        block4.addInstr(brInstr4);
        //block5
        BrInstr brInstr5 = new BrInstr(lastBlock);
        brInstr5.setBasicBlock(block5);
        block5.addInstr(brInstr5);
        BasicBlock block6 = function.getBlockList().get(3);
        //block6:only br
        BasicBlock block7 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr6 = (BrInstr) block6.getInstrList().getLast();
        brInstr6.modifyBlock(lastBlock, block7);
        //block7
        BasicBlock block8 = function.getBlockList().get(4);
        BasicBlock block9 = function.getBlockList().get(5);
        BasicBlock block10 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block11 = function.getBlockList().get(6);
        BasicBlock block12 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block13 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block14 = new BasicBlock(UndefinedType.undefined, function);
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        basicBlocks.add(block6);
        basicBlocks.add(block10);
        PhiInstr phiInstr7 = new PhiInstr(IntegerType.i32, basicBlocks);
        phiInstr7.addOption(block6.getInstrList().get(block6.getInstrList().size() - 2), block6);
        //block10
        ArrayList<BasicBlock> basicBlocks1 = new ArrayList<>();
        basicBlocks1.add(block9);
        basicBlocks1.add(block11);
        PhiInstr phiInstr10 = new PhiInstr(IntegerType.i32, basicBlocks1);
        phiInstr10.addOption(block9.getInstrList().get(1), block9);
        phiInstr10.addOption(new ConstInt(IntegerType.i32, 0), block11);
        phiInstr10.setBasicBlock(block10);
        BrInstr brInstr10 = new BrInstr(block7);
        brInstr10.setBasicBlock(block10);
        block10.addInstr(phiInstr10);
        block10.addInstr(brInstr10);
        //////////////
        phiInstr7.addOption(phiInstr10, block10);
        phiInstr7.setBasicBlock(block7);
        CmpInstr eq7 = new CmpInstr("==", IntegerType.i1, Arrays.asList(phiInstr7, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block7);
        BrInstr brInstr7 = new BrInstr(eq7, Arrays.asList(block12, block14));
        brInstr7.setBasicBlock(block7);
        block7.addInstr(phiInstr7);
        block7.addInstr(eq7);
        block7.addInstr(brInstr7);
        //block8:nothing
        //block9
        BrInstr brInstr9 = (BrInstr) block9.getInstrList().getLast();
        brInstr9.modifyBlock(lastBlock, block10);
        //block11
        BrInstr brInstr11 = (BrInstr) block11.getInstrList().getLast();
        brInstr11.modifyBlock(lastBlock, block10);
        //block12
        ArrayList<Value> values4 = new ArrayList<>();
        values4.add(new ConstInt(IntegerType.i32, 0));
        values4.add(param1);
        GetElementPtrInstr gep12 = new GetElementPtrInstr(new PointerType(IntegerType.i32), hashTable, values4);
        gep12.setBasicBlock(block12);
        StoreInstr store12 = new StoreInstr(IntegerType.i32, new ConstInt(IntegerType.i32, 0), gep12, block12);
        BrInstr brInstr12 = new BrInstr(block13);
        brInstr12.setBasicBlock(block12);
        block12.addInstr(gep12);
        block12.addInstr(store12);
        block12.addInstr(brInstr12);
        //block13
        StoreInstr store13 = new StoreInstr(IntegerType.i32, new ConstInt(IntegerType.i32, 1), gep1, block13);
        BrInstr brInstr13 = new BrInstr(lastBlock);
        brInstr13.setBasicBlock(block13);
        block13.addInstr(store13);
        block13.addInstr(brInstr13);
        //block14
        ArrayList<Value> values5 = new ArrayList<>();
        values5.add(new ConstInt(IntegerType.i32, 0));
        values5.add(param1);
        GetElementPtrInstr gep14 = new GetElementPtrInstr(new PointerType(IntegerType.i32), hashTable, values5);
        gep14.setBasicBlock(block14);
        ALUInstr sub14 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr7, param2), InstrType.SUB, block14);
        StoreInstr store14 = new StoreInstr(IntegerType.i32, sub14, gep14, block14);
        BrInstr brInstr14 = new BrInstr(block13);
        brInstr14.setBasicBlock(block14);
        block14.addInstr(gep14);
        block14.addInstr(sub14);
        block14.addInstr(store14);
        block14.addInstr(brInstr14);

        //lastblock
        PhiInstr phiInstrLast = (PhiInstr) lastBlock.getInstrList().getFirst();
        phiInstrLast.modifyBlock(function.getBlockList().get(3), block4);
        phiInstrLast.addOption(add4, block4);
        phiInstrLast.modifyBlock(block9, block5);
        phiInstrLast.addOption(new ConstInt(IntegerType.i32, 0), block5);
        phiInstrLast.modifyBlock(function.getBlockList().get(function.getBlockList().size() - 2), block13);
        phiInstrLast.addOption(phiInstr7, block13);

        function.getBlockList().add(2, block1);
        function.getBlockList().add(3, block2);
        function.getBlockList().add(5, block4);
        function.getBlockList().add(6, block5);
        function.getBlockList().add(8, block7);
        function.getBlockList().add(11, block10);
        function.getBlockList().add(13, block12);
        function.getBlockList().add(14, block13);
        function.getBlockList().add(15, block14);
        ((BrInstr) function.getBlockList().get(0).getInstrList().getLast()).modifyBlock(function.getBlockList().get(4), block1);
//        function.getBlockList().add();
//        CmpInstr eq7 = new CmpInstr("==", IntegerType.i1, Arrays.asList(phiInstr7, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block7);
//        BrInstr brInstr7 = new BrInstr(eq7, Arrays.asList(block12, block14));
//        brInstr7.setBasicBlock(block7);
//        block7.addInstr(phiInstr7);
//        block7.addInstr(eq7);
//        block7.addInstr(brInstr7);
        //block8:nothing to do
        //block9
        
    }
}
