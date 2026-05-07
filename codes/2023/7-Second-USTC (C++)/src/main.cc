#include <string>
#include <iostream>

#include "sysY_driver.hh"
#include "sysY_ast.hh"
#include "sysY_irbuilder.hh"

#include "Pass.hh"
#include "DominateTree.hh"
#include "Mem2Reg.hh"
#include "Check.hh"
#include "ActiveVar.hh"
#include "CFGAnalyse.hh"
#include "ConstProp.hh"
#include "LIR.hh"
#include "DeadCodeEli.hh"
#include "DeadCodeEliWithBr.hh"
#include "StrengthReduction.hh"
#include "LoopInvariant.hh"
#include "AlgeSimplify.hh"
#include "LocalComSubExprEli.hh"
#include "FuncInline.hh"
#include "LoopExpansion.hh"
#include "DeadStoreEli.hh"
#include "TailRecursionElim.hh"
#include "InstructionScheduling.hh"
#include "LoopSearch.hh"
#include "LoopInfo.hh"
#include "LoopStrengthReduction.hh"
#include "RedundantInstsEli.hh"
#include "AgressiveConstProp.hh"
#include "VarLoopExpansion.hh"
#include "AgressiveLocalComSubExprEli.hh"
#include "RecSeq.hh"

#include "sysY_asbuilder.hh"

using namespace std::literals::string_literals;

bool print_ast = false;
bool print_mir = false;
bool hasm_emit = false;
bool optimize = true;
bool asm_gen = true;


std::string input_path;
std::string target_path;

bool no_target_path = true;

void print_help(std::string exe_name)
{
    std::cout << "Usage: " << exe_name
              << " [ -h | --help ] [ -o <output file> ] [ -print_mir ] [-asm_gen] [-emit_hasm] [-O] <input file>"
              << std::endl;
}

bool parse_args(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] == "-h"s || argv[i] == "--help"s)
        {
            print_help(argv[0]);
            return false;
        }
        else if (argv[i] == "-print_mir"s)
        {
            print_mir = true;
        }
        else if (argv[i] == "-S"s)
        {
            asm_gen = true;
        }
        else if (argv[i] == "-S"s)
        {
            hasm_emit = true;
        }
        else if (argv[i] == "-O1"s)
        {
            optimize = true;
        }
        else if (argv[i] == "-o"s)
        {
            if (i + 1 < argc)
            {
                target_path = argv[i + 1];
                no_target_path = false;
                i += 1;
            }
            else
            {
                print_help(argv[0]);
                return false;
            }
        }
        else
        {
            if (input_path.empty())
            {
                input_path = argv[i];
            }
            else
            {
                print_help(argv[0]);
                return false;
            }
        }
    }

    if (input_path.empty())
    {
        print_help(argv[0]);
        return false;
    }

    if (target_path.empty())
    {
        auto pos = input_path.rfind('.');
        if (pos == std::string::npos)
        {
            std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
            return false;
        }
        else
        {
            if (input_path.substr(pos) != ".sy")
            {
                std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
                return false;
            }
            target_path = input_path.substr(0, pos);
        }
    }

    return true;
}

void emit_ir(Module *m)
{
    auto IR = m->print();
    std::ofstream output_stream;
    std::string output_file;

    if(no_target_path) {
        output_file = target_path + ".ll";
    } else {
        output_file = target_path;
    }

    output_stream.open(output_file, std::ios::out);
    output_stream << "; ModuleID = 'sysY'\n";
    output_stream << "source_filename = \"" + input_path + "\"\n\n";
    output_stream << IR;
    output_stream.close();

}

void emit_hasm(HAsmModule *m)
{
    auto hasm_code = m->print();
    std::ofstream output_stream;
    std::string output_file;

    if(no_target_path) {
        output_file = target_path + ".hasm";
    } else {
        output_file = target_path;
    }
    output_stream.open(output_file, std::ios::out);
    output_stream << hasm_code;
    output_stream.close();
}

