#include "../../include/ir/tca2llvm.h"
#include "../../include/ir/cfg.h"
#include "../../include/ir/mem2reg.h" 
#include "../../include/ir/constant_propagation.h"
#include "../../include/ir/cfg_simplify.h"
#include "../../include/ir/instcombine.h"
#include "../../include/ir/gvn.h"
#include "../../include/ir/gcm.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/licm.h"
#include "../../include/ir/function_analysis.h"
#include "../../include/ir/inline.h"
#include "../../include/ir/loop_unroll.h"
#include "../../include/ir/PHIElimination.h"
#include "../../include/ir/dce.h"
#include "../../include/ir/adce.h"
#include "../../include/ir/ssadestruction.h"
#include "../../include/ir/dead_loop_elim.h"
#include "../../include/ir/block_ordering.h"
#include "../../include/ir/scev.h"
#include "../../include/ir/merge_bb.h"
#include "../../include/ir/loop_strength_reduce.h"
#include "../../include/ir/global_to_local.h"
#include "../../include/ir/loop_interchange.h"
#include "../../include/ir/tail_rec_elim.h"

#include <iostream>
#include <set>
#include <sstream>
#include "../../debug.h"

namespace llvm_ir {

    LLVMIRGenerator::LLVMIRGenerator(const std::string& moduleName) : module(std::make_unique<Module>(moduleName)), currentFunction(nullptr), currentBlock(nullptr), labelCounter(0), tempCounter(0) {}

    std::unique_ptr<Module> LLVMIRGenerator::generateModule(const ir::Program& program) {
        // 生成内置函数声明
        generateBuiltinFunctions();

        // 生成全局变量
        generateGlobalVariables(program);

        // 生成每个函数
        for (const auto& irFunc : program.functions) {
            generateFunction(irFunc);
        }

    #if OPEN_TAIL_REC_ELIMINATION
        // 在构造唯一出口之前执行尾递归消除，这样更容易匹配形如
        //   ret f(args')
        // 的直接尾调用模式，而不是在统一出口/引入额外 basic block 之后再处理。
        // 这样也避免了我们需要在 pass 中处理 unified-exit 的 phi/branch 结构。
        // （后续的 MakeFunctionOneExit 仍会正常处理保留的基准情形 return。）
        tail_rec_elim::runOnModule(*module);
    #endif

        // 为每个函数生成唯一出口
        for (auto& func : module->functions) {
            cfg::MakeFunctionOneExit(*func);
        }

        // ====== IR 优化 pass：将仅在单函数中使用的全局变量降级为局部变量 ======
        // {
        //     bool demoted = false;
        //     demoted = global_to_local::runOnModule(*module) || demoted;
        //     if (demoted) {
        //         cfg::rebuildUseDefChainsModule(*module);
        //         for (auto& func : module->functions) {
        //             cfg::buildCFG(*func);
        //         }
        //     }
        // }

        // ====== IR 优化 pass：Mem2Reg ======
    #if OPEN_MEM2REG
        mem2reg::Mem2Reg mem2reg_pass;
        mem2reg_pass.runOnModule(*module);
    #endif

        bool hasChanged = false;
        int iteration = 0;
        do {
            hasChanged = false;
            iteration++;
            std::cout << "\n===============IR PASS ITERATION " << iteration << "===============\n" << std::endl;

            // (尾递归消除已在构造唯一出口之前执行，这里不再重复。)

            // ====== IR 优化 pass：函数内联 ======
        #if OPEN_INLINE
            hasChanged |= inliner::runOnModule(*module);
        #endif

            // ====== IR 优化 pass：常量传播 ======
        #if OPEN_CONSTANT_PROPAGATION
            hasChanged |= ir_opt::sparseConditionalConstantPropagation(*module);
        #endif

        #if OPEN_GLOBAL_CONST_PROP
            hasChanged |= instcombine::runGlobalConstProp(*module);
        #endif

        #if OPEN_LOOP_INTERCHANGE
        if(iteration == 2) {
            loop_interchange::LoopInterchangePass LIC(*module);
            hasChanged |= LIC.run();
        }
        #endif

        #if (OPEN_LOOP_UNROLL)
        bool unrolled = false;
        if (iteration > 1 && iteration < 8) {
            #if OPEN_DCE
                dce::runOnModule(*module);
            #endif
            loop_unroll::LoopUnrollPass loop_unroll_pass(*module);
            unrolled = loop_unroll_pass.run();
            #if OPEN_DCE
            if (unrolled) dce::runOnModule(*module);
            #endif
        } else if (iteration == 8) hasChanged = true;
        #endif

        #if (OPEN_MERGE_BB)
            merge_bb::runOnModule(*module);
            #if OPEN_CONSTANT_PROPAGATION
            hasChanged |= ir_opt::sparseConditionalConstantPropagation(*module);
            #endif
        #endif

            // ====== IR 优化 pass：InstCombine ======
        #if OPEN_INST_COMBINE
            hasChanged = instcombine::runOnModule(*module) || hasChanged;
        #endif

            // ====== IR 优化 pass：Loop Strength Reduction ======
        #if OPEN_LOOP_UNROLL
        #if OPEN_LOOP_STRENGTH_REDUCE
            if (unrolled) {
            hasChanged = lsr::runOnModule(*module) || hasChanged;
            }
        #endif
        #endif

            // ====== IR 优化 pass：GVN ======
        #if OPEN_GVN
            function_analysis::analyzeFunctionAttributes(*module);
            hasChanged |= llvm_ir::gvn::runOnModule(*module);
            cfg::rebuildUseDefChainsModule(*module); //临时实现, for safety
            for (auto& func : module->functions) {
                cfg::buildCFG(*func);
            }
        #endif

            // ====== IR 优化 pass：GCM (Global Code Motion) ======
        #if OPEN_GCM
            hasChanged |= llvm_ir::gcm::runOnModule(*module);
        #endif

            // ====== IR 优化 pass：LICM ======
        #if OPEN_LICM
            function_analysis::analyzeFunctionAttributes(*module);
            hasChanged |= llvm_ir::licm::runOnModule(*module);
        #endif

            // ====== IR 优化 pass：死循环消除 ======
        #if OPEN_DEAD_LOOP_ELIMINATION
            bool loop_changed = false;
            DeadLoopElimination DLE(*module);
            loop_changed = DLE.run();
            hasChanged |= loop_changed;
            if (loop_changed) {
            #if OPEN_DCE
                dce::runOnModule(*module);
            #endif
            }
        #endif

        // ====== IR 优化 pass：将仅在单函数中使用的全局变量降级为局部变量 ======
        #if OPEN_GLOBAL_TO_LOCAL
            bool demoted = false;
            demoted = global_to_local::runOnModule(*module) || demoted;
            hasChanged |= demoted;
            if (demoted) {
            #if OPEN_MEM2REG
                mem2reg::Mem2Reg mem2reg_pass;
                mem2reg_pass.runOnModule(*module);
            #endif
            }
        #endif

            // ====== IR 优化 pass：空跳转块消除 ======
        #if OPEN_CFG_SIMPLIFY
            for (auto& func : module->functions) {
                if (!func->isDeclaration()) {
                    cfg::RemoveTrampolineBlocks(*func);
                }
            }
            cfg::rebuildUseDefChainsModule(*module);
        #endif

            // ====== IR 优化 pass：死代码消除 ======
        #if OPEN_DCE
            dce::runOnModule(*module);
        #endif

            // ====== IR 优化 pass：激进死代码消除 ======
        #if OPEN_ADCE
            hasChanged |= adce::runOnModule(*module);
        #endif

            // ====== IR 优化 pass：基本块排序 ======
        #if OPEN_BLOCK_ORDERING
            for (auto& func : module->functions) {
                if (!func->isDeclaration()) {
                    llvm_ir::OrderBlocksByDominatorTree(*func);
                }
            }
        #endif
        } while ((hasChanged && iteration < MAX_IRPASS_ITERATIONS) || (iteration == 1 && MAX_IRPASS_ITERATIONS > 1));
        std::cout << "[PassIteration] 优化迭代次数：" << iteration << std::endl;

        // ====== IR 优化 pass：PHI 消除 ======
    #if OPEN_PHI_ELIMINATION
        phi_elimination::DemoteAllPHIs(*module);
        
        #if OPEN_GVN
            llvm_ir::gvn::runOnModule(*module);
            cfg::rebuildUseDefChainsModule(*module); //临时实现, for safety
            for (auto& func : module->functions) {
                cfg::buildCFG(*func);
            }
        #endif
    #endif

        // ssa destruction (phi to move)
        // 处理完之后的use-def链可能存在问题了
    #if OPEN_SSA_DESTRUCTION
        for (auto& func: module->functions) {
            if (!func->isDeclaration()) {
                ssadestruction::destroySSA(*func);
                // ssadestruction::removeUnecessayMove(*func); // TODO
            }
        }
    #endif

        // 临时debug，检查有没有指令的parent为nullptr
        std::cout << "临时debug，检查有没有指令的parent为nullptr\n";
        for (auto& func: module->functions) {
            for (auto& block: func->basicBlocks) {
                for (auto& inst: block->instructions) {
                    if (inst->parent == nullptr) {
                        std::cerr << "[DEBUG] Instruction " << inst->toString() << " has nullptr parent" << std::endl;
                        // abort();
                    } else if (inst->parent != block.get()) {
                        std::cerr << "[DEBUG] Instruction " << inst->toString() << " has wrong parent" << std::endl;
                        // abort();
                    }
                }
            }
        }

        //为每一个lable加上Function名称的前缀
        #if (OPEN_PHI_ELIMINATION || OPEN_SSA_DESTRUCTION)
        std::cout << "[DEBUG] Adding function name prefix to labels..." << std::endl;
        for (auto& func: module->functions) {
            if (func->name == "main") continue;
            for (auto& block: func->basicBlocks) {
                block->label = func->name + "_" + block->label;
                
                // 处理跳转指令
                for (auto& inst: block->instructions) {
                    if (inst->opcode == Opcode::Br) {
                        auto* branchInst = dynamic_cast<BranchInst*>(inst.get());
                        if (branchInst) {
                            // std::cout << "[DEBUG] Found branch instruction: " << branchInst->toString() << std::endl;
                            if (!branchInst->trueLabel.empty()) {
                                branchInst->trueLabel = func->name + "_" + branchInst->trueLabel;
                            }
                            if (!branchInst->falseLabel.empty()) {
                                branchInst->falseLabel = func->name + "_" + branchInst->falseLabel;
                            }
                        }
                    }
                }
            }
        }
        #endif

        return std::move(module);
    }

