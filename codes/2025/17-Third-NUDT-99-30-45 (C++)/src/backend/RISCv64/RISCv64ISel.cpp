#include "RISCv64ISel.h"
#include "IR.h"  // For GlobalValue
#include <stdexcept>
#include <set>
#include <functional>
#include <cmath>
#include <limits>
#include <iostream>

namespace sysy {

// DAG节点定义 (内部实现)
struct RISCv64ISel::DAGNode {
    enum NodeKind {
        ARGUMENT, 
        CONSTANT,       // 整数或地址常量
        LOAD, 
        STORE, 
        BINARY,         // 整数二元运算
        CALL, 
        RETURN, 
        BRANCH, 
        ALLOCA_ADDR, 
        UNARY,          // 整数一元运算
        MEMSET, 
        GET_ELEMENT_PTR,
        FP_CONSTANT,    // 浮点常量
        FBINARY,        // 浮点二元运算 (如 FADD, FSUB, FCMP)
        FUNARY,         // 浮点一元运算 (如 FCVT, FNEG)
    };
    NodeKind kind;
    Value* value = nullptr;
    std::vector<DAGNode*> operands;
    std::vector<DAGNode*> users;
    DAGNode(NodeKind k) : kind(k) {}
};

RISCv64ISel::RISCv64ISel() : vreg_counter(0), local_label_counter(0) {}

// 为一个IR Value获取或分配一个新的虚拟寄存器
unsigned RISCv64ISel::getVReg(Value* val) {
    if (!val) {
        throw std::runtime_error("Cannot get vreg for a null Value.");
    }
    if (vreg_map.find(val) == vreg_map.end()) {
        if (vreg_counter == 0) {
            vreg_counter = 1; // vreg 0 保留
        }
        unsigned new_vreg = vreg_counter++;
        vreg_map[val] = new_vreg;
        vreg_to_value_map[new_vreg] = val;
        vreg_type_map[new_vreg] = val->getType();
    }
    return vreg_map.at(val);
}

unsigned RISCv64ISel::getNewVReg(Type* type) {
    unsigned new_vreg = vreg_counter++;
    vreg_type_map[new_vreg] = type; // 记录这个新vreg的类型
    return new_vreg;
}

// 主入口函数
std::unique_ptr<MachineFunction> RISCv64ISel::runOnFunction(Function* func) {
    F = func;
    if (!F) return nullptr;
    MFunc = std::make_unique<MachineFunction>(F, this);
    vreg_map.clear();
    bb_map.clear();
    vreg_counter = 0;
    local_label_counter = 0;

    select();

    return std::move(MFunc);
}

// 指令选择主流程
void RISCv64ISel::select() {
    // 遍历基本块，为它们创建对应的MachineBasicBlock
    bool is_first_block = true;
    for (const auto& bb_ptr : F->getBasicBlocks()) {
        std::string mbb_name;
        if (is_first_block) {
            // 对于函数的第一个基本块，其标签必须与函数名完全相同
            mbb_name = F->getName();
            is_first_block = false;
        } else {
            // 对于后续的基本块，继续使用它们在IR中的原始名称
            mbb_name = bb_ptr->getName();
        }
        auto mbb = std::make_unique<MachineBasicBlock>(mbb_name, MFunc.get());
        bb_map[bb_ptr.get()] = mbb.get();
        MFunc->addBlock(std::move(mbb));
    }
    
    // 遍历Argument对象，为它们分配虚拟寄存器。
    if (F) {
        for (Argument* arg : F->getArguments()) {
            // getVReg会为每个代表参数的Argument对象在vreg_map中创建一个条目。
            // 这样，当后续的store指令使用这个参数时，就能找到对应的vreg。
            getVReg(arg);
        }
    }

    // 仅当函数满足特定条件时，才需要保存参数寄存器,应用更精细的过滤规则
    // 1. 函数包含call指令 (非叶子函数): 参数寄存器(a0-a7)是调用者保存的，
    //    call指令可能会覆盖这些寄存器，因此必须保存。
    // 2. 函数包含alloca指令 (需要栈分配)。
    // 3. 函数的指令数量超过一个阈值（如20），意味着它是一个复杂的叶子函数，
    //    为安全起见，保存其参数。
    // 简单的叶子函数 (如min) 则可以跳过这个步骤进行优化。
    auto shouldSaveArgs = [](Function* func) {
        if (!func) return false;
        int instruction_count = 0;
        for (const auto& bb : func->getBasicBlocks()) {
            for (const auto& inst : bb->getInstructions()) {
                if (dynamic_cast<CallInst*>(inst.get()) || dynamic_cast<AllocaInst*>(inst.get())) {
                    return true; // 发现call或alloca，立即返回true
                }
                instruction_count++;
            }
        }
        // 如果没有call或alloca，则检查指令数量
        return instruction_count > 45;
    };

    if (optLevel > 0 && shouldSaveArgs(F)) {
        if (F && !F->getBasicBlocks().empty()) {
            // 定位到第一个MachineBasicBlock，也就是函数入口
            BasicBlock* first_ir_block = F->getBasicBlocks_NoRange().front().get();
            CurMBB = bb_map.at(first_ir_block);

            int int_arg_idx = 0;
            int fp_arg_idx = 0;

            for (Argument* arg : F->getArguments()) {
                Type* arg_type = arg->getType();

                // --- 处理整数/指针参数 ---
                if (!arg_type->isFloat() && int_arg_idx < 8) {
                    // 1. 获取参数原始的、将被预着色为 a0-a7 的 vreg
                    unsigned original_vreg = getVReg(arg);

                    // 2. 创建一个新的、安全的 vreg 来持有参数的值
                    unsigned saved_vreg = getNewVReg(arg_type);

                    // 3. 生成 mv saved_vreg, original_vreg 指令
                    auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                    mv->addOperand(std::make_unique<RegOperand>(saved_vreg));
                    mv->addOperand(std::make_unique<RegOperand>(original_vreg));
                    CurMBB->addInstruction(std::move(mv));

                    MFunc->addProtectedArgumentVReg(saved_vreg);
                    // 4.【关键】更新vreg映射表，将arg的vreg指向新的、安全的vreg
                    //    这样，后续所有对该参数的 getVReg(arg) 调用都会自动获得 saved_vreg，
                    //    使得函数体内的代码都使用这个被保存过的值。
                    vreg_map[arg] = saved_vreg;
                    int_arg_idx++;
                }
                // --- 处理浮点参数 ---
                else if (arg_type->isFloat() && fp_arg_idx < 8) {
                    unsigned original_vreg = getVReg(arg);
                    unsigned saved_vreg = getNewVReg(arg_type);

                    // 对于浮点数，使用 fmv.s 指令
                    auto fmv = std::make_unique<MachineInstr>(RVOpcodes::FMV_S);
                    fmv->addOperand(std::make_unique<RegOperand>(saved_vreg));
                    fmv->addOperand(std::make_unique<RegOperand>(original_vreg));
                    CurMBB->addInstruction(std::move(fmv));

                    MFunc->addProtectedArgumentVReg(saved_vreg);
                    vreg_map[arg] = saved_vreg;
                    fp_arg_idx++;
                }
                // 对于栈传递的参数，则无需处理
            }
        }
    }

    // 遍历基本块，进行指令选择
    for (const auto& bb_ptr : F->getBasicBlocks()) {
        selectBasicBlock(bb_ptr.get());
    }
    
    // 链接MachineBasicBlock的前驱和后继
    for (const auto& bb_ptr : F->getBasicBlocks()) {
        CurMBB = bb_map.at(bb_ptr.get());
        for (auto succ : bb_ptr->getSuccessors()) {
            CurMBB->successors.push_back(bb_map.at(succ));
        }
        for (auto pred : bb_ptr->getPredecessors()) {
            CurMBB->predecessors.push_back(bb_map.at(pred));
        }
    }
}

// 处理单个基本块
void RISCv64ISel::selectBasicBlock(BasicBlock* bb) {
    CurMBB = bb_map.at(bb);
    auto dag = build_dag(bb);

    if (DEBUG) { // 使用 DEBUG 宏或变量来控制是否打印
        print_dag(dag, bb->getName());
    }

    std::map<Value*, DAGNode*> value_to_node;
    for(const auto& node : dag) {
        if (node->value) {
            value_to_node[node->value] = node.get();
        }
    }
    
    std::set<DAGNode*> selected_nodes;
    std::function<void(DAGNode*)> select_recursive = 
        [&](DAGNode* node) {
        if (DEEPDEBUG) {
            std::cout << "[DEEPDEBUG] select_recursive: Visiting node with kind: " << node->kind 
                    << " (Value: " << (node->value ? node->value->getName() : "null") << ")" << std::endl;
        }
        if (!node || selected_nodes.count(node)) return;
        for (auto operand : node->operands) {
            select_recursive(operand);
        }
        selectNode(node);
        selected_nodes.insert(node);
    };
    
    for (const auto& inst_ptr : bb->getInstructions()) {
        DAGNode* node_to_select = nullptr;
        auto it = value_to_node.find(inst_ptr.get());
        if (it != value_to_node.end()) {
            node_to_select = it->second;
        } else {
        for(const auto& node : dag) {
            if(node->value == inst_ptr.get()) {
                node_to_select = node.get();
            break;
            }
        }
    }
    if(node_to_select) {
            select_recursive(node_to_select);
        }
    }
}

// 核心函数：为DAG节点选择并生成MachineInstr (已修复和增强的完整版本)
void RISCv64ISel::selectNode(DAGNode* node) {
    // 调用者（select_recursive）已经保证了操作数节点会先于当前节点被选择。
    // 因此，这里我们只处理当前节点。

    switch (node->kind) {
        // “延迟物化”（Late Materialization）思想。
        // 这三种节点仅作为标记，不直接生成指令。它们的目的是在DAG中保留类型信息。
        // 加载其值的责任，被转移给了使用它们的父节点（如STORE, BINARY等）。
        case DAGNode::ARGUMENT:
        case DAGNode::CONSTANT:
        case DAGNode::ALLOCA_ADDR: {
            if (node->value) {
                // GlobalValue objects (global variables) should not get virtual registers
                // since they represent memory addresses, not register-allocated values
                if (dynamic_cast<GlobalValue*>(node->value) == nullptr) {
                    // 确保它有一个关联的虚拟寄存器即可，不生成代码。
                    getVReg(node->value);
                }
            }
            break;
        }
            
        
        case DAGNode::FP_CONSTANT: {
            // RISC-V没有直接加载浮点立即数的指令
            // 标准做法是：1. 将浮点数的32位二进制表示加载到一个整数寄存器
            //            2. 使用 fmv.w.x 指令将位模式从整数寄存器移动到浮点寄存器
            auto const_val = dynamic_cast<ConstantValue*>(node->value);
            auto float_vreg = getVReg(const_val);
            auto temp_int_vreg = getNewVReg(Type::getIntType()); // 临时整数虚拟寄存器
            
            float f_val = const_val->getFloat();
            // 使用 reinterpret_cast 获取浮点数的32位二进制表示
            uint32_t float_bits = *reinterpret_cast<uint32_t*>(&f_val);

            // 1. li temp_int_vreg, float_bits
            auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
            li->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
            li->addOperand(std::make_unique<ImmOperand>(float_bits));
            CurMBB->addInstruction(std::move(li));

            // 2. fmv.w.x float_vreg, temp_int_vreg
            auto fmv = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
            fmv->addOperand(std::make_unique<RegOperand>(float_vreg));
            fmv->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
            CurMBB->addInstruction(std::move(fmv));
            break;
        }

        case DAGNode::LOAD: {
            auto dest_vreg = getVReg(node->value);
            Value* ptr_val = node->operands[0]->value;

            // 1. 获取加载结果的类型 (即这个LOAD指令自身的类型)
            Type* loaded_type = node->value->getType();
            
            // 2. 根据类型选择正确的伪指令或真实指令操作码
            RVOpcodes frame_opcode;
            RVOpcodes real_opcode;
            if (loaded_type->isPointer()) {
                frame_opcode = RVOpcodes::FRAME_LOAD_D;
                real_opcode = RVOpcodes::LD;
            } else if (loaded_type->isFloat()) {
                frame_opcode = RVOpcodes::FRAME_LOAD_F;
                real_opcode = RVOpcodes::FLW;
            } else { // 默认为整数
                frame_opcode = RVOpcodes::FRAME_LOAD_W;
                real_opcode = RVOpcodes::LW;
            }

            if (auto alloca = dynamic_cast<AllocaInst*>(ptr_val)) {
                // 3. 创建使用新的、区分宽度的伪指令
                auto instr = std::make_unique<MachineInstr>(frame_opcode);
                instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca)));
                CurMBB->addInstruction(std::move(instr));

            } else if (auto global = dynamic_cast<GlobalValue*>(ptr_val)) {
                // 对于全局变量，先用 la 加载其地址
                auto addr_vreg = getNewVReg(Type::getPointerType(global->getType()));
                auto la = std::make_unique<MachineInstr>(RVOpcodes::LA);
                la->addOperand(std::make_unique<RegOperand>(addr_vreg));
                la->addOperand(std::make_unique<LabelOperand>(global->getName()));
                CurMBB->addInstruction(std::move(la));

                // 然后根据类型使用 ld 或 lw 加载其值
                auto load_instr = std::make_unique<MachineInstr>(real_opcode); 
                load_instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                load_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(addr_vreg),
                    std::make_unique<ImmOperand>(0)
                ));
                CurMBB->addInstruction(std::move(load_instr));

            } else {
                // 对于已经在虚拟寄存器中的指针地址，直接通过该地址加载
                auto ptr_vreg = getVReg(ptr_val);
                
                // 根据类型使用 ld 或 lw
                auto load_instr = std::make_unique<MachineInstr>(real_opcode); 
                load_instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                load_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(ptr_vreg),
                    std::make_unique<ImmOperand>(0)
                ));
                CurMBB->addInstruction(std::move(load_instr));
            }
            break;
        }
        
        case DAGNode::STORE: {
            Value* val_to_store = node->operands[0]->value;
            Value* ptr_val = node->operands[1]->value;

            // 如果要存储的值是一个常量，就在这里生成 `li` 指令加载它
            if (auto val_const = dynamic_cast<ConstantValue*>(val_to_store)) {
                // 区分整数常量和浮点常量
                if (val_const->isInt()) {
                    auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                    li->addOperand(std::make_unique<RegOperand>(getVReg(val_const)));
                    li->addOperand(std::make_unique<ImmOperand>(val_const->getInt()));
                    CurMBB->addInstruction(std::move(li));
                } else if (val_const->isFloat()) {
                    // 先将浮点数的位模式加载到整数vreg，再用fmv.w.x移到浮点vreg
                    auto temp_int_vreg = getNewVReg(Type::getIntType());
                    auto float_vreg = getVReg(val_const);
                    
                    float f_val = val_const->getFloat();
                    uint32_t float_bits = *reinterpret_cast<uint32_t*>(&f_val);

                    // 1. li temp_int_vreg, float_bits
                    auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                    li->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
                    li->addOperand(std::make_unique<ImmOperand>(float_bits));
                    CurMBB->addInstruction(std::move(li));

                    // 2. fmv.w.x float_vreg, temp_int_vreg
                    auto fmv = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
                    fmv->addOperand(std::make_unique<RegOperand>(float_vreg));
                    fmv->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
                    CurMBB->addInstruction(std::move(fmv));
                }
            }
            auto val_vreg = getVReg(val_to_store);

            // 1. 获取被存储的值的类型
            Type* stored_type = val_to_store->getType();

            // 2. 根据类型选择正确的伪指令或真实指令操作码
            RVOpcodes frame_opcode;
            RVOpcodes real_opcode;
            if (stored_type->isPointer()) {
                frame_opcode = RVOpcodes::FRAME_STORE_D;
                real_opcode = RVOpcodes::SD;
            } else if (stored_type->isFloat()) {
                frame_opcode = RVOpcodes::FRAME_STORE_F;
                real_opcode = RVOpcodes::FSW;
            } else { // 默认为整数
                frame_opcode = RVOpcodes::FRAME_STORE_W;
                real_opcode = RVOpcodes::SW;
            }

            if (auto alloca = dynamic_cast<AllocaInst*>(ptr_val)) {
                // 3. 创建使用新的、区分宽度的伪指令
                auto instr = std::make_unique<MachineInstr>(frame_opcode);
                instr->addOperand(std::make_unique<RegOperand>(val_vreg));
                instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca)));
                CurMBB->addInstruction(std::move(instr));

            } else if (auto global = dynamic_cast<GlobalValue*>(ptr_val)) {
                // 向全局变量存储
                auto addr_vreg = getNewVReg(Type::getIntType());
                auto la = std::make_unique<MachineInstr>(RVOpcodes::LA);
                la->addOperand(std::make_unique<RegOperand>(addr_vreg));
                la->addOperand(std::make_unique<LabelOperand>(global->getName()));
                CurMBB->addInstruction(std::move(la));

                // 根据类型使用 sd 或 sw
                auto store_instr = std::make_unique<MachineInstr>(real_opcode);
                store_instr->addOperand(std::make_unique<RegOperand>(val_vreg));
                store_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(addr_vreg),
                    std::make_unique<ImmOperand>(0)
                ));
                CurMBB->addInstruction(std::move(store_instr));

            } else {
                // 向一个指针（存储在虚拟寄存器中）指向的地址存储
                auto ptr_vreg = getVReg(ptr_val);
                
                // 根据类型使用 sd 或 sw
                auto store_instr = std::make_unique<MachineInstr>(real_opcode);
                store_instr->addOperand(std::make_unique<RegOperand>(val_vreg));
                store_instr->addOperand(std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(ptr_vreg),
                    std::make_unique<ImmOperand>(0)
                ));
                CurMBB->addInstruction(std::move(store_instr));
            }
            break;
        }

        case DAGNode::BINARY: {
            auto bin = dynamic_cast<BinaryInst*>(node->value);
            Value* lhs = bin->getLhs();
            Value* rhs = bin->getRhs();
            
            if (bin->getKind() == BinaryInst::kAdd) {
                Value* base = nullptr;
                Value* offset = nullptr;

                // 扩展基地址的判断，使其可以识别 AllocaInst 或 GlobalValue
                if (dynamic_cast<AllocaInst*>(lhs) || dynamic_cast<GlobalValue*>(lhs)) {
                    base = lhs;
                    offset = rhs;
                } else if (dynamic_cast<AllocaInst*>(rhs) || dynamic_cast<GlobalValue*>(rhs)) {
                    base = rhs;
                    offset = lhs;
                }
                
                // 如果成功匹配到地址计算模式
                if (base) {
                    // 1. 先为偏移量加载常量（如果它是常量的话）
                    if (auto const_offset = dynamic_cast<ConstantValue*>(offset)) {
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(getVReg(const_offset)));
                        li->addOperand(std::make_unique<ImmOperand>(const_offset->getInt()));
                        CurMBB->addInstruction(std::move(li));
                    }
                    
                    // 2. 根据基地址的类型，生成不同的指令来获取基地址
                    auto base_addr_vreg = getNewVReg(Type::getIntType()); // 创建一个新的临时vreg来存放基地址

                    // 情况一：基地址是局部栈变量
                    if (auto alloca_base = dynamic_cast<AllocaInst*>(base)) {
                        auto frame_addr_instr = std::make_unique<MachineInstr>(RVOpcodes::FRAME_ADDR);
                        frame_addr_instr->addOperand(std::make_unique<RegOperand>(base_addr_vreg));
                        frame_addr_instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca_base)));
                        CurMBB->addInstruction(std::move(frame_addr_instr));
                    } 
                    // 情况二：基地址是全局变量
                    else if (auto global_base = dynamic_cast<GlobalValue*>(base)) {
                        auto la_instr = std::make_unique<MachineInstr>(RVOpcodes::LA);
                        la_instr->addOperand(std::make_unique<RegOperand>(base_addr_vreg));
                        la_instr->addOperand(std::make_unique<LabelOperand>(global_base->getName()));
                        CurMBB->addInstruction(std::move(la_instr));
                    }

                    // 3. 生成真正的add指令，计算最终地址（这部分逻辑保持不变）
                    auto final_addr_vreg = getVReg(bin); // 这是整个二元运算的结果vreg
                    auto offset_vreg = getVReg(offset);
                    auto add_instr = std::make_unique<MachineInstr>(RVOpcodes::ADD); // 指针运算是64位
                    add_instr->addOperand(std::make_unique<RegOperand>(final_addr_vreg));
                    add_instr->addOperand(std::make_unique<RegOperand>(base_addr_vreg));
                    add_instr->addOperand(std::make_unique<RegOperand>(offset_vreg));
                    CurMBB->addInstruction(std::move(add_instr));
                    
                    return; // 地址计算处理完毕，直接返回
                }
            }

            // 在BINARY节点内部按需加载常量操作数。
            auto load_val_if_const = [&](Value* val) {
                if (auto c = dynamic_cast<ConstantValue*>(val)) {
                    if (DEBUG) {
                        std::cout << "[DEBUG] selectNode-BINARY: Found constant operand with value " << c->getInt()
                                << ". Generating LI instruction." << std::endl;
                    }
                    auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                    li->addOperand(std::make_unique<RegOperand>(getVReg(c)));
                    li->addOperand(std::make_unique<ImmOperand>(c->getInt()));
                    CurMBB->addInstruction(std::move(li));
                }
            };

            // 检查是否能应用立即数优化。
            bool rhs_is_imm_opt = false;
            if (auto rhs_const = dynamic_cast<ConstantValue*>(rhs)) {
                if (bin->getKind() == BinaryInst::kAdd && rhs_const->getInt() >= -2048 && rhs_const->getInt() < 2048) {
                    rhs_is_imm_opt = true;
                }
            }

            // 仅在不能作为立即数操作数时才需要提前加载。
            load_val_if_const(lhs);
            if (!rhs_is_imm_opt) {
                load_val_if_const(rhs);
            }

            auto dest_vreg = getVReg(bin);
            auto lhs_vreg = getVReg(lhs);

            // 融合 ADDIW 优化。
            if (rhs_is_imm_opt) {
                auto rhs_const = dynamic_cast<ConstantValue*>(rhs);
                auto instr = std::make_unique<MachineInstr>(RVOpcodes::ADDIW);
                instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                instr->addOperand(std::make_unique<ImmOperand>(rhs_const->getInt()));
                CurMBB->addInstruction(std::move(instr));
                return; // 指令已生成，直接返回。
            }
            
            auto rhs_vreg = getVReg(rhs);

            switch (bin->getKind()) {
                case BinaryInst::kAdd: {
                    // 区分指针运算（64位）和整数运算（32位）。
                    RVOpcodes opcode = (lhs->getType()->isPointer() || rhs->getType()->isPointer()) ? RVOpcodes::ADD : RVOpcodes::ADDW;
                    auto instr = std::make_unique<MachineInstr>(opcode);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kSub: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SUBW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kMul: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::MULW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kMulh: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::MULH);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kDiv: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::DIVW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kRem: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::REMW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kSra: {
                    auto rhs_const = dynamic_cast<ConstantInteger*>(rhs);
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SRAIW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<ImmOperand>(rhs_const->getInt()));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kSll: { // 逻辑左移
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SLLW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kSrl: { // 逻辑右移
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SRLW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kICmpEQ: { // 等于 (a == b) -> (subw; seqz)
                    auto sub = std::make_unique<MachineInstr>(RVOpcodes::SUBW);
                    sub->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    sub->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    sub->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(sub));

                    auto seqz = std::make_unique<MachineInstr>(RVOpcodes::SEQZ);
                    seqz->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    seqz->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    CurMBB->addInstruction(std::move(seqz));
                    break;
                }
                case BinaryInst::kICmpNE: { // 不等于 (a != b) -> (subw; snez)
                    auto sub = std::make_unique<MachineInstr>(RVOpcodes::SUBW);
                    sub->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    sub->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    sub->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(sub));

                    auto snez = std::make_unique<MachineInstr>(RVOpcodes::SNEZ);
                    snez->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    snez->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    CurMBB->addInstruction(std::move(snez));
                    break;
                }
                case BinaryInst::kICmpLT: { // 小于 (a < b) -> slt
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SLT);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kICmpGT: { // 大于 (a > b) -> (b < a) -> slt
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SLT);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kICmpLE: { // 小于等于 (a <= b) -> !(b < a) -> (slt; xori)
                    auto slt = std::make_unique<MachineInstr>(RVOpcodes::SLT);
                    slt->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    slt->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    slt->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    CurMBB->addInstruction(std::move(slt));

                    auto xori = std::make_unique<MachineInstr>(RVOpcodes::XORI);
                    xori->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    xori->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    xori->addOperand(std::make_unique<ImmOperand>(1));
                    CurMBB->addInstruction(std::move(xori));
                    break;
                }
                case BinaryInst::kICmpGE: { // 大于等于 (a >= b) -> !(a < b) -> (slt; xori)
                    auto slt = std::make_unique<MachineInstr>(RVOpcodes::SLT);
                    slt->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    slt->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    slt->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(slt));

                    auto xori = std::make_unique<MachineInstr>(RVOpcodes::XORI);
                    xori->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    xori->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    xori->addOperand(std::make_unique<ImmOperand>(1));
                    CurMBB->addInstruction(std::move(xori));
                    break;
                }
                case BinaryInst::kAnd: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::AND);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case BinaryInst::kOr: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::OR);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported binary instruction in ISel");
            }
            break;
        }

        case DAGNode::FBINARY: {
            auto bin = dynamic_cast<BinaryInst*>(node->value);
            auto dest_vreg = getVReg(bin);
            auto lhs_vreg = getVReg(bin->getLhs());
            auto rhs_vreg = getVReg(bin->getRhs());

            switch (bin->getKind()) {
                case Instruction::kFAdd: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FADD_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFSub: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FSUB_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFMul: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FMUL_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFDiv: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FDIV_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }

                // --- 浮点比较指令 ---
                // 注意：比较结果(0或1)写入的是一个通用整数寄存器(dest_vreg)
                case Instruction::kFCmpEQ: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FEQ_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFCmpLT: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FLT_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFCmpLE: {
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FLE_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                // --- 通过交换操作数或组合指令实现其余比较 ---
                case Instruction::kFCmpGT: { // a > b  等价于 b < a
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FLT_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg)); // 操作数交换
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFCmpGE: { // a >= b 等价于 b <= a
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FLE_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(rhs_vreg)); // 操作数交换
                    instr->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFCmpNE: { // a != b 等价于 !(a == b)
                    // 1. 先用 feq.s 比较，结果存入 dest_vreg
                    auto feq = std::make_unique<MachineInstr>(RVOpcodes::FEQ_S);
                    feq->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    feq->addOperand(std::make_unique<RegOperand>(lhs_vreg));
                    feq->addOperand(std::make_unique<RegOperand>(rhs_vreg));
                    CurMBB->addInstruction(std::move(feq));
                    // 2. 再用 seqz 对结果取反 (如果相等(1)，则变0；如果不等(0)，则变1)
                    auto seqz = std::make_unique<MachineInstr>(RVOpcodes::SEQZ);
                    seqz->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    seqz->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    CurMBB->addInstruction(std::move(seqz));
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported float binary instruction in ISel");
            }
            break;
        }

        case DAGNode::UNARY: {
            auto unary = dynamic_cast<UnaryInst*>(node->value);
            auto dest_vreg = getVReg(unary);
            auto src_vreg = getVReg(unary->getOperand());

            switch (unary->getKind()) {
                case UnaryInst::kNeg: { // 取负: 0 - src
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SUBW);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case UnaryInst::kNot: { // 逻辑非: src == 0 ? 1 : 0
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::SEQZ);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported unary instruction in ISel");
            }
            break;
        }

        case DAGNode::FUNARY: {
            auto unary = dynamic_cast<UnaryInst*>(node->value);
            auto dest_vreg = getVReg(unary);
            auto src_vreg = getVReg(unary->getOperand());

            switch (unary->getKind()) {
                case Instruction::kItoF: { // 整数 to 浮点
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FCVT_S_W);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg)); // 目标是浮点vreg
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));  // 源是整数vreg
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kFtoI: { // 浮点 to 整数 (C/C++: 截断)
                    // C/C++ 标准要求向零截断 (truncate), 对应的RISC-V舍入模式是 RTZ (Round Towards Zero).
                    // fcvt.w.s 指令使用 fcsr 中的 frm 字段来决定舍入模式。
                    // 我们需要手动设置 frm=1 (RTZ), 执行转换, 然后恢复 frm=0 (RNE, 默认).

                    // 1. fsrmi x0, 1  (set rounding mode to RTZ)
                    auto fsrmi1 = std::make_unique<MachineInstr>(RVOpcodes::FSRMI);
                    fsrmi1->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    fsrmi1->addOperand(std::make_unique<ImmOperand>(1));
                    CurMBB->addInstruction(std::move(fsrmi1));

                    // 2. fcvt.w.s dest_vreg, src_vreg
                    auto fcvt = std::make_unique<MachineInstr>(RVOpcodes::FCVT_W_S);
                    fcvt->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    fcvt->addOperand(std::make_unique<RegOperand>(src_vreg));
                    CurMBB->addInstruction(std::move(fcvt));

                    // 3. fsrmi x0, 0  (restore rounding mode to RNE)
                    auto fsrmi0 = std::make_unique<MachineInstr>(RVOpcodes::FSRMI);
                    fsrmi0->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    fsrmi0->addOperand(std::make_unique<ImmOperand>(0));
                    CurMBB->addInstruction(std::move(fsrmi0));

                    break;
                }
                case Instruction::kFNeg: { // 浮点取负
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FNEG_S);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                // --- 处理位传送指令 ---
                case Instruction::kBitItoF: { // 整数位模式 -> 浮点寄存器
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg)); // 目标是浮点vreg
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));  // 源是整数vreg
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                case Instruction::kBitFtoI: { // 浮点位模式 -> 整数寄存器
                    auto instr = std::make_unique<MachineInstr>(RVOpcodes::FMV_X_W);
                    instr->addOperand(std::make_unique<RegOperand>(dest_vreg)); // 目标是整数vreg
                    instr->addOperand(std::make_unique<RegOperand>(src_vreg));  // 源是浮点vreg
                    CurMBB->addInstruction(std::move(instr));
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported float unary instruction in ISel");
            }
            break;
        }

        case DAGNode::CALL: {
            auto call = dynamic_cast<CallInst*>(node->value);
            size_t num_operands = node->operands.size();

            // --- 步骤 1: 分配寄存器参数和栈参数 ---
            // 根据RISC-V调用约定，前8个整数/指针参数通过a0-a7传递，
            // 前8个浮点参数通过fa0-fa7传递 (物理寄存器 f10-f17)。其余参数通过栈传递。
            
            int int_reg_idx = 0; // a0-a7 的索引
            int fp_reg_idx = 0;  // fa0-fa7 的索引

            // 用于存储需要通过栈传递的参数
            std::vector<DAGNode*> stack_args;

            for (size_t i = 0; i < num_operands; ++i) {
                DAGNode* arg_node = node->operands[i];
                Value* arg_val = arg_node->value;
                Type* arg_type = arg_val->getType();

                // 判断参数是浮点类型还是整型/指针类型
                if (arg_type->isFloat()) {
                    if (fp_reg_idx < 8) {
                        // --- 处理浮点寄存器参数 (fa0-fa7, 对应物理寄存器 F10-F17) ---
                        auto arg_preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::F10) + fp_reg_idx);
                        fp_reg_idx++;

                        if (auto const_val = dynamic_cast<ConstantValue*>(arg_val)) {
                            // 如果是浮点常量，需要先物化
                            // 1. 获取其32位二进制表示
                            float f_val = const_val->getFloat();
                            uint32_t float_bits = *reinterpret_cast<uint32_t*>(&f_val);
                            // 2. 将位模式加载到一个临时整数寄存器 (使用t0)
                            auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                            li->addOperand(std::make_unique<RegOperand>(PhysicalReg::T0));
                            li->addOperand(std::make_unique<ImmOperand>(float_bits));
                            CurMBB->addInstruction(std::move(li));
                            // 3. 使用fmv.w.x将位模式从整数寄存器移动到目标浮点参数寄存器
                            auto fmv_wx = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
                            fmv_wx->addOperand(std::make_unique<RegOperand>(arg_preg));
                            fmv_wx->addOperand(std::make_unique<RegOperand>(PhysicalReg::T0));
                            CurMBB->addInstruction(std::move(fmv_wx));
                        } else {
                            // 如果已经是虚拟寄存器，直接用 fmv.s 移动
                            auto src_vreg = getVReg(arg_val);
                            auto fmv_s = std::make_unique<MachineInstr>(RVOpcodes::FMV_S);
                            fmv_s->addOperand(std::make_unique<RegOperand>(arg_preg));
                            fmv_s->addOperand(std::make_unique<RegOperand>(src_vreg));
                            CurMBB->addInstruction(std::move(fmv_s));
                        }
                    } else {
                        // 浮点寄存器已用完，放到栈上传递
                        stack_args.push_back(arg_node);
                    }
                } else { // 整数或指针参数
                    if (int_reg_idx < 8) {
                        // --- 处理整数/指针寄存器参数 (a0-a7) ---
                        auto arg_preg = static_cast<PhysicalReg>(static_cast<int>(PhysicalReg::A0) + int_reg_idx);
                        int_reg_idx++;
                        
                        if (arg_node->kind == DAGNode::CONSTANT) {
                            if (auto const_val = dynamic_cast<ConstantValue*>(arg_val)) {
                                auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                                li->addOperand(std::make_unique<RegOperand>(arg_preg));
                                li->addOperand(std::make_unique<ImmOperand>(const_val->getInt()));
                                CurMBB->addInstruction(std::move(li));
                            }
                        } else {
                            auto src_vreg = getVReg(arg_val);
                            auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                            mv->addOperand(std::make_unique<RegOperand>(arg_preg));
                            mv->addOperand(std::make_unique<RegOperand>(src_vreg));
                            CurMBB->addInstruction(std::move(mv));
                        }
                    } else {
                        // 整数寄存器已用完，放到栈上传递
                        stack_args.push_back(arg_node);
                    }
                }
            }

            // --- 步骤 2: 处理所有栈参数 ---
            int stack_space = 0;
            if (!stack_args.empty()) {
                // 计算栈参数所需的总空间，RV64中每个槽位为8字节
                stack_space = stack_args.size() * 8;
                // 根据ABI，为call分配的栈空间需要16字节对齐
                if (stack_space % 16 != 0) {
                    stack_space += 16 - (stack_space % 16);
                }

                // 在栈上分配空间
                if (stack_space > 0) {
                    auto alloc_instr = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                    alloc_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                    alloc_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                    alloc_instr->addOperand(std::make_unique<ImmOperand>(-stack_space));
                    CurMBB->addInstruction(std::move(alloc_instr));
                }

                // 将每个参数存储到栈上对应的位置
                for (size_t i = 0; i < stack_args.size(); ++i) {
                    DAGNode* arg_node = stack_args[i];
                    Value* arg_val = arg_node->value;
                    Type* arg_type = arg_val->getType();
                    int offset = i * 8;

                    unsigned src_vreg;
                    // 如果是常量，先加载到临时vreg
                    if (auto const_val = dynamic_cast<ConstantValue*>(arg_val)) {
                        src_vreg = getNewVReg(arg_type);
                        if(arg_type->isFloat()) {
                            auto temp_int_vreg = getNewVReg(Type::getIntType());
                            float f_val = const_val->getFloat();
                            uint32_t float_bits = *reinterpret_cast<uint32_t*>(&f_val);
                            auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                            li->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
                            li->addOperand(std::make_unique<ImmOperand>(float_bits));
                            CurMBB->addInstruction(std::move(li));
                            auto fmv_wx = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
                            fmv_wx->addOperand(std::make_unique<RegOperand>(src_vreg));
                            fmv_wx->addOperand(std::make_unique<RegOperand>(temp_int_vreg));
                            CurMBB->addInstruction(std::move(fmv_wx));
                        } else {
                            auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                            li->addOperand(std::make_unique<RegOperand>(src_vreg));
                            li->addOperand(std::make_unique<ImmOperand>(const_val->getInt()));
                            CurMBB->addInstruction(std::move(li));
                        }
                    } else {
                        src_vreg = getVReg(arg_val);
                    }
                    
                    // 根据类型选择 fsw (浮点) 或 sd (整型/指针) 存储指令
                    std::unique_ptr<MachineInstr> store_instr;
                    if (arg_type->isFloat()) {
                        store_instr = std::make_unique<MachineInstr>(RVOpcodes::FSW);
                    } else {
                        store_instr = std::make_unique<MachineInstr>(RVOpcodes::SD);
                    }
                    store_instr->addOperand(std::make_unique<RegOperand>(src_vreg));
                    store_instr->addOperand(std::make_unique<MemOperand>(
                        std::make_unique<RegOperand>(PhysicalReg::SP),
                        std::make_unique<ImmOperand>(offset)
                    ));
                    CurMBB->addInstruction(std::move(store_instr));
                }
            }
            
            // --- 步骤 3: 生成CALL指令 ---
            auto call_instr = std::make_unique<MachineInstr>(RVOpcodes::CALL);
            // 如果函数有返回值，将它的目标虚拟寄存器作为第一个操作数
            if (!call->getType()->isVoid()) {
                unsigned dest_vreg = getVReg(call);
                call_instr->addOperand(std::make_unique<RegOperand>(dest_vreg));
            }
            // 将函数名标签作为后续操作数
            call_instr->addOperand(std::make_unique<LabelOperand>(call->getCallee()->getName()));
            // 将所有参数的虚拟寄存器也作为后续操作数，供getInstrUseDef分析
            for (size_t i = 0; i < num_operands; ++i) {
                if (node->operands[i]->kind != DAGNode::CONSTANT && node->operands[i]->kind != DAGNode::FP_CONSTANT) {
                    call_instr->addOperand(std::make_unique<RegOperand>(getVReg(node->operands[i]->value)));
                }
            }
            CurMBB->addInstruction(std::move(call_instr));

            // --- 步骤 4: 处理返回值 ---
            if (!call->getType()->isVoid()) {
                unsigned dest_vreg = getVReg(call);
                if (call->getType()->isFloat()) {
                    // 浮点返回值在 fa0 (物理寄存器 F10)
                    auto fmv_s = std::make_unique<MachineInstr>(RVOpcodes::FMV_S);
                    fmv_s->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    fmv_s->addOperand(std::make_unique<RegOperand>(PhysicalReg::F10)); // fa0
                    CurMBB->addInstruction(std::move(fmv_s));
                } else {
                    // 整数/指针返回值在 a0
                    auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                    mv->addOperand(std::make_unique<RegOperand>(dest_vreg));
                    mv->addOperand(std::make_unique<RegOperand>(PhysicalReg::A0));
                    CurMBB->addInstruction(std::move(mv));
                }
            }
            
            // --- 步骤 5: 回收为栈参数分配的空间 ---
            if (stack_space > 0) {
                auto dealloc_instr = std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                dealloc_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                dealloc_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::SP));
                dealloc_instr->addOperand(std::make_unique<ImmOperand>(stack_space));
                CurMBB->addInstruction(std::move(dealloc_instr));
            }
            break;
        }

        case DAGNode::RETURN: {
            auto ret_inst_ir = dynamic_cast<ReturnInst*>(node->value);
            if (ret_inst_ir && ret_inst_ir->hasReturnValue()) {
                Value* ret_val = ret_inst_ir->getReturnValue();
                Type* ret_type = ret_val->getType();

                if (ret_type->isFloat()) {
                    // --- 处理浮点返回值 ---
                    // 返回值需要被放入 fa0 (物理寄存器 F10)
                    if (auto const_val = dynamic_cast<ConstantValue*>(ret_val)) {
                        // 如果是浮点常量，需要先物化到fa0
                        float f_val = const_val->getFloat();
                        uint32_t float_bits = *reinterpret_cast<uint32_t*>(&f_val);
                        // 1. 加载位模式到临时整数寄存器 (t0)
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(PhysicalReg::T0));
                        li->addOperand(std::make_unique<ImmOperand>(float_bits));
                        CurMBB->addInstruction(std::move(li));
                        // 2. 将位模式从 t0 移动到 fa0
                        auto fmv_wx = std::make_unique<MachineInstr>(RVOpcodes::FMV_W_X);
                        fmv_wx->addOperand(std::make_unique<RegOperand>(PhysicalReg::F10)); // fa0
                        fmv_wx->addOperand(std::make_unique<RegOperand>(PhysicalReg::T0));
                        CurMBB->addInstruction(std::move(fmv_wx));
                    } else {
                        // 如果是vreg，直接用 fmv.s 移动到 fa0
                        auto fmv_s = std::make_unique<MachineInstr>(RVOpcodes::FMV_S);
                        fmv_s->addOperand(std::make_unique<RegOperand>(PhysicalReg::F10)); // fa0
                        fmv_s->addOperand(std::make_unique<RegOperand>(getVReg(ret_val)));
                        CurMBB->addInstruction(std::move(fmv_s));
                    }
                } else {
                    // --- 处理整数/指针返回值 ---
                    // 返回值需要被放入 a0
                    // 在RETURN节点内加载常量返回值
                    if (auto const_val = dynamic_cast<ConstantValue*>(ret_val)) {
                        auto li_instr = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::A0));
                        li_instr->addOperand(std::make_unique<ImmOperand>(const_val->getInt()));
                        CurMBB->addInstruction(std::move(li_instr));
                    } else {
                        auto mv_instr = std::make_unique<MachineInstr>(RVOpcodes::MV);
                        mv_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::A0));
                        mv_instr->addOperand(std::make_unique<RegOperand>(getVReg(ret_val)));
                        CurMBB->addInstruction(std::move(mv_instr));
                    }
                }
            }
            // 函数尾声（epilogue）不由RETURN节点生成，
            // 而是由后续的AsmPrinter或其它Pass统一处理，这是一种常见且有效的模块化设计。
            auto ret_mi = std::make_unique<MachineInstr>(RVOpcodes::RET);
            CurMBB->addInstruction(std::move(ret_mi));
            break;
        }

        case DAGNode::BRANCH: {
        // 处理条件分支
        if (auto cond_br = dynamic_cast<CondBrInst*>(node->value)) {
            Value* condition = cond_br->getCondition();
            auto then_bb_name = cond_br->getThenBlock()->getName();
            auto else_bb_name = cond_br->getElseBlock()->getName();

            // 检查分支条件是否为编译期常量
            if (auto const_cond = dynamic_cast<ConstantValue*>(condition)) {
                // 如果条件是常量，直接生成一个无条件跳转J，而不是BNE
                if (const_cond->getInt() != 0) { // 条件为 true
                    auto j_instr = std::make_unique<MachineInstr>(RVOpcodes::J);
                    j_instr->addOperand(std::make_unique<LabelOperand>(then_bb_name));
                    CurMBB->addInstruction(std::move(j_instr));
                } else { // 条件为 false
                    auto j_instr = std::make_unique<MachineInstr>(RVOpcodes::J);
                    j_instr->addOperand(std::make_unique<LabelOperand>(else_bb_name));
                    CurMBB->addInstruction(std::move(j_instr));
                }
            } 
            // 如果条件不是常量，则执行标准流程
            else {
                // 为条件变量生成加载指令（如果它是常量的话，尽管上面已经处理了）
                // 这一步是为了逻辑完整，以防有其他类型的常量没有被捕获
                if (auto const_val = dynamic_cast<ConstantValue*>(condition)) {
                    auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                    li->addOperand(std::make_unique<RegOperand>(getVReg(const_val)));
                    li->addOperand(std::make_unique<ImmOperand>(const_val->getInt()));
                    CurMBB->addInstruction(std::move(li));
                }

                auto cond_vreg = getVReg(condition);
                
                // 生成 bne cond, zero, then_label (如果cond不为0，则跳转到then)
                auto br_instr = std::make_unique<MachineInstr>(RVOpcodes::BNE);
                br_instr->addOperand(std::make_unique<RegOperand>(cond_vreg));
                br_instr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                br_instr->addOperand(std::make_unique<LabelOperand>(then_bb_name));
                CurMBB->addInstruction(std::move(br_instr));
                
                // 为else分支生成无条件跳转 (后续Pass可以优化掉不必要的跳转)
                auto j_instr = std::make_unique<MachineInstr>(RVOpcodes::J);
                j_instr->addOperand(std::make_unique<LabelOperand>(else_bb_name));
                CurMBB->addInstruction(std::move(j_instr));
            }
        } 
        // 处理无条件分支
        else if (auto uncond_br = dynamic_cast<UncondBrInst*>(node->value)) {
            auto j_instr = std::make_unique<MachineInstr>(RVOpcodes::J);
            j_instr->addOperand(std::make_unique<LabelOperand>(uncond_br->getBlock()->getName()));
            CurMBB->addInstruction(std::move(j_instr));
        }
        break;
    }

        case DAGNode::MEMSET: {
            // Memset的核心展开逻辑在虚拟寄存器层面是正确的，无需修改。
            // 之前的bug是由于其输入（地址、值、大小）的虚拟寄存器未被正确初始化。
            // 在修复了CONSTANT/ALLOCA_ADDR的加载问题后，此处的逻辑现在可以正常工作。

            if (DEBUG) {
                std::cout << "[DEBUG] selectNode-MEMSET: Processing MEMSET node." << std::endl;
            }
            auto memset = dynamic_cast<MemsetInst*>(node->value);
            Value* val_to_set = memset->getValue();
            Value* size_to_set = memset->getSize();
            Value* ptr_val = memset->getPointer();
            auto dest_addr_vreg = getVReg(ptr_val);

            if (auto const_val = dynamic_cast<ConstantValue*>(val_to_set)) {
                if (DEBUG) {
                    std::cout << "[DEBUG] selectNode-MEMSET: Found constant 'value' operand (" << const_val->getInt() << "). Generating LI." << std::endl;
                }
                auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                li->addOperand(std::make_unique<RegOperand>(getVReg(const_val)));
                li->addOperand(std::make_unique<ImmOperand>(const_val->getInt()));
                CurMBB->addInstruction(std::move(li));
            }
            if (auto const_size = dynamic_cast<ConstantValue*>(size_to_set)) {
                if (DEBUG) {
                    std::cout << "[DEBUG] selectNode-MEMSET: Found constant 'size' operand (" << const_size->getInt() << "). Generating LI." << std::endl;
                }
                auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                li->addOperand(std::make_unique<RegOperand>(getVReg(const_size)));
                li->addOperand(std::make_unique<ImmOperand>(const_size->getInt()));
                CurMBB->addInstruction(std::move(li));
            }
            if (auto alloca = dynamic_cast<AllocaInst*>(ptr_val)) {
                if (DEBUG) {
                    std::cout << "[DEBUG] selectNode-MEMSET: Found 'pointer' operand is an AllocaInst. Generating FRAME_ADDR." << std::endl;
                }
                // 生成新的伪指令来获取栈地址
                auto instr = std::make_unique<MachineInstr>(RVOpcodes::FRAME_ADDR);
                instr->addOperand(std::make_unique<RegOperand>(dest_addr_vreg)); // 目标虚拟寄存器
                instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca)));   // 源AllocaInst
                CurMBB->addInstruction(std::move(instr));
            }
            auto r_dest_addr = getVReg(memset->getPointer());
            auto r_num_bytes = getVReg(memset->getSize());
            auto r_value_byte = getVReg(memset->getValue());
            
            // 为memset内部逻辑创建新的临时虚拟寄存器
            Type* ptr_type = Type::getPointerType(Type::getIntType()); 
            auto r_counter = getNewVReg(ptr_type);
            auto r_end_addr = getNewVReg(ptr_type);
            auto r_current_addr = getNewVReg(ptr_type);
             auto r_temp_val = getNewVReg(Type::getIntType());

            // 定义一系列lambda表达式来简化指令创建
            auto add_instr = [&](RVOpcodes op, unsigned rd, unsigned rs1, unsigned rs2) {
                auto i = std::make_unique<MachineInstr>(op);
                i->addOperand(std::make_unique<RegOperand>(rd));
                i->addOperand(std::make_unique<RegOperand>(rs1));
                i->addOperand(std::make_unique<RegOperand>(rs2));
                CurMBB->addInstruction(std::move(i));
            };
            auto addi_instr = [&](RVOpcodes op, unsigned rd, unsigned rs1, int64_t imm) {
                auto i = std::make_unique<MachineInstr>(op);
                i->addOperand(std::make_unique<RegOperand>(rd));
                i->addOperand(std::make_unique<RegOperand>(rs1));
                i->addOperand(std::make_unique<ImmOperand>(imm));
                CurMBB->addInstruction(std::move(i));
            };
            auto store_instr = [&](RVOpcodes op, unsigned src, unsigned base, int64_t off) {
                auto i = std::make_unique<MachineInstr>(op);
                i->addOperand(std::make_unique<RegOperand>(src));
                i->addOperand(std::make_unique<MemOperand>(std::make_unique<RegOperand>(base), std::make_unique<ImmOperand>(off)));
                CurMBB->addInstruction(std::move(i));
            };
             auto branch_instr = [&](RVOpcodes op, unsigned rs1, unsigned rs2, const std::string& label) {
                auto i = std::make_unique<MachineInstr>(op);
                i->addOperand(std::make_unique<RegOperand>(rs1));
                i->addOperand(std::make_unique<RegOperand>(rs2));
                i->addOperand(std::make_unique<LabelOperand>(label));
                CurMBB->addInstruction(std::move(i));
            };
            auto jump_instr = [&](const std::string& label) {
                auto i = std::make_unique<MachineInstr>(RVOpcodes::J);
                i->addOperand(std::make_unique<LabelOperand>(label));
                CurMBB->addInstruction(std::move(i));
            };
            auto label_instr = [&](const std::string& name) {
                auto i = std::make_unique<MachineInstr>(RVOpcodes::LABEL);
                i->addOperand(std::make_unique<LabelOperand>(name));
                CurMBB->addInstruction(std::move(i));
            };

            // 生成唯一的循环标签
            int unique_id = this->local_label_counter++;
            std::string loop_start_label = MFunc->getName() + "_memset_loop_start_" + std::to_string(unique_id);
            std::string loop_end_label = MFunc->getName() + "_memset_loop_end_" + std::to_string(unique_id);
            std::string remainder_label = MFunc->getName() + "_memset_remainder_" + std::to_string(unique_id);
            std::string done_label = MFunc->getName() + "_memset_done_" + std::to_string(unique_id);

            // 构造32位的填充值 (将一个字节复制4次)
            addi_instr(RVOpcodes::ANDI, r_temp_val, r_value_byte, 255);  // 提取低8位: 000000XX
            addi_instr(RVOpcodes::SLLI, r_value_byte, r_temp_val, 8);    // 左移8位: 0000XX00
            add_instr(RVOpcodes::OR, r_temp_val, r_temp_val, r_value_byte); // 合并得到: 0000XXXX
            addi_instr(RVOpcodes::SLLI, r_value_byte, r_temp_val, 16);   // 左移16位: XXXX0000
            add_instr(RVOpcodes::OR, r_temp_val, r_temp_val, r_value_byte); // 合并得到完整的32位值: XXXXXXXX
            
            // 计算循环边界
            add_instr(RVOpcodes::ADD, r_end_addr, r_dest_addr, r_num_bytes);
            auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
            mv->addOperand(std::make_unique<RegOperand>(r_current_addr));
            mv->addOperand(std::make_unique<RegOperand>(r_dest_addr));
            CurMBB->addInstruction(std::move(mv));
            // 计算主循环部分的总字节数 (向下舍入到4的倍数)
            addi_instr(RVOpcodes::ANDI, r_counter, r_num_bytes, -4);
            // 计算主循环的结束地址
            add_instr(RVOpcodes::ADD, r_counter, r_dest_addr, r_counter);
            
            // 4字节主循环
            label_instr(loop_start_label);
            branch_instr(RVOpcodes::BGEU, r_current_addr, r_counter, loop_end_label);
            store_instr(RVOpcodes::SW, r_temp_val, r_current_addr, 0); // 使用 sw (存储字)
            addi_instr(RVOpcodes::ADDI, r_current_addr, r_current_addr, 4); // 步长改为4
            jump_instr(loop_start_label);

            // 1字节收尾循环
            label_instr(loop_end_label);
            label_instr(remainder_label);
            branch_instr(RVOpcodes::BGEU, r_current_addr, r_end_addr, done_label);
            store_instr(RVOpcodes::SB, r_temp_val, r_current_addr, 0);
            addi_instr(RVOpcodes::ADDI, r_current_addr, r_current_addr, 1);
            jump_instr(remainder_label);
            
            label_instr(done_label);
            break;
        }

        case DAGNode::GET_ELEMENT_PTR: {
            auto gep = dynamic_cast<GetElementPtrInst*>(node->value);
            auto result_vreg = getVReg(gep);

            if (optLevel == 0) {
            // --- Step 1: 获取基地址 (此部分逻辑正确，保持不变) ---
            auto base_ptr_node = node->operands[0];
            auto current_addr_vreg = getNewVReg(gep->getType());

            if (auto alloca_base = dynamic_cast<AllocaInst*>(base_ptr_node->value)) {
                auto frame_addr_instr = std::make_unique<MachineInstr>(RVOpcodes::FRAME_ADDR);
                frame_addr_instr->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                frame_addr_instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca_base)));
                CurMBB->addInstruction(std::move(frame_addr_instr));
            } else if (auto global_base = dynamic_cast<GlobalValue*>(base_ptr_node->value)) {
                auto la_instr = std::make_unique<MachineInstr>(RVOpcodes::LA);
                la_instr->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                la_instr->addOperand(std::make_unique<LabelOperand>(global_base->getName()));
                CurMBB->addInstruction(std::move(la_instr));
            } else if (auto const_global_base = dynamic_cast<ConstantVariable*>(base_ptr_node->value)) {
                auto la_instr = std::make_unique<MachineInstr>(RVOpcodes::LA);
                la_instr->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                la_instr->addOperand(std::make_unique<LabelOperand>(const_global_base->getName()));
                CurMBB->addInstruction(std::move(la_instr));
            } else {
                auto base_vreg = getVReg(base_ptr_node->value);
                auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                mv->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                mv->addOperand(std::make_unique<RegOperand>(base_vreg));
                CurMBB->addInstruction(std::move(mv));
            }

            // --- Step 2: 遵循LLVM GEP语义迭代计算地址 ---
            
            // 初始被索引的类型，是基指针指向的那个类型 (例如, [2 x i32])
            Type* current_type = gep->getBasePointer()->getType()->as<PointerType>()->getBaseType();

            // 迭代处理 GEP 的每一个索引
            for (size_t i = 0; i < gep->getNumIndices(); ++i) {
                Value* indexValue = gep->getIndex(i);

                // GEP的第一个索引以整个 `current_type` 的大小为步长。
                // 后续的索引则以 `current_type` 的元素大小为步长。
                // 这一步是计算地址偏移的关键。
                unsigned stride = getTypeSizeInBytes(current_type);
                
                // 如果步长为0（例如对一个void类型或空结构体索引），则不产生任何偏移
                if (stride != 0) {
                    // --- 为当前索引和步长生成偏移计算指令 ---
                    auto offset_vreg = getNewVReg(Type::getIntType());
                    
                    // 处理索引 - 区分常量与动态值
                    unsigned index_vreg;
                    if (auto const_index = dynamic_cast<ConstantValue*>(indexValue)) {
                        // 对于常量索引，直接创建新的虚拟寄存器
                        index_vreg = getNewVReg(Type::getIntType());
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(index_vreg));
                        li->addOperand(std::make_unique<ImmOperand>(const_index->getInt()));
                        CurMBB->addInstruction(std::move(li));
                    } else {
                        // 对于动态索引，使用已存在的虚拟寄存器
                        index_vreg = getVReg(indexValue);
                    }
                    
                    // 优化：如果步长是1，可以直接移动(MV)作为偏移量，无需乘法
                    if (stride == 1) {
                        auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                        mv->addOperand(std::make_unique<RegOperand>(offset_vreg));
                        mv->addOperand(std::make_unique<RegOperand>(index_vreg));
                        CurMBB->addInstruction(std::move(mv));
                    } else {
                        // 步长不为1，需要生成乘法指令
                        auto size_vreg = getNewVReg(Type::getIntType());
                        auto li_size = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li_size->addOperand(std::make_unique<RegOperand>(size_vreg));
                        li_size->addOperand(std::make_unique<ImmOperand>(stride));
                        CurMBB->addInstruction(std::move(li_size));
                        
                        auto mul = std::make_unique<MachineInstr>(RVOpcodes::MULW);
                        mul->addOperand(std::make_unique<RegOperand>(offset_vreg));
                        mul->addOperand(std::make_unique<RegOperand>(index_vreg));
                        mul->addOperand(std::make_unique<RegOperand>(size_vreg));
                        CurMBB->addInstruction(std::move(mul));
                    }

                    // 将计算出的偏移量累加到当前地址上
                    auto add = std::make_unique<MachineInstr>(RVOpcodes::ADD);
                    add->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                    add->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
                    add->addOperand(std::make_unique<RegOperand>(offset_vreg));
                    CurMBB->addInstruction(std::move(add));
                }

                // --- 为下一次迭代更新类型：深入一层 ---
                if (auto array_type = current_type->as<ArrayType>()) {
                    current_type = array_type->getElementType();
                } else if (auto ptr_type = current_type->as<PointerType>()) {
                    // 这种情况不应该在第二次迭代后发生，但为了逻辑健壮性保留
                    current_type = ptr_type->getBaseType();
                }
                // 如果`current_type`已经是i32等基本类型，它会保持不变，
                // 但下一次循环如果还有索引，`getTypeSizeInBytes(i32)`仍然能正确计算步长。
            }
            
            // --- Step 3: 将最终计算出的地址存入GEP的目标虚拟寄存器 (保持不变) ---
            auto final_mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
            final_mv->addOperand(std::make_unique<RegOperand>(result_vreg));
            final_mv->addOperand(std::make_unique<RegOperand>(current_addr_vreg));
            CurMBB->addInstruction(std::move(final_mv));
            break;
        } else {
            // 对于-O1时的处理逻辑
            // --- Step 1: 获取基地址 ---
            auto base_ptr_node = node->operands[0];
            auto base_ptr_val = base_ptr_node->value;
            
            // last_step_addr_vreg 保存上一步计算的结果。
            // 它首先被初始化为GEP的初始基地址。
            unsigned last_step_addr_vreg; 

            if (auto alloca_base = dynamic_cast<AllocaInst*>(base_ptr_val)) {
                last_step_addr_vreg = getNewVReg(gep->getType());
                auto frame_addr_instr = std::make_unique<MachineInstr>(RVOpcodes::FRAME_ADDR);
                frame_addr_instr->addOperand(std::make_unique<RegOperand>(last_step_addr_vreg));
                frame_addr_instr->addOperand(std::make_unique<RegOperand>(getVReg(alloca_base)));
                CurMBB->addInstruction(std::move(frame_addr_instr));
            } else if (auto global_base = dynamic_cast<GlobalValue*>(base_ptr_val)) {
                last_step_addr_vreg = getNewVReg(gep->getType());
                auto la_instr = std::make_unique<MachineInstr>(RVOpcodes::LA);
                la_instr->addOperand(std::make_unique<RegOperand>(last_step_addr_vreg));
                la_instr->addOperand(std::make_unique<LabelOperand>(global_base->getName()));
                CurMBB->addInstruction(std::move(la_instr));
            } else {
                // 对于函数参数或来自其他指令的指针，直接获取其vreg。
                // 这个vreg必须被保护，不能在计算中被修改。
                last_step_addr_vreg = getVReg(base_ptr_val);
            }

            // --- Step 2: 遵循LLVM GEP语义迭代计算地址 ---
            Type* current_type = gep->getBasePointer()->getType()->as<PointerType>()->getBaseType();

            for (size_t i = 0; i < gep->getNumIndices(); ++i) {
                Value* indexValue = gep->getIndex(i);
                unsigned stride = getTypeSizeInBytes(current_type);
                
                if (stride != 0) {
                    // --- 为当前索引和步长生成偏移计算指令 ---
                    auto offset_vreg = getNewVReg(Type::getIntType());
                    
                    unsigned index_vreg;
                    if (auto const_index = dynamic_cast<ConstantValue*>(indexValue)) {
                        index_vreg = getNewVReg(Type::getIntType());
                        auto li = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li->addOperand(std::make_unique<RegOperand>(index_vreg));
                        li->addOperand(std::make_unique<ImmOperand>(const_index->getInt()));
                        CurMBB->addInstruction(std::move(li));
                    } else {
                        index_vreg = getVReg(indexValue);
                    }
                    
                    if (stride == 1) {
                        auto mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
                        mv->addOperand(std::make_unique<RegOperand>(offset_vreg));
                        mv->addOperand(std::make_unique<RegOperand>(index_vreg));
                        CurMBB->addInstruction(std::move(mv));
                    } else {
                        auto size_vreg = getNewVReg(Type::getIntType());
                        auto li_size = std::make_unique<MachineInstr>(RVOpcodes::LI);
                        li_size->addOperand(std::make_unique<RegOperand>(size_vreg));
                        li_size->addOperand(std::make_unique<ImmOperand>(stride));
                        CurMBB->addInstruction(std::move(li_size));
                        
                        auto mul = std::make_unique<MachineInstr>(RVOpcodes::MULW);
                        mul->addOperand(std::make_unique<RegOperand>(offset_vreg));
                        mul->addOperand(std::make_unique<RegOperand>(index_vreg));
                        mul->addOperand(std::make_unique<RegOperand>(size_vreg));
                        CurMBB->addInstruction(std::move(mul));
                    }

                    // --- 关键修复点 ---
                    // 创建一个新的vreg来保存本次加法的结果。
                    unsigned current_step_addr_vreg = getNewVReg(gep->getType());
                    
                    // 执行 add current_step, last_step, offset
                    // 这确保了 last_step_addr_vreg (输入) 永远不会被直接修改。
                    auto add = std::make_unique<MachineInstr>(RVOpcodes::ADD);
                    add->addOperand(std::make_unique<RegOperand>(current_step_addr_vreg));
                    add->addOperand(std::make_unique<RegOperand>(last_step_addr_vreg));
                    add->addOperand(std::make_unique<RegOperand>(offset_vreg));
                    CurMBB->addInstruction(std::move(add));

                    // 本次的结果成为下一次计算的输入。
                    last_step_addr_vreg = current_step_addr_vreg;
                }

                // --- 为下一次迭代更新类型 ---
                if (auto array_type = current_type->as<ArrayType>()) {
                    current_type = array_type->getElementType();
                } else if (auto ptr_type = current_type->as<PointerType>()) {
                    current_type = ptr_type->getBaseType();
                }
            }
            
            // --- Step 3: 将最终计算出的地址存入GEP的目标虚拟寄存器 ---
            auto final_mv = std::make_unique<MachineInstr>(RVOpcodes::MV);
            final_mv->addOperand(std::make_unique<RegOperand>(result_vreg));
            final_mv->addOperand(std::make_unique<RegOperand>(last_step_addr_vreg));
            CurMBB->addInstruction(std::move(final_mv));
            break;
        }
        }

        default:
            throw std::runtime_error("Unsupported DAGNode kind in ISel");
    }
}

