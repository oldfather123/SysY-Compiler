//
// Created by 牛奕博 on 2023/6/1.
//

#ifndef ANUC_LOWERTOLIR_H
#define ANUC_LOWERTOLIR_H

#include <queue>
#include "core.h"
#include "irBuilder.h"
#include "lowInst.h"
#include "rvValue.h"

namespace anuc {
    class LIRVisitor : public Visitor {
    protected:
        //将浮点数转换为ieee754十进制数
        static int getFloatImm(float number) {
            union FloatConverter {
                float floatValue;
                uint32_t intValue;
            };
            FloatConverter converter;
            converter.floatValue = number;
            return converter.intValue;

        }

        static bool isImm(Value *v) {
            if (isa<Constant>(v)) return true;
            return false;
        }

        static bool isImm(Value *v, Value *last ...) {
            return isImm(v) || isImm(last);
        }

    public:
        //判断能不能移动位置
        static int getShift(int i) {
            switch (i) {
                case 2:
                    return 1;
                case 4:
                    return 2;
                case 8:
                    return 3;
                case 16:
                    return 4;
                case 32:
                    return 5;
                case 64:
                    return 6;
                case 128:
                    return 7;
                case 256:
                    return 8;
                case 512:
                    return 9;
                case 1024:
                    return 10;
                case 2048:
                    return 11;
                case 4096:
                    return 12;
                case 8192:
                    return 13;
                case 16384:
                    return 14;
                case 32768:
                    return 15;
                default:
                    return -1;
            }
        }

        virtual bool visit(LowLoad *inst) { return 1; }

        virtual bool visit(LowStore *inst) { return 1; }

        virtual bool visit(LowGEP *inst) { return 1; }
        virtual bool visit(RVcondbranch *inst) {return 1;}

    };


    //将多条gep压缩为一条
    //原DAG搜索设计已经弃用
    class LIRVisitor0 : public LIRVisitor {
        IRBuilder *Builder;

        void lowGep(GEPInst *gep);

    public:
        LIRVisitor0(IRBuilder *Builder) : Builder(Builder) {}

        bool visit(GEPInst *inst);

        bool visit(AllocateInst *inst);

    };

    class LIRVisitor1 : public LIRVisitor {
        IRBuilder *Builder;
        RegTable *regTable;

    public:
        LIRVisitor1(IRBuilder *Builder, RegTable *regTable) : Builder(Builder), regTable(regTable) {}

        bool visit(GEPInst *inst);

        bool visit(AllocateInst *inst);

        bool visit(PhiInst *inst);

        bool visit(RetInst *inst);

        bool visit(LoadInst *inst);

        bool visit(StoreInst *inst);

        bool visit(FAddInst *inst);

        bool visit(FSubInst *inst);

        bool visit(FMulInst *inst);

        bool visit(FDivInst *inst);

        bool visit(CondBranchInst *inst);

        bool visit(BranchInst *inst);

        bool visit(FToIInst *inst);

        bool visit(IToFInst *inst);


