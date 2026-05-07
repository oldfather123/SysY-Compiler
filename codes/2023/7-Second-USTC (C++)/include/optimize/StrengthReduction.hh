#pragma once

#include "Module.hh"
#include "Pass.hh"
#include "Constant.hh"
#include "ConstProp.hh"
#include "IRBuilder.hh"
#include "logging.hh"
#include "utils.hh"
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>

class StrengthReduction : public Pass
{
private:
    Module *module_;
    std::string name = "StrengthReduction";

public:
    explicit StrengthReduction(Module* module, bool print_ir=false): Pass(module, print_ir){module_ = module;}
    ~StrengthReduction(){};
    void execute() final;
    const std::string get_name() const override {return name;}

    bool isNthPower(int x);
    void specInstReduct();
    void specInstReductStrict();
private:
    std::string filename = "";
};

