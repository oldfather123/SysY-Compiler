#include <set>
#include <stack>
#include <algorithm>
#include "Loop.hpp"

std::string IVExp::print() const {
    std::string result = "IVExp:\n";
//     result += "    inst: " + inst->print() + "\n";
//     result += "    base: " + base->print() + "\n";
//     result += "    k: " + std::to_string(k);
//     result += ",  b: " + std::to_string(b) + "\n";
//     result += "    suc_exp.size(): " + std::to_string(suc_exp.size());
    return result;
}

bool Loop::is_always_executed(BasicBlock* target) const {
    if(!header || !target || !latch) {
        return false;
    }
    if(header == target || latch == target) { // 如果目标是循环头或循环出口，则总是执行
        return true;
    }
    stack<BasicBlock*> worklist;
    set<BasicBlock*> visited;
    worklist.push(header); // 从循环头开始遍历
    visited.insert(header);
    while(!worklist.empty()) {
        BasicBlock* cur = worklist.top();
        worklist.pop();
        for(auto succ: cur->succ_bbs) {
            if(visited.count(succ)) { // 如果已经访问过该基本块，则跳过
                continue;
            }
            if(std::find(body.begin(), body.end(), succ) == body.end()) { // 如果后继基本块不在循环体内，则跳过
                continue;
            }
            if(succ == target) { // 如果后继基本块是目标基本块，则跳过
                continue;
            }
            if(succ == latch) {
                return false;
            }
            visited.insert(succ); // 标记为已访问
            worklist.push(succ); // 将后继基本块加入工作列表
        }
    }
    return true;
}

bool Loop::is_invariant(Value* val) {
    if(dynamic_cast<Const*>(val)) {
        // cerr << "Const Value: " << val->print() << "\n";
        return true;
    }
    if(auto inst = dynamic_cast<Instruction*>(val)) {
        if(inst->is_ret() || inst->is_br() // 返回指令是返回值的依赖逻辑，分支指令是控制流的依赖
            //? ChatGPT: 哪怕 store 的地址和值都是循环不变量，也不提 -> TODO: 有待商榷，个人认为可以提
            || inst->is_alloca() || inst->is_store() // 分配指令为栈分配，存储指令为内存存储
            || inst->is_call() // 函数调用一般不可提，除非调用的函数为纯函数（无副作用）
            || inst->is_phi() // Phi 指令同样是控制流的依赖 // TODO: 如果 Phi 指令的数据都是循环不变量且相等，那么可以对该指令进行一定的处理
            || inst->is_load()) { // load 指令分析见下
            return false;
        }
        //! load 指令的参数是一个指针，结果是这个指针指向的值，就算该指针是循环不变量而且该指针指向的地址未被修改，但是还是有可能存在其他 gep 指令计算出来的指针
        //! 在本质上和当前指针的值相同，即两个不同的指针指向相同的地址，这种情况下如果这个其他的指针被 store 修改了，那么当前 load 指令的结果就会被修改，即不是循环不变量
        // TODO: 所以 load 指令的外提个人认为无法进行，因为运行时的可能向 gep 指令传入变量，从而导致程序的编译期指针指向不确定，从而完全无法确定 load 指令是否可以外提
        // 其余指令：Binary, GEP, Cast, ICmp, FCmp 等
        // gep 指令只要操作数都是循环不变量，那么其计算出来的指针也是循环不变量，后续使用该指针的命令和指针本身无关，哪怕修改了指针指向的地址，但是指针本身依然不变
        for(auto op: inst->operands) {
            if(op->is_const()) {
                continue;
            }
            if(auto def_inst = dynamic_cast<Instruction*>(op)) {
                if(!is_invar[def_inst]) {
                    return false; // 如果操作数是循环体中的指令且不是循环不变量，则不是循环不变量
                }
            }
        }
        return true; // 如果所有操作数都是常量或循环不变量，则可以提
    }
    return false;
}

void show_loops(const LoopVecPtr& loops) {
    for(const auto& loop : *loops) {
        show_loop(loop);
    }
}

void show_loop(const LoopPtr& loop) {
    std::cerr << "***** showing loop information *****\n";
    std::cerr << "  Loop Header: " << loop->header->name << "\n";
    std::cerr << "  Loop Body:\n";
    for(const auto& bb : loop->body) {
        std::cerr << "    " << bb->name << "\n";
    }
    if(loop->parent) {
        std::cerr << "  Parent Loop: " << loop->parent->header->name << "\n";
    } else {
        std::cerr << "  No Parent Loop\n";
    }
    if(!loop->children->empty()) {
        std::cerr << "  Child Loops:\n";
        for (const auto& child : *(loop->children)) {
            std::cerr << "    " << child->header->name << "\n";
        }
    } else {
        std::cerr << "  No Child Loops\n";
    }
    if(loop->pre_header) {
        std::cerr << "  Pre-header: " << loop->pre_header->name << "\n";
    } else {
        std::cerr << "  No Pre-header\n";
    }
    if(loop->latch) {
        std::cerr << "  Latch: " << loop->latch->name << "\n";
    } else {
        std::cerr << "  No Latch\n";
    }
    std::cerr << "************************************\n";
}

void show_biv_to_divs(const unordered_map<Value*, vector<IVExp*>>& biv_to_divs) {
    std::cerr << "********** showing basic induction variables to derived induction variables ***********\n";
    for(const auto& [biv, divs] : biv_to_divs) {
        std::cerr << "Basic Induction Variable: ";
        biv->print(cerr);
        cerr << "\n";
        std::cerr << "  Derived Induction Variable: \n";
        for(const auto& div : divs) {
            std::cerr << div->print() << "\n";
        }
    }
    std::cerr << "***************************************************************************************\n";
}