#include "../../include/ir/gvn.h"
#include <unordered_map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <set>
#include "../../include/ir/cfg.h"
#include "../../include/ir/function_analysis.h"
#include <iostream>
#include <assert.h>

namespace llvm_ir {
namespace gvn {

// 用于表达式签名的结构体
struct Signature {
    Opcode opcode;
    std::vector<int> operand_vns;
    ICmpCond icmp_cond = ICmpCond::EQ; // ICMP专用，默认值
    int64_t const_value = 0; // 对于常量
    size_t unique_id = 0;    // PHI/CALL 唯一ID

    bool operator==(const Signature& other) const {
        return std::tie(opcode, operand_vns, icmp_cond, const_value, unique_id) ==
               std::tie(other.opcode, other.operand_vns, other.icmp_cond, other.const_value, other.unique_id);
    }
};

} // namespace gvn
} // namespace llvm_ir

// hash 实现
namespace std {
template <>
struct hash<llvm_ir::gvn::Signature> {
    size_t operator()(const llvm_ir::gvn::Signature& s) const {
        size_t h = std::hash<int>()(static_cast<int>(s.opcode));
        for (int vn : s.operand_vns) {
            h ^= std::hash<int>()(vn) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        h ^= std::hash<int>()(static_cast<int>(s.icmp_cond)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int64_t>()(s.const_value) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<size_t>()(s.unique_id) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
}

namespace llvm_ir {
namespace gvn {

// 判断是否交换律
bool isCommutative(Opcode op) {
    return op == Opcode::Add || op == Opcode::Mul ||
           op == Opcode::And || op == Opcode::Or ||
           op == Opcode::Xor;
}

// 判断是否有返回值
bool hasResult(Instruction* inst) {
    // store, br, ret, call void, 等无返回值
    if (!inst) return false;
    if (inst->opcode == Opcode::Store || inst->opcode == Opcode::Br || inst->opcode == Opcode::Ret)
        return false;
    if (inst->opcode == Opcode::Call && inst->type == Type::Void)
        return false;
    return true;
}

// 为常量分配VN
int getVNForConstant(Value* v, std::unordered_map<int64_t, int>& const_vn_map, int& next_vn) {
    if (auto ci = dynamic_cast<ConstantInt*>(v)) {
        int64_t val = ci->value;
        if (const_vn_map.count(val)) {
            // std::cout << "[GVN] ConstantInt " << val << " already has VN " << const_vn_map[val] << std::endl;
            return const_vn_map[val];
        }
        int vn = next_vn++;
        const_vn_map[val] = vn;
        // std::cout << "[GVN] Assign VN " << vn << " to ConstantInt " << val << std::endl;
        return vn;
    }
    if (auto cf = dynamic_cast<ConstantFloat*>(v)) {
        int64_t val = static_cast<int64_t>(cf->value * 1e6); // 简单hash
        if (const_vn_map.count(val)) {
            // std::cout << "[GVN] ConstantFloat " << cf->value << " already has VN " << const_vn_map[val] << std::endl;
            return const_vn_map[val];
        }
        int vn = next_vn++;
        const_vn_map[val] = vn;
        // std::cout << "[GVN] Assign VN " << vn << " to ConstantFloat " << cf->value << std::endl;
        return vn;
    }
    return -1;
}

// 生成表达式签名
Signature createSignature(Instruction* inst,
                         std::unordered_map<Value*, int>& vn_map,
                         std::unordered_map<int64_t, int>& const_vn_map,
                         int& next_vn,
                         std::unordered_map<int, Value*>& vn_to_leader) {
    Signature sig;
    sig.opcode = inst->opcode;

    // 常量
    if (auto ci = dynamic_cast<ConstantInt*>(inst)) {
        sig.opcode = Opcode::Add; // 伪指令
        sig.const_value = ci->value;
        return sig;
    }
    if (auto cf = dynamic_cast<ConstantFloat*>(inst)) {
        sig.opcode = Opcode::Add; // 伪指令
        sig.const_value = static_cast<int64_t>(cf->value * 1e6);
        return sig;
    }

    // 只对 Alloca, PHI, Call, Load 分配 unique_id
    if (inst->opcode == Opcode::Alloca ||
        inst->opcode == Opcode::PHI ||
        inst->opcode == Opcode::Call ||
        inst->opcode == Opcode::Load) {
        sig.unique_id = reinterpret_cast<size_t>(inst);
        // std::cout << "[GVN] Create unique signature for " << inst->getOpcodeName()
        //           << ", unique_id=" << sig.unique_id << std::endl;
        return sig;
    }
    if (auto icmp = dynamic_cast<ICmpInst*>(inst)) {
        sig.icmp_cond = icmp->condition;
    }

    // 普通指令 (包括比较指令)
    for (auto* op : inst->operands) {
        int vn = -1;
        if (dynamic_cast<ConstantInt*>(op) || dynamic_cast<ConstantFloat*>(op)) {
            vn = getVNForConstant(op, const_vn_map, next_vn);
            // std::cout << "[GVN] Found constant operand, VN=" << vn << std::endl;
        } else {
            auto it = vn_map.find(op);
            if (it != vn_map.end()) {
                vn = it->second;
                // std::cout << "[GVN] Found operand VN=" << vn << " for " << op->toString() << std::endl;
            } else {
                // 全局变量兜底
                if (!op->name.empty() && op->name[0] == '@') {
                    vn = next_vn++;
                    vn_map[op] = vn;
                    vn_to_leader[vn] = op;
                    // std::cout << "[GVN] Assign VN " << vn << " to global (late) " << op->name << std::endl;
                } else {
                    std::cerr << "[GVN][ERROR] Operand has no VN! Inst: " << inst->toString() << ", Operand: " << op->toString() << std::endl;
                    if (auto* opInst = dynamic_cast<Instruction*>(op)) {
                        std::cerr << "parent of op: " << opInst->parent->label << std::endl;
                        for (auto& instInParent : opInst->parent->instructions) {
                            std::cerr << "instInParent: " << instInParent->name << std::endl;
                        }
                    }
                    assert(false && "All non-constant operands must have a VN.");
                }
            }
        }
        sig.operand_vns.push_back(vn);
    }
    if (isCommutative(inst->opcode)) {
        std::sort(sig.operand_vns.begin(), sig.operand_vns.end());
    }
    return sig;
}

// 按支配树顺序递归遍历
void dfs(BasicBlock* BB,
        Module& M,
        cfg::DominatorTree& DT,
        std::unordered_map<Signature, int>& value_table,
        std::unordered_map<Value*, int>& vn_map,
        std::unordered_map<int, Value*>& vn_to_leader,
        std::unordered_map<int64_t, int>& const_vn_map,
        int& next_vn,
        std::set<Instruction*>& dead_set,
        bool& hasChanged) {
    // --- 作用域管理: 记录本BB新加入的签名 ---
    std::vector<Signature> new_signatures_in_scope;

    // 先处理PHI节点（只在块入口，VN全局有效，不加入作用域清理）
    for (auto& phi_node : BB->phi_instructions) {
        Instruction* I = phi_node.get();
        if (!hasResult(I)) continue;
        Signature sig = createSignature(I, vn_map, const_vn_map, next_vn, vn_to_leader);
        if (value_table.count(sig)) {
            int vn = value_table[sig];
            vn_map[I] = vn;
        } else {
            int vn = next_vn++;
            value_table[sig] = vn;
            vn_map[I] = vn;
            vn_to_leader[vn] = I;
            // std::cout << "[GVN] PHI assigned VN=" << vn << std::endl;
            // PHI节点不加入new_signatures_in_scope
        }
    }

    // 再处理普通指令
    for (auto& inst : BB->instructions) {
        Instruction* I = inst.get();

        // --- 副作用处理 ---
        if (I->opcode == Opcode::Call) {
            auto callInst = dynamic_cast<CallInst*>(I);
            if (callInst) {
                // 使用function_analysis进行更精确的call指令处理
                function_analysis::FunctionAttributeAnalyzer analyzer;
                
                // 查找被调用的函数
                Function* calledFunc = nullptr;
                for (auto& func : M.functions) {
                    if (func->name == callInst->functionName) {
                        calledFunc = func.get();
                        break;
                    }
                }
                
                if (calledFunc) {
                    // 检查函数属性
                    bool isReadNone = analyzer.hasAttribute(*calledFunc, FunctionAttribute::ReadNone);
                    bool isReadOnly = analyzer.hasAttribute(*calledFunc, FunctionAttribute::ReadOnly);
                    
                    if (isReadNone) {
                        // readnone函数：不读取也不写入内存，可以保留所有表达式
                        std::cout << "[GVN] Call to readnone function " << callInst->functionName 
                                  << ", preserving all expressions" << std::endl;
                        // 不清理任何表达式
                    } else if (isReadOnly) {
                        // readonly函数：只读取内存，不写入，可以保留非load表达式
                        std::cout << "[GVN] Call to readonly function " << callInst->functionName 
                                  << ", clearing only load expressions" << std::endl;
                        for (auto it = value_table.begin(); it != value_table.end(); ) {
                            if (it->first.opcode == Opcode::Load) {
                                it = value_table.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    } else {
                        // 可能写入内存的函数：清除所有非PHI和非Alloca表达式
                        std::cout << "[GVN] Call to function " << callInst->functionName 
                                  << " may write memory, clearing most expressions" << std::endl;
                        for (auto it = value_table.begin(); it != value_table.end(); ) {
                            if (it->first.opcode != Opcode::PHI && it->first.opcode != Opcode::Alloca) {
                                it = value_table.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                } else {
                    // 找不到函数定义，保守处理
                    std::cout << "[GVN] Call to unknown function " << callInst->functionName 
                              << ", conservatively clearing expressions" << std::endl;
                    for (auto it = value_table.begin(); it != value_table.end(); ) {
                        if (it->first.opcode != Opcode::PHI && it->first.opcode != Opcode::Alloca) {
                            it = value_table.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        } else if (I->opcode == Opcode::Store) {
            // std::cout << "[GVN] Store detected. Clearing load-related value table entries." << std::endl;
            for (auto it = value_table.begin(); it != value_table.end(); ) {
                if (it->first.opcode == Opcode::Load) {
                    it = value_table.erase(it);
                } else {
                    ++it;
                }
            }
        }
        // --- 副作用处理结束 ---

        if (!hasResult(I)) continue;
        Signature sig = createSignature(I, vn_map, const_vn_map, next_vn, vn_to_leader);
        // std::cout << "[GVN] Processing inst: " << I->toString() << ", sig.opcode=" << static_cast<int>(sig.opcode) << ", operand_vns=[";
        // for (int vn : sig.operand_vns) std::cout << vn << " ";
        // std::cout << "], const_value=" << sig.const_value << std::endl;
        
        if (value_table.count(sig)) {
            int vn = value_table[sig];
            Value* leader = vn_to_leader[vn];
            Instruction* leader_inst = dynamic_cast<Instruction*>(leader);
            // Debug: GVN optimization - try to replace redundant instruction
            // std::cout << "[GVN] Try to optimize: " << I->toString() << " with leader " << (leader ? leader->toString() : "null") << std::endl;
            
            // 安全检查：确保leader和当前指令都有效
            if (!leader) {
                std::cout << "[GVN][WARNING] Leader is null for VN=" << vn << std::endl;
                // 分配新VN继续处理
                int new_vn = next_vn++;
                value_table[sig] = new_vn;
                vn_map[I] = new_vn;
                vn_to_leader[new_vn] = I;
                // std::cout << "[GVN] Inst assigned new VN=" << new_vn << " (leader was null)" << std::endl;
                if (I->opcode != Opcode::PHI && I->opcode != Opcode::Alloca) {
                    new_signatures_in_scope.push_back(sig);
                }
                continue;
            }
            
            // 支配检查
            if (leader_inst && leader_inst->parent && I->parent && DT.dominates(leader_inst->parent, I->parent)) {
                // 额外安全检查：确保leader指令仍然在其基本块中
                bool leader_valid = false;
                if (leader_inst->parent) {
                    // 检查leader是否还在phi_instructions中
                    if (auto phi = dynamic_cast<PhiInst*>(leader_inst)) {
                        for (auto& phi_ptr : leader_inst->parent->phi_instructions) {
                            if (phi_ptr.get() == leader_inst) {
                                leader_valid = true;
                                break;
                            }
                        }
                    } else {
                        // 检查leader是否还在instructions中
                        for (auto& inst_ptr : leader_inst->parent->instructions) {
                            if (inst_ptr.get() == leader_inst) {
                                leader_valid = true;
                                break;
                            }
                        }
                    }
                }
                
                if (leader_valid) {
                    // std::cout << "[GVN] Replacing " << I->toString() << " with " << leader->toString() << std::endl;
                    try {
                        I->replaceAllUsesWith(leader);
                        dead_set.insert(I);
                        hasChanged = true;
                        std::cout << "[GVN] Redundant instruction replaced: " << I->toString() << " -> " << leader->toString() << std::endl;
                    } catch (...) {
                        std::cout << "[GVN][ERROR] Exception during replaceAllUsesWith, skipping replacement" << std::endl;
                        // 分配新VN继续处理
                        int new_vn = next_vn++;
                        value_table[sig] = new_vn;
                        vn_map[I] = new_vn;
                        vn_to_leader[new_vn] = I;
                        if (I->opcode != Opcode::PHI && I->opcode != Opcode::Alloca) {
                            new_signatures_in_scope.push_back(sig);
                        }
                    }
                } else {
                    std::cout << "[GVN][WARNING] Leader instruction is no longer valid, creating new VN" << std::endl;
                    // leader已失效，分配新VN
                    int new_vn = next_vn++;
                    value_table[sig] = new_vn;
                    vn_map[I] = new_vn;
                    vn_to_leader[new_vn] = I;
                    if (I->opcode != Opcode::PHI && I->opcode != Opcode::Alloca) {
                        new_signatures_in_scope.push_back(sig);
                    }
                }
            } else {
                // 支配失败，分配新VN，签名加入作用域清理
                // std::cout << "[GVN] Inst NOT replaced (dominance check failed). VN=" << vn << ". Creating new VN." << std::endl;
                int new_vn = next_vn++;
                // 关键：如果签名已存在但支配失败，需要用新VN更新value_table
                value_table[sig] = new_vn;
                vn_map[I] = new_vn;
                vn_to_leader[new_vn] = I;
                new_signatures_in_scope.push_back(sig);
            }
        } else {
            int vn = next_vn++;
            value_table[sig] = vn;
            vn_map[I] = vn;
            vn_to_leader[vn] = I;
            // Debug: GVN assigns new value number
            // std::cout << "[GVN] New value number assigned for instruction: " << I->toString() << ", VN=" << vn << std::endl;
            // 只有普通指令签名需要作用域管理
            if (I->opcode != Opcode::PHI && I->opcode != Opcode::Alloca) {
                new_signatures_in_scope.push_back(sig);
            }
        }
    }

    // 递归遍历支配树
    for (auto* child : DT.getChildren(BB)) {
        dfs(child, M, DT, value_table, vn_map, vn_to_leader, const_vn_map, next_vn, dead_set, hasChanged);
    }

    // 离开本BB作用域，移除本BB新加的签名，防止兄弟分支信息泄露
    for (const auto& sig : new_signatures_in_scope) {
        // 修复：这里不能简单删除，因为可能内部块的同签名指令已经被替换
        // 只有当leader是本块内指令时才移除
        if (value_table.count(sig)) {
            int vn = value_table[sig];
            if (vn_to_leader.count(vn)) {
                Value* leader = vn_to_leader[vn];
                if (auto leader_inst = dynamic_cast<Instruction*>(leader)) {
                    if (leader_inst->parent == BB) {
                        value_table.erase(sig);
                    }
                }
            }
        }
    }
}

// runOnFunction 新签名，接受 map 的引用
bool runOnFunction(Function& F,
                   Module& M,
                   cfg::DominatorTree& DT,
                   std::unordered_map<Signature, int>& value_table,
                   std::unordered_map<Value*, int>& vn_map,
                   std::unordered_map<int, Value*>& vn_to_leader,
                   std::unordered_map<int64_t, int>& const_vn_map,
                   int& next_vn) {
    bool hasChanged = false;
    // 为每个函数创建完全独立的map，避免跨函数污染
    std::unordered_map<Signature, int> value_table_local;
    std::unordered_map<Value*, int> vn_map_local;
    std::unordered_map<int, Value*> vn_to_leader_local;
    std::unordered_map<int64_t, int> const_vn_map_local;
    int next_vn_local = next_vn; // 从全局next_vn开始，但独立计数

    // 在GVN开始前重建use-def关系，防止instcombine留下的悬空指针
    // std::cout << "[GVN] Rebuilding use-def chains for function: " << F.name << std::endl;
    // 使用通用的use-def链重建函数
    // cfg::rebuildUseDefChainsOnFunction(F, true);

    // 复制全局变量和常量的VN映射到函数局部map
    for (const auto& [sig, vn] : value_table) {
        value_table_local[sig] = vn;
    }
    for (const auto& [val, vn] : vn_map) {
        vn_map_local[val] = vn;
    }
    for (const auto& [vn, val] : vn_to_leader) {
        vn_to_leader_local[vn] = val;
    }
    for (const auto& [const_val, vn] : const_vn_map) {
        const_vn_map_local[const_val] = vn;
    }

    DT.runOnFunction(F);
    std::set<Instruction*> dead_set;

    // --- 为每个参数分配唯一VN ---
    // std::cout << "[GVN] Numbering function arguments for " << F.name << std::endl;
    for (auto* arg : F.parameters) {
        if (vn_map_local.find(arg) == vn_map_local.end()) {
            int vn = next_vn_local++;
            vn_map_local[arg] = vn;
            vn_to_leader_local[vn] = arg;
            // std::cout << "[GVN] Assign VN " << vn << " to argument " << arg->name << std::endl;
        }
    }
    // --- 结束 ---

    BasicBlock* entry = F.getEntryBlock();
    if (!entry) return hasChanged;
    // std::cout << "[GVN] Start GVN traversal on Function: " << F.name << std::endl;
    dfs(entry, M, DT, value_table_local, vn_map_local, vn_to_leader_local, const_vn_map_local, next_vn_local, dead_set, hasChanged);

    // 死代码清理
    for (auto& bb : F.basicBlocks) {
        auto& insts = bb->instructions;
        size_t before = insts.size();
        insts.erase(std::remove_if(insts.begin(), insts.end(),
            [&](const std::unique_ptr<Instruction>& inst) {
                return dead_set.count(inst.get());
            }), insts.end());
        size_t after = insts.size();
        if (before != after) {
            hasChanged = true;
            std::cout << "[GVN] Dead code eliminated in block: " << bb->label << ", removed " << (before - after) << " instructions." << std::endl;
        }
    }

    // 更新全局next_vn，确保不同函数使用不同的VN范围
    next_vn = next_vn_local;
    return hasChanged;
}

bool runOnModule(Module& M) {
    bool hasChanged = false;
    std::cout << "[GVN] Start runOnModule" << std::endl;
    // --- 创建 module 级别的 map ---
    std::unordered_map<Signature, int> value_table;
    std::unordered_map<Value*, int> vn_map;
    std::unordered_map<int, Value*> vn_to_leader;
    std::unordered_map<int64_t, int> const_vn_map;
    int next_vn = 1;
    cfg::DominatorTree DT;

    // --- 预编号全局变量 ---
    // std::cout << "[GVN] Numbering global variables" << std::endl;
    for (auto& global : M.globalVariables) {
        if (vn_map.find(global.get()) == vn_map.end()) {
            int vn = next_vn++;
            vn_map[global.get()] = vn;
            vn_to_leader[vn] = global.get();
            // std::cout << "[GVN] Assign VN " << vn << " to global " << global->name << std::endl;
        }
    }

    // --- 处理每个函数 ---
    for (auto& func : M.functions) {
        if (!func->isDeclaration()) {
            bool funcChanged = runOnFunction(*func, M, DT, value_table, vn_map, vn_to_leader, const_vn_map, next_vn);
            hasChanged = hasChanged || funcChanged;
        }
    }
    
    return hasChanged;
}

} // namespace gvn
} // namespace llvm_ir