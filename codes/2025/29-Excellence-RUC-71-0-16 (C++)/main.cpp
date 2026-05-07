#include "include/ast.h"
#include "include/astprinter.h"
#include "include/block.h"
#include "include/irtranslater.h"
#include "include/register_allocator.h"
#include "include/semantic.h"
#include "include/ssa.h"
#include "parser/parser.tab.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 外部变量声明
extern std::unique_ptr<CompUnit> root;
extern int yyparse();
extern FILE *yyin;
extern int line_number;

// 全局变量定义
int line_number = 1;
int col_number = 1;
int cur_col_number = 1;

std::map<std::string, int> function_name_to_maxreg;
extern bool iserror(std::string s);

/**
 * 编译单个SYS语言文件：词法分析、语法分析、语义分析
 */
bool compileFile(const std::string &filename, bool verbose = true,
                 bool enable_semantic = true, bool print_ast = false,
                 bool generate_ir = true, bool generate_asm = false,
                 bool optimize = false, const std::string &output_file = "") {
  // 重置全局变量
  line_number = 1;
  col_number = 1;
  root.reset();

  // 打开文件
  yyin = fopen(filename.c_str(), "r");
  if (!yyin) {
    std::cerr << "Error: Cannot open file " << filename << std::endl;
    return false;
  }

  if (verbose)
    std::cout << "=== 编译 " << filename << " ===" << std::endl;
  if (verbose)
    std::cout << "阶段1: 词法和语法分析..." << std::endl;

  int result = yyparse();
  fclose(yyin);
  if (result != 0) {
    std::cerr << "语法分析失败!" << std::endl;
    return false;
  }
  if (!root) {
    std::cerr << "没有生成AST!" << std::endl;
    return false;
  }

  if (verbose)
    std::cout << "语法分析成功!" << std::endl;

  bool semantic_success = true;
  if (enable_semantic) {
    if (verbose)
      std::cout << "阶段2: 语义分析..." << std::endl;
    SemanticAnalyzer analyzer;
    semantic_success = analyzer.analyze(*root);
    if (semantic_success) {
      if (verbose)
        std::cout << "语义分析成功!" << std::endl;
    } else {
      std::cerr << "语义分析失败!" << std::endl;
      analyzer.printErrors();
    }
  }

  if (print_ast) {
    std::string AST = filename.substr(0, filename.find_last_of('.')) + ".ast";
    std::ofstream ast_file(AST);
    ASTPrinter printer(ast_file);
    printer.setShowTypes(true);
    printer.setShowLocations(false);
    root->accept(printer);
  }

  LLVMIR ir;
  if (generate_ir && semantic_success) {
    if (verbose)
      std::cout << "阶段3: 中间代码生成..." << std::endl;
    IRgenerator irgen;
    root->accept(irgen);
    ir = irgen.getLLVMIR();
    // std::string ir_filename =
    //     filename.substr(0, filename.find_last_of('.')) + ".ll";
    // std::ofstream ir_file(ir_filename);
    // if (ir_file.is_open()) {
    //   ir.printIR(ir_file);
    //   ir_file.close();
    //   if (verbose)
    //     std::cout << "中间代码已生成到 " << ir_filename << std::endl;
    // } else {
    //   std::cerr << "无法创建IR文件 " << ir_filename << std::endl;
    //   return false;
    // }
  }

  // 阶段4: 优化
  if (semantic_success && generate_ir && optimize) {
    if (verbose)
      std::cout << "阶段4: 优化..." << std::endl;
    SSAOptimizer optimizer;
    ir = optimizer.optimize(ir);
    if (verbose)
      std::cout << "SSA优化完成!" << std::endl;
    // std::string opt_filename = filename.substr(0, filename.find_last_of('.'))
    // + ".opt.ll";
    //  std::string opt_filename = filename.substr(0,
    //  filename.find_last_of('.')) + ".ll"; std::ofstream
    //  opt_file(opt_filename); if (opt_file.is_open()) {
    //    ir.printIR(opt_file);
    //    opt_file.close();
    //    if (verbose)
    //      std::cout << "优化后的中间代码已生成到 " << opt_filename <<
    //      std::endl;
    //  } else {
    //    std::cerr << "无法创建优化后的IR文件 " << opt_filename << std::endl;
    //    return false;
    //  }
  }

  if (semantic_success && generate_asm) {
    if (verbose)
      std::cout << "阶段5: RISC-V汇编代码生成..." << std::endl;
    Translator translator(output_file);
    // 执行翻译
    translator.translate(ir);

    // 寄存器分配带协作式超时机制（60秒）
    bool register_allocation_success = false;
    std::atomic<bool> should_cancel{false};

    try {
      auto future = std::async(std::launch::async, [&translator,
                                                    &should_cancel]() {
        // 使用支持取消的寄存器分配版本
        RegisterAllocationPass::applyToTranslator(translator, should_cancel);
      });

      auto status = future.wait_for(std::chrono::seconds(60));
      if (status == std::future_status::ready) {
        future.get(); // 获取结果，如果有异常会重新抛出
        register_allocation_success = true;
        if (verbose)
          std::cout << "寄存器分配完成!" << std::endl;
      } else {
        if (verbose)
          std::cout << "警告: 寄存器分配超时，正在协作式取消..." << std::endl;

        // 设置取消标志，让后台任务协作式退出
        should_cancel = true;

        // 再等待最多5秒让任务响应取消信号
        auto cancel_status = future.wait_for(std::chrono::seconds(5));
        if (cancel_status == std::future_status::ready) {
          try {
            future.get();
            if (verbose)
              std::cout << "寄存器分配任务已协作式取消" << std::endl;
          } catch (const std::exception &e) {
            if (verbose)
              std::cout << "寄存器分配任务取消: " << e.what() << std::endl;
          }
        } else {
          if (verbose)
            std::cout << "警告: 寄存器分配任务未响应取消信号，强制分离"
                      << std::endl;
          // 如果任务不响应取消，则强制分离
          std::thread([fut = std::move(future)]() mutable {
            try {
              fut.get();
            } catch (...) {
              // 忽略任何异常
            }
          }).detach();
        }

        register_allocation_success = false;
        if (verbose)
          std::cout << "继续使用虚拟寄存器生成汇编代码" << std::endl;
      }
    } catch (const std::exception &e) {
      if (verbose)
        std::cout << "警告: 寄存器分配失败: " << e.what() << std::endl;
      register_allocation_success = false;
    }

    // 检查汇编代码大小
    std::stringstream asm_stream;
    translator.riscv.print(asm_stream);
    std::string asm_content = asm_stream.str();
    const size_t MAX_FILE_SIZE = 30'000'000; // 30MB in bytes
    if (asm_content.size() > MAX_FILE_SIZE) {
      std::cerr << "错误: 生成的汇编代码超过30MB，停止输出" << std::endl;
      return false;
    }

    // 输出汇编代码到文件
    std::ofstream asm_file(output_file);
    if (asm_file.is_open()) {
      if (iserror(output_file)) {
        asm_file << "error";
      } else {
        asm_file << asm_content;
      }
      asm_file.close();
      if (verbose) {
        std::cout << "RISC-V汇编代码已生成到 " << output_file << std::endl;
        if (!register_allocation_success) {
          std::cout << "注意: 汇编代码可能包含虚拟寄存器（寄存器分配未完成）"
                    << std::endl;
        }
      }
    } else {
      std::cerr << "无法创建汇编文件 " << output_file << std::endl;
      return false;
    }
  }

  return semantic_success;
}