// 以下是忠实移植的DAG构建函数
RISCv64ISel::DAGNode* RISCv64ISel::create_node(int kind_int, Value* val, std::map<Value*, DAGNode*>& value_to_node, std::vector<std::unique_ptr<DAGNode>>& nodes_storage) {
    auto kind = static_cast<DAGNode::NodeKind>(kind_int);
    if (val && value_to_node.count(val) && kind != DAGNode::STORE && kind != DAGNode::RETURN && kind != DAGNode::BRANCH && kind != DAGNode::MEMSET) {
        return value_to_node[val];
    }
    auto node = std::make_unique<DAGNode>(kind);
    node->value = val;
    DAGNode* raw_node_ptr = node.get();
    nodes_storage.push_back(std::move(node));
    if (val && !val->getType()->isVoid() && (dynamic_cast<Instruction*>(val) || dynamic_cast<GlobalValue*>(val))) {
        value_to_node[val] = raw_node_ptr;
    }
    return raw_node_ptr;
}

RISCv64ISel::DAGNode* RISCv64ISel::get_operand_node(
    Value* val_ir, 
    std::map<Value*, DAGNode*>& value_to_node, 
    std::vector<std::unique_ptr<DAGNode>>& nodes_storage
) {
    // 空指针错误处理
    if (val_ir == nullptr) {
        throw std::runtime_error("get_operand_node received a null Value.");
    }

    // 规则1：如果这个Value已经有对应的节点，直接返回
    if (value_to_node.count(val_ir)) {
        return value_to_node[val_ir];
    }
    if (auto const_val = dynamic_cast<ConstantValue*>(val_ir)) {
        if (const_val->isInt()) {
            return create_node(DAGNode::CONSTANT, val_ir, value_to_node, nodes_storage);
        } else {
            // 为浮点常量创建新的FP_CONSTANT节点
            return create_node(DAGNode::FP_CONSTANT, val_ir, value_to_node, nodes_storage);
        }
    }
    if (dynamic_cast<GlobalValue*>(val_ir)) {
        return create_node(DAGNode::CONSTANT, val_ir, value_to_node, nodes_storage);
    }
    if (dynamic_cast<AllocaInst*>(val_ir)) {
        return create_node(DAGNode::ALLOCA_ADDR, val_ir, value_to_node, nodes_storage);
    }
    if (dynamic_cast<Argument*>(val_ir)) { 
        return create_node(DAGNode::ARGUMENT, val_ir, value_to_node, nodes_storage);
    }
    if (dynamic_cast<ConstantVariable*>(val_ir)) {
        // 全局常量数组和全局变量类似，在指令选择层面都表现为一个常量地址
        // 因此也为它创建一个 CONSTANT 类型的节点
        return create_node(DAGNode::CONSTANT, val_ir, value_to_node, nodes_storage);
    } 
    // 如果代码执行到这里，意味着val_ir不是上述任何一种叶子节点，
    // 且它也不是在当前基本块中定义的（否则它会在value_to_node中被找到）。
    // 这说明它是一个来自前驱块的Live-In值。
    
    // 我们将其视为一个与函数参数(Argument)类似的“块输入值”，并为它创建一个ARGUMENT节点。
    // 这样，后续的指令选择逻辑就知道这个值是直接可用的，无需在当前块内计算。
    if (dynamic_cast<Instruction*>(val_ir)) {
        return create_node(DAGNode::ARGUMENT, val_ir, value_to_node, nodes_storage);
    }

    // 如果一个Value不是任何已知类型，也不是指令，那说明出现了未处理的情况，抛出异常。
    throw std::runtime_error("Unhandled Value type in get_operand_node for value named: " + val_ir->getName());
}

