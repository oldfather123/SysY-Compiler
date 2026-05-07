#include "LICM.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "logging.hpp"

#include <set>
#include <queue>
#include <algorithm>
#include <iostream>

void LoopInvariantCodeMotion::run() {
    loop_detection_ = std::make_unique<LoopDetection>(m_);
    loop_detection_->run();

    func_info_ = std::make_unique<FuncInfo>(m_);
    func_info_->run();

    // 初始化所有循环的处理状态
    for (auto &loop : loop_detection_->get_loops()) {
        is_loop_done_[loop] = false;
    }

    // 收集所有循环并按深度排序（最内层循环先处理）
    std::vector<std::shared_ptr<Loop>> sorted_loops;
    for (auto &loop : loop_detection_->get_loops()) {
        sorted_loops.push_back(loop);
    }
    
    // 按深度降序排序
    std::sort(sorted_loops.begin(), sorted_loops.end(), 
              [](const std::shared_ptr<Loop>& a, const std::shared_ptr<Loop>& b) {
                  return a->get_depth() > b->get_depth();
              });
    
    // 处理每个循环
    int hoisted_count = 0;
    for (auto &loop : sorted_loops) {
        if (!is_loop_done_[loop]) {
            hoisted_count += run_on_loop(loop);
            is_loop_done_[loop] = true;
        }
    }
    
    LOG_INFO << "LICM: hoisted " << hoisted_count << " instructions";
}

void LoopInvariantCodeMotion::collect_loop_info(
    std::shared_ptr<Loop> loop,
    std::set<Value *> &loop_instructions,
    std::set<Value *> &updated_values,
    std::set<BasicBlock *> &loop_blocks,
    bool &contains_impure_call) {
    
    // 收集循环内的所有基本块
    for (auto &bb : loop->get_blocks()) {
        loop_blocks.insert(bb);
    }
    
    // 收集循环内的所有指令和被修改的值
    for (auto &bb : loop->get_blocks()) {
        for (auto &instr : bb->get_instructions()) {
            loop_instructions.insert(&instr);
            
            // 检查store指令
            if (instr.is_store()) {
                auto store = static_cast<StoreInst*>(&instr);
                Value *lval = store->get_lval();
                updated_values.insert(lval);
                
                // 如果是通过GEP访问，记录基址
                if (auto gep = dynamic_cast<GetElementPtrInst*>(lval)) {
                    updated_values.insert(gep->get_operand(0));
                    // 记录所有可能的别名
                    auto base = gep->get_operand(0);
                    if (auto ptr = dynamic_cast<Instruction*>(base)) {
                        updated_values.insert(ptr);
                    }
                }
            }
            
            // 检查call指令
            if (instr.is_call()) {
                auto call = static_cast<CallInst*>(&instr);
                auto func = call->get_function();
                
                // 使用FuncInfo判断函数纯度
                if (!func_info_ || !func_info_->is_pure_function(func)) {
                    contains_impure_call = true;
                }
            }
        }
    }
}

