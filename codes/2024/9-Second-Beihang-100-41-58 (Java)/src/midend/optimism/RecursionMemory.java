package midend.optimism;

import midend.*;
import midend.LLVMType.*;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;

public class RecursionMemory {
    private Module module;

    public RecursionMemory(Module module) {
        this.module = module;
    }

    public void run() {
        if (module.getFunctions().size() != 2) {
            return;
        }
        if (!module.getFunctions().get(0).isRecursive()) {
            return;
        }
        Function recursion = module.getFunctions().get(0);
        if (recursion.getParams().get(0).getType().isFloat() && recursion.getParams().get(1).getType().isInteger()) {
//            for (int count = 0; count < recursion.getBlockList().size(); count++) {
//                BasicBlock block = recursion.getBlockList().get(count);
//                for (Instruction instruction : block.getInstrList()) {
//
//                }
//            }
            BasicBlock block = recursion.getBlockList().getFirst();
            if (!(block.getInstrList().get(0) instanceof CmpInstr && block.getInstrList().get(1) instanceof BrInstr)) {
                return;
            }
            for (BasicBlock basicBlock : recursion.getBlockList()) {
                for (Instruction instruction : basicBlock.getInstrList()) {
                    if (instruction instanceof StoreInstr) {
                        if (((StoreInstr) instruction).getPointer() instanceof GlobalVar) {
                            return;
                        }
                    }
                    if (instruction instanceof CallInstr) {
                        if (!(((CallInstr) instruction).getFunction().equals(recursion))) {
                            return;
                        }
                    }
                }
            }
            recursionForFloatInt(recursion);
            recursion.setMem();
        }
    }

