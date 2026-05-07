#ifndef GEECEECEE_CONFIG_HPP
#define GEECEECEE_CONFIG_HPP
#include <algorithm>
#include <stdexcept>
#include <string>

struct Config {
    std::string input_path;
    std::string output_path;
    bool assembly;
    bool llvm_ir;
    int opt_level;

    std::string asm_path() const { return output_path + ".s"; }
    std::string asm_after_allocated_path() const { return output_path + "_after_allocated.s"; }
    std::string asm_not_allocated_path() const { return output_path + "_no_allocated.s"; }
    std::string asm_after_translate_path() const { return output_path + "_after_translate.s"; }
    std::string asm_after_reorder_path() const { return output_path + "_after_reorder.s"; }
    std::string llvm_ir_path() const { return output_path + ".old.ll"; }
    std::string optimized_llvm_ir_path() const { return output_path + ".ll"; }
    std::string llvm_ir_after_remove_phi_path() const { return output_path + "_after_remove_phi.ll"; }

    explicit Config() {
        input_path = "testcase.sysy";
        output_path = "testcase";
        assembly = true;
        llvm_ir = false;
        opt_level = 0;
    }

    explicit Config(int argc, char **argv) : Config() {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-S") {
                assembly = true;
            } else if (arg == "-emit-llvm") {
                llvm_ir = true;
            } else if (arg == "-o" && i + 1 < argc) {
                std::string out = argv[++i];
                if (out.size() > 2 && out.substr(out.size() - 2) == ".s") {
                    out = out.substr(0, out.size() - 2);
                } else if (out.size() > 3 && out.substr(out.size() - 3) == ".ll") {
                    out = out.substr(0, out.size() - 3);
                }
                output_path = out;
            } else if (arg.substr(0, 1) != "-") {
                input_path = arg;
            } else if (arg == "-O1") {
                opt_level = 1;
            } else {
                // throw std::runtime_error("Unknown option: " + arg);
            }
        }
    }
};

#endif  // GEECEECEE_CONFIG_HPP