        static void transformFASMD(Instruction *inst, IRBuilder *Builder, RVfasmd::OpKind opKind) {
            BasicBlock *bb = inst->getParent();
            Value *l = inst->getOperands(0)->value;
            Value *r = inst->getOperands(1)->value;
            Value *result = inst->getResult();
            if (LIRVisitor::isImm(l, r)) {
                ConstantFloat *lImm = dyn_cast<ConstantFloat>(l);
                ConstantFloat *rImm = dyn_cast<ConstantFloat>(r);
                if (lImm && rImm) {
                    ConstantFloat *imm;
                    switch (opKind) {
#define CASE_COMPUTE_FIMM(X, OP) \
case RVfasmd::X: imm = Builder->GetConstantFloat(lImm->getValue() OP rImm->getValue()); \
break;
                        CASE_COMPUTE_FIMM(fadd, +)
                        CASE_COMPUTE_FIMM(fsub, -)
                        CASE_COMPUTE_FIMM(fmul, *)
                        CASE_COMPUTE_FIMM(fdiv, /)
                    }
                    RegisterVar *dest = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
                    FloatLoad *lowInst = new FloatLoad(bb, dest, imm);
                    bb->insertIntoBackChild(inst, lowInst);
                    inst->getResult()->replaceAllUseWith(dest);
                    inst->eraseFromParent();
                    Builder->InsertIntoPool(lowInst, dest);
                    return;
                } else {
                    float value;
                    RegisterVar *rs1;
                    if (lImm) {
                        value = lImm->getValue();
                        rs1 = cast<RegisterVar>(r);
                    } else {
                        value = rImm->getValue();
                        rs1 = cast<RegisterVar>(l);
                    }
                    ConstantFloat *imm = Builder->GetConstantFloat(value);
                    RegisterVar *rs2 = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
                    FloatLoad *lowInst = new FloatLoad(bb, rs2, imm);
                    bb->insertIntoBackChild(inst, lowInst);
                    inst->eraseFromParent();

                    RegisterVar *dest = cast<RegisterVar>(result);
                    RVfasmd *rvInst = new RVfasmd(bb, rs1, rs2, dest, opKind);
                    bb->insertIntoBackChild(lowInst, rvInst);
                    Builder->InsertIntoPool(rvInst, lowInst, rs2);
                    return;
                }
            } else {
                RegisterVar *rs1 = cast<RegisterVar>(l);
                RegisterVar *rs2 = cast<RegisterVar>(r);
                RegisterVar *dest = cast<RegisterVar>(result);
                RVfasmd *rvInst = new RVfasmd(bb, rs1, rs2, dest, opKind);
                bb->insertIntoBackChild(inst, rvInst);
                inst->eraseFromParent();
                Builder->InsertIntoPool(rvInst);
                return;
            }
        }

