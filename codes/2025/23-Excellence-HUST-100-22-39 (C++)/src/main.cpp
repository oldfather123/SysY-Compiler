#include <iostream>
#include <fstream>
#include "ast.hpp"
#include "ir_module.hpp"
#include "ir_generator.hpp"
#include "opt.hpp"
#include "LT.hpp"
#include "Mem2Reg.hpp"
#include "ConstantAnalysis.hpp"
#include "InstCombine.hpp"
#include "DCE.hpp"
#include "SimplifyCFG.hpp"
#include "CSE.hpp"
#include "FuncInline.hpp"
#include "TRE.hpp"
#include "riscv_generator.hpp"

extern unique_ptr<CompUnitAST> root;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char **argv) {
    //======================================  Argument Parser ======================================
    if(argc < 2) {
        cout << "Compiler Usage: " << argv[0] << " [options] <filename>\n"
                  << "Options:\n"
                  << "  -S          Print assembly code\n"
                  << "  -I          Print intermediate representation (IR) code\n"
                  << "  -o <file>   Output to specified file (default: stdout)\n"
                  << "  -O          Enable optimization\n";
        return -1;
    }
    char* src_file = nullptr;
    bool print_ir = false, print_asm = false, isO1 = false;
    string output = "-";
    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'S':
                    print_asm = true;
                    break;
                case 'I':
                    print_ir = true;
                    break;
                case 'o':
                    if(i + 1 < argc) {
                        output = argv[++i];
                    } else {
                        cout << "Error: -o option requires a filename argument." << endl;
                        return -1;
                    }
                    break;
                case 'O':
                    isO1 = true;
                    break;
                default:
                    cout << "Unknown option: " << argv[i] << endl;
                    return -1;
            }
        } else {
            src_file = argv[i];
        }
    }
    yyin = fopen(src_file, "r");
    if(yyin == nullptr) {
        cout << "yyin open " << src_file << " failed" << endl;
        return -1;
    }
    //======================================  Front-end  ======================================
    yyparse();
    IRGenerator ir_generator;
    root->accept(ir_generator);
    Module* module = ir_generator.get_module();
    //======================================  Middle-end  ======================================
    // TODO: Pass Manager
    if(isO1) {
        LengauerTarjan(module).execute(); // 构建支配树信息
        Mem2Reg(module).execute(); // 消除局部单个变量的 alloca 指令，转为 phi 指令，构造 SSA 形式中间代码
        // TODO: 局部数组变量的 alloca 指令消除
        ConstantAnalysis(module).execute(); // 常量折叠、常量传播、稀疏条件常量传播
        InstCombine(module).execute(); // 合并指令
        DeadCodeElimination(module).execute(); // 死代码删除
        LengauerTarjan(module).execute();
        SimplifyCFG(module).execute(); // 简化控制流图，主要删除不可达基本块和合并“直线”基本块
        // TODO: 循环优化
        ConstantAnalysis(module).execute();
        DeadCodeElimination(module).execute();
        LengauerTarjan(module).execute();
        SimplifyCFG(module).execute();
        CSE(module).execute(); // 公共子表达式删除
        FuncInline(module).execute(); // 函数内联 // TODO: 指令克隆时的操作数使用关系
        ConstantAnalysis(module).execute(); // 内联后常数参数的常量分析
        TailRecurEliminate(module).execute(); // 尾递归消除
        // SimplifyCFG(module).execute(); // 再次简化控制流图
    }
    if(print_ir) {
        ofstream ir_out(output);
        module->print(ir_out);
    }
    //======================================  Back-end  ======================================
    if(print_asm) {
        ofstream riscv_out(output);
        RiscvGenerator riscv_generator(module);
        riscv_generator.generate(riscv_out);
    }
    return 0;
}