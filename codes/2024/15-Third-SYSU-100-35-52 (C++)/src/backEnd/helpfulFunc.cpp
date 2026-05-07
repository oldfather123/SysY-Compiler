#include "riscv.h"

using namespace std;

bool canBeImm12(int32 x) {
    return (x >= -2048) && (x <= 2047);
}

bool IIsCalleeSave(uint8 reg) {
    return reg == x9 || (reg >= x18 && reg <= x27) || reg == fp || reg == sp || reg == ra;
}

bool IIsCallerSave(uint8 reg) {
    return (reg >= x5 && reg <= x7) || (reg >= x10 && reg <= x17) || (reg >= x28 && reg <= x31);
}

bool FIsCalleeSave(uint8 reg) {
    return (reg >= f8 && reg <= f9) || (reg >= f18 && reg <= f27);
}

bool FIsCallerSave(uint8 sreg) {
    return (sreg >= f0 && sreg <= f7) || (sreg >= f10 && sreg <= f17) || (sreg >= f28 && sreg <= f31);
}

Array<MOperand> getDefs(MInst* I) {
    return IGetDefs(I) + FGetDefs(I);
}

Array<MOperand> IGetDefs(MInst* I) {
    Array<MOperand> users;
    switch(I->tag) {
        case MI_BINARY: {
            users.push(((MI_Binary*)I)->dst);
        } break;
        case MI_SLT: {
            users.push(((MI_Slt*)I)->dst);
        } break;
        case MI_LOAD: {
            users.push(((MI_Load*)I)->reg);
        } break;
        case MI_CLZ: {
            users.push(((MI_Clz*)I)->dst);
        } break;
        case MI_ZEXT: {
            users.push(((MI_Zext*)I)->dst);
        } break;
        case MI_FUNC_CALL: {
            users.push(makeIReg(ra));
            for(uint8 r = x1; r < REG_COUNT; r++) {
                if(IIsCallerSave(r)) {
                    if(r == ra)
                        cout << "ra appear in function call" << endl;
                    users.push(makeIReg(r));
                }
            }
        } break;
        case MI_MOVE: {
            users.push(((MI_Move*)I)->dst);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->toType == S32) {
                users.push(fcvt->dst);
            }
        } break;
        case MI_FCMP: {
            users.push(((MI_FCmp*)I)->dst);
        } break;
        case MI_BRANCH:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_RETURN:
        case MI_COMPARE:
        //----float----
        case MI_FBINARY:
        case MI_FLOAD:
        case MI_FSTORE:
        case MI_FPUSH:
        case MI_FPOP:
        case MI_FMOVE:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return users;
}

Array<MOperand> FGetDefs(MInst* I) {
    Array<MOperand> users;
    switch(I->tag) {
        case MI_FUNC_CALL: {
            for(uint8 r = f0; r < SREG_COUNT; r++) {
                if(FIsCallerSave(r)) {
                    users.push(makeFReg(r));
                }
            }
        } break;
        case MI_FBINARY: {
            users.push(((MI_FBinary*)I)->dst);
        } break;
        case MI_FLOAD: {
            users.push(((MI_FLoad*)I)->reg);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->toType == F32) {
                users.push(fcvt->dst);
            }
        } break;
        case MI_FCMP: {
            users.push(((MI_FCmp*)I)->dst);
        } break;
        case MI_FMOVE: {
            users.push(((MI_FMove*)I)->dst);
        } break;
        case MI_BINARY:
        case MI_BRANCH:
        case MI_SLT:
        case MI_LOAD:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_CLZ:
        case MI_ZEXT:
        case MI_RETURN:
        case MI_COMPARE:
        case MI_MOVE:
        case MI_FSTORE:
        case MI_FPUSH:
        case MI_FPOP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return users;
}

Array<MOperand> getUses(MInst* I, bool funcHasReturnValue) {
    return IGetUses(I, funcHasReturnValue) + FGetUses(I, funcHasReturnValue);
}

