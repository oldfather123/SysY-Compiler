#pragma once

#include "types.h"
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// 符号种类
enum class SymbolKind {
  VARIABLE, // 变量
  CONSTANT, // 常量
  FUNCTION, // 函数
  PARAMETER // 函数参数
};

// 符号信息类，存储符号的详细信息
class SymbolInfo {
public:
  SymbolKind kind;            // 符号种类
  std::shared_ptr<Type> type; // 类型信息
  std::string name;           // 符号名称
  bool is_initialized;        // 是否已初始化

  // 对于常量，存储其值（可选）
  std::optional<std::variant<int, float>> constant_value;

  // 对于数组，存储维度信息
  std::vector<int> array_dimensions;

  // 对于函数参数，标记是否为数组指针
  bool is_array_pointer = false;

  // 构造函数
  SymbolInfo(SymbolKind k, std::shared_ptr<Type> t, std::string n,
             bool init = false)
      : kind(k), type(std::move(t)), name(std::move(n)), is_initialized(init) {}

  // 设置常量值
  void setConstantValue(int value) { constant_value = value; }

  void setConstantValue(float value) { constant_value = value; }

  // 获取常量值
  template <typename T> std::optional<T> getConstantValue() const {
    if (!constant_value.has_value())
      return std::nullopt;

    if (auto *val = std::get_if<T>(&constant_value.value())) {
      return *val;
    }
    return std::nullopt;
  }

  // 设置数组维度
  void setArrayDimensions(std::vector<int> dims) {
    array_dimensions = std::move(dims);
  }

  // 判断是否为数组
  bool isArray() const { return !array_dimensions.empty() || is_array_pointer; }

  // 获取类型字符串表示
  std::string getTypeString() const { return type->toString(); }
};

using Scope = std::unordered_map<std::string, std::shared_ptr<SymbolInfo>>;

class SymbolTable {
public:
  // 构造函数，创建全局作用域
  SymbolTable() {
    enterScope();
    // 预定义一些内置函数
    initBuiltinFunctions();
  }

  // 查找符号，从内层作用域向外查找
  SymbolInfo *lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        return found->second.get();
      }
    }
    return nullptr;
  }

  // 在当前作用域中查找符号（不向外查找）
  SymbolInfo *lookupCurrent(const std::string &name) const {
    if (scopes.empty())
      return nullptr;

    auto &currentScope = scopes.back();
    auto found = currentScope.find(name);
    if (found != currentScope.end()) {
      return found->second.get();
    }
    return nullptr;
  }

  // 在当前作用域中插入符号
  bool insert(const std::string &name, std::shared_ptr<SymbolInfo> info) {
    // 检查当前作用域是否已存在该符号
    auto &currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
      return false; // 插入失败，符号已存在
    }
    currentScope[name] = info;
    return true;
  }

  // 强制插入符号（用于内置函数等）
  void forceInsert(const std::string &name, std::shared_ptr<SymbolInfo> info) {
    auto &currentScope = scopes.back();
    currentScope[name] = info;
  }

  // 进入新的作用域
  void enterScope() { scopes.emplace_back(); }

  // 退出当前作用域
  void exitScope() {
    if (!scopes.empty()) {
      scopes.pop_back();
    }
  }

  // 获取当前作用域深度
  size_t getScopeDepth() const { return scopes.size(); }

  // 判断是否在全局作用域
  bool isGlobalScope() const { return scopes.size() == 1; }

  // 打印符号表（调试用）
  void print() const {
    for (size_t i = 0; i < scopes.size(); ++i) {
      std::cout << "Scope " << i << ":\n";
      for (const auto &[name, info] : scopes[i]) {
        std::cout << "  " << name << ": " << info->getTypeString();
        switch (info->kind) {
        case SymbolKind::VARIABLE:
          std::cout << " (variable)";
          break;
        case SymbolKind::CONSTANT:
          std::cout << " (constant)";
          break;
        case SymbolKind::FUNCTION:
          std::cout << " (function)";
          break;
        case SymbolKind::PARAMETER:
          std::cout << " (parameter)";
          break;
        }
        std::cout << "\n";
      }
    }
  }

