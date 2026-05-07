#pragma once

#include <queue>
#include <stack>

#include "Function.hh"
#include "Instruction.hh"
#include "BasicBlock.hh"

#include "Pass.hh"

class DependencyTreeNode {
public:
    DependencyTreeNode(Instruction *inst): inst_(inst) {}

    int get_self_delay();
    void set_delay(int delay) { if(delay > delay_) delay_ = delay; };

    void set_start_time(int start) { start_ = start; }
    int get_end_time() { return start_ + get_self_delay(); }
    
    int get_priority() { return delay_; }
    
    bool is_leaf_node() { return dependencies_.empty(); }

    std::set<DependencyTreeNode*>& get_dependencys() { return dependencies_; }

    void add_dependency(DependencyTreeNode* node) {
        dependencies_.insert(node);
        node->add_parent(this);
    }

    void delete_dependency(DependencyTreeNode* node) {
        dependencies_.erase(node);
    }

    void clear_parents() { parents_.clear(); }

    bool get_num_of_dependencys() { return dependencies_.size(); }

    void clear_dependency() { dependencies_.clear(); }

    void add_parent(DependencyTreeNode* node) {
        parents_.insert(node);
    }

    void remove_parent(DependencyTreeNode *node) {
        parents_.erase(node);
    } 

    std::set<DependencyTreeNode*>& get_parents() { return parents_; }

    bool is_root() { return parents_.empty(); }

    Instruction *get_inst() { return inst_; }

    

private:
    Instruction* inst_;
    int start_ = -1;
    int delay_ = -1;
    std::set<DependencyTreeNode*> parents_;
    std::set<DependencyTreeNode*> dependencies_;
};

struct DependencyTreeNodeComparator {
    bool operator()(DependencyTreeNode *a, DependencyTreeNode *b) {
        return a->get_priority() < b->get_priority();
    }
};


//& 在mir层次上,指针参数在被使用时只会通过gep指令获取地址后再使用
struct LocUse {
    LocUse(bool is_arg, Value *base) : is_arg_(is_arg), base_(base) {}
    bool is_arg_;
    Value *base_;  //& 只能是alloc变量或者global变量或者参数变量
    std::set<Instruction*> storeinsts;
    std::set<Instruction*> loadinsts;
};


struct PhiAddrNode {
    PhiAddrNode(PhiInst* phi_inst, Value* leaf): phi_(phi_inst), leaf_(leaf) {}
    PhiInst* phi_;
    Value* leaf_;
    std::vector<PhiAddrNode*> childs_;
    std::vector<PhiAddrNode*> parents_;
    std::set<Value*> real_addrs; //& 只能是alloc变量或者global变量或者参数变量
};


class InstructionScheduling: public Pass {
public:
    explicit InstructionScheduling(Module *m, bool print_ir=false) : Pass(m, print_ir) {
        root = new DependencyTreeNode(nullptr);
    }

public:
    void execute() final;
    void schedule(BasicBlock *bb);

    void init_ready_queue();

    void analyse_ptrphis();
    void bottom_up_propagatioin(PhiAddrNode *node);
    std::set<Value*> get_real_addrs(GetElementPtrInst* inst);

    void numbering_insts();

    //& 初始化
    void init(BasicBlock *bb);

    //& 根据phi指令、alloc指令、memset指令的语义和用途,其可在在块的开始处立即执行
    void handle_phi_insts();
    void handle_alloc_insts(); 
    void handle_memset_insts();
    void handle_terminator_inst();

    //& 构建依赖关系图
    void build_dependency_tree() {
        build_usedef_dependency_tree();
        build_loadstore_dependency_tree();
        aggregate_roots();
    }
    void build_usedef_dependency_tree();
    void build_loadstore_dependency_tree();
    void aggregate_roots();

    //& 计算延迟代价(其中依赖关系图是一个DAG,从root深度优先计算到该节点的最大延迟)
    void compute_delays();
    
    const std::string get_name() const override { return name; }

private:
    int cycle;

    DependencyTreeNode *root;
    BasicBlock* cur_bb;
    std::priority_queue<DependencyTreeNode*, std::vector<DependencyTreeNode*>, DependencyTreeNodeComparator> ready; 
    std::vector<Instruction*> active;

    //& 指令编号(记录原本的先后顺序)
    std::map<Instruction*, int> inst2idx;
    
    //& 记录当前块访问的变量地址及变量和地址的映射
    std::map<Value*, LocUse*> cur_array2locuse;
    std::map<Value*, LocUse*> cur_var2locuse;
    std::map<Argument*, LocUse*> cur_arg2locuse;  
    std::vector<DependencyTreeNode*> cur_callnode;                

    std::map<Instruction*, DependencyTreeNode*> inst2node;

    //& phi insts analyse
    std::map<PhiInst*, PhiAddrNode*> phi2phinode;
    std::map<Value*, PhiAddrNode*> leaf2phinode;
    PhiAddrNode* start; 
    Value *cur_loc;

    std::list<Instruction*> scheduled_insts_of_curbb; 
    std::string name = "InstructionScheduling";
};