    llvm_ir::Type LLVMIRGenerator::convertType(ir::Type irType) {
        switch (irType) {
        case ir::Type::Int:
        case ir::Type::IntLiteral:
            return llvm_ir::Type::I32;
        case ir::Type::Float:
        case ir::Type::FloatLiteral:
            return llvm_ir::Type::Float;
        case ir::Type::IntPtr:
            return llvm_ir::Type::Ptr;  // 指向i32的指针
        case ir::Type::FloatPtr:
            return llvm_ir::Type::Ptr;  // 指向float的指针
        case ir::Type::null:
        default:
            return llvm_ir::Type::Void;
        }
    }

    llvm_ir::Type LLVMIRGenerator::convertTypeForArrayElement(ir::Type irType) {
        switch (irType) {
            case ir::Type::Int:
            case ir::Type::IntLiteral:
            case ir::Type::IntPtr:
                return llvm_ir::Type::I32;
            case ir::Type::Float:
            case ir::Type::FloatLiteral:
            case ir::Type::FloatPtr:
                return llvm_ir::Type::Float;
            case ir::Type::null:
            default:
                return llvm_ir::Type::Void;
        }
    }

    void LLVMIRGenerator::generateFunction(const ir::Function& irFunc) {
        // 创建LLVM函数
        llvm_ir::Type retType = convertType(irFunc.returnType);
        auto llvmFunc = std::make_unique<Function>(irFunc.name, retType);

        // 添加参数
        for (const auto& param : irFunc.ParameterList) {
            llvm_ir::Type paramType = convertType(param.type);
            auto paramValue = std::make_unique<Value>("%" + param.name, paramType);
            llvmFunc->parameters.push_back(paramValue.get());
            valueMap[param.name] = paramValue.release();  // 注意：这里需要管理内存
        }

        currentFunction = llvmFunc.get();
        currentIRFunction = &irFunc;

        // 创建入口基本块
        auto entryBlock = std::make_unique<BasicBlock>("entry");
        currentBlock = entryBlock.get();
        builder.setInsertPoint(currentBlock);
        blockMap[0] = entryBlock.get();  // 将entry基本块映射到指令0

        llvmFunc->addBasicBlock(std::move(entryBlock));

        // 预分配所有变量（解决支配关系问题）
        preallocateVariables(irFunc);

        // 预先创建所有跳转目标的基本块
        createBasicBlocks(irFunc);

        // 检查是否有跳转指向指令0
        bool hasJumpToZero = blockMap.find(-1) != blockMap.end();  // 检查是否有label_0基本块

        // 如果有跳转指向指令0，在入口基本块添加跳转到label_0并切换到label_0
        if (hasJumpToZero) {
            builder.createBr("label_0");
            // 切换到label_0基本块继续处理指令
            currentBlock = blockMap[-1];
            builder.setInsertPoint(currentBlock);
        }

        // 生成指令
        for (size_t i = 0; i < irFunc.InstVec.size(); ++i) {
            currentInstructionIndex = i;  // 设置当前指令索引

            // 检查是否是跳转目标，如果是则切换到对应的基本块
            BasicBlock* targetBlock = nullptr;
            if (i == 0 && blockMap.find(-1) != blockMap.end()) {
                // 指令0且有label_0基本块
                targetBlock = blockMap[-1];
            } else if (blockMap.find(i) != blockMap.end()) {
                targetBlock = blockMap[i];
            }

            if (targetBlock && targetBlock != currentBlock) {
                // 如果当前基本块存在且没有终止符，添加fall-through跳转
                if (currentBlock && !currentBlock->instructions.empty() && currentBlock->instructions.back()->opcode != Opcode::Ret && currentBlock->instructions.back()->opcode != Opcode::Br) {
                    // 添加跳转到新基本块
                    std::string nextLabel = (i == 0) ? "label_0" : ("label_" + std::to_string(i));
                    builder.createBr(nextLabel);
                }

                currentBlock = targetBlock;
                builder.setInsertPoint(currentBlock);
            }
            generateInstruction(irFunc.InstVec[i]);
        }

        // 如果当前基本块存在且没有terminator指令，添加默认的return
        if (currentBlock && (currentBlock->instructions.empty() || (currentBlock->instructions.back()->opcode != Opcode::Ret && currentBlock->instructions.back()->opcode != Opcode::Br))) {
            if (retType == llvm_ir::Type::Void) {
                builder.createRet();
            } else {
                // 为非void函数创建默认返回值
                Value* defaultVal;
                if (retType == llvm_ir::Type::I32) {
                    defaultVal = new ConstantInt(0);
                } else if (retType == llvm_ir::Type::Float) {
                    defaultVal = new ConstantFloat(0.0);
                } else {
                    defaultVal = new ConstantInt(0);
                }
                builder.createRet(defaultVal);
            }
        }

        module->addFunction(std::move(llvmFunc));
        currentFunction = nullptr;
        currentBlock = nullptr;
        blockMap.clear();  // 清理基本块映射
    }

    void LLVMIRGenerator::generateInstruction(const ir::Instruction* inst) {
        // 如果当前没有基本块，不要创建新的基本块，因为所有需要的基本块应该在createBasicBlocks中预先创建
        // 只有在遇到控制流指令时才会清空currentBlock
        if (!currentBlock) {
            // 如果当前指令索引对应一个跳转目标，切换到对应的基本块
            if (blockMap.find(currentInstructionIndex) != blockMap.end()) {
                currentBlock = blockMap[currentInstructionIndex];
                builder.setInsertPoint(currentBlock);
#if (DEBUG_TCA_2_LLVM)
                std::cout << "[DEBUG] Recovered basic block: " << currentBlock->label << " for PC " << currentInstructionIndex << std::endl;
#endif
            } else {
#if (DEBUG_TCA_2_LLVM)
                // 如果没有对应的基本块，说明这是一个不可达的指令，跳过
                std::cout << "[DEBUG] Skipping unreachable instruction at PC " << currentInstructionIndex << std::endl;
#endif
                return;
            }
        }

        switch (inst->op) {
        case ir::Operator::add:
        case ir::Operator::fadd:
        case ir::Operator::sub:
        case ir::Operator::fsub:
        case ir::Operator::mul:
        case ir::Operator::fmul:
        case ir::Operator::div:
        case ir::Operator::fdiv:
        case ir::Operator::mod:
        case ir::Operator::_and:
        case ir::Operator::_or:
            generateBinaryOperation(inst);
            break;

        case ir::Operator::_not:
            generateUnaryOperation(inst);
            break;

        case ir::Operator::cvt_i2f:
        case ir::Operator::cvt_f2i:
            generateTypeConversion(inst);
            break;

        case ir::Operator::lss:
        case ir::Operator::flss:
        case ir::Operator::leq:
        case ir::Operator::fleq:
        case ir::Operator::gtr:
        case ir::Operator::fgtr:
        case ir::Operator::geq:
        case ir::Operator::fgeq:
        case ir::Operator::eq:
        case ir::Operator::feq:
        case ir::Operator::neq:
        case ir::Operator::fneq:
            generateCompareOperation(inst);
            break;

        case ir::Operator::alloc:
        case ir::Operator::store:
        case ir::Operator::load:
        case ir::Operator::getptr:
            generateMemoryOperation(inst);
            break;

        case ir::Operator::_return:
        case ir::Operator::_goto:
            generateControlFlow(inst);
            break;

        case ir::Operator::call:
            if (auto callInst = dynamic_cast<const ir::CallInst*>(inst)) {
                generateCallInstruction(callInst);
            }
            break;

        case ir::Operator::mov:
        case ir::Operator::fmov:
            // 赋值操作：存储值到预分配的变量
            {
                Value* src = convertOperand(inst->op1);

                // 检查是否是全局变量
                if (inst->des.name.find("s0_") == 0) { //#! 好危险的操作
                    // 全局变量赋值
                    std::string globalName = "@" + inst->des.name;
                    Value* globalPtr = new Value(globalName, llvm_ir::Type::Ptr);
                    builder.createStore(src, globalPtr);
                } else {
                    // 查找预分配的局部变量
                    auto it = valueMap.find(inst->des.name);
                    if (it != valueMap.end() && it->second->type == llvm_ir::Type::Ptr) {
                        // 使用预分配的alloca
                        Value* ptr = it->second;
                        builder.createStore(src, ptr);
                    } else {
                        // 如果没有预分配，说明可能是参数或其他值，直接赋值
                        valueMap[inst->des.name] = src;
                    }
                }
            }
            break;

        case ir::Operator::def:
        case ir::Operator::fdef:
            // 变量定义，检查是否已经在preallocateVariables中分配了alloca
            {
                if (valueMap.find(inst->des.name) == valueMap.end()) {
                    // 只有当变量未被预分配时才创建alloca
                    llvm_ir::Type allocType = convertType(inst->des.type);
                    Value* alloca = builder.createAlloca(allocType, "%" + inst->des.name);
                    valueMap[inst->des.name] = alloca;
                }
                // 如果已经预分配，则不做任何操作
            }
            break;

        default:
            std::cerr << "Warning: Unsupported instruction type" << std::endl;
            break;
        }
    }

