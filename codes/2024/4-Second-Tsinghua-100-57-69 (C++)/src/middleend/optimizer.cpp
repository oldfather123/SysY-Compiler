#include "middleend/optmizer.hpp"
#include "middleend/cfg.hpp"
#include "middleend/mem2reg.hpp"
#include "middleend/constant_propagation.hpp"
#include "middleend/remove_useless_inst.hpp"
#include "middleend/copy_propagation.hpp"
#include "middleend/function_inline.hpp"
#include "middleend/value_numbering.hpp"
#include "middleend/algebra_simplification.hpp"
#include "middleend/global_value_numbering.hpp"
#include "middleend/global_code_motion.hpp"
#include "middleend/remove_recursive_tail_call.hpp"
#include "middleend/loop_unroll.hpp"
#include "middleend/dead_code_eliminate.hpp"
#include "middleend/split_critical_edges.hpp"
#include "middleend/gep_deconstruction.hpp"
#include "middleend/osr.hpp"
#include "middleend/constant_embedding.hpp"
#include "middleend/arrayssa.hpp"
#include "middleend/remove_useless_memory.hpp"
#include "middleend/remove_one_store.hpp"
#include "middleend/global2local.hpp"
#include "middleend/loop_interchange.hpp"
#include "middleend/loop_fusion.hpp"
#include "middleend/loop_parallel.hpp"
#include "middleend/expand_instruction.hpp"
#include "middleend/induction_var_simplification.hpp"
#include "middleend/range_analysis.hpp"
// #include "middleend/loop_dowhile.hpp"
#include "middleend/remove_useless_call.hpp"

