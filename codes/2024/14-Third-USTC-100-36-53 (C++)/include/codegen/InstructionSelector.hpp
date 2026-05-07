#pragma once
#include "MIBuilder.hpp"
#include "MachinePass.hpp"
#include "exitcode.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>

class MachineModule;
class MIBuilder;

class InstructionSelector : public MachinePass {
  public:
    InstructionSelector(std::shared_ptr<MachineModule> machine_module)
        : MachinePass(machine_module) {}
    ~InstructionSelector() override = default;
    void run() override;

  private:
    void gen_inst();

    void gen_store_params();
    void set_all_regs();
    void gen_prologue_epilogue();

    void gen_ret();
    void gen_br();
    void gen_binary();
    void gen_float_binary();
    void gen_alloca();
    void gen_load();
    void gen_store();
    void gen_icmp();
    void gen_fcmp();
    void gen_zext();
    void gen_call();
    void gen_gep();
    void gen_sitofp();
    void gen_fptosi();
    void gen_bitcast();
    void gen_phi();

    static std::vector<BasicBlock *> compute_block_order(Function *func);

    std::shared_ptr<Register> get_reg(Value *val);
    void set_reg(Value *val, std::shared_ptr<Register> reg);
    struct {
        std::shared_ptr<MachineFunction> machine_func;           // 当前函数
        std::shared_ptr<MachineBasicBlock> machine_bb = nullptr; // 当前指令
        Instruction *IR_inst = nullptr;                          // 当前指令
        std::unordered_map<Constant *, std::shared_ptr<Register>> local_cached_const;

        void clear() {
            machine_func = nullptr;
            machine_bb = nullptr;
            IR_inst = nullptr;
            // val_map.clear();
        }

    } context;

    std::unordered_map<Value *, std::shared_ptr<Register>> val_map{};
    Module *IR_module;
    std::unordered_map<Function *, std::shared_ptr<MachineFunction>> func_map;
    std::unordered_map<BasicBlock *, std::shared_ptr<MachineBasicBlock>> bb_map;
};