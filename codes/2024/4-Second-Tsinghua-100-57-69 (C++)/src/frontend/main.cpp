#include <iostream>
#include <fstream>
#include <exception>
#include <chrono>

#include "frontend/SysYLexer.h"
#include "frontend/SysYParser.h"
#include "frontend/AstVisitor.hpp"
#include "frontend/Typer.hpp"
#include "frontend/genIr.hpp"

#include "common/argparse.hpp"
#include "common/utils.hpp"

#include "middleend/optmizer.hpp"
#include "middleend/llvm.hpp"
#include "backend/program.hpp"
#include "backend/passes.hpp"

using namespace antlr4;
using namespace std;

struct ErrorListener : public antlr4::BaseErrorListener {
  virtual void syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t line,
                           size_t pos, const string &err_msg,
                           std::exception_ptr) override {
    auto msg = "line " + to_string(line) + ":" + to_string(pos) + " " + err_msg;
    throw antlr4::ParseCancellationException(msg);
  }
};

std::istream& gen_input(int argc, char* argv[]) {
  for(int i = 1; i < argc; ) {
    auto const arg = std::string_view{argv[i]};
    if (arg == "-o") {
      i += 2;
    } else if (!arg.empty() && arg.front() == '-') {
      ++i;
    } else {
      static std::ifstream ifs;
      ifs.open(argv[i]);
      return ifs;
    }
  }
  return std::cin;
}

std::ostream& output_path(int argc, char* argv[]) {
  for(int i = 1; i < argc; ) {
    auto const arg = std::string_view{argv[i]};
    if (arg == "-o") {
      static std::ofstream ifs;
      ifs.open(argv[i+1]);
      return ifs;
    } else if (!arg.empty()) {
      ++i;
    }
  }
  return std::cout;
}

void delete_space(string &s) 
 {
    if (s.empty()) 
    {
        return ;
    }
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
}

void modify_input_file(std::istream& inp, std::ostream& modified_inp) {
    std::string line;
    int current_line = 1;

    while (std::getline(inp, line)) {
        //std::cout << "dd" <<  line << "dd" << std::endl;
        delete_space(line);
        if (line == "starttime();" || line == "stoptime();") {
            modified_inp << line.substr(0, line.size() - 3) << "(" << current_line << ");" << std::endl;
        } else {
            modified_inp << line << std::endl;
        }
        current_line++;
    }
}

int main(int argc, char* argv[]) {
    auto beforeTime = std::chrono::steady_clock::now();
    std::stringstream modified_input_content;
    auto &inp = gen_input(argc, argv);
    auto &oup = output_path(argc, argv);
    modify_input_file(inp, modified_input_content);
    ErrorListener error_listener;
    //std::cout << modified_input_content.str();
    ANTLRInputStream input(modified_input_content);
    ANTLRInputStream inputStream(input);

    SysYLexer lexer(&inputStream);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&error_listener);

    CommonTokenStream tokens(&lexer);
    SysYParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&error_listener);

    try{
      auto tree = parser.compUnit();

      // build ast tree
      frontend::AstVisitor ast_visitor;
      ast_visitor.visitCompUnit(tree);
      auto& ast = ast_visitor.compileUnit();
      if(has_option(argc, argv, "--ast")){
        ast.print(cout, 0);
        return 0;
      }

      frontend::Typer typer;
      typer.visit_compile_unit(ast);
      
      frontend::GenIr *genIr = new frontend::GenIr();
      middleend::ir::Module *module = genIr->transform(ast);

      bool main_ok1 = false;
      for (auto func : *module->get_functions()) {
        if (func->get_name() == "main") {
          main_ok1 = true;
          break;
        }
      }
      if (!main_ok1) {
        return 1;
      }

      bool o1 = false;
      if (has_option(argc, argv, "-O1")) o1 = true;
      middleend::Optimizer optimizer = middleend::Optimizer(module, o1);

      bool main_ok2 = false;
      for (auto func : *module->get_functions()) {
        if (func->get_name() == "main") {
          main_ok2 = true;
          break;
        }
      }
      if (!main_ok2) {
        return 2;
      }

      if(has_option(argc, argv, "--ir")){
        module->print(cout);
        return 0;
      }

      if (has_option(argc, argv, "--llvm")) {
        middleend::LLVM llvm(module);
        llvm.print_llvm(cout);
        return 0;
      }

      backend::riscv::Program *prog = new backend::riscv::Program(module);
      backend::riscv::backend_passes(prog);

      bool main_ok3 = false;
      for (auto func : prog->functions()) {
        if (func->name() == "main") {
          main_ok3 = true;
          break;
        }
      }
      if (!main_ok3) {
        return 3;
      }

      // if(has_option(argc, argv, "--test")){
      //   prog->test(cout);
      //   return 0;
      // }

      // if(has_option(argc, argv, "-o")){
        prog->gen_asm(oup);
        return 0;
      // }

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 6;
    }

    return 0;
}