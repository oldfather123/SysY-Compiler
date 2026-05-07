#include "../../include/ir/llvm_ir.h"
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <functional>

namespace llvm_ir {
namespace ssadestruction {

// 用于检测强连通分量的辅助结构
struct SCCInfo {
    int index = -1;
    int lowlink = -1;
    bool onStack = false;
};

// 统一的PHI依赖分析器：提供SCC检测和拓扑排序功能
class PhiDependencyAnalyzer {
private:
    std::unordered_map<PhiInst*, SCCInfo> nodeInfo;
    std::stack<PhiInst*> nodeStack;
    std::vector<std::vector<PhiInst*>> sccs;
    int currentIndex;
    
    // 构建依赖图：phi -> 它依赖的其他phi
    std::unordered_map<PhiInst*, std::vector<PhiInst*>> dependencyGraph;
    
public:
    // 构建依赖图并检测强连通分量
    std::vector<std::vector<PhiInst*>> detectSCCs(const std::vector<PhiInst*>& allPhis) {
        // 清空状态
        nodeInfo.clear();
        while (!nodeStack.empty()) nodeStack.pop();
        sccs.clear();
        dependencyGraph.clear();
        currentIndex = 0;
        
        // 构建依赖图
        buildDependencyGraph(allPhis);
        
        // 对每个未访问的节点运行 Tarjan 算法
        for (PhiInst* phi : allPhis) {
            if (nodeInfo[phi].index == -1) {
                tarjan(phi);
            }
        }
        
        return sccs;
    }
    
    // 对简单phi指令进行拓扑排序
    std::vector<PhiInst*> topologicalSort(const std::vector<PhiInst*>& simplePhi) {
        if (simplePhi.empty()) {
            return {};
        }
        
        // 清空状态并构建依赖图
        dependencyGraph.clear();
        buildDependencyGraph(simplePhi);
        
        // 拓扑排序
        std::unordered_set<PhiInst*> visited;
        std::unordered_set<PhiInst*> temp;
        std::vector<PhiInst*> result;
        
        std::function<void(PhiInst*)> dfs = [&](PhiInst* phi) {
            if (temp.count(phi)) {
                // 检测到循环，这不应该发生，因为简单phi不应该有循环
                std::cerr << "[SSADestruction] Warning: Detected cycle in simple phi, this should not happen" << std::endl;
                return;
            }
            if (visited.count(phi)) {
                return;
            }
            
            temp.insert(phi);
            for (PhiInst* dep : dependencyGraph[phi]) {
                dfs(dep);
            }
            temp.erase(phi);
            visited.insert(phi);
            result.push_back(phi);
        };
        
        for (PhiInst* phi : simplePhi) {
            if (!visited.count(phi)) {
                dfs(phi);
            }
        }
        
        // 反转结果，因为dfs是后序遍历
        std::reverse(result.begin(), result.end());
        
        return result;
    }
    
private:
    // 构建依赖图
    void buildDependencyGraph(const std::vector<PhiInst*>& allPhis) {
        // 1. 创建一个所有 phi 指令的集合，用于快速查找
        std::unordered_set<Value*> allPhisSet(allPhis.begin(), allPhis.end());
    
        // 2. 遍历每一个 phi 指令，构建全局依赖图
        for (PhiInst* phi : allPhis) {
            // 确保图中包含该节点，即使它没有出边
            if (dependencyGraph.find(phi) == dependencyGraph.end()) {
                dependencyGraph[phi] = {};
            }
    
            // 3. 检查此 phi 的所有 incoming values
            for (const auto& incoming : phi->incoming_values) {
                Value* val = incoming.first;
                
                // 4. 如果 incoming value 是任何一个 phi 指令（无论在哪个块），
                //    则添加一条依赖边。
                if (allPhisSet.count(val)) {
                    // val 是一个 phi 指令，所以存在依赖关系：phi -> val
                    dependencyGraph[phi].push_back(static_cast<PhiInst*>(val));
                }
            }
        }
    }
    
