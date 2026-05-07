#include "../../include/ir/live_analysis.h"
#include "../../debug.h"
#include <algorithm>
#include <set>

namespace llvm_ir {

// 辅助：获取一条指令的def/use集合
static void get_def_use(Instruction* inst, std::unordered_set<Value*>& def, std::unordered_set<Value*>& use, Function* func, const std::unordered_set<Value*>& alloca_pointers) {
    // SSA: 如果指令有定义（非void类型），则指令本身是定义
    // 但是要跳过alloca产生的静态指针
    if (inst->type != llvm_ir::Type::Void && alloca_pointers.count(inst) == 0) {
        // 如果是moveinst，则需要将temp_verg_value也加入def
        // if (auto* move_inst = dynamic_cast<MoveInst*>(inst)) {
        //     def.insert(move_inst->get_value_ptr() ? move_inst->get_value_ptr() : move_inst);
        // } else {
        //     def.insert(inst);
        // }
        def.insert(inst->get_value_ptr());
    }
    
    // 处理操作数（使用的变量）
    for (auto* op : inst->operands) {
        if (!op) continue;
        
        // 跳过常量（它们不是活跃变量）
        if (dynamic_cast<Constant*>(op)) continue;
        if (dynamic_cast<UndefValue*>(op)) continue;
        
        // 跳过alloca产生的静态指针（不为其分配寄存器）
        if (alloca_pointers.count(op) > 0) continue;
        
        // 所有非常量、非alloca指针的Value都应该被考虑为use
        // 这包括：Instruction*（其他指令的结果）和函数形参
        use.insert(op);
    }
    // if (inst->parent->label == "ifElseIf_label_18") {
    //     std::cout << "process inst: " << inst->toString() << std::endl;
    //     std::cout << "  def: ";
    //     for (auto* v : def) {
    //         std::cout << v->toString() << "; ";
    //     }
    //     std::cout << "inst use: " << std::endl;
    //     for (auto* v : inst->getUses()) {
    //         std::cout << v->toString() << "; ";
    //     }
    //     std::cout << std::endl;
    // }
}

LiveInfo live_variable_analysis(Function* func) {
    LiveInfo info;
    if (!func) return info; // 防止空指针
    
    // 0. 预处理：识别所有alloca产生的静态指针
    std::unordered_set<Value*> alloca_pointers;
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue;
        
        for (auto& inst_ptr : bb->instructions) {
            Instruction* inst = inst_ptr.get();
            if (!inst) continue;
            
            // 检查是否为alloca指令
            if (auto* alloca_inst = dynamic_cast<llvm_ir::AllocaInst*>(inst)) {
                alloca_pointers.insert(alloca_inst);
            }
        }
    }
    
    // 1. 预处理每个基本块的def/use集合
    std::unordered_map<BasicBlock*, std::unordered_set<Value*>> def, use;
    
    // 预分配空间以减少重新分配
    def.reserve(func->basicBlocks.size());
    use.reserve(func->basicBlocks.size());
    
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue; // 防止空指针
        
        std::unordered_set<Value*> def_set, use_set;
        
        // 预分配空间
        def_set.reserve(bb->instructions.size());
        use_set.reserve(bb->instructions.size() * 2); // 假设每条指令平均2个操作数
        
        // 处理普通指令
        for (auto& inst_ptr : bb->instructions) {
            Instruction* inst = inst_ptr.get();
            if (!inst) continue; // 防止空指针
            
            std::unordered_set<Value*> inst_def, inst_use;
            get_def_use(inst, inst_def, inst_use, func, alloca_pointers);
            

            // 只为特定指令输出调试信息
            if (inst->name.find("t_9_phi_0") != std::string::npos) {
                std::cout << "process inst: " << inst->toString() << std::endl;
                std::cout << "  def: ";
                for (auto* v : inst_def) {
                    std::cout << v->toString() << "; ";
                }
                std::cout << std::endl;
                std::cout << "  use: ";
                for (auto* v : inst_use) {
                    std::cout << v->toString() << "; ";
                }
                std::cout << std::endl;
            }


            // 优化：直接合并而不是逐个插入
            for (auto* v : inst_use) {
                if (v && def_set.count(v) == 0) use_set.insert(v);
            }
            for (auto* v : inst_def) {
                if (v) def_set.insert(v);
            }
        }
        
