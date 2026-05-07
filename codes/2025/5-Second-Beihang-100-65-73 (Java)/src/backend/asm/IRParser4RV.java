package backend.asm;

import backend.asm.immediate.ASMImmediate;
import backend.asm.immediate.ASMNegStackSize;
import backend.asm.immediate.ASMStackOffset;
import backend.asm.instr.rv.arithmetic.*;
import backend.asm.instr.rv.branch.RVBeq;
import backend.asm.instr.rv.branch.RVBge;
import backend.asm.instr.rv.branch.RVBlt;
import backend.asm.instr.rv.branch.RVBne;
import backend.asm.instr.rv.cmp.RVFeq;
import backend.asm.instr.rv.cmp.RVSltu;
import backend.asm.instr.rv.cmp.RVlt;
import backend.asm.instr.rv.jump.RVJ;
import backend.asm.instr.rv.jump.RVJal;
import backend.asm.instr.rv.jump.RVJr;
import backend.asm.instr.rv.memory.RVLoad;
import backend.asm.instr.rv.memory.RVStore;
import backend.asm.instr.rv.others.*;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirFReg;
import backend.asm.register.vir.VirIReg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMGlobalObject;
import frontend.ir.Value;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;

import java.util.List;

public class IRParser4RV extends IRParser {
    private static IRParser4RV IR_PARSER = null;

    public static IRParser4RV getInstance() {
        if (IR_PARSER == null) {
            IR_PARSER = new IRParser4RV();
        }
        return IR_PARSER;
    }