bool LoopInvariantCodeMotion::is_safe_to_hoist(
    Instruction *instr,
    const std::set<Value *> &loop_instructions,
    const std::set<Value *> &updated_values,
    const std::set<BasicBlock *> &loop_blocks) {
    
    // 不能移动的指令类型
    if (instr->is_store() || instr->is_ret() || instr->is_br() || 
        instr->is_phi() || instr->is_alloca()) {
        return false;
    }
    
    // 检查call指令
    if (instr->is_call()) {
        auto call = static_cast<CallInst*>(instr);
        auto func = call->get_function();
        
        // 只有纯函数的调用才能外提
        if (!func_info_ || !func_info_->is_pure_function(func)) {
            return false;
        }
    }
    
    // 检查load指令的内存依赖
    if (instr->is_load()) {
        auto load = static_cast<LoadInst*>(instr);
        Value *addr = load->get_lval();
        
        // 如果加载的地址在循环中被修改，不能外提
        if (updated_values.count(addr)) {
            return false;
        }
        
        // 检查是否可能存在别名
        for (auto updated : updated_values) {
            if (may_alias(addr, updated)) {
                return false;
            }
        }
    }
    
    // 检查所有操作数是否在preheader可用
    for (auto &op : instr->get_operands()) {
        // 常量总是可用的
        if (dynamic_cast<Constant*>(op)) {
            continue;
        }
        
        // 全局变量是可用的
        if (dynamic_cast<GlobalVariable*>(op)) {
            continue;
        }
        
        // 函数参数是可用的
        if (dynamic_cast<Argument*>(op)) {
            continue;
        }
        
        // 检查操作数是否在循环内定义
        if (auto op_inst = dynamic_cast<Instruction*>(op)) {
           //如果操作数在节点内
            if (op_inst->is_phi()) {
                if (loop_blocks.find(op_inst->get_parent()) != loop_blocks.end()) {
                    return false;
                }
                
                // 检查所有循环的嵌套关系
                for (auto &other_loop : loop_detection_->get_loops()) {
                    if (other_loop->contains(op_inst->get_parent())) {
                       
                        bool other_contains_current = false;
                        for (auto &bb : other_loop->get_blocks()) {
                            if (bb == instr->get_parent()) {
                                other_contains_current = true;
                                break;
                            }
                        }
                        
                      
                        // 循环包含当前循环的preheader，则不能外提，若非，则外提。
                        if (other_contains_current) {
                            return false;
                        }
                    }
                }
            }
            
        
            if (loop_blocks.find(op_inst->get_parent()) == loop_blocks.end()) {
               
                // 如果操作数在任何其他循环内定义，则不外提
                for (auto &other_loop : loop_detection_->get_loops()) {
                    if (other_loop->contains(op_inst->get_parent())) {
                        return false;
                    }
                }
                continue;
            }
            
            //检查是否已被识别为循环不变
            if (!loop_invariant_instrs_.count(op_inst)) {
                return false;
            }
        }
    }
    
    return true;
}

bool LoopInvariantCodeMotion::may_alias(Value *ptr1, Value *ptr2) {
    // 简单的别名分析
    if (ptr1 == ptr2) return true;
    
    
    if (dynamic_cast<AllocaInst*>(ptr1) && dynamic_cast<AllocaInst*>(ptr2)) {
        return false;
    }
    
   
    if (auto gv1 = dynamic_cast<GlobalVariable*>(ptr1)) {
        if (auto gv2 = dynamic_cast<GlobalVariable*>(ptr2)) {
            return gv1 == gv2;
        }
    }
    
    // 处理GEP指令
    auto gep1 = dynamic_cast<GetElementPtrInst*>(ptr1);
    auto gep2 = dynamic_cast<GetElementPtrInst*>(ptr2);
    
    if (gep1 && gep2) {
        // 获取基址
        auto base1 = gep1->get_operand(0);
        auto base2 = gep2->get_operand(0);
        
      
        if (base1 != base2) {
            if ((dynamic_cast<GlobalVariable*>(base1) && dynamic_cast<GlobalVariable*>(base2)) ||
                (dynamic_cast<AllocaInst*>(base1) && dynamic_cast<AllocaInst*>(base2))) {
                return false;
            }
        }
        
        else {
            // 所有索引都是常量且不同，则不别名
            bool all_const = true;
            bool same_indices = true;
            
            if (gep1->get_num_operand() == gep2->get_num_operand()) {
                for (size_t i = 1; i < gep1->get_num_operand(); i++) {
                    auto idx1 = dynamic_cast<ConstantInt*>(gep1->get_operand(i));
                    auto idx2 = dynamic_cast<ConstantInt*>(gep2->get_operand(i));
                    
                    if (!idx1 || !idx2) {
                        all_const = false;
                        break;
                    }
                    
                    if (idx1->get_value() != idx2->get_value()) {
                        same_indices = false;
                    }
                }
                
                if (all_const && !same_indices) {
                    return false;
                }
            }
        }
    }
    
    //可能别名
    return true;
}

