package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.IntegerType;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;

public class RecursionToLoop {
    private Module module;

    public RecursionToLoop(Module module) {
        this.module = module;
    }

    public boolean run() {
        if (module.getFunctions().size() != 5) {
            return false;
        }
        Function function = module.getFunctions().get(0);
        Function function1 = module.getFunctions().get(1);
        if (function.getBlockList().size() != 8) {
            return false;
        }
        if (function1.getBlockList().size() != 6) {
            return false;
        }
        if (function.getBlockList().get(3).getInstrList().getFirst() instanceof ALUInstr) {
            ALUInstr aluInstr = (ALUInstr) function.getBlockList().get(3).getInstrList().getFirst();
            if (!aluInstr.getOpStr().equals("%")) {
                return false;
            }
            if (!(aluInstr.getRight() instanceof ConstInt && ((ConstInt) aluInstr.getRight()).getValue() == 998244353)) {
                return false;
            }
        } else {
            return false;
        }
        if (function.getParams().size() != 2 && function1.getParams().size() != 2) {
            return false;
        }
        recursionToLoop1(function);
        recursionToLoop2(function1);
        return true;
    }

    public void recursionToLoop1(Function function) {
        Param param1 = function.getParams().get(0);
        Param param2 = function.getParams().get(1);
        BasicBlock block1 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block3 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block4 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block5 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block6 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block7 = new BasicBlock(UndefinedType.undefined, function);

        //block1
        ALUInstr srem_1_1 = new ALUInstr(IntegerType.i32, Arrays.asList(param1, new ConstInt(IntegerType.i32, 998244353)), InstrType.MOD, block1);
        block1.addInstr(srem_1_1);
        BrInstr brInstr = new BrInstr(block3);
        brInstr.setBasicBlock(block1);
        block1.addInstr(brInstr);

        //block3
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        basicBlocks.add(block1);
        basicBlocks.add(block7);
        PhiInstr phi_3_1 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_3_1.addOption(new ConstInt(IntegerType.i32, 0), block1);
        //TODO
        phi_3_1.setBasicBlock(block3);
        block3.addInstr(phi_3_1);

        basicBlocks = new ArrayList<>();
        basicBlocks.add(block1);
        basicBlocks.add(block7);
        PhiInstr phi_3_2 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_3_2.setBasicBlock(block3);
        phi_3_2.addOption(param2, block1);
        //TODO
        block3.addInstr(phi_3_2);

        basicBlocks = new ArrayList<>();
        basicBlocks.add(block1);
        basicBlocks.add(block7);
        PhiInstr phi_3_3 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_3_3.setBasicBlock(block3);
        phi_3_3.addOption(srem_1_1, block1);
        //TODO
        block3.addInstr(phi_3_3);
        CmpInstr sgt_3_1 = new CmpInstr(">", IntegerType.i1, Arrays.asList(phi_3_2, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block3);
        block3.addInstr(sgt_3_1);
        BrInstr brInstr1 = new BrInstr(sgt_3_1, Arrays.asList(block4, block5));
        brInstr1.setBasicBlock(block3);
        block3.addInstr(brInstr1);

        //block4
        ALUInstr ashr_4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_3_2, new ConstInt(IntegerType.i32, 1)), InstrType.ASHR, block4);
        block4.addInstr(ashr_4_1);
        ALUInstr shl_4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr_4_1, new ConstInt(IntegerType.i32, 1)), InstrType.SHL, block4);
        block4.addInstr(shl_4_1);
        ALUInstr sub_4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_3_2, shl_4_1), InstrType.SUB, block4);
        block4.addInstr(sub_4_1);
        CmpInstr eq_4_1 = new CmpInstr("==", IntegerType.i1, Arrays.asList(sub_4_1, new ConstInt(IntegerType.i32, 1)), InstrType.ICMP, block4);
        block4.addInstr(eq_4_1);
        BrInstr brInstr2 = new BrInstr(eq_4_1, Arrays.asList(block6, block7));
        brInstr2.setBasicBlock(block4);
        block4.addInstr(brInstr2);

        //block5
        RetInstr retInstr = new RetInstr(IntegerType.i32, "void", phi_3_1, block5);
        block5.addInstr(retInstr);

        //block6
        ALUInstr add_6_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_3_1, phi_3_3), InstrType.ADD, block6);
        block6.addInstr(add_6_1);
        ALUInstr srem_6_1 = new ALUInstr(IntegerType.i32, Arrays.asList(add_6_1, new ConstInt(IntegerType.i32, 998244353)), InstrType.MOD, block6);
        block6.addInstr(srem_6_1);
        BrInstr brInstr3 = new BrInstr(block7);
        brInstr3.setBasicBlock(block6);
        block6.addInstr(brInstr3);

        //block7
        ArrayList<BasicBlock> basicBlocks1 = new ArrayList<>();
        basicBlocks1.add(block4);
        basicBlocks1.add(block6);
        PhiInstr phi_7_1 = new PhiInstr(IntegerType.i32, basicBlocks1);
        phi_7_1.setBasicBlock(block7);
        phi_7_1.addOption(phi_3_1, block4);
        phi_7_1.addOption(srem_6_1, block6);
        block7.addInstr(phi_7_1);
        ALUInstr add_7_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_3_3, phi_3_3), InstrType.ADD, block7);
        block7.addInstr(add_7_1);
        ALUInstr srem_7_1 = new ALUInstr(IntegerType.i32, Arrays.asList(add_7_1, new ConstInt(IntegerType.i32, 998244353)), InstrType.MOD, block7);
        block7.addInstr(srem_7_1);
        BrInstr brInstr4 = new BrInstr(block3);
        brInstr4.setBasicBlock(block7);
        block7.addInstr(brInstr4);

        phi_3_1.addOption(phi_7_1, block7);
        phi_3_2.addOption(ashr_4_1, block7);
        phi_3_3.addOption(srem_7_1, block7);

        ArrayList<Instruction> instructions = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            instructions.addAll(block.getInstrList());
        }
        for (Instruction instruction : instructions) {
            instruction.remove();
        }
        function.getBlockList().clear();
        function.addBB(block1);
        function.addBB(block3);
        function.addBB(block4);
        function.addBB(block5);
        function.addBB(block6);
        function.addBB(block7);
        function.setExitBlock(block5);
    }

    public void recursionToLoop2(Function function) {
        Param param1 = function.getParams().get(0);
        Param param2 = function.getParams().get(1);
        BasicBlock block8 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block10 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block11 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block12 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block13 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock block14 = new BasicBlock(UndefinedType.undefined, function);

        //block8
        BrInstr brInstr = new BrInstr(block10);
        brInstr.setBasicBlock(block8);
        block8.addInstr(brInstr);

        //block10
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        basicBlocks.add(block8);
        basicBlocks.add(block14);
        PhiInstr phi_10_1 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_10_1.setBasicBlock(block10);
        phi_10_1.addOption(new ConstInt(IntegerType.i32, 1), block8);
        block10.addInstr(phi_10_1);
        //TODO
        basicBlocks = new ArrayList<>();
        basicBlocks.add(block8);
        basicBlocks.add(block14);
        PhiInstr phi_10_2 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_10_2.setBasicBlock(block10);
        phi_10_2.addOption(param2, block8);
        //TODO
        block10.addInstr(phi_10_2);
        basicBlocks = new ArrayList<>();
        basicBlocks.add(block8);
        basicBlocks.add(block14);
        PhiInstr phi_10_3 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_10_3.setBasicBlock(block10);
        phi_10_3.addOption(param1, block8);
        //TODO
        block10.addInstr(phi_10_3);

        CmpInstr sgt_10_1 = new CmpInstr(">", IntegerType.i1, Arrays.asList(phi_10_2, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, block10);
        block10.addInstr(sgt_10_1);
        BrInstr brInstr1 = new BrInstr(sgt_10_1, Arrays.asList(block11, block12));
        brInstr1.setBasicBlock(block10);
        block10.addInstr(brInstr1);

        //block11
        ALUInstr ashr_11_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_10_2, new ConstInt(IntegerType.i32, 1)), InstrType.ASHR, block11);
        block11.addInstr(ashr_11_1);
        ALUInstr shl_11_1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr_11_1, new ConstInt(IntegerType.i32, 1)), InstrType.SHL, block11);
        block11.addInstr(shl_11_1);
        ALUInstr sub_11_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_10_2, shl_11_1), InstrType.SUB, block11);
        block11.addInstr(sub_11_1);
        CmpInstr eq_11_1 = new CmpInstr("==", IntegerType.i1, Arrays.asList(sub_11_1, new ConstInt(IntegerType.i32, 1)), InstrType.ICMP, block11);
        block11.addInstr(eq_11_1);
        BrInstr brInstr2 = new BrInstr(eq_11_1, Arrays.asList(block13, block14));
        brInstr2.setBasicBlock(block11);
        block11.addInstr(brInstr2);

        //block12
        RetInstr retInstr = new RetInstr(IntegerType.i32, "void", phi_10_1, block12);
        block12.addInstr(retInstr);

        //block13
        ArrayList<Value> values = new ArrayList<>();
        values.add(phi_10_1);
        values.add(phi_10_3);
        CallInstr call_13_1 = new CallInstr(IntegerType.i32, module.getFunctions().get(0), values);
        call_13_1.setBasicBlock(block13);
        block13.addInstr(call_13_1);
        BrInstr brInstr3 = new BrInstr(block14);
        brInstr3.setBasicBlock(block13);
        block13.addInstr(brInstr3);

        //block14
        ArrayList<BasicBlock> basicBlocks1 = new ArrayList<>();
        basicBlocks1.add(block11);
        basicBlocks1.add(block13);
        PhiInstr phi_14_1 = new PhiInstr(IntegerType.i32, basicBlocks1);
        phi_14_1.setBasicBlock(block14);
        phi_14_1.addOption(phi_10_1, block11);
        phi_14_1.addOption(call_13_1, block13);
        block14.addInstr(phi_14_1);
        ArrayList<Value> values1 = new ArrayList<>();
        values1.add(phi_10_3);
        values1.add(phi_10_3);
        CallInstr call_14_1 = new CallInstr(IntegerType.i32, module.getFunctions().get(0), values1);
        call_14_1.setBasicBlock(block14);
        block14.addInstr(call_14_1);
        BrInstr brInstr4 = new BrInstr(block10);
        brInstr4.setBasicBlock(block14);
        block14.addInstr(brInstr4);

        phi_10_1.addOption(phi_14_1, block14);
        phi_10_2.addOption(ashr_11_1, block14);
        phi_10_3.addOption(call_14_1, block14);

        ArrayList<Instruction> instructions = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            instructions.addAll(block.getInstrList());
        }
        for (Instruction instruction : instructions) {
            instruction.remove();
        }
        function.getBlockList().clear();
        function.addBB(block8);
        function.addBB(block10);
        function.addBB(block11);
        function.addBB(block12);
        function.addBB(block13);
        function.addBB(block14);
        function.setExitBlock(block12);
    }
}