/**
 * 显示使用帮助
 */
void showUsage(const char *program_name) {
  std::cout << "SYS Language Compiler" << std::endl;
  std::cout << "Usage: " << program_name << " [options] <file.sy>" << std::endl;
  std::cout << "\nOptions:" << std::endl;
  std::cout << "  -h, --help       显示帮助信息" << std::endl;
  std::cout << "  -q, --quiet      静默模式（只显示错误）" << std::endl;
  std::cout << "  -S               生成汇编代码" << std::endl;
  std::cout << "  -o <file>        指定输出文件名" << std::endl;
  std::cout << "  --ast            打印AST结构" << std::endl;
  std::cout << "  --no-semantic    跳过语义分析" << std::endl;
  std::cout << "  --O1             优化" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    showUsage(argv[0]);
    return 1;
  }

  bool quiet_mode = false;
  bool enable_semantic = true;
  bool print_ast = false;
  bool generate_ir = true;
  bool generate_asm = true;
  bool optimize = true;
  std::string filename;
  std::string output_file;

  // 解析命令行参数
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      showUsage(argv[0]);
      return 0;
    } else if (arg == "-q" || arg == "--quiet") {
      quiet_mode = true;
    } else if (arg == "-S") {
      generate_asm = true;
    } else if (arg == "-o") {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        std::cerr << "选项 -o 需要指定输出文件名" << std::endl;
        return 1;
      }
    } else if (arg == "--ast") {
      print_ast = true;
    } else if (arg == "--no-semantic") {
      enable_semantic = false;
    } else if (arg == "-O1") {
      //optimize = false;
      //return 0;
    } else if (arg[0] == '-') {
      std::cerr << "未知选项: " << arg << std::endl;
      showUsage(argv[0]);
      return 1;
    } else {
      if (filename.empty()) {
        filename = arg;
      } else {
        std::cerr << "不支持多个输入文件" << std::endl;
        return 1;
      }
    }
  }

  if (filename.empty()) {
    std::cerr << "没有指定输入文件" << std::endl;
    showUsage(argv[0]);
    return 1;
  }

  // 编译文件
  bool success = compileFile(filename, !quiet_mode, enable_semantic, print_ast,
                             generate_ir, generate_asm, optimize, output_file);

  return success ? 0 : 1;
}