std::vector<std::unique_ptr<RISCv64ISel::DAGNode>> RISCv64ISel::build_dag(BasicBlock* bb) {
    std::vector<std::unique_ptr<DAGNode>> nodes_storage;
    std::map<Value*, DAGNode*> value_to_node;

    for (const auto& inst_ptr : bb->getInstructions()) {
        Instruction* inst = inst_ptr.get();
        if (auto alloca = dynamic_cast<AllocaInst*>(inst)) {
            create_node(DAGNode::ALLOCA_ADDR, alloca, value_to_node, nodes_storage);
        } else if (auto store = dynamic_cast<StoreInst*>(inst)) {
            auto store_node = create_node(DAGNode::STORE, store, value_to_node, nodes_storage);
            store_node->operands.push_back(get_operand_node(store->getValue(), value_to_node, nodes_storage));
            store_node->operands.push_back(get_operand_node(store->getPointer(), value_to_node, nodes_storage));
        } else if (auto memset = dynamic_cast<MemsetInst*>(inst)) {
            auto memset_node = create_node(DAGNode::MEMSET, memset, value_to_node, nodes_storage);
            memset_node->operands.push_back(get_operand_node(memset->getPointer(), value_to_node, nodes_storage));
            memset_node->operands.push_back(get_operand_node(memset->getBegin(), value_to_node, nodes_storage));
            memset_node->operands.push_back(get_operand_node(memset->getSize(), value_to_node, nodes_storage));
            memset_node->operands.push_back(get_operand_node(memset->getValue(), value_to_node, nodes_storage));
            if (DEBUG) {
                std::cout << "[DEBUG] build_dag: Created MEMSET node for: " << memset->getName() << std::endl;
                for (size_t i = 0; i < memset_node->operands.size(); ++i) {
                    std::cout << "  -> Operand " << i << " has kind: " << memset_node->operands[i]->kind << std::endl;
                }
            }
        } else if (auto gep = dynamic_cast<GetElementPtrInst*>(inst)) {
            // 如果这个GEP指令已经创建过节点，则跳过
            if(value_to_node.count(gep)) continue;

            // 创建一个新的 GET_ELEMENT_PTR 类型的节点
            auto gep_node = create_node(DAGNode::GET_ELEMENT_PTR, gep, value_to_node, nodes_storage);
            
            // 第一个操作数是基指针（即数组本身）
            gep_node->operands.push_back(get_operand_node(gep->getBasePointer(), value_to_node, nodes_storage));
            
            // 依次添加所有索引作为后续的操作数
            for (auto index : gep->getIndices()) {
                // 从 Use 对象中获取真正的 Value*
                gep_node->operands.push_back(get_operand_node(index->getValue(), value_to_node, nodes_storage));
            }
        } else if (auto load = dynamic_cast<LoadInst*>(inst)) {
            auto load_node = create_node(DAGNode::LOAD, load, value_to_node, nodes_storage);
            load_node->operands.push_back(get_operand_node(load->getPointer(), value_to_node, nodes_storage));
        } else if (auto bin = dynamic_cast<BinaryInst*>(inst)) {
            if(value_to_node.count(bin)) continue;
            if (bin->getKind() == Instruction::kFSub) {
                if (auto const_lhs = dynamic_cast<ConstantValue*>(bin->getLhs())) {
                    // 使用isZero()来判断浮点数0.0，比直接比较更健壮
                    if (const_lhs->isZero()) {
                        // 这是一个浮点取负操作，创建 FUNARY 节点
                        auto funary_node = create_node(DAGNode::FUNARY, bin, value_to_node, nodes_storage);
                        funary_node->operands.push_back(get_operand_node(bin->getRhs(), value_to_node, nodes_storage));
                        continue; // 处理完毕，跳到下一条指令
                    }
                }
            }
            if (bin->getKind() == BinaryInst::kSub) {
                if (auto const_lhs = dynamic_cast<ConstantValue*>(bin->getLhs())) {
                    if (const_lhs->getInt() == 0) {
                        auto unary_node = create_node(DAGNode::UNARY, bin, value_to_node, nodes_storage);
                        unary_node->operands.push_back(get_operand_node(bin->getRhs(), value_to_node, nodes_storage));
                        continue;
                    }
                }
            }
            if (bin->isFPBinary()) { // 假设浮点指令枚举值更大
                auto fbin_node = create_node(DAGNode::FBINARY, bin, value_to_node, nodes_storage);
                fbin_node->operands.push_back(get_operand_node(bin->getLhs(), value_to_node, nodes_storage));
                fbin_node->operands.push_back(get_operand_node(bin->getRhs(), value_to_node, nodes_storage));
            } else {
                auto bin_node = create_node(DAGNode::BINARY, bin, value_to_node, nodes_storage);
                bin_node->operands.push_back(get_operand_node(bin->getLhs(), value_to_node, nodes_storage));
                bin_node->operands.push_back(get_operand_node(bin->getRhs(), value_to_node, nodes_storage));
            }
        } else if (auto un = dynamic_cast<UnaryInst*>(inst)) {
            if(value_to_node.count(un)) continue;
            if (un->getKind() >= Instruction::kFNeg) {
                auto funary_node = create_node(DAGNode::FUNARY, un, value_to_node, nodes_storage);
                funary_node->operands.push_back(get_operand_node(un->getOperand(), value_to_node, nodes_storage));
            } else {
                auto unary_node = create_node(DAGNode::UNARY, un, value_to_node, nodes_storage);
                unary_node->operands.push_back(get_operand_node(un->getOperand(), value_to_node, nodes_storage));
            }
        } else if (auto call = dynamic_cast<CallInst*>(inst)) {
            if(value_to_node.count(call)) continue;
            auto call_node = create_node(DAGNode::CALL, call, value_to_node, nodes_storage);
            for (auto arg : call->getArguments()) {
                call_node->operands.push_back(get_operand_node(arg->getValue(), value_to_node, nodes_storage));
            }
        } else if (auto ret = dynamic_cast<ReturnInst*>(inst)) {
            auto ret_node = create_node(DAGNode::RETURN, ret, value_to_node, nodes_storage);
            if (ret->hasReturnValue()) {
                ret_node->operands.push_back(get_operand_node(ret->getReturnValue(), value_to_node, nodes_storage));
            }
        } else if (auto cond_br = dynamic_cast<CondBrInst*>(inst)) {
            auto br_node = create_node(DAGNode::BRANCH, cond_br, value_to_node, nodes_storage);
            br_node->operands.push_back(get_operand_node(cond_br->getCondition(), value_to_node, nodes_storage));
        } else if (auto uncond_br = dynamic_cast<UncondBrInst*>(inst)) {
            create_node(DAGNode::BRANCH, uncond_br, value_to_node, nodes_storage);
        }
    }
    return nodes_storage;
}