int LoopInvariantCodeMotion::run_on_loop(std::shared_ptr<Loop> loop) {
    std::set<Value *> loop_instructions;
    std::set<Value *> updated_values;
    std::set<BasicBlock *> loop_blocks;
    bool contains_impure_call = false;
    
    // 清空之前的数据
    loop_invariant_instrs_.clear();
    load_addresses_.clear();
    
    // 收集循环信息
    collect_loop_info(loop, loop_instructions, updated_values, loop_blocks, contains_impure_call);
    
    // 保守处理
    if (contains_impure_call) {
        return 0;
    }
    
    // 找出所有循环不变指令
    std::vector<Instruction *> candidates;
    bool changed;
    
    // 多轮迭代，直到没有新的循环不变指令
    do {
        changed = false;
        
        for (auto &bb : loop->get_blocks()) {
            for (auto &instr : bb->get_instructions()) {
                // 跳过已经识别为循环不变的指令
                if (loop_invariant_instrs_.count(&instr)) {
                    continue;
                }
                
                // 确保指令不使用在其他循环中定义的值
                bool uses_other_loop_value = false;
                for (auto &op : instr.get_operands()) {
                    if (auto op_inst = dynamic_cast<Instruction*>(op)) {
                        // 检查操作数是否在其他循环中定义
                        for (auto &other_loop : loop_detection_->get_loops()) {
                            if (other_loop != loop && other_loop->contains(op_inst->get_parent())) {
                                // 其他循环不是当前循环的子循环，则不能外提
                                bool is_sub_loop = false;
                                for (auto &sub : loop->get_sub_loops()) {
                                    if (sub == other_loop) {
                                        is_sub_loop = true;
                                        break;
                                    }
                                }
                                if (!is_sub_loop) {
                                    uses_other_loop_value = true;
                                    break;
                                }
                            }
                        }
                        if (uses_other_loop_value) break;
                    }
                }
                
                if (uses_other_loop_value) {
                    continue;
                }
                
                if (is_safe_to_hoist(&instr, loop_instructions, updated_values, 
                                   loop_blocks)) {
                    candidates.push_back(&instr);
                    loop_invariant_instrs_.insert(&instr);
                    changed = true;
                    
                    LOG_DEBUG << "Found loop invariant: " << instr.print();
                }
            }
        }
    } while (changed);
    
    // 若没有可以外提的指令，直接返回
    if (candidates.empty()) {
        return 0;
    }
    
   // LOG_INFO << "Loop " << loop->get_header()->get_name() 
     //        << ": found " << candidates.size() << " invariant instructions";
    
    // 获取或创建preheader
    BasicBlock *preheader = loop->get_preheader();
    if (!preheader) {
        preheader = create_preheader(loop);
        loop->set_preheader(preheader);
    }
    
    // 将循环不变指令移动到preheader，保持依赖顺序
    std::vector<Instruction *> ordered_candidates = topological_sort(candidates);
    
    for (auto *instr : ordered_candidates) {
       // LOG_DEBUG << "Hoisting: " << instr->print() << " to " << preheader->get_name();
        move_to_preheader(instr, preheader);
    }
    
    return candidates.size();
}

