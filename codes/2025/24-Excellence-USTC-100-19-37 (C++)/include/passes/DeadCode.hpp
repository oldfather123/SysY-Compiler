#pragma once

#include "Module.hpp"
#include "PureFuncDect.hpp"

#include <unordered_set>

class DeadCode {
    public:
        DeadCode(Module *m) : m_(m) {
            auto pure_func_dect = std::make_unique<PureFuncDect>(m_);
            pure_func_dect->run();
            this->is_pure_func = pure_func_dect->get_pure_func_map();
        }
        void run();

    private:
        Module *m_;
        // 纯函数信息
        std::unordered_map<Function *, bool> is_pure_func;
        // 工作列表
        std::deque<Instruction *> worklist{};
        std::unordered_map<Instruction *, bool> is_critical_inst{};

        void mark_critical_func(Function *func);
        void mark_critical_inst(Instruction *inst);
        bool is_critical(Instruction *inst);

        bool clear_basic_blocks(Function *func);
        bool clear_func_instructions(Function *func);
        void clear_global_declarations();

};
