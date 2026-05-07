package riscv;

import midend.MyModule;
import midend.Type;
import midend.value.Argument;
import midend.value.BasicBlock;
import midend.value.Constant.ConstantFloat;
import midend.value.Constant.ConstantInteger;
import midend.value.Function;
import midend.value.Value;
import midend.value.global.GlobalVariable;
import midend.value.instrs.*;
import riscv.optimize.*;
import riscv.regalloc.AsmRegAlloc;
import riscv.util.Dump;
import riscv.util.JustifyShortImm;
import riscv.value.*;

import java.io.IOException;
import java.util.List;
import java.util.Objects;

@SuppressWarnings({"UnnecessaryLocalVariable", "RedundantLabeledSwitchRuleCodeBlock"})
public class AsmGen {
    private final MyModule irModule;
    private final AsmModule asmModule;

    private final boolean doOptimize = true;

    public AsmGen(MyModule irModule) {
        this.irModule = irModule;
        this.asmModule = new AsmModule();
    }

    public void generateRiscV(String asmPath) throws IOException {
        genModule();
        if (doOptimize) {
            RemoveJumpNext.optimize(asmModule);
            Sw2Sd.optimize(asmModule);
            PeepHole.optimize(asmModule);
        }
        JustifyShortImm.runJustify(asmModule);
        Dump.dumpToFile(asmModule, asmPath);
    }

    private AsmFunction curAsmFunc;
    private AsmBasicBlock curAsmBB;

    private void genModule() {
        // global
        GlobalAsmGen.globalAsmGen(irModule, asmModule);

        // funcs
        // 先实例化所有的 asmFunc
        // main 函数的栈帧移动指令，在输出时才添加；见 util/Dump.java
        var funcMap = irModule.getFunctions();
        for (Function func : funcMap.values()) {
            if (ExternFunctions.isExternFunc(func.getName())) continue;
            String funcName = func.getName();
            asmModule.funcMap.put(funcName, new AsmFunction(funcName));
        }

        // 寄存器分配
        AsmRegAlloc.runAlloc(irModule, asmModule);

        // 再生成每一个 asmFunc
        for (Function func : funcMap.values()) {
            if (ExternFunctions.isExternFunc(func.getName())) continue;
            genFunc(func);
        }
    }

    private void genFunc(Function irFunc) {
        String funcName = irFunc.getName();
        AsmFunction asmFunc = asmModule.funcMap.get(funcName);
        curAsmFunc = asmFunc;

        for (var param : irFunc.getArgument()) {
            curAsmFunc.allocTable.addParam((Argument) param);
        }

        // 解析完成函数名和参数之后就加入函数表中，以供递归调用
        asmModule.funcMap.put(asmFunc.funcName, asmFunc);

        // 先实例化好每一个 asmBB，保证 bb 的顺序和 IR 一致
        for (var bbp : irFunc.getBasicBlocks()) {
            BasicBlock bb = bbp.get();
            curAsmFunc.getOrCreateBB(bb);
        }

        for (var bbp : irFunc.getBasicBlocks()) {
            BasicBlock bb = bbp.get();
            genBasicBlock(bb);
        }
    }

    private void genBasicBlock(BasicBlock bb) {
        AsmBasicBlock asmBB = curAsmFunc.getOrCreateBB(bb);
        curAsmBB = asmBB;

        for (var irp = bb.getInstrList().firstNode(); irp != bb.getInstrList().tailNode(); irp = irp.next()) {
            Instr ir = irp.get();
//            addAsmInstr(new AsmInstr.Nop().withComment(ir.toString()));

            if (ir instanceof Alloc instr) {
                genAlloc(instr);
            }
            // t = icmp a, b
            // br t, bb1, bb2
            // 生成为一条 branch 语句
            else if (isICmp(ir) &&
                    bb.getInstrList().hasNext(irp) &&
                    irp.next().get() instanceof Branch br &&
                    ir == br.getOperandList().get(0)) {
                genBranchWithCmp(br, (BinaryOp) ir);
                irp = irp.next();
            }
            else if (ir instanceof BinaryOp instr) {
                genBinaryOp(instr);
            }
            else if (ir instanceof BitCast instr) {
                genBitCast(instr);
            }
            else if (ir instanceof Branch instr) {
                genBranch(instr);
            }
            else if (ir instanceof Call instr) {
                genCall(instr);
            }
            else if (ir instanceof CallVoid instr) {
                genCallVoid(instr);
            }
            else if (ir instanceof FtoI instr) {
                genFtoI(instr);
            }
            else if (ir instanceof GetElementPtr instr) {
                genGetElementPtr(instr);
            }
            else if (ir instanceof ItoF instr) {
                genItoF(instr);
            }
            else if (ir instanceof Jump instr) {
                genJump(instr);
            }
            else if (ir instanceof Load instr) {
                genLoad(instr);
            }
            else if (ir instanceof Move instr) {
                genMove(instr);
            }
            else if (ir instanceof Return instr) {
                genReturn(instr);
            }
            else if (ir instanceof Store instr) {
                genStore(instr);
            }
            else if (ir instanceof Zext instr) {
                genZext(instr);
            }
        }
    }

