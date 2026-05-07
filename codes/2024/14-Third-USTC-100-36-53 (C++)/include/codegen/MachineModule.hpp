#pragma once

#include "Constant.hpp"
#include "MIBuilder.hpp"
#include <memory>
#include <vector>


class MachineFunction;
class Module;
class Constant;
class MIBuilder;

class MachineModule {
  public:
    MachineModule(Module *IR_module) : IR_module(IR_module) {
        builder = std::make_shared<MIBuilder>();
    }
    ~MachineModule() = default;
    void add_function(std::shared_ptr<MachineFunction> MF);

    void create_init_val(Constant *init, std::string &s, bool &is_first) const;
    std::string global_var_print() const;
    std::string print() const;
    std::vector<std::shared_ptr<MachineFunction>> get_functions() const;
    Module *get_IR_module() const { return IR_module; }
    std::shared_ptr<MIBuilder> get_builder() const { return builder; }

  private:
    Module *IR_module;
    std::shared_ptr<MIBuilder> builder;
    std::vector<std::shared_ptr<MachineFunction>> functions;
};