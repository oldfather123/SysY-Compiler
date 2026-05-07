#pragma once

#include <functional> // For std::function
#include <map>
#include <memory>
#include <set>
#include <string>
#include <typeindex> // For std::type_index (although void* ID is more common in LLVM)
#include <vector>
#include <type_traits>
#include "IR.h"
#include "IRBuilder.h"

extern int DEBUG; // 全局调试标志

namespace sysy {

//前向声明
class PassManager;
class AnalysisManager;

// 抽象基类：分析结果
class AnalysisResultBase {
public:
  virtual ~AnalysisResultBase() = default;
};

// 抽象基类：Pass
class Pass {
public:
  enum class Granularity { Module, Function, BasicBlock };

  enum class PassKind { Analysis, Optimization };

  Pass(const std::string &name, Granularity g, PassKind k) : Name(name), G(g), K(k) {}
  virtual ~Pass() = default;

  const std::string &getName() const { return Name; }
  Granularity getGranularity() const { return G; }
  PassKind getPassKind() const { return K; }

  virtual bool runOnModule(Module *M, AnalysisManager& AM) { return false; }
  virtual bool runOnFunction(Function *F, AnalysisManager& AM) { return false; }
  virtual bool runOnBasicBlock(BasicBlock *BB, AnalysisManager& AM) { return false; }

  // 所有 Pass 都必须提供一个唯一的 ID
  // 这通常是一个静态成员，并在 Pass 类外部定义
  virtual void *getPassID() const = 0;

protected:
  std::string Name;
  Granularity G;
  PassKind K;
};

// 抽象基类：分析遍
class AnalysisPass : public Pass {
public:
  AnalysisPass(const std::string &name, Granularity g) : Pass(name, g, PassKind::Analysis) {}

  virtual std::unique_ptr<AnalysisResultBase> getResult() = 0;
};

// 抽象基类：优化遍
class OptimizationPass : public Pass {
public:
  OptimizationPass(const std::string &name, Granularity g) : Pass(name, g, PassKind::Optimization) {}

  virtual void getAnalysisUsage(std::set<void *> &analysisDependencies, std::set<void *> &analysisInvalidations) const {
    // 默认不依赖也不修改任何分析
  }
};

// ======================================================================
// PassRegistry: 全局 Pass 注册表 (单例)
// ======================================================================
class PassRegistry {
public:
  // Pass 工厂函数类型：返回 Pass 的唯一指针
  using PassFactory = std::function<std::unique_ptr<Pass>()>;

  // 获取 PassRegistry 实例 (单例模式)
  static PassRegistry &getPassRegistry() {
    static PassRegistry instance;
    return instance;
  }

  // 注册一个 Pass
  // passID 是 Pass 类的唯一静态 ID (例如 MyPass::ID 的地址)
  // factory 是一个 lambda 或函数指针，用于创建该 Pass 的实例
  void registerPass(void *passID, PassFactory factory) {
    if (factories.count(passID)) {
      // Error: Pass with this ID already registered
      // You might want to throw an exception or log an error
      return;
    }
    factories[passID] = std::move(factory);
  }

  // 通过 Pass ID 创建一个 Pass 实例
  std::unique_ptr<Pass> createPass(void *passID) {
    auto it = factories.find(passID);
    if (it == factories.end()) {
      // Error: Pass with this ID not registered
      return nullptr;
    }
    return it->second(); // 调用工厂函数创建实例
  }

private:
  PassRegistry() = default; // 私有构造函数，实现单例
  ~PassRegistry() = default;
  PassRegistry(const PassRegistry &) = delete;            // 禁用拷贝构造
  PassRegistry &operator=(const PassRegistry &) = delete; // 禁用赋值操作

  std::map<void *, PassFactory> factories;
};

// ======================================================================
// AnalysisManager: 负责管理和提供分析结果
// ======================================================================
class AnalysisManager {
private:
  Module *pModuleRef; // 指向被分析的Module

  // 缓存不同粒度的分析结果
  std::map<void *, std::unique_ptr<AnalysisResultBase>> moduleCachedResults;
  std::map<std::pair<Function *, void *>, std::unique_ptr<AnalysisResultBase>> functionCachedResults;
  std::map<std::pair<BasicBlock *, void *>, std::unique_ptr<AnalysisResultBase>> basicBlockCachedResults;


public:
  // 构造函数接收 Module 指针
  AnalysisManager(Module *M) : pModuleRef(M) {}
  AnalysisManager() = delete; // 禁止无参构造

