#ifndef MACHINE_H
#define MACHINE_H
#include<string>
#include<vector>
#include<list>
#include<stack>
#include<queue>
#include"../include/Instruction.h"
#include"register.h"
#include"../inst_process/machine_instruction.h"
#include"../../llvm/optimize/analysis/dominator_tree.h"

class RiscV64Printer;

// 注：参考原框架的 MachineBlock, RiscV64Block, MachineCFG, MachineFunction, RiscV64Function, MachineUnit, RiscV64Unit

class MachineUnit;
class MachineFunction;
class MachineCFG;

class MachineBlock {
private:
    int label_id;
private:
    MachineFunction *parent;

public:
    std::list<MachineBaseInstruction *> instructions;
    int loop_depth = 0;
    virtual std::list<MachineBaseInstruction *>::iterator getInsertBeforeBrIt() = 0;
    decltype(instructions) &GetInsList() { return instructions; }
    void clear() { instructions.clear(); }
    auto erase(decltype(instructions.begin()) it) { return instructions.erase(it); }
    auto insert(decltype(instructions.begin()) it, MachineBaseInstruction *ins) { return instructions.insert(it, ins); }
    auto getParent() { return parent; }
    void setParent(MachineFunction *parent) { this->parent = parent; }
    auto getLabelId() { return label_id; }
    auto ReverseBegin() { return instructions.rbegin(); }
    auto ReverseEnd() { return instructions.rend(); }
    auto begin() { return instructions.begin(); }
    auto end() { return instructions.end(); }
    auto size() { return instructions.size(); }
    void push_back(MachineBaseInstruction *ins) { instructions.push_back(ins); }
    void push_front(MachineBaseInstruction *ins) { instructions.push_front(ins); }
    void pop_back() { instructions.pop_back(); }
    void pop_front() { instructions.pop_front(); }
    int getBlockInNumber() {
        for (auto ins : instructions) {
            return ins->getNumber() - 1;
        }
        ERROR("Empty block");
        // return (*(instructions.begin()))->getNumber();
    }
    int getBlockOutNumber() {
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
            auto ins = *it;
            return ins->getNumber();
        }
        ERROR("Empty block");
        // return (*(instructions.rbegin()))->getNumber();
    }
    MachineBlock(int id) : label_id(id) {}

    void display();
};
class MachineFunction {
private:
    // 函数名
    std::string func_name;
    // 所属的MachineUnit
    MachineUnit *parent;
    // 函数形参列表
    std::vector<Register> parameters;
    // 现存的最大块编号
    int max_exist_label;
    //可以修改成RiscvInstruction类型
    std::vector<MachineBaseInstruction *> allocalist;

protected:
    // 栈大小
    int stack_sz;
    // 传实参所用到的栈空间大小
    int para_sz;

	bool is_para_instack;

public:
    // 更新现存的最大块编号
    void UpdateMaxLabel(int labelid) { max_exist_label = max_exist_label > labelid ? max_exist_label : labelid; }
    // 获取形参列表
    const decltype(parameters) &GetParameters() { return parameters; }
    // 增加形参列表
    void AddParameter(Register reg) { parameters.push_back(reg); }
    // 设置栈大小
    void SetStackSize(int sz) { stack_sz = sz; }
    // 更新传实参所用到的栈空间大小
    void UpdateParaSize(int parasz) {
        if (parasz > para_sz) {
            para_sz = parasz;
        }
    }
    // 获取传实参所用到的栈空间大小
    int GetParaSize() { return para_sz; }
    // 增加栈大小
    virtual void AddStackSize(int sz) { stack_sz += sz; }
    // 获取栈空间大小（自动对齐）
    int GetStackSize() { return ((stack_sz + 15) / 16) * 16; }
    // 获取栈底偏移
    int GetStackOffset() { return stack_sz; }
    // 获取MachineUnit
    MachineUnit *getParentMachineUnit() { return parent; }
    // 获取函数名
    std::string getFunctionName() { return func_name; }

    // 设置MachineUnit
    void SetParent(MachineUnit *parent) { this->parent = parent; }

    // 函数包含的所有块
    std::vector<MachineBlock *> blocks{};

public:
    // 获取新的虚拟寄存器
    Register GetNewRegister(int regtype, int regwidth, bool save_across_call=false);
    // 获取新的虚拟寄存器，等同于GetNewRegister(type.data_type, type.data_length)
    Register GetNewReg(MachineDataType type);

protected:
    // 获取新的块
    MachineBlock *InitNewBlock();
    bool has_inpara_instack;//新增
    MachineCFG *mcfg;

public:
    MachineFunction(std::string name)
        : func_name(name), stack_sz(0), para_sz(0), max_exist_label(0),has_inpara_instack(false){}
    //新增：
    bool HasInParaInStack() { return has_inpara_instack; }
    void SetHasInParaInStack(bool has) { has_inpara_instack = has; }
    MachineCFG *getMachineCFG() { return mcfg; }
    // 设置CFG
    void SetMachineCFG(MachineCFG *mcfg) { this->mcfg = mcfg; }
};

class MachineDominatorTree {
public:
    MachineCFG *C;
    std::vector<std::vector<MachineBlock *>> dom_tree{};
    std::map<int, int> sdom_map{};

    MachineDominatorTree(){}
    bool IsDominate(int id1, int id2);
};


class MachineCFG {

public:
    class MachineCFGNode {
    public:
        MachineBlock *Mblock;
    };

