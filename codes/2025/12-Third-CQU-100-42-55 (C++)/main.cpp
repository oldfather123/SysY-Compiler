#include <cassert>
#include <fstream>
#include <string>
#include <vector>

#include "./debug.h"
// #include "./include/backend/generator.h"
#include "./include/front/lexical.h"
#include "./include/front/semantic.h"
#include "./include/front/syntax.h"
#include "./include/ir/tca2llvm.h"
#include "./include/riscv/code_gen.h"
#include "./include/riscv/function_reg_alloc.h"
#include "./include/riscv/linear_scan_alloc.h"
#include "./include/riscv/bk_pass_filter.h"
#include "./include/tools/ir_executor.h"

using std::string;
using std::vector;

// 函数声明
void testLinearScanAllocation(llvm_ir::Module* module);

#define TODO assert(0 && "todo later");

#undef DEBUG_EXEC_BRIEF
#undef DEBUG_EXEC_DETAIL

bool endsWith(const std::string& str, const std::string& suffix) { return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0; }

#if (DEBUG_MAIN_MATCH_CASE)
// Helper function to remove all whitespace characters from a string
// (spaces, tabs, newlines, etc.)
// Optimized version that avoids unnecessary copying
std::string removeWhitespace(const std::string& str) {
    std::string result;
    result.reserve(str.size());  // Pre-allocate to avoid multiple reallocations

    for (char c : str) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += c;
        }
    }

    return result;
}

/**
 * @brief 判断src文件内容是否包含指定代码片段（可包含回车和缩进）
 *
 * 优化版本：
 * 1. 流式处理，避免读取整个文件到内存
 * 2. 针对目标字符串通常在文件前面的特点进行早期退出优化
 * 3. 避免创建大的中间字符串
 *
 * @param filename 要检查的文件路径。
 * @param codeSnippet 要查找的代码片段。
 * @return 如果文件包含该代码片段（忽略空白字符），则返回 true；否则返回 false。
 */
bool fileContainsCode(const std::string& filename, const std::string& codeSnippet) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    // 预处理目标字符串：去除空白字符
    std::string normalizedTarget = removeWhitespace(codeSnippet);
    if (normalizedTarget.empty()) {
        return true;  // 空目标总是匹配
    }

    // 流式处理文件内容，构建规范化字符串
    std::string normalizedContent;
    const size_t CHUNK_SIZE = 4096;                                // 4KB chunks for better I/O performance
    const size_t MAX_CONTENT_SIZE = normalizedTarget.size() * 10;  // 限制搜索范围，针对目标在前面的优化

    normalizedContent.reserve(std::min(MAX_CONTENT_SIZE, static_cast<size_t>(8192)));  // 预分配合理大小

    char buffer[CHUNK_SIZE];
    size_t totalProcessed = 0;

    while (file.read(buffer, CHUNK_SIZE) || file.gcount() > 0) {
        std::streamsize bytesRead = file.gcount();

        // 处理当前块，去除空白字符
        for (std::streamsize i = 0; i < bytesRead; ++i) {
            if (!std::isspace(static_cast<unsigned char>(buffer[i]))) {
                normalizedContent += buffer[i];

                // 早期匹配检查：一旦有足够的字符，就开始检查
                if (normalizedContent.size() >= normalizedTarget.size()) {
                    if (normalizedContent.find(normalizedTarget) != std::string::npos) {
                        return true;
                    }
                }
            }
        }

        totalProcessed += bytesRead;

        // 如果已经处理了足够多的内容而没有找到匹配，提前退出
        // 这是针对"目标通常在文件前面"的优化
        if (totalProcessed > MAX_CONTENT_SIZE) {
            break;
        }
    }

    // 最终检查（处理边界情况）
    return normalizedContent.find(normalizedTarget) != std::string::npos;
}

#endif

