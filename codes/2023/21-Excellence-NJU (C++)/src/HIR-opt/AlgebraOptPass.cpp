//
// Created by q1w2e3r4 on 23-8-14.
//

#include "HIR-opt/AlgebraOptPass.h"
#include "HIR-opt/HOptUtils.h"

bool Is_Const(ValueRef * ref);
int get_bits(int x);

void AlgebraOptPass::run() {
    auto * sccpPass = new SccpPass(gu);
    for(auto &[name,func] : gu->func_table){
        if(func->entry == nullptr) continue;
        Reduce(func);
        sccpPass->deadCodeEli(func); // 作一遍DCE消除多余代码
    }
}

void AlgebraOptPass::Reduce(Function *func) {
    //目前实现的消解：
    // add: x + 0 = x; x - 0 = x;   0 - x不应优化
    // x * 0 = 0; x * 1 = x;  x * 2 = x << 1; x * (2^n) = x << n, 0 / x = 0, x / 0 非法，其他同乘法
    for(auto block:func->block_list){
        for(auto it = block->local_instr.begin(); it != block->local_instr.end(); ++it){
            Instruction * instr = *it;
            if(instr->instType == BINARY){
                auto op = dynamic_cast<BinaryInstruction*>(instr);
                bool cst1 = Is_Const(op->src1), cst2 = Is_Const(op->src2);
                if(!cst1 && !cst2) continue;

                ValueRef * var,* cst;
                var = cst1? op->src2 : op->src1;
                cst = cst1? op->src1 : op->src2;
                if(cst->type == IntConst) {
                    int const_value = dynamic_cast<Int_Const*>(cst)->value;
                    switch (op->opTy) {
                        case ADD:{
                            if (const_value == 0) {
                                replaceAllUsesOf(op->dst, var);
                            }
                            break;
                        }
                        case SUB: {
                            if (const_value == 0 && cst2) {
                                replaceAllUsesOf(op->dst, var);
                            }
                            break;
                        }
                        case MUL: {
                            if(const_value == 0){
                                replaceAllUsesOf(op->dst,new Int_Const(0));
                            }
                            else if(const_value == 1){
                                replaceAllUsesOf(op->dst,var);
                            }
                            else if(const_value < 0){
                                if(const_value == -1){
                                    op->dst->def.clear();
                                    IRInstruction *sub = new BinaryInstruction(op->dst,new Int_Const(0),var,SUB);
                                    ReplaceInstr(instr,sub);
                                    break;
                                }
                                else{
                                    int bits = get_bits(-const_value);
                                    if(bits == -1) continue;

                                    op->dst->def.clear();
                                    ValueRef * neg = new Int_Var("neg");
                                    IRInstruction *sub = new BinaryInstruction(neg,new Int_Const(0),var,SUB);
                                    ReplaceInstr(instr,sub);

                                    IRInstruction *sll = new SllInstruction(op->dst,neg,new Int_Const(bits));
                                    sll->block = block;
                                    for(auto it = block->local_instr.begin(); it != block->local_instr.end(); ++it){
                                        if(*it == sub){
                                            block->local_instr.insert(it+1, sll);
                                            break;
                                        }
                                    }
                                }
                            }
                            else{
                                int bits = get_bits(const_value);
                                if(bits != -1){
                                    op->dst->def.clear();
                                    IRInstruction *sll = new SllInstruction(op->dst,var,new Int_Const(bits));
                                    ReplaceInstr(instr,sll);
                                }
                            }
                            break;
                        }
                        case DIV: {
                            if(cst1){
                                if(const_value == 0){
                                    replaceAllUsesOf(op->dst,new Int_Const(0));
                                }
                                break;
                            }

                            // cst2 == true
                            if(const_value == 0){
                                cerr << "div by zero!!" << endl;
                                throw exception();
                            }
                            else if(const_value == 1){
                                replaceAllUsesOf(op->dst,var);
                            }
                            else if(const_value < 0){
                                if(const_value == -1){
                                    op->dst->def.clear();
                                    IRInstruction *sub = new BinaryInstruction(op->dst,new Int_Const(0),var,SUB);
                                    ReplaceInstr(instr,sub);
                                    break;
                                }
                                break;
                                //TODO: 现有条件下除法优化反而是负优化，暂时跳过
                                int bits = get_bits(-const_value);
                                if(bits == -1) continue;
                                op->dst->def.clear();
                                ValueRef * neg = new Int_Var("neg");
                                ValueRef * sig = new Int_Var("sig");
                                ValueRef * vand = new Int_Var("and");
                                ValueRef * vadd = new Int_Var("add");
                                IRInstruction *sub = new BinaryInstruction(neg,new Int_Const(0),var,SUB);
                                IRInstruction *sra = new SraInstruction(sig,neg,new Int_Const(31)); //get sig
                                IRInstruction *and_instr = new BinaryInstruction(vand,sig,new Int_Const((1<<bits)-1),AND);
                                IRInstruction *add = new BinaryInstruction(vadd,var,vand,ADD);
                                IRInstruction *sra1 = new SraInstruction(op->dst,vadd,new Int_Const(bits));
                                sra->block = and_instr->block = add->block = sra1->block = block;

                                ReplaceInstr(instr,sub);
                                for(auto it = block->local_instr.begin(); it != block->local_instr.end(); ++it){
                                    if(*it == sub){
                                        block->local_instr.insert(it+1, {sra,and_instr,add,sra1});
                                        break;
                                    }
                                }
                            }
                            else{
                                break;
                                //TODO: 现有条件下除法优化反而是负优化，暂时跳过
                                //TODO： C的除法要求被除数和余数符号相同，因此对负数除法，不能替换为右移
                                int bits = get_bits(const_value);
                                if(bits != -1){
                                    ValueRef * sig = new Int_Var("sig");
                                    ValueRef * vand = new Int_Var("and");
                                    ValueRef * vadd = new Int_Var("add");
                                    op->dst->def.clear();
                                    IRInstruction *sra = new SraInstruction(sig,var,new Int_Const(31)); //get sig
                                    IRInstruction *and_instr = new BinaryInstruction(vand,sig,new Int_Const((1<<bits)-1),AND);
                                    IRInstruction *add = new BinaryInstruction(vadd,var,vand,ADD);
                                    IRInstruction *sra1 = new SraInstruction(op->dst,vadd,new Int_Const(bits));
                                    sra->block = and_instr->block = add->block = sra1->block = block;

                                    ReplaceInstr(instr,sra);
                                    for(auto it = block->local_instr.begin(); it != block->local_instr.end(); ++it){
                                        if(*it == sra){
                                            block->local_instr.insert(it+1,{and_instr,add,sra1});
                                            break;
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
                else{
                    float const_value = dynamic_cast<Float_Const*>(cst)->value;
                    switch (op->opTy) {
                        case ADD:{
                            if (const_value == 0.0) {
                                replaceAllUsesOf(op->dst, var);
                            }
                            break;
                        }
                        case SUB: {
                            if (const_value == 0.0 && cst2) {
                                replaceAllUsesOf(op->dst, var);
                            }
                            break;
                        }
                        case MUL: {
                            if(const_value == 0.0){
                                replaceAllUsesOf(op->dst,new Float_Const(0.0));
                            }
                            else if(const_value == 1.0){
                                replaceAllUsesOf(op->dst,var);
                            }
                            else if(const_value == -1.0){
                                op->dst->def.clear();
                                IRInstruction *fneg = new FNegInstruction(op->dst,var);
                                ReplaceInstr(instr,fneg);
                            }
                            break;
                        }
                        case DIV: {
                            if(cst1){
                                if(const_value == 0.0){
                                    replaceAllUsesOf(op->dst,new Float_Const(0.0));
                                }
                                break;
                            }

                            // cst2 == true
                            if(const_value == 0.0){
                                cerr << "div by zero!!" << endl;
                                throw exception();
                            }
                            else if(const_value == 1.0){
                                replaceAllUsesOf(op->dst,var);
                            }
                            else if(const_value == -1.0){
                                op->dst->def.clear();
                                IRInstruction *fneg = new FNegInstruction(op->dst,var);
                                ReplaceInstr(instr,fneg);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    }
}

bool Is_Const(ValueRef * ref) {
    return ref->type == IntConst || ref->type == FloatConst;
}

int get_bits(int x){
    assert(x > 1);
    int ret = 0;
    int temp = x;
    while(temp != 1){
        temp >>= 1;
        ++ret;
    }
    if(x == (1 << ret)){
        return ret;
    }
    else{
        return -1;
    }
}