  ~AnalysisManager() = default;

  // 获取分析结果的通用模板函数
  // T 是 AnalysisResult 的具体类型，E 是 AnalysisPass 的具体类型
  // F 和 BB 参数用于提供上下文，根据分析遍的粒度来使用
  template <typename T, typename E> T *getAnalysisResult(Function *F = nullptr, BasicBlock *BB = nullptr) {
    void *analysisID = E::ID; // 获取分析遍的唯一 ID

    // 尝试从注册表创建分析遍实例
    std::unique_ptr<Pass> basePass = PassRegistry::getPassRegistry().createPass(analysisID);
    if (!basePass) {
      // Error: Analysis pass not registered
      std::cerr << "Error: Analysis pass with ID " << analysisID << " not registered.\n";
      return nullptr;
    }
    AnalysisPass *analysisPass = static_cast<AnalysisPass *>(basePass.get());

    // 根据分析遍的粒度处理
    switch (analysisPass->getGranularity()) {
      case Pass::Granularity::Module: {
        // 检查是否已存在有效结果
        auto it = moduleCachedResults.find(analysisID);
        if (it != moduleCachedResults.end()) {
          if(DEBUG) {
            std::cout << "Using cached result for Analysis Pass: " << analysisPass->getName() << "\n";
          }
          return static_cast<T *>(it->second.get()); // 返回缓存结果
        }
        // 只有在实际运行时才打印调试信息
        if(DEBUG){
          std::cout << "Running Analysis Pass: " << analysisPass->getName() << "\n";
        }
        // 运行模块级分析遍
        if (!pModuleRef) {
          std::cerr << "Error: Module reference not set for AnalysisManager to run Module Pass.\n";
          return nullptr;
        }
        analysisPass->runOnModule(pModuleRef, *this);
        // 获取结果并缓存
        std::unique_ptr<AnalysisResultBase> result = analysisPass->getResult();
        T *specificResult = static_cast<T *>(result.get());
        moduleCachedResults[analysisID] = std::move(result); // 缓存结果
        return specificResult;
      }
      case Pass::Granularity::Function: {
        // 检查请求的上下文是否正确
        if (!F) {
          std::cerr << "Error: Function context required for Function-level Analysis Pass.\n";
          return nullptr;
        }
        // 检查是否已存在有效结果
        auto it = functionCachedResults.find({F, analysisID});
        if (it != functionCachedResults.end()) {
          if(DEBUG) {
            std::cout << "Using cached result for Analysis Pass: " << analysisPass->getName() << " (Function: " << F->getName() << ")\n";
          }
          return static_cast<T *>(it->second.get()); // 返回缓存结果
        }
        // 只有在实际运行时才打印调试信息
        if(DEBUG){
          std::cout << "Running Analysis Pass: " << analysisPass->getName() << "\n";
          std::cout << "Function: " << F->getName() << "\n";
        }
        // 运行函数级分析遍
        analysisPass->runOnFunction(F, *this);
        // 获取结果并缓存
        std::unique_ptr<AnalysisResultBase> result = analysisPass->getResult();
        T *specificResult = static_cast<T *>(result.get());
        functionCachedResults[{F, analysisID}] = std::move(result); // 缓存结果
        return specificResult;
      }
      case Pass::Granularity::BasicBlock: {
        // 检查请求的上下文是否正确
        if (!BB) {
          std::cerr << "Error: BasicBlock context required for BasicBlock-level Analysis Pass.\n";
          return nullptr;
        }
        // 检查是否已存在有效结果
        auto it = basicBlockCachedResults.find({BB, analysisID});
        if (it != basicBlockCachedResults.end()) {
          if(DEBUG) {
            std::cout << "Using cached result for Analysis Pass: " << analysisPass->getName() << " (BasicBlock: " << BB->getName() << ")\n";
          }
          return static_cast<T *>(it->second.get()); // 返回缓存结果
        }
        // 只有在实际运行时才打印调试信息
        if(DEBUG){
          std::cout << "Running Analysis Pass: " << analysisPass->getName() << "\n";
          std::cout << "BasicBlock: " << BB->getName() << "\n";
        }
        // 运行基本块级分析遍
        analysisPass->runOnBasicBlock(BB, *this);
        // 获取结果并缓存
        std::unique_ptr<AnalysisResultBase> result = analysisPass->getResult();
        T *specificResult = static_cast<T *>(result.get());
        basicBlockCachedResults[{BB, analysisID}] = std::move(result); // 缓存结果
        return specificResult;
      }
    }
    return nullptr; // 不会到达这里
  }

