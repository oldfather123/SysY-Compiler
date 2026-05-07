#include <iostream>
#include <fstream>
#include <string>

// 使用 extern "C" 来调用C语言编写的函数和访问其全局变量
// extern "C"
// {
#include "ast.h"
#include "symbol.h"
#include "semantic.h" // 假设这个文件负责语义分析和填充符号表
#include "sysy.tab.h"

    extern ASTNode *root_node;
    extern FILE *yyin;
    extern int yyparse();
// }

#include "ir_generator.h"
#include "IRBuilder.h"
#include "ast_optimizer.h"
#include "AsmGenerator.h"
#include "AsmGenerator.h"

int main(int argc, char **argv) {
    std::string input_file;
    std::string output_file;
    bool generate_asm = false;
    int opt_level = 0;  // Default optimization level

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-S") {
            generate_asm = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg.find(".sy") != std::string::npos) {
            input_file = arg;
        } else if (arg == "-O1") {
            opt_level = 1;
        } else if (arg == "-O2") {
            opt_level = 2;
        } else if (arg == "-O3") {
            opt_level = 3;
        }
    }

    // Check required arguments
    if (input_file.empty()) {
        std::cerr << "Usage: compiler -S [-O1|-O2|-O3] -o <output.s> <input.sy>" << std::endl;
        return 1;
    }

    if (generate_asm && output_file.empty()) {
        // Default output filename: replace .sy with .s
        output_file = input_file.substr(0, input_file.length() - 3) + ".s";
    }

    FILE *src = fopen(input_file.c_str(), "r");
    if (!src) {
        perror("Cannot open source file");
        return 1;
    }

    yyin = src;

    // --- 步骤 1: 语法分析 ---
    int parse_result = yyparse();
    fclose(src);

    if (parse_result != 0 || root_node == nullptr) {
        std::cerr << "语法分析失败。" << std::endl;
        return 1;
    }

    // AST optimization based on optimization level
    ASTOptimizer optimizer;
    // optimizer.setOptimizationLevel(opt_level);  // You'll need to add this method
    optimizer.analyzeVarUsage(root_node);
    root_node = optimizer.optimize(root_node);

    // --- 步骤 2: 语义分析 (填充符号表) ---
    // 你的语义检查器至关重要，它为IR生成器提供了必要的类型信息
    check_semantics(root_node);

    // --- 步骤 3: IR 生成 ---
    IRBuilder ir_builder;
    std::unique_ptr<MyIR::IRUnit> ir_unit;
    try {
        ir_unit = ir_builder.build(root_node);
    } catch (const std::exception &e) {
        std::cerr << "IR生成过程中发生错误: " << e.what() << std::endl;
        return 1;
    }

    // --- 步骤4: 创建优化器并运行优化Pass
    // MyIR::IROptimizer IRoptimizer;
    // IRoptimizer.run(ir_unit.get());

    // --- 步骤 5: 生成RISC-V汇编 ---
    if (generate_asm) {
        AsmGenerator asmgen;
        std::string riscv_asm = asmgen.generate(*ir_unit);
        
        // 将汇编代码写入输出文件
        std::ofstream asm_file(output_file);
        if (!asm_file) {
            std::cerr << "无法打开输出文件: " << output_file << std::endl;
            return 1;
        }
        asm_file << riscv_asm;
        asm_file.close();
    }

    // --- 清理 ---
    free_ast(root_node);

    return 0;
}