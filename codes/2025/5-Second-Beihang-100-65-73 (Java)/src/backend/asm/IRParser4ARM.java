package backend.asm;

import backend.asm.immediate.ASMImmediate;
import backend.asm.immediate.ASMNegStackSize;
import backend.asm.immediate.ASMStackOffset;
import backend.asm.instr.arm.ARMCond;
import backend.asm.instr.arm.binary.*;
import backend.asm.instr.arm.cmp.ARMCmp;
import backend.asm.instr.arm.cmp.ARMCset;
import backend.asm.instr.arm.jump.ARMB;
import backend.asm.instr.arm.jump.ARMBranch;
import backend.asm.instr.arm.jump.ARMBl;
import backend.asm.instr.arm.jump.ARMBr;
import backend.asm.instr.arm.memory.ARMLoad;
import backend.asm.instr.arm.memory.ARMStore;
import backend.asm.instr.arm.neon.*;
import backend.asm.instr.arm.neon.binary.ARMVecAdd;
import backend.asm.instr.arm.neon.binary.ARMVecEor;
import backend.asm.instr.arm.neon.binary.ARMVecMAdd;
import backend.asm.instr.arm.neon.memory.ARMVecLoad;
import backend.asm.instr.arm.neon.memory.ARMVecStore;
import backend.asm.instr.arm.others.*;
import backend.asm.instr.arm.ternary.ARMMsub;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirFReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMGlobalObject;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;

public class IRParser4ARM extends IRParser {
    private static IRParser4ARM IR_PARSER = null;

    private IRParser4ARM() {
        super();
    }

    public static IRParser4ARM getInstance() {
        if (IR_PARSER == null) {
            IR_PARSER = new IRParser4ARM();
        }
        return IR_PARSER;
    }

    @Override
    protected void parseIntBranch(CmpInstr cmpInstr, BranchInstr branchInstr, ASMBasicBlock newBlock) {
        this.parseCmpInstr(cmpInstr, newBlock);
        this.parseBranchInstr(branchInstr, newBlock);
    }

    @Override
    protected void genPrologue(ASMFunction asmFunction, ASMBasicBlock firstBlock) {
        genAdd(regStore.getStackPtr(), new ASMNegStackSize(asmFunction), firstBlock, regStore.getStackPtr()); // 移动栈指针
        genStore(new ASMStackOffset(asmFunction, -8), regStore.getStackPtr(), regStore.getRetAddr(), firstBlock, false, true); // 暂存 $ra
        genJump((ASMBasicBlock) firstBlock.getNext(), firstBlock);
    }