        // 使用移动语义避免拷贝
        def[bb] = std::move(def_set);
        use[bb] = std::move(use_set);
    }
    
#ifdef LIVEDEBUG
    std::cout<<"reach stage1"<<"\n";
#endif

    // 2. 迭代求解in/out集合
    bool changed = true;
    int iteration = 0;
    const int MAX_ITERATIONS = 1000; // 防止死循环
    
    // 预分配info的map空间
    info.in.reserve(func->basicBlocks.size());
    info.out.reserve(func->basicBlocks.size());
    
    // 特殊处理：将所有函数形参加入入口基本块的liveIn集合
    if (!func->basicBlocks.empty() && func->basicBlocks[0]) {
        BasicBlock* entry_bb = func->basicBlocks[0].get();
        std::unordered_set<Value*>& entry_live_in = info.in[entry_bb];
        
        // 将所有函数形参加入入口基本块的liveIn
        for (auto* param : func->parameters) {
            if (param) {
                entry_live_in.insert(param);
#ifdef LIVEDEBUG
                std::cout << "[DEBUG] Adding function parameter " << param->name << " to entry block liveIn" << std::endl;
#endif
            }
        }
    }
    
    while (changed && iteration < MAX_ITERATIONS) {
        iteration++;
#ifdef LIVEDEBUG
        if (iteration % 10 == 0) {
            std::cout << "  [DEBUG] Iteration " << iteration << std::endl;
        }
#endif
        changed = false;
        
        for (auto& bb_ptr : func->basicBlocks) {
            BasicBlock* bb = bb_ptr.get();
            if (!bb) {
#ifdef LIVEDEBUG
                std::cout << "  [DEBUG] Skipping null basic block" << std::endl;
#endif
                continue; // 防止空指针
            }
            
            auto& in_b = info.in[bb];
            auto& out_b = info.out[bb];
            
            // out[B] = ∪ in[S] (S为后继)
            std::unordered_set<Value*> new_out;
            new_out.reserve(bb->successors.size() * 10); // 预估每个后继平均10个活跃变量
            
            for (size_t i = 0; i < bb->successors.size(); ++i) {
                auto* succ = bb->successors[i];
                if (!succ) continue; // 防止空指针
                
                // 安全地访问 info.in[succ]，如果不存在则创建空集合
                auto it = info.in.find(succ);
                if (it != info.in.end()) {
                    const auto& in_succ = it->second; // 使用const引用避免拷贝
                    new_out.insert(in_succ.begin(), in_succ.end());
                } else {
                    // 如果不存在，创建一个空的集合
                    info.in[succ] = std::unordered_set<Value*>();
                }
            }
            
            // in[B] = use[B] ∪ (out[B] - def[B])
            std::unordered_set<Value*> new_in;
            const auto& use_set = use[bb]; // 使用const引用
            const auto& def_set = def[bb]; // 使用const引用
            
            new_in.reserve(use_set.size() + new_out.size());
            
            // 先添加use集合
            for (auto* v : use_set) {
                if (!v) continue;
                if (dynamic_cast<Constant*>(v)) continue;
                if (dynamic_cast<UndefValue*>(v)) continue;
                new_in.insert(v);
            }
            
            // 再添加out-def
            for (auto* v : new_out) {
                if (!v) continue;
                if (dynamic_cast<Constant*>(v)) continue;
                if (dynamic_cast<UndefValue*>(v)) continue;
                if (def_set.count(v) == 0) new_in.insert(v);
            }
            
            // 优化：直接比较集合大小和内容，避免不必要的拷贝
            if (new_in.size() != in_b.size() || new_out.size() != out_b.size() || 
                new_in != in_b || new_out != out_b) {
#ifdef LIVEDEBUG
                std::cout << "  [DEBUG] Block " << bb->label << " changed in iteration " << iteration << std::endl;
                std::cout << "    old in_b: ";
                for (auto* v : in_b) if (v) std::cout << v->name << " ";
                std::cout << "\n    new in_b: ";
                for (auto* v : new_in) if (v) std::cout << v->name << " ";
                std::cout << "\n    old out_b: ";
                for (auto* v : out_b) if (v) std::cout << v->name << " ";
                std::cout << "\n    new out_b: ";
                for (auto* v : new_out) if (v) std::cout << v->name << " ";
                std::cout << std::endl;
#endif
                // 使用移动语义避免拷贝
                in_b = std::move(new_in);
                out_b = std::move(new_out);
                changed = true;
#ifdef LIVEDEBUG
                std::cout << "  [DEBUG] Block " << bb->label << " changed in iteration " << iteration << std::endl;
#endif
            }
        }
    }
    
    if (iteration >= MAX_ITERATIONS) {
        // std::cerr << "[WARNING] Live variable analysis reached maximum iterations (" << MAX_ITERATIONS << "), may not have converged!\n";
        // 输出所有块的in/out信息，便于调试
        for (auto& bb_ptr : func->basicBlocks) {
            BasicBlock* bb = bb_ptr.get();
            if (!bb) continue;
            std::cerr << "  Block " << bb->label << ":\n    in: ";
            for (auto* v : info.in[bb]) if (v) std::cerr << v->name << " ";
            std::cerr << "\n    out: ";
            for (auto* v : info.out[bb]) if (v) std::cerr << v->name << " ";
            std::cerr << std::endl;
        }
    } else {
#ifdef LIVEDEBUG
        std::cout << "  [DEBUG] Live variable analysis converged after " << iteration << " iterations" << std::endl;
#endif
    }
    
