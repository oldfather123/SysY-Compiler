#include "../../include/riscv/graph_coloring_alloc.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <cassert>

namespace llvm_ir {

// 静态辅助函数：检查类型
static bool isFloatType(Type type) {
    return type == Type::Float || type == Type::Double;
}

// 构造函数
GraphColoringAllocator::GraphColoringAllocator(Function* f) : func(f) {
    initialize();
}

// 初始化
void GraphColoringAllocator::initialize() {
    initializeRegisters();
    
    // 计算活跃变量分析
    live_info = live_variable_analysis(func);
    
    // 计算活跃区间
    intervals = compute_live_intervals(func);
}

void GraphColoringAllocator::initializeRegisters() {
    // 整数寄存器：只使用保存寄存器，与线性扫描算法保持一致
    int_regs = {
        riscv::x8, riscv::x9, riscv::x18, riscv::x19, riscv::x20, riscv::x21,
        riscv::x22, riscv::x23, riscv::x24, riscv::x25, riscv::x26, riscv::x27
    };
    
    // 浮点寄存器：只使用保存寄存器
    float_regs = {
        riscv::f8, riscv::f9, riscv::f18, riscv::f19, riscv::f20, riscv::f21,
        riscv::f22, riscv::f23, riscv::f24, riscv::f25, riscv::f26, riscv::f27
    };
    
    K_int = int_regs.size();
    K_float = float_regs.size();
}

// 主要分配函数
LinearScanResult GraphColoringAllocator::allocate() {
    // 1. 构建干扰图
    buildInterferenceGraph();
    
    // 2. 检测move指令 (在构建干扰图之后进行)
    detectMoveInstructions();
    
    // 3. 计算溢出代价
    calculateSpillCosts();
    
    // 4. 初始化工作列表
    makeWorklist();
    
    // 5. 主循环：简化、合并、冻结、溢出
    while (!simplify_worklist.empty() || !worklist_moves.empty() || 
           !freeze_worklist.empty() || !spill_worklist.empty()) {
        
        if (!simplify_worklist.empty()) {
            simplify();
        } else if (!worklist_moves.empty()) {
            coalesce();
        } else if (!freeze_worklist.empty()) {
            freeze();
        } else if (!spill_worklist.empty()) {
            selectSpill();
        }
    }
    
    // 6. 选择阶段：为节点分配颜色
    select();
    
    // 7. 生成结果
    // 直接将溢出节点分配到栈上
    return generateResult();
}

// 构建干扰图
void GraphColoringAllocator::buildInterferenceGraph() {
    std::cout << "[GraphColoring] Building interference graph..." << std::endl;
    
    // 创建所有虚拟寄存器的节点
    for (const auto& bb : func->basicBlocks) {
        for (const auto& inst : bb->instructions) {
            if (inst->name != "" && intervals.find(inst.get()) != intervals.end()) {
                auto node = std::make_unique<InterferenceNode>(inst.get());
                nodes[inst.get()] = node.get();
                all_nodes.push_back(std::move(node));
                std::cout << "[GraphColoring] Added node for instruction: " << inst->name << std::endl;
            }
        }
        
        // 处理PHI指令
        for (const auto& phi : bb->phi_instructions) {
            if (phi->name != "" && intervals.find(phi.get()) != intervals.end()) {
                auto node = std::make_unique<InterferenceNode>(phi.get());
                nodes[phi.get()] = node.get();
                all_nodes.push_back(std::move(node));
                std::cout << "[GraphColoring] Added node for PHI: " << phi->name << std::endl;
            }
        }
    }
    
    std::cout << "[GraphColoring] Total nodes created: " << all_nodes.size() << std::endl;
    
    // 构建干扰边：如果两个虚拟寄存器的活跃区间重叠，则它们干扰
    // 注意：move相关的冲突边排除将在后续的合并阶段处理
    for (auto it1 = intervals.begin(); it1 != intervals.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != intervals.end(); ++it2) {
            if (interferes(it1->first, it2->first)) {
                auto node1 = nodes[it1->first];
                auto node2 = nodes[it2->first];
                if (node1 && node2) {
                    addEdge(node1, node2);
                }
            }
        }
    }
}

// 添加干扰边
void GraphColoringAllocator::addEdge(InterferenceNode* u, InterferenceNode* v) {
    if (u != v && u->neighbors.find(v) == u->neighbors.end()) {
        u->neighbors.insert(v);
        v->neighbors.insert(u);
        u->degree++;
        v->degree++;
    }
}

