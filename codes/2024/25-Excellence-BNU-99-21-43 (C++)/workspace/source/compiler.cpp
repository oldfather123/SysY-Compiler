#include <iostream>
#include <assert.h>
#include "lexer.h"
#include "abstract_syntax_tree.h"
#include "passcontroller.h"
#include "AsmCode.h"
#include "parseargv.h"
#include <fstream>

IR::Module *ParserToIR(std::string code)
{
    IR::Module *module = new IR::Module("sysy");
    IR::IRBuilder *builder = new IR::IRBuilder();
    Lexer *lexer = new Lexer(code);
    Parser *parser = new Parser(lexer);
    parser->program_to_IR(module, builder);
    delete builder;
    delete lexer;
    delete parser;
    return module;
}

void Pass(bool opt, IR::Module *module, std::ostream &os)
{
    IR::PassController *passController = new IR::PassController();
    passController->run(opt, module);
}

void IRToAssembly(IR::Module *module, std::ostream &os)
{
    Backend::AsmCode *asmCode = new Backend::AsmCode(module);

    std::cout << "---------------- RISCV ----------------" << std::endl;
    std::string s = asmCode->emit();
    os << s;
    std::cout << s << std::endl;
    return;
}

bool isSpecifiedFile(const std::string &fileName, const std::string &suffix)
{
    if (fileName.size() <= suffix.size())
        return false;
    return fileName.substr(fileName.size() - suffix.size()) == suffix;
}

int main(int argc, char const *argv[])
{
    utils::ArgvParser parser;
    auto args = parser.run(argc, argv);

    // uesd for test specifed case
    // if (!isSpecifiedFile(args.inputFileName, "94_nested_loops.sy"))
    // {
    //     std::cerr << "Input file name: " << args.inputFileName << std::endl;
    //     return 1;
    // }

    std::fstream fs;
    fs.open(args.inputFileName, std::ios::in);
    assert(fs.is_open());
    std::string code(std::istreambuf_iterator<char>(fs), {});
    fs.close();

    IR::Module *res = nullptr;
    res = ParserToIR(code);
    if (args.optLevel == 1)
        Pass(true, res, std::cout);
    else
        Pass(false, res, std::cout);

    std::ofstream os;
    os.open(args.ASMFileName, std::ios::out);
    IRToAssembly(res, os);
    os.close();
    return 0;
}