#ifdef LIVEDEBUG
    std::cout<<"reach stage2"<<"\n";
#endif

    // 3. 块内逆序传播，得到每条指令的live_in/live_out
    // 预分配指令级map的空间
    size_t total_instructions = 0;
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue;
        total_instructions += bb->instructions.size();
    }
    info.live_in.reserve(total_instructions);
    info.live_out.reserve(total_instructions);
    
    for (auto& bb_ptr : func->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!bb) continue; // 防止空指针
        
        std::vector<Instruction*> insts;
        insts.reserve(bb->instructions.size());
        
        // 只处理普通指令
        for (auto& inst_ptr : bb->instructions) {
            Instruction* inst = inst_ptr.get();
            if (inst) insts.push_back(inst);
        }
        
        std::unordered_set<Value*> live;
        const auto& out_set = info.out[bb]; // 使用const引用
        live.reserve(out_set.size());
        
        for (auto* v : out_set) {
            if (!v) continue;
            if (dynamic_cast<Constant*>(v)) continue;
            if (dynamic_cast<UndefValue*>(v)) continue;
            live.insert(v);
        }
        
        for (int i = (int)insts.size() - 1; i >= 0; --i) {
            Instruction* inst = insts[i];
            if (!inst) continue; // 防止空指针
            
            info.live_out[inst] = live;
            std::unordered_set<Value*> def, use;
            get_def_use(inst, def, use, func, alloca_pointers);
            
            // live_in = use ∪ (live_out - def)
            std::unordered_set<Value*> live_in;
            live_in.reserve(use.size() + live.size());
            
            for (auto* v : use) {
                if (!v) continue;
                if (dynamic_cast<Constant*>(v)) continue;
                if (dynamic_cast<UndefValue*>(v)) continue;
                live_in.insert(v);
            }
            for (auto* v : live) {
                if (!v) continue;
                if (dynamic_cast<Constant*>(v)) continue;
                if (dynamic_cast<UndefValue*>(v)) continue;
                if (def.count(v) == 0) live_in.insert(v);
            }
            info.live_in[inst] = std::move(live_in);
            live = info.live_in[inst]; // 使用移动后的结果
        }
    }
    return info;
}

}