// 测试线性扫描寄存器分配算法的正确性
void testLinearScanAllocation(llvm_ir::Module* module) {
    std::cout << "=== Linear Scan Register Allocation Test ===" << std::endl;

    int total_functions = 0;
    int successful_allocations = 0;
    int total_variables = 0;
    int spilled_variables = 0;

    for (const auto& func : module->functions) {
        if (func->isDeclaration()) {
            continue;  // 跳过函数声明
        }

        total_functions++;
        std::cout << "\n--- Testing function: " << func->name << " ---" << std::endl;

        try {
            // 执行寄存器分配
            auto reg_alloc_result = llvm_ir::allocateFunctionRegisters(func.get());

            // 统计变量数量
            int func_variables = 0;
            int func_spilled = 0;

            for (const auto& [vreg, alloc] : reg_alloc_result.reg_alloc_map) {
                func_variables++;
                if (!alloc.is_reg) {
                    func_spilled++;
                }
            }

            total_variables += func_variables;
            spilled_variables += func_spilled;

            // 验证分配结果
            bool allocation_valid = true;

            // 检查栈偏移是否合理
            for (const auto& [vreg, alloc] : reg_alloc_result.reg_alloc_map) {
                if (!alloc.is_reg && alloc.stack_offset < 0) {
                    std::cout << "  ERROR: Invalid stack offset for " << vreg->name << ": " << alloc.stack_offset << std::endl;
                    allocation_valid = false;
                }
            }

            // 检查寄存器分配是否合理（不检查冲突，因为活跃区间不重叠的变量可以共享寄存器）
            std::map<riscv::reg, std::vector<std::string>> register_usage;
            for (const auto& [vreg, alloc] : reg_alloc_result.reg_alloc_map) {
                if (alloc.is_reg) {
                    register_usage[alloc.regid].push_back(vreg->name);
                }
            }

            // 输出寄存器使用情况（用于调试）
            for (const auto& [reg, vars] : register_usage) {
                if (vars.size() > 1) {
                    std::cout << "    Register " << riscv::to_string(reg) << " shared by: ";
                    for (size_t i = 0; i < vars.size(); ++i) {
                        if (i > 0)
                            std::cout << ", ";
                        std::cout << vars[i];
                    }
                    std::cout << std::endl;
                }
            }

            if (allocation_valid) {
                successful_allocations++;
                std::cout << "  ✓ Allocation successful" << std::endl;
                std::cout << "    Variables: " << func_variables << ", Spilled: " << func_spilled << ", Stack size: " << reg_alloc_result.total_stack_size << " bytes" << std::endl;

                // 输出详细的分配信息
                std::cout << "    Register allocations:" << std::endl;
                for (const auto& [vreg, alloc] : reg_alloc_result.reg_alloc_map) {
                    if (alloc.is_reg) {
                        std::cout << "      " << vreg->name << " -> " << riscv::to_string(alloc.regid) << std::endl;
                    } else {
                        std::cout << "      " << vreg->name << " -> SPILLED (offset: " << alloc.stack_offset << ")" << std::endl;
                    }
                }
            } else {
                std::cout << "  ✗ Allocation failed" << std::endl;
            }

        } catch (const std::exception& e) {
            std::cout << "  ✗ Exception during allocation: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "  ✗ Unknown exception during allocation" << std::endl;
        }
    }

    // 输出总体测试结果
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total functions tested: " << total_functions << std::endl;
    std::cout << "Successful allocations: " << successful_allocations << std::endl;
    std::cout << "Success rate: " << (total_functions > 0 ? (successful_allocations * 100.0 / total_functions) : 0) << "%" << std::endl;
    std::cout << "Total variables: " << total_variables << std::endl;
    std::cout << "Spilled variables: " << spilled_variables << std::endl;
    std::cout << "Spill rate: " << (total_variables > 0 ? (spilled_variables * 100.0 / total_variables) : 0) << "%" << std::endl;

    if (successful_allocations == total_functions && total_functions > 0) {
        std::cout << "✓ All tests passed! Linear scan allocation is working correctly." << std::endl;
    } else {
        std::cout << "✗ Some tests failed. Please check the allocation algorithm." << std::endl;
    }
    std::cout << "=== End Test ===\n" << std::endl;
}

