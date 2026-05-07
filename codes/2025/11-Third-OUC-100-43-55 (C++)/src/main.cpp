#include "CLI11.hpp"
#include "generate_ir.h"
#include "generate_mir.h"
#include "pass_manager.h"
#include "preprocessor.h"
#include "reg_allocate.h"
#include <unordered_map>

extern int yyparse();
extern FILE *yyin;
extern unique_ptr<CompUnit> root;

enum class OutputTypeEnum { LLVM, MIR, ASM };
enum class OptLevelEnum { O0, O1 };

void writeGlobals(std::ofstream &ofs, ir::Module *module) {
  std::vector<ir::GlobalVariable *> symbolsInData;
  std::vector<ir::GlobalVariable *> symbolsInBss;
  for (const auto &global : module->getGlobals()) {
    if (!global->isSingle() && global->isInBss()) {
      symbolsInBss.push_back(global);
    } else {
      symbolsInData.push_back(global);
    }
  }
  if (!symbolsInBss.empty()) {
    ofs << "\t.bss\n";
  }
  for (const auto &global : symbolsInBss) {
    const int size = static_cast<int>(global->getTypeSize() / 8);
    ofs << "\t.align 8\n";
    ofs << "\t.size " << global->getRawName() << ", " << std::to_string(size)
        << '\n';
    ofs << global->getRawName() << ":\n";
    ofs << "\t.space " << std::to_string(size) << '\n';
  }
  if (!symbolsInData.empty()) {
    ofs << "\t.data\n";
  }
  for (const auto &global : symbolsInData) {
    const int size = static_cast<int>(global->getTypeSize()) / 8;
    ofs << "\t.align 8\n";
    ofs << "\t.size " << global->getRawName() << ", " << std::to_string(size)
        << '\n';
    ofs << global->getRawName() << ":\n";
    const int num = size / 4;
    if (global->isSingle()) {
      ofs << "\t.word ";
      std::string value;
      const auto type = global->getType();
      if (type->isF32()) {
        float f = global->getFloat();
        value = std::to_string(*reinterpret_cast<int *>(&f));
      } else if (type->isI32()) {
        value = std::to_string(global->getInt());
      } else {
        throw std::runtime_error(
            "Unsupported type in CodeGenerator::buildGlobals");
      }
      ofs << value << '\n';
    } else {
      auto type = global->getType();
      while (auto arrayType = dynamic_cast<const ir::ArrayType *>(type)) {
        type = arrayType->getBaseType();
      }
      for (int i = 0; i < num; i++) {
        ofs << "\t.word ";
        std::string value;
        if (type->isF32()) {
          float f = global->getFloat(i);
          value = std::to_string(*reinterpret_cast<int *>(&f));
        } else if (type->isI32()) {
          value = std::to_string(global->getInt(i));
        } else {
          throw std::runtime_error(
              "Unsupported type in CodeGenerator::buildGlobals");
        }
        ofs << value << '\n';
      }
    }
  }
}

int main(int argc, const char *argv[]) {
  CLI::App app{"A compiler for SysY language", "compile2025-0"};

  std::string sourceFile;
  app.add_option("sources", sourceFile, "Source file to compile")
      ->required()
      ->check(CLI::ExistingFile);

  std::string outputFile;
  app.add_option("-o,--output", outputFile, "Output file")
      ->default_str("a.out");

  std::unordered_map<std::string, OptLevelEnum> opt_map{
      {"0", OptLevelEnum::O0}, {"1", OptLevelEnum::O1}};
  OptLevelEnum optLevel;
  app.add_option("-O,--optimize", optLevel, "Enable optimization level 1")
      ->default_val(OptLevelEnum::O0)
      ->transform(CLI::CheckedTransformer(opt_map, CLI::ignore_case));

  bool emitLLVM = false, emitMIR = false, emitASM = false;
  app.add_flag("--emit-llvm", emitLLVM, "Emit LLVM IR as output");
  app.add_flag("--emit-mir", emitMIR, "Emit MIR as output");
  app.add_flag("-S,--assembly", emitASM, "Emit assembly code as output");

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  std::ofstream ofs(outputFile, ios_base::out | ios_base::trunc);
  if (!ofs) {
    std::cerr << "[!] Error opening output file: " << outputFile << std::endl;
    exit(1);
  }

  Preprocessor preprocessor(sourceFile);
  std::string preprocessed = preprocessor.preprocess();
  yyin = fmemopen((void *)preprocessed.data(), preprocessed.size(), "r");
  if (!yyin) {
    std::cerr << "[!] Error opening source file: " << sourceFile << std::endl;
    exit(1);
  }

  OutputTypeEnum outputType;
  if (emitLLVM)
    outputType = OutputTypeEnum::LLVM;
  else if (emitMIR)
    outputType = OutputTypeEnum::MIR;
  else
    outputType = OutputTypeEnum::ASM;

  // std::cout << "compile2025-0 (C) OUCC. 2025" << std::endl;
  // std::cout << ">> Compiling " << sourceFile << " to " << outputFile
  //           << std::endl;

  yyparse();
  GenerateIR genIR;
  root->accept(genIR);
  genIR.checkTerminator();
  auto mod = genIR.getModule();
  pass::PassManager passManager(mod);
  // ATTENTION!
  if (optLevel == OptLevelEnum::O1) {
    // std::cout << ">> Running optimization passes..." << std::endl;
    passManager.run();
  }
  switch (outputType) {
  case OutputTypeEnum::LLVM: {
    // std::cout << ">> Generating LLVM IR..." << std::endl;

    for (const auto &glob : mod->getGlobals())
      ofs << glob->str() << "\n";
    if (mod->hasGlobal())
      ofs << "\n";

    auto funcs = mod->getFunctions();
    std::sort(funcs.begin(), funcs.end(), [](const auto &lhs, const auto &rhs) {
      if (lhs->empty() != rhs->empty())
        return lhs->empty() < rhs->empty();
      return lhs->getRawName() < rhs->getRawName();
    });

    for (const auto &func : funcs) {
      ofs << func->str() << "\n";
    }

    if (ofs.fail())
      std::cerr << "[!] Error writing to output file: " << outputFile
                << std::endl;
    else
      std::cout << ">> LLVM IR written to " << outputFile << std::endl;
  } break;
  case OutputTypeEnum::MIR: {
    std::cout << ">> Generating MIR..." << std::endl;
    riscv::GenerateMIR genMIR(mod);
    genMIR.generate();

    for (const auto &mFunc : mod->getMFuncs()) {
      ofs << mFunc->str();
      ofs << "\tret\n";
    }

    if (ofs.fail())
      std::cerr << "[!] Error writing to output file: " << outputFile
                << std::endl;
    else
      std::cout << ">> MIR written to " << outputFile << std::endl;
  } break;
  case OutputTypeEnum::ASM: {
    riscv::GenerateMIR genMIR(mod);
    genMIR.generate();
    // std::cout << "Generating ASM..." << std::endl;
    riscv::ModuleRegAlloc regAlloc(mod);
    regAlloc.allocate();
    // passManager.runLast();
    writeGlobals(ofs, mod);
    ofs << "\t.text\n";
    for (const auto &mFunc : mod->getMFuncs()) {
      ofs << mFunc->str();
      ofs << "\tret\n";
    }
  } break;
  }

  ofs.close();
  if (yyin)
    fclose(yyin);
  return 0;
}