Array<MOperand> IGetUses(MInst* I, bool funcHasReturnValue) {
    Array<MOperand> useds;
    switch(I->tag) {
        case MI_BINARY: {
            useds.push(((MI_Binary*)I)->lhs);
            if(((MI_Binary*)I)->rhs.tag != IMM)
                useds.push(((MI_Binary*)I)->rhs);
        } break;
        case MI_BRANCH: {
            auto br_I = (MI_Branch*)I;
            if(br_I->cond != BR_JU) {
                useds.push(br_I->lhs);
                useds.push(br_I->rhs);
            }
        } break;
        case MI_SLT: {
            useds.push(((MI_Slt*)I)->lhs);
            useds.push(((MI_Slt*)I)->rhs);
        } break;
        case MI_LOAD: {
            auto load = (MI_Load*)I;
            if(load->base.tag == VIREG || load->base.tag == IREG)
                useds.push(load->base);
            if(load->offset.tag == VIREG || load->offset.tag == IREG)
                useds.push(load->offset);
        } break;
        case MI_STORE: {
            auto store = (MI_Store*)I;
            useds.push(store->reg);
            if(store->base.tag == VIREG || store->base.tag == IREG)
                useds.push(store->base);
            if(store->offset.tag == VIREG || store->offset.tag == IREG)
                useds.push(store->offset);
        } break;
        case MI_PUSH: {
            auto push = (MI_Push*)I;
            for(auto push_operand : push->operands) {
                useds.push(push_operand);
            }
        } break;
        case MI_CLZ: {
            useds.push(((MI_Clz*)I)->operand);
        } break;
        case MI_ZEXT: {
            useds.push(((MI_Zext*)I)->src);
        } break;
        case MI_FUNC_CALL: {
            auto call = (MI_Func_Call*)I;
            // @TODO: get uses based on arg count
            // @TODO: temporarily use 4 argument regs to keep compatibility
            for(uint8 r = a0; r <= a3; r++) {
                if(r >= call->argCount + a0)
                    break;
                useds.push(makeIReg(r));
            }
        } break;
        case MI_RETURN: {
            useds.push(makeIReg(ra));
            if(funcHasReturnValue) {
                useds.push(makeIReg(a0));
            }
        } break;
        case MI_COMPARE: {
            useds.push(((MI_Compare*)I)->lhs);
            useds.push(((MI_Compare*)I)->rhs);
        } break;
        case MI_MOVE: {
            if(((MI_Move*)I)->src.tag == IREG || ((MI_Move*)I)->src.tag == VIREG) {
                useds.push(((MI_Move*)I)->src);
            }
        } break;
        case MI_FLOAD: {
            auto vldr = (MI_FLoad*)I;
            if(vldr->base.tag != ERRORTYPE)
                useds.push(vldr->base);
            if(vldr->offset.tag != ERRORTYPE)
                useds.push(vldr->offset);
        } break;
        case MI_FSTORE: {
            auto vstr = (MI_FStore*)I;
            if(vstr->base.tag != ERRORTYPE)
                useds.push(vstr->base);
            if(vstr->offset.tag != ERRORTYPE)
                useds.push(vstr->offset);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->fromType == S32) {
                useds.push(fcvt->src);
            }
        } break;
        case MI_FMOVE: {
            if(((MI_FMove*)I)->src.tag == IREG || ((MI_FMove*)I)->src.tag == VIREG) {
                useds.push(((MI_FMove*)I)->src);
            }
        } break;
        case MI_POP:
        //----float----
        case MI_FBINARY:
        case MI_FPUSH:
        case MI_FPOP:
        case MI_FCMP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return useds;
}