// 检查两个虚拟寄存器是否干扰
bool GraphColoringAllocator::interferes(Value* v1, Value* v2) {
    auto it1 = intervals.find(v1);
    auto it2 = intervals.find(v2);
    
    if (it1 == intervals.end() || it2 == intervals.end()) {
        return false;
    }
    
    const LiveInterval& interval1 = it1->second;
    const LiveInterval& interval2 = it2->second;
    
    // 检查活跃区间是否重叠
    for (const auto& range1 : interval1.ranges) {
        for (const auto& range2 : interval2.ranges) {
            // 区间 [start1, end1) 和 [start2, end2) 重叠的条件
            if (range1.start < range2.end && range2.start < range1.end) {
                return true;
            }
        }
    }
    
    return false;
}

// 检测move指令
void GraphColoringAllocator::detectMoveInstructions() {
    std::cout << "[GraphColoring] Detecting move instructions..." << std::endl;
    int idx=0;
    for (const auto& bb : func->basicBlocks) {
        for (const auto& inst : bb->instructions) {
            idx++;
            std::cout << "[GraphColoring] Examining instruction #" << idx << ": " << inst->name << std::endl;
            if (isMoveInstruction(inst.get())) {
                // 对于Move指令，创建move关系
                std::cout << "[GraphColoring] Found move instruction: " << inst->name << std::endl;
                if (inst->operands.size() >= 1) {
                    // 检查源操作数是否是虚拟寄存器
                    Value* src_value = inst->operands[0];
                    auto src_node = nodes.find(src_value);
                    
                    // Move指令的目标就是指令本身（这是LLVM IR的标准形式）
                    Value* dst_value = inst.get();
                    auto dst_node = nodes.find(dst_value);

                    if (src_node != nodes.end() && dst_node != nodes.end()) {
                        move_instrs.emplace_back(src_node->second, dst_node->second);
                        src_node->second->move_list.insert(dst_node->second);
                        dst_node->second->move_list.insert(src_node->second);
                        worklist_moves.push_back(&move_instrs.back());
                        move_instrs.back().is_worklist_move = true;
                        
                        std::cout << "[GraphColoring] Detected move: " 
                                  << src_value->name << " -> " << dst_value->name << std::endl;
                    } else {
                        std::cout << "[GraphColoring] Skipping move with non-virtual operands: "
                                  << "src=" << src_value->name 
                                  << " (in nodes: " << (src_node != nodes.end() ? "yes" : "no") << ")"
                                  << ", dst=" << dst_value->name 
                                  << " (in nodes: " << (dst_node != nodes.end() ? "yes" : "no") << ")" << std::endl;
                    }
                } else {
                    std::cout << "[GraphColoring] Warning: Move instruction has no operands" << std::endl;
                }
            }
        }
    }
    
    std::cout << "[GraphColoring] Total move instructions detected: " << move_instrs.size() << std::endl;
}

// 检查是否为move指令
bool GraphColoringAllocator::isMoveInstruction(Instruction* inst) {
    return inst->opcode == Opcode::Move;
}

// 计算溢出代价
void GraphColoringAllocator::calculateSpillCosts() {
    for (auto& node_ptr : all_nodes) {
        node_ptr->spill_cost = calculateNodeSpillCost(node_ptr.get());
    }
}

double GraphColoringAllocator::calculateNodeSpillCost(InterferenceNode* node) {
    // 基于访问频率、循环深度等因素计算溢出代价
    double cost = 0.0;
    
    // 基础访问代价
    cost += 10.0;
    
    // TODO: 这里可以集成更复杂的代价模型，如访问频率分析
    // 暂时使用简单的启发式
    
    // 度数越高，代价越低（更容易溢出）
    if (node->degree > 0) {
        cost /= node->degree;
    }
    
    return cost;
}

// 初始化工作列表
void GraphColoringAllocator::makeWorklist() {
    for (auto& node_ptr : all_nodes) {
        InterferenceNode* node = node_ptr.get();
        
        if (!node->is_precolored) {
            addToWorklist(node);
        }
    }
}

void GraphColoringAllocator::addToWorklist(InterferenceNode* node) {
    // 确保节点不在任何工作列表中
    auto remove_from_list = [](std::vector<InterferenceNode*>& list, InterferenceNode* n) {
        auto it = std::find(list.begin(), list.end(), n);
        if (it != list.end()) {
            list.erase(it);
        }
    };
    
    remove_from_list(simplify_worklist, node);
    remove_from_list(freeze_worklist, node);
    remove_from_list(spill_worklist, node);
    
    int K = getK(node->vreg);
    
    if (node->degree >= K) {
        spill_worklist.push_back(node);
    } else if (moveRelated(node)) {
        freeze_worklist.push_back(node);
    } else {
        simplify_worklist.push_back(node);
    }
}

