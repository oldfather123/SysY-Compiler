#ifndef GEECEECEE_LIB_HPP
#define GEECEECEE_LIB_HPP

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include "backend/opt/calculate_opt.hpp"
#include "backend/opt/cfg_opt/block_inline.hpp"
#include "backend/opt/cfg_opt/block_re_sort.hpp"
#include "backend/opt/cfg_opt/simplify_cfg.hpp"
#include "backend/riscv/rv_peephole.hpp"
#include "backend/riscv/rv_reg_allocator.hpp"
#include "backend/riscv/rv_reordering.hpp"
#include "backend/riscv/rv_translator.hpp"
#include "config.hpp"
#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/visitor.hpp"
#include "ir/mod.hpp"
#include "log.hpp"
#include "opt/engine.hpp"
#include "opt/pass.hpp"

inline int run(const Config &config) {
    logger::set_log_level(logger::LogLevel::INFO);  // TODO(Xingkun) make this inside Config
    // read input file content
    std::ifstream file(config.input_path);
    if (!file.is_open()) {
        std::cerr << "error: could not open file " << config.input_path << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_code = buffer.str();
#ifdef ENABLE_LOG
    auto path = logger::log_to_file("source_code.txt", source_code);
    logger::INFO("source code is saved to ", path);
#endif

    frontend::lexer::Lexer lexer(source_code);
    auto parser = frontend::parser::Parser(&lexer);
    auto ast = parser.parse();
    auto comp_unit = std::move(std::get<frontend::ast::ASTNodePtr>(ast));

    auto *module = new ir::Module();

    frontend::visitor::Visitor visitor(module);
    visitor.visit(comp_unit);

    if (config.llvm_ir) {
        std::ofstream file(config.llvm_ir_path());
        file << module->to_string();
    }

    opt::opt_engine.init();
    opt::opt_engine.set_cycle_times(5);  // TODO(Xingkun) make this configurable
    opt::opt_engine.compile(module);

    if (config.llvm_ir) {
        std::ofstream file(config.optimized_llvm_ir_path());
        file << module->to_string();
    }

    opt::pass::RemovePhi().run(module);
    opt::pass::RemoveUnreachableInstructions().run(module);
    opt::pass::ControlFlowGraphAnalyzation().run(module);
    opt::pass::RemoveUnreachableBasicBlocks().run(module);
    opt::pass::LengauerTarjanDominanceAnalyzation().run(module);
    opt::pass::LoopAnalyzation().run(module);

    if (config.llvm_ir) {
        std::ofstream file(config.llvm_ir_after_remove_phi_path());
        file << module->to_string();
    }

    // RISC-V backend translation and code generation
    if (config.assembly) {
        // git pull github master; git push origin master
        // python3 checker.py function -c ../cmake-build-debug/compiler
        // sysc-build/sysc -S -o -O1 testcase.sysy
        logger::INFO("Starting RISC-V backend translation...");
        // Translate IR to RISC-V
        auto *rv_module = backend::riscv::RVTranslator::translate(module);
        logger::INFO("RISC-V translation completed");

        // logger::INFO("Generating assembly output(after translate)...");
        // std::ofstream asm_after_translate_file(config.asm_after_translate_path());
        // asm_after_translate_file << rv_module->emit();
        // asm_after_translate_file.close();
        // logger::INFO("Assembly output(after translate) written to ", config.asm_after_translate_path());

        logger::INFO("Starting reordering...");
        backend::riscv::ReOrdering reordering(rv_module);
        reordering.run();
        logger::INFO("Reordering completed...");

        // logger::INFO("Generating assembly output(after reorder)...");
        // std::ofstream asm_after_reorder_file(config.asm_after_reorder_path());
        // asm_after_reorder_file << rv_module->emit();
        // asm_after_reorder_file.close();
        // logger::INFO("Assembly output(after reorder) written to ", config.asm_after_reorder_path());

        logger::INFO("Starting data flow peephole...");
        backend::riscv::PeepHole peephole(rv_module);
        peephole.before_ra_run();
        logger::INFO("Data flow peephole completed...");

        logger::INFO("Starting calculate opt...");
        backend::opt::CalculateOpt::run_before_ra(rv_module);
        logger::INFO("Calculate opt completed...");

        // logger::INFO("Generating assembly output(not allocated)...");
        // std::ofstream asm_not_allocated_file(config.asm_not_allocated_path());
        // asm_not_allocated_file << rv_module->emit();
        // asm_not_allocated_file.close();
        // logger::INFO("Assembly output(not allocated) written to ", config.asm_not_allocated_path());

        logger::INFO("Starting register allocation...");
        backend::riscv::RVRegAllocator allocator;
        allocator.allocate(*rv_module);
        logger::INFO("Register allocation completed");

        // logger::INFO("Generating assembly output(after allocated)...");
        // std::ofstream asm_after_allocated_file(config.asm_after_allocated_path());
        // asm_after_allocated_file << rv_module->emit();
        // asm_after_allocated_file.close();
        // logger::INFO("Assembly output(after allocated) written to ", config.asm_after_allocated_path());

        logger::INFO("Starting final peephole...");
        peephole.after_ra_run();
        logger::INFO("Final peephole completed...");

        logger::INFO("Starting block inlining...");
        backend::opt::cfg_opt::BlockInline::run(rv_module);
        logger::INFO("Block inlining completed...");

        logger::INFO("Starting block resorting...");
        backend::opt::cfg_opt::BlockReSort::block_sort(rv_module);
        logger::INFO("Block resorting completed...");

        logger::INFO("Starting CFG optimization...");
        backend::opt::cfg_opt::SimplifyCFG::run(rv_module);
        logger::INFO("CFG optimization completed...");

        // rv_module->simulate_random_context();
        // rv_module->check_memory_offsets();

        // Generate and write assembly output
        logger::INFO("Generating assembly output...");
        std::ofstream asm_file(config.asm_path());
        asm_file << rv_module->emit();
        asm_file.close();
        logger::INFO("Assembly output written to ", config.asm_path());
    }

    delete module;
    return 0;
}

#endif
