#ifndef PASSES_PASS_MANAGER
#define PASSES_PASS_MANAGER

#include "../ir/ir.hpp"
#include "pass.hpp"
#include "pass_type.hpp"
#include <memory>

template<typename T>
using ptr = std::shared_ptr<T>;

template <typename T>
using ptr_list = std::vector<ptr<T>>;

namespace Passes{
class PassManager {
    ptr<ir::ir_module> compunit;
    std::vector<std::shared_ptr<Pass>> scheduled_passes;
    ptr<ir::ir_visitor> printer = nullptr;
public:
    PassManager(ptr<ir::ir_module> compunit) : compunit(compunit) {}
    void add_pass(PassType pass_type);
    void run();
    void enable_printer(ptr<ir::ir_visitor> printer) {this->printer = printer;};
};

}

#endif