Array<MOperand> FGetUses(MInst* I, bool funcHasReturnValue) {
    Array<MOperand> useds;
    switch(I->tag) {
        case MI_FUNC_CALL: {
            for(uint8 r = fa0; r <= fa3; r++) {
                useds.push(makeFReg(r));
            }
        } break;
        case MI_RETURN: {
            if(funcHasReturnValue) {
                useds.push(makeFReg(fa0));
            }
        } break;
        case MI_FBINARY: {
            useds.push(((MI_FBinary*)I)->lhs);
            if(((MI_FBinary*)I)->fneg == 0) {
                useds.push(((MI_FBinary*)I)->rhs);
            }
        } break;
        case MI_FSTORE: {
            useds.push(((MI_FStore*)I)->reg);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->fromType == F32) {
                useds.push(fcvt->src);
            }
        } break;
        case MI_FCMP: {
            useds.push(((MI_FCmp*)I)->lhs);
            useds.push(((MI_FCmp*)I)->rhs);
        } break;
        case MI_FMOVE: {
            if(!(((MI_FMove*)I)->fromS32)) {
                useds.push(((MI_FMove*)I)->src);
            }
        } break;
        case MI_BINARY:
        case MI_BRANCH:
        case MI_SLT:
        case MI_LOAD:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_CLZ:
        case MI_ZEXT:
        case MI_COMPARE:
        case MI_MOVE:
        case MI_FLOAD:
        case MI_FPUSH:
        case MI_FPOP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return useds;
}

Array<MOperand*> getDefsPtr(MInst* I) {
    return IGetDefsPtr(I) + FGetDefsPtr(I);
}

Array<MOperand*> IGetDefsPtr(MInst* I) {
    Array<MOperand*> users;
    switch(I->tag) {
        case MI_BINARY: {
            users.push(&(((MI_Binary*)I)->dst));
        } break;
        case MI_SLT: {
            users.push(&((MI_Slt*)I)->dst);
        } break;
        case MI_LOAD: {
            users.push(&(((MI_Load*)I)->reg));
        } break;
        case MI_CLZ: {
            users.push(&(((MI_Clz*)I)->dst));
        } break;
        case MI_ZEXT: {
            users.push(&(((MI_Zext*)I)->dst));
        } break;
        case MI_MOVE: {
            users.push(&(((MI_Move*)I)->dst));
        } break;
        case MI_FCMP: {
            users.push(&((MI_FCmp*)I)->dst);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->toType == S32) {
                users.push(&fcvt->dst);
            }
        } break;
        case MI_BRANCH:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_FUNC_CALL:
        case MI_RETURN:
        case MI_COMPARE:
        //----float----
        case MI_FBINARY:
        case MI_FLOAD:
        case MI_FSTORE:
        case MI_FPUSH:
        case MI_FPOP:
        case MI_FMOVE:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return users;
}

Array<MOperand*> FGetDefsPtr(MInst* I) {
    Array<MOperand*> users;
    switch(I->tag) {
        case MI_FBINARY: {
            users.push(&((MI_FBinary*)I)->dst);
        } break;
        case MI_FLOAD: {
            users.push(&((MI_FLoad*)I)->reg);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->toType == F32) {
                users.push(&fcvt->dst);
            }
        } break;
        case MI_FCMP: {
            users.push(&((MI_FCmp*)I)->dst);
        } break;
        case MI_FMOVE: {
            users.push(&((MI_FMove*)I)->dst);
        } break;
        case MI_BINARY:
        case MI_BRANCH:
        case MI_SLT:
        case MI_LOAD:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_CLZ:
        case MI_ZEXT:
        case MI_FUNC_CALL:
        case MI_RETURN:
        case MI_COMPARE:
        case MI_MOVE:
        case MI_FSTORE:
        case MI_FPUSH:
        case MI_FPOP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return users;
}

Array<MOperand*> getUsesPtr(MInst* I, bool funcHasReturnValue) {
    return IGetUsesPtr(I, funcHasReturnValue) + FGetUsesPtr(I, funcHasReturnValue);
}

