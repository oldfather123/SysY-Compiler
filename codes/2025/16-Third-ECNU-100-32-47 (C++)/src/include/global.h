/**
 * @file global.h
 * 该头文件定义了编译器全局数据结构和变量的列表
 */
#ifndef AAAC_GLOBAL_H
#define AAAC_GLOBAL_H

#include <algorithm>
#include <memory>
#pragma once

#include "backend.h"
#include "common.h"
// #include "frontend.h"
#include "sym.h"
#include "utils/memory_manager.h"

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace aaac {
class CompilerGlobals {
private:
  // 单例模式
  static CompilerGlobals *instance;
  // unsigned int var_count;
  CompilerGlobals(){};

public:
  // 全局变量列表
  sym::VarList global_vars;
  inline static std::unordered_map<std::string,backend::GlobalVarValue> g_registry; //有初始值

  // 函数列表
  std::list<std::shared_ptr<backend::Function>> functions;

  // 符号表
  sym::SymbolList global_symbols;

  // 内存管理器
  // std::list<std::shared_ptr<sym::Block>> block_manager;
  std::list<std::shared_ptr<backend::BasicBlock>> bb_manager;

  // 获取单例实例
  static CompilerGlobals &getInstance() {
    if (!instance) {
      instance = new CompilerGlobals();
    }
    return *instance;
  }

  // 释放资源
  static void cleanup() {
    if (instance) {
      delete instance;
      instance = nullptr;
    }
  }

  // 添加全局变量
  std::shared_ptr<sym::Var> addGlobalVar(std::shared_ptr<frontend::Type> type,
                                         const std::string &var_name) {
    std::shared_ptr<sym::Var> var =
        std::make_shared<sym::Var>(type, var_name);
    var->prop<sym::BasicInfo>().is_global = true;
    global_vars.push_back(var);
    return var;
  }

  // 添加函数
  std::shared_ptr<backend::Function>
  addFunction(std::shared_ptr<sym::Function> sym_func) {
    //auto func = std::make_shared<backend::Function>();
    //func->func = sym_func;
    auto func = sym_func->getFunctionBody();
    functions.push_back(func);
    return func;
  }

  void addInitGlobalVar(const backend::GlobalVarValue& gv){
    g_registry.emplace(gv.get_name(),gv);
  }

  // 创建基本块
  std::shared_ptr<backend::BasicBlock>
  createBasicBlock() {
    auto bb = std::make_shared<backend::BasicBlock>();
    bb_manager.push_back(bb);
    return bb;
  }

  std::string genTempName(const std::string &prefix, unsigned int &count) {
    return prefix + std::to_string(count++);
  }

  /// @deprecated
  // std::shared_ptr<sym::Type> getType(const std::string &name) {
  //   auto iter = std::find(types.begin(), types.end(), name);
  //   return iter==types.end() ? nullptr : *iter;
  // }
};

// 一些初始化
inline CompilerGlobals *CompilerGlobals::instance = nullptr;
// 提供一个全局的访问点
inline CompilerGlobals &globals = CompilerGlobals::getInstance();

} // namespace aaac

#endif