        static void transformCondBranch(Instruction *inst, IRBuilder *Builder) {

            BasicBlock *bb = inst->getParent();
            BasicBlock *trueBB = cast<BasicBlock>(inst->getOperands(1)->value);
            BasicBlock *falseBB = cast<BasicBlock>(inst->getOperands(2)->value);
            Value *condValue = inst->getOperands(0)->value;
            if(condValue->getDef() && isa<ZExtInst>(condValue->getDef())) {
                auto zext = cast<ZExtInst>(condValue->getDef());
                zext->getSrc()->setType(Builder->GetInt32Ty());
                zext->getResult()->replaceAllUseWith(zext->getSrc());
                //zext->eraseFromParent();
            }
            //if(condValue->getDef()) condValue->getDef()->print();
            condValue = inst->getOperands(0)->value;
            //cond位置为常数
            //可以直接跳转
            if (ConstantInt *cond = dyn_cast<ConstantInt>(condValue)) {
                int value = cond->getValue();
                BasicBlock *label;
                if (value) label = trueBB;
                else label = falseBB;
                RVbranch *rvInst = new RVbranch(bb, label);
                bb->insertIntoBackChild(inst, rvInst);
                Builder->InsertIntoPool(rvInst);
                inst->eraseFromParent();
                return;
            }
            //cond位置为cmp计算结果
            else {
                RegisterVar *condReg = cast<RegisterVar>(condValue);
                Instruction *condInst = condReg->getDef();
                //看看是浮点数还是整数
                Type *ty = condInst->getOperands(0)->value->getType();
                //整数
                if (isa<Int32Type>(ty)) {
                    RVcondbranch::OpKind opKind;
                    //得到类型
#define GET_COND_INST_OP(KIND, OPKIND) if(isa<ICmp##KIND>(condInst)) \
opKind = RVcondbranch::OPKIND;
                    GET_COND_INST_OP(EQ, beq)
                    else GET_COND_INST_OP(NE, bne)
                    else GET_COND_INST_OP(LT, blt)
                    else GET_COND_INST_OP(LE, ble)
                    else GET_COND_INST_OP(GT, bgt)
                    else GET_COND_INST_OP(GE, bge)

                    Value *l = condInst->getOperands(0)->value;
                    Value *r = condInst->getOperands(1)->value;
                    //存在一个立即数
                    if (LIRVisitor::isImm(l, r)) {
                        ConstantInt *imm;
                        ConstantInt *lImm = dyn_cast<ConstantInt>(l);
                        ConstantInt *rImm = dyn_cast<ConstantInt>(r);
                        //同为立即数，算出结果
                        if (lImm && rImm) {
                            switch (opKind) {
#define CASE_CMP_IMM(X, OP) \
case RVcondbranch::X: imm = Builder->GetConstantInt32(lImm->getValue() OP rImm->getValue()); \
break;
                                CASE_CMP_IMM(beq, ==)
                                CASE_CMP_IMM(bne, !=)
                                CASE_CMP_IMM(blt, <)
                                CASE_CMP_IMM(ble, <=)
                                CASE_CMP_IMM(bgt, >)
                                CASE_CMP_IMM(bge, >=)
                            }
                            int value = imm->getValue();
                            BasicBlock *label;
                            if (value) label = trueBB;
                            else label = falseBB;
                            RVbranch *rvInst = new RVbranch(bb, label);
                            bb->insertIntoBackChild(inst, rvInst);
                            Builder->InsertIntoPool(rvInst);
                            inst->eraseFromParent();
                            return;
                        }
                        //对一个立即数进行li
                        else {
                            int value;
                            RegisterVar *rs1;
                            if (lImm) {
                                value = lImm->getValue();
                                rs1 = cast<RegisterVar>(r);
                            } else {
                                value = rImm->getValue();
                                rs1 = cast<RegisterVar>(l);
                            }
                            RegisterVar *rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                            imm = Builder->GetConstantInt32(value);
                            RVli *li = new RVli(bb, imm, rs2);
                            Builder->InsertIntoPool(li);
                            RVcondbranch *rvInst = new RVcondbranch(bb, rs1, rs2, trueBB, opKind);
                            RVbranch *rvInst2 = new RVbranch(bb, falseBB);
                            bb->insertIntoBackChild(inst, li);
                            bb->insertIntoBackChild(li, rvInst);
                            bb->insertIntoBackChild(rvInst, rvInst2);
                            inst->eraseFromParent();
                            Builder->InsertIntoPool(rvInst, rvInst2, li);
                            return;
                        }
                    } else {
                        RegisterVar *rs1 = cast<RegisterVar>(l);
                        RegisterVar *rs2 = cast<RegisterVar>(r);
                        RVcondbranch *rvInst = new RVcondbranch(bb, rs1, rs2, trueBB, opKind);
                        RVbranch *rvInst2 = new RVbranch(bb, falseBB);
                        bb->insertIntoBackChild(inst, rvInst);
                        bb->insertIntoBackChild(rvInst, rvInst2);
                        inst->eraseFromParent();
                        Builder->InsertIntoPool(rvInst);
                    }
                }
                //浮点数
                else if (isa<FloatType>(ty)) {
                    RVfcmp::OpKind opKind;
                    //得到类型
#define GET_FCOND_INST_OP(KIND, OPKIND) if(isa<FCmp##KIND>(condInst)) \
opKind = RVfcmp::OPKIND;
                    GET_FCOND_INST_OP(EQ, feq)
                    else GET_FCOND_INST_OP(LT, flt)
                    else GET_FCOND_INST_OP(LE, fle)
                    else GET_FCOND_INST_OP(GT, fgt)
                    else GET_FCOND_INST_OP(GE, fge)
                    Value *l = condInst->getOperands(0)->value;
                    Value *r = condInst->getOperands(1)->value;
                    if (LIRVisitor::isImm(l, r)) {
                        ConstantInt *imm;
                        ConstantFloat *lImm = dyn_cast<ConstantFloat>(l);
                        ConstantFloat *rImm = dyn_cast<ConstantFloat>(r);
                        //同为立即数，算出结果
                        if (lImm && rImm) {
                            switch (opKind) {
#define CASE_FCMP_IMM(X, OP) \
case RVfcmp::X: imm = Builder->GetConstantInt32(lImm->getValue() OP rImm->getValue()); \
break;
                                CASE_FCMP_IMM(feq, ==)
                                CASE_FCMP_IMM(flt, <)
                                CASE_FCMP_IMM(fle, <=)
                                CASE_FCMP_IMM(fgt, >)
                                CASE_FCMP_IMM(fge, >=)
                                default:
                                    imm = Builder->GetConstantInt32(lImm->getValue() != rImm->getValue());
                            }
                            int value = imm->getValue();
                            BasicBlock *label;
                            if (value) label = trueBB;
                            else label = falseBB;
                            RVbranch *rvInst = new RVbranch(bb, label);
                            bb->insertIntoBackChild(inst, rvInst);
                            Builder->InsertIntoPool(rvInst);
                            inst->eraseFromParent();
                            return;
                        } else {
                            float value;
                            RegisterVar *rs1;
                            if (lImm) {
                                value = lImm->getValue();
                                rs1 = cast<RegisterVar>(r);
                            } else {
                                value = rImm->getValue();
                                rs1 = cast<RegisterVar>(l);
                            }
                            RegisterVar *dest = cast<RegisterVar>(condInst->getResult());
                            RegisterVar *rs2 = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
                            ConstantFloat *fImm = Builder->GetConstantFloat(value);
                            FloatLoad *fl = new FloatLoad(bb, rs2, fImm);
                            RVfcmp *rvInst = new RVfcmp(bb, rs1, rs2, dest, opKind);
                            bb->insertIntoBackChild(condInst, fl);
                            bb->insertIntoBackChild(fl, rvInst);
                            condInst->eraseFromParent();
                            RVzerocondbranch::OpKind zOpKind = RVzerocondbranch::beqz;
                            RVzerocondbranch *rvInst2 = new RVzerocondbranch(bb, dest, trueBB, zOpKind);
                            RVbranch *rvInst3 = new RVbranch(bb, falseBB);
                            bb->insertIntoBackChild(inst, rvInst2);
                            bb->insertIntoBackChild(rvInst2, rvInst3);
                            inst->eraseFromParent();
                            Builder->InsertIntoPool(rs2, fl, rvInst, rvInst2, rvInst3);
                            return;
                        }
                    }
                    RegisterVar *rs1 = cast<RegisterVar>(l);
                    RegisterVar *rs2 = cast<RegisterVar>(r);
                    RegisterVar *dest = cast<RegisterVar>(condInst->getResult());
                    RVfcmp *rvInst = new RVfcmp(bb, rs1, rs2, dest, opKind);
                    RVzerocondbranch *rvInst2 = new RVzerocondbranch(bb, dest, trueBB, RVzerocondbranch::beqz);
                    RVbranch *rvInst3 = new RVbranch(bb, falseBB);
                    bb->insertIntoBackChild(inst, rvInst2);
                    bb->insertIntoBackChild(rvInst2, rvInst3);
                    inst->eraseFromParent();
                    Builder->InsertIntoPool(rvInst, rvInst2, rvInst3);
                    return;
                }
            }
        }

