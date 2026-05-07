#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h> // for getopt
#include <stdexcept> // for std::stoi and exceptions

using namespace std;

#include "SysYLexer.h"
#include "SysYParser.h"
using namespace antlr4;

#include "SysYIRGenerator.h"
#include "SysYIRPrinter.h"
#include "SysYIRCFGOpt.h" // 包含 CFG 优化
#include "RISCv64Backend.h"
#include "Pass.h" // 包含新的 Pass 框架

using namespace sysy;

int DEBUG = 0;
int DEEPDEBUG = 0;
int DEEPERDEBUG = 0;
int DEBUGLENGTH = 50;

static string argStopAfter;
static string argInputFile;
static bool argFormat = false; // 目前未使用，但保留
static string argOutputFilename;
int optLevel = 0; // 优化级别，默认为0 (不加-O参数时)

void usage(int code) {
  const char *msg = "Usage: sysyc [options] inputfile\n\n"
                    "Supported options:\n"
                    "  -h \tprint help message and exit\n"
                    "  -f \tpretty-format the input file\n"
                    "  -s {ast,ir,asm,asmd,ird}\tstop after generating AST/IR/Assembly\n"
                    "  -S \tcompile to assembly (.s file)\n"
                    "  -o <file>\tplace the output into <file>\n"
                    "  -O<level>\tenable optimization at <level> (e.g., -O0, -O1)\n";
  cerr << msg;
  exit(code);
}

void parseArgs(int argc, char **argv) {
  const char *optstr = "hfs:So:O:"; 
  int opt = 0;

  while ((opt = getopt(argc, argv, optstr)) != -1) {
    switch (opt) {
    case 'h':
      usage(EXIT_SUCCESS);
      break;
    case 'f':
      argFormat = true;
      break;
    case 's':
      argStopAfter = optarg;
      break;
    case 'S':
      argStopAfter = "asm";
      break;
    case 'o':
      argOutputFilename = optarg;
      break;
    case 'O':
      try {
        optLevel = std::stoi(optarg);
        if (optLevel < 0) {
          cerr << "Error: Optimization level must be non-negative." << endl;
          usage(EXIT_FAILURE);
        }
      } catch (const std::invalid_argument& ia) {
        cerr << "Error: Invalid argument for -O: " << optarg << endl;
        usage(EXIT_FAILURE);
      } catch (const std::out_of_range& oor) {
        cerr << "Error: Optimization level out of range: " << optarg << endl;
        usage(EXIT_FAILURE);
      }
      break;
    default: /* '?' */
      usage(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    usage(EXIT_FAILURE);
  }
  argInputFile = argv[optind];
}

int main(int argc, char **argv) {
  parseArgs(argc, argv);
  // 1. 打开输入文件
  ifstream fin(argInputFile);
  if (not fin.is_open()) {
    cerr << "Failed to open input file: " << argInputFile << endl;
    return EXIT_FAILURE;
  }

  // 2. 解析 SysY 源代码到 AST (前端)
  ANTLRInputStream input(fin);
  SysYLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  SysYParser parser(&tokens);
  auto moduleAST = parser.compUnit();

  // 如果指定停止在 AST 阶段，则打印并退出
  if (argStopAfter == "ast") {
    cout << moduleAST->toStringTree(true) << '\n';
    sysy::cleanupIRPools(); // 清理内存池
    return EXIT_SUCCESS;
  }

  // 3. 遍历 AST 生成 IR (中端开始)
  SysYIRGenerator generator;
  generator.visitCompUnit(moduleAST);
  auto moduleIR = generator.get(); 
  auto builder = generator.getBuilder(); // 获取 IRBuilder 实例

  // 4. 执行 IR 优化 pass
  // 无论最终输出是 IR 还是 ASM，只要不是停止在 AST 阶段，都会进入此优化流程。
  // optLevel = 0 时，执行默认优化。
  // optLevel >= 1 时，执行默认优化 + 额外的 -O1 优化。
  if (DEBUG) cout << "Applying middle-end optimizations (level -O" << optLevel << ")...\n";

  // 设置 DEBUG 模式（如果指定了 'ird'）
  if (argStopAfter == "ird") {
    DEBUG = 1; // 这里可能需要更精细地控制 DEBUG 的开启时机和范围
  }
  
  if (DEBUG) {
    cout << "=== Init IR ===\n";
    moduleIR->print(cout); // 使用新实现的print方法直接打印IR
  }

  // 创建 Pass 管理器并运行优化管道
  PassManager passManager(moduleIR, builder); // 创建 Pass 管理器
  // 好像都不用传递module和builder了，因为 PassManager 初始化了
  passManager.runOptimizationPipeline(moduleIR, builder, optLevel);

  // 5. 根据 argStopAfter 决定后续操作
  // a) 如果指定停止在 IR 阶段，则打印最终 IR 并退出
  if (argStopAfter == "ir" || argStopAfter == "ird") {
    // 打印最终 IR
    if (DEBUG) cerr << "=== Final IR ===\n";
    if (!argOutputFilename.empty()) {
      // 输出到指定文件
      ofstream fout(argOutputFilename);
      if (not fout.is_open()) {
        cerr << "Failed to open output file: " << argOutputFilename << endl;
        moduleIR->cleanup(); // 清理模块
        sysy::cleanupIRPools(); // 清理内存池
        return EXIT_FAILURE;
      }
      moduleIR->print(fout);
      fout.close();
    } else {
      // 输出到标准输出
      moduleIR->print(cout);
    }
    moduleIR->cleanup(); // 清理模块
    sysy::cleanupIRPools(); // 清理内存池
    return EXIT_SUCCESS;

  }

  // b) 如果未停止在 IR 阶段，则继续生成汇编 (后端)
  // 设置 DEBUG 模式（如果指定了 'asmd'）
  if (argStopAfter == "asmd") {
    DEBUG = 1;
    DEEPDEBUG = 1;
  }
  
  sysy::RISCv64CodeGen codegen(moduleIR); // 传入优化后的 moduleIR
  string asmCode = codegen.code_gen();

  // 如果指定停止在 ASM 阶段，则打印/保存汇编并退出
  if (argStopAfter == "asm" || argStopAfter == "asmd") {
    if (!argOutputFilename.empty()) {
      ofstream fout(argOutputFilename);
      if (not fout.is_open()) {
        cerr << "Failed to open output file: " << argOutputFilename << endl;
        moduleIR->cleanup(); // 清理模块
        sysy::cleanupIRPools(); // 清理内存池
        return EXIT_FAILURE;
      }
      fout << asmCode << endl;
      fout.close();
    } else {
      cout << asmCode << endl;
    }
    moduleIR->cleanup(); // 清理模块
    sysy::cleanupIRPools(); // 清理内存池
    return EXIT_SUCCESS;
  }

  // 如果没有匹配到任何 -s 或 -S 选项，即意味着需要生成完整可执行文件（未来的功能）
  // 目前，可以简单退出，或者打印一条提示
  cout << "Compilation completed. No output specified (neither -s nor -S). Exiting.\n";
  // return EXIT_SUCCESS; // 或者这里调用一个链接器生成可执行文件

  moduleIR->cleanup(); // 清理模块
  sysy::cleanupIRPools(); // 清理内存池
  return EXIT_SUCCESS; 
}