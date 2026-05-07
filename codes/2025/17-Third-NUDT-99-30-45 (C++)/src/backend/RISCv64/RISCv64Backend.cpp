#include "RISCv64Backend.h"
#include "RISCv64ISel.h"
#include "RISCv64RegAlloc.h"
#include "RISCv64LinearScan.h"
#include "RISCv64SimpleRegAlloc.h"
#include "RISCv64BasicBlockAlloc.h"
#include "RISCv64AsmPrinter.h"
#include "RISCv64Passes.h"
#include <sstream>
#include <future>
#include <chrono>
#include <atomic>
#include <memory>
#include <thread>
#include <iostream>
namespace sysy {

// 顶层入口
std::string RISCv64CodeGen::code_gen() {
    return module_gen();
}

unsigned RISCv64CodeGen::getTypeSizeInBytes(Type* type) {
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
            // assert(false && "Unsupported type for size calculation.");
            return 0; // 对于像Label或Void这样的类型，返回0是合理的
    }
}


void printInitializer(std::stringstream& ss, const ValueCounter& init_values) {
    for (size_t i = 0; i < init_values.getValues().size(); ++i) {
        auto val = init_values.getValues()[i];
        auto count = init_values.getNumbers()[i];
        if (auto constant = dynamic_cast<ConstantValue*>(val)) { 
            for (unsigned j = 0; j < count; ++j) {
                if (constant->isInt()) {
                    ss << "    .word " << constant->getInt() << "\n";
                } else {
                    float f = constant->getFloat();
                    uint32_t float_bits = *(uint32_t*)&f;
                    ss << "    .word " << float_bits << "\n";
                }
            }
        }
    }
}

std::string RISCv64CodeGen::module_gen() {
    std::stringstream ss;
    
    // --- 步骤1：将全局变量(GlobalValue)分为.data和.bss两组 ---
    std::vector<GlobalValue*> data_globals;
    std::vector<GlobalValue*> bss_globals;

    for (const auto& global_ptr : module->getGlobals()) {
        GlobalValue* global = global_ptr.get();
        
        // 使用更健壮的逻辑来判断是否为大型零初始化数组
        bool is_all_zeros = true;
        const auto& init_values = global->getInitValues();
        
        // 检查初始化值是否全部为0
        if (init_values.getValues().empty()) {
            // 如果 ValueCounter 为空，GlobalValue 的构造函数会确保它是零初始化的
            is_all_zeros = true;
        } else {
            for (auto val : init_values.getValues()) {
                if (auto const_val = dynamic_cast<ConstantValue*>(val)) {
                    if (!const_val->isZero()) {
                        is_all_zeros = false;
                        break;
                    }
                } else {
                    // 如果初始值包含非常量（例如，另一个全局变量的地址），则不认为是纯零初始化
                    is_all_zeros = false;
                    break;
                }
            }
        }

        // 使用 getTypeSizeInBytes 检查总大小是否超过阈值 (16个整数 = 64字节)
        Type* allocated_type = global->getType()->as<PointerType>()->getBaseType();
        unsigned total_size = getTypeSizeInBytes(allocated_type);
        
        // bool is_large_zero_array = is_all_zeros && (total_size > 64);

        // if (is_large_zero_array) {
        //     bss_globals.push_back(global);
        // } else {
        //     data_globals.push_back(global);
        // }
        data_globals.push_back(global);
    }

    // --- 步骤2：生成 .bss 段的代码 ---
    if (!bss_globals.empty()) {
        ss << ".bss\n";
        for (GlobalValue* global : bss_globals) {
            Type* allocated_type = global->getType()->as<PointerType>()->getBaseType();
            unsigned total_size = getTypeSizeInBytes(allocated_type);

            ss << "    .align 3\n";
            ss << ".globl " << global->getName() << "\n";
            ss << ".type " << global->getName() << ", @object\n";
            ss << ".size " << global->getName() << ", " << total_size << "\n";
            ss << global->getName() << ":\n";
            ss << "    .space " << total_size << "\n";
        }
    }
    
    // --- 步骤3：生成 .data 段的代码 ---
    if (!data_globals.empty() || !module->getConsts().empty()) {
        ss << ".data\n";

        // a. 处理普通的全局变量 (GlobalValue)
        for (GlobalValue* global : data_globals) {
            Type* allocated_type = global->getType()->as<PointerType>()->getBaseType();
            unsigned total_size = getTypeSizeInBytes(allocated_type);

            ss << "    .align 3\n";
            ss << ".globl " << global->getName() << "\n";
            ss << ".type " << global->getName() << ", @object\n";
            ss << ".size " << global->getName() << ", " << total_size << "\n";
            ss << global->getName() << ":\n";
            bool is_all_zeros = true;
            const auto& init_values = global->getInitValues();
            if (init_values.getValues().empty()) {
                is_all_zeros = true;
            } else {
                for (auto val : init_values.getValues()) {
                    if (auto const_val = dynamic_cast<ConstantValue*>(val)) {
                        if (!const_val->isZero()) {
                            is_all_zeros = false;
                            break;
                        }
                    } else {
                        is_all_zeros = false;
                        break;
                    }
                }
            }
            if (is_all_zeros) {
                ss << "    .zero " << total_size << "\n";
            } else {
                // 对于有非零初始值的变量，保持原有的打印逻辑。
                printInitializer(ss, global->getInitValues());
            }
        }

        // b. 处理全局常量 (ConstantVariable)
        for (const auto& const_ptr : module->getConsts()) {
            ConstantVariable* cnst = const_ptr.get();
            Type* allocated_type = cnst->getType()->as<PointerType>()->getBaseType();
            unsigned total_size = getTypeSizeInBytes(allocated_type);

            ss << "    .align 3\n";
            ss << ".globl " << cnst->getName() << "\n";
            ss << ".type " << cnst->getName() << ", @object\n";
            ss << ".size " << cnst->getName() << ", " << total_size << "\n";
            ss << cnst->getName() << ":\n";
            printInitializer(ss, cnst->getInitValues());
        }
    }

    // --- 步骤4：处理函数 (.text段) 的逻辑 ---
    if (!module->getFunctions().empty()) {
        ss << ".text\n";
        for (const auto& func_pair : module->getFunctions()) {
            if (func_pair.second.get() && !func_pair.second->getBasicBlocks().empty()) {
                ss << function_gen(func_pair.second.get());
                if (DEBUG) std::cerr << "Function: " << func_pair.first << " generated.\n";
            }
        }
    }
    return ss.str();
}