// 简化阶段
void GraphColoringAllocator::simplify() {
    if (simplify_worklist.empty()) return;
    
    InterferenceNode* node = simplify_worklist.back();
    simplify_worklist.pop_back();
    
    select_stack.push(node);
    
    // 更新邻居的度数
    for (InterferenceNode* neighbor : node->neighbors) {
        decrementDegree(neighbor);
    }
}

// 合并阶段
void GraphColoringAllocator::coalesce() {
    if (worklist_moves.empty()) return;
    
    MoveInstr* move = worklist_moves.back();
    worklist_moves.pop_back();
    move->is_worklist_move = false;
    
    InterferenceNode* x = getAlias(move->dst);
    InterferenceNode* y = getAlias(move->src);
    
    InterferenceNode* u, *v;
    if (y->is_precolored) {
        u = y; v = x;
    } else {
        u = x; v = y;
    }
    
    if (u == v) {
        // 已经是同一个节点
        coalesced_moves.push_back(move);
        addToWorklist(u);
        std::cout << "[GraphColoring] Move already coalesced: " << u->vreg->name << std::endl;
    } else if (v->is_precolored || u->neighbors.find(v) != u->neighbors.end()) {
        // 不能合并
        constrained_moves.push_back(move);
        addToWorklist(u);
        addToWorklist(v);
        std::cout << "[GraphColoring] Move constrained: " << u->vreg->name 
                  << " -> " << v->vreg->name << std::endl;
    } else if (canCoalesce(u, v)) {
        // 可以合并
        coalesced_moves.push_back(move);
        combine(u, v);
        addToWorklist(u);
        std::cout << "[GraphColoring] Move coalesced: " << v->vreg->name 
                  << " -> " << u->vreg->name << std::endl;
    } else {
        // 暂时不能合并
        active_moves.push_back(move);
        std::cout << "[GraphColoring] Move made active: " << u->vreg->name 
                  << " -> " << v->vreg->name << std::endl;
    }
}

// 冻结阶段
void GraphColoringAllocator::freeze() {
    if (freeze_worklist.empty()) return;
    
    InterferenceNode* node = freeze_worklist.back();
    freeze_worklist.pop_back();
    
    simplify_worklist.push_back(node);
    
    // 冻结与该节点相关的move
    for (MoveInstr* move : nodeMoves(node)) {
        if (std::find(active_moves.begin(), active_moves.end(), move) != active_moves.end()) {
            active_moves.erase(std::find(active_moves.begin(), active_moves.end(), move));
            frozen_moves.push_back(move);
        } else if (std::find(worklist_moves.begin(), worklist_moves.end(), move) != worklist_moves.end()) {
            worklist_moves.erase(std::find(worklist_moves.begin(), worklist_moves.end(), move));
            move->is_worklist_move = false;
            frozen_moves.push_back(move);
        }
    }
}

// 溢出选择阶段
void GraphColoringAllocator::selectSpill() {
    if (spill_worklist.empty()) return;
    
    // 选择溢出代价最小的节点
    InterferenceNode* spill_node = *std::min_element(
        spill_worklist.begin(), spill_worklist.end(),
        [](InterferenceNode* a, InterferenceNode* b) {
            return a->spill_cost < b->spill_cost;
        }
    );
    
    spill_worklist.erase(std::find(spill_worklist.begin(), spill_worklist.end(), spill_node));
    simplify_worklist.push_back(spill_node);
    
    // 冻结与该节点相关的move
    for (MoveInstr* move : nodeMoves(spill_node)) {
        if (std::find(worklist_moves.begin(), worklist_moves.end(), move) != worklist_moves.end()) {
            worklist_moves.erase(std::find(worklist_moves.begin(), worklist_moves.end(), move));
            move->is_worklist_move = false;
            frozen_moves.push_back(move);
        }
    }
}

// 选择阶段：为节点分配颜色
void GraphColoringAllocator::select() {
    while (!select_stack.empty()) {
        InterferenceNode* node = select_stack.top();
        select_stack.pop();
        
        riscv::reg color = selectColor(node);
        
        if (color != riscv::x0) {  // 假设x0表示无效寄存器
            node->color = color;
            node->is_colored = true;
            colored_nodes.push_back(node);
        } else {
            node->is_spilled = true;
            spilled_nodes.push_back(node);
        }
    }
}

// 选择颜色
riscv::reg GraphColoringAllocator::selectColor(InterferenceNode* node) {
    std::set<riscv::reg> used_colors = getUsedColors(node);
    std::vector<riscv::reg>& available_regs = getAvailableRegs(node->vreg);
    
    for (riscv::reg reg : available_regs) {
        if (used_colors.find(reg) == used_colors.end()) {
            return reg;
        }
    }
    
    return riscv::x0;  // 无法着色，需要溢出
}