void emit_asm(HAsmModule *m)
{
    auto asm_code = m->get_asm_code();
    std::ofstream output_stream;
    std::string output_file;

    if(no_target_path) {
        output_file = target_path + ".s";
    } else {
        output_file = target_path;
    }
    output_stream.open(output_file, std::ios::out);
    output_stream << asm_code;
    output_stream.close();
}

int main(int argc, char *argv[])
{

    bool success = parse_args(argc, argv);

    if (!success)
        exit(-1);

    sysY_driver driver;
    ASTCompUnit *ast_root = driver.parse(input_path);

    if (print_ast)
    {
        ASTPrinter astprinter;
        ast_root->accept(astprinter);
    }

    sysY_irbuilder irbuilder;

    ast_root->accept(irbuilder);
    auto m = irbuilder.get_module();

    m->set_print_name();
    m->set_file_name(input_path);

    PassMgr passmgr(m.get());

    if (optimize) {
        //& 第一轮
        passmgr.add_pass<DeadStoreEli>();
        passmgr.add_pass<Mem2Reg>();
        passmgr.add_pass<ConstProp>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<RecSeq>();
        passmgr.add_pass<FuncInline>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<AgressiveLocalComSubExprEli>();
        passmgr.add_pass<LocalComSubExprEli>();
        passmgr.add_pass<AlgeSimplify>();
        passmgr.add_pass<LoopInvariant>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<LoopInvariant>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<LoopExpansion>();
        passmgr.add_pass<VarLoopExpansion>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        // passmgr.add_pass<Check>();

        //& 第二轮
        passmgr.add_pass<ConstProp>();
        passmgr.add_pass<AlgeSimplify>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<LoopInvariant>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<TailRecursionElim>();

        //& 第三轮
        passmgr.add_pass<LIR>();
        // passmgr.add_pass<LoopStrengthReduction>();
        passmgr.add_pass<LocalComSubExprEli>();
        passmgr.add_pass<ConstProp>();
        passmgr.add_pass<DeadStoreEli>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<StrengthReduction>();
        passmgr.add_pass<LocalComSubExprEli>();
        passmgr.add_pass<DeadCodeEliWithBr>();
        passmgr.add_pass<RedundantInstsEli>();
        passmgr.add_pass<AgressiveConstProp>();
        // passmgr.add_pass<InstructionScheduling>();
        passmgr.add_pass<ActiveVar>();
        passmgr.add_pass<CFGAnalyse>();
        
        // passmgr.add_pass<Check>();
        passmgr.execute();

        auto mptr = m.get();
        mptr->set_print_name();

        if(print_mir) {
            emit_ir(mptr);
        } else {
            sysY_asbuilder asbuilder(mptr);
            asbuilder.module_gen();
            if(hasm_emit) {
                emit_hasm(asbuilder.get_module());
            } else {
                emit_asm(asbuilder.get_module());
            }
        }  
    } else {
        if (asm_gen) {
            passmgr.add_pass<Mem2Reg>();
            passmgr.add_pass<LIR>();
            passmgr.add_pass<LocalComSubExprEli>();
            passmgr.add_pass<CFGAnalyse>();
            passmgr.add_pass<ActiveVar>();
            // passmgr.add_pass<Check>();
            passmgr.execute();
            auto mptr = m.get();
            mptr->set_print_name();
            sysY_asbuilder asbuilder(mptr);
            asbuilder.module_gen();
            if(hasm_emit) {
                emit_hasm(asbuilder.get_module());
            } else {
                emit_asm(asbuilder.get_module());
            }
        } else {
            passmgr.add_pass<DominateTree>();
            passmgr.add_pass<Mem2Reg>();
            passmgr.add_pass<CFGAnalyse>();
            passmgr.add_pass<ActiveVar>();
            passmgr.add_pass<LIR>();

            passmgr.execute();

            emit_ir(m.get());
        }
    }

    return 0;
}