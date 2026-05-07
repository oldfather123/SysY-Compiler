#include "./parser/SyntaxTree.hpp"
#include "./ir/irbuilder.hpp"
#include "./ir/ir_printer.hpp"
#include "./riscv/program_builder.hpp"
// #include "passes/pass.hpp"
#include "./passes/pass_type.hpp"
#include "./passes/pass_manager.hpp"
#include "./utils/arg.hpp"
#include <fstream>
// #include <iostream>
// #include <istream>
#include <memory>
// #include <stdlib.h>

bool using_rookie = false;

ast::SyntaxTree syntax_tree;
int main(int argc, char* argv[]){
    Config arg(argc, argv);
    if(arg.rookie_allocator) {
        using_rookie = true;
    }

    auto infile = std::ifstream(arg.input_file);
    auto outfile = std::ofstream(arg.output_file);

    ast::parse_file(infile);
    // syntax_tree.print();
    std::shared_ptr<ir::IrBuilder> irbuilder = std::make_shared<ir::IrBuilder>();
    syntax_tree.accept(*irbuilder);
    
    // std::shared_ptr<ir::IrPrinter> irprinter = std::make_shared<ir::IrPrinter>();
    // irbuilder->compunit->accept(*irprinter);

    // if(arg.optimise) {
        Passes::PassManager pass_manager(irbuilder->compunit);
        pass_manager.add_pass(Passes::PassType::MEM2REG);
        pass_manager.add_pass(Passes::PassType::DEAD_CODE_ELIMINATION);
        pass_manager.add_pass(Passes::PassType::TAIL_CALL);
        pass_manager.add_pass(Passes::LOOP_INVARIANTS);
        pass_manager.add_pass(Passes::PassType::DEAD_CODE_ELIMINATION);
        if(arg.show_passes) {
            std::shared_ptr<ir::IrPrinter> irprinter = std::make_shared<ir::IrPrinter>();
            pass_manager.enable_printer(irprinter);
        }
        pass_manager.run();
    // }


    // std::cout << "----------------------------------------------------------" << std::endl;

    // std::shared_ptr<ir::IrPrinter> irprinter = std::make_shared<ir::IrPrinter>();
    // irbuilder->compunit->accept(*irprinter);

    if(arg.emitllvm) {
        ptr<ir::IrPrinter> irprinter = std::make_shared<ir::IrPrinter>(outfile);
        irbuilder->compunit->accept(*irprinter);
    }
    else if(arg.emitasm) {
        //下面是后端的部分
        std::shared_ptr<LoongArch::ProgramBuilder> progbuilder= std::make_shared<LoongArch::ProgramBuilder>();
        irbuilder->compunit->accept(*progbuilder);
        auto prog = progbuilder->prog;

        prog->get_asm(outfile);
    }

    // irbuilder->compunit->accept(*irprinter);
    // delete (ast::compunit_syntax *)syntax_tree.root;    // double free or corruption (out)，即代表Bison也会释放这个root？
}