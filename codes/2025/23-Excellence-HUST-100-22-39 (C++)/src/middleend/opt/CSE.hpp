#ifndef __CSE_H__
#define __CSE_H__

#include <unordered_map>
#include "opt.hpp"

using std::vector;
using std::unordered_map;

struct ExpKey {
    IRInstID iid;
    vector<Value*> operands;

    ExpKey(IRInstID iid, vector<Value*> operands): iid(iid), operands(operands) {}

    bool operator==(const ExpKey& other) const {
        if(iid != other.iid) return false;
        if(operands.size() != other.operands.size()) return false;
        auto is_commutative = [](IRInstID iid) {
            return iid == IRInstID::Add || iid == IRInstID::Mul ||
                   iid == IRInstID::FAdd || iid == IRInstID::FMul;
        };
        auto compare_values = [](Value* a, Value* b) {
            if(a == b) {
                return true;
            }
            if(auto const_a = dynamic_cast<ConstInt*>(a)) {
                if(auto const_b = dynamic_cast<ConstInt*>(b)) {
                    return const_a->val == const_b->val;
                }
            } else if(auto const_a = dynamic_cast<ConstFloat*>(a)) {
                if(auto const_b = dynamic_cast<ConstFloat*>(b)) {
                    return const_a->val == const_b->val;
                }
            }
            return false;
        };
        if(is_commutative(iid) && operands.size() == 2) {
            return (compare_values(operands[0], other.operands[0]) && compare_values(operands[1], other.operands[1])) ||
                   (compare_values(operands[0], other.operands[1]) && compare_values(operands[1], other.operands[0]));
        }
        for(size_t i = 0; i < operands.size(); ++i) {
            if(!compare_values(operands[i], other.operands[i])) {
                return false;
            }
        }
        return true;
    }
};

struct ExpInfo {
    Instruction* inst; // 指向对应的指令
    BasicBlock* bb; // 指令所在的基本块
    ExpKey key; // 该表达式的键

    ExpInfo(Instruction* inst, BasicBlock* bb, ExpKey key): inst(inst), bb(bb), key(key) {}
};

inline size_t hash_value(Value* val) {
    if(auto const_int = dynamic_cast<ConstInt*>(val)) {
        return std::hash<int>()(const_int->val);
    } else if(auto const_float = dynamic_cast<ConstFloat*>(val)) {
        return std::hash<float>()(const_float->val);
    } else {
        return std::hash<Value*>()(val);
    }
}

namespace std {
    template<>
    struct hash<ExpKey> {
        size_t operator()(const ExpKey& key) const {
            size_t h = std::hash<int>()(key.iid);

            auto is_commutative = [](IRInstID op) {
                return op == IRInstID::Add || op == IRInstID::Mul ||
                       op == IRInstID::FAdd || op == IRInstID::FMul;
            };

            if(is_commutative(key.iid) && key.operands.size() == 2) {
                // 无序哈希组合
                size_t h1 = hash_value(key.operands[0]);
                size_t h2 = hash_value(key.operands[1]);
                size_t min_h = std::min(h1, h2);
                size_t max_h = std::max(h1, h2);
                h ^= (min_h ^ max_h) + 0x9e3779b9 + (h << 6) + (h >> 2);
            } else {
                for(auto* v : key.operands) {
                    h ^= hash_value(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
                }
            }
            return h;
        }
    };
}

class CSE: public Optimization {
public:
    explicit CSE(Module* m): Optimization(m) {}
    void execute() override;

private:
    // 可以进行 CSE 的指令类型
    bool is_available_for_cse(Instruction* inst);

    // 基本块内进行公共子表达式删除
    void run_cse_in_block(BasicBlock* bb);

    // TODO: 经粗略统计，全局 CSE 只能删除 2% 左右的指令，故暂时不实现
    // 函数内进行公共子表达式删除，需要进行数据流分析：可用表达式分析
    void run_global_cse(Function* func);
};

#endif // __CSE_H__