namespace middleend {

Optimizer::Optimizer(ir::Module *module, bool o1) {
    if (!o1) return;
    DeadCodeEliminate dd1(module);
    Global2Local g2l(module);
    Mem2Reg mem2reg(module);

    mark_pure_functions(module);

    ValueNumbering vn0(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        RemoveUselessInst ru(func);
    }
    {
        ArraySSA array_ssa(module);
        array_ssa.array2reg();
        for (auto func : *module->get_functions()) {
            CFG *cfg = new CFG(func);
            GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
            GlobalCodeMotion gcm(cfg);
        }
        LoopFusion lp(module);
        array_ssa.reg2array();
    }

    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        ConstantPropagation cp(func);
        RemoveUselessInst ru(func);
        cfg->rebuild();
        RemoveRecursiveTailCall rrtc(func);
        CopyPropagation cpp(func);
        // func->print(std::cout);
        RangeAnalysis ra(func);
        // func->print(std::cout);
        RemoveUselessInst ru1(func);
    }
    // return;
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        RemoveUselessInst ru1(func);
    }
    // return;
    DeadCodeEliminate dd1_5(module);
    RemoveUselessCall ruc(module);
    FunctionInline fi1(module);
    remove_useless_function(module);
    // Global2Local g2l1(module);
    // if (g2l1.has_new_alloca) Mem2Reg mem2reg1(module);
    // mark_pure_functions(module);
    // bool parallel_success = false;
    // for (auto func : *module->get_functions()) {
    //     CopyPropagation cpp(func);
    //     // func->print(std::cout);
    //     LoopInterchange lic(func);
    //     // func->print(std::cout);
    //     LoopUnroll lur(func);
    //     // func->print(std::cout);
    //     // LoopInterchange lic(func);
    //     LoopParallel parallel(func, module);
    //     // func->print(std::cout);
    //     CFG *cfg = new CFG(func);
    //     ConstantPropagation cp(func);
    //     CopyPropagation cpp2(func);
    //     RemoveUselessInst ru(func);
    //     cfg->rebuild();
    //     // if (parallel.success) { parallel_success = true; return; }
    // }
    for (auto func : *module->get_functions()) {
        CopyPropagation cpp(func);
        LoopUnroll lur(func);
        LoopParallel parallel(func, module);
        CFG *cfg = new CFG(func);
        ConstantPropagation cp(func);
        CopyPropagation cpp2(func);
        RemoveUselessInst ru(func);
        cfg->rebuild();
        if (parallel.success && lur.success) return;
        if (parallel.cnt >= 10) return;
    }
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        RemoveUselessInst ru1(func);
    }
    // if (!parallel_success)
        DeadCodeEliminate dd2(module);
    RemoveOneStore ros1(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        RemoveUselessInst ru1(func);
    }
    ValueNumbering vn1(module);
    {
        ArraySSA array_ssa(module);
        array_ssa.array2reg();
        for (auto func : *module->get_functions()) {
            CFG *cfg = new CFG(func);
            GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
            GlobalCodeMotion gcm(cfg);
        }
        array_ssa.reg2array();
    }
    {
        ArraySSA array_ssa(module);
        array_ssa.array2reg();
        for (auto func : *module->get_functions()) {
            CFG *cfg = new CFG(func);
            RemoveUselessMemory rum(cfg);
        }
        array_ssa.reg2array();
    }
    RemoveOneStore ros2(module);
    // module->print(std::cout);
    // exit(0);
    AlgebraSimplification as1(module);
    FunctionInline fi2(module);
    remove_useless_function(module);
    // Global2Local g2l2(module);
    // if (g2l2.has_new_alloca) Mem2Reg mem2reg2(module);
    // mark_pure_functions(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        ElementPtrDeconstruct gep_deconstruct(func);
        ConstantPropagation cp(func);
        CopyPropagation cpp(func);
        cfg->rebuild();
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        RemoveUselessInst ru(func);
        cfg->rebuild();
        // if (!parallel_success)
            OSR osr(func);
        ConstantEmbedding ce(func);
        RemoveUselessInst ru2(func);
    }
    AlgebraSimplification as2(module);
    ValueNumbering vn2(module);
    FunctionInline fi3(module);
    remove_useless_function(module);
    // Global2Local g2l3(module);
    // if (g2l3.has_new_alloca) Mem2Reg mem2reg3(module);
    // mark_pure_functions(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        ConstantEmbedding ce(func);
        RemoveUselessInst ru(func);
        cfg->rebuild();
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
        InductionVarSimplification ivs(cfg);
    }
    AlgebraSimplification as3(module);
    ValueNumbering vn3(module);
    FunctionInline fi4(module);
    remove_useless_function(module);
    // Global2Local g2l4(module);
    // if (g2l4.has_new_alloca) Mem2Reg mem2reg4(module);
    // mark_pure_functions(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        ConstantEmbedding ce(func);
        RemoveUselessInst ru(func);
        cfg->rebuild();
        GlobalValueNumbering gvn(cfg); // gvn后必须接gcm
        GlobalCodeMotion gcm(cfg);
    }
    AlgebraSimplification as4(module);
    // if (!parallel_success)
        DeadCodeEliminate dd(module);
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        CopyPropagation cpp(func);
        cpp.address_resolve();
        RemoveUselessInst ru(func);
        cfg->rebuild();
        // func->print(std::cout);
    }
    for (auto func : *module->get_functions()) {
        CFG *cfg = new CFG(func);
        ExpandInstruction ei(cfg);
        RemoveUselessInst ru(func);
        cfg->rebuild();
    }
    // if (!parallel_success)
        DeadCodeEliminate dd4(module);
    // SplitCriticalEdges spe(module);
    // if (check_assignment(module)) std::cout << "no assign" << std::endl;
    // if (check_use_def(module)) std::cout << "no use def" << std::endl;
}

// bool Optimizer::check_assignment(ir::Module *module) {
//     for (auto &function : *module->get_functions()) {
//         for (auto bb : *function->get_basic_blocks()) {
//             for (auto & inst : *bb->get_instructions()) {
//                 if (auto assign = dynamic_cast<ir::instruction::Assign *>(inst)) {
//                     std::cout << "Warning: " << assign->to_str() << " is an assign statement." << std::endl;
//                     return false;
//                 }
//             }
//         }
//     }
//     return true;
// }

// bool Optimizer::check_use_def(ir::Module *module) {
//     for (auto &function : *module->get_functions()) {
//         CFG *cfg = new CFG(function);
//         UseDefAnalysis *ud = new UseDefAnalysis(cfg);
//         for (auto bb : *function->get_basic_blocks()) {
//             for (auto & inst : *bb->get_instructions()) {
//                 for (auto src: *inst->get_src()) {
//                     if (ud->get_defsets().count(src) == 0) {
//                         std::cout << "Warning: " << src->to_str() << " in " << function->get_name() << " is not defined." << std::endl;
//                         return false;
//                     }
//                 }
//             }
//         }
//     }
//     return true;
// }

} // namespace middleend