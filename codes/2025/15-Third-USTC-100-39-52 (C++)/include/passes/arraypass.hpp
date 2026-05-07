#pragma once


#include "PassManager.hpp"
#include "Module.hpp"
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "Dominators.hpp"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <vector>

// namespace array {
//     using has_store = std::unordered_map<Value*, std::unordered_set<BasicBlock*>>;

class ArrayPass : public Pass {
    //using has_store = std::unordered_map<Value*, std::unordered_set<BasicBlock*>>;

    public:
        explicit ArrayPass(Module *m);
        using has_store = std::unordered_map<Value*, std::unordered_set<BasicBlock*>>;
        struct GEPKey {
            Value* base;                  // 基地址
            std::vector<Value*> indices;  // 所有索引
            bool operator==(const GEPKey& other) const {
                if (base != other.base) return false;
                if (indices.size() != other.indices.size()) return false;
                for (size_t i = 0; i < indices.size(); ++i) {
                    if (indices[i] != other.indices[i]) return false;
                }
                return true;
            }
        };
        struct GEPKeyHash {
            std::size_t operator()(const GEPKey& k) const {
                std::size_t h = std::hash<Value*>{}(k.base);
                for (auto idx : k.indices) {
                    h ^= std::hash<Value*>{}(idx) + 0x9e3779b9 + (h << 6) + (h >> 2); // boost hash_combine
                }
                return h;
            }
        };
        struct LoadKey {
            Value* ptr;  // load指令的地址操作数

            bool operator==(const LoadKey &other) const {
                return ptr == other.ptr;
            }
        };
        struct LoadKeyHash {
            std::size_t operator()(const LoadKey &k) const {
                return std::hash<Value*>{}(k.ptr);
            }
        };
        void run() override;
        void run2();
        bool has_intervening_store_cross_blocks(Instruction* def, Instruction* use, Dominators& dom, has_store& store_blocks);
    private:
        //bool has_intervening_store_cross_blocks(Instruction* def, Instruction* use, Dominators& dom, std::unordered_map<Value*, std::unordered_set<BasicBlock*>>& store_blocks);
        //bool has_intervening_store_cross_blocks(Instruction* def, Instruction* use,Dominators& dom, Module& m);
        bool dom_dominates(Instruction *def_instr, Instruction *use_instr, Dominators &dominators);
        Module *m_;
        std::unique_ptr<Dominators> dominators_;

};