BasicBlock* LoopInvariantCodeMotion::create_preheader(std::shared_ptr<Loop> loop) {
    auto header = loop->get_header();
    auto func = header->get_parent();
    
    // 创建新的preheader基本块
    std::string preheader_name = "loop_" + header->get_name() + "_preheader";
    auto preheader = BasicBlock::create(m_, preheader_name, func);
    
    // 收集所有从循环外到header的边
    std::vector<BasicBlock*> outside_preds;
    for (auto pred : header->get_pre_basic_blocks()) {
        if (!loop->contains(pred)) {
            outside_preds.push_back(pred);
        }
    }
    
    // 如果没有外部前驱，这个循环可能有问题
    if (outside_preds.empty()) {
        LOG_WARNING << "Loop has no outside predecessors: " << header->get_name();
        return nullptr;
    }
    
    for (auto pred : outside_preds) {
        // 获取pred的终结指令并更新其操作数
        auto terminator = pred->get_terminator();
        if (auto br = dynamic_cast<BranchInst*>(terminator)) {
            for (unsigned int i = 0; i < br->get_num_operand(); i++) {
                if (br->get_operand(i) == header) {
                    br->set_operand(i, preheader);
                }
            }
        }
        
        // 更新CFG
        pred->remove_succ_basic_block(header);
        pred->add_succ_basic_block(preheader);
        preheader->add_pre_basic_block(pred);
        header->remove_pre_basic_block(pred);
    }
    
    // 从preheader跳转到header
    BranchInst::create_br(header, preheader);
    preheader->add_succ_basic_block(header);
    header->add_pre_basic_block(preheader);
    
    // 更新header中的phi节点
    for (auto &instr : header->get_instructions()) {
        if (auto phi = dynamic_cast<PhiInst*>(&instr)) {
            // 创建一个新的值在preheader中
            std::vector<std::pair<Value*, BasicBlock*>> values_from_outside;
            
            // 收集所有来自outside_preds的值
            for (unsigned int i = 0; i < phi->get_num_operand(); i += 2) {
                auto bb = static_cast<BasicBlock*>(phi->get_operand(i + 1));
                if (std::find(outside_preds.begin(), outside_preds.end(), bb) != outside_preds.end()) {
                    values_from_outside.push_back({phi->get_operand(i), bb});
                }
            }
            
            // 如果只有一个值，直接使用
            if (values_from_outside.size() == 1) {
               
                for (auto &[val, bb] : values_from_outside) {
                    phi->remove_phi_operand(bb);
                }
                
                phi->add_operand(values_from_outside[0].first);
                phi->add_operand(preheader);
            } else if (values_from_outside.size() > 1) {
                
                auto new_phi = PhiInst::create_phi(phi->get_type(), preheader);
                for (auto &[val, bb] : values_from_outside) {
                    new_phi->add_operand(val);
                    new_phi->add_operand(bb);
                    phi->remove_phi_operand(bb);
                }
                phi->add_operand(new_phi);
                phi->add_operand(preheader);
            }
        }
    }
    
    return preheader;
}

void LoopInvariantCodeMotion::move_to_preheader(Instruction *instr, BasicBlock *preheader) {
   
    auto orig_bb = instr->get_parent();
    orig_bb->get_instructions().remove(instr);
    
   
    auto &instrs = preheader->get_instructions();
    auto terminator = preheader->get_terminator();
    
    if (terminator) {
       //手动查找
        auto term_it = instrs.end();
        for (auto it = instrs.begin(); it != instrs.end(); ++it) {
            if (&*it == terminator) {
                term_it = it;
                break;
            }
        }
        
        if (term_it != instrs.end()) {
            instrs.insert(term_it, instr);
        } else {
            // 如果没找到，添加到末尾
            preheader->add_instruction(instr);
        }
    } else {
        // 直接添加到末尾
        preheader->add_instruction(instr);
    }
    
    // 更新指令的parent
    instr->set_parent(preheader);
}

std::vector<Instruction*> LoopInvariantCodeMotion::topological_sort(
    const std::vector<Instruction*> &instrs) {
    
    // 构建依赖图
    std::unordered_map<Instruction*, std::set<Instruction*>> deps;
    std::unordered_map<Instruction*, int> in_degree;
    
    for (auto instr : instrs) {
        deps[instr] = std::set<Instruction*>();
        in_degree[instr] = 0;
    }
    
    // 计算依赖关系
    for (auto instr : instrs) {
        for (auto op : instr->get_operands()) {
            if (auto dep_instr = dynamic_cast<Instruction*>(op)) {
                if (std::find(instrs.begin(), instrs.end(), dep_instr) != instrs.end()) {
                    deps[dep_instr].insert(instr);
                    in_degree[instr]++;
                }
            }
        }
    }
    
    // 拓扑排序
    std::queue<Instruction*> worklist;
    std::vector<Instruction*> result;
    
    for (auto instr : instrs) {
        if (in_degree[instr] == 0) {
            worklist.push(instr);
        }
    }
    
    while (!worklist.empty()) {
        auto instr = worklist.front();
        worklist.pop();
        result.push_back(instr);
        
        for (auto dep : deps[instr]) {
            in_degree[dep]--;
            if (in_degree[dep] == 0) {
                worklist.push(dep);
            }
        }
    }
    
    return result;
}