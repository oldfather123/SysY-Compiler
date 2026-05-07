#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>
#include <string>
#include "antlr4-runtime.h"
#include "SysyParser.h"
#include "SysyLexer.h"
#include "SysyBaseVisitor.h"
#include "SysyBaseListener.h"
#include "AstVisitor.h"
#include "IRVisitor.h"  
#include "RVbuilder.h"
#include "IR.h"

using namespace std;
using namespace antlr4;
using namespace frontend;

int main(int argc, const char* argv[]) {
    bool print_ast = true;
    std::string inputfilename = argv[4];
    std::string outputfilename = argv[3];
    std::ofstream fout;
    std::ostream *out;

    ifstream stream(inputfilename); //读入测试用例 
    ANTLRInputStream input(stream);  
    SysyLexer lexer(&input);  //词法解析
    CommonTokenStream tokens(&lexer);  //生成TokenStream
    SysyParser parser(&tokens);   //语法解析
    auto root = parser.compUnit();   //得到AST的根节点

    frontend::AstVisitor visitor;   
    visitor.visitCompUnit(root);   //从根节点开始遍历
    auto ast = visitor.compileUnit();   
    ast->print(std::cout,0);   //打印AST

    IR::IRVisitor test;
    test.visit(*ast); //遍历

    std::unique_ptr<Module> module_ptr = test.getModule(); 

    std::string IRcode = module_ptr.get()->print(); 
    std::cout << IRcode << std::endl; //打印IR

    auto riscv = new IRtoRISCV();
    const std::string RiscvCode = riscv->buildRISCV(module_ptr.get()); //生成RISCV
    std::cout << RiscvCode << std::endl; //打印RISCV代码

    fout.open(outputfilename);
    out = &fout;
    *out << RiscvCode << std::endl; //输出到文件中

    delete riscv;
    return 0;
}