public:
  // 作用域栈，每个作用域是一个符号表
  std::vector<Scope> scopes;

  // 初始化内置函数
  void initBuiltinFunctions() {
    // 可以在这里添加内置函数，如printf等
    // 例如：void putint(int)
    auto putint_type = makeFunctionType(makeBasicType(BaseType::VOID),
                                        {makeBasicType(BaseType::INT)});
    auto putint_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putint_type, std::string("putint"), true);
    forceInsert("putint", putint_info);

    // void putfloat(float)
    auto putfloat_type = makeFunctionType(makeBasicType(BaseType::VOID),
                                          {makeBasicType(BaseType::FLOAT)});
    auto putfloat_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putfloat_type, std::string("putfloat"), true);
    forceInsert("putfloat", putfloat_info);

    // int getint()
    auto getint_type = makeFunctionType(makeBasicType(BaseType::INT), {});
    auto getint_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, getint_type, std::string("getint"), true);
    forceInsert("getint", getint_info);

    // float getfloat()
    auto getfloat_type = makeFunctionType(makeBasicType(BaseType::FLOAT), {});
    auto getfloat_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, getfloat_type, std::string("getfloat"), true);
    forceInsert("getfloat", getfloat_info);

    // int getch() - 输入字符，返回ASCII码
    auto getch_type = makeFunctionType(makeBasicType(BaseType::INT), {});
    auto getch_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, getch_type, std::string("getch"), true);
    forceInsert("getch", getch_info);

    // void putch(int) - 输出字符
    auto putch_type = makeFunctionType(makeBasicType(BaseType::VOID),
                                       {makeBasicType(BaseType::INT)});
    auto putch_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putch_type, std::string("putch"), true);
    forceInsert("putch", putch_info);

    // int getarray(int[]) - 输入整数数组
    auto getarray_type =
        makeFunctionType(makeBasicType(BaseType::INT),
                         {makeArrayType(makeBasicType(BaseType::INT), {-1})});
    auto getarray_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, getarray_type, std::string("getarray"), true);
    forceInsert("getarray", getarray_info);

    // int getfarray(float[]) - 输入浮点数组
    auto getfarray_type =
        makeFunctionType(makeBasicType(BaseType::INT),
                         {makeArrayType(makeBasicType(BaseType::FLOAT), {-1})});
    auto getfarray_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, getfarray_type, std::string("getfarray"), true);
    forceInsert("getfarray", getfarray_info);

    // void putarray(int, int[]) - 输出整数数组
    auto putarray_type =
        makeFunctionType(makeBasicType(BaseType::VOID),
                         {makeBasicType(BaseType::INT),
                          makeArrayType(makeBasicType(BaseType::INT), {-1})});
    auto putarray_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putarray_type, std::string("putarray"), true);
    forceInsert("putarray", putarray_info);

    // void putfarray(int, float[]) - 输出浮点数组
    auto putfarray_type =
        makeFunctionType(makeBasicType(BaseType::VOID),
                         {makeBasicType(BaseType::INT),
                          makeArrayType(makeBasicType(BaseType::FLOAT), {-1})});
    auto putfarray_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putfarray_type, std::string("putfarray"), true);
    forceInsert("putfarray", putfarray_info);

    // void starttime() - 开始计时
    auto starttime_type = makeFunctionType(makeBasicType(BaseType::VOID), {});
    auto starttime_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, starttime_type, std::string("starttime"), true);
    forceInsert("starttime", starttime_info);

    // void stoptime() - 停止计时
    auto stoptime_type = makeFunctionType(makeBasicType(BaseType::VOID), {});
    auto stoptime_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, stoptime_type, std::string("stoptime"), true);
    forceInsert("stoptime", stoptime_info);

    // void putf(string, ...) - 格式化输出
    auto putf_type = makeFunctionType(makeBasicType(BaseType::VOID),
                                      {makeBasicType(BaseType::STRING)},
                                      true // 可变参数
    );
    auto putf_info = std::make_shared<SymbolInfo>(
        SymbolKind::FUNCTION, putf_type, std::string("putf"), true);
    forceInsert("putf", putf_info);
  }
};