        static void transformIToF(Instruction *inst, IRBuilder *Builder) {
            BasicBlock *bb = inst->getParent();
            IToFInst *itf = cast<IToFInst>(inst);
            RegisterVar *dest = cast<RegisterVar>(itf->getResult());
            Value *src = itf->getSrc();
            RegisterVar *rs1;
            //如果为立即数，需要先li
            if (ConstantInt *imm = dyn_cast<ConstantInt>(src)) {
                rs1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                RVli *li = new RVli(bb, imm, rs1);
                bb->insertIntoChild(li, inst);
                Builder->InsertIntoPool(rs1, li);
            } else {
                rs1 = cast<RegisterVar>(src);
            }
            RVIToF *rvInst = new RVIToF(bb, dest, rs1);
            bb->insertIntoChild(rvInst, inst);
            Builder->InsertIntoPool(rvInst);
            inst->eraseFromParent();
        }

        static void transformFToI(Instruction *inst, IRBuilder *Builder) {
            BasicBlock *bb = inst->getParent();
            FToIInst *itf = cast<FToIInst>(inst);
            RegisterVar *dest = cast<RegisterVar>(itf->getResult());
            Value *src = itf->getSrc();
            RegisterVar *rs1;
            //如果为立即数，需要先FloatLoad
            if (ConstantFloat *imm = dyn_cast<ConstantFloat>(src)) {
                rs1 = new RegisterVar(Builder->GetFloatTy(), Builder->GetNewVarName());
                FloatLoad *fl = new FloatLoad(bb, rs1, imm);
                bb->insertIntoChild(fl, inst);
                Builder->InsertIntoPool(rs1, fl);
            } else {
                rs1 = cast<RegisterVar>(src);
            }
            RVFToI *rvInst = new RVFToI(bb, dest, rs1);
            bb->insertIntoChild(rvInst, inst);
            Builder->InsertIntoPool(rvInst);
            inst->eraseFromParent();
        }


    };