  // 使所有分析结果失效 (当 IR 被修改时调用)
  void invalidateAllAnalyses() {
    moduleCachedResults.clear();
    functionCachedResults.clear();
    basicBlockCachedResults.clear();
  }

  // 使特定分析结果失效
  // void *analysisID: 要失效的分析的ID
  // Function *F: 如果是函数级分析，指定函数；如果是模块级或基本块级，则为nullptr (取决于调用方式)
  // BasicBlock *BB: 如果是基本块级分析，指定基本块；否则为nullptr
  void invalidateAnalysis(void *analysisID, Function *F = nullptr, BasicBlock *BB = nullptr) {
    if (BB) {
      // 使特定基本块的特定分析结果失效
      basicBlockCachedResults.erase({BB, analysisID});
    } else if (F) {
      // 使特定函数的特定分析结果失效 (也可能包含聚合的BasicBlock结果)
      functionCachedResults.erase({F, analysisID});
      // 遍历所有属于F的基本块，使其BasicBlockCache失效 (如果该分析是BasicBlock粒度的)
      // 这需要遍历F的所有基本块，效率较低，更推荐在BasicBlockPass的invalidateAnalysisUsage中精确指定
      // 或者在Function级别的invalidate时，清空该Function的所有BasicBlock分析
      // 这里的实现简单地清空该Function下所有该ID的BasicBlock缓存
      for (auto it = basicBlockCachedResults.begin(); it != basicBlockCachedResults.end(); ) {
          // 假设BasicBlock::getParent()方法存在，可以获取所属Function
          if (it->first.second == analysisID /* && it->first.first->getParent() == F */) { // 需要BasicBlock能获取其父函数
              it = basicBlockCachedResults.erase(it);
          } else {
              ++it;
          }
      }

    } else {
      // 使所有函数的特定分析结果失效 (Module级和所有Function/BasicBlock级)
      moduleCachedResults.erase(analysisID);
      for (auto it = functionCachedResults.begin(); it != functionCachedResults.end(); ) {
        if (it->first.second == analysisID) {
          it = functionCachedResults.erase(it);
        } else {
          ++it;
        }
      }
      for (auto it = basicBlockCachedResults.begin(); it != basicBlockCachedResults.end(); ) {
        if (it->first.second == analysisID) {
          it = basicBlockCachedResults.erase(it);
        } else {
          ++it;
        }
      }
    }
  }
};

// ======================================================================
// PassManager：遍管理器
// ======================================================================
class PassManager {
private:
  std::vector<std::unique_ptr<Pass>> passes;
  AnalysisManager analysisManager;
  Module *pmodule;
  IRBuilder *pBuilder;

public:
  PassManager() = delete;
  ~PassManager() = default;

  PassManager(Module *module, IRBuilder *builder) : pmodule(module) ,pBuilder(builder), analysisManager(module) {}
  
  // 运行所有注册的遍
  bool run();
  
  // 运行优化管道主要负责注册和运行优化遍
  // 这里可以根据 optLevel 和 DEBUG 控制不同的优化遍
  void runOptimizationPipeline(Module* moduleIR, IRBuilder* builder, int optLevel);

  // 添加遍：现在接受 Pass 的 ID，而不是直接的 unique_ptr
  void addPass(void *passID);

  AnalysisManager &getAnalysisManager() { return analysisManager; }

  void clearPasses();
  
  // 输出pass列表并打印IR信息供观察优化遍效果
  void printPasses() const;
};

// ======================================================================
// 辅助宏或函数，用于简化 Pass 的注册
// ======================================================================

// 用于分析遍的注册
template <typename AnalysisPassType> void registerAnalysisPass();

// (1) 针对需要 IRBuilder 参数的优化遍的重载
// 这个模板只在 OptimizationPassType 可以通过 IRBuilder* 构造时才有效
template <typename OptimizationPassType, typename std::enable_if<
    std::is_constructible<OptimizationPassType, IRBuilder*>::value, int>::type = 0>
void registerOptimizationPass(IRBuilder* builder);

// (2) 针对不需要 IRBuilder 参数的所有其他优化遍的重载
// 这个模板只在 OptimizationPassType 不能通过 IRBuilder* 构造时才有效
template <typename OptimizationPassType, typename std::enable_if<
    !std::is_constructible<OptimizationPassType, IRBuilder*>::value, int>::type = 0>
void registerOptimizationPass();

} // namespace sysy