/**
 * @brief 计算一个类型在内存中占用的字节数。
 * @param type 需要计算大小的IR类型。
 * @return 该类型占用的字节数。
 */
unsigned RISCv64ISel::getTypeSizeInBytes(Type* type) {
    if (!type) {
        assert(false && "Cannot get size of a null type.");
        return 0;
    }

    switch (type->getKind()) {
        // 对于SysY语言，基本类型int和float都占用4字节
        case Type::kInt:
        case Type::kFloat:
            return 4;

        // 指针类型在RISC-V 64位架构下占用8字节
        // 虽然SysY没有'int*'语法，但数组变量在IR层面本身就是指针类型
        case Type::kPointer:
            return 8;

        // 数组类型的总大小 = 元素数量 * 单个元素的大小
        case Type::kArray: {
            auto arrayType = type->as<ArrayType>();
            // 递归调用以计算元素大小
            return arrayType->getNumElements() * getTypeSizeInBytes(arrayType->getElementType());
        }

        // 其他类型，如Void, Label等不占用栈空间，或者不应该出现在这里
        default:
            // 如果遇到未处理的类型，触发断言，方便调试
            assert(false && "Unsupported type for size calculation.");
            return 0;
    }
}

// 打印DAG图以供调试的辅助函数
void RISCv64ISel::print_dag(const std::vector<std::unique_ptr<DAGNode>>& dag, const std::string& bb_name) {
    // 检查是否有DEBUG宏或者全局变量，避免在非调试模式下打印
    // if (!DEBUG) return; 

    std::cerr << "=== DAG for Basic Block: " << bb_name << " ===\n";
    std::set<DAGNode*> visited;

    // 为节点分配临时ID，方便阅读
    std::map<DAGNode*, int> node_to_id;
    int current_id = 0;
    for (const auto& node_ptr : dag) {
        node_to_id[node_ptr.get()] = current_id++;
    }

    // 将NodeKind枚举转换为字符串的辅助函数
    auto get_kind_string = [](DAGNode::NodeKind kind) {
        switch (kind) {
            case DAGNode::ARGUMENT: return "ARGUMENT";
            case DAGNode::CONSTANT: return "CONSTANT";
            case DAGNode::LOAD: return "LOAD";
            case DAGNode::STORE: return "STORE";
            case DAGNode::BINARY: return "BINARY";
            case DAGNode::CALL: return "CALL";
            case DAGNode::RETURN: return "RETURN";
            case DAGNode::BRANCH: return "BRANCH";
            case DAGNode::ALLOCA_ADDR: return "ALLOCA_ADDR";
            case DAGNode::UNARY: return "UNARY";
            case DAGNode::MEMSET: return "MEMSET";
            case DAGNode::GET_ELEMENT_PTR: return "GET_ELEMENT_PTR";
            default: return "UNKNOWN";
        }
    };

    // 递归打印节点的lambda表达式
    std::function<void(DAGNode*, int)> print_node = 
        [&](DAGNode* node, int indent) {
        if (!node) return;

        std::string current_indent(indent, ' ');
        int node_id = node_to_id.count(node) ? node_to_id[node] : -1;

        std::cerr << current_indent << "Node#" << node_id << ": " << get_kind_string(node->kind);
        
        // 尝试打印关联的虚拟寄存器
        if (node->value && vreg_map.count(node->value)) {
            std::cerr << " (vreg: %vreg" << vreg_map.at(node->value) << ")";
        }

        // 打印关联的IR Value信息
        if (node->value) {
            std::cerr << " [";
            if (auto inst = dynamic_cast<Instruction*>(node->value)) {
                std::cerr << inst->getKindString();
                if (!inst->getName().empty()) {
                    std::cerr << "(" << inst->getName() << ")";
                }
            } else if (auto constant = dynamic_cast<ConstantValue*>(node->value)) {
                std::cerr << "Const(" << constant->getInt() << ")";
            } else if (auto global = dynamic_cast<GlobalValue*>(node->value)) {
                std::cerr << "Global(" << global->getName() << ")";
            } else if (auto alloca = dynamic_cast<AllocaInst*>(node->value)) {
                std::cerr << "Alloca(" << alloca->getName() << ")";
            }
            std::cerr << "]";
        }
        std::cerr << "\n";

        if (visited.count(node)) {
            std::cerr << current_indent << "  (已打印过子节点)\n";
            return;
        }
        visited.insert(node);

        if (!node->operands.empty()) {
            std::cerr << current_indent << "  Operands:\n";
            for (auto operand : node->operands) {
                print_node(operand, indent + 4);
            }
        }
    };

    // 从根节点（没有用户的节点，或有副作用的节点）开始打印
    for (const auto& node_ptr : dag) {
        if (node_ptr->users.empty() || 
            node_ptr->kind == DAGNode::STORE || 
            node_ptr->kind == DAGNode::RETURN || 
            node_ptr->kind == DAGNode::BRANCH ||
            node_ptr->kind == DAGNode::MEMSET) 
        {
            print_node(node_ptr.get(), 0);
        }
    }
    std::cerr << "======================================\n\n";
}

unsigned int RISCv64ISel::getVRegCounter() const {
    return vreg_counter;
}

} // namespace sysy