    class LIRVisitor2 : public LIRVisitor {
        IRBuilder *Builder;
        RegTable *regTable;
    public:
        LIRVisitor2(IRBuilder *Builder, RegTable *regTable
        ) : Builder(Builder), regTable(regTable) {}

        bool visit(AddInst *inst);

        bool visit(SubInst *inst);

        bool visit(MulInst *inst);

        bool visit(DivInst *inst);

        bool visit(RemInst *inst);

        bool visit(ICmpEQ *inst);

        bool visit(ICmpNE *inst);

        bool visit(ICmpGE *inst);

        bool visit(ICmpGT *inst);

        bool visit(ICmpLE *inst);

        bool visit(ICmpLT *inst);

        bool visit(FCmpEQ *inst);

        bool visit(FCmpNE *inst);

        bool visit(FCmpGE *inst);

        bool visit(FCmpGT *inst);

        bool visit(FCmpLE *inst);

        bool visit(FCmpLT *inst);

        bool visit(ZExtInst *inst);

        bool visit(LowGEP *inst);

        bool visit(RVcondbranch *inst);

        static void transformASMD(Instruction *inst, IRBuilder *Builder, RVasmd::OpKind opKind) {
            BasicBlock *bb = inst->getParent();
            Value *l = inst->getOperands(0)->value;
            Value *r = inst->getOperands(1)->value;
            Value *result = inst->getResult();
            if (LIRVisitor::isImm(l, r)) {
                ConstantInt *lImm = dyn_cast<ConstantInt>(l);
                ConstantInt *rImm = dyn_cast<ConstantInt>(r);
                if (lImm && rImm) {
                    //解决两个变量均为立即数的情况
                    ConstantInt *imm;
                    switch (opKind) {
#define CASE_COMPUTE_IMM(X, OP) case  RVasmd::X: imm = Builder->GetConstantInt32(lImm->getValue() OP rImm->getValue()); break;
                        CASE_COMPUTE_IMM(add, +)
                        CASE_COMPUTE_IMM(sub, -)
                        CASE_COMPUTE_IMM(mul, *)
                        CASE_COMPUTE_IMM(div, /)
                        CASE_COMPUTE_IMM(rem, %)
                    }
                    RegisterVar *dest = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                    RVli *rvInst = new RVli(bb, imm, dest);
                    bb->insertIntoBackChild(inst, rvInst);
                    inst->getResult()->replaceAllUseWith(dest);
                    inst->eraseFromParent();
                    Builder->InsertIntoPool(rvInst, dest);
                } else {
                    //解决只有一个立即数的问题
                    int value;
                    RegisterVar *rs1{nullptr};
                    RegisterVar *rs2{nullptr};
                    RegisterVar *addi{nullptr};
                    if (lImm) {
                        value = lImm->getValue();
                        rs2 = cast<RegisterVar>(r);
                        addi = rs2;
                    } else {
                        value = rImm->getValue();
                        rs1 = cast<RegisterVar>(l);
                        addi = rs1;
                    }
                    RegisterVar *dest = cast<RegisterVar>(result);
                    ConstantInt *imm = Builder->GetConstantInt32(value);
                    if (value <= 2017 && (opKind == RVasmd::add ||
                                          (opKind == RVasmd::sub && rs1))) {
                        //加能转换为addi
                        Instruction *rvInst;
                        if (opKind == RVasmd::sub) imm = Builder->GetConstantInt32(-value);
                        rvInst = new RVaddi(bb, addi, imm, dest);
                        bb->insertIntoBackChild(inst, rvInst);
                        inst->eraseFromParent();
                        Builder->InsertIntoPool(rvInst);
                        return;

                    }
                    //乘除模
                    RVli *li{nullptr};
                    if (!rs2) {
                        rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                        li = new RVli(bb, imm, rs2);
                    } else {
                        rs1 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                        li = new RVli(bb, imm, rs1);
                    }
                    bb->insertIntoBackChild(inst, li);
                    inst->eraseFromParent();
                    Builder->InsertIntoPool(li);
                    RVasmd *rvInst = new RVasmd(bb, rs1, rs2, dest, opKind);
                    bb->insertIntoBackChild(li, rvInst);
                    Builder->InsertIntoPool(rvInst, li);
                    return;
                }
            } else {
                RegisterVar *rs1 = cast<RegisterVar>(l);
                RegisterVar *rs2 = cast<RegisterVar>(r);
                RegisterVar *dest = cast<RegisterVar>(result);
                RVasmd *rvInst = new RVasmd(bb, rs1, rs2, dest, opKind);
                bb->insertIntoBackChild(inst, rvInst);
                inst->eraseFromParent();
                Builder->InsertIntoPool(rvInst);
            }
        }