// 获取邻居已使用的颜色
std::set<riscv::reg> GraphColoringAllocator::getUsedColors(InterferenceNode* node) {
    std::set<riscv::reg> used_colors;
    
    for (InterferenceNode* neighbor : node->neighbors) {
        InterferenceNode* alias_neighbor = getAlias(neighbor);
        if (alias_neighbor->is_colored || alias_neighbor->is_precolored) {
            used_colors.insert(alias_neighbor->color);
        }
    }
    
    return used_colors;
}

// 合并相关的辅助函数
bool GraphColoringAllocator::canCoalesce(InterferenceNode* u, InterferenceNode* v) {
    if (u->is_precolored) {
        return ok(v, u);
    } else {
        return conservative(u, v);
    }
}

void GraphColoringAllocator::combine(InterferenceNode* u, InterferenceNode* v) {
    if (std::find(freeze_worklist.begin(), freeze_worklist.end(), v) != freeze_worklist.end()) {
        freeze_worklist.erase(std::find(freeze_worklist.begin(), freeze_worklist.end(), v));
    } else {
        spill_worklist.erase(std::find(spill_worklist.begin(), spill_worklist.end(), v));
    }
    
    coalesced_nodes.push_back(v);
    alias[v] = u;
    
    // 合并move列表
    u->move_list.insert(v->move_list.begin(), v->move_list.end());
    
    // 合并邻接表
    for (InterferenceNode* t : v->neighbors) {
        addEdge(t, u);
        decrementDegree(t);
    }
    
    if (u->degree >= getK(u->vreg) && 
        std::find(freeze_worklist.begin(), freeze_worklist.end(), u) != freeze_worklist.end()) {
        freeze_worklist.erase(std::find(freeze_worklist.begin(), freeze_worklist.end(), u));
        spill_worklist.push_back(u);
    }
}

InterferenceNode* GraphColoringAllocator::getAlias(InterferenceNode* node) {
    if (std::find(coalesced_nodes.begin(), coalesced_nodes.end(), node) != coalesced_nodes.end()) {
        return getAlias(alias[node]);
    } else {
        return node;
    }
}

bool GraphColoringAllocator::conservative(InterferenceNode* u, InterferenceNode* v) {
    std::set<InterferenceNode*> combined_neighbors;
    combined_neighbors.insert(u->neighbors.begin(), u->neighbors.end());
    combined_neighbors.insert(v->neighbors.begin(), v->neighbors.end());
    
    int high_degree_count = 0;
    int K = std::max(getK(u->vreg), getK(v->vreg));
    
    for (InterferenceNode* neighbor : combined_neighbors) {
        if (neighbor->degree >= K) {
            high_degree_count++;
        }
    }
    
    return high_degree_count < K;
}

bool GraphColoringAllocator::ok(InterferenceNode* t, InterferenceNode* r) {
    int K = getK(t->vreg);
    return t->degree < K || t->is_precolored || 
           t->neighbors.find(r) != t->neighbors.end();
}

// Move相关的辅助函数
std::vector<MoveInstr*> GraphColoringAllocator::nodeMoves(InterferenceNode* node) {
    std::vector<MoveInstr*> moves;
    
    for (MoveInstr* move : active_moves) {
        if (move->src == node || move->dst == node) {
            moves.push_back(move);
        }
    }
    
    for (MoveInstr* move : worklist_moves) {
        if (move->src == node || move->dst == node) {
            moves.push_back(move);
        }
    }
    
    return moves;
}

bool GraphColoringAllocator::moveRelated(InterferenceNode* node) {
    return !nodeMoves(node).empty();
}

// 启用移动指令
void GraphColoringAllocator::enableMoves(InterferenceNode* node) {
    for (MoveInstr* move : nodeMoves(node)) {
        if (std::find(active_moves.begin(), active_moves.end(), move) != active_moves.end()) {
            active_moves.erase(std::find(active_moves.begin(), active_moves.end(), move));
            worklist_moves.push_back(move);
            move->is_worklist_move = true;
        }
    }
}

void GraphColoringAllocator::enableMoves(const std::vector<InterferenceNode*>& nodes) {
    for (InterferenceNode* node : nodes) {
        enableMoves(node);
    }
}

