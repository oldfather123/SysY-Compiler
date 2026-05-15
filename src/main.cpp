#include "antlr4-runtime.h"
#include "frontend/ASTBuilder.h"
#include "frontend/ASTDumper.h"
#include "frontend/Interpreter.h"
#include "frontend/SemanticAnalysis.h"
#include "frontend/generated/SysYLexer.h"
#include "frontend/generated/SysYParser.h"
#include "asm/AsmEmitter.h"
#include "codegen/CodeGen.h"
#include "codegen/IRDumper.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

// 命令行参数解析结果。
struct Options {
    std::string inputPath;
    std::string outputPath;
    std::string astOutputPath;
    std::string irOutputPath;
    std::string asmOutputPath;
    bool emitAssembly = false;
    int optimizationLevel = 0;
};

void printUsage(const char *program) {
    std::cerr << "用法:\n"
              << "  " << program << " <input.sy> -S -o <output.s> [-O0|-O1]\n"
              << "  " << program
              << " <input.sy> [output.txt] [--dump-ast ast.txt] [--dump-ir ir.txt] [--dump-asm out.s]\n";
}

bool parseArgs(int argc, char *argv[], Options &options) {
    if (argc < 2) {
        return false;
    }

    std::string dashOPath;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-S") {
            options.emitAssembly = true;
            continue;
        }
        if (arg == "-o") {
            if (i + 1 >= argc) {
                return false;
            }
            dashOPath = argv[++i];
            if (dashOPath.empty() || dashOPath[0] == '-') {
                return false;
            }
            continue;
        }
        if (arg == "-O0") {
            options.optimizationLevel = 0;
            continue;
        }
        if (arg == "-O1") {
            options.optimizationLevel = 1;
            continue;
        }
        if (arg == "--dump-ast") {
            if (i + 1 >= argc) {
                return false;
            }
            options.astOutputPath = argv[++i];
            if (options.astOutputPath.empty() || options.astOutputPath[0] == '-') {
                return false;
            }
            continue;
        }
        if (arg == "--dump-ir") {
            if (i + 1 >= argc) {
                return false;
            }
            options.irOutputPath = argv[++i];
            if (options.irOutputPath.empty() || options.irOutputPath[0] == '-') {
                return false;
            }
            continue;
        }
        if (arg == "--dump-asm") {
            if (i + 1 >= argc) {
                return false;
            }
            options.asmOutputPath = argv[++i];
            if (options.asmOutputPath.empty() || options.asmOutputPath[0] == '-') {
                return false;
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            return false;
        }

        if (options.inputPath.empty()) {
            options.inputPath = arg;
            continue;
        }

        if (options.outputPath.empty()) {
            options.outputPath = arg;
            continue;
        }

        return false;
    }

    if (options.inputPath.empty()) {
        return false;
    }

    if (!dashOPath.empty()) {
        if (options.emitAssembly || !options.asmOutputPath.empty()) {
            if (!options.asmOutputPath.empty() && options.asmOutputPath != dashOPath) {
                return false;
            }
            options.asmOutputPath = dashOPath;
        } else {
            if (!options.outputPath.empty() && options.outputPath != dashOPath) {
                return false;
            }
            options.outputPath = dashOPath;
        }
    }

    if (options.emitAssembly && options.asmOutputPath.empty()) {
        return false;
    }

    return true;
}

bool shouldSkipInterpreter(const Options &options) {
    if (options.asmOutputPath.empty() || !options.astOutputPath.empty() ||
        !options.irOutputPath.empty()) {
        return false;
    }
    return options.outputPath.empty() || options.outputPath == "/dev/null";
}

// 读取整个文件内容到字符串。
std::string readFile(const std::string &path, const char *kind) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("无法打开" + std::string(kind) + ": " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 将字符串完整写入文件。
void writeFile(const std::string &path, const std::string &content,
               const char *kind) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("无法打开" + std::string(kind) + ": " + path);
    }
    file << content;
}

// 按评测要求拼接程序输出和返回值。
std::string formatResult(std::string output, int returnCode) {
    if (!output.empty() && output.back() != '\n') {
        output.push_back('\n');
    }
    output += std::to_string(((returnCode % 256) + 256) % 256);
    output.push_back('\n');
    return output;
}

} // namespace

int main(int argc, char *argv[]) {
    // 0. 解析参数
    Options options;
    if (!parseArgs(argc, argv, options)) {
        printUsage(argv[0]);
        return 1;
    }

    // 1. 读取源文件
    std::string sourceText;
    try {
        sourceText = readFile(options.inputPath, "输入文件");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    // 2. 词法分析
    antlr4::ANTLRInputStream input(sourceText);
    SysYLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    // 3. 语法分析
    SysYParser parser(&tokens);
    SysYParser::CompUnitContext *tree = parser.compUnit();
    const size_t errorCount =
        lexer.getNumberOfSyntaxErrors() + parser.getNumberOfSyntaxErrors();
    if (errorCount != 0) {
        std::cerr << "解析失败，共 " << errorCount << " 个语法错误\n";
        return 1;
    }

    // 4. 构建 AST
    ASTBuilder astBuilder;
    std::unique_ptr<ast::TranslationUnit> ast = astBuilder.build(tree);

    // 5. 语义检查
    SemanticAnalysis semanticAnalysis;
    if (!semanticAnalysis.analyze(*ast)) {
        std::cerr << "语义检查失败，共 " << semanticAnalysis.errors().size()
                  << " 个错误\n";
        for (const std::string &error : semanticAnalysis.errors()) {
            std::cerr << error << '\n';
        }
        return 1;
    }

    // 6. 按需输出 AST
    if (!options.astOutputPath.empty()) {
        std::ofstream astOutputFile(options.astOutputPath);
        if (!astOutputFile) {
            std::cerr << "无法打开 AST 输出文件: " << options.astOutputPath
                      << '\n';
            return 1;
        }
        ASTDumper dumper(astOutputFile);
        dumper.dump(*ast);
    }

    // 7. 按需输出 IR
    if (!options.irOutputPath.empty()) {
        std::ofstream irOutputFile(options.irOutputPath);
        if (!irOutputFile) {
            std::cerr << "无法打开 IR 输出文件: " << options.irOutputPath
                      << '\n';
            return 1;
        }
        codegen::CodeGen codegen(*ast);
        codegen::IRDumper dumper(irOutputFile);
        dumper.dump(*codegen.region());
    }

    // 7.5 按需输出 RISC-V 汇编
    if (!options.asmOutputPath.empty()) {
        std::ofstream asmOutputFile(options.asmOutputPath);
        if (!asmOutputFile) {
            std::cerr << "无法打开 ASM 输出文件: " << options.asmOutputPath
                      << '\n';
            return 1;
        }
        codegen::CodeGen codegen(*ast);
        codegen::AsmEmitter emitter(asmOutputFile);
        emitter.emit(*codegen.region());
    }

    if (shouldSkipInterpreter(options)) {
        return 0;
    }

    // 8. 解释执行并输出文件
    Interpreter interpreter(std::cin);
    try {
        const int returnCode = interpreter.run(*ast);
        const std::string result =
            formatResult(interpreter.takeOutput(), returnCode);
        if (options.outputPath.empty()) {
            std::cout << result;
            return 0;
        }
        writeFile(options.outputPath, result, "输出文件");
    } catch (const std::exception &error) {
        std::cerr << "解释执行失败: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
