#pragma once

#include "Function.hpp"
#include "MachineBasicBlock.hpp"
#include "MachineModule.hpp"
#include "Module.hpp"
#include "Operand.hpp"
#include "Value.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

class FrameScheduler;
class MachineModule;
class MachineBasicBlock;
class PhysicalRegister;
class Register;
class MachineFunction : public std::enable_shared_from_this<MachineFunction> {
  public:
    MachineFunction(Function *function, std::weak_ptr<MachineModule> parent)
        : function(function), parent(parent) {
        frame_scheduler = std::make_unique<FrameScheduler>();
        calc_params_schedule();
    }
    ~MachineFunction() = default;
    void add_basic_block(std::shared_ptr<MachineBasicBlock> bb) {
        basic_blocks.push_back(bb);
    }
    void set_prologue_block(std::shared_ptr<MachineBasicBlock> bb) {
        prologue_block = bb;
    }
    void set_epilogue_block(std::shared_ptr<MachineBasicBlock> bb) {
        epilogue_block = bb;
    }
    std::shared_ptr<MachineBasicBlock> get_prologue_block() const {
        return prologue_block;
    }
    std::shared_ptr<MachineBasicBlock> get_epilogue_block() const {
        return epilogue_block;
    }
    Function *get_IR_function() const { return function; }

    void calc_params_schedule();
    std::string print() const;
    std::string get_name() const;
    const std::vector<std::shared_ptr<MachineBasicBlock>> &
    get_basic_blocks() const {
        return basic_blocks;
    }
    std::vector<std::shared_ptr<MachineBasicBlock>> &get_basic_blocks() {
        return basic_blocks;
    }

    std::unique_ptr<FrameScheduler> frame_scheduler;
    struct params_schedule {
        // on stack or in register
        bool on_stack;
        // offset in stack or register
        int offset;
        std::shared_ptr<PhysicalRegister> reg;
    };
    std::unordered_map<Value *, params_schedule> params_schedule_map;
    int params_size = 0;

  private:
    Function *function;
    std::weak_ptr<MachineModule> parent;
    std::vector<std::shared_ptr<MachineBasicBlock>> basic_blocks;
    std::shared_ptr<MachineBasicBlock> prologue_block;
    std::shared_ptr<MachineBasicBlock> epilogue_block;
};

class FrameScheduler {
  public:
    void insert_alloca(Value *val) {
        auto alloca_inst = static_cast<AllocaInst *>(val);
        frame_size =
            align(frame_size + alloca_inst->get_alloca_type()->get_size(), 16);
        alloca_map[val] = frame_size;
    }
    void insert_reg(std::shared_ptr<Register> reg) {
        frame_size = align(frame_size + 8, 8);
        reg_map[reg] = frame_size;
    }
    int get_alloca_offset(Value *val) { return -alloca_map[val]; }
    int get_reg_offset(std::shared_ptr<Register> reg) { return -reg_map[reg]; }
    int get_frame_size() {
        frame_size = align(frame_size, 16);
        return frame_size;
    }

  private:
    int align(int size, unsigned alignment) {
        return ((size + (alignment - 1)) & ~(alignment - 1));
    }
    int frame_size = 0;
    std::unordered_map<Value *, int> alloca_map;
    std::unordered_map<std::shared_ptr<Register>, int> reg_map;
};
