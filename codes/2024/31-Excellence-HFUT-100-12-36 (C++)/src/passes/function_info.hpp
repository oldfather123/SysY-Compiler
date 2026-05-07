#ifndef PASSES_FUNCTION_INFO
#define PASSES_FUNCTION_INFO

#include "../ir/ir.hpp"
#include "pass.hpp"
#include <queue>
namespace Passes {

class FunctionInfo : public Pass {
    std::list<std::pair<std::string,std::shared_ptr<ir::ir_func>>> func_lst;
    std::queue<ptr<ir::ir_func>> work_lst;
public:
    FunctionInfo(ptr<ir::ir_module> compunit) : Pass(compunit) {}
    virtual std::string get_name() override final {return "FUNCTIONAL_INFO";}
    virtual void run() override final;
    void judge_pure(string name, ptr<ir::ir_userfunc> fun);
    void judge_caller(ptr<ir::ir_func> fun);
};

};

#endif