//
// Created by q1w2e3r4 on 23-8-16.
//

#include "HIR-opt/GVNPass.h"
#include <stack>
#include <set>

stack<BasicBlock*> st1;
set<BasicBlock*> vis1;

map<int,ValueRef*> const_int;
map<float,ValueRef*> const_float;
map<UnaryType,map<ValueRef*,ValueRef*>> UnaryExp;
map<BinaryType,map<pair<ValueRef*,ValueRef*>,ValueRef*>> BinaryExp;

void PostOrder(BasicBlock * bb){
    if(vis1.count(bb)) return;
    vis1.insert(bb);
    for(auto succ: bb->succ){
        PostOrder(succ);
    }
    st1.push(bb);
}

ValueRef* Update_Const(ValueRef* ref){
    if(ref->type == IntConst){
        int val = dynamic_cast<Int_Const*>(ref)->value;
        if(const_int.find(val) == const_int.end()){
            const_int[val] = ref;
            return ref;
        }
        else{
            return const_int[val];
        }
    }
    else if(ref->type == FloatConst){
        float val = dynamic_cast<Float_Const*>(ref)->value;
        if(const_float.find(val) == const_float.end()){
            const_float[val] = ref;
            return ref;
        }
        else{
            return const_float[val];
        }
    }
    return ref;
}

void Update(BinaryType type, ValueRef* op1, ValueRef* op2, ValueRef* res){
//    cerr << op1 << " " << op2 << endl;
    auto& mp = BinaryExp[type];
    if(mp.find({op1,op2}) == mp.end()){
        mp[{op1,op2}] = res;
    }
    else{
        if(res == mp[{op1,op2}]) return;
        DomNode *d1 = dynamic_cast<IRInstruction*>(res->def[0])->block->domNode;
        DomNode *d2 = dynamic_cast<IRInstruction*>(mp[{op1,op2}]->def[0])->block->domNode;
        if(d2->dominate(d1)) {
            replaceAllUsesOf(res, mp[{op1, op2}]);
        }
    }
}

void Update(UnaryType type, ValueRef* ref, ValueRef* res){
    auto& mp = UnaryExp[type];
    if(mp.find(ref) == mp.end()){
        mp[ref] = res;
    }
    else{
        if(res == mp[ref]) return;
        replaceAllUsesOf(res,mp[ref]);
    }
}

void GVNPass::run() {
    SccpPass* sccpPass = new SccpPass(gu);
    for(auto&[name,func]:gu->func_table){
        if(func->entry == nullptr) continue;
        GVN(func);
        //gu->Emit(std::cerr);
        sccpPass->deadCodeEli(func);
    }
}

void GVNPass::GVN(Function *func) {
    // 目前的GVN只做binary / cond expr, ZEXT, XOR, SLL, SRA, FNEG, itfp,fpti等运算指令
    vis1.clear(); const_int.clear(); const_float.clear(); UnaryExp.clear(); BinaryExp.clear();
    PostOrder(func->entry);

    while(!st1.empty()) { // RPO
        BasicBlock *bb = st1.top(); st1.pop();
        auto & v = bb->local_instr;
        for(auto it = v.begin(); it != v.end();it ++){
            Instruction * i = *it;
//            i->outPut(std::cerr);
            switch(i->instType){
                case BINARY:{
                    auto instr = dynamic_cast<BinaryInstruction*>(i);
                    ValueRef* op1 = Update_Const(instr->src1);
                    ValueRef* op2 = Update_Const(instr->src2);
                    switch(instr->opTy){
                        case ADD:{
                            if(op1->type == IntConst || op1->type == IntVar){
                                Update(BinaryType::add,op1,op2,instr->dst);
                                Update(BinaryType::add,op2,op1,instr->dst);
                            }
                            else{
                                //float不一定满足交换律
                                Update(BinaryType::add,op1,op2,instr->dst);
                            }
                            break;
                        }
                        case MUL:{
                            if(op1->type == IntConst || op1->type == IntVar){
                                Update(BinaryType::mul,op1,op2,instr->dst);
                                Update(BinaryType::mul,op2,op1,instr->dst);
                            }
                            else{
                                //float不一定满足交换律
                                Update(BinaryType::mul,op1,op2,instr->dst);
                            }
                            break;
                        }
                        case SUB:
                            Update(BinaryType::sub,op1,op2,instr->dst);
                            break;
                        case DIV:
                            Update(BinaryType::divide,op1,op2,instr->dst);
                            break;
                        case MOD:
                            Update(BinaryType::mod,op1,op2,instr->dst);
                            break;
                        case AND:
                            Update(BinaryType::andi,op1,op2,instr->dst);
                            break;
                        default: break;
                    }
                    break;
                }
                case CMP:{
                    auto instr = dynamic_cast<CmpInstruction*>(i);
                    ValueRef* op1 = Update_Const(instr->src1);
                    ValueRef* op2 = Update_Const(instr->src2);
                    switch(instr->opTy){
                        case EQ:
                            Update(BinaryType::eq,op1,op2,instr->result);
                            Update(BinaryType::eq,op2,op1,instr->result);
                            break;
                        case NE:
                            Update(BinaryType::ne,op1,op2,instr->result);
                            Update(BinaryType::ne,op2,op1,instr->result);
                            break;
                        case LT:
                            Update(BinaryType::lt,op1,op2,instr->result);
                            break;
                        case LE:
                            Update(BinaryType::le,op1,op2,instr->result);
                            break;
                        case GT:
                            Update(BinaryType::gt,op1,op2,instr->result);
                            break;
                        case GE:
                            Update(BinaryType::ge,op1,op2,instr->result);
                            break;
                    }
                    break;
                }
                case ZEXT:{
                    auto instr = dynamic_cast<ZExtInstruction*>(i);
                    Update(UnaryType::zext,instr->src,instr->dst);
                    break;
                }
                case XOR:{
                    auto instr = dynamic_cast<XorInstruction*>(i);
                    Update(UnaryType::xori,instr->src,instr->dst);
                    break;
                }
                case SLL:{
                    auto instr = dynamic_cast<SllInstruction*>(i);
                    ValueRef* op1 = Update_Const(instr->src1);
                    ValueRef* op2 = Update_Const(instr->bits);
                    Update(BinaryType::sll,op1,op2,instr->dst);
                    break;
                }
                case GEP:{
                    auto gep = dynamic_cast<GEPInstruction*>(i);
                    ValueRef* op1 = Update_Const(gep->src);
                    ValueRef* op2 = Update_Const(gep->index);
                    Update(BinaryType::gep,op1,op2,gep->dst);
                    break;
                }
                case SRA:{
                    auto instr = dynamic_cast<SraInstruction*>(i);
                    ValueRef* op1 = Update_Const(instr->src1);
                    ValueRef* op2 = Update_Const(instr->bits);
                    Update(BinaryType::sra,op1,op2,instr->dst);
                    break;
                }
                case FNEG:{
                    auto instr = dynamic_cast<FNegInstruction*>(i);
                    Update(UnaryType::fneg,instr->src,instr->dst);
                    break;
                }
                case ITFP:{
                    auto instr = dynamic_cast<ITFPInstruction*>(i);
                    Update(UnaryType::itfp,instr->src,instr->dst);
                    break;
                }
                case FPTI:{
                    auto instr = dynamic_cast<FPTIInstruction*>(i);
                    Update(UnaryType::fpti,instr->src,instr->dst);
                    break;
                }
                default: break;
            }
        }
    }
}