    public void recursionForFloatInt(Function function) {
        //构造全局变量
        ArrayList<Value> values = new ArrayList<>();
        GlobalVar globalVar = new GlobalVar(new PointerType(new ArrayType(FloatType.f32, 10000)), "hashTable", values);
        ArrayList<Value> values1 = new ArrayList<>();
        GlobalVar globalVar1 = new GlobalVar(new PointerType(new ArrayType(IntegerType.i32, 10000)), "visited", values1);
        module.addGlobalVar(globalVar);
        module.addGlobalVar(globalVar1);
        //构造哈希
        Param floatParam = function.getParams().get(0);
        Param intParam = function.getParams().get(1);
        BasicBlock block = new BasicBlock(UndefinedType.undefined, function);
        ConversionInstr conversionInstr = new ConversionInstr(FloatType.f32, InstrType.SITOFP, new ConstInt(IntegerType.i32, 100));
        conversionInstr.setBasicBlock(block);
        ALUInstr mul = new ALUInstr(FloatType.f32, Arrays.asList(floatParam, conversionInstr), InstrType.FMUL, block);
        ConversionInstr conversionInstr1 = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul);
        conversionInstr1.setBasicBlock(block);
        CmpInstr slt = new CmpInstr("<", IntegerType.i1, Arrays.asList(conversionInstr1, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block);
        BasicBlock block1 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block2 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr = new BrInstr(slt, Arrays.asList(block1, block2));
        brInstr.setBasicBlock(block);
        block.addInstr(conversionInstr);
        block.addInstr(mul);
        block.addInstr(conversionInstr1);
        block.addInstr(slt);
        block.addInstr(brInstr);
        //block1
        ALUInstr aluInstr = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), conversionInstr1), InstrType.SUB, block1);
        BrInstr brInstr1 = new BrInstr(block2);
        brInstr1.setBasicBlock(block1);
        block1.addInstr(aluInstr);
        block1.addInstr(brInstr1);
        //block2
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        basicBlocks.add(block);
        basicBlocks.add(block1);
        PhiInstr phiInstr = new PhiInstr(IntegerType.i32, basicBlocks);
        phiInstr.addOption(conversionInstr1, block);
        phiInstr.addOption(aluInstr, block1);
        phiInstr.setBasicBlock(block2);
        ALUInstr sub1 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr, intParam), InstrType.SUB, block2);
        ALUInstr add = new ALUInstr(IntegerType.i32, Arrays.asList(sub1, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, block2);
        CmpInstr slt1 = new CmpInstr("<", IntegerType.i1, Arrays.asList(add, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block2);
        BasicBlock block3 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block4 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr2 = new BrInstr(slt1, Arrays.asList(block3, block4));
        brInstr2.setBasicBlock(block2);
        block2.addInstr(phiInstr);
        block2.addInstr(sub1);
        block2.addInstr(add);
        block2.addInstr(slt1);
        block2.addInstr(brInstr2);
        //block3
        BasicBlock block5 = new BasicBlock(UndefinedType.undefined, function);
        ALUInstr aluInstr1 = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), add), InstrType.SUB, block3);
        BrInstr brInstr3 = new BrInstr(block5);
        brInstr3.setBasicBlock(block3);
        block3.addInstr(aluInstr1);
        block3.addInstr(brInstr3);
        //block4
        BrInstr brInstr4 = new BrInstr(block5);
        brInstr4.setBasicBlock(block4);
        block4.addInstr(brInstr4);
        //block5
        ArrayList<BasicBlock> basicBlocks1 = new ArrayList<>();
        basicBlocks1.add(block3);
        basicBlocks1.add(block4);
        PhiInstr phiInstr1 = new PhiInstr(IntegerType.i32, basicBlocks1);
        phiInstr1.addOption(aluInstr1, block3);
        phiInstr1.addOption(add, block4);
        phiInstr1.setBasicBlock(block5);
        ALUInstr sdiv = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr1, new ConstInt(IntegerType.i32, 10000)), InstrType.SDIV, block5);
        ALUInstr mul1 = new ALUInstr(IntegerType.i32, Arrays.asList(sdiv, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, block5);
        ALUInstr sub2 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr1, mul1), InstrType.SUB, block5);
        CmpInstr slt2 = new CmpInstr("<", IntegerType.i1, Arrays.asList(sub2, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block5);
        BasicBlock block6 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block7 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr5 = new BrInstr(slt2, Arrays.asList(block6, block7));
        brInstr5.setBasicBlock(block5);
        block5.addInstr(phiInstr1);
        block5.addInstr(sdiv);
        block5.addInstr(mul1);
        block5.addInstr(sub2);
        block5.addInstr(slt2);
        block5.addInstr(brInstr5);
        //block6
        ALUInstr sub6 = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), sub2), InstrType.SUB, block6);
        block6.addInstr(sub6);
        BrInstr brInstr6 = new BrInstr(block7);
        brInstr6.setBasicBlock(block6);
        block6.addInstr(brInstr6);
        //block7
        ArrayList<BasicBlock> basicBlocks2 = new ArrayList<>();
        basicBlocks2.add(block5);
        basicBlocks2.add(block6);
        PhiInstr phiInstr7 = new PhiInstr(IntegerType.i32, basicBlocks2);
        phiInstr7.addOption(sub2, block5);
        phiInstr7.addOption(sub6, block6);
        phiInstr7.setBasicBlock(block7);
        BasicBlock block8 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr7 = new BrInstr(block8);
        brInstr7.setBasicBlock(block7);
        block7.addInstr(phiInstr7);
        block7.addInstr(brInstr7);
        //block8 **********
        ConversionInstr cvs8 = new ConversionInstr(IntegerType.i32, InstrType.ZEXT, new ConstInt(IntegerType.i1, 0));
        cvs8.setBasicBlock(block8);
        ConversionInstr cvs82 = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, floatParam);
        cvs82.setBasicBlock(block8);
        ALUInstr mul8 = new ALUInstr(IntegerType.i32, Arrays.asList(cvs82, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, block8);
        ALUInstr shl8 = new ALUInstr(IntegerType.i32, Arrays.asList(intParam, new ConstInt(IntegerType.i32, 8)), InstrType.SHL, block8);
        ALUInstr add8 = new ALUInstr(IntegerType.i32, Arrays.asList(mul8, shl8), InstrType.ADD, block8);
        ALUInstr shl82 = new ALUInstr(IntegerType.i32, Arrays.asList(intParam, new ConstInt(IntegerType.i32, 6)), InstrType.SHL, block8);
        ALUInstr add82 = new ALUInstr(IntegerType.i32, Arrays.asList(add8, shl82), InstrType.ADD, block8);
        ALUInstr add83 = new ALUInstr(IntegerType.i32, Arrays.asList(add82, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, block8);
//        ALUInstr sdiv8 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr7, new ConstInt(IntegerType.i32, 10000)), InstrType.SDIV, block8);
//        ALUInstr mul8 = new ALUInstr(IntegerType.i32, Arrays.asList(sdiv8, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, block8);
//        ALUInstr sub8 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr7, mul8), InstrType.SUB, block8);
//        CmpInstr slt8 = new CmpInstr("<", IntegerType.i1, Arrays.asList(sub8, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block8);
        BasicBlock block9 = new BasicBlock(UndefinedType.undefined, function);
//        BasicBlock block10 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr8 = new BrInstr(block9);
        brInstr8.setBasicBlock(block8);
        block8.addInstr(cvs8);
        block8.addInstr(cvs82);
        block8.addInstr(mul8);
        block8.addInstr(shl8);
        block8.addInstr(add8);
        block8.addInstr(shl82);
        block8.addInstr(add82);
        block8.addInstr(add83);
        block8.addInstr(brInstr8);
        //block9
        BasicBlock block15 = new BasicBlock(UndefinedType.undefined, function);
        ArrayList<BasicBlock> basicBlocks3 = new ArrayList<>();
        basicBlocks3.add(block8);
        basicBlocks3.add(block15);
        PhiInstr phiInstr9 = new PhiInstr(IntegerType.i32, basicBlocks3);
        //block15
        ALUInstr add15 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr9, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, block15);
        BrInstr brInstr15 = new BrInstr(block9);
        brInstr15.setBasicBlock(block15);
        block15.addInstr(add15);
        block15.addInstr(brInstr15);
        //*********
        phiInstr9.addOption(new ConstInt(IntegerType.i32, 0), block8);
        phiInstr9.addOption(add15, block15);
        phiInstr9.setBasicBlock(block9);
        CmpInstr slt9 = new CmpInstr("<", IntegerType.i1, Arrays.asList(phiInstr9, new ConstInt(IntegerType.i32, 10000)), InstrType.ICMP, block9);
        BasicBlock block10 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block11 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr9 = new BrInstr(slt9, Arrays.asList(block10, block11));
        brInstr9.setBasicBlock(block9);
        block9.addInstr(phiInstr9);
        block9.addInstr(slt9);
        block9.addInstr(brInstr9);
        //block10
        ALUInstr mul10 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr9, new ConstInt(IntegerType.i32, 1)), InstrType.MUL, block10);
        ALUInstr add10 = new ALUInstr(IntegerType.i32, Arrays.asList(phiInstr7, mul10), InstrType.ADD, block10);
        ALUInstr div10 = new ALUInstr(IntegerType.i32, Arrays.asList(add10, new ConstInt(IntegerType.i32, 10000)), InstrType.SDIV, block10);
        ALUInstr mul101 = new ALUInstr(IntegerType.i32, Arrays.asList(div10, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, block10);
        ALUInstr sub10 = new ALUInstr(IntegerType.i32, Arrays.asList(add10, mul101), InstrType.SUB, block10);
        CmpInstr slt10 = new CmpInstr("<", IntegerType.i1, Arrays.asList(sub10, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block10);
        BasicBlock block12 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block13 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr10 = new BrInstr(slt10, Arrays.asList(block12, block13));
        brInstr10.setBasicBlock(block10);
        block10.addInstr(mul10);
        block10.addInstr(add10);
        block10.addInstr(div10);
        block10.addInstr(mul101);
        block10.addInstr(sub10);
        block10.addInstr(slt10);
        block10.addInstr(brInstr10);
        //block11
        BasicBlock block17 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr11 = new BrInstr(block17);
        brInstr11.setBasicBlock(block11);
        block11.addInstr(brInstr11);
        //block12
        ALUInstr sub12 = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), sub10), InstrType.SUB, block12);
        BrInstr brInstr12 = new BrInstr(block13);
        brInstr12.setBasicBlock(block12);
        block12.addInstr(sub12);
        block12.addInstr(brInstr12);
        //block13
        ArrayList<BasicBlock> basicBlocks4 = new ArrayList<>();
        basicBlocks4.add(block10);
        basicBlocks4.add(block12);
        PhiInstr phiInstr13 = new PhiInstr(IntegerType.i32, basicBlocks4);
        phiInstr13.addOption(sub10, block10);
        phiInstr13.addOption(sub12, block12);
        phiInstr13.setBasicBlock(block13);
        ArrayList<Value> values2 = new ArrayList<>();
        values2.add(new ConstInt(IntegerType.i32, 0));
        values2.add(phiInstr13);
        GetElementPtrInstr gep13 = new GetElementPtrInstr(new PointerType(IntegerType.i32), globalVar1, values2);
        gep13.setBasicBlock(block13);
        LoadInstr load13 = new LoadInstr(IntegerType.i32, gep13, block13);
        CmpInstr eq13 = new CmpInstr("==", IntegerType.i1, Arrays.asList(load13, cvs8), InstrType.ICMP, block13);
        BasicBlock block14 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block16 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr13 = new BrInstr(eq13, Arrays.asList(block14, block16));
        brInstr13.setBasicBlock(block13);
        block13.addInstr(phiInstr13);
        block13.addInstr(gep13);
        block13.addInstr(load13);
        block13.addInstr(eq13);
        block13.addInstr(brInstr13);
        //block14
        BrInstr brInstr14 = new BrInstr(block17);
        brInstr14.setBasicBlock(block14);
        block14.addInstr(brInstr14);
        //block16
        LoadInstr load16 = new LoadInstr(IntegerType.i32, gep13, block16);
        CmpInstr eq16 = new CmpInstr("==", IntegerType.i1, Arrays.asList(load16, add83), InstrType.ICMP, block16);
        BrInstr brInstr16 = new BrInstr(eq16, Arrays.asList(block14, block15));
        brInstr16.setBasicBlock(block16);
        block16.addInstr(load16);
        block16.addInstr(eq16);
        block16.addInstr(brInstr16);
        //block17
        ArrayList<BasicBlock> basicBlocks5 = new ArrayList<>();
        basicBlocks5.add(block11);
        basicBlocks5.add(block14);
        PhiInstr phiInstr17 = new PhiInstr(IntegerType.i32, basicBlocks5);
        phiInstr17.addOption(new ConstInt(IntegerType.i32, -1), block11);
        phiInstr17.addOption(phiInstr13, block14);
        phiInstr17.setBasicBlock(block17);
        ArrayList<Value> values3 = new ArrayList<>();
        values3.add(new ConstInt(IntegerType.i32, 0));
        values3.add(phiInstr17);
        GetElementPtrInstr gep17 = new GetElementPtrInstr(new PointerType(IntegerType.i32), globalVar1, values3);
        gep17.setBasicBlock(block17);
        LoadInstr load17 = new LoadInstr(IntegerType.i32, gep17, block17);
        CmpInstr ne17 = new CmpInstr("!=", IntegerType.i1, Arrays.asList(load17, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block17);
        BasicBlock block18 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block19 = function.getBlockList().get(function.getBlockList().size() - 2);
        BrInstr brInstr17 = new BrInstr(ne17, Arrays.asList(block18, block19));
        brInstr17.setBasicBlock(block17);
        block17.addInstr(phiInstr17);
        block17.addInstr(gep17);
        block17.addInstr(load17);
        block17.addInstr(ne17);
        block17.addInstr(brInstr17);
        //block18
        ArrayList<Value> values4 = new ArrayList<>();
        values4.add(new ConstInt(IntegerType.i32, 0));
        values4.add(phiInstr17);
        GetElementPtrInstr gep18 = new GetElementPtrInstr(new PointerType(FloatType.f32), globalVar, values4);
        gep18.setBasicBlock(block18);
        LoadInstr load18 = new LoadInstr(FloatType.f32, gep18, block18);
        BasicBlock block20 = function.getBlockList().getLast();
        BrInstr brInstr18 = new BrInstr(block20);
        brInstr18.setBasicBlock(block18);
        block18.addInstr(gep18);
        block18.addInstr(load18);
        block18.addInstr(brInstr18);
        //block19
        ArrayList<Value> values5 = new ArrayList<>();
        values5.add(new ConstInt(IntegerType.i32, 0));
        values5.add(phiInstr17);
        GetElementPtrInstr gep19 = new GetElementPtrInstr(new PointerType(FloatType.f32), globalVar, values5);
        gep19.setBasicBlock(block19);
        StoreInstr store19 = new StoreInstr(FloatType.f32, block19.getInstrList().get(block19.getInstrList().size() - 2), gep19, block19);
        ConversionInstr cvs19 = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, floatParam);
        cvs19.setBasicBlock(block19);
        ALUInstr mul19 = new ALUInstr(IntegerType.i32, Arrays.asList(cvs19, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, block19);
        ALUInstr add191 = new ALUInstr(IntegerType.i32, Arrays.asList(mul19, shl8), InstrType.ADD, block19);
        ALUInstr add192 = new ALUInstr(IntegerType.i32, Arrays.asList(add191, shl82), InstrType.ADD, block19);
        ALUInstr add193 = new ALUInstr(IntegerType.i32, Arrays.asList(add192, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, block19);
        StoreInstr store192 = new StoreInstr(IntegerType.i32, add193, gep17, block19);
        int count = block19.getInstrList().size() - 1;
        block19.getInstrList().add(count, store192);
        block19.getInstrList().add(count, add193);
        block19.getInstrList().add(count, add192);
        block19.getInstrList().add(count, add191);
        block19.getInstrList().add(count, mul19);
        block19.getInstrList().add(count, cvs19);
        block19.getInstrList().add(count, store19);
        block19.getInstrList().add(count, gep19);
        //block20
        PhiInstr phiInstr20 = (PhiInstr) block20.getInstrList().getFirst();
        phiInstr20.addOp(load18, block18);
        //加入块
        function.getBlockList().add(2, block18);
        function.getBlockList().add(2, block17);
        function.getBlockList().add(2, block16);
        function.getBlockList().add(2, block15);
        function.getBlockList().add(2, block14);
        function.getBlockList().add(2, block13);
        function.getBlockList().add(2, block12);
        function.getBlockList().add(2, block11);
        function.getBlockList().add(2, block10);
        function.getBlockList().add(2, block9);
        function.getBlockList().add(2, block8);
        function.getBlockList().add(2, block7);
        function.getBlockList().add(2, block6);
        function.getBlockList().add(2, block5);
        function.getBlockList().add(2, block4);
        function.getBlockList().add(2, block3);
        function.getBlockList().add(2, block2);
        function.getBlockList().add(2, block1);
        function.getBlockList().add(2, block);
        ((BrInstr) function.getBlockList().get(0).getInstrList().getLast()).setIfFalseBlock(block);
    }
}
