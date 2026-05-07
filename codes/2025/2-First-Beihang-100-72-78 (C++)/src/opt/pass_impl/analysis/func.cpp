#include <queue>
#include <unordered_set>

#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

std::shared_ptr<ir::Function> main;
std::unordered_map<std::shared_ptr<ir::Function>, std::unordered_set<std::shared_ptr<ir::Function>>> caller_to_callee;
std::vector<std::shared_ptr<ir::Function>> topological_order;
void init(ir::Module *module);
void travel(ir::Module *module);
void remove_unused_functions(ir::Module *module);
void analyze(const std::shared_ptr<ir::Function> &function);
void get_topological_order(std::shared_ptr<ir::Function> root,
                           std::unordered_set<std::shared_ptr<ir::Function>> &visited);
void inherit();
void clean_up();

bool FunctionAnalyzation::run(ir::Module *module) {
    init(module);
    travel(module);
    remove_unused_functions(module);
    module->for_each_func([](const auto &function) { analyze(function); });
    inherit();
    clean_up();
    return false;
}

void init(ir::Module *module) {
    for (const auto &function : module->get_all_functions()) {
        function->init_func_analyzation_info();
        if (function->is_main()) {
            main = function;
        }
        if (ir::Function::is_lib(function)) {
            function->opt_info.has_inout = true;
        }
    }
}

void travel(ir::Module *module) {
    for (const auto &function : module->get_all_functions()) {
        for (const auto &user : function->get_users_ref()) {
            if (auto call = std::dynamic_pointer_cast<ir::Call>(user.lock())) {
                auto caller = call->get_parent_block().lock()->get_parent_func().lock();
                caller_to_callee[caller].insert(function);
            }
        }
    }
}

void remove_unused_functions(ir::Module *module) {
    // bfs
    std::unordered_set<std::shared_ptr<ir::Value>> visited;
    std::queue<std::shared_ptr<ir::Function>> queue;
    queue.emplace(main);
    while (!queue.empty()) {
        auto function = queue.front();
        queue.pop();
        visited.emplace(function);
        for (const auto &callee : caller_to_callee[function]) {
            if (visited.find(callee) == visited.end()) {
                queue.emplace(callee);
            }
        }
    }

    // remove unused functions
    std::vector<std::shared_ptr<ir::Function>> to_remove;
    module->for_each_func([&](const auto &function) {
        if (visited.find(function) == visited.end()) {
            to_remove.push_back(function);
        }
    });
    for (const auto &function : to_remove) {
        util::remove_function(module, function);
    }
}

void analyze(const std::shared_ptr<ir::Function> &function) {
    function->for_each_block([function](const auto &basic_block) {
        basic_block->for_each_instruction([function](const auto &instruction) {
            if (instruction->get_ins_type() == ir::Instruction::InstructionType::LOAD) {
                if (util::is_global(instruction->get_operands_ref().front())) {
                    function->opt_info.has_global_memory_read = true;
                }
            } else if (instruction->get_ins_type() == ir::Instruction::InstructionType::STORE) {
                if (util::is_global(instruction->get_operands_ref().back())) {
                    function->opt_info.has_global_memory_write = true;
                } else if (util::is_local(instruction->get_operands_ref().back(), function)) {
                    function->opt_info.has_local_memory_write = true;
                }
            }
        });
    });
}

void get_topological_order(std::shared_ptr<ir::Function> root,
                           std::unordered_set<std::shared_ptr<ir::Function>> &visited) {
    visited.insert(root);
    for (const auto &callee : caller_to_callee[root]) {
        if (visited.find(callee) == visited.end()) {
            get_topological_order(callee, visited);
        }
    }
    topological_order.push_back(root);
}

void inherit() {
    std::unordered_set<std::shared_ptr<ir::Function>> visited;
    get_topological_order(main, visited);
    for (const auto &caller : topological_order) {
        for (const auto &callee : caller_to_callee[caller]) {
            caller->opt_info.has_global_memory_read |= callee->opt_info.has_global_memory_read;
            caller->opt_info.has_global_memory_write |= callee->opt_info.has_global_memory_write;
            caller->opt_info.has_inout |= callee->opt_info.has_inout;
        }
    }
}

void clean_up() {
    main = nullptr;
    caller_to_callee.clear();
    topological_order.clear();
}

}  // namespace opt::pass