    // oprs = Operand Source, oprd = Operand Destination (IR)
    // rs = Register Source, rd = Register Destination   (ASM)

    private void genAlloc(Alloc instr) {
        Loc loc = instr;
        Type elementType = loc.getType().getElementType();
        if (elementType instanceof Type.ArrayType arrayType) {
            curAsmFunc.allocTable.allocStackLoc(loc, arrayType.getLength() * 4);
        }
        else {
            int size = loc.getType().isPointerType() ? 8 : 4;
            curAsmFunc.allocTable.allocStackLoc(loc, size);
        }
    }

    private void genBinaryOp(BinaryOp instr) {
        Reg oprd = instr;
        Value oprs1 = instr.getOperandList().get(0);
        Value oprs2 = instr.getOperandList().get(1);
        AsmReg rdSpill = instr.getType().isIntegerType() ? AsmReg.rSpill1 : AsmReg.frSpill1;
        AsmReg rd = asmRegOf(oprd, rdSpill, true);

        Op op = instr.getOp();
        switch (op) {
            case Add -> {
                if (oprs1 instanceof ConstantInteger || oprs2 instanceof ConstantInteger) {
                    Reg reg = (Reg) (oprs1 instanceof ConstantInteger ? oprs2 : oprs1);
                    int imm = ((ConstantInteger) (oprs1 instanceof ConstantInteger ? oprs1 : oprs2)).getConstantIntegerValue();
                    AsmReg rs = asmRegOf(reg);
                    addAsmInstr(new AsmInstr.Addi(rd, rs, imm));
                }
                else {
                    AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                    addAsmInstr(new AsmInstr.Add(rd, rs1, rs2));
                }
            }
            case Sub -> {
                if (oprs1 instanceof Reg reg && oprs2 instanceof ConstantInteger constInt) {
                    AsmReg rs = asmRegOf(reg);
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstr(AsmInstr.Pseudo.Subi(rd, rs, imm));
                }
                else {
                    AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                    addAsmInstr(new AsmInstr.Sub(rd, rs1, rs2));
                }
            }
            case Mul -> {
                if (doOptimize && (oprs1 instanceof ConstantInteger || oprs2 instanceof ConstantInteger)) {
                    Reg reg = (Reg) (oprs1 instanceof ConstantInteger ? oprs2 : oprs1);
                    int imm = ((ConstantInteger) (oprs1 instanceof ConstantInteger ? oprs1 : oprs2)).getConstantIntegerValue();
                    AsmReg rs = asmRegOf(reg);
                    addAsmInstrs(Muli.genMuli(rd, rs, imm));
                }
                else {
                    AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                    addAsmInstr(new AsmInstr.Mul(rd, rs1, rs2));
                }
            }
            case Div -> {
                if (doOptimize && oprs1 instanceof Reg reg && oprs2 instanceof ConstantInteger constInt) {
                    AsmReg rs = asmRegOf(reg);
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstrs(Divi.genDivi(rd, rs, imm));
                }
                else if (doOptimize && oprs1 instanceof ConstantInteger constInt && oprs2 instanceof Reg reg) {
                    AsmReg rs = asmRegOf(reg);
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstrs(Divi.genIDivR(rd, imm, rs));
                }
                else {
                    AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                    addAsmInstr(new AsmInstr.Div(rd, rs1, rs2));
                }
            }
            case Mod -> {
                if (doOptimize && oprs1 instanceof Reg reg && oprs2 instanceof ConstantInteger constInt) {
                    AsmReg rs = asmRegOf(reg);
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstrs(Remi.genRemi(rd, rs, imm));
                }
                else if (doOptimize && oprs1 instanceof ConstantInteger constInt && oprs2 instanceof Reg reg) {
                    AsmReg rs = asmRegOf(reg);
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstrs(Remi.genIRemR(rd, imm, rs));
                }
                else {
                    AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                    addAsmInstr(new AsmInstr.Rem(rd, rs1, rs2));
                }
            }
            case FAdd -> {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.FAdd(rd, rs1, rs2));
            }
            case FSub -> {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.FSub(rd, rs1, rs2));
            }
            case FMul -> {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.FMul(rd, rs1, rs2));
            }
            case FDiv -> {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.FDiv(rd, rs1, rs2));
            }

            case Le -> genLe(rd, oprs1, oprs2);
            case Lt -> genLt(rd, oprs1, oprs2);
            case Ge -> genLe(rd, oprs2, oprs1);
            case Gt -> genLt(rd, oprs2, oprs1);
            case Eq -> genEqNe(rd, oprs1, oprs2, true);
            case Ne -> genEqNe(rd, oprs1, oprs2, false);

            default -> throw new AssertionError();
        }
    }

    private void genLt(AsmReg rd, Value oprs1, Value oprs2) {
        if (oprs1.getType().isIntegerType()) {
            if (oprs1 instanceof Reg reg && oprs2 instanceof ConstantInteger constInt) {
                AsmReg rs = asmRegOf(reg);
                int imm = constInt.getConstantIntegerValue();
                addAsmInstr(new AsmInstr.Slti(rd, rs, imm));
            }
            else {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.Slt(rd, rs1, rs2));
            }
        }
        else {
            AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
            addAsmInstr(new AsmInstr.FLt(rd, rs1, rs2));
        }
    }

    private void genLe(AsmReg rd, Value oprs1, Value oprs2) {
        if (oprs1.getType().isIntegerType()) {
            genLt(rd, oprs2, oprs1);
            addAsmInstr(AsmInstr.Pseudo.LogicNot(rd, rd));
        }
        else {
            AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
            addAsmInstr(new AsmInstr.FLe(rd, rs1, rs2));
        }
    }

    private void genEqNe(AsmReg rd, Value oprs1, Value oprs2, boolean isEq) {
        if (oprs1.getType().isIntegerType()) {
            if (oprs1 instanceof ConstantInteger || oprs2 instanceof ConstantInteger) {
                Reg reg = (Reg) (oprs1 instanceof ConstantInteger ? oprs2 : oprs1);
                int imm = ((ConstantInteger) (oprs1 instanceof ConstantInteger ? oprs1 : oprs2)).getConstantIntegerValue();
                AsmReg rs = asmRegOf(reg);
                if (imm == 0) {
                    if (isEq) addAsmInstr(AsmInstr.Pseudo.Seqz(rd, rs));
                    else addAsmInstr(AsmInstr.Pseudo.Snez(rd, rs));
                }
                else {
                    addAsmInstr(new AsmInstr.Xori(rd, rs, imm));
                    if (isEq) addAsmInstr(AsmInstr.Pseudo.Seqz(rd, rd));
                    else addAsmInstr(AsmInstr.Pseudo.Snez(rd, rd));
                }
            }
            else {
                AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
                addAsmInstr(new AsmInstr.Xor(rd, rs1, rs2));
                if (isEq) addAsmInstr(AsmInstr.Pseudo.Seqz(rd, rd));
                else addAsmInstr(AsmInstr.Pseudo.Snez(rd, rd));
            }
        }
        else {
            AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
            addAsmInstr(new AsmInstr.FEq(rd, rs1, rs2));
            if (!isEq) addAsmInstr(AsmInstr.Pseudo.LogicNot(rd, rd));
        }
    }

    private void genBitCast(BitCast instr) {
        throw new AssertionError("Not implemented");
    }

    private void genBranch(Branch instr) {
        Value reg = instr.getOperandList().get(0); // 可能会有为立即数的情况，不过中端优化后就消除了
        AsmReg rs = asmRegOf(reg);
        BasicBlock trueBB = (BasicBlock) instr.getOperandList().get(1);
        BasicBlock falseBB = (BasicBlock) instr.getOperandList().get(2);
        AsmBasicBlock trueAsmBB = curAsmFunc.getOrCreateBB(trueBB);
        AsmBasicBlock falseAsmBB = curAsmFunc.getOrCreateBB(falseBB);

        addAsmInstr(new AsmInstr.Beq(rs, AsmReg.zero, falseAsmBB));
        addAsmInstr(new AsmInstr.J(trueAsmBB));
    }

    private void genBranchWithCmp(Branch branch, BinaryOp cmp) {
        assert Objects.equals(cmp, branch.getOperandList().get(0));
        Value oprs1 = cmp.getOperandList().get(0);
        Value oprs2 = cmp.getOperandList().get(1);
        AsmReg rs1 = asmRegOf(oprs1), rs2 = asmRegOf(oprs2);
        Op op = cmp.getOp();

        BasicBlock trueBB = (BasicBlock) branch.getOperandList().get(1);
        BasicBlock falseBB = (BasicBlock) branch.getOperandList().get(2);
        AsmBasicBlock trueAsmBB = curAsmFunc.getOrCreateBB(trueBB);
        AsmBasicBlock falseAsmBB = curAsmFunc.getOrCreateBB(falseBB);

        switch (op) {
            case Lt -> addAsmInstr(new AsmInstr.Bge(rs1, rs2, falseAsmBB));
            case Le -> addAsmInstr(AsmInstr.Pseudo.Bgt(rs1, rs2, falseAsmBB));
            case Ge -> addAsmInstr(new AsmInstr.Blt(rs1, rs2, falseAsmBB));
            case Gt -> addAsmInstr(AsmInstr.Pseudo.Ble(rs1, rs2, falseAsmBB));
            case Eq -> addAsmInstr(new AsmInstr.Bne(rs1, rs2, falseAsmBB));
            case Ne -> addAsmInstr(new AsmInstr.Beq(rs1, rs2, falseAsmBB));
            default -> throw new AssertionError();
        }

        addAsmInstr(new AsmInstr.J(trueAsmBB));
    }

    private void genCall(Call instr) {
        AsmReg rd = asmRegOf(instr, true);
        genFuncCall(instr, rd);
    }

    private void genCallVoid(CallVoid instr) {
        genFuncCall(instr, null);
    }

    private void genFuncCall(Instr instr, AsmReg rd) {
        String funcName = instr.getOperandList().get(0).getName();
        if (ExternFunctions.isExternFunc(funcName)) {
            genExternFuncCall(instr, rd);
        }
        else {
            genDefinedFuncCall(instr, rd);
        }
    }

    private void genDefinedFuncCall(Instr instr, AsmReg rd) {
        Function func = (Function) instr.getOperandList().get(0);
        List<Value> argList = instr.getOperandList().subList(1, instr.getOperandList().size());
        List<Value> paramList = func.getArgument();
        assert argList.size() == paramList.size();
        AsmFunction asmFunc = asmModule.funcMap.get(func.getName());
        assert asmFunc != null;
        int frameSize = asmFunc.allocTable.frameSize();

        // 保留的寄存器
        List<AsmReg> rSaveList = curAsmFunc.allocTable.rActiveListMap.get(instr);
        rSaveList.add(AsmReg.ra);

        // fast call 参数保存
        // 按照调用约定存储 fastcall 参数
        for (int i = 0; i < argList.size(); i++) {
            Value arg = argList.get(i);
            Argument param = (Argument) paramList.get(i);
            Object paramPosition = asmFunc.allocTable.getParamPosition(param);
            if (paramPosition instanceof AsmReg rParam) {
                // 形参是寄存器，将实参复制过去即可
                if (arg instanceof ConstantInteger constInt) {
                    int imm = constInt.getConstantIntegerValue();
                    addAsmInstr(new AsmInstr.Li(rParam, imm));
                }
                else {
                    AsmReg rArg = asmRegOf(arg);
                    // 将 asmLoc 计算出来
                    if (arg instanceof Loc loc) {
                        AsmLoc asmLoc = curAsmFunc.allocTable.getAsmLocOfLoc(loc);
                        addAsmInstr(new AsmInstr.Addi(rArg, asmLoc.rBase(), asmLoc.offset()));
                    }
                    addAsmInstr(AsmInstr.Pseudo.Move(rParam, rArg));
                }
            }
            else if (paramPosition instanceof AsmLoc paramAsmLoc) {
                // 形参在栈上，存储到对应的栈地址
                AsmReg rArg = asmRegOf(arg);
                if (arg instanceof Loc loc) {
                    AsmLoc argAsmLoc = curAsmFunc.allocTable.getAsmLocOfLoc(loc);
                    addAsmInstr(new AsmInstr.Addi(rArg, argAsmLoc.rBase(), argAsmLoc.offset()));
                }
                int spTotalMove = ceilTo16(frameSize) + ceilTo16(rSaveList.size() * 4);
                int offset = paramAsmLoc.offset() - spTotalMove;
                int argSize = arg.getType().isPointerType() ? 8 : 4;
                addAsmInstr(AsmInstr.Pseudo.Store(rArg, paramAsmLoc.rBase(), offset, argSize));
            }
        }

        genPushRegs(rSaveList);

        // 移动 sp，存储多余的参数，跳转
        // sp 需要 16 字节对齐
        int spMoveSize = ceilTo16(frameSize);
        addAsmInstr(AsmInstr.Pseudo.Subi(AsmReg.sp, AsmReg.sp, spMoveSize));
        addAsmInstr(new AsmInstr.Jal(asmFunc));
        addAsmInstr(new AsmInstr.Addi(AsmReg.sp, AsmReg.sp, spMoveSize));

        // 弹出保存的寄存器
        genPopRegs(rSaveList);

        // 加载返回值（如果有）
        if (rd != null) {
            addAsmInstr(AsmInstr.Pseudo.Move(rd, AsmReg.a0));
        }
    }

    private void genExternFuncCall(Instr instr, AsmReg rd) {
        // 保留的寄存器
        List<AsmReg> rSaveList = curAsmFunc.allocTable.rActiveListMap.get(instr);
        rSaveList.add(AsmReg.ra);
        genPushRegs(rSaveList);

        String funcName = instr.getOperandList().get(0).getName();
        List<Value> args = instr.getOperandList().subList(1, instr.getOperandList().size());
        switch (funcName) {
            // int
            case "getint", "getch" -> {
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
                addAsmInstr(AsmInstr.Pseudo.Move(rd, AsmReg.a0));
            }
            // float
            case "getfloat" -> {
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
                addAsmInstr(AsmInstr.Pseudo.Move(rd, AsmReg.fa0));
            }
            // ptr -> int
            case "getarray", "getfarray" -> {
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.a0, asmRegOf(args.get(0))));
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
                addAsmInstr(AsmInstr.Pseudo.Move(rd, AsmReg.a0));
            }
            // int -> ()
            case "putint", "putch" -> {
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.a0, asmRegOf(args.get(0))));
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
            }
            // float -> ()
            case "putfloat" -> {
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.fa0, asmRegOf(args.get(0))));
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
            }
            // int -> ptr -> ()
            case "putarray", "putfarray" -> {
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.a0, asmRegOf(args.get(0))));
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.a1, asmRegOf(args.get(1))));
                addAsmInstr(new AsmInstr.Call(funcName));
                genPopRegs(rSaveList);
            }
            // todo
            case "putf" -> {}
            case "starttime" -> {
                addAsmInstr(new AsmInstr.Li(AsmReg.a0, 0));
                addAsmInstr(new AsmInstr.Call("_sysy_starttime"));
                genPopRegs(rSaveList);
            }
            case "stoptime" -> {
                addAsmInstr(new AsmInstr.Li(AsmReg.a0, 0));
                addAsmInstr(new AsmInstr.Call("_sysy_stoptime"));
                genPopRegs(rSaveList);
            }
        }
    }

    private void genPushRegs(List<AsmReg> regList) {
        // 保守处理，每个寄存器都当做指针，占 8 字节
        addAsmInstr(AsmInstr.Pseudo.Subi(AsmReg.sp, AsmReg.sp, ceilTo16(regList.size() * 8)));
        for (int i = 0; i < regList.size(); i++) {
            addAsmInstr(AsmInstr.Pseudo.Store(regList.get(i), AsmReg.sp, i * 8, 8));

        }
    }

    private void genPopRegs(List<AsmReg> regList) {
        // 保守处理，每个寄存器都当做指针，占 8 字节
        for (int i = 0; i < regList.size(); i++) {
            addAsmInstr(AsmInstr.Pseudo.Load(regList.get(i), AsmReg.sp, i * 8, 8));
        }
        addAsmInstr(new AsmInstr.Addi(AsmReg.sp, AsmReg.sp, ceilTo16(regList.size() * 8)));
    }

    private void genFtoI(FtoI instr) {
        Reg oprd = instr; // i32
        Value oprs = instr.getOperandList().get(0); // float, can be Argument | Reg
        AsmReg rd = asmRegOf(oprd, true);
        AsmReg rs = asmRegOf(oprs);
        addAsmInstr(new AsmInstr.FCvtF2I(rd, rs));
    }

    private void genItoF(ItoF instr) {
        Reg oprd = instr; // float
        Value oprs = instr.getOperandList().get(0); // i32, can be Argument | Reg
        AsmReg rd = asmRegOf(oprd, true);
        AsmReg rs = asmRegOf(oprs);
        addAsmInstr(new AsmInstr.FCvtI2F(rd, rs));
    }

    private void genGetElementPtr(GetElementPtr instr) {
        Loc ptrd = instr;
        Value basePtr = instr.getPointer();
        Type elementType = basePtr.getType().getElementType(); // 获得指针指向的元素类型
        Value offsetValue;
        // 指针指向数组，没有整体偏移，只有数组的元素偏移
        if (elementType.isArrayType()) {
            assert instr.getOffsets().get(0) instanceof ConstantInteger i && i.getConstantIntegerValue() == 0;
            offsetValue = instr.getOffsets().get(1);
        }
        // 指针指向元素，只有整体偏移
        else if (elementType.isBasicType()) {
            assert instr.getOffsets().size() == 1;
            offsetValue = instr.getOffsets().get(0);
        }
        else throw new AssertionError("意外的类型 " + elementType);

        // 偏移量乘 4 再与基地址和基偏移量相加即可
        AsmReg base;
        int baseOffset;

        // 栈数组
        if (basePtr instanceof Loc baseLoc) {
            AsmLoc asmLoc = curAsmFunc.allocTable.getAsmLocOfLoc(baseLoc);
            base = asmLoc.rBase();
            baseOffset = asmLoc.offset();
        }
        // 全局数组
        else if (basePtr instanceof GlobalVariable baseGlobalArr) {
            base = AsmReg.gp;
            baseOffset = asmModule.globalVarAllocTable.getGlobalVarOffset(baseGlobalArr);
        }
        // 指针（因为函数参数而存在）
        // 基地址就是 reg 里的值，没有基地址偏移
        else if (basePtr instanceof Argument || basePtr instanceof Reg) {
            base = asmRegOf(basePtr);
            baseOffset = 0;
        }
        else throw new AssertionError("意外的指针类型 " + basePtr);

        // 添加到 loc 表中
        if (offsetValue instanceof ConstantInteger constInt) {
            int offset = constInt.getConstantIntegerValue() * 4;
            curAsmFunc.allocTable.addLoc(ptrd, new AsmLoc(base, baseOffset + offset));
        }
        else {
            AsmReg rd = asmRegOf(ptrd, true);
            AsmReg roff = asmRegOf(offsetValue);
            addAsmInstr(new AsmInstr.Slli64(AsmReg.rtmp, roff, 2));
            addAsmInstr(new AsmInstr.Add(rd, base, AsmReg.rtmp));
            curAsmFunc.allocTable.addLoc(ptrd, new AsmLoc(rd, baseOffset));
        }
    }

    private void genJump(Jump instr) {
        BasicBlock bb = (BasicBlock) instr.getOperandList().get(0);
        AsmBasicBlock asmBB = curAsmFunc.getOrCreateBB(bb);
        addAsmInstr(new AsmInstr.J(asmBB));
    }

    private void genLoad(Load instr) {
        Reg oprd = instr;
        Value ptr = instr.getOperandList().get(0);
        AsmReg rd = asmRegOf(oprd, true);
        AsmReg rbase;
        int offset;

        if (ptr instanceof Loc loc) {
            AsmLoc asmLoc = curAsmFunc.allocTable.getAsmLocOfLoc(loc);
            rbase = asmLoc.rBase();
            offset = asmLoc.offset();
        }
        else if (ptr instanceof GlobalVariable globalVar) {
            rbase = AsmReg.gp;
            offset = asmModule.globalVarAllocTable.getGlobalVarOffset(globalVar);
        }
        else throw new AssertionError();

        int size = oprd.getType().isPointerType() ? 8 : 4;
        addAsmInstr(AsmInstr.Pseudo.Load(rd, rbase, offset, size));
    }

    private void genMove(Move instr) {
        Value oprd = instr.getOperandList().get(1);
        Value oprs = instr.getOperandList().get(0);
        AsmReg rd = asmRegOf(oprd, true);

        if (oprs instanceof ConstantInteger constInt) {
            int imm = constInt.getConstantIntegerValue();
            addAsmInstr(new AsmInstr.Li(rd, imm));
        }
        else {
            AsmReg rs = asmRegOf(oprs);
            addAsmInstr(AsmInstr.Pseudo.Move(rd, rs));
        }
    }

    private void genReturn(Return instr) {
        // 有返回值，而非 void 函数的 return
        if (instr.getOperandList().size() > 0) {
            Value retValue = instr.getOperandList().get(0);
            if (retValue instanceof ConstantInteger constInt) {
                int imm = constInt.getConstantIntegerValue();
                addAsmInstr(new AsmInstr.Li(AsmReg.a0, imm));
            }
            else {
                AsmReg rs = asmRegOf(retValue);
                addAsmInstr(AsmInstr.Pseudo.Move(AsmReg.a0, rs));
            }
        }
        addAsmInstr(new AsmInstr.Ret());
    }

    private void genStore(Store instr) {
        Value oprs = instr.getOperandList().get(0);
        Value ptr = instr.getOperandList().get(1);
        AsmReg rs, rbase;
        int offset;
        rs = asmRegOf(oprs);
        if (ptr instanceof Loc loc) {
            AsmLoc asmLoc = curAsmFunc.allocTable.getAsmLocOfLoc(loc);
            rbase = asmLoc.rBase();
            offset = asmLoc.offset();
        }
        else if (ptr instanceof GlobalVariable globalVar) {
            rbase = AsmReg.gp;
            offset = asmModule.globalVarAllocTable.getGlobalVarOffset(globalVar);
        }
        else throw new AssertionError();

        int size = oprs.getType().isPointerType() ? 8 : 4;
        addAsmInstr(AsmInstr.Pseudo.Store(rs, rbase, offset, size));
    }

    private void genZext(Zext instr) {
        Value oprd = instr;
        Value oprs = instr.getOperandList().get(0);
        AsmReg rd = asmRegOf(oprd, true);
        if (oprs instanceof ConstantInteger constInt) {
            int imm = constInt.getConstantIntegerValue();
            addAsmInstr(new AsmInstr.Li(rd, imm));
        }
        else {
            AsmReg rs = asmRegOf(oprs);
            addAsmInstr(AsmInstr.Pseudo.Move(rd, rs));
        }
    }


    // Util Functions

    private AsmLoc rSpill1Loc = null;
    private AsmLoc rSpill2Loc = null;
    private AsmLoc frSpill1Loc = null;
    private AsmLoc frSpill2Loc = null;

    private void addAsmInstr(AsmInstr instr) {
        // 添加指令到基本块中
        curAsmBB.instrs.addLast(instr);

        // 如果写了溢出寄存器，添加 sw 语句
        // 对于溢出寄存器，均使用 8 字节
        AsmReg rd = instr instanceof AsmInstr.WriteType writeType ? writeType.rd : null;
        if (rSpill1Loc != null && rd == AsmReg.rSpill1) {
            curAsmBB.instrs.addLast(AsmInstr.Pseudo.Store(rd, rSpill1Loc.rBase(), rSpill1Loc.offset(), 8));
        }
        else if (rSpill2Loc != null && rd == AsmReg.rSpill2) {
            curAsmBB.instrs.addLast(AsmInstr.Pseudo.Store(rd, rSpill2Loc.rBase(), rSpill2Loc.offset(), 8));
        }
        else if (frSpill1Loc != null && rd == AsmReg.frSpill1) {
            curAsmBB.instrs.addLast(AsmInstr.Pseudo.Store(rd, frSpill1Loc.rBase(), frSpill1Loc.offset(), 8));
        }
        else if (frSpill2Loc != null && rd == AsmReg.frSpill2) {
            curAsmBB.instrs.addLast(AsmInstr.Pseudo.Store(rd, frSpill2Loc.rBase(), frSpill2Loc.offset(), 8));
        }

        // 如果读写了溢出寄存器，需要释放
        for (var rUse : instr.useList) {
            freeSpillReg(rUse);
        }
        if (rd != null) {
            freeSpillReg(rd);
        }
    }

    private void addAsmInstrs(List<AsmInstr> instrs) {
        for (var instr : instrs) {
            addAsmInstr(instr);
        }
    }

    private void freeSpillReg(AsmReg rSpill) {
        if (rSpill == AsmReg.rSpill1) rSpill1Loc = null;
        else if (rSpill == AsmReg.rSpill2) rSpill2Loc = null;
        else if (rSpill == AsmReg.frSpill1) frSpill1Loc = null;
        else if (rSpill == AsmReg.frSpill2) frSpill2Loc = null;
    }

    // 返回一个 Value 对应的寄存器
    private AsmReg asmRegOf(Value value) {
        return asmRegOf(value, null, false);
    }

    private AsmReg asmRegOf(Value value, boolean isWrite) {
        return asmRegOf(value, null, isWrite);
    }

    private AsmReg asmRegOf(Value value, AsmReg rSpill) {
        return asmRegOf(value, rSpill, false);
    }

    private AsmReg asmRegOf(Value value, AsmReg rSpill, boolean isWrite) {
        // Reg：查表返回对应的 AsmReg；如果查不到，说明它被 spill 到栈上去了，查 spill 表生成 load 语句
        // gep 指令的目标 loc 也当做 reg 处理。因为对 asmLoc 的计算在 gep 指令生成时完成了
        if (value instanceof Reg || value instanceof GetElementPtr) {
            if (curAsmFunc.allocTable.isAsmReg(value)) {
                return curAsmFunc.allocTable.getAsmRegOfReg(value);
            }
            else {
                // float
                if (value.getType().isFloatType()) {
                    if (value.getType().isFloatType()) {
                        AsmReg rd;
                        AsmLoc spillLoc = curAsmFunc.allocTable.getSpillLoc(value);
                        if (rSpill != null) {
                            rd = rSpill;
                        }
                        else if (frSpill1Loc == null) {
                            rd = AsmReg.frSpill1;
                            frSpill1Loc = spillLoc;
                        }
                        else if (frSpill2Loc == null) {
                            rd = AsmReg.frSpill2;
                            frSpill2Loc = spillLoc;
                        }
                        else throw new AssertionError("No spill reg free");
                        // 不使用 addAsmInstr 函数，因为此时的 rdSpill 不能被释放
                        if (!isWrite) {
                            curAsmBB.instrs.addLast(
                                    AsmInstr.Pseudo.Load(rd, spillLoc.rBase(), spillLoc.offset(), 8)
                            );
                        }
                        return rd;
                    }
                    else throw new AssertionError("意外的类型 " + value);
                }
                // int 或 ptr (gep)
                else {
                    AsmReg rd;
                    AsmLoc spillLoc = curAsmFunc.allocTable.getSpillLoc(value);
                    if (rSpill != null) {
                        rd = rSpill;
                    }
                    else if (rSpill1Loc == null) {
                        rd = AsmReg.rSpill1;
                        rSpill1Loc = spillLoc;
                    }
                    else if (rSpill2Loc == null) {
                        rd = AsmReg.rSpill2;
                        rSpill2Loc = spillLoc;
                    }
                    else throw new AssertionError("No spill reg free");
                    // 不使用 addAsmInstr 函数，因为此时的 rdSpill 不能被释放
                    if (!isWrite) {
                        curAsmBB.instrs.addLast(
                                AsmInstr.Pseudo.Load(rd, spillLoc.rBase(), spillLoc.offset(), 8)
                        );
                    }
                    return rd;
                }
            }
        }
        // 立即数：如果是 0，返回 zero；否则添加 li 语句加载到 rimm 上
        else if (value instanceof ConstantInteger constInt) {
            int imm = constInt.getConstantIntegerValue();
            if (imm != 0) {
                curAsmBB.instrs.addLast(new AsmInstr.Li(AsmReg.rimm, imm));
                return AsmReg.rimm;
            }
            else return AsmReg.zero;
        }
        // 浮点立即数：加载其 int 形式到 t2 上，再使用 FMvI2F 指令移动到 frtmp
        else if (value instanceof ConstantFloat constFloat) {
            int ieeeHex = constFloat.getIEEEInt32();
            curAsmBB.instrs.addLast(
                    new AsmInstr.Li(AsmReg.rimm, ieeeHex, true).withComment(String.valueOf(constFloat.getConstantFloatValue()))
            );
            curAsmBB.instrs.addLast(AsmInstr.Pseudo.Move(AsmReg.frtmp, AsmReg.rimm));
            return AsmReg.frtmp;
        }
        // 函数参数：如果存在寄存器中，返回寄存器；如果存在栈上，添加 lw 语句加载到临时寄存器上
        else if (value instanceof Argument arg) {
            var argPos = curAsmFunc.allocTable.getParamPosition(arg);
            if (argPos instanceof AsmReg rArg) {
                return rArg;
            }
            else if (argPos instanceof AsmLoc asmLoc) {
                AsmReg rd = AsmReg.rimm;
                int size = arg.getType().isPointerType() ? 8 : 4;
                curAsmBB.instrs.addLast(AsmInstr.Pseudo.Load(rd, asmLoc.rBase(), asmLoc.offset(), size));
                return rd;
            }
            else throw new AssertionError();
        }
        // 全局变量：同上
        else if (value instanceof GlobalVariable globalVar) {
            AsmReg rd = AsmReg.rimm;
            int offset = asmModule.globalVarAllocTable.getGlobalVarOffset(globalVar);
            curAsmBB.instrs.addLast(new AsmInstr.Addi(rd, AsmReg.gp, offset));
            return rd;
        }
        else throw new AssertionError(value.toString());
    }

    private static boolean isICmp(Value instr) {
        return instr instanceof BinaryOp bi && bi.getOpType().isIntegerType() &&
                (bi.getOp() == Op.Lt || bi.getOp() == Op.Le || bi.getOp() == Op.Gt ||
                        bi.getOp() == Op.Ge || bi.getOp() == Op.Eq || bi.getOp() == Op.Ne);
    }

    // 向上对齐到 16 的倍数
    public static int ceilTo16(int x) {
        return (x % 16 == 0) ? x : (x / 16 + 1) * 16;
    }
}