Array<MOperand*> IGetUsesPtr(MInst* I, bool funcHasReturnValue) {
    Array<MOperand*> useds;
    switch(I->tag) {
        case MI_BINARY: {
            useds.push(&((MI_Binary*)I)->lhs);
            useds.push(&((MI_Binary*)I)->rhs);
        } break;
        case MI_BRANCH: {
            auto br_I = (MI_Branch*)I;
            if(br_I->cond != BR_JU) {
                useds.push(&br_I->lhs);
                useds.push(&br_I->rhs);
            }
        } break;
        case MI_SLT: {
            useds.push(&((MI_Slt*)I)->lhs);
            useds.push(&((MI_Slt*)I)->rhs);
        } break;
        case MI_LOAD: {
            auto load = (MI_Load*)I;
            if(load->base.tag != ERRORTYPE)
                useds.push(&load->base);
            if(load->offset.tag != ERRORTYPE)
                useds.push(&load->offset);
        } break;
        case MI_STORE: {
            auto store = (MI_Store*)I;
            useds.push(&store->reg);
            if(store->base.tag != ERRORTYPE)
                useds.push(&store->base);
            if(store->offset.tag != ERRORTYPE)
                useds.push(&store->offset);
        } break;
        case MI_CLZ: {
            useds.push(&((MI_Clz*)I)->operand);
        } break;
        case MI_ZEXT: {
            useds.push(&((MI_Zext*)I)->dst);
        } break;
        case MI_COMPARE: {
            useds.push(&((MI_Compare*)I)->lhs);
            useds.push(&((MI_Compare*)I)->rhs);
        } break;
        case MI_MOVE: {
            if(((MI_Move*)I)->src.tag == IREG || ((MI_Move*)I)->src.tag == VIREG) {
                useds.push(&((MI_Move*)I)->src);
            }
        } break;
        case MI_FLOAD: {
            auto vldr = (MI_FLoad*)I;
            if(vldr->base.tag != ERRORTYPE)
                useds.push(&vldr->base);
            if(vldr->offset.tag != ERRORTYPE)
                useds.push(&vldr->offset);
        } break;
        case MI_FSTORE: {
            auto vstr = (MI_FStore*)I;
            if(vstr->base.tag != ERRORTYPE)
                useds.push(&vstr->base);
            if(vstr->offset.tag != ERRORTYPE)
                useds.push(&vstr->offset);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->fromType == S32) {
                useds.push(&fcvt->src);
            }
        } break;
        case MI_FMOVE: {
            if(((MI_FMove*)I)->src.tag == IREG || ((MI_FMove*)I)->src.tag == VIREG) {
                useds.push(&((MI_FMove*)I)->src);
            }
        } break;
        case MI_PUSH:
        case MI_POP:
        case MI_FUNC_CALL:
        case MI_RETURN:
        //----float----
        case MI_FBINARY:
        case MI_FPUSH:
        case MI_FPOP:
        case MI_FCMP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return useds;
}

Array<MOperand*> FGetUsesPtr(MInst* I, bool funcHasReturnValue) {
    Array<MOperand*> useds;
    switch(I->tag) {
        case MI_FBINARY: {
            useds.push(&((MI_FBinary*)I)->lhs);
            if(((MI_FBinary*)I)->fneg == 0) {
                useds.push(&((MI_FBinary*)I)->rhs);
            }
        } break;
        case MI_FSTORE: {
            useds.push(&((MI_FStore*)I)->reg);
        } break;
        case MI_FCVT: {
            auto fcvt = (MI_FCvt*)I;
            if(fcvt->fromType == F32) {
                useds.push(&fcvt->src);
            }
        } break;
        case MI_FCMP: {
            useds.push(&((MI_FCmp*)I)->lhs);
            useds.push(&((MI_FCmp*)I)->rhs);
        } break;
        case MI_FMOVE: {
            if(!(((MI_FMove*)I)->fromS32)) {
                useds.push(&((MI_FMove*)I)->src);
            }
        } break;
        // 'base' and 'offset' are vreg not vsreg
        case MI_BINARY:
        case MI_BRANCH:
        case MI_SLT:
        case MI_LOAD:
        case MI_STORE:
        case MI_PUSH:
        case MI_POP:
        case MI_CLZ:
        case MI_ZEXT:
        case MI_FUNC_CALL:
        case MI_RETURN:
        case MI_COMPARE:
        case MI_MOVE:
        case MI_FLOAD:
        case MI_FPUSH:
        case MI_FPOP:
            break;
        default: {
            assert(false && "meet unknown inst type");
        }
    }
    return useds;
}
