#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include "arg.hpp"

using std::operator""s;

void Config::parse_cmd_line() {
    exe_name = argv[0];
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help();
        } else if (argv[i] == "-o"s) {
            if (output_file.empty() && i + 1 < argc) {
                output_file = argv[i + 1];
                i += 1;
            } else {
                print_err("bad output file");
            }
        } else if (argv[i] == "-emit-llvm"s) {
            emitllvm = true;
        } else if (argv[i] == "-S"s) {
            emitasm = true;
        } else if (argv[i] == "-O1"s) {
            optimise = true;
        } else if(argv[i] == "-show-passes"s) {
			show_passes = true;
		} else if(argv[i] == "-rookie-allocator"s) {
            rookie_allocator = true;
        } else {
            if (input_file.empty()) {
                input_file = argv[i];
            } else {
                std::string err =
                    "unrecognized command-line option \'"s + argv[i] + "\'"s;
                print_err(err);
            }
        }
    }
}

void Config::check() {
    if (input_file.empty()) {
        print_err("no input file");
    }
    if (input_file.extension() != ".sy") {
        print_err("file format not recognized");
    }
    if (emitllvm and emitasm) {
        print_err("emit llvm and emit asm both set");
    }
    if (not emitllvm and not emitasm) {
        print_err("not supported: generate executable file directly");
    }
	if(show_passes and not optimise) {
		print_err("optimise hasn't activate yet");
	}
    if (output_file.empty()) {
        output_file = input_file.stem();
        if (emitllvm) {
            output_file.replace_extension(".ll");
        } else if (emitasm) {
            output_file.replace_extension(".s");
        }
    }
    if(rookie_allocator) {
        std::clog << "Experimental: Now we are using rookie allocator to allocate registers, and it may make mistakes." << std::endl;
    }
}

void Config::print_help() const {
    std::cout << "Usage: " << exe_name
              << " [-h|--help] [-o <target-file>] [-O1] [-show-passes] [-rookie-allocator] [-emit-llvm] [-S] "
                 "<input-file>"
              << std::endl;
    std::cout << "\t-optimise\t\t启用所有实现的优化"
              << std::endl
              << "\t-show-passes\t\t展示每一遍优化的过程，需要先启用优化"
              << std::endl
              << "\t-rookie-allocator\t采用之前（中期答辩前）实现的菜鸟寄存器分配算法，否则默认采用后来（中期答辩后）实现的简陋的但相对更为标准的图染色分配算法"
              << std::endl;
    exit(0);
}

void Config::print_err(const std::string &msg) const {
    std::cout << exe_name << ": " << msg << std::endl;
    exit(-1);
}