        //此时cmp应该只剩下用来处理！的了
        //借用condbranch的opkind，
        static void transformICmp(Instruction *inst, IRBuilder *Builder, RVcondbranch::OpKind opKind) {
            //use链为空，直接删除
            if (inst->getResult()->usesEmpty()) {
                inst->eraseFromParent();
                return;
            }
            //进行进行其他操作
            BasicBlock *bb = inst->getParent();
            Value *l = inst->getOperands(0)->value;
            Value *r = inst->getOperands(1)->value;
            RegisterVar *dest = cast<RegisterVar>(inst->getResult());
            if (LIRVisitor::isImm(l, r)) {
                ConstantInt *lImm = dyn_cast<ConstantInt>(l);
                ConstantInt *rImm = dyn_cast<ConstantInt>(r);
                //如果都为常数，直接rauw
                if (lImm && rImm) {
                    int l = lImm->getValue();
                    int r = rImm->getValue();
                    ConstantInt *x = Builder->GetConstantInt32(l  ==  r);
                    dest->replaceAllUseWith(x);
                    inst->eraseFromParent();
                    return;
                }
                else {
                    int value;
                    RegisterVar *rs1;
                    if (lImm) {
                        value = lImm->getValue();
                        rs1 = cast<RegisterVar>(r);
                    } else {
                        value = rImm->getValue();
                        rs1 = cast<RegisterVar>(l);
                    }
                    //为0，创建0比较
                    if (value == 0) {
                        RVZeroCmp::OpKind zOpKind;
                        switch (opKind) {
#define ZOP_CASE(OPKIND, ZOPKIND) case RVcondbranch::OPKIND: zOpKind = RVZeroCmp::ZOPKIND; break;
                            ZOP_CASE(beq, seqz)
                            ZOP_CASE(bne, snez)
                            ZOP_CASE(bgt, sgtz)
                            ZOP_CASE(blt, sltz)
                            default:
                                break;
                        }
                        RVZeroCmp *rvInst = new RVZeroCmp(bb, rs1, dest, zOpKind);
                        bb->insertIntoBackChild(inst, rvInst);
                        inst->eraseFromParent();
                        Builder->InsertIntoPool(rvInst);
                    }

                }
            }


        }

