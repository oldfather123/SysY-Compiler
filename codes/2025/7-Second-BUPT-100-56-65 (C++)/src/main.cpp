#include <getopt.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "IR/IRPrinter.h"
#include "Pass/Analysis/CallGraph.h"
#include "Pass/Analysis/MemorySSA.h"
#include "Pass/Pass.h"
#include "Pass/Transform/ADCEPass.h"
#include "Pass/Transform/ComptimePass.h"
#include "Pass/Transform/GEPToByteOffsetPass.h"
#include "Pass/Transform/GVNPass.h"
#include "Pass/Transform/InlinePass.h"
#include "Pass/Transform/InstCombinePass.h"
#include "Pass/Transform/LICMPass.h"
#include "Pass/Transform/Mem2RegPass.h"
#include "Pass/Transform/SimplifyCFGPass.h"
#include "Pass/Transform/StoreGlobalize.h"
#include "Pass/Transform/StrengthReductionPass.h"
#include "Pass/Transform/TailRecursionOptimizationPass.h"
#include "Target.h"
#include "ir_gen.h"

void print_help() {
    std::cout << "Usage: a.out [options] <input_file>\n"
              << "Options:\n"
              << "  -o <output>    Specify output file\n"
              << "  -S             Generate assembly output\n"
              << "  -emit-ir       Generate IR output\n"
              << "  -O0            No optimization (default)\n"
              << "  -O1            Enable optimization\n"
              << "  -p <pipeline>  Custom optimization pipeline\n"
              << "  -h, --help     Show this help message\n"
              << "\n"
              << "Examples:\n"
              << "  a.out input.sy -o output.s -S\n"
              << "  a.out -o output.s input.sy -S\n"
              << "  a.out -S -o output.s input.sy -O1\n"
              << "  a.out -S -p mem2reg,inline input.sy -o output.s\n"
              << "\n"
              << "Available passes: mem2reg, inline, adce, instcombine, "
                 "simplifycfg, tailrec, strengthreduce\n"
              << "\n"
              << "GitHub: https://github.com/BUPT-a-out/\n";
}

