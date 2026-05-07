#pragma once

#include "MIBuilder.hpp"
#include <memory>
#include <vector>

class MachineModule;
class MachinePass;
class MIBuilder;

class MachinePass {
  public:
    MachinePass(std::shared_ptr<MachineModule> module) : module(module) {
        builder = module->get_builder();  
    }
    virtual ~MachinePass() = default;
    virtual void run() = 0;
  protected:
    std::shared_ptr<MIBuilder> builder;
    std::shared_ptr<MachineModule> module;
};

class MachinePassManager {
  public:
    MachinePassManager(std::shared_ptr<MachineModule> module) : machine_module(module) {}
    template <typename PassType, typename... Args>
    void add_pass(Args &&...args) {
        machine_passes.emplace_back(new PassType(machine_module, std::forward<Args>(args)...));
    }

    void run() {
        for (auto &pass : machine_passes) {
            pass->run();
        }
    }
  private:
    std::shared_ptr<MachineModule> machine_module;
    std::vector<std::unique_ptr<MachinePass>> machine_passes;
};