        //fcmp
        //floatload一个0，再用fcmp比较
        static void
        transformFCmp(Instruction *inst, IRBuilder *Builder, RegTable *regTable, RVcondbranch::OpKind opKind) {
            //use链为空，直接删除
            if (inst->getResult()->usesEmpty()) {
                inst->eraseFromParent();
                return;
            }
            BasicBlock *bb = inst->getParent();
            Value *l = inst->getOperands(0)->value;
            Value *r = inst->getOperands(1)->value;
            RegisterVar *dest = cast<RegisterVar>(inst->getResult());
            if (LIRVisitor::isImm(l, r)) {
                ConstantFloat *lImm = dyn_cast<ConstantFloat>(l);
                ConstantFloat *rImm = dyn_cast<ConstantFloat>(r);
                if (lImm && rImm) {
                    float l = lImm->getValue();
                    float r = rImm->getValue();
                    ConstantInt *x = Builder->GetConstantInt32(fabs(l - r) < 0.00001);
                    dest->replaceAllUseWith(x);
                    inst->eraseFromParent();
                    return;
                }
                else {
                    int value;
                    RegisterVar *rs1;
                    if (lImm) {
                        value = lImm->getValue();
                        rs1 = cast<RegisterVar>(r);
                    } else {
                        value = rImm->getValue();
                        rs1 = cast<RegisterVar>(l);
                    }
                    //为0，创建0比较
                    if (value == 0) {
                        RVfcmp::OpKind zOpKind;
                        switch (opKind) {
#define FCMP_CASE(OPKIND, ZOPKIND) case RVcondbranch::OPKIND: zOpKind = RVfcmp::ZOPKIND; break;
                            FCMP_CASE(beq, feq)
                            default:
                                break;
                        }
                        //zero只是一个占位置的，实际已经给他分配了寄存器
                        RegisterVar *zero = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                        RegisterVar *rs2 = new RegisterVar(Builder->GetInt32Ty(), Builder->GetNewVarName());
                        RVIToF *itf = new RVIToF(bb, rs2, regTable->getReg(RvRegister::zero));
                        RVfcmp *rvInst = new RVfcmp(bb, rs1, rs2, dest, zOpKind);
                        bb->insertIntoBackChild(inst, itf);
                        bb->insertIntoBackChild(itf, rvInst);
                        inst->eraseFromParent();
                        Builder->InsertIntoPool(rvInst);
                    }

                }
            }


        }

    private:
        bool isImm(Value *v) {
            if (isa<Constant>(v)) return true;
            return false;
        }

        bool isImm(Value *v, Value *last ...) {
            return isImm(v) || isImm(last);
        }
    };

    //进行重定向，处理lowLoad和lowStore以及lowgep
    class LIRVisitor3 : public LIRVisitor {
        IRBuilder *Builder;
        RegTable *regTable;
    public:
        LIRVisitor3(IRBuilder *Builder, RegTable *regTable
        ) : Builder(Builder), regTable(regTable) {}

        bool visit(LowLoad *inst);

        bool visit(LowStore *inst);

        bool visit(LowGEP *inst);

    };

    //处理call
    class LIRVisitor4 : public LIRVisitor {
        IRBuilder *Builder;
        RegTable *regTable;
    public:
        LIRVisitor4(IRBuilder *Builder, RegTable *regTable
        ) : Builder(Builder), regTable(regTable) {}

    };

}


#endif //ANUC_LOWERTOLIR_H