    void LLVMIRGenerator::generateCallInstruction(const ir::CallInst* callInst) {
        std::vector<Value*> args;

        // 转换参数，对于函数调用需要特殊处理指针参数
        for (const auto& arg : callInst->argumentList) {
            Value* argValue = convertOperandForCall(arg);
            args.push_back(argValue);
        }

        // 确定返回类型
        llvm_ir::Type retType = convertType(callInst->des.type);

        // 创建call指令
        Value* result = builder.createCall(callInst->op1.name, args, callInst->des.name.empty() ? "" : "%" + callInst->des.name, retType);

        if (!callInst->des.name.empty()) {
            valueMap[callInst->des.name] = result;
        }
    }

    void LLVMIRGenerator::generateBinaryOperation(const ir::Instruction* inst) {
        Value* lhs = convertOperand(inst->op1);
        Value* rhs = convertOperand(inst->op2);
        Value* result = nullptr;

        // 检查目标变量是否已经有预分配的存储空间
        auto it = valueMap.find(inst->des.name);
        bool hasPreallocatedStorage = (it != valueMap.end() && it->second->type == llvm_ir::Type::Ptr);

        // 如果有预分配的存储空间，使用临时名称；否则使用目标变量名
        std::string resultName = hasPreallocatedStorage ? getNextTemp() : ("%" + inst->des.name);

        switch (inst->op) {
        case ir::Operator::add:
            // 对于算术运算，需要确保操作数类型一致
            if (lhs->type == llvm_ir::Type::I1) {
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            }
            if (rhs->type == llvm_ir::Type::I1) {
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            }
            result = builder.createAdd(lhs, rhs, resultName);
            break;
        case ir::Operator::fadd:
            result = builder.createFAdd(lhs, rhs, resultName);
            break;
        case ir::Operator::sub:
            // 对于算术运算，需要确保操作数类型一致
            if (lhs->type == llvm_ir::Type::I1) {
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            }
            if (rhs->type == llvm_ir::Type::I1) {
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            }
            result = builder.createSub(lhs, rhs, resultName);
            break;
        case ir::Operator::fsub:
            result = builder.createFSub(lhs, rhs, resultName);
            break;
        case ir::Operator::mul:
            // 对于算术运算，需要确保操作数类型一致
            if (lhs->type == llvm_ir::Type::I1) {
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            }
            if (rhs->type == llvm_ir::Type::I1) {
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            }
            result = builder.createMul(lhs, rhs, resultName);
            break;
        case ir::Operator::fmul:
            result = builder.createFMul(lhs, rhs, resultName);
            break;
        case ir::Operator::div:
            // 对于算术运算，需要确保操作数类型一致
            if (lhs->type == llvm_ir::Type::I1) {
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            }
            if (rhs->type == llvm_ir::Type::I1) {
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            }
            result = builder.createSDiv(lhs, rhs, resultName);
            break;
        case ir::Operator::fdiv:
            result = builder.createFDiv(lhs, rhs, resultName);
            break;
        case ir::Operator::mod:
            // 求余运算，使用srem指令，需要确保操作数类型一致
            if (lhs->type == llvm_ir::Type::I1) {
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            }
            if (rhs->type == llvm_ir::Type::I1) {
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            }
            result = builder.createSRem(lhs, rhs, resultName);
            break;
        case ir::Operator::_and:
            // 确保两个操作数都是i1类型
            if (lhs->type != llvm_ir::Type::I1) {
                // 将i32转换为i1 (非零为true，零为false)
                lhs = builder.createICmp(ICmpCond::NE, lhs, new ConstantInt(0), getNextTemp());
            }
            if (rhs->type != llvm_ir::Type::I1) {
                // 将i32转换为i1 (非零为true，零为false)
                rhs = builder.createICmp(ICmpCond::NE, rhs, new ConstantInt(0), getNextTemp());
            }
            result = builder.createAnd(lhs, rhs, resultName);
            break;
        case ir::Operator::_or:
            // 确保两个操作数都是i1类型
            if (lhs->type != llvm_ir::Type::I1) {
                // 将i32转换为i1 (非零为true，零为false)
                lhs = builder.createICmp(ICmpCond::NE, lhs, new ConstantInt(0), getNextTemp());
            }
            if (rhs->type != llvm_ir::Type::I1) {
                // 将i32转换为i1 (非零为true，零为false)
                rhs = builder.createICmp(ICmpCond::NE, rhs, new ConstantInt(0), getNextTemp());
            }
            result = builder.createOr(lhs, rhs, resultName);
            break;
        default:
            break;
        }

        if (result) {
            if (hasPreallocatedStorage) {
                // 如果有预分配的存储空间，检查类型兼容性
                Value* storeValue = result;

                // 检查预分配变量的类型（通过AllocaInst获取）
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                    llvm_ir::Type targetType = allocaInst->allocatedType;

                    // 如果结果是i1但目标是i32，需要扩展
                    if (result->type == llvm_ir::Type::I1 && targetType == llvm_ir::Type::I32) {
                        storeValue = builder.createZExt(result, llvm_ir::Type::I32, getNextTemp());
                    }
                    // 如果结果是i32但目标是i1，需要转换为布尔值
                    else if (result->type == llvm_ir::Type::I32 && targetType == llvm_ir::Type::I1) {
                        storeValue = builder.createICmp(ICmpCond::NE, result, new ConstantInt(0), getNextTemp());
                    }
                }

                builder.createStore(storeValue, it->second);
            } else {
                // 否则直接存储为临时值
                valueMap[inst->des.name] = result;
            }
        }
    }

    void LLVMIRGenerator::generateUnaryOperation(const ir::Instruction* inst) {
        Value* operand = convertOperand(inst->op1);
        Value* result = nullptr;

        // 检查目标变量是否已经有预分配的存储空间
        auto it = valueMap.find(inst->des.name);
        bool hasPreallocatedStorage = (it != valueMap.end() && it->second->type == llvm_ir::Type::Ptr);

        // 如果有预分配的存储空间，使用临时名称；否则使用目标变量名
        std::string resultName = hasPreallocatedStorage ? getNextTemp() : ("%" + inst->des.name);

        switch (inst->op) {
        case ir::Operator::_not:
            // 如果not的结果只被紧随其后的 if goto 使用，则不生成not指令，交由分支处反转
            if (currentIRFunction && currentInstructionIndex + 1 < currentIRFunction->InstVec.size()) {
                const ir::Instruction* nextInst = currentIRFunction->InstVec[currentInstructionIndex + 1];
                bool onlyUsedByFollowingGoto = false;
                if (nextInst && nextInst->op == ir::Operator::_goto && nextInst->op1.name == inst->des.name) {
                    // 向后扫描，检查是否还有其他使用
                    bool hasOtherUse = false;
                    for (size_t k = currentInstructionIndex + 2; k < currentIRFunction->InstVec.size(); ++k) {
                        const ir::Instruction* later = currentIRFunction->InstVec[k];
                        if (!later) continue;
                        if ((later->op1.name == inst->des.name) || (later->op2.name == inst->des.name) || (later->des.name == inst->des.name)) {
                            hasOtherUse = true;
                            break;
                        }
                    }
                    if (!hasOtherUse) {
                        // 不生成not指令，直接返回，后续if生成时会识别并反转
                        return;
                    }
                }
            }
            // 逻辑非运算
            if (operand->type == llvm_ir::Type::I1) {
                // 如果操作数是布尔类型，使用xor与true
                result = builder.createXor(operand, new ConstantInt(1, llvm_ir::Type::I1), resultName);
            } else {
                // 如果操作数是整数类型，先转换为布尔值，然后取反
                Value* boolVal = builder.createICmp(ICmpCond::NE, operand, new ConstantInt(0), getNextTemp());
                result = builder.createXor(boolVal, new ConstantInt(1, llvm_ir::Type::I1), resultName);
            }
            break;
        default:
            break;
        }

        if (result) {
            if (hasPreallocatedStorage) {
                // 如果有预分配的存储空间，检查类型兼容性
                Value* storeValue = result;

                // 检查预分配变量的类型（通过AllocaInst获取）
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                    llvm_ir::Type targetType = allocaInst->allocatedType;

                    // 如果结果是i1但目标是i32，需要扩展
                    if (result->type == llvm_ir::Type::I1 && targetType == llvm_ir::Type::I32) {
                        storeValue = builder.createZExt(result, llvm_ir::Type::I32, getNextTemp());
                    }
                    // 如果结果是i32但目标是i1，需要转换为布尔值
                    else if (result->type == llvm_ir::Type::I32 && targetType == llvm_ir::Type::I1) {
                        storeValue = builder.createICmp(ICmpCond::NE, result, new ConstantInt(0), getNextTemp());
                    }
                }

                builder.createStore(storeValue, it->second);
            } else {
                // 否则直接存储为临时值
                valueMap[inst->des.name] = result;
            }
        }
    }

    void LLVMIRGenerator::generateTypeConversion(const ir::Instruction* inst) {
        Value* operand = convertOperand(inst->op1);
        Value* result = nullptr;

        // 检查目标变量是否已经有预分配的存储空间
        auto it = valueMap.find(inst->des.name);
        bool hasPreallocatedStorage = (it != valueMap.end() && it->second->type == llvm_ir::Type::Ptr);

        // 如果有预分配的存储空间，使用临时名称；否则使用目标变量名
        std::string resultName = hasPreallocatedStorage ? getNextTemp() : ("%" + inst->des.name);

        switch (inst->op) {
        case ir::Operator::cvt_i2f:
            // 整数转浮点：sitofp指令
            if (operand->type == llvm_ir::Type::I1) {
                // 先将i1扩展为i32，再转换为float
                Value* extOperand = builder.createZExt(operand, llvm_ir::Type::I32, getNextTemp());
                result = builder.createSIToFP(extOperand, llvm_ir::Type::Float, resultName);
            } else {
                result = builder.createSIToFP(operand, llvm_ir::Type::Float, resultName);
            }
            break;

        case ir::Operator::cvt_f2i:
            // 浮点转整数：fptosi指令
            result = builder.createFPToSI(operand, llvm_ir::Type::I32, resultName);
            break;

        default:
            break;
        }

        if (result) {
            if (hasPreallocatedStorage) {
                // 如果有预分配的存储空间，将结果存储到那里
                builder.createStore(result, it->second);
            } else {
                // 否则直接存储为临时值
                valueMap[inst->des.name] = result;
            }
        }
    }
    void LLVMIRGenerator::generateCompareOperation(const ir::Instruction* inst) {
        Value* lhs = convertOperand(inst->op1);
        Value* rhs = convertOperand(inst->op2);
        Value* result = nullptr;

        // 检查目标变量是否已经有预分配的存储空间
        auto it = valueMap.find(inst->des.name);
        bool hasPreallocatedStorage = (it != valueMap.end() && it->second->type == llvm_ir::Type::Ptr);
        std::string resultName = hasPreallocatedStorage ? getNextTemp() : ("%" + inst->des.name);

        switch (inst->op) {
        case ir::Operator::lss:
            if (lhs->type == llvm_ir::Type::I1)
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            if (rhs->type == llvm_ir::Type::I1)
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            result = builder.createICmp(ICmpCond::SLT, lhs, rhs, resultName);
            break;
        case ir::Operator::flss:
            if (lhs->type != llvm_ir::Type::Float)
                lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
            if (rhs->type != llvm_ir::Type::Float)
                rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
            result = builder.createFCmp(FCmpCond::OLT, lhs, rhs, resultName);
            break;
        case ir::Operator::leq:
            if (lhs->type == llvm_ir::Type::I1)
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            if (rhs->type == llvm_ir::Type::I1)
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            result = builder.createICmp(ICmpCond::SLE, lhs, rhs, resultName);
            break;
        case ir::Operator::fleq:
            if (lhs->type != llvm_ir::Type::Float)
                lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
            if (rhs->type != llvm_ir::Type::Float)
                rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
            result = builder.createFCmp(FCmpCond::OLE, lhs, rhs, resultName);
            break;
        case ir::Operator::gtr:
            if (lhs->type == llvm_ir::Type::I1)
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            if (rhs->type == llvm_ir::Type::I1)
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            result = builder.createICmp(ICmpCond::SGT, lhs, rhs, resultName);
            break;
        case ir::Operator::fgtr:
            if (lhs->type != llvm_ir::Type::Float)
                lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
            if (rhs->type != llvm_ir::Type::Float)
                rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
            result = builder.createFCmp(FCmpCond::OGT, lhs, rhs, resultName);
            break;
        case ir::Operator::geq:
            if (lhs->type == llvm_ir::Type::I1)
                lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
            if (rhs->type == llvm_ir::Type::I1)
                rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
            result = builder.createICmp(ICmpCond::SGE, lhs, rhs, resultName);
            break;
        case ir::Operator::fgeq:
            if (lhs->type != llvm_ir::Type::Float)
                lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
            if (rhs->type != llvm_ir::Type::Float)
                rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
            result = builder.createFCmp(FCmpCond::OGE, lhs, rhs, resultName);
            break;
        case ir::Operator::eq:
        case ir::Operator::neq: {
            // 类型对齐：如果有 float，全部转 float 用 fcmp，否则都转 i32 用 icmp
            if (lhs->type == llvm_ir::Type::Float || rhs->type == llvm_ir::Type::Float) {
                if (lhs->type != llvm_ir::Type::Float)
                    lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
                if (rhs->type != llvm_ir::Type::Float)
                    rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
                FCmpCond cond = (inst->op == ir::Operator::eq) ? FCmpCond::OEQ : FCmpCond::ONE;
                result = builder.createFCmp(cond, lhs, rhs, resultName);
            } else {
                if (lhs->type == llvm_ir::Type::I1)
                    lhs = builder.createZExt(lhs, llvm_ir::Type::I32, getNextTemp());
                if (rhs->type == llvm_ir::Type::I1)
                    rhs = builder.createZExt(rhs, llvm_ir::Type::I32, getNextTemp());
                ICmpCond cond = (inst->op == ir::Operator::eq) ? ICmpCond::EQ : ICmpCond::NE;
                result = builder.createICmp(cond, lhs, rhs, resultName);
            }
            break;
        }
        case ir::Operator::feq:
        case ir::Operator::fneq: {
            // float 比较
            if (lhs->type != llvm_ir::Type::Float)
                lhs = builder.createSIToFP(lhs, llvm_ir::Type::Float, getNextTemp());
            if (rhs->type != llvm_ir::Type::Float)
                rhs = builder.createSIToFP(rhs, llvm_ir::Type::Float, getNextTemp());
            FCmpCond cond = (inst->op == ir::Operator::feq) ? FCmpCond::OEQ : FCmpCond::ONE;
            result = builder.createFCmp(cond, lhs, rhs, resultName);
            break;
        }
        default:
            break;
        }

        if (result) {
            if (hasPreallocatedStorage) {
                // 如果有预分配的存储空间，检查类型兼容性
                Value* storeValue = result;

                // 检查预分配变量的类型（通过AllocaInst获取）
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                    llvm_ir::Type targetType = allocaInst->allocatedType;

                    // 如果结果是i1但目标是i32，需要扩展
                    if (result->type == llvm_ir::Type::I1 && targetType == llvm_ir::Type::I32) {
                        storeValue = builder.createZExt(result, llvm_ir::Type::I32, getNextTemp());
                    }
                    // 如果结果是i32但目标是i1，需要转换为布尔值
                    else if (result->type == llvm_ir::Type::I32 && targetType == llvm_ir::Type::I1) {
                        storeValue = builder.createICmp(ICmpCond::NE, result, new ConstantInt(0), getNextTemp());
                    }
                }

                builder.createStore(storeValue, it->second);
            } else {
                // 否则直接存储为临时值
                valueMap[inst->des.name] = result;
            }
        }
    }

    void LLVMIRGenerator::generateMemoryOperation(const ir::Instruction* inst) {
        switch (inst->op) {
        case ir::Operator::alloc: {
            // 确定分配的类型
            llvm_ir::Type allocType = llvm_ir::Type::I32;  // 默认为i32
            if (inst->des.type == ir::Type::IntPtr) {
                allocType = llvm_ir::Type::I32;
            } else if (inst->des.type == ir::Type::FloatPtr) {
                allocType = llvm_ir::Type::Float;
            }

            // 检查是否有数组大小参数
            if (!inst->op1.name.empty() && inst->op1.name != "null") {
                // 数组分配：alloc array_name, size
                int arraySize = std::stoi(inst->op1.name);
                Value* alloca = builder.createAlloca(allocType, arraySize, "%" + inst->des.name);
                valueMap[inst->des.name] = alloca;

                // 使用memset初始化数组为0
                // 计算数组总字节数
                size_t elementSize = 0;
                if (allocType == llvm_ir::Type::I32) {
                    elementSize = 4;  // i32是4字节
                } else if (allocType == llvm_ir::Type::Float) {
                    elementSize = 4;  // float是4字节
                }

                if (elementSize > 0) {
                    size_t totalBytes = arraySize * elementSize;
                    
                    // 获取数组的第一个元素指针
                    Value* arrayPtr = builder.createGEP(alloca, new ConstantInt(0), getNextTemp(), allocType, arraySize);
                    
                    // 创建memset调用参数
                    std::vector<Value*> memsetArgs;
                    memsetArgs.push_back(arrayPtr);                           // destination ptr
                    memsetArgs.push_back(new ConstantInt(0));                 // value (0)
                    memsetArgs.push_back(new ConstantInt(totalBytes, llvm_ir::Type::I64));  // size
                    
                    // 调用memset（不需要返回值）
                    builder.createCall("memset", memsetArgs);
                }
            } else {
                // 单个元素分配
                Value* alloca = builder.createAlloca(allocType, "%" + inst->des.name);
                valueMap[inst->des.name] = alloca;
            }
        } break;

        case ir::Operator::store: {
            #if DEBUG_TCA_2_LLVM
            std::cout << "  memory operation store" << std::endl;
            std::cout << "  inst->op1.name: " << inst->op1.name << std::endl;
            std::cout << "  inst->op2.name: " << inst->op2.name << std::endl;
            #endif
            Value* val = convertOperand(inst->des);

            // 检查是否有索引操作数（数组存储）
            if (!inst->op2.name.empty() && inst->op2.name != "null") {
                // 数组存储：store value, array, index
                Value* arrayPtr = getArrayPointer(inst->op1);
                Value* index = convertOperand(inst->op2);

                // 尝试获取数组类型信息
                auto it = valueMap.find(inst->op1.name);
                if (it != valueMap.end()) {
                    if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                        // 使用数组类型信息生成GEP
                        Value* elementPtr = builder.createGEP(arrayPtr, index, getNextTemp(), allocaInst->allocatedType, allocaInst->arraySize);
                        builder.createStore(val, elementPtr);
                    } else if (auto globalVar = dynamic_cast<const GlobalVariable*>(it->second)) {
                        // 全局数组变量
                        if (globalVar->arraySize > 0) {
                            // 使用全局数组类型信息生成GEP
                            Value* elementPtr = builder.createGEP(arrayPtr, index, getNextTemp(), globalVar->elementType, globalVar->arraySize);
                            builder.createStore(val, elementPtr);
                        } else {
                            // 回退到简单的GEP
                            Value* elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                            builder.createStore(val, elementPtr);
                        }
                    } else {
                        // 回退到简单的GEP
                        Value* elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                        builder.createStore(val, elementPtr);
                    }
                } else {
                    // 回退到简单的GEP
                    Value* elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                    builder.createStore(val, elementPtr);
                }
            } else {
                // 普通指针存储
                Value* ptr = convertOperand(inst->op1);
                builder.createStore(val, ptr);
            }
        } break;

        case ir::Operator::load: {
        #if DEBUG_TCA_2_LLVM
            std::cout << "  memory operation load" << std::endl;
            std::cout << "  inst->op1.name: " << inst->op1.name << std::endl;
            std::cout << "  inst->op2.name: " << inst->op2.name << std::endl;
            std::cout << "  inst->des.name: " << inst->des.name << std::endl;
            std::cout << "  inst->des.type: " << (int)inst->des.type << std::endl;
        #endif
            // 检查是否有索引操作数（数组加载）
            if (!inst->op2.name.empty() && inst->op2.name != "null") {
                // 数组加载：load result, array, index
                Value* arrayPtr = getArrayPointer(inst->op1);
                Value* index = convertOperand(inst->op2);

                // 尝试获取数组类型信息
                auto it = valueMap.find(inst->op1.name);
                Value* elementPtr = nullptr;
                if (it != valueMap.end()) {
                    if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                        // 使用数组类型信息生成GEP
                        elementPtr = builder.createGEP(arrayPtr, index, getNextTemp(), allocaInst->allocatedType, allocaInst->arraySize);
                    } else if (auto globalVar = dynamic_cast<const GlobalVariable*>(it->second)) {
                        // 全局数组变量
                        if (globalVar->arraySize > 0) {
                            // 使用全局数组类型信息生成GEP
                            elementPtr = builder.createGEP(arrayPtr, index, getNextTemp(), globalVar->elementType, globalVar->arraySize);
                        } else {
                            // 回退到简单的GEP
                            elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                        }
                    } else {
                        // 回退到简单的GEP
                        elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                    }
                } else {
                    // 回退到简单的GEP
                    elementPtr = builder.createGEP(arrayPtr, index, getNextTemp());
                }

                // 推断load的类型
                llvm_ir::Type loadType = llvm_ir::Type::I32;
                if (inst->des.type == ir::Type::Float) {
                    loadType = llvm_ir::Type::Float;
                }

                Value* result = builder.createLoad(elementPtr, "%" + inst->des.name, loadType);
                result->type = loadType;  // 设置正确的类型
                valueMap[inst->des.name] = result;
            } else {
                // 普通指针加载
                Value* ptr = convertOperand(inst->op1);

                // 推断load的类型
                llvm_ir::Type loadType = llvm_ir::Type::I32;
                if (inst->des.type == ir::Type::Float) {
                    loadType = llvm_ir::Type::Float;
                }

                Value* result = builder.createLoad(ptr, "%" + inst->des.name, loadType);
                result->type = loadType;  // 设置正确的类型
                valueMap[inst->des.name] = result;
            }
        } break;

        case ir::Operator::getptr: {
            #if DEBUG_TCA_2_LLVM
            std::cout << "memory operation getptr" << std::endl;
            #endif
            // getptr指令：获取数组指针
            // 格式：getptr result, array, offset
            Value* arrayPtr = getArrayPointer(inst->op1);
            Value* offset = convertOperand(inst->op2);

            // 尝试获取数组类型信息
            auto it = valueMap.find(inst->op1.name);
            Value* elementPtr = nullptr;
            if (it != valueMap.end()) {
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(it->second)) {
                    // 使用数组类型信息生成GEP
                    elementPtr = builder.createGEP(arrayPtr, offset, "%" + inst->des.name, allocaInst->allocatedType, allocaInst->arraySize);
                } else if (auto globalVar = dynamic_cast<const GlobalVariable*>(it->second)) {
                    // 全局数组变量
                    if (globalVar->arraySize > 0) {
                        elementPtr = builder.createGEP(arrayPtr, offset, "%" + inst->des.name, globalVar->elementType, globalVar->arraySize);
                    } else {
                        // 回退到简单的GEP
                        elementPtr = builder.createGEP(arrayPtr, offset, "%" + inst->des.name);
                    }
                } else {
                    // 回退到简单的GEP
                    elementPtr = builder.createGEP(arrayPtr, offset, "%" + inst->des.name);
                }
            } else {
                // 回退到简单的GEP
                elementPtr = builder.createGEP(arrayPtr, offset, "%" + inst->des.name);
            }

            // 存储结果指针
            valueMap[inst->des.name] = elementPtr;
        } break;

        default:
            break;
        }
    }

    void LLVMIRGenerator::generateControlFlow(const ir::Instruction* inst) {
        switch (inst->op) {
        case ir::Operator::_return:
            if (inst->op1.name != "null" && !inst->op1.name.empty()) {
                Value* retVal = convertOperand(inst->op1);
                builder.createRet(retVal);
            } else {
                builder.createRet();
            }
            // return指令后，当前基本块结束
            currentBlock = nullptr;
            break;

        case ir::Operator::_goto:
            if (inst->op1.name != "null" && !inst->op1.name.empty()) {
                // 条件跳转：if condition goto target
                // 优化：检测前一条是否为 not，若是，通过交换分支实现反转
                bool invertByNot = false;
                ir::Operand rawCondOperand = inst->op1;
                if (currentInstructionIndex > 0) {
                    const ir::Instruction* prevInst = currentIRFunction->InstVec[currentInstructionIndex - 1];
                    if (prevInst && prevInst->op == ir::Operator::_not && prevInst->des.name == inst->op1.name) {
                        invertByNot = true;
                        rawCondOperand = prevInst->op1; // 使用not的输入作为真实条件
                    }
                }

                Value* cond = convertOperand(rawCondOperand);

                // 如果条件不是i1类型，需要转换
                if (cond->type != llvm_ir::Type::I1) {
                    if (cond->type == llvm_ir::Type::I32) {
                        Value* zero = new ConstantInt(0);
                        // 统一规范化为 x != 0
                        cond = builder.createICmp(ICmpCond::NE, cond, zero, getNextTemp());
                    } else if (cond->type == llvm_ir::Type::Float) {
                        Value* zeroFloat = new ConstantFloat(0.0);
                        // 统一规范化为 x != 0.0
                        cond = builder.createFCmp(FCmpCond::ONE, cond, zeroFloat, getNextTemp());
                    }
                }

                // 解析true分支跳转目标
                int trueTargetPC = parseJumpTarget(inst->des.name, currentInstructionIndex);

                // false分支是下一条指令
                int falseTargetPC = currentInstructionIndex + 1;

                // 检查true分支目标是否存在
                bool trueTargetExists = false;
                if (trueTargetPC == 0 && blockMap.count(-1) > 0) {
                    trueTargetExists = true;  // label_0存在
                } else if (blockMap.count(trueTargetPC) > 0) {
                    trueTargetExists = true;  // 其他标签存在
                }

                // 检查false分支目标是否存在
                bool falseTargetExists = false;
                if (falseTargetPC == 0 && blockMap.count(-1) > 0) {
                    falseTargetExists = true;  // label_0存在
                } else if (blockMap.count(falseTargetPC) > 0) {
                    falseTargetExists = true;  // 其他标签存在
                }

                if (trueTargetExists && falseTargetExists) {
                    std::string trueLabel = (trueTargetPC == 0) ? "label_0" : ("label_" + std::to_string(trueTargetPC));
                    std::string falseLabel = (falseTargetPC == 0) ? "label_0" : ("label_" + std::to_string(falseTargetPC));
                    // 创建条件分支
                    if (invertByNot) {
                        // 当未通过比较实现反转（例如原始条件就是i1）时，交换分支
                        builder.createCondBr(cond, falseLabel, trueLabel);
                    } else {
                        builder.createCondBr(cond, trueLabel, falseLabel);
                    }
                } else {
#if (DEBUG_TCA_2_LLVM)
                    // 如果有跳转目标不存在，生成隐式return
                    std::cout << "[DEBUG] Conditional jump with non-existent targets, generating implicit return" << std::endl;
#endif
                    if (currentIRFunction->returnType == ir::Type::null) {
                        builder.createRet();
                    } else if (currentIRFunction->returnType == ir::Type::Int) {
                        Value* zero = new ConstantInt(0);
                        builder.createRet(zero);
                    } else if (currentIRFunction->returnType == ir::Type::Float) {
                        Value* zero = new ConstantFloat(0.0);
                        builder.createRet(zero);
                    } else {
                        builder.createRet();
                    }
                }

            } else {
                // 无条件跳转：goto target
                int targetPC = parseJumpTarget(inst->des.name, currentInstructionIndex);

                // 检查目标基本块是否存在
                bool targetExists = false;
                if (targetPC == 0 && blockMap.count(-1) > 0) {
                    targetExists = true;  // label_0存在
                } else if (blockMap.count(targetPC) > 0) {
                    targetExists = true;  // 其他标签存在
                }

                if (targetExists) {
                    std::string targetLabel = (targetPC == 0) ? "label_0" : ("label_" + std::to_string(targetPC));
                    builder.createBr(targetLabel);
                } else {
// 跳转目标不存在，可能是跳转到函数末尾
// 根据函数返回类型生成适当的return
#if (DEBUG_TCA_2_LLVM)
                    std::cout << "[DEBUG] Jump to non-existent target PC " << targetPC << ", generating implicit return" << std::endl;
#endif
                    if (currentIRFunction->returnType == ir::Type::null) {
                        builder.createRet();
                    } else if (currentIRFunction->returnType == ir::Type::Int) {
                        // 对于int函数，默认返回0
                        Value* zero = new ConstantInt(0);
                        builder.createRet(zero);
                    } else if (currentIRFunction->returnType == ir::Type::Float) {
                        // 对于float函数，默认返回0.0
                        Value* zero = new ConstantFloat(0.0);
                        builder.createRet(zero);
                    } else {
                        // 其他类型，生成return null
                        builder.createRet();
                    }
                }
            }

            // 跳转后，当前基本块结束
            currentBlock = nullptr;
            break;

        default:
            break;
        }
    }

    Value* LLVMIRGenerator::convertOperand(const ir::Operand& operand) {
        // 检查是否是常量
        if (operand.type == ir::Type::IntLiteral) {
            return new ConstantInt(std::stoi(operand.name));
        } else if (operand.type == ir::Type::FloatLiteral) {
            return new ConstantFloat(std::stod(operand.name));
        }

        // 检查是否是全局变量
        if (operand.name.find("s0_") == 0) {
            std::string globalName = "@" + operand.name;
            // 创建全局变量引用（指针类型）
            Value* globalPtr = new Value(globalName, llvm_ir::Type::Ptr);

            // 检查是否是全局数组变量
            auto it = valueMap.find(operand.name);
            if (it != valueMap.end()) {
                if (auto globalVar = dynamic_cast<const GlobalVariable*>(it->second)) {
                    if (globalVar->arraySize > 0) {
                        // 全局数组变量，直接返回数组指针，不需要load
                        return globalPtr;
                    }
                }
            }

            // 普通全局变量需要生成load指令加载值
            llvm_ir::Type loadType = convertType(operand.type);
            if (loadType == llvm_ir::Type::Void) {
                loadType = llvm_ir::Type::I32;  // 默认为i32
            }
            return builder.createLoad(globalPtr, getNextTemp(), loadType);
        }

        // 检查是否已经存在
        auto it = valueMap.find(operand.name);
        if (it != valueMap.end()) {
            Value* value = it->second;
            // 如果是指针类型（alloca的结果），需要生成load指令
            // 但是如果这个操作数是在store指令的数组参数位置，则不应该load
            if (value->type == llvm_ir::Type::Ptr) {
                // 检查是否是AllocaInst且是数组
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(value)) {
                    if (allocaInst->arraySize > 0) {
                        // 数组变量，返回数组指针而不是load
                        return value;
                    }
                    // 非数组的alloca，需要load，使用alloca指向的实际类型
                    llvm_ir::Type loadType = allocaInst->allocatedType;
                    Value* result = builder.createLoad(value, getNextTemp(), loadType);
                    result->type = loadType;  // 设置正确的类型
                    return result;
                } else {
                    // 不是AllocaInst，可能是函数参数
                    // 检查是否是当前函数的参数
                    if (currentFunction) {
                        for (const auto& param : currentFunction->parameters) {
                            if (param->name == "%" + operand.name) {
                                // 是函数参数，如果参数类型是指针，直接返回，不需要load
                                if (operand.type == ir::Type::IntPtr || operand.type == ir::Type::FloatPtr) {
                                    return value;
                                }
                                break;
                            }
                        }
                    }

                    // 其他情况，执行load
                    llvm_ir::Type loadType = convertType(operand.type);
                    if (loadType == llvm_ir::Type::Void) {
                        loadType = llvm_ir::Type::I32;  // 默认为i32
                    }
                    return builder.createLoad(value, getNextTemp(), loadType);
                }
            }
            return value;
        }

        // 创建新的值
        llvm_ir::Type type = convertType(operand.type);
        return getOrCreateValue("%" + operand.name, type);
    }

    Value* LLVMIRGenerator::convertOperandForCall(const ir::Operand& operand) {
        // 检查是否是常量
        if (operand.type == ir::Type::IntLiteral) {
            return new ConstantInt(std::stoi(operand.name));
        } else if (operand.type == ir::Type::FloatLiteral) {
            return new ConstantFloat(std::stod(operand.name));
        }

        // 检查是否是全局变量
        if (operand.name.find("s0_") == 0) {
            std::string globalName = "@" + operand.name;
            // 创建全局变量引用（指针类型）
            Value* globalPtr = new Value(globalName, llvm_ir::Type::Ptr);

            // 检查是否是全局数组变量
            auto it = valueMap.find(operand.name);
            if (it != valueMap.end()) {
                if (auto globalVar = dynamic_cast<const GlobalVariable*>(it->second)) {
                    if (globalVar->arraySize > 0) {
                        // 全局数组变量，直接返回数组指针，不需要load
                        return globalPtr;
                    }
                }
            }

            // 普通全局变量需要生成load指令加载值
            llvm_ir::Type loadType = convertType(operand.type);
            if (loadType == llvm_ir::Type::Void) {
                loadType = llvm_ir::Type::I32;  // 默认为i32
            }
            return builder.createLoad(globalPtr, getNextTemp(), loadType);
        }

        // 检查是否已经存在
        auto it = valueMap.find(operand.name);
        if (it != valueMap.end()) {
            Value* value = it->second;

            // 对于函数调用，如果值是指针类型（特别是getptr指令的结果），直接返回指针
            if (value->type == llvm_ir::Type::Ptr) {
                // 检查是否是getptr指令的结果或数组指针
                if (auto allocaInst = dynamic_cast<const AllocaInst*>(value)) {
                    if (allocaInst->arraySize > 0) {
                        // 数组变量，返回数组指针而不是load
                        return value;
                    }
                    // 非数组的alloca，需要load，使用alloca指向的实际类型
                    llvm_ir::Type loadType = allocaInst->allocatedType;
                    Value* result = builder.createLoad(value, getNextTemp(), loadType);
                    result->type = loadType;  // 设置正确的类型
                    return result;
                } else {
                    // 不是AllocaInst，可能是getptr指令的结果或函数参数
                    // 检查是否是当前函数的参数
                    if (currentFunction) {
                        for (const auto& param : currentFunction->parameters) {
                            if (param->name == "%" + operand.name) {
                                // 是函数参数，如果参数类型是指针，直接返回，不需要load
                                if (operand.type == ir::Type::IntPtr || operand.type == ir::Type::FloatPtr) {
                                    return value;
                                }
                                break;
                            }
                        }
                    }

                    // 可能是getptr指令的结果，直接返回指针（不要load）
                    // 这对于getarray和putarray等函数调用很重要
                    return value;
                }
            }
            return value;
        }

        // 创建新的值
        llvm_ir::Type type = convertType(operand.type);
        return getOrCreateValue("%" + operand.name, type);
    }

    std::string LLVMIRGenerator::getNextTemp() { return "%tmp" + std::to_string(tempCounter++); }

    Value* LLVMIRGenerator::getOrCreateValue(const std::string& name, llvm_ir::Type type) {
        auto it = valueMap.find(name);
        if (it != valueMap.end()) {
            return it->second;
        }

        // 创建新值（这里简化处理，实际需要更复杂的逻辑）
        Value* value = new Value(name, type);
        valueMap[name] = value;
        return value;
    }

    Value* LLVMIRGenerator::getArrayPointer(const ir::Operand& operand) {
        // 检查是否是全局变量
        if (operand.name.find("s0_") == 0) {
            std::string globalName = "@" + operand.name;
            // 对于全局数组变量，直接返回全局变量指针
            return new Value(globalName, llvm_ir::Type::Ptr);
        }

        // 检查是否已经存在
        auto it = valueMap.find(operand.name);
        if (it != valueMap.end()) {
            Value* value = it->second;
            // 如果是指针类型（alloca的结果），直接返回指针，不进行load
            if (value->type == llvm_ir::Type::Ptr) {
                return value;
            }
            return value;
        }

        // 创建新的指针值
        llvm_ir::Type type = llvm_ir::Type::Ptr;
        return getOrCreateValue("%" + operand.name, type);
    }

    void LLVMIRGenerator::generateBuiltinFunctions() {
        // 生成memset函数声明 - 接受ptr, i32, i64参数，返回ptr
        auto memsetFunc = std::make_unique<Function>("memset", llvm_ir::Type::Ptr);
        auto memsetParam1 = std::make_unique<Value>("", llvm_ir::Type::Ptr);  // destination
        auto memsetParam2 = std::make_unique<Value>("", llvm_ir::Type::I32);  // value
        auto memsetParam3 = std::make_unique<Value>("", llvm_ir::Type::I64);  // size
        memsetFunc->parameters.push_back(memsetParam1.release());
        memsetFunc->parameters.push_back(memsetParam2.release());
        memsetFunc->parameters.push_back(memsetParam3.release());
        module->addFunction(std::move(memsetFunc));

        // 生成printf函数声明
        auto printfFunc = std::make_unique<Function>("printf", llvm_ir::Type::I32);
        module->addFunction(std::move(printfFunc));

        // 生成scanf函数声明
        auto scanfFunc = std::make_unique<Function>("scanf", llvm_ir::Type::I32);
        module->addFunction(std::move(scanfFunc));

        // 生成putint函数声明 - 接受一个i32参数
        auto putintFunc = std::make_unique<Function>("putint", llvm_ir::Type::Void);
        auto putintParam = std::make_unique<Value>("", llvm_ir::Type::I32);
        putintFunc->parameters.push_back(putintParam.release());
        module->addFunction(std::move(putintFunc));

        // 生成putch函数声明 - 接受一个i32参数
        auto putchFunc = std::make_unique<Function>("putch", llvm_ir::Type::Void);
        auto putchParam = std::make_unique<Value>("", llvm_ir::Type::I32);
        putchFunc->parameters.push_back(putchParam.release());
        module->addFunction(std::move(putchFunc));

        // 生成putfloat函数声明 - 接受一个float参数
        auto putfloatFunc = std::make_unique<Function>("putfloat", llvm_ir::Type::Void);
        auto putfloatParam = std::make_unique<Value>("", llvm_ir::Type::Float);
        putfloatFunc->parameters.push_back(putfloatParam.release());
        module->addFunction(std::move(putfloatFunc));

        // 生成getint函数声明
        auto getintFunc = std::make_unique<Function>("getint", llvm_ir::Type::I32);
        module->addFunction(std::move(getintFunc));

        // 生成getch函数声明
        auto getchFunc = std::make_unique<Function>("getch", llvm_ir::Type::I32);
        module->addFunction(std::move(getchFunc));

        // 生成getfloat函数声明
        auto getfloatFunc = std::make_unique<Function>("getfloat", llvm_ir::Type::Float);
        module->addFunction(std::move(getfloatFunc));

        // 生成getarray函数声明 - 接受一个i32*参数，返回i32
        auto getarrayFunc = std::make_unique<Function>("getarray", llvm_ir::Type::I32);
        auto getarrayParam = std::make_unique<Value>("", llvm_ir::Type::Ptr);
        getarrayFunc->parameters.push_back(getarrayParam.release());
        module->addFunction(std::move(getarrayFunc));

        // 生成getfarray函数声明 - 接受一个float*参数，返回i32
        auto getfarrayFunc = std::make_unique<Function>("getfarray", llvm_ir::Type::I32);
        auto getfarrayParam = std::make_unique<Value>("", llvm_ir::Type::Ptr);
        getfarrayFunc->parameters.push_back(getfarrayParam.release());
        module->addFunction(std::move(getfarrayFunc));

        // 生成putarray函数声明 - 接受i32和i32*参数
        auto putarrayFunc = std::make_unique<Function>("putarray", llvm_ir::Type::Void);
        auto putarrayParam1 = std::make_unique<Value>("", llvm_ir::Type::I32);
        auto putarrayParam2 = std::make_unique<Value>("", llvm_ir::Type::Ptr);
        putarrayFunc->parameters.push_back(putarrayParam1.release());
        putarrayFunc->parameters.push_back(putarrayParam2.release());
        module->addFunction(std::move(putarrayFunc));

        // 生成putfarray函数声明 - 接受i32和float*参数
        auto putfarrayFunc = std::make_unique<Function>("putfarray", llvm_ir::Type::Void);
        auto putfarrayParam1 = std::make_unique<Value>("", llvm_ir::Type::I32);
        auto putfarrayParam2 = std::make_unique<Value>("", llvm_ir::Type::Ptr);
        putfarrayFunc->parameters.push_back(putfarrayParam1.release());
        putfarrayFunc->parameters.push_back(putfarrayParam2.release());
        module->addFunction(std::move(putfarrayFunc));

        // 生成starttime和stoptime函数声明
        auto starttimeFunc = std::make_unique<Function>("starttime", llvm_ir::Type::Void);
        module->addFunction(std::move(starttimeFunc));

        auto stoptimeFunc = std::make_unique<Function>("stoptime", llvm_ir::Type::Void);
        module->addFunction(std::move(stoptimeFunc));

        // 生成_sysy_starttime和_sysy_stoptime函数声明 - 接受i32参数
        auto sysy_starttimeFunc = std::make_unique<Function>("_sysy_starttime", llvm_ir::Type::Void);
        auto sysy_starttimeParam = std::make_unique<Value>("", llvm_ir::Type::I32);
        sysy_starttimeFunc->parameters.push_back(sysy_starttimeParam.release());
        module->addFunction(std::move(sysy_starttimeFunc));

        auto sysy_stoptimeFunc = std::make_unique<Function>("_sysy_stoptime", llvm_ir::Type::Void);
        auto sysy_stoptimeParam = std::make_unique<Value>("", llvm_ir::Type::I32);
        sysy_stoptimeFunc->parameters.push_back(sysy_stoptimeParam.release());
        module->addFunction(std::move(sysy_stoptimeFunc));
    }

    void LLVMIRGenerator::generateGlobalVariables(const ir::Program& program) {
        for (const auto& globalVal : program.globalVal) {
            llvm_ir::Type elemtype = convertTypeForArrayElement(globalVal.val.type);

            Value* initializer = nullptr;

            if (globalVal.maxlen > 0) {
                // 数组类型全局变量
                llvm_ir::Type arrayType = llvm_ir::Type::Array;
                initializer = new ConstantInt(0);  // 数组零初始化
                auto gvar = std::make_unique<GlobalVariable>(globalVal.val.name, arrayType, initializer, false, globalVal.maxlen, elemtype);
                valueMap[globalVal.val.name] = gvar.get();
                module->addGlobalVariable(std::move(gvar));
            } else {
                // 普通类型全局变量
                // 简化的初始化处理
                if (elemtype == llvm_ir::Type::I32) {
                    initializer = new ConstantInt(0);
                } else if (elemtype == llvm_ir::Type::Float) {
                    initializer = new ConstantFloat(0.0);
                }

                auto gvar = std::make_unique<GlobalVariable>(globalVal.val.name, elemtype, initializer, false);
                valueMap[globalVal.val.name] = gvar.get();
                module->addGlobalVariable(std::move(gvar));
            }
        }
    }

    Value* LLVMIRGenerator::createConstant(const std::string& value, llvm_ir::Type type) {
        switch (type) {
        case llvm_ir::Type::I32:
            return new ConstantInt(std::stoi(value));
        case llvm_ir::Type::Float:
            return new ConstantFloat(std::stod(value));
        default:
            return new ConstantInt(0);
        }
    }

    void LLVMIRGenerator::createBasicBlocks(const ir::Function& irFunc) {
        // 分析所有跳转指令，确定跳转目标
        std::set<int> jumpTargets;

        // 指令0总是一个基本块的开始（入口点）
        jumpTargets.insert(0);

        for (size_t i = 0; i < irFunc.InstVec.size(); ++i) {
            const auto& inst = irFunc.InstVec[i];

            if (inst->op == ir::Operator::_goto) {
                if (!inst->op1.name.empty() && inst->op1.name != "null") {
                    // 条件跳转：true分支和false分支（下一条指令）
                    int trueTarget = parseJumpTarget(inst->des.name, i);
                    if (trueTarget >= 0 && trueTarget < (int)irFunc.InstVec.size()) {
                        jumpTargets.insert(trueTarget);
                    }
                    // false分支是下一条指令
                    if (i + 1 < irFunc.InstVec.size()) {
                        jumpTargets.insert(i + 1);
                    }
                } else {
                    // 无条件跳转
                    int target = parseJumpTarget(inst->des.name, i);
                    if (target >= 0 && target < (int)irFunc.InstVec.size()) {
                        jumpTargets.insert(target);
                    }
                    // 无条件跳转后的下一条指令也是一个新基本块的开始
                    if (i + 1 < irFunc.InstVec.size()) {
                        jumpTargets.insert(i + 1);
                    }
                }
            }
        }

        // 检查是否有跳转指向指令0
        bool hasJumpToZero = jumpTargets.count(0) > 0;

        // 为每个跳转目标创建基本块
        for (int target : jumpTargets) {
            if (target == 0 && hasJumpToZero) {
                // 特殊处理：为指令0创建label_0基本块，但使用特殊键存储
                std::string blockName = "label_0";
                auto block = std::make_unique<BasicBlock>(blockName);
                blockMap[-1] = block.get();  // 使用-1作为label_0的特殊键
                currentFunction->addBasicBlock(std::move(block));
            } else if (target != 0 && blockMap.find(target) == blockMap.end()) {
                std::string blockName = "label_" + std::to_string(target);
                auto block = std::make_unique<BasicBlock>(blockName);
                blockMap[target] = block.get();
                currentFunction->addBasicBlock(std::move(block));
            }
        }

        // 如果有跳转指向指令0，我们需要特殊处理
        // 因为入口基本块不能有前驱，所以我们需要在入口基本块和指令0之间建立正确的关系
        if (hasJumpToZero && blockMap.count(0) > 0) {
            // 标记需要在入口基本块末尾添加跳转到label_0
            // 这个标记将在generateFunction中使用
        }
    }

    int LLVMIRGenerator::parseJumpTarget(const std::string& target, int currentPC) {
        // 解析 [pc,] 格式的跳转目标
        if (target.find("[pc,") == 0) {
            size_t commaPos = target.find(',');
            size_t endPos = target.find(']');
            if (commaPos != std::string::npos && endPos != std::string::npos) {
                std::string offsetStr = target.substr(commaPos + 1, endPos - commaPos - 1);
                // 去除空格
                offsetStr.erase(0, offsetStr.find_first_not_of(" \t"));
                offsetStr.erase(offsetStr.find_last_not_of(" \t") + 1);

                int offset = std::stoi(offsetStr);
                return currentPC + offset;
            }
        }

        // 如果是简单的数字，也是相对跳转，需要加上当前PC
        try {
            int offset = std::stoi(target);
            return currentPC + offset;
        } catch (...) {
            return -1;  // 无效目标
        }
    }

    void LLVMIRGenerator::preallocateVariables(const ir::Function& irFunc) {
        // 分析所有指令，收集需要分配存储空间的变量
        std::set<std::string> storageVariables;
        std::set<std::string> functionParams;

        // 收集函数参数名称
        for (const auto& param : irFunc.ParameterList) {
            functionParams.insert(param.name);
#if (DEBUG_TCA_2_LLVM)
            std::cout << "[DEBUG] Function parameter: " << param.name << std::endl;
#endif
        }

        for (const auto& inst : irFunc.InstVec) {
            // 为mov指令的目标分配空间
            if (inst->op == ir::Operator::mov || inst->op == ir::Operator::fmov) {
                if (!inst->des.name.empty() && (inst->des.name.find("s") == 0 || inst->des.name.find("t_") == 0)) {
                    // 包括所有s开头的局部变量（s1_, s2_, s4_, s8_等）和t_开头的临时变量
                    storageVariables.insert(inst->des.name);
#if (DEBUG_TCA_2_LLVM)
                    std::cout << "[DEBUG] Added storage variable: " << inst->des.name << std::endl;
#endif
                }
            }
            // 对于def指令，也需要分配存储空间
            else if (inst->op == ir::Operator::def || inst->op == ir::Operator::fdef) {
                if (!inst->des.name.empty()) {
                    storageVariables.insert(inst->des.name);
                }
            }
            // 对于可能被后续mov指令使用的变量，也要预分配
            // 检查是否有同名变量在后续被mov使用
            else if (inst->op == ir::Operator::add || inst->op == ir::Operator::sub || inst->op == ir::Operator::mul || inst->op == ir::Operator::div || inst->op == ir::Operator::eq || inst->op == ir::Operator::neq || inst->op == ir::Operator::lss || inst->op == ir::Operator::gtr || inst->op == ir::Operator::leq || inst->op == ir::Operator::geq || inst->op == ir::Operator::_and || inst->op == ir::Operator::_or) {
                // 检查这个变量是否在后续指令中被mov使用
                if (!inst->des.name.empty() && inst->des.name.find("t_") == 0) {
                    // 扫描后续指令，看是否有mov使用这个变量
                    for (size_t j = 0; j < irFunc.InstVec.size(); ++j) {
                        const auto& laterInst = irFunc.InstVec[j];
                        if ((laterInst->op == ir::Operator::mov || laterInst->op == ir::Operator::fmov) && laterInst->des.name == inst->des.name) {
                            storageVariables.insert(inst->des.name);
                            break;
                        }
                    }
                }
            }
        }

        // 为需要存储的变量预先分配alloca
        for (const std::string& varName : storageVariables) {
#if (DEBUG_TCA_2_LLVM)
            std::cout << "[DEBUG] Processing storage variable: " << varName << std::endl;
#endif

            // 推断变量类型
            llvm_ir::Type allocType = inferVariableType(irFunc, varName);

            if (functionParams.count(varName)) {
#if (DEBUG_TCA_2_LLVM)
                std::cout << "[DEBUG] " << varName << " is a function parameter, creating alloca" << std::endl;
#endif
                // 这是一个需要被修改的函数参数
                // 为它分配一个alloca，使用不同的名称避免冲突
                std::string allocaName = varName + "_addr";
                Value* alloca = builder.createAlloca(allocType, "%" + allocaName);

                // 获取函数参数值并存储到alloca中
                auto paramIt = valueMap.find(varName);
                if (paramIt != valueMap.end()) {
                    Value* paramValue = paramIt->second;
                    builder.createStore(paramValue, alloca);
                    // 更新valueMap，让变量名指向alloca而不是参数
                    valueMap[varName] = alloca;
#if (DEBUG_TCA_2_LLVM)
                    std::cout << "[DEBUG] Stored parameter " << varName << " to alloca " << allocaName << std::endl;
#endif
                }
            } else {
                // 检查是否已经分配过（对于非函数参数的变量）
                if (valueMap.find(varName) != valueMap.end()) {
                    continue;  // 跳过已经分配的变量
                }
#if (DEBUG_TCA_2_LLVM)
                std::cout << "[DEBUG] " << varName << " is a regular variable, creating alloca" << std::endl;
#endif
                // 普通局部变量
                Value* alloca = builder.createAlloca(allocType, "%" + varName);
                valueMap[varName] = alloca;
            }
        }
    }

    llvm_ir::Type LLVMIRGenerator::inferVariableType(const ir::Function& irFunc, const std::string& varName) {
        // 简化类型推断逻辑，避免复杂的类型不一致问题

        // 查找变量的第一个定义或赋值
        for (const auto& inst : irFunc.InstVec) {
            if (inst->des.name == varName) {
                // 根据操作类型推断类型
                switch (inst->op) {
                case ir::Operator::fadd:
                case ir::Operator::fsub:
                case ir::Operator::fmul:
                case ir::Operator::fdiv:
                case ir::Operator::fmov:
                    // 浮点运算结果是浮点类型
                    return llvm_ir::Type::Float;

                case ir::Operator::mov:
                    // 对于mov操作，检查源操作数的类型
                    if (inst->op1.type == ir::Type::FloatLiteral) {
                        return llvm_ir::Type::Float;
                    } else {
                        // 其他情况（包括整数字面量、变量等）都使用i32
                        return llvm_ir::Type::I32;
                    }
                    break;

                default:
                    // 所有其他操作（包括比较、逻辑运算、算术运算）都使用i32
                    // 这样可以避免类型不一致的问题
                    return llvm_ir::Type::I32;
                }
            }
        }

        // 如果没有找到定义，默认返回i32
        return llvm_ir::Type::I32;
    }

}  // namespace llvm_ir