void GraphColoringAllocator::decrementDegree(InterferenceNode* node) {
    int K = getK(node->vreg);
    int old_degree = node->degree;
    node->degree--;
    
    if (old_degree == K) {
        // 当度数从K降到K-1时，启用相关移动
        enableMoves(node);
        
        // 从spill_worklist移动到其他列表
        auto it = std::find(spill_worklist.begin(), spill_worklist.end(), node);
        if (it != spill_worklist.end()) {
            spill_worklist.erase(it);
            
            if (moveRelated(node)) {
                freeze_worklist.push_back(node);
            } else {
                simplify_worklist.push_back(node);
            }
        }
    }
}

// 辅助函数实现
bool GraphColoringAllocator::isFloatType(Value* vreg) {
    return ::llvm_ir::isFloatType(vreg->type);
}

int GraphColoringAllocator::getK(Value* vreg) {
    return isFloatType(vreg) ? K_float : K_int;
}

std::vector<riscv::reg>& GraphColoringAllocator::getAvailableRegs(Value* vreg) {
    return isFloatType(vreg) ? float_regs : int_regs;
}

// 生成结果
LinearScanResult GraphColoringAllocator::generateResult() {
    LinearScanResult result;
    
    int int_stack_offset = 0;
    int float_stack_offset = 0;
    
    // 首先处理着色成功的节点
    for (InterferenceNode* node : colored_nodes) {
        RegAllocResult alloc_result;
        alloc_result.is_reg = true;
        alloc_result.regid = node->color;
        alloc_result.stack_offset = -1;
        
        result.reg_alloc_map[node->vreg] = alloc_result;
    }
    
    // 然后处理溢出的节点
    for (InterferenceNode* node : spilled_nodes) {
        RegAllocResult alloc_result;
        alloc_result.is_reg = false;
        alloc_result.regid = riscv::x0;
        
        if (isFloatType(node->vreg)) {
            alloc_result.stack_offset = int_stack_offset + float_stack_offset;
            float_stack_offset += 8;  // 浮点数占8字节
            result.spilled_vregs.insert(node->vreg);
        } else {
            alloc_result.stack_offset = int_stack_offset + float_stack_offset;
            int_stack_offset += 8;    // 整数占8字节
            result.spilled_vregs.insert(node->vreg);
        }
        
        result.reg_alloc_map[node->vreg] = alloc_result;
    }
    
    // 最后处理合并的节点（此时别名节点已经被处理）
    std::cout << "[GraphColoring] Processing " << coalesced_nodes.size() << " coalesced nodes..." << std::endl;
    for (InterferenceNode* node : coalesced_nodes) {
        InterferenceNode* alias_node = getAlias(node);
        
        std::cout << "[GraphColoring] Coalesced node " << node->vreg->name 
                  << " -> alias " << alias_node->vreg->name << std::endl;
        
        // 查找别名节点的分配结果
        auto alias_it = result.reg_alloc_map.find(alias_node->vreg);
        if (alias_it != result.reg_alloc_map.end()) {
            // 合并节点使用与别名节点相同的分配结果
            result.reg_alloc_map[node->vreg] = alias_it->second;
            
            std::cout << "[GraphColoring] Assigned " << node->vreg->name 
                      << " same allocation as " << alias_node->vreg->name;
            if (alias_it->second.is_reg) {
                std::cout << " (register)" << std::endl;
            } else {
                std::cout << " (stack offset " << alias_it->second.stack_offset << ")" << std::endl;
            }
            
            // 如果别名节点被溢出，也将合并节点加入溢出集合
            if (!alias_it->second.is_reg) {
                result.spilled_vregs.insert(node->vreg);
            }
        } else {
            // 这种情况不应该发生，但为了安全起见提供默认处理
            std::cerr << "Warning: Alias node " << alias_node->vreg->name 
                      << " not found for coalesced node " << node->vreg->name << std::endl;
            
            // 默认溢出处理
            RegAllocResult alloc_result;
            alloc_result.is_reg = false;
            alloc_result.regid = riscv::x0;
            
            if (isFloatType(node->vreg)) {
                alloc_result.stack_offset = int_stack_offset + float_stack_offset;
                float_stack_offset += 8;
            } else {
                alloc_result.stack_offset = int_stack_offset + float_stack_offset;
                int_stack_offset += 8;
            }
            
            result.reg_alloc_map[node->vreg] = alloc_result;
            result.spilled_vregs.insert(node->vreg);
        }
    }
    
    result.stack_size_int = int_stack_offset;
    result.stack_size_float = float_stack_offset;
    result.intervals = intervals;
    
    return result;
}

// 主要接口函数
LinearScanResult graph_coloring_allocate(Function* func) {
    GraphColoringAllocator allocator(func);
    return allocator.allocate();
}

} // namespace llvm_ir