    @Override
    protected void parseIntBranch(CmpInstr cmpInstr, BranchInstr branchInstr, ASMBasicBlock newBlock) {
        Value op1 = cmpInstr.getOperand1();
        Value op2 = cmpInstr.getOperand2();

        ASMValue value1 = getValue(op1);
        ASMValue value2 = getValue(op2);
        if (value1 instanceof ASMImmediate imm) {
            value1 = immInt2reg(imm, newBlock);
        }
        if (value2 instanceof ASMImmediate imm) {
            value2 = immInt2reg(imm, newBlock);
        }
        Reg reg1 = (Reg) value1;
        Reg reg2 = (Reg) value2;

        switch (cmpInstr.getCond()) {
            // 以下调用的函数实际上生成出来主要是 SLT
            case ILT -> new RVBlt(reg1, reg2, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            case IGT -> new RVBlt(reg2, reg1, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            case IEQ -> new RVBeq(reg1, reg2, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            case INE -> new RVBne(reg1, reg2, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            case ILE -> new RVBge(reg2, reg1, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            case IGE -> new RVBge(reg1, reg2, basicBlockMap.get(branchInstr.getThenBlk()), newBlock);
            default -> throw new RuntimeException("未定义的比较指令");
        }

        genJump(basicBlockMap.get(branchInstr.getElseBlk()), newBlock);
    }

    @Override
    protected ASMValue generateSGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        ASMValue sltVal = generateSLT(value1, value2, newBlock);
        if (sltVal instanceof ASMImmediate immediate) {
            return new ASMImmediate(immediate.getImm() ^ 1);
        } else {
            RVXor xor = new RVXor((Reg) sltVal, ASMImmediate.One, newBlock, new VirIReg());
            return xor.getRegister();
        }
    }

    @Override
    protected ASMValue generateFGE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        ASMValue fltVal = generateFLT(value1, value2, newBlock);
        RVXor xor = new RVXor((Reg) fltVal, ASMImmediate.One, newBlock, new VirIReg());
        return xor.getRegister();
    }

    @Override
    protected ASMValue generateSLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        ASMValue sltVal = generateSGT(value1, value2, newBlock);
        if (sltVal instanceof ASMImmediate immediate) {
            return new ASMImmediate(immediate.getImm() ^ 1);
        } else {
            RVXor xor = new RVXor((Reg) sltVal, ASMImmediate.One, newBlock, new VirIReg());
            return xor.getRegister();
        }
    }

    @Override
    protected ASMValue generateFLE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        ASMValue fltVal = generateFGT(value1, value2, newBlock);
        RVXor xor = new RVXor((Reg) fltVal, ASMImmediate.One, newBlock, new VirIReg());
        return xor.getRegister();
    }

    @Override
    protected ASMValue generateSNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        RVXor xor;
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() != imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                xor = new RVXor(immInt2reg(imm1, newBlock), value2, newBlock, new VirIReg()); // $at
            }
        } else {
            xor = new RVXor((Reg) value1, value2, newBlock, new VirIReg()); // $at
        }
        RVSltu sltu = new RVSltu(regStore.getZero(), xor.getRegister(), newBlock, new VirIReg());
        return sltu.getRegister();
    }

    @Override
    protected ASMValue generateFNE(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        ASMValue feqVal = generateFEQ(value1, value2, newBlock);
        RVXor xor = new RVXor((Reg) feqVal, ASMImmediate.One, newBlock, new VirIReg());
        return xor.getRegister();
    }

    @Override
    protected ASMValue generateSEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        RVXor xor;
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() == imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                xor = new RVXor(immInt2reg(imm1, newBlock), value2, newBlock, new VirIReg()); // cannot be $at
            }
        } else {
            xor = new RVXor((Reg) value1, value2, newBlock, new VirIReg());
        }
        RVSltu sltu = new RVSltu(xor.getRegister(), immInt2reg(ASMImmediate.One, newBlock), newBlock, new VirIReg());
        return sltu.getRegister();
    }

    @Override
    protected ASMValue generateFEQ(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        RVFeq feq = new RVFeq(value1, value2, newBlock, new VirIReg());
        return feq.getRegister();
    }

    @Override
    protected ASMValue generateSGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return generateSLT(value2, value1, newBlock);
    }

    @Override
    protected ASMValue generateFGT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        return generateFLT(value2, value1, newBlock);
    }

    @Override
    protected ASMValue generateSLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        if (value1 instanceof ASMImmediate imm1) {
            if (value2 instanceof ASMImmediate imm2) {
                return imm1.getImm() < imm2.getImm() ? ASMImmediate.One : ASMImmediate.ZERO;
            } else {
                RVlt slt = new RVlt(immInt2reg(imm1, newBlock), value2, newBlock, new VirIReg());
                return slt.getRegister();
            }
        } else {
            RVlt slt = new RVlt(value1, value2, newBlock, new VirIReg());
            return slt.getRegister();
        }
    }

    @Override
    protected ASMValue generateFLT(ASMValue value1, ASMValue value2, ASMBasicBlock newBlock) {
        RVlt slt = new RVlt(value1, value2, newBlock, new VirIReg());
        return slt.getRegister();
    }

    @Override
    protected void genPrologue(ASMFunction asmFunction, ASMBasicBlock firstBlock) {
        genAdd(regStore.getStackPtr(), new ASMNegStackSize(asmFunction), firstBlock, regStore.getStackPtr()); // 移动栈指针
        genStore(new ASMStackOffset(asmFunction, -8), regStore.getStackPtr(), regStore.getRetAddr(), firstBlock, true, false); // 暂存 $ra
        genJump((ASMBasicBlock) firstBlock.getNext(), firstBlock); // 跳转到下一个基本块
    }

    @Override
    protected void genMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst) {
        if (src instanceof ASMImmediate imm) {
            new RVLi(imm, parentBlock, dst);
        } else {
            new RVMove((Reg) src, parentBlock, dst);
        }
    }

    @Override
    protected void genFrozenMove(ASMValue src, ASMBasicBlock parentBlock, Reg dst) {
        if (src instanceof ASMImmediate imm) {
            new RVLi(imm, parentBlock, dst);
        } else {
            new RVMove((Reg) src, parentBlock, dst, true);
        }
    }

    @Override
    protected void genLoad(ASMImmediate offset, ASMValue base, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat) {
        new RVLoad(offset, base, parentBB, reg, isDouble, isFloat);
    }

    @Override
    protected void genLoad(ASMValue ptr, ASMBasicBlock parentBB, Reg reg, boolean isDouble, boolean isFloat, boolean isFltLi) {
        RVLoad load = new RVLoad(ptr, parentBB, reg, isDouble, isFloat);
        if (isFltLi) {
            load.markAsFltLi();
        }
    }

    @Override
    protected void genStore(ASMImmediate offset, ASMValue base, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        new RVStore(offset, base, value, parentBB, isDouble, isFloat);
    }

    @Override
    protected void genStore(ASMValue ptr, ASMValue value, ASMBasicBlock parentBB, boolean isDouble, boolean isFloat) {
        new RVStore(ptr, value, parentBB, isDouble, isFloat);
    }

    @Override
    protected void genLa(ASMGlobalObject globalObject, ASMBasicBlock parentBB, Reg reg) {
        new RVLa(globalObject, parentBB, reg);
    }

    @Override
    protected void genLi(ASMImmediate imm, ASMBasicBlock parentBlock, Reg reg) {
        new RVLi(imm, parentBlock, reg);
    }

    @Override
    protected void genFAdd(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVFAdd(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFSub(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVFSub(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVFMul(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVFDiv(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        Reg zero = regStore.getZero();
        if (operand1 == zero) {
            genMove(operand2, parentBB, reg);
        } else if (operand2 == zero || operand2 instanceof ASMImmediate imm && imm.getImm() == 0) {
            genMove(operand1, parentBB, reg);
        } else {
            new RVAdd(operand1, operand2, parentBB, reg);
        }
    }

    @Override
    protected void genSub(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVSub(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVMul(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVDiv(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genRem(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVRem(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genFtoi(Reg src, ASMBasicBlock block, Reg reg) {
        new RVFtoi(src, block, reg);
    }

    @Override
    protected void genItof(Reg src, ASMBasicBlock block, Reg reg) {
        new RVItof(src, block, reg);
    }

    @Override
    protected void genJump(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        new RVJ(target, parentBlock);
    }

    @Override
    protected void genBeq(Reg operand1, Reg operand2, ASMBasicBlock target, ASMBasicBlock parentBlock) {
        new RVBeq(operand1, operand2, target, parentBlock);
    }

    @Override
    protected void genJr(Reg target, ASMBasicBlock parentBlock) {
        new RVJr(target, parentBlock);
    }

    @Override
    protected void genJal(ASMFunction targetFunc, ASMBasicBlock parentBlock) {
        new RVJal(targetFunc, parentBlock);
    }

    @Override
    protected void genFNeg(ASMValue src, ASMBasicBlock parentBlock, Reg reg) {
        new RVFNeg(src, parentBlock, reg);
    }

    @Override
    protected void genSll(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVSll(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genSra(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVSra(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genSrl(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVSrl(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genAnd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        new RVAnd(operand1, operand2, parentBB, reg);
    }

    @Override
    protected void genAtomAdd(ASMValue ptr, ASMValue inc, ASMBasicBlock parentBB, Reg reg) {
        new RVAtomAdd(inc, ptr, reg, parentBB);
    }

    @Override
    protected void genVecMoveFromSca(VirReg srcVal, int index, ASMBasicBlock parentBB, VirFReg dst) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecMoveToSca(VirFReg srcVal, int index, ASMBasicBlock parentBB, VirReg dst) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecStore(VirFReg valueToStore, Reg basicPointer, ASMBasicBlock parentBB) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecLoad(Reg basicPointer, ASMBasicBlock parentBB, VirFReg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecDup(VirReg scalarValue, ASMBasicBlock parentBB, VirFReg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecMulAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecMove(VirFReg src, ASMBasicBlock parentBlock, VirFReg dst) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genAddv(VirFReg srcVec, ASMBasicBlock parentBlock, VirFReg register) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genBitCopy(Reg src, ASMBasicBlock block, Reg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecAdd(VirFReg op1, VirFReg op2, ASMBasicBlock parentBB, VirFReg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override
    protected void genVecRegClear(ASMBasicBlock parentBB, VirFReg reg) {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}
