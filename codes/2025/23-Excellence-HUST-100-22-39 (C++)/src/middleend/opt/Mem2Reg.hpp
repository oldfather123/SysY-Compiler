#ifndef __MEM2REG_H__
#define __MEM2REG_H__

#include <queue>
#include <unordered_set>
#include <unordered_map>
#include "ir.hpp"
#include "opt.hpp"

struct AllocaInfo;
class LargeBlockInfo;
struct RenamePassData;

class Mem2Reg: public Optimization {
private:
    map<pair<unsigned, unsigned>, PhiInst*> new_phi_nodes_;  // <block_id, alloca_id> → phi
    vector<AllocaInst*> promotable_allocas;
    unsigned num_phi_insert_ = 0;
    map<PhiInst*, unsigned> phi_to_alloca_map_;
    unordered_map<AllocaInst*, int> allocaLookup;
    unordered_set<BasicBlock*> visited;

    void insert_phi(Function* func);
    bool is_promotable(AllocaInst* alloca);
    void removeLifetimeIntrinsicUsers(AllocaInst* AI);
    bool rewriteSingleStoreAlloca(AllocaInst* AI, AllocaInfo& Info, LargeBlockInfo& LBI);
    bool promoteSingleBlockAlloca(AllocaInst* AI, AllocaInfo& Info, LargeBlockInfo& LBI);
    bool QueuePhiNode(BasicBlock* BB, unsigned AllocaNum, unsigned& Version);
    void RenamePass(BasicBlock* bb, BasicBlock* pred, vector<Value*>& incomingVals,
                    vector<RenamePassData>& workList, unordered_set<Instruction*>& delInstr);
    void compute_live_in_blocks(AllocaInst* AI, AllocaInfo& Info, set<BasicBlock*>& def_blocks, set<BasicBlock*>& live_in_blocks);
    void RemoveFromAllocasList(unsigned& AllocaIdx) {
        promotable_allocas[AllocaIdx] = promotable_allocas.back();
        promotable_allocas.pop_back();
        --AllocaIdx;
    }
public:
    explicit Mem2Reg(Module* m): Optimization(m) {}
    void execute() override;
};

//用于收集信息
struct AllocaInfo {
    vector<BasicBlock*> DefBlocks;
    vector<BasicBlock*> UseBlocks;
    StoreInst* OnlyStore = nullptr;
    BasicBlock* OnlyBlock = nullptr;
    bool UsedInOneBlock = true;
  
    void analyze(AllocaInst* AI) {
        clear();
        for(auto& use: AI->use_list) {
            auto inst = dynamic_cast<Instruction*>(use.val);
            if(!inst) continue;
            auto bb = inst->parent;
            //store指令是定义
            if(auto store = dynamic_cast<StoreInst*>(inst)) {
                DefBlocks.push_back(bb);
                //cout << bb->name << endl;
                OnlyStore = store;
            } else { //load指令是使用
                UseBlocks.push_back(bb);
            }
            // 是否都在同一个块中？
            if(UsedInOneBlock){
                if(!OnlyBlock)
                    OnlyBlock = bb;
                else if(OnlyBlock != bb)
                    UsedInOneBlock = false;
            }
        }
    }
    void clear() {
        DefBlocks.clear();
        UseBlocks.clear();
        OnlyStore = nullptr;
        OnlyBlock = nullptr;
        UsedInOneBlock = true;
    }
};

class LargeBlockInfo {
public:
    // 判断是否是对 alloca 的 load/store 指令
    static bool is_interesting(Instruction* inst) {
        if(inst->is_load()) {
            auto ptr = inst->get_op(0);
            return dynamic_cast<AllocaInst*>(ptr) != nullptr;
        }
        if(inst->is_store()) {
            auto ptr = inst->get_op(1);
            return dynamic_cast<AllocaInst*>(ptr) != nullptr;
        }
        return false;
    }
    // 获取某条指令在 basic block 中的顺序编号
    unsigned get_instruction_index(Instruction* inst) {
        if(inst_index_map_.count(inst))
            return inst_index_map_.at(inst);

        BasicBlock* bb = inst->parent;
        unsigned idx = 0;
        for(auto cur_inst: bb->inst_list) {
            if(is_interesting(cur_inst)) {
                inst_index_map_[cur_inst] = idx++;
            }
        }
        return inst_index_map_.at(inst);
    }
    // 删除某条已记录指令
    void delete_value(Instruction* inst) { inst_index_map_.erase(inst); }
    // 清空所有记录
    void clear() { inst_index_map_.clear(); }

private:
    // 记录某条指令在所属 BasicBlock 中的顺序编号
    unordered_map<Instruction*, unsigned> inst_index_map_;
};

class IDFCalculator {
public:
    // 构造函数传入 dominator tree 根（可选）
    IDFCalculator() = default;
    // 设置定义块集合（store发生的地方）
    void set_defining_blocks(const set<BasicBlock*>& defs) { def_blocks_ = defs; }
    // 设置活跃入口块集合（变量活跃使用）
    void set_live_in_blocks(const set<BasicBlock*>& live_ins) { live_in_blocks_ = live_ins; }

    void calculate(vector<BasicBlock*>& phi_blocks) {
        queue<BasicBlock*> worklist;
        unordered_set<BasicBlock*> has_phi;        // 记录哪些块已插入phi
        unordered_set<BasicBlock*> defsites(def_blocks_.begin(), def_blocks_.end());
    
        for(auto bb: def_blocks_) {
            worklist.push(bb);
        }    
        while (!worklist.empty()) {
            BasicBlock* bb = worklist.front();
            worklist.pop();
            for(BasicBlock* df: bb->dom_frontier) {
                // 只在活跃块里插phi
                if(!live_in_blocks_.count(df)) {
                    continue;
                }
                if(!has_phi.count(df)) {
                    phi_blocks.push_back(df);
                    has_phi.insert(df);
                    // phi相当于新的defsite
                    if(!defsites.count(df)) {
                        defsites.insert(df);
                        worklist.push(df); // 递归处理DF
                    }
                }
            }
        }
    }
    
private:
    set<BasicBlock*> def_blocks_;
    set<BasicBlock*> live_in_blocks_;
};


struct RenamePassData {
    BasicBlock* bb;
    BasicBlock* pred;
    vector<Value*> vals;

    RenamePassData(BasicBlock* bb_, BasicBlock* pred_, vector<Value*> val) :bb(bb_), pred(pred_), vals((val)) {} // 删除了move
};

class PostOrderDFS {
public:
    unordered_map<BasicBlock*, int> post_order;
    unordered_set<BasicBlock*> visited;
    int counter;

    void run(BasicBlock* entry) {
        counter = 0;
        dfs(entry);
    }

    vector<BasicBlock*> get_order() {
        vector<BasicBlock*> res(post_order.size(), nullptr);
        for(auto it: post_order) {
            res[it.second] = it.first;
        }
        return res;
    }

private:
    void dfs(BasicBlock* bb) {
        visited.insert(bb);
        for(auto succ: bb->succ_bbs) {
            if(!visited.count(succ)) {
                dfs(succ);
            }
        }
        post_order[bb] = counter++;
    }
};

#endif // __MEM2REG_H__