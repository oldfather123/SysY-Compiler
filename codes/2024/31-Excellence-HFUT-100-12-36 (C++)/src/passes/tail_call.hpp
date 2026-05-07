#ifndef PASSES_TAIL_CALL
#define PASSES_TAIL_CALL

#include "../ir/ir.hpp"
#include "pass.hpp"
#include <memory>
namespace Passes {

class TailCall : public Pass {
private:
    std::vector<std::pair<ptr<ir::ir_value>, std::weak_ptr<ir::ir_basicblock>>> work_lst;
    ptr<ir::ir_userfunc> dealing_fun;
public:
    TailCall(ptr<ir::ir_module> compunit) : Pass(compunit) {}
    virtual std::string get_name() override final {return "TAIL_CALL";}
    virtual void run() override final;
    void get_work_lst();
    void create_tail_call();
};

};

#endif