int main(int argc, char* argv[]) {
    std::optional<std::string> input_file;
    std::optional<std::string> output_file;
    std::optional<std::string> custom_pipeline;
    bool assembly_output = false;
    bool emit_ir = false;
    int optimize_level = 0;

    static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                           {"emit-ir", no_argument, 0, 'e'},
                                           {0, 0, 0, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "o:SO:p:he", long_options,
                              nullptr)) != -1) {
        switch (opt) {
            case 'o':
                output_file = std::string{optarg};
                break;
            case 'S':
                assembly_output = true;
                break;
            case 'h':
                print_help();
                return 0;
            case 'O':
                if (optarg) {
                    std::string_view opt_level{optarg};
                    if (opt_level == "0") {
                        optimize_level = 0;
                    } else if (opt_level == "1") {
                        optimize_level = 1;
                    } else {
                        std::cerr << "Error: Invalid optimization level. Use "
                                     "-O0 or -O1\n";
                        return 1;
                    }
                } else {
                    std::cerr << "Error: Invalid optimization level. Use -O0 "
                                 "or -O1\n";
                    return 1;
                }
                break;
            case 'p':
                custom_pipeline = std::string{optarg};
                break;
            case 'e':
                emit_ir = true;
                break;
            default:
                std::cerr << "Try 'a.out --help' for more information.\n";
                return 1;
        }
    }

    if (optind < argc) {
        input_file = std::string{argv[optind]};
    }

    if (!input_file.has_value()) {
        std::cerr << "Error: No input file specified\n";
        std::cerr << "Try 'a.out --help' for more information.\n";
        return 1;
    }

    if (assembly_output && emit_ir) {
        std::cerr << "Error: Cannot use both -S and -emit-ir flags together.\n";
        return 1;
    }

    if (!assembly_output && !emit_ir) {
        std::cerr << "Error: Must specify either -S (assembly) or -emit-ir (IR "
                     "output).\n";
        return 1;
    }

    FILE* file_in = fopen(input_file->c_str(), "r");
    if (!file_in) {
        perror("Failed to open input file");
        return 1;
    }
    auto module = generate_IR(file_in);
    fclose(file_in);

    // Declare passManager and analysisManager with extended lifetime
    std::unique_ptr<midend::PassManager> passManager;
    midend::AnalysisManager* analysisManager = nullptr;

    if (optimize_level > 0 || custom_pipeline.has_value()) {
        passManager = std::make_unique<midend::PassManager>();
        auto& am = passManager->getAnalysisManager();
        analysisManager = &am;
        analysisManager->registerAnalysisType<midend::DominanceAnalysis>();
        analysisManager->registerAnalysisType<midend::PostDominanceAnalysis>();
        analysisManager->registerAnalysisType<midend::CallGraphAnalysis>();
        analysisManager->registerAnalysisType<midend::AliasAnalysis>();
        analysisManager->registerAnalysisType<midend::LoopAnalysis>();
        analysisManager->registerAnalysisType<midend::MemorySSAAnalysis>();
        midend::PassBuilder passBuilder;
        midend::InlinePass::setInlineThreshold(1000);
        midend::InlinePass::setMaxSizeGrowthThreshold(10000);
        passBuilder.registerPass<midend::Mem2RegPass>("mem2reg");
        passBuilder.registerPass<midend::InlinePass>("inline");
        passBuilder.registerPass<midend::ADCEPass>("adce");
        passBuilder.registerPass<midend::InstCombinePass>("instcombine");
        passBuilder.registerPass<midend::SimplifyCFGPass>("simplifycfg");
        passBuilder.registerPass<midend::GVNPass>("gvn");
        passBuilder.registerPass<midend::LICMPass>("licm");
        passBuilder.registerPass<midend::ComptimePass>("comptime");
        passBuilder.registerPass<midend::StoreGlobalize>("storeglobalize");
        passBuilder.registerPass<midend::GEPToByteOffsetPass>("gep2bytes");
        passBuilder.registerPass<midend::TailRecursionOptimizationPass>(
            "tailrec");
        passBuilder.registerPass<midend::StrengthReductionPass>(
            "strengthreduce");

        std::string pipeline;
        if (custom_pipeline.has_value()) {
            pipeline = *custom_pipeline;
        } else {
            pipeline = R"(
                mem2reg,tailrec,adce,simplifycfg
                (gvn,inline,licm,gvn)*5
                (storeglobalize,comptime)*1
                gep2bytes,(gvn,licm,instcombine,strengthreduce)*5
                mem2reg,adce,simplifycfg
            )";
        }

        auto parseResult =
            passBuilder.parsePassPipeline(*passManager, pipeline);
        if (!parseResult.success) {
            std::cerr << "Error: Failed to parse optimization pipeline\n";
            std::cerr << "  " << parseResult.errorMessage << "\n";

            if (parseResult.errorPosition < pipeline.length()) {
                std::cerr << "  Pipeline: " << pipeline << "\n";
                std::cerr << "  "
                          << std::string(parseResult.errorPosition + 10, ' ')
                          << "^\n";
            }
            return 1;
        }

        passManager->run(*module);

        if (!emit_ir) {
            std::cout << "Optimization results:\n";
            std::cout << midend::IRPrinter::toString(module.get()) << std::endl;
        }
    }

    if (emit_ir) {
        std::string ir_output = midend::IRPrinter::toString(module.get());
        if (output_file.has_value()) {
            std::ofstream out{*output_file};
            if (!out) {
                std::cerr << "Error: Cannot open output file: " << *output_file
                          << std::endl;
                return 1;
            }
            out << ir_output << std::endl;
        } else {
            std::cout << ir_output << std::endl;
        }
    } else {
        auto assembly = riscv64::RISCV64Target().compileToAssembly(
            *module, analysisManager);

        if (output_file.has_value()) {
            std::ofstream out{*output_file};
            if (!out) {
                std::cerr << "Error: Cannot open output file: " << *output_file
                          << std::endl;
                return 1;
            }
            out << assembly << std::endl;
        } else {
            std::cout << assembly << std::endl;
        }
    }

    return 0;
}