std::string RISCv64CodeGen::function_gen(Function* func) {
    // 阶段 1: 指令选择 (sysy::IR -> LLIR with virtual registers)
    RISCv64ISel isel;
    std::unique_ptr<MachineFunction> mfunc = isel.runOnFunction(func);
    // 第一次调试打印输出
    std::stringstream ss_after_isel;
    RISCv64AsmPrinter printer_isel(mfunc.get());
    printer_isel.run(ss_after_isel, true);
    // DEBUG = 1;
    if (DEBUG) {
        std::cerr << "====== Intermediate Representation after Instruction Selection ======\n" 
        << ss_after_isel.str();
    }
    // DEBUG = 0;
    // 阶段 2: 消除帧索引 (展开伪指令，计算局部变量偏移)
    EliminateFrameIndicesPass efi_pass;
    efi_pass.runOnMachineFunction(mfunc.get());

    if (DEBUG) {
        std::cerr << "====== stack info after eliminate frame indices  ======\n";
        mfunc->dumpStackFrameInfo(std::cerr);
        std::stringstream ss_after_eli;
        printer_isel.run(ss_after_eli, true);
        std::cerr << "====== LLIR after eliminate frame indices ======\n" 
        << ss_after_eli.str();
    }

    // 阶段 2.1: 除法强度削弱优化 (Division Strength Reduction)
    DivStrengthReduction div_strength_reduction;
    div_strength_reduction.runOnMachineFunction(mfunc.get());

    // // 阶段 2.2: 指令调度 (Instruction Scheduling)
    // PreRA_Scheduler scheduler;
    // scheduler.runOnMachineFunction(mfunc.get());

    // 阶段 3: 物理寄存器分配 (Register Allocation)
    bool allocation_succeeded = false;

    // 尝试迭代图着色 (IRC)
    if (!irc_failed) {
        if (DEBUG) std::cerr << "Attempting Register Allocation with Iterated Register Coloring (IRC)...\n";        
        RISCv64RegAlloc irc_alloc(mfunc.get());
        auto stop_flag = std::make_shared<std::atomic<bool>>(false);
        auto future = std::async(std::launch::async, &RISCv64RegAlloc::run, &irc_alloc, stop_flag);
        std::future_status status = future.wait_for(std::chrono::seconds(25));
        bool success_irc = false;
        if (status == std::future_status::ready) {
            try {
                if (future.get()) {
                    success_irc = true;
                } else {
                    std::cerr << "Warning: IRC explicitly returned failure for function '" << func->getName() << "'.\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: IRC allocation threw an exception: " << e.what() << std::endl;
            }
        } else if (status == std::future_status::timeout) {
            std::cerr << "Warning: IRC allocation timed out after 25 seconds. Requesting cancellation...\n";            
            stop_flag->store(true);
            try {
                future.get();
            } catch (const std::exception& e) {
                std::cerr << "Exception occurred during IRC thread shutdown after timeout: " << e.what() << std::endl;
            }
        }

        if (success_irc) {
            allocation_succeeded = true;
            if (DEBUG) std::cerr << "IRC allocation succeeded.\n";
        } else {
            std::cerr << "Info: Blacklisting IRC for subsequent functions and falling back.\n";
            irc_failed = true;
        }
    }
    
    // 尝试简单图着色 (SGC) 
    if (!allocation_succeeded) {
        // 如果是从IRC失败回退过来的，需要重新创建干净的mfunc和ISel
        RISCv64ISel isel_for_sgc;
        if (irc_failed) {
            if (DEBUG) std::cerr << "Info: Resetting MachineFunction for SGC attempt.\n";
            mfunc = isel_for_sgc.runOnFunction(func);
            EliminateFrameIndicesPass efi_pass_for_sgc;
            efi_pass_for_sgc.runOnMachineFunction(mfunc.get());
        }

        if (DEBUG) std::cerr << "Attempting Register Allocation with Simple Graph Coloring (SGC)...\n";

        bool sgc_completed_in_time = false;
        {
            RISCv64SimpleRegAlloc sgc_alloc(mfunc.get());
            auto future = std::async(std::launch::async, &RISCv64SimpleRegAlloc::run, &sgc_alloc);
            std::future_status status = future.wait_for(std::chrono::seconds(25));

            if (status == std::future_status::ready) {
                try {
                    future.get(); // 检查是否有异常
                    sgc_completed_in_time = true;
                    if (DEBUG) std::cerr << "SGC allocation completed successfully within the time limit.\n";
                } catch (const std::exception& e) {
                    std::cerr << "Error: SGC allocation threw an exception: " << e.what() << std::endl;
                }
            }
        }

        if (sgc_completed_in_time) {
            allocation_succeeded = true;
        } else {
            std::cerr << "Warning: SGC allocation timed out or failed for function '" << func->getName() 
                    << "'. Falling back.\n";
        }
    }

    // 如果都失败了，则使用基本块分配器 (BBA)
    if (!allocation_succeeded) {
        // 为BBA准备干净的mfunc和ISel
        std::cerr << "Info: Resetting MachineFunction for BBA fallback.\n";
        RISCv64ISel isel_for_bba;
        mfunc = isel_for_bba.runOnFunction(func);
        EliminateFrameIndicesPass efi_pass_for_bba;
        efi_pass_for_bba.runOnMachineFunction(mfunc.get());

        std::cerr << "Info: Using Basic Block Allocator as final fallback.\n";
        RISCv64BasicBlockAlloc bb_alloc(mfunc.get());
        bb_alloc.run();
    }
    
    if (DEBUG) {
        std::cerr << "====== stack info after reg alloc ======\n";
        mfunc->dumpStackFrameInfo(std::cerr);
    }

    // 阶段 3.1: 处理被调用者保存寄存器
    CalleeSavedHandler callee_handler;
    callee_handler.runOnMachineFunction(mfunc.get());

    if (DEBUG) {
        std::cerr << "====== stack info after callee handler ======\n";
        mfunc->dumpStackFrameInfo(std::cerr);
    }

    // 阶段 4: 窥孔优化 (Peephole Optimization)
    PeepholeOptimizer peephole;
    peephole.runOnMachineFunction(mfunc.get());

    // // 阶段 5: 局部指令调度 (Local Scheduling)
    // PostRA_Scheduler local_scheduler;
    // local_scheduler.runOnMachineFunction(mfunc.get());

    // 阶段 3.2: 插入序言和尾声
    PrologueEpilogueInsertionPass pei_pass;
    pei_pass.runOnMachineFunction(mfunc.get());

    // 阶段 3.3: 大立即数合法化
    LegalizeImmediatesPass legalizer;
    legalizer.runOnMachineFunction(mfunc.get());

    // 阶段 6: 代码发射 (Code Emission)
    std::stringstream ss;
    RISCv64AsmPrinter printer(mfunc.get());
    printer.run(ss);

    return ss.str();
}

} // namespace sysy