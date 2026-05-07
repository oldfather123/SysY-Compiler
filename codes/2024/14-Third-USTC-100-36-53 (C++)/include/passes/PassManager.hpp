#pragma once

#include "Module.hpp"

#include <memory>
#include <vector>
#include "AnalysisPass.hpp"

template<typename IRUnit>
class Pass {
  public:
    Pass() = default;
    virtual ~Pass() = default;
    virtual void run(IRUnit *item, AnalysisPassManager& APM) = 0;
    virtual void print() {}
};

class PassManager: Pass<Module> {
  public:

    void addPass(std::unique_ptr<Pass<Module>> pass) {
        passes_.push_back(std::move(pass));
    }

    void run(Module *item, AnalysisPassManager& APM) override {
        for (auto &pass : passes_) {
            pass->run(item, APM);
        }
    }
    void print() override{

        for (auto &pass : passes_) {
            std::cerr << "Pass: ";
            auto &pass_ref = *pass;
            std::cerr << typeid(pass_ref).name() << std::endl;
            pass->print();
        }
    }

  private:
    std::vector<std::unique_ptr<Pass<Module>>> passes_;
};

template<typename FromIRUnit, typename ToIRUnit>
class PassAdapter : public Pass<ToIRUnit> {
public:
    PassAdapter(std::unique_ptr<Pass<FromIRUnit>> pass) 
        : pass_(std::move(pass)) {}

    virtual void run(ToIRUnit *item, AnalysisPassManager& APM) override {}

    void print() override {
        pass_->print();
    }

protected:
    std::unique_ptr<Pass<FromIRUnit>> pass_;
};

template<>
void PassAdapter<Function, Module>::run(Module *item, AnalysisPassManager& APM);
