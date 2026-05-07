#include <fstream>
#include <string>
#include <vector>
#include "PassDriver/passDriver.h"
#include "Frontend/ASTVisitor.h"
#include "SysyLexer.h"
#include "SysyParser.h"
#include "antlr4-runtime.h"

using namespace anuc;
using namespace std;
using namespace antlr4;
string targetFileName;
string sourceFileName;
bool llvmIr = false;
void processCommandLine(int argc, char* argv[]) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-S") == 0) {
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            targetFileName = argv[i + 1];
            // 在这里获取 -o 后面的输出文件名，并执行相应的操作
            i++;  // 跳过输出文件名参数
        }
        else if(strcmp(argv[i], "-O1") == 0 ) {

        }
        else if(strcmp(argv[i], "-l") == 0 ) {
            llvmIr = true;
        }
        else {
            sourceFileName = argv[i];
        }
    }

}
int main(int argc , char* argv[]) {
    processCommandLine(argc, argv);
    std::ifstream in;
    in.open(sourceFileName);
    ANTLRInputStream input(in);
    SysyLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();
    SysyParser parser(&tokens);
    SysyParser::CompUnitContext *root = parser.compUnit();
    ASTVisitor visitor;
    visitor.visitCompUnit(root);
    auto M = visitor.getModule();
    //if(llvmIr) M->print("../l.ll");
    auto Builder = visitor.getBuilder();
    PassDriver driver(std::move(M), std::move(Builder), targetFileName);
    driver.run();
    return 0;
}