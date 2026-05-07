#pragma once

#include "Module.hh"
#include "Pass.hh"
#include "Constant.hh"
#include "logging.hh"
#include "LoopInvariant.hh"
#include "LoopExpansion.hh"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <map>
#include <set>

struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

struct PairEqual {
    template <class T1, class T2>
    bool operator () (const std::pair<T1, T2>& lhs, const std::pair<T1, T2>& rhs) const {
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

struct Addr
{
    GetElementPtrInst *base;
    Value *offset;

    Addr(GetElementPtrInst *base_, Value *offset_) : base(base_), offset(offset_) {}
    Addr() : base(), offset() {}

    Addr& operator=(const Addr& other) {
        if (this == &other) {
            return *this;  // 处理自我赋值情况
        }

        base = other.base;
        offset = other.offset;
        return *this;
    }

    bool operator==(const Addr& other) const {
        return (base == other.base) && (offset == other.offset);
    }

    // 定义哈希函数
    struct Hash {
        size_t operator()(const Addr& addr) const {
            // 实现自定义的哈希算法，例如：
            return std::hash<GetElementPtrInst*>()(addr.base) ^ std::hash<Value*>()(addr.offset);
        }
    };

    // 定义相等比较函数
    struct Equal {
        bool operator()(const Addr& lhs, const Addr& rhs) const {
            return lhs.base == rhs.base && lhs.offset == rhs.offset;
        }
    };
};

class AgressiveConstProp : public Pass
{
private:
    Module *module_;
    std::string name = "AgressiveConstProp";

public:
    explicit AgressiveConstProp(Module* module, bool print_ir=false): Pass(module, print_ir){module_ = module;}
    ~AgressiveConstProp(){};
    void execute() final;
    const std::string get_name() const override {return name;}



    bool test();

    //数组常量传播
    // std::map<std::pair<Value*, Value*>, Value*> array_val_map;
    std::set<BasicBlock *> deadBlock;
    //全局变量常量传播
    std::set<Value*> unchangedGlobalVal;
    void collect_unchanged_global_val();


    // 将所有gep指令集合在entry
    bool gep_equal(Value *value1,Value *value2);
    bool gep_exists(Value *value);
    void collect_gep(Function *fun);
    void insert_gep_in_entry_end(Instruction *instruction, BasicBlock *entry);

    // 在整个函数中进行数组常量传播
    std::unordered_map<BasicBlock *, std::unordered_map<Addr, std::unordered_set<StoreOffsetInst *>, Addr::Hash, Addr::Equal>> uselessStoreoffsets;
    void array_const_prop(Function *fun);
    void collect_useless_storeoffset(Function *fun);
    std::unordered_map<Addr, std::unordered_set<StoreOffsetInst *>, Addr::Hash, Addr::Equal> combine_inputs(BasicBlock *bb);

};