    void tarjan(PhiInst* v) {
        // 设置节点的索引和低链接值
        nodeInfo[v].index = currentIndex;
        nodeInfo[v].lowlink = currentIndex;
        currentIndex++;
        nodeStack.push(v);
        nodeInfo[v].onStack = true;
        
        // 遍历所有邻居
        for (PhiInst* w : dependencyGraph[v]) {
            if (nodeInfo[w].index == -1) {
                // 未访问过的邻居
                tarjan(w);
                nodeInfo[v].lowlink = std::min(nodeInfo[v].lowlink, nodeInfo[w].lowlink);
            } else if (nodeInfo[w].onStack) {
                // 邻居在栈上，说明是后向边
                nodeInfo[v].lowlink = std::min(nodeInfo[v].lowlink, nodeInfo[w].index);
            }
        }
        
        // 如果 v 是强连通分量的根
        if (nodeInfo[v].lowlink == nodeInfo[v].index) {
            std::vector<PhiInst*> scc;
            PhiInst* w;
            do {
                w = nodeStack.top();
                nodeStack.pop();
                nodeInfo[w].onStack = false;
                scc.push_back(w);
            } while (w != v);
            
            sccs.push_back(scc);
        }
    }
};

void handlePhiReplacement(PhiInst* phi, Function& F);

// 将phi指令转换为move指令
void destroySSA(Function& F) {
    std::cout << "[SSADestruction] destroying SSA for function: " << F.name << std::endl;
    int tmpCounter = 0; // 临时寄存器计数器
    
    // 收集所有phi指令
    std::vector<PhiInst*> allPhis;
    for (auto& BB : F.basicBlocks) {
        for (auto& phi : BB->phi_instructions) {
            PhiInst* phi_ptr = phi.get();
            // 只处理有incoming_values的PHI节点
            if (!phi_ptr->incoming_values.empty()) {
                allPhis.push_back(phi_ptr);
            }
        }
    }
    std::cout << "[SSADestruction] found " << allPhis.size() << " phi instructions" << std::endl;

    if (allPhis.empty()) {
        std::cout << "[SSADestruction] no phi instructions to destroy" << std::endl;
        return;
    }

    // 检测强连通分量（循环依赖）
    PhiDependencyAnalyzer analyzer;
    auto sccs = analyzer.detectSCCs(allPhis);
    
    std::cout << "[SSADestruction] detected " << sccs.size() << " strongly connected components" << std::endl;
    
    // 分类处理：区分需要临时变量和不需要的phi
    std::unordered_set<PhiInst*> needsTempVar;
    std::vector<PhiInst*> simplePhi;
    
    for (const auto& scc : sccs) {
        if (scc.size() > 1) {
            // 多个phi形成循环，需要临时变量
            std::cout << "[SSADestruction] SCC with " << scc.size() << " nodes needs temporary variables" << std::endl;
            for (PhiInst* phi : scc) {
                std::cout << "[SSADestruction] SCC phi: " << phi->toString() << std::endl;
                needsTempVar.insert(phi);
            }
        } else {
            // 单个phi，检查是否自引用
            PhiInst* phi = scc[0];
            bool selfReference = false;
            for (const auto& incoming : phi->incoming_values) {
                if (incoming.first == phi) {
                    selfReference = true;
                    break;
                }
            }
            
            if (selfReference) {
                std::cout << "[SSADestruction] Self-referencing phi needs temporary variable: " << phi->name << std::endl;
                needsTempVar.insert(phi);
            } else {
                simplePhi.push_back(phi);
            }
        }
    }
    
    std::cout << "[SSADestruction] " << needsTempVar.size() << " phi instructions need temporary variables" << std::endl;
    std::cout << "[SSADestruction] " << simplePhi.size() << " phi instructions can be handled directly" << std::endl;
    
    // 为需要临时变量的phi创建临时寄存器
    std::map<PhiInst*, Value*> phiToTmpMap;
    for (PhiInst* phi : needsTempVar) {
        // 创建唯一的临时寄存器名
        std::string tmpName = "%tmp_" + phi->name.substr(1) + "_" + std::to_string(tmpCounter++);
        
        // 创建临时寄存器值
        Value* tmpValue = new Value(tmpName, phi->type);
        phiToTmpMap[phi] = tmpValue;
    }

    // 处理需要临时变量的phi指令
    for (PhiInst* phi : needsTempVar) {
        // 检查 PHI 节点是否仍然存在
        bool phi_still_exists = false;
        for (auto& bb_ptr : F.basicBlocks) {
            for (auto& phi_ptr : bb_ptr->phi_instructions) {
                if (phi_ptr.get() == phi) {
                    phi_still_exists = true;
                    break;
                }
            }
            if (phi_still_exists) break;
        }
        
        if (!phi_still_exists) {
            std::cout << "[SSADestruction] PHI node " << phi << " already deleted by other optimization, skipping" << std::endl;
            continue;
        }

        std::cout << "[SSADestruction] processing complex phi: " << phi->toString() << std::endl;
        Value* tmpValue = phiToTmpMap[phi];
        BasicBlock* currentBB = phi->parent;

        // 在前驱块末尾添加赋值到临时寄存器
        for (auto& incoming : phi->incoming_values) {
            Value* val = incoming.first;
            BasicBlock* fromBB = incoming.second;

            // 如果incoming value是phi本身，则跳过
            if (val == phi) {
                std::cout << "[SSADestruction] Skipping self-reference in phi incoming" << std::endl;
                continue;
            }
            
            // 检查是否为undef，如果是则替换为0
            if (dynamic_cast<UndefValue*>(val)) {
                std::cout << "[SSADestruction] Replacing undef value with 0 in phi incoming" << std::endl;
                // 根据PHI节点的类型创建对应的0值
                if (phi->type == Type::I1 || phi->type == Type::I8 || phi->type == Type::I32 || phi->type == Type::I64) {
                    val = new ConstantInt(0, phi->type);
                } else if (phi->type == Type::Float || phi->type == Type::Double) {
                    val = new ConstantFloat(0.0, phi->type);
                } else {
                    std::cout << "[SSADestruction] Warning: Cannot create zero value for unsupported type, skipping" << std::endl;
                    continue;
                }
            }
            
            // 创建MoveInst: tmp = val
            auto moveToTmp = std::make_unique<MoveInst>(val, tmpValue->name, phi->type, tmpValue);
            std::cout << "[SSADestruction] move inst: " << moveToTmp->toString() << " from " << fromBB->label << std::endl;

            // 计算插入位置：终止指令前
            size_t insert_pos = fromBB->instructions.size();
            if (!fromBB->instructions.empty()) {
                auto last = fromBB->instructions.end();
                --last;
                if ((*last)->opcode == Opcode::Br || (*last)->opcode == Opcode::Ret || (*last)->opcode == Opcode::Switch) {
                    insert_pos = fromBB->instructions.size() - 1;
                }
            }
            fromBB->insertInstructionAt(std::move(moveToTmp), insert_pos);
        }

        // 在phi所在基本块开头添加从临时寄存器赋值
        auto moveFromTmp = std::make_unique<MoveInst>(tmpValue, phi->name, phi->type, nullptr);
        std::cout << "[SSADestruction] move from tmp: " << moveFromTmp->toString() << std::endl;
        
        // 直接插入到currentBB->instructions的最开始
        currentBB->insertInstructionAt(std::move(moveFromTmp), 0);

        // 替换所有对phi的使用
        handlePhiReplacement(phi, F);
    }
    
    // 对简单phi指令按依赖关系进行拓扑排序
    std::vector<PhiInst*> sortedSimplePhi = analyzer.topologicalSort(simplePhi);
    
    // 处理简单的phi指令
    for (PhiInst* phi : sortedSimplePhi) {
        // 创建临时寄存器值（仅用于引用，不实际添加到IR）
        Value* tmpValue = new Value(phi->name, phi->type);

        // 检查 PHI 节点是否仍然存在
        bool phi_still_exists = false;
        for (auto& bb_ptr : F.basicBlocks) {
            for (auto& phi_ptr : bb_ptr->phi_instructions) {
                if (phi_ptr.get() == phi) {
                    phi_still_exists = true;
                    break;
                }
            }
            if (phi_still_exists) break;
        }
        
        if (!phi_still_exists) {
            std::cout << "[SSADestruction] PHI node " << phi << " already deleted by other optimization, skipping" << std::endl;
            continue;
        }

        std::cout << "[SSADestruction] processing simple phi: " << phi->toString() << std::endl;

        // 直接在前驱块末尾添加赋值
        for (auto& incoming : phi->incoming_values) {
            Value* val = incoming.first;
            BasicBlock* fromBB = incoming.second;

            // 如果incoming value是phi本身，则跳过
            if (val == phi) {
                std::cout << "[SSADestruction] Skipping self-reference in phi incoming" << std::endl;
                continue;
            }
            
            // 检查是否为undef，如果是则替换为0
            if (dynamic_cast<UndefValue*>(val)) {
                std::cout << "[SSADestruction] Replacing undef value with 0 in phi incoming" << std::endl;
                // 根据PHI节点的类型创建对应的0值
                if (phi->type == Type::I1 || phi->type == Type::I8 || phi->type == Type::I32 || phi->type == Type::I64) {
                    val = new ConstantInt(0, phi->type);
                } else if (phi->type == Type::Float || phi->type == Type::Double) {
                    val = new ConstantFloat(0.0, phi->type);
                } else {
                    std::cout << "[SSADestruction] Warning: Cannot create zero value for unsupported type, skipping" << std::endl;
                    continue;
                }
            }
            
            // 创建MoveInst: phi_name = val
            auto moveInst = std::make_unique<MoveInst>(val, phi->name, phi->type, tmpValue);
            std::cout << "[SSADestruction] direct move inst: " << moveInst->toString() << " from " << fromBB->label << std::endl;

            // 计算插入位置：终止指令前
            size_t insert_pos = fromBB->instructions.size();
            if (!fromBB->instructions.empty()) {
                auto last = fromBB->instructions.end();
                --last;
                if ((*last)->opcode == Opcode::Br || (*last)->opcode == Opcode::Ret || (*last)->opcode == Opcode::Switch) {
                    insert_pos = fromBB->instructions.size() - 1;
                }
            }
            fromBB->insertInstructionAt(std::move(moveInst), insert_pos);
        }

        // 替换所有对phi的使用
        handlePhiReplacement(phi, F);
    }

    // 移除所有phi指令
    for (auto& BB : F.basicBlocks) {
        auto& phis = BB->phi_instructions;
        phis.erase(std::remove_if(phis.begin(), phis.end(), [](const std::unique_ptr<PhiInst>& phi) {
            return true;
        }), phis.end());
    }
    
    std::cout << "[SSADestruction] destroyed SSA for function: " << F.name << std::endl;
}

// 辅助函数：处理phi替换
void handlePhiReplacement(PhiInst* phi, Function& F) {
    // 获取刚插入的Move指令指针
    MoveInst* moveInst = nullptr;
    BasicBlock* currentBB = phi->parent;
    
    // 对于复杂phi，从开头找；对于简单phi，从多个前驱块中找第一个移动指令
    for (auto& inst : currentBB->instructions) {
        if (inst->opcode == Opcode::Move && inst->name == phi->name) {
            moveInst = static_cast<MoveInst*>(inst.get());
            break;
        }
    }
    
    // 如果在当前块没找到，说明是简单phi，在前驱块中创建了move指令
    // 这种情况下我们不需要替换，因为move指令已经使用了正确的名字

    // 清理 PHI 节点的 uses 列表，移除已经被删除的指令
    auto& phi_uses = phi->getUses();
    std::vector<Instruction*> valid_uses;
    for (auto* user : phi_uses) {
        bool user_still_exists = false;
        // 检查这个使用者是否还存在于某个基本块中
        for (auto& bb_ptr : F.basicBlocks) {
            // 检查普通指令
            for (auto& inst_ptr : bb_ptr->instructions) {
                if (inst_ptr.get() == user) {
                    user_still_exists = true;
                    break;
                }
            }
            if (user_still_exists) break;
            
            // 检查 PHI 指令
            for (auto& phi_ptr : bb_ptr->phi_instructions) {
                if (phi_ptr.get() == user) {
                    user_still_exists = true;
                    break;
                }
            }
            if (user_still_exists) break;
        }
        
        if (user_still_exists) {
            valid_uses.push_back(user);
        } else {
            std::cout << "[SSADestruction] Removing dangling use from PHI uses list" << std::endl;
        }
    }
    phi_uses = valid_uses;
    
    if (moveInst) {
        // 替换所有对phi的使用
        phi->replaceAllUsesWith(moveInst);
        std::cout << "[SSADestruction] replaced all uses of phi with move instruction" << std::endl;
    } else {
        std::cout << "[SSADestruction] handling simple phi" << std::endl;
        for (auto& bb : F.basicBlocks) {
            for (auto& inst : bb->instructions) {
                if (inst->opcode == Opcode::Move && inst->name == phi->name) {
                    moveInst = static_cast<MoveInst*>(inst.get());
                    break;
                }
            }
        }
        phi->replaceAllUsesWith(moveInst->get_value_ptr());
    }
}

} // namespace ssadestruction
} // namespace llvm_ir