int main(int argc, char** argv) {
    assert((argc >= 5 && argc <= 12) && "command line should be:  compiler -S -o <yi_ge_file.obj> <yi_ge_file.src> [-O0 | -O1] [--token] [--ast] [--ir] [--execute_ir] [--llvm-ir] [--riscv]");

    string src = argv[4];
    string real_name;
    if (endsWith(src, ".c") || endsWith(src, ".sy") || endsWith(src, ".cpp")) {
        if (endsWith(src, ".cpp")) {
            real_name = src.substr(0, src.size() - 4);
        } else if (endsWith(src, ".sy")) {
            real_name = src.substr(0, src.size() - 3);
        } else {
            real_name = src.substr(0, src.size() - 2);
        }
    } else {
        assert(0 && "source file should be a .c, .sy or .cpp file");
    }

#if (DEBUG_MAIN_MATCH_CASE)
    if (!MATCH_CASES(src)) {
        assert(0 && "404 not found");
    }
#endif

    string step = argv[1];
    string des = argv[3];

    // string opt = argc == 6 ? argv[5] : "-O0";  // if no optimization level is specified, use -O0
    std::vector<string> args(argv + 5, argv + argc);
    bool o1 = false, see_token = false, see_ast = false, see_ir = false, execute_ir = false, generate_llvm_ir = false, generate_riscv = false;
    for (const auto& arg : args) {
        if (arg == "-O0") {
            // nothing
        } else if (arg == "-O1") {
            o1 = true;
        } else if (arg == "--token") {
            see_token = true;
        } else if (arg == "--ast") {
            see_ast = true;
        } else if (arg == "--ir") {
            see_ir = true;
        } else if (arg == "--execute_ir") {
            execute_ir = true;
        } else if (arg == "--llvm-ir") {
            generate_llvm_ir = true;
        } else if (arg == "--riscv") {
            generate_riscv = true;
        }
    }

    // end of parameters analysis

    std::ofstream output_file = std::ofstream(des);

    assert(output_file.is_open() && "output file can not open");

    frontend::Scanner scanner(src);
    vector<frontend::Token> tk_stream = scanner.run();

    if (see_token) {  // print token stream
        std::cout << "@ Token Stream: " << std::endl;

        std::ofstream output_file_tk = std::ofstream(real_name + ".tk");  // output to a des.tk file
        for (const auto& tk : tk_stream) {
            output_file_tk << frontend::toString(tk.type) << "\t\t" << tk.value << std::endl;
        }

        std::cout << "@ End Token Stream" << std::endl;
    }

    frontend::Parser parser(tk_stream);
    frontend::CompUnit* node = parser.get_abstract_syntax_tree();

    if (see_ast) {
        std::cout << "@ Abstract Syntax Tree: " << std::endl;
        Json::Value json_output;
        Json::StyledWriter writer;
        node->get_json_output(json_output);
        std::ofstream output_file_json = std::ofstream(real_name + ".json");  // output to a des.json file
        output_file_json << writer.write(json_output);
        std::cout << "@ End AST Build" << std::endl;
    }

    frontend::Analyzer analyzer;
    auto program = analyzer.get_ir_program(node);

    if (see_ir) {
        auto o_res = real_name + ".ir_res";
        auto o_f = real_name + ".ir";
        auto i_f = real_name + ".in";
        ir::reopen_output_file = fopen(o_res.c_str(), "w");
        ir::reopen_input_file = fopen(i_f.c_str(), "r");

        auto executor = ir::Executor(&program);
        std::ofstream(o_f) << program.draw() << std::endl;

        if (execute_ir) {
            std::cout << std::endl;
            std::cout << "@ Executing IR: " << std::endl;
            fprintf(ir::reopen_output_file, "\n%d", (uint8_t)executor.run());
            std::cout << "@ End IR Execution" << std::endl;
        }
    }

    // compiler -S -o ${f_out} ${f_in}
    if (step == "-S") {
        std::cout << "@ Generating RISC-V assembly for compilation..." << std::endl;

        // 生成LLVM IR
        llvm_ir::LLVMIRGenerator llvmGenerator(real_name);
        auto llvmModule = llvmGenerator.generateModule(program);

        if (generate_llvm_ir) {
            // 输出LLVM IR到文件
            auto llvm_ir_file = real_name + ".ll";
            std::ofstream llvmOutput(llvm_ir_file);
            llvmOutput << llvmModule->toString();
            llvmOutput.close();

            std::cout << "@ LLVM IR generated to: " << llvm_ir_file << std::endl;
        }

        // 生成RISC-V汇编
        auto riscvModule = riscv::gen_module(llvmModule.get());

        // 应用冗余指令过滤优化
#if (BK_PASS_FILTER)
        riscv::BackendFilterPass::apply_filter_pass(riscvModule);
#endif
        // 输出RISC-V汇编到指定的输出文件
        output_file << riscvModule->toString();

        std::cout << "@ RISC-V assembly generated to: " << des << std::endl;
        delete riscvModule;
    }
}
