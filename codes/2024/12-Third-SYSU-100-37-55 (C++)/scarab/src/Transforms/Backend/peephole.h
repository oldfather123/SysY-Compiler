#include "riscv.h"

struct BinaryMOperands {
    VOper lhs, rhs;
    Binary_Op_Type op;

    BinaryMOperands() {}
    BinaryMOperands(VOper _lhs, VOper _rhs, Binary_Op_Type _op): lhs(_lhs), rhs(_rhs), op(_op) {}

    bool operator==(const BinaryMOperands &bm) const {
        if(lhs == bm.lhs && rhs == bm.rhs && op == bm.op)
            return true;
        else
            return false;
    }

    bool operator<(const BinaryMOperands &bm) const {
        if(op != bm.op)
            return op < bm.op;
        else if(!(lhs == bm.lhs))
            return lhs < bm.lhs;
        else
            return rhs < bm.rhs;
    }
};

// yyy: 或许可以利用value_map来对应其地址和vreg,  reg考虑sp

struct LoadMOperands{
    VOper base,offset;
    LoadMOperands(){}
    LoadMOperands(VOper base, VOper offset): base(base), offset(offset){}
    bool operator==(const LoadMOperands &lm) const{
        if(base.tag==ADR_GLOBAL){
            return lm.base.tag==ADR_GLOBAL&&strcmp(base.adr,lm.base.adr)==0;
        }
        if(base.tag==VREG||base.tag==VFREG){// yyy: 如果base是一个寄存器，那么可能被改过，但是如果是虚拟寄存器，由于SSA应该不会变
            return base.value==lm.base.value&&offset.value==lm.offset.value;
        }
        if(base.tag==IMM){
            return base.value==lm.base.value;
        }
        return false;
    }
    bool operator<(const LoadMOperands &lm)const{
        if(!(lm.base==base)){
            return base<lm.base;
        }
        return offset.value<lm.offset.value;
    }
};

struct StoreMOperands{
    VOper base,offset;
    StoreMOperands(){}
    StoreMOperands(VOper base, VOper offset): base(base), offset(offset){}
    // yyy: 考虑offset和base，其中ADR_GLOBAL要用op.adr来比较，是const char*，此时没有offset
    bool operator==(const StoreMOperands &sm) const{
        if(base.tag==VREG||base.tag==VFREG){// yyy: 如果base是一个寄存器，那么可能被改过，但是如果是虚拟寄存器，由于SSA应该不会变
            return base.value==sm.base.value&&offset.value==sm.offset.value;
        }
        return false;
    }
    bool operator<(const StoreMOperands &sm) const{
        if(base.tag != sm.base.tag)return base.tag < sm.base.tag;
        if(base.value != sm.base.value) return base.value < sm.base.value;
        return offset.value < sm.offset.value;
    }
};

static map<VOper, VI_Load *> load_map;
static map<BinaryMOperands, VI_Binary *> binary_map;

static map<LoadMOperands, VI_Load *> my_load_map;

static map<StoreMOperands, VI_Store *> store_map;

void clear_map() {
    load_map.clear();
    binary_map.clear();
    my_load_map.clear();
    store_map.clear();
}

void peephole(Program_Asm *prog) {
    int cnt = 0;
    cerr << "begin peephole:\n";
    for(auto f : prog->functions) {
        
        for(auto mb : f->mbs) {
            VI *next_instr = mb->inst;
            // yyy: 维护一个load_map和binary_map用于load指令和二元操作
            clear_map();
            for(auto I = mb->inst; I; I=next_instr) {
                next_instr = I->next;
                switch (I->tag)// yyyg: 计划新增store后load，常数折叠
                {
                case VI_STORE:{
                    auto store = (VI_Store * )I;
                    if(store->base.tag==VREG||store->base.tag==VFREG){
                        StoreMOperands sm{store->base,store->offset};
                        store_map[sm]=store;
                    }
                } break;
                case VI_LOAD: {
                    auto load = (VI_Load *)I;
                    // yyy: 如果是VREG和VSREG对应内存的地址，是否有可能被store改变过
                    if(load->base.tag==IMM||load->base.tag==ADR_GLOBAL||load->base.tag==VREG||load->base.tag==VFREG){
                        LoadMOperands lm{load->base,load->offset};
                        if(my_load_map.count(lm)){
                            auto former_load = my_load_map[lm];
                            replace_operand_by_operand(&load->reg, &former_load->reg, f);
                            I->mark();
                            cnt ++;
                        }
                        else{
                            my_load_map[lm] = load;
                        }
                    }
                } break;
                
                case VI_BINARY: {
                    auto binary = (VI_Binary *)I;
                    BinaryMOperands bm{binary->lhs, binary->rhs, binary->op};
                    if(binary_map.count(bm)) {
                        auto former_binary = binary_map[bm];
                        replace_operand_by_operand(&binary->dst, &former_binary->dst, f);
                        I->mark();
                        cnt ++;
                    }
                    else {
                        binary_map[bm] = binary;
                    }
                } break;
                default:
                    break;
                }
            }
            mb->erase_marked_values();
        }
    }
    cerr << "[opt]: peephole ereased " << cnt << " instructions\n";
}