    std::map<int, MachineCFGNode *> block_map{};
    std::vector<std::vector<MachineCFGNode *>> G{}, invG{};
    int max_label;

    MachineDominatorTree* DomTree;
    void SetMachineDomTree(DominatorTree* domtree);

    // 用于 DFS 的状态变量
    std::map<int, int> dfs_visited;
    std::stack<int> dfs_stk;
 
    // 用于 BFS 的状态变量
    std::map<int, int> bfs_visited;
    std::queue<int> bfs_que;
 
    // 用于 SeqScan 的状态变量
    decltype(block_map.begin()) seqscan_current;
 
    // 用于 Reverse 的状态变量
    std::vector<MachineCFGNode*> reverse_cache;
    decltype(reverse_cache.rbegin()) reverse_current_pos;


    MachineCFGNode *ret_block;//新增
    MachineCFG() : max_label(0){ DomTree = new MachineDominatorTree; };
    void AssignEmptyNode(int id, MachineBlock *Mblk);

    // Just modify CFG edge, no change on branch instructions
    void MakeEdge(int edg_begin, int edg_end);
    void RemoveEdge(int edg_begin, int edg_end);

    MachineCFGNode *GetNodeByBlockId(int id) { return block_map[id]; }
    std::vector<MachineCFGNode *> GetSuccessorsByBlockId(int id) { return G[id]; }
    std::vector<MachineCFGNode *> GetPredecessorsByBlockId(int id) { return invG[id]; }

    // DFS 遍历
    void dfs_open() {
        dfs_visited.clear();
        while (!dfs_stk.empty())
            dfs_stk.pop();
        dfs_stk.push(0);
    }
    MachineCFGNode* dfs_next() {
        auto return_idx = dfs_stk.top();
        dfs_visited[return_idx] = 1;
        dfs_stk.pop();
        for (auto succ : G[return_idx]) {
            if (dfs_visited[succ->Mblock->getLabelId()] == 0) {
                dfs_visited[succ->Mblock->getLabelId()] = 1;
                dfs_stk.push(succ->Mblock->getLabelId());
            }
        }
        return block_map[return_idx];
    }
    bool dfs_hasNext() { return !dfs_stk.empty(); }
    void dfs_rewind() { dfs_open(); }
    void dfs_close() {
        dfs_visited.clear();
        while (!dfs_stk.empty())
            dfs_stk.pop();
    }

    // BFS 遍历
    void bfs_open() {
        bfs_visited.clear();
        while (!bfs_que.empty())
            bfs_que.pop();
        bfs_que.push(0);
    }
    MachineCFGNode* bfs_next() {
        auto return_idx = bfs_que.front();
        bfs_visited[return_idx] = 1;
        bfs_que.pop();
        for (auto succ : G[return_idx]) {
            if (bfs_visited[succ->Mblock->getLabelId()] == 0) {
                bfs_visited[succ->Mblock->getLabelId()] = 1;
                bfs_que.push(succ->Mblock->getLabelId());
            }
        }
        return block_map[return_idx];
    }
    bool bfs_hasNext() { return !bfs_que.empty(); }
    void bfs_rewind() { bfs_open(); }
    void bfs_close() {
        bfs_visited.clear();
        while (!bfs_que.empty())
            bfs_que.pop();
    }

    // SeqScan 遍历
    void seqscan_open() { seqscan_current = block_map.begin(); }
    MachineCFGNode* seqscan_next() { return (seqscan_current++)->second; }
    bool seqscan_hasNext() { return seqscan_current != block_map.end(); }
    void seqscan_rewind() {
        seqscan_close();
        seqscan_open();
    }

    void seqscan_close() { seqscan_current = block_map.end(); }
    // reverse 遍历
    void reverse_open() {
        bfs_open();
        reverse_cache.clear();
        while (bfs_hasNext()) {
            reverse_cache.push_back(bfs_next());
        }
        reverse_current_pos = reverse_cache.rbegin();
    }
    MachineCFGNode* reverse_next() { return *(reverse_current_pos++); }
    bool reverse_hasNext() { return reverse_current_pos != reverse_cache.rend(); }
    void reverse_rewind() {
        reverse_close();
        reverse_open();
    }
    void reverse_close() {
        bfs_close();
        reverse_cache.clear();
        reverse_current_pos = reverse_cache.rend();
    }

public:
    void display() {
        std::cerr << "\n[MachineCFG] 控制流图结构:" << std::endl;
        for (auto &[id, node] : block_map) {
            std::cerr << "基本块 B" << id << " 的后继基本块:";
            for (auto succ : G[id]) {
                std::cerr << " B" << succ->Mblock->getLabelId();
            }
            std::cerr << std::endl;
            node->Mblock->display();
        }
        std::cerr << std::endl;
    }


};

class RiscV64Block : public MachineBlock {
public:
    RiscV64Block(int id) : MachineBlock(id) {}
    std::list<MachineBaseInstruction *>::iterator getInsertBeforeBrIt();
};
    
class RiscV64Function : public MachineFunction {
public:
    RiscV64Function(std::string name) : MachineFunction(name) {}

private:
    std::vector<RiscV64Instruction *> allocalist;
public:
    void AddAllocaInst(RiscV64Instruction *ins) { allocalist.push_back(ins); }
    void AddParameterSize(int sz) {
        for (auto ins : allocalist) {
            ins->setImm(ins->getImm() + sz);
        }
    }
};

#endif

