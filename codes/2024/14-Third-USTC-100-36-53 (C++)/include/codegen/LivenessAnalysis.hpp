#pragma once
#include "MachinePass.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>


class Register;
class MachineBasicBlock;
class MachineFunction;
class MachineModule;
using RegisterSet = std::unordered_set<std::shared_ptr<Register>>;
using MBBSetMap = std::unordered_map<std::shared_ptr<MachineBasicBlock>, RegisterSet>;

class LivenessAnalysis : public MachinePass {
  public:
    LivenessAnalysis(std::shared_ptr<MachineModule> MM) : MachinePass(MM) {}
    ~LivenessAnalysis() override = default;
    void run_on_func(std::shared_ptr<MachineFunction> func);
    void run() override;
    std::string print();
    RegisterSet get_live_in(std::shared_ptr<MachineBasicBlock> block);
    RegisterSet get_live_out(std::shared_ptr<MachineBasicBlock> block);

  private:
    MBBSetMap live_in;
    MBBSetMap live_out;
};