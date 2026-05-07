#include "riscv.h"

// 判断是2的幂次
bool is_exp(int n) {
    return !(n & (n - 1));
}

std::pair<int, int> magic_number(int d);

void algebraic_identity(Program_Asm *prog) {
    int erase_count = 0;
    auto zero_reg = create_reg(0);
    for(auto f : prog->functions) {
        map<VOper, int> &int_val_map = f->int_val_map;

        auto judge_zero = [&int_val_map](VOper& vopr) {
            return vopr.tag == IMM && vopr.value == 0 || int_val_map.count(vopr) && int_val_map.at(vopr) == 0;
        };

        for(auto mb : f->mbs) {
            for(auto I = mb->inst; I; I=I->next) {
                if (I->marked){
                    I = I->next;
                    if(!I)break;
                }
                if (I->tag == VI_BINARY) {
                    auto binary = (VI_Binary *) I;
                    switch (binary->op) {
                        case BINARY_ADD: {
                            // 0 + x
                            if(judge_zero(binary->lhs)) {
                                replace_operand_by_operand(&binary->dst, &binary->rhs, f);
                                I->mark();
                            }
                            // x + 0
                            else if(judge_zero(binary->rhs)) {
                                replace_operand_by_operand(&binary->dst, &binary->lhs, f);
                                I->mark();
                            }
                            // x + x
                            else if(binary->lhs == binary->rhs) {
                                auto lsl = new VI_Binary(BINARY_LSL, binary->dst, binary->lhs, create_imm(1));
                                insert(lsl, I);
                                I->mark();
                            }
                            // x + imm12-constant
                            else if(int_val_map.count(binary->rhs) && can_be_imm12(int_val_map[binary->rhs])) {
                                binary->rhs = create_imm(int_val_map[binary->rhs]);
                            }
                        } break;
                        case BINARY_SUBTRACT: {
                            // x - 0
                            if(judge_zero(binary->rhs)) {
                                replace_operand_by_operand(&binary->dst, &binary->lhs, f);
                                I->mark();
                            }
                            // x - x
                            else if(binary->lhs == binary->rhs) {
                                replace_operand_by_operand(&binary->dst, &zero_reg, f);
                                I->mark();
                            }
                            // x - imm12-constant
                            else if(int_val_map.count(binary->rhs) && can_be_imm12(int_val_map[binary->rhs])) {
                                binary->rhs = create_imm(int_val_map[binary->rhs]);
                            }
                        } break;
                        case BINARY_MULTIPLY: {
                            // 0 * x
                            if(judge_zero(binary->lhs)) {
                                replace_operand_by_operand(&binary->dst, &zero_reg, f);
                                I->mark();
                            }
                            // x * 0
                            else if(judge_zero(binary->rhs)) {
                                replace_operand_by_operand(&binary->dst, &zero_reg, f);
                                I->mark();
                            }
                            // 1 * x
                            else if(int_val_map.count(binary->lhs) && int_val_map[binary->lhs] == 1) {
                                replace_operand_by_operand(&binary->dst, &binary->rhs, f);
                                I->mark();
                            }
                            // x * 1
                            else if(int_val_map.count(binary->rhs) && int_val_map[binary->rhs] == 1) {
                                replace_operand_by_operand(&binary->dst, &binary->lhs, f);
                                I->mark();
                            }

                            if(int_val_map.count(binary->rhs) == 0)
                                break;
                            int const_rhs = int_val_map[binary->rhs];
                            if(const_rhs <= 1)
                                break;
                            // yyy: __builtin_ctz返回输入数二进制表示从最低位开始(右起)的连续的0的个数
                            int log2v = __builtin_ctz(const_rhs);
                            int v0 = 1 << log2v;
                            // x * 2^sh
                            if(const_rhs == v0) {
                                auto shmat = int(log2(const_rhs));
                                auto lsl = new VI_Binary(BINARY_LSL, binary->dst, binary->lhs, create_imm(shmat));
                                insert(lsl, I);
                                I->mark();
                            }
                            // x * (2^sh1 + 2^sh2) = x << sh1 + x << sh2  即类似10000100这样的
                            // yyy: __builtin_popcount返回输入数二进制表1的个数
                            else if(__builtin_popcount(const_rhs - v0) == 1) {
                                int s = __builtin_ctz(const_rhs - v0);
                                auto tmp = create_vreg(f->vreg_num++);
                                auto lsl = new VI_Binary(BINARY_LSL, tmp, binary->lhs, create_imm(s - log2v));
                                auto add = new VI_Binary(BINARY_ADD, binary->dst, tmp, binary->lhs);
                                insert(lsl, I);
                                insert(add, I);
                                if(log2v) {
                                    tmp = create_vreg(f->vreg_num++);
                                    lsl = new VI_Binary(BINARY_LSL, binary->dst, binary->dst, create_imm(log2v));
                                    insert(lsl, I);
                                }
                                I->mark();
                            }
                            // x * (2^sh1 - 2^sh2) = x << sh1 - x << sh2
                            else if(__builtin_popcount(const_rhs + v0) == 1) {
                                int s = __builtin_ctz(const_rhs + v0);
                                auto tmp = create_vreg(f->vreg_num++);
                                auto lsl = new VI_Binary(BINARY_LSL, tmp, binary->lhs, create_imm(s - log2v));
                                auto sub = new VI_Binary(BINARY_SUBTRACT, binary->dst, tmp, binary->lhs);
                                insert(lsl, I);
                                insert(sub, I);
                                if(log2v) {
                                    tmp = create_vreg(f->vreg_num++);
                                    lsl = new VI_Binary(BINARY_LSL, binary->dst, binary->dst, create_imm(log2v));
                                    insert(lsl, I);
                                }
                                I->mark();
                            }
                        } break;
                        case BINARY_MOD:
                        case BINARY_DIVIDE: {
                            if(binary->op==BINARY_MOD&&binary->is_dw)continue;
                            if(int_val_map.count(binary->rhs) == 0)continue;
                            int const_rhs = int_val_map[binary->rhs];
                            if(const_rhs <= 1)continue;
                            int log2v = __builtin_ctz(const_rhs);
                            int v0 = 1 << log2v;

                            // x / 2^sh
                            if(is_exp(const_rhs)) {
                                
                                auto lsl_dst = create_vreg(f->vreg_num++);//lsl_lhs
                                auto lsr_dst = create_vreg(f->vreg_num++);// r0
                                auto add_dst = create_vreg(f->vreg_num++);// r1
                                
                                auto lsl = new VI_Binary(BINARY_LSL, lsl_dst, binary->lhs, create_imm(1));
                                auto lsr = new VI_Binary(BINARY_LSR, lsr_dst, lsl_dst, create_imm(64 - log2v));
                                auto add = new VI_Binary(BINARY_ADD, add_dst, binary->lhs, lsr_dst);
                                add->is_dw = true;
                                insert(lsl, I);
                                insert(lsr, I);
                                insert(add, I);
                                if(binary->op==BINARY_MOD){
                                    auto r2 = create_vreg(f->vreg_num++);
                                    auto const_opr = create_imm(-const_rhs);
                                    if(!can_be_imm12(-const_rhs)) {
                                        const_opr = create_vreg(f->vreg_num++);
                                        generate_li(const_opr, -const_rhs, mb, I);
                                    }
                                    auto and_instr = new VI_Binary(BINARY_BITWISE_AND, r2, add_dst, const_opr);
                                    auto sub = new VI_Binary(BINARY_SUBTRACT, binary->dst, binary->lhs, r2);
                                    insert(and_instr, I);
                                    insert(sub, I);
                                    I->mark();
                                }else{
                                    auto asr = new VI_Binary(BINARY_ASR, binary->dst, add_dst, create_imm(log2v));
                                    insert(asr, I);
                                    I->mark();
                                }
                                
                            }
                            else { // 利用magic number快速计算除法
                                auto mag = magic_number(const_rhs);
                                auto M = create_imm(mag.first);
                                auto s = create_imm(mag.second);
                                if(!can_be_imm12(mag.first)) {
                                    M = create_vreg(f->vreg_num++);
                                    generate_li(M, mag.first, mb, I);
                                }
                                auto r0 = create_vreg(f->vreg_num++);
                                auto r1 = create_vreg(f->vreg_num++);
                                auto r2 = create_vreg(f->vreg_num++);
                                auto r3 = create_vreg(f->vreg_num++);
                                auto r4 = create_vreg(f->vreg_num++);
                                auto r5 = create_vreg(f->vreg_num++);
                                auto mul1 = new VI_Binary(BINARY_MULTIPLY, r0, binary->lhs, M);
                                mul1->is_dw = true;
                                insert(mul1, I);

                                if(mag.first & (1LL << 31)) {
                                    s = create_imm(mag.second);
                                    auto srli1 = new VI_Binary(BINARY_LSR, r5, r0, create_imm(32));
                                    auto add2 = new VI_Binary(BINARY_ADD, r0, r5, binary->lhs);
                                    auto srli2 = new VI_Binary(BINARY_LSR, r1, r0, create_imm(31));
                                    auto srai = new VI_Binary(BINARY_ASR, r2, r0, s);
                                    insert(srli1, I);
                                    insert(add2, I);
                                    insert(srli2, I);
                                    insert(srai, I);
                                }
                                else {
                                    s = create_imm(mag.second + 32);
                                    auto srli = new VI_Binary(BINARY_LSR, r1, r0, create_imm(63));
                                    auto srai = new VI_Binary(BINARY_ASR, r2, r0, s);
                                    srli->is_dw = true;
                                    srai->is_dw = true;
                                    insert(srli, I);
                                    insert(srai, I);
                                }
                                
                                if(binary->op==BINARY_MOD){
                                    auto add = new VI_Binary(BINARY_ADD, r3, r1, r2);
                                    auto mul2 = new VI_Binary(BINARY_MULTIPLY, r4, r3, binary->rhs);
                                    auto sub = new VI_Binary(BINARY_SUBTRACT, binary->dst, binary->lhs, r4);
                                    add->is_dw = true;
                                    
                                    insert(add, I);
                                    insert(mul2, I);
                                    insert(sub, I);
                                    I->mark();
                                }else{
                                    auto add = new VI_Binary(BINARY_ADD, binary->dst, r1, r2);
                                    add->is_dw = true;
                                    insert(add, I);
                                    I->mark();
                                }
                            }
                        } break;
                        default: break;
                    }
                    if(I->marked) {
                        erase_count ++;
                    }
                }
            }
            mb->erase_marked_values();
        }
    }
    cerr<<"[opt]: algebraic identity ereased "<<erase_count<<" instructions\n";
}

std::pair<int, int> magic_number(int d)
{
    int p;
    unsigned ad, anc,delta, q1,r1,q2,r2,t;
    const unsigned two31 = 0x80000000;
    ad = abs(d);
    t = two31 + ((unsigned) d >> 31);
    anc = t -1 -t%ad;
    p=31;
    q1 = two31 /anc;
    r1 = two31 -q1 * anc;
    q2 = two31 /ad;
    r2 = two31 -q2 * ad;
    do
    {
        p =p +1;
        q1 = 2*q1;
        r1 = 2 * r1;
        if(r1>=anc)
        {
            q1 = q1 + 1;
            r1 = r1 - anc;
        }
        q2 = 2* q2;
        r2 = 2 * r2;
        if(r2>=ad)
        {
            q2 = q2 + 1;
            r2 = r2 - ad;
        }
        delta = ad - r2;
    } while (q1 < delta || (q1 == delta && q1 == 0));

    int M = q2 + 1;
    if(d<0) M = -M;
    int s = p - 32;
    return {M, s};
}