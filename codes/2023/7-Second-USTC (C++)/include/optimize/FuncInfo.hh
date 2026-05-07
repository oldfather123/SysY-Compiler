#pragma once
#include "Function.hh"
#include "Pass.hh"
#include "logging.hh"

#include <deque>
#include <unordered_map>
#include <unordered_set>

/**
 * 计算哪些函数是纯函数
 * ! 假定所有函数都是纯函数，除非他写入了全局变量、修改了传入的数组、或者直接间接调用了非纯函数
 */

class FuncInfo : public Pass {
  public:
    FuncInfo(Module *m, bool print_ir=false) : Pass(m, print_ir) {}

    void execute();

    bool is_pure_function(Function *func) const {
        //// exit_if(is_pure.find(func) == is_pure.end(), ERROR_IN_PURE_FUNCTION_ANALYSIS);
        return is_pure.at(func);
    }

    void log() {
        for (auto it : is_pure) {
            LOG(INFO) << it.first->get_name() << " is pure? " << it.second;
        }
    }


    const std::string get_name() const override { return name; }

  private:
    //& 有 store 操作的函数非纯函数来处理
    void trivial_mark(Function *func);

    void process(Function *func);

    //& 对局部变量进行 store 没有副作用
    bool is_side_effect_inst(Instruction *inst) ;

    bool is_local_load(LoadInst *inst);

    bool is_local_load(LoadOffsetInst *inst);

    bool is_local_store(StoreInst *inst);

    bool is_local_store(StoreOffsetInst *inst);

    Value *get_first_addr(Value *val);

    std::deque<Function *> worklist;
    std::unordered_map<Function *, bool> is_pure;

    

private:
    std::string name = "FuncInfo";
};
