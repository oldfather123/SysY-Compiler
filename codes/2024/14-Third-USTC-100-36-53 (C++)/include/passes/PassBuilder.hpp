#pragma once

#include "AnalysisPass.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "sysyc.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class PassBuilder {
  public:
    using PassCreator = std::function<std::unique_ptr<Pass<Module>>()>;

    void registerPass(const std::string &name, PassCreator creator);

    std::unique_ptr<Pass<Module>> createPass(const std::string &name);

    void registerAnalysisPassManager(std::shared_ptr<AnalysisPassManager> apm);

    void registerPassManager(std::shared_ptr<PassManager> pm);

    template <typename... PassNames> bool collectPass(PassNames &&...passNames);
    bool parsePipeline(std::string pipeline,
                       OptimizationLevel opt = OptimizationLevel::O0);

  private:
    std::unordered_map<std::string, PassCreator> passCreators_;
    std::weak_ptr<AnalysisPassManager> apm_;
    std::weak_ptr<PassManager> pm_;
    bool collectSinglePass(const std::string &passName,
                           std::shared_ptr<PassManager> pm);
};