    @Override
    protected void genLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat) {
        new ARMLoad(offset, base, parentBB, reg);
    }

    @Override
    protected void genLoad(ASMValue ptr, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat, boolean isFltLi) {
        ARMLoad load = new ARMLoad(ptr, parentBB, reg);
        if (isFltLi) {
            load.markAsFltLi();
        }
    }

    @Override
    protected void genStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        new ARMStore(offset, base, value, parentBB);
    }

    @Override
    protected void genStore(ASMValue ptr, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        new ARMStore(ptr, value, parentBB);
    }

    @Override
    protected void genMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst) {
        if (src instanceof ASMImmediate imm) {
            new ARMLi(imm, parentBlock, dst);
        } else if (src instanceof Reg reg) {
            if (src instanceof VirIReg srcVir && dst instanceof VirIReg dstVir
                    && srcVir.isDouble() && !dstVir.isDouble()) {
                new ARMTrunc(srcVir, parentBlock, dst);
            } else {
                new ARMMove(reg, parentBlock, dst);
            }
        } else {
            throw new RuntimeException("move的来源只能是立即数或者寄存器");
        }
    }

    @Override
    protected void genFrozenMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst) {
        throw new RuntimeException("ARM does not support frozen move directly.");
    }

    @Override
    protected void genLa(ASMGlobalObject globalObject, ASMBasicBlock parentBB, Reg reg) {
        VirIReg tmp = new VirIReg();
        tmp.setDouble(true);
        new ARMAdrp(globalObject.getLabelName(), parentBB, tmp);
        genAdd(tmp, new ASMLo12(globalObject.getLabelName()), parentBB, reg);
    }

    @Override
    protected void genLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        new ARMLi(imm, parentBlock, reg);
    }

    @Override
    protected void genFAdd(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMFAdd(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFSub(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMFSub(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMFMul(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMFDiv(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg zero = regStore.getZero();
        if (operand1 == zero) {
            genMove(operand2, parentBB, reg);
        } else if (operand2 == zero || operand2 instanceof ASMImmediate imm && imm.getImm() == 0) {
            genMove(operand1, parentBB, reg);
        } else {
            // 如果 32 位和 64 位相加需要进行符号拓展，要把 32 位放后面
            // 语义上应该只有加减法涉及不同位宽的混合，而且只有加法可能涉及到二者交换位置
            if (operand1 instanceof VirIReg virIReg1 && operand2 instanceof VirIReg virIReg2) {
                if (!virIReg1.isDouble() && virIReg2.isDouble()) {
                    new ARMAdd(virIReg2, virIReg1, parentBB, reg);
                    return;
                }
            }
            new ARMAdd(operand1, operand2, parentBB, reg);
        }
    }

    @Override
    protected void genSub(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMSub(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg op1 = operand1;
        Reg op2 = operand2;
        if (reg instanceof VirIReg virIReg && virIReg.isDouble()) {
            if (operand1 instanceof VirIReg op && !op.isDouble()) {
                op1 = makeSxtw(operand1, parentBB);
            }
            if (operand2 instanceof VirIReg op && !op.isDouble()) {
                op2 = makeSxtw(operand2, parentBB);
            }
        }
        new ARMMul(op1, op2, parentBB, reg);
    }

    @Override
    protected void genDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new ARMSDiv(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genRem(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        // `rem rd, rs1, rs2` <==> `SDIV Xt, Xn, Xm`<br>`MSUB Xd, Xt, Xm, Xn`
        VirIReg xt = new VirIReg();
        new ARMSDiv(operand1, operand2, parentBB, xt);  // 被除数在前，除数在后
        new ARMMsub(xt, operand2, operand1, parentBB, reg);  // 除数在前，被除数在后
    }

    @Override
    protected void genFtoi(Reg src, ASMBasicBlock block, Reg reg) {
        new ARMFtoi(src, block, reg);
    }

    @Override
    protected void genItof(Reg src, ASMBasicBlock block, Reg reg) {
        new ARMItof(src, block, reg);
    }

    @Override
    protected void genJump(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        new ARMB(target, parentBlock);
    }

    @Override
    protected void genBeq(Reg operand1, Reg operand2, ASMBasicBlock target, ASMBasicBlock parentBlock) {
        new ARMCmp(operand1, operand2, parentBlock);
        new ARMBranch(target, parentBlock);
    }

    @Override
    protected void genJr(Reg target, ASMBasicBlock parentBlock) {
        new ARMBr(target, parentBlock);
    }

    private VirIReg genCmpAndCset(ASMValue value1, ASMValue value2, ARMCond cond, ASMBasicBlock newBlock) {
        new ARMCmp((Reg) value1, value2, newBlock);
        VirIReg cmpRes = new VirIReg();
        new ARMCset(cond, newBlock, cmpRes);
        return cmpRes;
    }

    @Override
    protected ASMValue generateFLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.FLT, newBlock);
    }

    @Override
    protected ASMValue generateFGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.FGT, newBlock);
    }

    @Override
    protected ASMValue generateFEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.EQ, newBlock);
    }

    @Override
    protected ASMValue generateFNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.NE, newBlock);
    }

    @Override
    protected ASMValue generateFLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.FLE, newBlock);
    }

    @Override
    protected ASMValue generateFGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return genCmpAndCset(value1, value2, ARMCond.FGE, newBlock);
    }

    @Override
    protected ASMValue generateSLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() < imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.LT, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.LT, newBlock);
        }
    }

    @Override
    protected ASMValue generateSGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() > imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.GT, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.GT, newBlock);
        }
    }

    @Override
    protected ASMValue generateSEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() == imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.EQ, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.EQ, newBlock);
        }
    }

    @Override
    protected ASMValue generateSNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() != imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.NE, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.NE, newBlock);
        }
    }

    @Override
    protected ASMValue generateSLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() <= imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.LE, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.LE, newBlock);
        }
    }

    @Override
    protected ASMValue generateSGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() >= imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                return genCmpAndCset(immInt2reg(imm1, newBlock), value2, ARMCond.GE, newBlock);
            }
        } else {
            return genCmpAndCset(value1, value2, ARMCond.GE, newBlock);
        }
    }

    @Override
    protected void genJal(ASMFunction targetFunc, ASMBasicBlock parentBlock) {
        new ARMBl(targetFunc, parentBlock);
    }

    @Override
    protected void genFNeg(ASMValue src, ASMBasicBlock parentBlock, Reg reg) {
        new ARMFNeg((Reg) src, parentBlock, reg);
    }

    @Override
    protected void genSll(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg op1 = operand1;
        if (operand1 instanceof VirIReg op && !op.isDouble() &&
                reg instanceof VirIReg virIReg && virIReg.isDouble()) {
            op1 = makeSxtw(operand1, parentBB);
        }
        new ARMLsl(op1, (ASMImmediate) operand2, parentBB, reg);
    }

    @Override
    protected void genSra(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg op1 = operand1;
        if (operand1 instanceof VirIReg op && !op.isDouble() &&
                reg instanceof VirIReg virIReg && virIReg.isDouble()) {
            op1 = makeSxtw(operand1, parentBB);
        }
        new ARMAsr(op1, operand2, parentBB, reg);
    }

    @Override
    protected void genSrl(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg op1 = operand1;
        if (operand1 instanceof VirIReg op && !op.isDouble() &&
                reg instanceof VirIReg virIReg && virIReg.isDouble()) {
            op1 = makeSxtw(operand1, parentBB);
        }
        new ARMLsr(op1, operand2, parentBB, reg);
    }

    private VirIReg makeSxtw(Reg operand, ASMBasicBlock parentBB) {
        VirIReg newReg = new VirIReg();
        newReg.setDouble(true);
        new ARMSxtw(operand, parentBB, newReg);
        return newReg;
    }

    protected void genAnd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        if (operand2 instanceof ASMImmediate imm) {
            VirIReg tmp = new VirIReg();
            new ARMLi(imm, parentBB, tmp);
            new ARMAnd(operand1, tmp, parentBB, reg);
        } else {
            new ARMAnd(operand1, (Reg) operand2, parentBB, reg);
        }
    }

    protected void genAtomAdd(ASMValue ptr, ASMValue inc, ASMBasicBlock parentBB, Reg reg) {
        throw new UnsupportedOperationException("ARM does not support atomic add directly. Use a different approach.");
    }

    @Override
    protected void genVecMoveFromSca(VirReg srcVal, int index, ASMBasicBlock parentBB, VirFReg dst) {
        new ARMVecMoveFromSca(srcVal, index, parentBB, dst);
    }

    @Override
    protected void genVecMoveToSca(VirFReg srcVal, int index, ASMBasicBlock parentBB, VirReg dst) {
        new ARMVecMoveToSca(srcVal, index, parentBB, dst);
    }

    @Override
    protected void genVecStore(VirFReg valueToStore, Reg basicPointer, ASMBasicBlock parentBB) {
        new ARMVecStore(basicPointer, valueToStore, parentBB);
    }

    @Override
    protected void genVecLoad(Reg basicPointer, ASMBasicBlock parentBB, VirFReg reg) {
        new ARMVecLoad(basicPointer, parentBB, reg);
    }

    @Override
    protected void genVecDup(VirReg scalarValue, ASMBasicBlock parentBB, VirFReg reg) {
        new ARMVecDup(scalarValue, parentBB, reg);
    }

    @Override
    protected void genVecMulAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg) {
        new ARMVecMAdd(op1, op2, parentBB, reg);
    }

    @Override
    protected void genVecMove(VirFReg src, ASMBasicBlock parentBlock, VirFReg dst) {
        new ARMVecMove(src, parentBlock, dst);
    }

    @Override
    protected void genAddv(VirFReg srcVec, ASMBasicBlock parentBlock, VirFReg register) {
        new ARMVecAddv(srcVec, parentBlock, register);
    }

    @Override
    protected void genBitCopy(Reg src, ASMBasicBlock block, Reg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg) {
        new ARMVecAdd(op1, op2, parentBB, reg);
    }

    @Override
    protected void genVecRegClear(ASMBasicBlock parentBB, VirFReg reg) {
        new ARMVecEor(reg, reg, parentBB, reg);
    }
}
