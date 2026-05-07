#include "antlr4-runtime.h"
#include "SysyLexer.h"
#include "SysyParser.h"
#include "SysyVisitor.h"
#include "AstVisitor.h"
#include "IR.h"
#include "IRPasses.h"
#include "backend.h"
#include <iostream>
#include <string>

#pragma execution_character_set("utf-8")
#pragma comment(lib, "antlr4-runtime.lib") // 安装方法中静态库引用

int main(int argc, char **argv) {
    if (argc < 5) { // 确保正确的参数个数
        std::cerr << "Usage: compiler -S -o <asm_out_path> <source_path> [-O1 | -O0] [-o-ir <ir_out_path>]" << std::endl;
        return 1;
    }

    // 读取文件输入流
    std::ifstream in(argv[4]); // 使用命令行参数指定的文件路径
    if (!in) {                 // 确保文件可以被成功打开
        std::cerr << "Failed to open file: " << argv[4] << std::endl;
        return 1;
    }

    int optLevel;
    if (argc >= 6 && std::string(argv[5]) == "-O1") {
        dbgout << "Optimization level: O1" << std::endl;
        optLevel = 1;
    } else {
        dbgout << "Optimization level: O0" << std::endl;
        optLevel = 0;
    }

    antlr4::ANTLRInputStream input(in);
    SysyLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();

    dbgout << std::endl
           << "Token stream generated." << std::endl;

    // 输出token流
    SysyParser parser(&tokens);
    SysyParser::CompUnitContext *parsedTree = parser.compUnit();

    dbgout << std::endl
           << "Token stream parsed." << std::endl;

    // dbgout << antlr4::tree::Trees::toStringTree(parsedTree, &parser, true) << std::endl;
    // dbgout << parsedTree->getText() << std::endl;
    // dbgout << "children number is:" + parsedTree->children.size() << std::endl;

    AstVisitor visitor;

    auto astRoot = AS(visitor.visitCompUnit(parsedTree), Ptr<ast::CompUnit>);
#ifdef DEBUG
    astRoot->print(dbgout, 0);
#endif
    dbgout << std::endl
           << "AST generated." << std::endl;

    auto irBuilder = ir_builder::Builder::create();
    Ptr<ir::Module> irModule;
    try {
        irModule = astRoot->codegen(irBuilder);

        dbgout << std::endl
               << "IR generated." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error generating IR: " << e.what() << std::endl;
        return 1;
    }

    dbgout << std::endl
           << "Initial IR:" << std::endl
           << irModule->toString() << std::endl;

    try {
        ir::runIRPasses(irModule, optLevel);

        dbgout << std::endl
               << "IR transforms done." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error optimizing IR: " << e.what() << std::endl;
        return 1;
    }

    // dbgout << std::endl
    //        << "Transformed IR:" << std::endl
    //        << irModule->toString() << std::endl;

    if (argc >= 8 && std::string(argv[6]) == "-o-ir") {
        std::ofstream irOut(argv[7]);
        irOut << irModule->toString();
    }

    Ptr<backend::RiscModule> asmModule = std::make_shared<backend::RiscModule>();
    try {
        backend::runBackend(irModule, asmModule);
        dbgout << "ASM generated." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error generating ASM: " << e.what() << std::endl;
        return 1;
    }

    // dbgout << "ASM:" << std::endl
    //        << asmModule->toString();

    std::ofstream out(argv[3]);
    out << asmModule->toString();

    return 0;
}
